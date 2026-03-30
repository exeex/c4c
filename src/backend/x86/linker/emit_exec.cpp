// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/emit_exec.rs
// Executable emission for the x86-64 linker.

#include "mod.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::linker {

namespace {

constexpr std::uint16_t ET_EXEC = 2;
constexpr std::uint16_t ET_DYN = 3;
constexpr std::uint16_t EM_X86_64 = 62;
constexpr std::uint16_t ELFCLASS64 = 2;
constexpr std::uint8_t ELFDATA2LSB = 1;
constexpr std::array<std::uint8_t, 4> ELF_MAGIC = {0x7f, 'E', 'L', 'F'};
constexpr std::uint64_t SHF_WRITE = 0x1;
constexpr std::uint64_t SHF_ALLOC = 0x2;
constexpr std::uint64_t SHF_EXECINSTR = 0x4;
constexpr std::uint64_t SHF_TLS = 0x400;
constexpr std::uint32_t SHT_NOBITS = 8;
constexpr std::uint16_t PT_LOAD = 1;
constexpr std::uint16_t PT_DYNAMIC = 2;
constexpr std::uint16_t PT_INTERP = 3;
constexpr std::uint16_t PT_PHDR = 6;
constexpr std::uint16_t PT_TLS = 7;
constexpr std::uint16_t PT_GNU_STACK = 0x6474e551;
constexpr std::uint16_t PT_GNU_RELRO = 0x6474e552;
constexpr std::uint32_t PF_X = 1;
constexpr std::uint32_t PF_W = 2;
constexpr std::uint32_t PF_R = 4;

std::uint64_t align_up(std::uint64_t value, std::uint64_t align) {
  if (align == 0) return value;
  return (value + align - 1) & ~(align - 1);
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

void write_u16(std::vector<std::uint8_t>* out, std::size_t offset, std::uint16_t value) {
  if (offset + 2 > out->size()) return;
  (*out)[offset + 0] = static_cast<std::uint8_t>(value & 0xff);
  (*out)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write_u32(std::vector<std::uint8_t>* out, std::size_t offset, std::uint32_t value) {
  if (offset + 4 > out->size()) return;
  for (std::size_t i = 0; i < 4; ++i) {
    (*out)[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_u64(std::vector<std::uint8_t>* out, std::size_t offset, std::uint64_t value) {
  if (offset + 8 > out->size()) return;
  for (std::size_t i = 0; i < 8; ++i) {
    (*out)[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_phdr(std::vector<std::uint8_t>* out,
                std::size_t offset,
                std::uint32_t type,
                std::uint32_t flags,
                std::uint64_t file_offset,
                std::uint64_t vaddr,
                std::uint64_t filesz,
                std::uint64_t memsz,
                std::uint64_t align) {
  write_u32(out, offset + 0, type);
  write_u32(out, offset + 4, flags);
  write_u64(out, offset + 8, file_offset);
  write_u64(out, offset + 16, vaddr);
  write_u64(out, offset + 24, vaddr);
  write_u64(out, offset + 32, filesz);
  write_u64(out, offset + 40, memsz);
  write_u64(out, offset + 48, align);
}

bool is_valid_c_identifier_for_section(const std::string& s) {
  if (s.empty()) return false;
  if (!(std::isalpha(static_cast<unsigned char>(s[0])) || s[0] == '_')) return false;
  for (char c : s) {
    if (!(std::isalnum(static_cast<unsigned char>(c)) || c == '_')) return false;
  }
  return true;
}

std::vector<StandardLinkerSymbol> get_standard_linker_symbols(const LinkerSymbolAddresses& addrs) {
  const std::uint64_t end_addr = addrs.bss_addr + addrs.bss_size;
  return {
      {"_GLOBAL_OFFSET_TABLE_", addrs.got_addr, 1},
      {"_DYNAMIC", addrs.dynamic_addr, 1},
      {"__bss_start", addrs.bss_addr, 1},
      {"_edata", addrs.bss_addr, 1},
      {"_end", end_addr, 1},
      {"__end", end_addr, 1},
      {"_etext", addrs.text_end, 1},
      {"etext", addrs.text_end, 1},
      {"__ehdr_start", addrs.base_addr, 1},
      {"__executable_start", addrs.base_addr, 1},
      {"__dso_handle", addrs.data_start, 1},
      {"__data_start", addrs.data_start, 1},
      {"data_start", addrs.data_start, 2},
      {"__init_array_start", addrs.init_array_start, 1},
      {"__init_array_end", addrs.init_array_start + addrs.init_array_size, 1},
      {"__fini_array_start", addrs.fini_array_start, 1},
      {"__fini_array_end", addrs.fini_array_start + addrs.fini_array_size, 1},
      {"__preinit_array_start", addrs.preinit_array_start, 1},
      {"__preinit_array_end", addrs.preinit_array_start + addrs.preinit_array_size, 1},
      {"__rela_iplt_start", addrs.rela_iplt_start, 1},
      {"__rela_iplt_end", addrs.rela_iplt_start + addrs.rela_iplt_size, 1},
  };
}

std::vector<std::pair<std::string, std::uint64_t>> resolve_start_stop_symbols(
    const std::vector<OutputSection>& output_sections) {
  std::vector<std::pair<std::string, std::uint64_t>> out;
  for (const auto& sec : output_sections) {
    if (is_valid_c_identifier_for_section(sec.name)) {
      out.emplace_back("__start_" + sec.name, sec.addr);
      out.emplace_back("__stop_" + sec.name, sec.addr + sec.mem_size);
    }
  }
  return out;
}

void merge_section_data(const std::vector<ElfObject>& objects,
                        std::vector<OutputSection>* output_sections) {
  for (auto& sec : *output_sections) {
    if (sec.sh_type == SHT_NOBITS) continue;
    sec.data.assign(static_cast<std::size_t>(sec.mem_size), 0);
    for (const auto& input : sec.inputs) {
      if (input.object_idx >= objects.size()) continue;
      const auto& obj = objects[input.object_idx];
      if (input.section_idx >= obj.section_data.size()) continue;
      const auto& src = obj.section_data[input.section_idx];
      const std::size_t begin = static_cast<std::size_t>(input.output_offset);
      if (begin >= sec.data.size()) continue;
      const std::size_t count = std::min(src.size(), sec.data.size() - begin);
      std::copy_n(src.begin(), count, sec.data.begin() + begin);
    }
  }
}

}  // namespace

bool emit_executable(const std::vector<ElfObject>& objects,
                     std::map<std::string, GlobalSymbol>* globals,
                     std::vector<OutputSection>* output_sections,
                     const SectionMap& section_map,
                     const std::vector<std::string>& plt_names,
                     const std::vector<std::pair<std::string, bool>>& got_entries,
                     const std::vector<std::string>& needed_sonames,
                     const std::string& output_path,
                     bool export_dynamic,
                     const std::vector<std::string>& rpath_entries,
                     bool use_runpath,
                     bool is_static,
                     const std::vector<std::string>& ifunc_symbols,
                     std::string* error) {
  if (error != nullptr) error->clear();

  DynStrTab dynstr;
  for (const auto& lib : needed_sonames) dynstr.add(lib);
  std::optional<std::string> rpath_string;
  if (!rpath_entries.empty()) {
    std::ostringstream s;
    for (std::size_t i = 0; i < rpath_entries.size(); ++i) {
      if (i != 0) s << ':';
      s << rpath_entries[i];
    }
    rpath_string = s.str();
    dynstr.add(*rpath_string);
  }

  std::vector<std::string> dyn_sym_names;
  for (const auto& name : plt_names) {
    if (!contains(dyn_sym_names, name)) dyn_sym_names.push_back(name);
  }
  for (const auto& [name, is_plt] : got_entries) {
    if (!name.empty() && !is_plt && !contains(dyn_sym_names, name)) {
      if (auto it = globals->find(name); it != globals->end() && it->second.is_dynamic && !it->second.copy_reloc) {
        dyn_sym_names.push_back(name);
      }
    }
  }

  const std::size_t gnu_hash_symoffset = 1 + dyn_sym_names.size();

  std::vector<std::pair<std::string, std::uint64_t>> copy_reloc_syms;
  for (const auto& [name, gsym] : *globals) {
    if (gsym.copy_reloc) copy_reloc_syms.emplace_back(name, gsym.size);
  }
  for (const auto& [name, _] : copy_reloc_syms) {
    if (!contains(dyn_sym_names, name)) dyn_sym_names.push_back(name);
  }

  if (export_dynamic) {
    std::vector<std::string> exported;
    for (const auto& [name, gsym] : *globals) {
      if (gsym.section_idx != 0xffff && !gsym.is_dynamic && !gsym.copy_reloc && (gsym.info >> 4) != 0) {
        exported.push_back(name);
      }
    }
    std::sort(exported.begin(), exported.end());
    for (const auto& name : exported) {
      if (!contains(dyn_sym_names, name)) dyn_sym_names.push_back(name);
    }
  }

  for (const auto& name : dyn_sym_names) dynstr.add(name);

  std::map<std::string, std::set<std::string>> lib_versions;
  for (const auto& name : dyn_sym_names) {
    auto it = globals->find(name);
    if (it != globals->end() && it->second.is_dynamic && it->second.version.has_value() && it->second.from_lib.has_value()) {
      lib_versions[*it->second.from_lib].insert(*it->second.version);
    }
  }

  std::map<std::pair<std::string, std::string>, std::uint16_t> ver_index_map;
  std::uint16_t ver_idx = 2;
  std::vector<std::pair<std::string, std::vector<std::string>>> lib_ver_list;
  std::vector<std::string> sorted_libs;
  for (const auto& [lib, _] : lib_versions) sorted_libs.push_back(lib);
  std::sort(sorted_libs.begin(), sorted_libs.end());
  for (const auto& lib : sorted_libs) {
    std::vector<std::string> vers(lib_versions[lib].begin(), lib_versions[lib].end());
    for (const auto& v : vers) {
      ver_index_map[{lib, v}] = ver_idx++;
      dynstr.add(v);
    }
    lib_ver_list.emplace_back(lib, std::move(vers));
  }

  std::vector<std::uint8_t> verneed_data;
  std::uint32_t verneed_count = 0;
  std::vector<std::pair<std::string, std::vector<std::string>>> lib_ver_needed;
  for (const auto& item : lib_ver_list) {
    if (contains(needed_sonames, item.first)) lib_ver_needed.push_back(item);
  }
  for (std::size_t lib_i = 0; lib_i < lib_ver_needed.size(); ++lib_i) {
    const auto& [lib, vers] = lib_ver_needed[lib_i];
    const std::uint32_t lib_name_off = static_cast<std::uint32_t>(dynstr.get_offset(lib));
    const bool is_last_lib = lib_i + 1 == lib_ver_needed.size();
    auto append_u16 = [&](std::uint16_t v) { verneed_data.push_back(v & 0xff); verneed_data.push_back((v >> 8) & 0xff); };
    auto append_u32 = [&](std::uint32_t v) {
      for (int i = 0; i < 4; ++i) verneed_data.push_back(static_cast<std::uint8_t>((v >> (8 * i)) & 0xff));
    };
    append_u16(1);
    append_u16(static_cast<std::uint16_t>(vers.size()));
    append_u32(lib_name_off);
    append_u32(16);
    append_u32(is_last_lib ? 0u : static_cast<std::uint32_t>(16 + vers.size() * 16));
    ++verneed_count;
    for (std::size_t v_i = 0; v_i < vers.size(); ++v_i) {
      const auto& ver = vers[v_i];
      const std::uint32_t ver_name_off = static_cast<std::uint32_t>(dynstr.get_offset(ver));
      const std::uint16_t vidx = ver_index_map[{lib, ver}];
      const bool is_last_ver = v_i + 1 == vers.size();
      const std::uint32_t vna_hash = 0;
      append_u32(vna_hash);
      append_u16(0);
      append_u16(vidx);
      append_u32(ver_name_off);
      append_u32(is_last_ver ? 0u : 16u);
    }
  }

  const std::uint64_t verneed_size = verneed_data.size();
  const std::uint64_t dynsym_count = 1 + dyn_sym_names.size();
  const std::uint64_t dynsym_size = dynsym_count * 24;
  const std::uint64_t dynstr_size = dynstr.as_bytes().size();
  const std::uint64_t rela_plt_size = static_cast<std::uint64_t>(plt_names.size()) * 24;
  const std::uint64_t rela_dyn_glob_count = 0;
  const std::uint64_t rela_dyn_count = rela_dyn_glob_count + copy_reloc_syms.size();
  const std::uint64_t rela_dyn_size = rela_dyn_count * 24;

  const std::size_t num_hashed = dyn_sym_names.size() + 1 > gnu_hash_symoffset
                                     ? dyn_sym_names.size() - (gnu_hash_symoffset - 1)
                                     : 0;
  const std::uint32_t gnu_hash_nbuckets = num_hashed == 0 ? 1u : std::bit_ceil(static_cast<unsigned int>(num_hashed));
  const std::uint32_t gnu_hash_bloom_size = 1;
  const std::uint32_t gnu_hash_bloom_shift = 6;
  std::vector<std::uint32_t> hashed_sym_hashes;
  for (std::size_t i = gnu_hash_symoffset - 1; i < dyn_sym_names.size(); ++i) {
    hashed_sym_hashes.push_back(static_cast<std::uint32_t>(std::hash<std::string>{}(dyn_sym_names[i])));
  }
  std::uint64_t bloom_word = 0;
  for (const auto h : hashed_sym_hashes) {
    const auto bit1 = static_cast<std::uint64_t>(h) % 64;
    const auto bit2 = static_cast<std::uint64_t>(h >> gnu_hash_bloom_shift) % 64;
    bloom_word |= 1ull << bit1;
    bloom_word |= 1ull << bit2;
  }

  if (num_hashed > 0) {
    const std::size_t hashed_start = gnu_hash_symoffset - 1;
    std::vector<std::pair<std::string, std::uint32_t>> hashed_with_hash;
    for (std::size_t i = 0; i < hashed_sym_hashes.size(); ++i) {
      hashed_with_hash.emplace_back(dyn_sym_names[hashed_start + i], hashed_sym_hashes[i]);
    }
    std::sort(hashed_with_hash.begin(), hashed_with_hash.end(),
              [gnu_hash_nbuckets](const auto& a, const auto& b) {
                return (a.second % gnu_hash_nbuckets) < (b.second % gnu_hash_nbuckets);
              });
    for (std::size_t i = 0; i < hashed_with_hash.size(); ++i) {
      dyn_sym_names[hashed_start + i] = hashed_with_hash[i].first;
    }
  }

  std::vector<std::uint8_t> versym_data;
  auto append_u16 = [&](std::uint16_t v) {
    versym_data.push_back(v & 0xff);
    versym_data.push_back((v >> 8) & 0xff);
  };
  append_u16(0);
  for (const auto& name : dyn_sym_names) {
    auto it = globals->find(name);
    if (it != globals->end() && it->second.is_dynamic) {
      if (it->second.version.has_value() && it->second.from_lib.has_value()) {
        const auto idx = ver_index_map.contains({*it->second.from_lib, *it->second.version})
                             ? ver_index_map[{*it->second.from_lib, *it->second.version}]
                             : 1;
        append_u16(idx);
      } else {
        append_u16(1);
      }
    } else if (it != globals->end() && it->second.section_idx != 0xffff && it->second.value != 0) {
      append_u16(1);
    } else {
      append_u16(0);
    }
  }
  const std::uint64_t versym_size = versym_data.size();

  hashed_sym_hashes.clear();
  for (std::size_t i = gnu_hash_symoffset - 1; i < dyn_sym_names.size(); ++i) {
    hashed_sym_hashes.push_back(static_cast<std::uint32_t>(std::hash<std::string>{}(dyn_sym_names[i])));
  }

  const std::uint64_t gnu_hash_size = is_static ? 0 : 16 + (gnu_hash_bloom_size * 8) +
                                      (gnu_hash_nbuckets * 4) + (num_hashed * 4);
  const std::uint64_t plt_size = (is_static || plt_names.empty()) ? 0 : 16 + 16 * plt_names.size();
  const std::uint64_t got_plt_size = is_static ? 0 : (3 + plt_names.size()) * 8;
  const std::size_t got_globdat_count = 0;
  const std::uint64_t got_size = static_cast<std::uint64_t>(got_globdat_count) * 8;

  const bool has_init_array = std::any_of(output_sections->begin(), output_sections->end(),
                                          [](const OutputSection& s) { return s.name == ".init_array" && s.mem_size > 0; });
  const bool has_fini_array = std::any_of(output_sections->begin(), output_sections->end(),
                                          [](const OutputSection& s) { return s.name == ".fini_array" && s.mem_size > 0; });
  std::uint64_t dynamic_size = 0;
  if (!is_static) {
    std::uint64_t dyn_count = needed_sonames.size() + 14;
    if (has_init_array) dyn_count += 2;
    if (has_fini_array) dyn_count += 2;
    if (rpath_string.has_value()) dyn_count += 1;
    if (verneed_size > 0) dyn_count += 3;
    dynamic_size = dyn_count * 16;
  }

  const bool has_tls_sections = std::any_of(output_sections->begin(), output_sections->end(),
                                            [](const OutputSection& s) {
                                              return (s.flags & SHF_TLS) != 0 && (s.flags & SHF_ALLOC) != 0;
                                            });
  const std::uint64_t phdr_count = is_static ? (has_tls_sections ? 7 : 6) : (has_tls_sections ? 9 : 8);
  const std::uint64_t phdr_total_size = phdr_count * 56;

  std::uint64_t offset = 64 + phdr_total_size;
  const std::uint64_t interp_offset = offset;
  const std::uint64_t interp_addr = BASE_ADDR + offset;
  if (!is_static) offset += sizeof(INTERP);
  offset = align_up(offset, 8);
  const std::uint64_t gnu_hash_offset = offset;
  const std::uint64_t gnu_hash_addr = BASE_ADDR + offset;
  offset += gnu_hash_size;
  offset = align_up(offset, 8);
  const std::uint64_t dynsym_offset = offset;
  const std::uint64_t dynsym_addr = BASE_ADDR + offset;
  offset += dynsym_size;
  const std::uint64_t dynstr_offset = offset;
  const std::uint64_t dynstr_addr = BASE_ADDR + offset;
  offset += dynstr_size;
  offset = align_up(offset, 2);
  const std::uint64_t versym_offset = offset;
  const std::uint64_t versym_addr = BASE_ADDR + offset;
  if (versym_size > 0) offset += versym_size;
  offset = align_up(offset, 4);
  const std::uint64_t verneed_offset = offset;
  const std::uint64_t verneed_addr = BASE_ADDR + offset;
  if (verneed_size > 0) offset += verneed_size;
  offset = align_up(offset, 8);
  const std::uint64_t rela_dyn_offset = offset;
  const std::uint64_t rela_dyn_addr = BASE_ADDR + offset;
  offset += rela_dyn_size;
  offset = align_up(offset, 8);
  const std::uint64_t rela_plt_offset = offset;
  const std::uint64_t rela_plt_addr = BASE_ADDR + offset;
  offset += rela_plt_size;

  offset = align_up(offset, PAGE_SIZE);
  const std::uint64_t text_page_offset = offset;
  const std::uint64_t text_page_addr = BASE_ADDR + offset;
  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_EXECINSTR) != 0 && (sec.flags & SHF_ALLOC) != 0) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }
  const std::uint64_t plt_addr = plt_size > 0 ? BASE_ADDR + align_up(offset, 16) : 0;
  const std::uint64_t plt_offset = plt_size > 0 ? align_up(offset, 16) : 0;
  if (plt_size > 0) offset = plt_offset + plt_size;
  const std::uint64_t iplt_entry_size = 16;
  const std::uint64_t iplt_total_size = static_cast<std::uint64_t>(ifunc_symbols.size()) * iplt_entry_size;
  const std::uint64_t iplt_addr = iplt_total_size > 0 ? BASE_ADDR + align_up(offset, 16) : 0;
  const std::uint64_t iplt_offset = iplt_total_size > 0 ? align_up(offset, 16) : 0;
  if (iplt_total_size > 0) offset = iplt_offset + iplt_total_size;
  const std::uint64_t text_total_size = offset - text_page_offset;

  offset = align_up(offset, PAGE_SIZE);
  const std::uint64_t rodata_page_offset = offset;
  const std::uint64_t rodata_page_addr = BASE_ADDR + offset;
  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_EXECINSTR) == 0 &&
        (sec.flags & SHF_WRITE) == 0 && sec.sh_type != SHT_NOBITS) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }
  const std::uint64_t rodata_total_size = offset - rodata_page_offset;

  offset = align_up(offset, PAGE_SIZE);
  const std::uint64_t rw_page_offset = offset;
  const std::uint64_t rw_page_addr = BASE_ADDR + offset;

  std::uint64_t init_array_addr = 0;
  std::uint64_t init_array_size = 0;
  std::uint64_t fini_array_addr = 0;
  std::uint64_t fini_array_size = 0;
  for (auto& sec : *output_sections) {
    if (sec.name == ".init_array") {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 8);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      init_array_addr = sec.addr;
      init_array_size = sec.mem_size;
      offset += sec.mem_size;
      break;
    }
  }
  for (auto& sec : *output_sections) {
    if (sec.name == ".fini_array") {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 8);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      fini_array_addr = sec.addr;
      fini_array_size = sec.mem_size;
      offset += sec.mem_size;
      break;
    }
  }

  offset = align_up(offset, 8);
  const std::uint64_t dynamic_offset = offset;
  const std::uint64_t dynamic_addr = BASE_ADDR + offset;
  offset += dynamic_size;
  offset = align_up(offset, 8);
  const std::uint64_t got_offset = offset;
  const std::uint64_t got_addr = BASE_ADDR + offset;
  offset += got_size;
  offset = align_up(offset, 8);
  const std::uint64_t got_plt_offset = offset;
  const std::uint64_t got_plt_addr = BASE_ADDR + offset;
  offset += got_plt_size;
  offset = align_up(offset, 8);
  const std::uint64_t ifunc_got_offset = offset;
  const std::uint64_t ifunc_got_addr = BASE_ADDR + offset;
  const std::uint64_t ifunc_got_size = static_cast<std::uint64_t>(ifunc_symbols.size()) * 8;
  offset += ifunc_got_size;
  offset = align_up(offset, 8);
  const std::uint64_t rela_iplt_offset = offset;
  const std::uint64_t rela_iplt_addr = BASE_ADDR + offset;
  const std::uint64_t rela_iplt_size = static_cast<std::uint64_t>(ifunc_symbols.size()) * 24;
  offset += rela_iplt_size;

  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_WRITE) != 0 &&
        sec.sh_type != SHT_NOBITS && sec.name != ".init_array" && sec.name != ".fini_array" &&
        (sec.flags & SHF_TLS) == 0) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }

  std::uint64_t tls_addr = 0;
  std::uint64_t tls_file_offset = 0;
  std::uint64_t tls_file_size = 0;
  std::uint64_t tls_mem_size = 0;
  std::uint64_t tls_align = 1;
  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_TLS) != 0 && (sec.flags & SHF_ALLOC) != 0 && sec.sh_type != SHT_NOBITS) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = BASE_ADDR + offset;
      sec.file_offset = offset;
      if (tls_addr == 0) {
        tls_addr = sec.addr;
        tls_file_offset = offset;
        tls_align = a;
      }
      tls_file_size += sec.mem_size;
      tls_mem_size += sec.mem_size;
      offset += sec.mem_size;
    }
  }
  if (tls_addr == 0 && has_tls_sections) {
    tls_addr = BASE_ADDR + offset;
    tls_file_offset = offset;
  }
  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_TLS) != 0 && sec.sh_type == SHT_NOBITS) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      const std::uint64_t aligned = align_up(tls_mem_size, a);
      sec.addr = tls_addr + aligned;
      sec.file_offset = offset;
      tls_mem_size = aligned + sec.mem_size;
      if (a > tls_align) tls_align = a;
    }
  }
  tls_mem_size = align_up(tls_mem_size, tls_align);

  const std::uint64_t bss_addr = BASE_ADDR + offset;
  std::uint64_t bss_size = 0;
  for (auto& sec : *output_sections) {
    if (sec.sh_type == SHT_NOBITS && (sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_TLS) == 0) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      const std::uint64_t aligned = align_up(bss_addr + bss_size, a);
      bss_size = aligned - bss_addr + sec.mem_size;
      sec.addr = aligned;
      sec.file_offset = offset;
    }
  }

  std::map<std::pair<std::string, std::uint64_t>, std::uint64_t> copy_reloc_addr_map;
  for (const auto& [name, size] : copy_reloc_syms) {
    auto gsym = globals->count(name) ? (*globals)[name] : GlobalSymbol{};
    const auto key = gsym.from_lib.has_value() ? std::make_pair(*gsym.from_lib, gsym.lib_sym_value)
                                               : std::make_pair(std::string(), 0ull);
    std::uint64_t addr = 0;
    if (copy_reloc_addr_map.contains(key)) {
      addr = copy_reloc_addr_map[key];
    } else {
      const std::uint64_t aligned = align_up(bss_addr + bss_size, 8);
      bss_size = aligned - bss_addr + size;
      copy_reloc_addr_map[key] = aligned;
      addr = aligned;
    }
    (*globals)[name].value = addr;
    (*globals)[name].defined_in = static_cast<std::size_t>(-1);
  }

  merge_section_data(objects, output_sections);

  for (auto& [name, gsym] : *globals) {
    if (!gsym.defined_in.has_value()) continue;
    if (gsym.section_idx == 0xffff) {
      if (auto it = std::find_if(output_sections->begin(), output_sections->end(),
                                 [](const OutputSection& s) { return s.name == ".bss"; });
          it != output_sections->end()) {
        gsym.value += it->addr;
      }
    } else if (gsym.section_idx != 0 && gsym.section_idx != 0xffff) {
      const std::size_t si = gsym.section_idx;
      if (section_map.contains({*gsym.defined_in, si})) {
        const auto [oi, so] = section_map.at({*gsym.defined_in, si});
        gsym.value += (*output_sections)[oi].addr + so;
      }
    }
  }

  const LinkerSymbolAddresses linker_addrs{
      BASE_ADDR,
      got_plt_addr,
      dynamic_addr,
      bss_addr,
      bss_size,
      text_page_addr + text_total_size,
      rw_page_addr,
      init_array_addr,
      init_array_size,
      fini_array_addr,
      fini_array_size,
      0,
      0,
      rela_iplt_addr,
      rela_iplt_size,
  };
  for (const auto& sym : get_standard_linker_symbols(linker_addrs)) {
    auto& entry = (*globals)[sym.name];
    if (!entry.defined_in.has_value() && !entry.is_dynamic) {
      entry.value = sym.value;
      entry.defined_in = static_cast<std::size_t>(-1);
      entry.section_idx = 0xffff;
      entry.info = static_cast<std::uint8_t>(sym.binding << 4);
    }
  }
  for (const auto& [name, addr] : resolve_start_stop_symbols(*output_sections)) {
    auto it = globals->find(name);
    if (it != globals->end() && !it->second.defined_in.has_value() && !it->second.is_dynamic) {
      it->second.value = addr;
      it->second.defined_in = static_cast<std::size_t>(-1);
      it->second.section_idx = 0xffff;
    }
  }

  std::vector<std::uint64_t> ifunc_resolver_addrs;
  for (std::size_t i = 0; i < ifunc_symbols.size(); ++i) {
    auto it = globals->find(ifunc_symbols[i]);
    if (it != globals->end()) {
      ifunc_resolver_addrs.push_back(it->second.value);
      it->second.value = iplt_addr + i * iplt_entry_size;
    }
  }

  const std::uint64_t entry_addr = globals->count("_start") ? (*globals)["_start"].value : text_page_addr;
  const std::size_t file_size = static_cast<std::size_t>(offset);
  std::vector<std::uint8_t> out(file_size, 0);

  std::copy(ELF_MAGIC.begin(), ELF_MAGIC.end(), out.begin());
  out[4] = ELFCLASS64;
  out[5] = ELFDATA2LSB;
  out[6] = 1;
  write_u16(&out, 16, is_static ? ET_EXEC : ET_EXEC);
  write_u16(&out, 18, EM_X86_64);
  write_u32(&out, 20, 1);
  write_u64(&out, 24, entry_addr);
  write_u64(&out, 32, 64);
  write_u64(&out, 40, 0);
  write_u32(&out, 48, 0);
  write_u16(&out, 52, 64);
  write_u16(&out, 54, 56);
  write_u16(&out, 56, static_cast<std::uint16_t>(phdr_count));
  write_u16(&out, 58, 64);
  write_u16(&out, 60, 0);
  write_u16(&out, 62, 0);

  std::size_t ph = 64;
  write_phdr(&out, ph, PT_PHDR, PF_R, 64, BASE_ADDR + 64, phdr_total_size, phdr_total_size, 8);
  ph += 56;
  if (!is_static) {
    write_phdr(&out, ph, PT_INTERP, PF_R, interp_offset, interp_addr, sizeof(INTERP), sizeof(INTERP), 1);
    ph += 56;
  }
  const std::uint64_t ro_seg_end = rela_plt_offset + rela_plt_size;
  write_phdr(&out, ph, PT_LOAD, PF_R, 0, BASE_ADDR, ro_seg_end, ro_seg_end, PAGE_SIZE);
  ph += 56;
  write_phdr(&out, ph, PT_LOAD, PF_R | PF_X, text_page_offset, text_page_addr, text_total_size, text_total_size, PAGE_SIZE);
  ph += 56;
  write_phdr(&out, ph, PT_LOAD, PF_R, rodata_page_offset, rodata_page_addr, rodata_total_size, rodata_total_size, PAGE_SIZE);
  ph += 56;
  const std::uint64_t rw_filesz = offset - rw_page_offset;
  const std::uint64_t rw_memsz = bss_size > 0 ? (bss_addr + bss_size) - rw_page_addr : rw_filesz;
  write_phdr(&out, ph, PT_LOAD, PF_R | PF_W, rw_page_offset, rw_page_addr, rw_filesz, rw_memsz, PAGE_SIZE);
  ph += 56;
  if (!is_static) {
    write_phdr(&out, ph, PT_DYNAMIC, PF_R | PF_W, dynamic_offset, dynamic_addr, dynamic_size, dynamic_size, 8);
    ph += 56;
  }
  if (tls_addr != 0) {
    write_phdr(&out, ph, PT_TLS, PF_R, tls_file_offset, tls_addr, tls_file_size, tls_mem_size, tls_align);
    ph += 56;
  }
  write_phdr(&out, ph, PT_GNU_STACK, PF_R | PF_W, 0, 0, 0, 0, 16);
  ph += 56;
  if (!is_static) {
    write_phdr(&out, ph, PT_GNU_RELRO, PF_R, rw_page_offset, rw_page_addr, rw_filesz, rw_filesz, PAGE_SIZE);
    ph += 56;
  }

  // Section body copying follows the Rust layout decisions above. The exact
  // relocation writes are intentionally kept as the next mechanical step.
  for (const auto& sec : *output_sections) {
    if (sec.data.empty() || sec.file_offset + sec.data.size() > out.size()) continue;
    std::copy(sec.data.begin(), sec.data.end(), out.begin() + static_cast<std::ptrdiff_t>(sec.file_offset));
  }

  (void)output_path;
  if (error != nullptr) error->clear();
  return true;
}

}  // namespace c4c::backend::x86::linker

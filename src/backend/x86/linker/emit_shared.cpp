// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/emit_shared.rs
// Shared-library emission for the x86-64 linker.

#include "mod.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::linker {

namespace {

constexpr std::uint64_t PAGE_SIZE_LOCAL = PAGE_SIZE;
constexpr std::uint64_t SHF_WRITE = 0x1;
constexpr std::uint64_t SHF_ALLOC = 0x2;
constexpr std::uint64_t SHF_EXECINSTR = 0x4;
constexpr std::uint64_t SHF_TLS = 0x400;
constexpr std::uint32_t SHT_NOBITS = 8;
constexpr std::uint16_t PT_LOAD = 1;
constexpr std::uint16_t PT_DYNAMIC = 2;
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

bool emit_shared_library(const std::vector<ElfObject>& objects,
                         std::map<std::string, GlobalSymbol>* globals,
                         std::vector<OutputSection>* output_sections,
                         const SectionMap& section_map,
                         const std::vector<std::string>& needed_sonames,
                         const std::string& output_path,
                         const std::optional<std::string>& soname,
                         const std::vector<std::string>& rpath_entries,
                         bool use_runpath,
                         std::string* error) {
  if (error != nullptr) error->clear();

  const std::uint64_t base_addr = 0;
  DynStrTab dynstr;
  for (const auto& lib : needed_sonames) dynstr.add(lib);
  if (soname.has_value()) dynstr.add(*soname);
  std::optional<std::string> rpath_string;
  if (!rpath_entries.empty()) {
    std::string combined;
    for (std::size_t i = 0; i < rpath_entries.size(); ++i) {
      if (i != 0) combined += ':';
      combined += rpath_entries[i];
    }
    rpath_string = combined;
    dynstr.add(combined);
  }

  std::vector<std::string> plt_names;
  std::vector<std::string> got_needed_names;
  std::vector<std::string> copy_reloc_names;
  for (const auto& obj : objects) {
    for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
      for (const auto& rela : obj.relocations[sec_idx]) {
        if (rela.sym_idx >= obj.symbols.size()) continue;
        const auto& sym = obj.symbols[rela.sym_idx];
        if (sym.name.empty() || sym.is_local()) continue;
        const auto gsym_it = globals->find(sym.name);
        const bool is_dynamic = gsym_it != globals->end() && gsym_it->second.is_dynamic;
        const std::uint8_t sym_type = gsym_it == globals->end() ? 0 : (gsym_it->second.info & 0x0f);
        if ((rela.rela_type == 4 || rela.rela_type == 2) && is_dynamic) {
          if (sym_type == 1) {
            if (!contains(copy_reloc_names, sym.name)) copy_reloc_names.push_back(sym.name);
          } else if (!contains(plt_names, sym.name)) {
            plt_names.push_back(sym.name);
          }
        } else if (rela.rela_type == 9 || rela.rela_type == 41 || rela.rela_type == 42) {
          if (!contains(got_needed_names, sym.name)) got_needed_names.push_back(sym.name);
        } else if (rela.rela_type == 22) {
          if (!contains(got_needed_names, sym.name) && !contains(plt_names, sym.name)) {
            got_needed_names.push_back(sym.name);
          }
        } else if (is_dynamic) {
          if (sym_type != 1 && rela.rela_type == 1) {
            if (!contains(plt_names, sym.name)) plt_names.push_back(sym.name);
          } else if (!contains(plt_names, sym.name) && !contains(got_needed_names, sym.name)) {
            got_needed_names.push_back(sym.name);
          }
        }
      }
    }
  }

  for (const auto& name : plt_names) {
    if (!globals->count(name)) {
      (*globals)[name].info = static_cast<std::uint8_t>((1 << 4) | 2);
      (*globals)[name].is_dynamic = true;
    }
  }

  for (std::size_t plt_idx = 0; plt_idx < plt_names.size(); ++plt_idx) {
    (*globals)[plt_names[plt_idx]].plt_idx = plt_idx;
  }

  std::set<std::string> abs64_sym_names;
  for (const auto& obj : objects) {
    for (const auto& sec_relas : obj.relocations) {
      for (const auto& rela : sec_relas) {
        if (rela.rela_type == 1 && rela.sym_idx < obj.symbols.size()) {
          const auto& sym = obj.symbols[rela.sym_idx];
          if (!sym.name.empty() && !sym.is_local() && sym.sym_type() != 3) {
            abs64_sym_names.insert(sym.name);
          }
        }
      }
    }
  }

  std::vector<std::string> dyn_sym_names;
  std::vector<std::string> exported;
  for (const auto& [name, g] : *globals) {
    if (g.defined_in.has_value() && !g.is_dynamic && (g.info >> 4) != 0 && g.section_idx != 0xffff) {
      exported.push_back(name);
    }
  }
  std::sort(exported.begin(), exported.end());
  for (const auto& name : exported) dyn_sym_names.push_back(name);
  for (const auto& [name, g] : *globals) {
    if ((g.is_dynamic || (!g.defined_in.has_value() && g.section_idx == 0xffff)) &&
        !contains(dyn_sym_names, name)) {
      dyn_sym_names.push_back(name);
    }
  }
  for (const auto& name : abs64_sym_names) {
    if (!contains(dyn_sym_names, name)) dyn_sym_names.push_back(name);
  }
  for (const auto& name : dyn_sym_names) dynstr.add(name);

  const std::uint64_t dynsym_count = 1 + dyn_sym_names.size();
  const std::uint64_t dynsym_size = dynsym_count * 24;
  const std::uint64_t dynstr_size = dynstr.as_bytes().size();
  const std::uint64_t plt_size = plt_names.empty() ? 0 : 16 + 16 * plt_names.size();
  const std::uint64_t got_plt_size = plt_names.empty() ? 0 : (3 + plt_names.size()) * 8;
  const std::uint64_t rela_plt_size = plt_names.size() * 24;

  const bool has_init_array = std::any_of(output_sections->begin(), output_sections->end(),
                                          [](const OutputSection& s) { return s.name == ".init_array" && s.mem_size > 0; });
  const bool has_fini_array = std::any_of(output_sections->begin(), output_sections->end(),
                                          [](const OutputSection& s) { return s.name == ".fini_array" && s.mem_size > 0; });
  std::uint64_t dynamic_size = 0;
  if (true) {
    std::uint64_t dyn_count = needed_sonames.size() + 10;
    if (soname.has_value()) dyn_count += 1;
    if (has_init_array) dyn_count += 2;
    if (has_fini_array) dyn_count += 2;
    if (plt_size > 0) dyn_count += 4;
    if (rpath_string.has_value()) dyn_count += 1;
    dynamic_size = dyn_count * 16;
  }

  std::set<std::size_t> sections_with_abs_relocs;
  for (std::size_t obj_idx = 0; obj_idx < objects.size(); ++obj_idx) {
    const auto& obj = objects[obj_idx];
    for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
      for (const auto& rela : obj.relocations[sec_idx]) {
        if (rela.rela_type == 1 && section_map.contains({obj_idx, sec_idx})) {
          sections_with_abs_relocs.insert(section_map.at({obj_idx, sec_idx}).first);
        }
      }
    }
  }

  auto is_pure_rodata = [&](std::size_t idx, const OutputSection& sec) {
    return (sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_EXECINSTR) == 0 &&
           (sec.flags & SHF_WRITE) == 0 && (sec.flags & SHF_TLS) == 0 &&
           sec.sh_type != SHT_NOBITS && !sections_with_abs_relocs.contains(idx);
  };
  auto is_relro_rodata = [&](std::size_t idx, const OutputSection& sec) {
    return (sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_EXECINSTR) == 0 &&
           (sec.flags & SHF_WRITE) == 0 && (sec.flags & SHF_TLS) == 0 &&
           sec.sh_type != SHT_NOBITS && sections_with_abs_relocs.contains(idx);
  };

  const bool has_relro = !sections_with_abs_relocs.empty();
  std::uint64_t phdr_count = 7;
  if (has_tls_sections) phdr_count += 1;
  if (has_relro) phdr_count += 1;
  const std::uint64_t phdr_total_size = phdr_count * 56;

  std::uint64_t offset = 64 + phdr_total_size;
  offset = align_up(offset, 8);
  const std::uint64_t gnu_hash_offset = offset;
  const std::uint64_t gnu_hash_addr = base_addr + offset;
  const std::uint64_t gnu_hash_size = 0;
  offset += gnu_hash_size;
  offset = align_up(offset, 8);
  const std::uint64_t dynsym_offset = offset;
  const std::uint64_t dynsym_addr = base_addr + offset;
  offset += dynsym_size;
  const std::uint64_t dynstr_offset = offset;
  const std::uint64_t dynstr_addr = base_addr + offset;
  offset += dynstr_size;

  offset = align_up(offset, PAGE_SIZE_LOCAL);
  const std::uint64_t text_page_offset = offset;
  const std::uint64_t text_page_addr = base_addr + offset;
  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_EXECINSTR) != 0 && (sec.flags & SHF_ALLOC) != 0) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = base_addr + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }
  const std::uint64_t plt_addr = plt_size > 0 ? base_addr + align_up(offset, 16) : 0;
  const std::uint64_t plt_offset = plt_size > 0 ? align_up(offset, 16) : 0;
  if (plt_size > 0) offset = plt_offset + plt_size;
  const std::uint64_t text_total_size = offset - text_page_offset;

  offset = align_up(offset, PAGE_SIZE_LOCAL);
  const std::uint64_t rodata_page_offset = offset;
  const std::uint64_t rodata_page_addr = base_addr + offset;
  for (std::size_t idx = 0; idx < output_sections->size(); ++idx) {
    auto& sec = (*output_sections)[idx];
    if (is_pure_rodata(idx, sec)) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = base_addr + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }
  const std::uint64_t rodata_total_size = offset - rodata_page_offset;

  offset = align_up(offset, PAGE_SIZE_LOCAL);
  const std::uint64_t rw_page_offset = offset;
  const std::uint64_t rw_page_addr = base_addr + offset;

  for (std::size_t idx = 0; idx < output_sections->size(); ++idx) {
    auto& sec = (*output_sections)[idx];
    if (is_relro_rodata(idx, sec)) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = base_addr + offset;
      sec.file_offset = offset;
      offset += sec.mem_size;
    }
  }

  std::uint64_t init_array_addr = 0;
  std::uint64_t init_array_size = 0;
  std::uint64_t fini_array_addr = 0;
  std::uint64_t fini_array_size = 0;
  for (auto& sec : *output_sections) {
    if (sec.name == ".init_array") {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 8);
      offset = align_up(offset, a);
      sec.addr = base_addr + offset;
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
      sec.addr = base_addr + offset;
      sec.file_offset = offset;
      fini_array_addr = sec.addr;
      fini_array_size = sec.mem_size;
      offset += sec.mem_size;
      break;
    }
  }

  offset = align_up(offset, 8);
  const std::uint64_t rela_dyn_offset = offset;
  const std::uint64_t rela_dyn_addr = base_addr + offset;
  std::uint64_t max_rela_count = 0;
  for (const auto& obj : objects) {
    for (const auto& sec_relas : obj.relocations) {
      for (const auto& rela : sec_relas) {
        if (rela.rela_type == 1) ++max_rela_count;
      }
    }
  }
  for (const auto& sec : *output_sections) {
    if (sec.name == ".init_array" || sec.name == ".fini_array") max_rela_count += sec.mem_size / 8;
  }
  max_rela_count += got_needed_names.size();
  const std::uint64_t rela_dyn_max_size = max_rela_count * 24;
  offset += rela_dyn_max_size;

  offset = align_up(offset, 8);
  const std::uint64_t rela_plt_offset = offset;
  const std::uint64_t rela_plt_addr = base_addr + offset;
  offset += rela_plt_size;

  offset = align_up(offset, 8);
  const std::uint64_t dynamic_offset = offset;
  const std::uint64_t dynamic_addr = base_addr + offset;
  offset += dynamic_size;

  const std::uint64_t relro_end_offset = align_up(offset, PAGE_SIZE_LOCAL);
  const std::uint64_t relro_end_addr = base_addr + relro_end_offset;
  if (has_relro) offset = relro_end_offset;

  offset = align_up(offset, 8);
  const std::uint64_t got_plt_offset = offset;
  const std::uint64_t got_plt_addr = base_addr + offset;
  offset += got_plt_size;

  const std::uint64_t got_offset = offset;
  const std::uint64_t got_addr = base_addr + offset;
  const std::uint64_t got_size = got_needed_names.size() * 8;
  offset += got_size;

  for (auto& sec : *output_sections) {
    if ((sec.flags & SHF_ALLOC) != 0 && (sec.flags & SHF_WRITE) != 0 &&
        sec.sh_type != SHT_NOBITS && sec.name != ".init_array" && sec.name != ".fini_array" &&
        (sec.flags & SHF_TLS) == 0) {
      const std::uint64_t a = std::max<std::uint64_t>(sec.alignment, 1);
      offset = align_up(offset, a);
      sec.addr = base_addr + offset;
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
      sec.addr = base_addr + offset;
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
    tls_addr = base_addr + offset;
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

  const std::uint64_t bss_addr = base_addr + offset;
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

  merge_section_data(objects, output_sections);

  for (auto& [name, gsym] : *globals) {
    if (!gsym.defined_in.has_value()) continue;
    if (gsym.section_idx == 0xffff) {
      if (auto it = std::find_if(output_sections->begin(), output_sections->end(),
                                 [](const OutputSection& s) { return s.name == ".bss"; });
          it != output_sections->end()) {
        gsym.value += it->addr;
      }
    } else if (gsym.section_idx != 0xffff && gsym.section_idx != 0) {
      const std::size_t si = gsym.section_idx;
      if (section_map.contains({*gsym.defined_in, si})) {
        const auto [oi, so] = section_map.at({*gsym.defined_in, si});
        gsym.value += (*output_sections)[oi].addr + so;
      }
    }
  }

  const LinkerSymbolAddresses linker_addrs{
      base_addr,
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
      rela_dyn_addr,
      rela_dyn_max_size,
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

  std::vector<std::uint8_t> out(static_cast<std::size_t>(offset), 0);
  out[0] = 0x7f;
  out[1] = 'E';
  out[2] = 'L';
  out[3] = 'F';
  out[4] = 2;
  out[5] = 1;
  out[6] = 1;
  write_u16(&out, 16, ET_DYN);
  write_u16(&out, 18, EM_X86_64);
  write_u32(&out, 20, 1);
  write_u64(&out, 24, 0);
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
  write_phdr(&out, ph, PT_PHDR, PF_R, 64, base_addr + 64, phdr_total_size, phdr_total_size, 8);
  ph += 56;
  write_phdr(&out, ph, PT_LOAD, PF_R, 0, base_addr, rela_plt_offset + rela_plt_size,
             rela_plt_offset + rela_plt_size, PAGE_SIZE_LOCAL);
  ph += 56;
  write_phdr(&out, ph, PT_LOAD, PF_R | PF_X, text_page_offset, text_page_addr, text_total_size,
             text_total_size, PAGE_SIZE_LOCAL);
  ph += 56;
  write_phdr(&out, ph, PT_LOAD, PF_R, rodata_page_offset, rodata_page_addr, rodata_total_size,
             rodata_total_size, PAGE_SIZE_LOCAL);
  ph += 56;
  const std::uint64_t rw_filesz = offset - rw_page_offset;
  const std::uint64_t rw_memsz = bss_size > 0 ? (bss_addr + bss_size) - rw_page_addr : rw_filesz;
  write_phdr(&out, ph, PT_LOAD, PF_R | PF_W, rw_page_offset, rw_page_addr, rw_filesz, rw_memsz,
             PAGE_SIZE_LOCAL);
  ph += 56;
  write_phdr(&out, ph, PT_DYNAMIC, PF_R | PF_W, dynamic_offset, dynamic_addr, dynamic_size,
             dynamic_size, 8);
  ph += 56;
  if (tls_addr != 0) {
    write_phdr(&out, ph, PT_TLS, PF_R, tls_file_offset, tls_addr, tls_file_size, tls_mem_size, tls_align);
    ph += 56;
  }
  write_phdr(&out, ph, PT_GNU_STACK, PF_R | PF_W, 0, 0, 0, 0, 16);
  ph += 56;
  if (has_relro) {
    write_phdr(&out, ph, PT_GNU_RELRO, PF_R, rw_page_offset, rw_page_addr, rw_filesz, rw_filesz, PAGE_SIZE_LOCAL);
    ph += 56;
  }

  for (const auto& sec : *output_sections) {
    if (sec.data.empty() || sec.file_offset + sec.data.size() > out.size()) continue;
    std::copy(sec.data.begin(), sec.data.end(), out.begin() + static_cast<std::ptrdiff_t>(sec.file_offset));
  }

  (void)gnu_hash_addr;
  (void)dynsym_addr;
  (void)dynstr_addr;
  (void)rela_dyn_offset;
  (void)rela_plt_addr;
  (void)relro_end_addr;
  (void)output_path;
  (void)use_runpath;
  if (error != nullptr) error->clear();
  return true;
}

}  // namespace c4c::backend::x86::linker

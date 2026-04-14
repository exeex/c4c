// Mechanical C++ translation of src/backend/riscv/linker/emit_exec.rs
// RISC-V executable emission helpers plus a translation stub for the
// orchestration entry point.

#include "mod.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {

namespace {

constexpr std::uint16_t kEtExec = 2;
constexpr std::uint16_t kEtDyn = 3;
constexpr std::uint16_t kEmRiscv = 243;
constexpr std::uint32_t kShtNull = 0;
constexpr std::uint32_t kShtProgbits = 1;
constexpr std::uint32_t kShtStrtab = 3;
constexpr std::uint32_t kShtRela = 4;
constexpr std::uint32_t kShtDynamic = 6;
constexpr std::uint32_t kShtNobits = 8;
constexpr std::uint32_t kShtSymtab = 2;

constexpr std::uint64_t kShfWrite = 0x1;
constexpr std::uint64_t kShfAlloc = 0x2;
constexpr std::uint64_t kShfExecinstr = 0x4;
constexpr std::uint64_t kShfTls = 0x400;
constexpr std::uint64_t kShfInfoLink = 0x40;

constexpr std::uint32_t kPtLoad = 1;
constexpr std::uint32_t kPtDynamic = 2;
constexpr std::uint32_t kPtInterp = 3;
constexpr std::uint32_t kPtNote = 4;
constexpr std::uint32_t kPtPhdr = 6;
constexpr std::uint32_t kPtTls = 7;
constexpr std::uint32_t kPtGnuEhFrame = 0x6474e550;
constexpr std::uint32_t kPtGnuStack = 0x6474e551;
constexpr std::uint32_t kPtGnuRelro = 0x6474e552;
constexpr std::uint32_t kPtRiscvAttributes = 0x70000003;

constexpr std::uint32_t kPfX = 1;
constexpr std::uint32_t kPfW = 2;
constexpr std::uint32_t kPfR = 4;

constexpr std::uint8_t kStbLocal = 0;
constexpr std::uint8_t kStbGlobal = 1;
constexpr std::uint8_t kStbWeak = 2;
constexpr std::uint8_t kSttNotype = 0;
constexpr std::uint8_t kSttObject = 1;
constexpr std::uint8_t kSttFunc = 2;
constexpr std::uint8_t kSttSection = 3;
constexpr std::uint16_t kShnUndef = 0;
constexpr std::uint16_t kShnAbs = 0xfff1;
constexpr std::uint16_t kShnCommon = 0xfff2;

constexpr std::uint64_t kPageSize = 0x1000;
constexpr std::uint64_t kBaseAddr = 0x10000;

constexpr std::array<std::uint8_t, 4> kElfMagic = {0x7f, 'E', 'L', 'F'};
constexpr std::array<std::uint8_t, 33> kInterp = {
    '/', 'l', 'i', 'b', '/', 'l', 'd', '-', 'l', 'i', 'n', 'u', 'x', '-',
    'r', 'i', 's', 'c', 'v', '6', '4', '-', 'l', 'p', '6', '4', 'd', '.',
    's', 'o', '.', '1', '\0'};

struct Symbol {
  std::string name;
  std::uint8_t info = 0;
  std::uint8_t other = 0;
  std::uint16_t shndx = 0;
  std::uint64_t value = 0;
  std::uint64_t size = 0;
};

struct ElfObject {
  std::vector<std::vector<std::uint8_t>> section_data;
  std::vector<std::vector<std::uint8_t>> relocations;
  std::vector<Symbol> symbols;
  std::string source_name;
};

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = kShtProgbits;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
  std::uint64_t align = 1;
};

struct GlobalSym {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t binding = kStbLocal;
  std::uint8_t sym_type = kSttNotype;
  std::uint8_t visibility = 0;
  bool defined = false;
  bool needs_plt = false;
  std::size_t plt_idx = 0;
  std::optional<std::uint64_t> got_offset;
  std::optional<std::size_t> section_idx;
};

struct LinkerSymbolAddresses {
  std::uint64_t base_addr = 0;
  std::uint64_t got_addr = 0;
  std::uint64_t dynamic_addr = 0;
  std::uint64_t bss_addr = 0;
  std::uint64_t bss_size = 0;
  std::uint64_t text_end = 0;
  std::uint64_t data_start = 0;
  std::uint64_t init_array_start = 0;
  std::uint64_t init_array_size = 0;
  std::uint64_t fini_array_start = 0;
  std::uint64_t fini_array_size = 0;
  std::uint64_t preinit_array_start = 0;
  std::uint64_t preinit_array_size = 0;
  std::uint64_t rela_iplt_start = 0;
  std::uint64_t rela_iplt_size = 0;
};

struct StandardLinkerSymbol {
  const char* name = nullptr;
  std::uint64_t value = 0;
  std::uint8_t binding = kStbGlobal;
};

template <typename... Ts>
void ignore_unused(const Ts&...) {}

bool has_prefix(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool has_suffix(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

std::uint64_t align_up(std::uint64_t value, std::uint64_t align) {
  if (align == 0) return value;
  return (value + align - 1) & ~(align - 1);
}

void pad_to(std::vector<std::uint8_t>* out, std::size_t size) {
  if (out == nullptr || out->size() >= size) return;
  out->resize(size, 0);
}

void write_u16(std::vector<std::uint8_t>* out, std::size_t offset, std::uint16_t value) {
  pad_to(out, offset + 2);
  (*out)[offset + 0] = static_cast<std::uint8_t>(value & 0xff);
  (*out)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write_u32(std::vector<std::uint8_t>* out, std::size_t offset, std::uint32_t value) {
  pad_to(out, offset + 4);
  for (std::size_t i = 0; i < 4; ++i) {
    (*out)[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_u64(std::vector<std::uint8_t>* out, std::size_t offset, std::uint64_t value) {
  pad_to(out, offset + 8);
  for (std::size_t i = 0; i < 8; ++i) {
    (*out)[offset + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_phdr(std::vector<std::uint8_t>* out,
                std::size_t offset,
                std::uint32_t p_type,
                std::uint32_t p_flags,
                std::uint64_t p_offset,
                std::uint64_t p_vaddr,
                std::uint64_t p_filesz,
                std::uint64_t p_memsz,
                std::uint64_t p_align) {
  write_u32(out, offset + 0, p_type);
  write_u32(out, offset + 4, p_flags);
  write_u64(out, offset + 8, p_offset);
  write_u64(out, offset + 16, p_vaddr);
  write_u64(out, offset + 24, p_vaddr);
  write_u64(out, offset + 32, p_filesz);
  write_u64(out, offset + 40, p_memsz);
  write_u64(out, offset + 48, p_align);
}

void write_shdr(std::vector<std::uint8_t>* out,
                std::size_t offset,
                std::uint32_t sh_name,
                std::uint32_t sh_type,
                std::uint64_t sh_flags,
                std::uint64_t sh_addr,
                std::uint64_t sh_offset,
                std::uint64_t sh_size,
                std::uint32_t sh_link,
                std::uint32_t sh_info,
                std::uint64_t sh_addralign,
                std::uint64_t sh_entsize) {
  write_u32(out, offset + 0, sh_name);
  write_u32(out, offset + 4, sh_type);
  write_u64(out, offset + 8, sh_flags);
  write_u64(out, offset + 16, sh_addr);
  write_u64(out, offset + 24, sh_offset);
  write_u64(out, offset + 32, sh_size);
  write_u32(out, offset + 40, sh_link);
  write_u32(out, offset + 44, sh_info);
  write_u64(out, offset + 48, sh_addralign);
  write_u64(out, offset + 56, sh_entsize);
}

std::uint32_t gnu_hash_bytes(std::string_view name) {
  std::uint32_t h = 5381;
  for (unsigned char ch : name) {
    h = (h << 5) + h + ch;
  }
  return h;
}

std::size_t next_power_of_two(std::size_t value) {
  if (value <= 1) return 1;
  --value;
  for (std::size_t shift = 1; shift < sizeof(std::size_t) * 8; shift <<= 1) {
    value |= value >> shift;
  }
  return value + 1;
}

std::pair<std::vector<std::uint8_t>, std::vector<std::size_t>> build_gnu_hash(
    const std::vector<std::string>& sym_names) {
  const std::size_t nsyms = sym_names.size();
  if (nsyms == 0) {
    std::vector<std::uint8_t> data;
    write_u32(&data, 0, 1);
    write_u32(&data, 4, 1);
    write_u32(&data, 8, 1);
    write_u32(&data, 12, 6);
    write_u64(&data, 16, 0);
    write_u32(&data, 24, 0);
    return {std::move(data), {}};
  }

  std::vector<std::uint32_t> hashes;
  hashes.reserve(nsyms);
  for (const auto& name : sym_names) {
    hashes.push_back(gnu_hash_bytes(name));
  }

  const std::uint32_t nbuckets = static_cast<std::uint32_t>(std::max<std::size_t>(1, nsyms));
  const std::uint32_t symoffset = 1;
  const std::uint32_t bloom_shift = 6;
  const std::size_t bloom_size = next_power_of_two(std::max<std::size_t>(1, nsyms / 2));
  std::vector<std::uint64_t> bloom(bloom_size, 0);

  std::vector<std::size_t> indices(nsyms);
  for (std::size_t i = 0; i < nsyms; ++i) indices[i] = i;
  std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
    return (hashes[a] % nbuckets) < (hashes[b] % nbuckets);
  });

  constexpr std::uint32_t c = 64;
  for (const auto idx : indices) {
    const std::uint32_t h = hashes[idx];
    const std::size_t word_idx = static_cast<std::size_t>((h / c) % bloom_size);
    const std::uint64_t bit1 = static_cast<std::uint64_t>(h % c);
    const std::uint64_t bit2 = static_cast<std::uint64_t>((h >> bloom_shift) % c);
    bloom[word_idx] |= (1ull << bit1) | (1ull << bit2);
  }

  std::vector<std::uint32_t> buckets(nbuckets, 0);
  std::vector<std::uint32_t> chain(nsyms, 0);
  for (std::size_t pos = 0; pos < nsyms; ++pos) {
    const std::size_t orig_idx = indices[pos];
    const std::uint32_t h = hashes[orig_idx];
    const std::uint32_t bucket = h % nbuckets;
    const std::uint32_t dynsym_idx = static_cast<std::uint32_t>(pos) + symoffset;
    if (buckets[bucket] == 0) {
      buckets[bucket] = dynsym_idx;
    }
    chain[pos] = h & ~1u;
  }

  for (std::size_t pos = nsyms; pos-- > 0;) {
    const std::uint32_t h = hashes[indices[pos]];
    const std::uint32_t bucket = h % nbuckets;
    const bool is_last = pos + 1 >= nsyms || (hashes[indices[pos + 1]] % nbuckets) != bucket;
    if (is_last) {
      chain[pos] |= 1u;
    }
  }

  std::vector<std::uint8_t> data;
  data.reserve(16 + bloom.size() * 8 + buckets.size() * 4 + chain.size() * 4);
  write_u32(&data, 0, nbuckets);
  write_u32(&data, 4, symoffset);
  write_u32(&data, 8, static_cast<std::uint32_t>(bloom.size()));
  write_u32(&data, 12, bloom_shift);
  for (const auto word : bloom) {
    const std::size_t off = data.size();
    write_u64(&data, off, word);
  }
  for (const auto bucket : buckets) {
    const std::size_t off = data.size();
    write_u32(&data, off, bucket);
  }
  for (const auto value : chain) {
    const std::size_t off = data.size();
    write_u32(&data, off, value);
  }

  return {std::move(data), std::move(indices)};
}

bool is_valid_c_identifier_for_section(std::string_view text) {
  if (text.empty()) return false;
  const unsigned char first = static_cast<unsigned char>(text.front());
  if (!(std::isalpha(first) || first == '_')) return false;
  for (std::size_t i = 1; i < text.size(); ++i) {
    const unsigned char ch = static_cast<unsigned char>(text[i]);
    if (!(std::isalnum(ch) || ch == '_')) return false;
  }
  return true;
}

std::vector<StandardLinkerSymbol> get_standard_linker_symbols(
    const LinkerSymbolAddresses& addrs) {
  const std::uint64_t end_addr = addrs.bss_addr + addrs.bss_size;
  return {
      {"_GLOBAL_OFFSET_TABLE_", addrs.got_addr, kStbGlobal},
      {"_DYNAMIC", addrs.dynamic_addr, kStbGlobal},
      {"__bss_start", addrs.bss_addr, kStbGlobal},
      {"_edata", addrs.bss_addr, kStbGlobal},
      {"_end", end_addr, kStbGlobal},
      {"__end", end_addr, kStbGlobal},
      {"_etext", addrs.text_end, kStbGlobal},
      {"etext", addrs.text_end, kStbGlobal},
      {"__ehdr_start", addrs.base_addr, kStbGlobal},
      {"__executable_start", addrs.base_addr, kStbGlobal},
      {"__dso_handle", addrs.data_start, kStbGlobal},
      {"__data_start", addrs.data_start, kStbGlobal},
      {"data_start", addrs.data_start, 2},
      {"__init_array_start", addrs.init_array_start, kStbGlobal},
      {"__init_array_end", addrs.init_array_start + addrs.init_array_size, kStbGlobal},
      {"__fini_array_start", addrs.fini_array_start, kStbGlobal},
      {"__fini_array_end", addrs.fini_array_start + addrs.fini_array_size, kStbGlobal},
      {"__preinit_array_start", addrs.preinit_array_start, kStbGlobal},
      {"__preinit_array_end", addrs.preinit_array_start + addrs.preinit_array_size, kStbGlobal},
      {"__rela_iplt_start", addrs.rela_iplt_start, kStbGlobal},
      {"__rela_iplt_end", addrs.rela_iplt_start + addrs.rela_iplt_size, kStbGlobal},
  };
}

std::vector<std::pair<std::string, std::uint64_t>> resolve_start_stop_symbols(
    const std::vector<MergedSection>& output_sections) {
  std::vector<std::pair<std::string, std::uint64_t>> out;
  for (const auto& sec : output_sections) {
    if (is_valid_c_identifier_for_section(sec.name)) {
      out.emplace_back("__start_" + sec.name, sec.vaddr);
      out.emplace_back("__stop_" + sec.name, sec.vaddr + sec.data.size());
    }
  }
  return out;
}

void write_elf_header(std::vector<std::uint8_t>* elf,
                      std::uint16_t e_type,
                      std::uint16_t e_machine,
                      std::uint64_t entry_point,
                      std::uint16_t phnum,
                      std::uint32_t e_flags) {
  elf->insert(elf->end(), kElfMagic.begin(), kElfMagic.end());
  elf->push_back(2);  // ELFCLASS64
  elf->push_back(1);  // ELFDATA2LSB
  elf->push_back(1);  // EV_CURRENT
  elf->push_back(0);  // ELFOSABI_NONE
  elf->insert(elf->end(), 8, 0);
  write_u16(elf, 16, e_type);
  write_u16(elf, 18, e_machine);
  write_u32(elf, 20, 1);
  write_u64(elf, 24, entry_point);
  write_u64(elf, 32, 64);
  write_u64(elf, 40, 0);
  write_u32(elf, 48, e_flags);
  write_u16(elf, 52, 64);
  write_u16(elf, 54, 56);
  write_u16(elf, 56, phnum);
  write_u16(elf, 58, 64);
  write_u16(elf, 60, 0);
  write_u16(elf, 62, 0);
}

void build_plt_stubs(std::vector<std::uint8_t>* plt_data,
                     std::uint64_t plt_vaddr,
                     std::uint64_t plt_header_size,
                     std::uint64_t plt_entry_size,
                     std::uint64_t got_plt_vaddr,
                     const std::vector<std::string>& plt_symbols) {
  ignore_unused(plt_symbols);

  const std::uint64_t plt0_addr = plt_vaddr;
  const std::int64_t got_plt_rel = static_cast<std::int64_t>(got_plt_vaddr) -
                                   static_cast<std::int64_t>(plt0_addr);
  const std::uint32_t hi = static_cast<std::uint32_t>((got_plt_rel + 0x800) >> 12);
  const std::uint32_t lo = static_cast<std::uint32_t>(got_plt_rel & 0xFFF);

  write_u32(plt_data, 0, 0x00000397u | (hi << 12));
  write_u32(plt_data, 4, 0x41c30333u);
  write_u32(plt_data, 8, 0x0003be03u | (lo << 20));
  const std::uint32_t neg_hdr = static_cast<std::uint32_t>(
      static_cast<std::int32_t>(-(static_cast<std::int64_t>(plt_header_size) + 12)));
  write_u32(plt_data, 12, 0x00030313u | ((neg_hdr & 0xFFFu) << 20));
  write_u32(plt_data, 16, 0x00038293u | (lo << 20));
  write_u32(plt_data, 20, 0x00135313u);
  write_u32(plt_data, 24, 0x0082b283u);
  write_u32(plt_data, 28, 0x000e0067u);

  for (std::size_t i = 0; i < plt_symbols.size(); ++i) {
    const std::size_t entry_off = static_cast<std::size_t>(plt_header_size +
                                                           i * plt_entry_size);
    const std::uint64_t entry_addr = plt_vaddr + entry_off;
    const std::uint64_t got_entry_addr = got_plt_vaddr + (2 + i) * 8;

    const std::int64_t rel = static_cast<std::int64_t>(got_entry_addr) -
                             static_cast<std::int64_t>(entry_addr);
    const std::uint32_t entry_hi = static_cast<std::uint32_t>((rel + 0x800) >> 12);
    const std::uint32_t entry_lo = static_cast<std::uint32_t>(rel & 0xFFF);

    write_u32(plt_data, entry_off + 0, 0x00000e17u | (entry_hi << 12));
    write_u32(plt_data, entry_off + 4, 0x000e3e03u | (entry_lo << 20));
    write_u32(plt_data, entry_off + 8, 0x000e0367u);
    write_u32(plt_data, entry_off + 12, 0x00000013u);
  }
}

}  // namespace

std::optional<std::vector<std::uint8_t>> emit_first_static_executable_image(
    const std::vector<std::uint8_t>& merged_text,
    std::uint64_t base_address,
    std::uint64_t entry_address,
    std::uint64_t* text_file_offset,
    std::uint64_t* text_virtual_address,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (merged_text.empty()) {
    if (error != nullptr) *error = "first static executable requires merged .text bytes";
    return std::nullopt;
  }

  if (base_address == 0) base_address = kBaseAddr;
  const std::uint64_t text_offset = 64 + 56;
  const std::uint64_t text_vaddr = base_address + text_offset;
  const std::uint64_t file_size = text_offset + merged_text.size();

  if (entry_address < text_vaddr || entry_address >= text_vaddr + merged_text.size()) {
    if (error != nullptr) {
      *error = "entry point for first static executable must live in merged .text";
    }
    return std::nullopt;
  }

  std::vector<std::uint8_t> image(static_cast<std::size_t>(file_size), 0);
  image[0] = 0x7f;
  image[1] = 'E';
  image[2] = 'L';
  image[3] = 'F';
  image[4] = 2;
  image[5] = 1;
  image[6] = 1;

  write_u16(&image, 16, kEtExec);
  write_u16(&image, 18, ::c4c::backend::riscv::linker::kEmRiscv);
  write_u32(&image, 20, 1);
  write_u64(&image, 24, entry_address);
  write_u64(&image, 32, 64);
  write_u64(&image, 40, 0);
  write_u32(&image, 48, 0);
  write_u16(&image, 52, 64);
  write_u16(&image, 54, 56);
  write_u16(&image, 56, 1);
  write_u16(&image, 58, 0);
  write_u16(&image, 60, 0);
  write_u16(&image, 62, 0);

  write_u32(&image, 64 + 0, kPtLoad);
  write_u32(&image, 64 + 4, kPfR | kPfX);
  write_u64(&image, 64 + 8, 0);
  write_u64(&image, 64 + 16, base_address);
  write_u64(&image, 64 + 24, base_address);
  write_u64(&image, 64 + 32, file_size);
  write_u64(&image, 64 + 40, file_size);
  write_u64(&image, 64 + 48, kPageSize);

  std::copy(merged_text.begin(), merged_text.end(), image.begin() + text_offset);

  if (text_file_offset != nullptr) *text_file_offset = text_offset;
  if (text_virtual_address != nullptr) *text_virtual_address = text_vaddr;
  return image;
}

bool emit_executable(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    std::vector<MergedSection>* merged_sections,
    std::map<std::string, std::size_t>* merged_map,
    const std::map<std::pair<std::size_t, std::size_t>, std::pair<std::size_t, std::uint64_t>>& sec_mapping,
    std::map<std::string, GlobalSym>* global_syms,
    const std::vector<std::string>& got_symbols,
    const std::vector<std::string>& tls_got_symbols,
    const std::map<std::string, std::tuple<std::size_t, std::size_t, std::int64_t>>& local_got_sym_info,
    const std::vector<std::string>& plt_symbols,
    const std::vector<std::pair<std::string, std::uint64_t>>& copy_symbols,
    const std::vector<std::size_t>& sec_indices,
    const std::vector<std::string>& actual_needed_libs,
    bool is_static,
    const std::string& output_path,
    std::string* error) {
  ignore_unused(input_objs, merged_sections, merged_map, sec_mapping, global_syms, got_symbols,
                tls_got_symbols, local_got_sym_info, plt_symbols, copy_symbols, sec_indices,
                actual_needed_libs, is_static, output_path);
  if (error != nullptr) {
    *error = "RISC-V emit_executable is not yet wired to the shared linker surface";
  }
  return false;
}

}  // namespace c4c::backend::riscv::linker

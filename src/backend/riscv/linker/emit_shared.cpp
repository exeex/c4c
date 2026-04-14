// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/emit_shared.rs
// Shared-library emission helpers and phase inventory for the RISC-V linker.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {

namespace {

inline constexpr std::uint32_t R_RISCV_64 = 2;
inline constexpr std::uint32_t R_RISCV_RELATIVE = 3;
inline constexpr std::uint32_t R_RISCV_JUMP_SLOT = 5;
inline constexpr std::uint32_t R_RISCV_GLOB_DAT = 6;
inline constexpr std::uint32_t R_RISCV_CALL_PLT = 19;

inline constexpr std::uint32_t ET_DYN = 3;
inline constexpr std::uint32_t PT_LOAD = 1;
inline constexpr std::uint32_t PT_DYNAMIC = 2;
inline constexpr std::uint32_t PT_TLS = 7;
inline constexpr std::uint32_t PT_GNU_STACK = 0x6474e551;
inline constexpr std::uint32_t PT_GNU_RELRO = 0x6474e552;
inline constexpr std::uint32_t PT_RISCV_ATTRIBUTES = 0x70000003;

inline constexpr std::uint32_t PT_PHDR = 6;
inline constexpr std::uint32_t PF_X = 0x1;
inline constexpr std::uint32_t PF_W = 0x2;
inline constexpr std::uint32_t PF_R = 0x4;

inline constexpr std::uint32_t SHT_NULL = 0;
inline constexpr std::uint32_t SHT_PROGBITS = 1;
inline constexpr std::uint32_t SHT_STRTAB = 3;
inline constexpr std::uint32_t SHT_RELA = 4;
inline constexpr std::uint32_t SHT_DYNAMIC = 6;
inline constexpr std::uint32_t SHT_DYNSYM = 11;
inline constexpr std::uint32_t SHT_NOBITS = 8;
inline constexpr std::uint32_t SHT_RISCV_ATTRIBUTES = 0x70000003;

inline constexpr std::uint64_t SHF_WRITE = 0x1;
inline constexpr std::uint64_t SHF_ALLOC = 0x2;
inline constexpr std::uint64_t SHF_EXECINSTR = 0x4;
inline constexpr std::uint64_t SHF_TLS = 0x400;

inline constexpr std::uint8_t STB_LOCAL = 0;
inline constexpr std::uint8_t STB_GLOBAL = 1;
inline constexpr std::uint8_t STB_WEAK = 2;

inline constexpr std::uint8_t STT_NOTYPE = 0;
inline constexpr std::uint8_t STT_OBJECT = 1;
inline constexpr std::uint8_t STT_FUNC = 2;
inline constexpr std::uint8_t STT_SECTION = 3;

inline constexpr std::uint16_t SHN_UNDEF = 0;

inline constexpr std::uint64_t PAGE_SIZE = 0x1000;

std::uint64_t align_up(std::uint64_t value, std::uint64_t alignment) {
  if (alignment == 0) return value;
  const std::uint64_t mask = alignment - 1;
  return (value + mask) & ~mask;
}

void pad_to(std::vector<std::uint8_t>* out, std::size_t size) {
  if (out->size() < size) out->resize(size, 0);
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

void append_u32(std::vector<std::uint8_t>* out, std::uint32_t value) {
  for (std::size_t i = 0; i < 4; ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
  }
}

void append_u64(std::vector<std::uint8_t>* out, std::uint64_t value) {
  for (std::size_t i = 0; i < 8; ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
  }
}

bool contains(const std::vector<std::string>& values, const std::string& value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

void append_unique(std::vector<std::string>* values, const std::string& value) {
  if (values == nullptr) return;
  if (!contains(*values, value)) values->push_back(value);
}

std::uint32_t gnu_hash_bytes(std::string_view text) {
  std::uint32_t value = 5381;
  for (unsigned char ch : text) {
    value = value * 33u + ch;
  }
  return value;
}

bool has_prefix(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

struct DynStrTab {
  std::vector<std::uint8_t> data{0};
  std::map<std::string, std::uint32_t> offsets;

  std::uint32_t add(const std::string& value) {
    const auto it = offsets.find(value);
    if (it != offsets.end()) return it->second;
    const std::uint32_t off = static_cast<std::uint32_t>(data.size());
    offsets.emplace(value, off);
    data.insert(data.end(), value.begin(), value.end());
    data.push_back(0);
    return off;
  }

  [[nodiscard]] std::uint32_t get_offset(const std::string& value) const {
    const auto it = offsets.find(value);
    return it == offsets.end() ? 0u : it->second;
  }
};

struct Relocation {
  std::uint64_t offset = 0;
  std::uint32_t rela_type = 0;
  std::uint32_t sym_idx = 0;
  std::int64_t addend = 0;
};

struct Symbol {
  std::string name;
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t binding_ = STB_LOCAL;
  std::uint8_t type_ = STT_NOTYPE;
  std::uint16_t shndx = SHN_UNDEF;

  [[nodiscard]] std::uint8_t binding() const { return binding_; }
  [[nodiscard]] std::uint8_t sym_type() const { return type_; }
  [[nodiscard]] bool is_local() const { return binding_ == STB_LOCAL; }
  [[nodiscard]] bool is_undefined() const { return shndx == SHN_UNDEF; }
};

struct ElfObject {
  std::vector<std::vector<Relocation>> relocations;
  std::vector<Symbol> symbols;
};

struct GlobalSym {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t binding = STB_LOCAL;
  std::uint8_t sym_type = STT_NOTYPE;
  bool defined = false;
  std::optional<std::size_t> section_idx;
  std::optional<std::uint64_t> got_offset;
};

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = SHT_NULL;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
  std::uint64_t align = 1;
};

void write_phdr_at(std::vector<std::uint8_t>* out,
                   std::size_t offset,
                   std::uint32_t p_type,
                   std::uint32_t flags,
                   std::uint64_t file_offset,
                   std::uint64_t vaddr,
                   std::uint64_t paddr,
                   std::uint64_t filesz,
                   std::uint64_t memsz,
                   std::uint64_t align) {
  if (offset + 56 > out->size()) out->resize(offset + 56, 0);
  write_u32(out, offset + 0, p_type);
  write_u32(out, offset + 4, flags);
  write_u64(out, offset + 8, file_offset);
  write_u64(out, offset + 16, vaddr);
  write_u64(out, offset + 24, paddr);
  write_u64(out, offset + 32, filesz);
  write_u64(out, offset + 40, memsz);
  write_u64(out, offset + 48, align);
}

std::pair<std::vector<std::uint8_t>, std::vector<std::size_t>> build_gnu_hash(
    const std::vector<std::string>& sym_names) {
  const std::size_t nsyms = sym_names.size();
  if (nsyms == 0) {
    std::vector<std::uint8_t> data;
    append_u32(&data, 1);
    append_u32(&data, 1);
    append_u32(&data, 1);
    append_u32(&data, 6);
    append_u64(&data, 0);
    append_u32(&data, 0);
    return {data, {}};
  }

  const std::vector<std::uint32_t> hashes = [&] {
    std::vector<std::uint32_t> out;
    out.reserve(nsyms);
    for (const auto& name : sym_names) out.push_back(gnu_hash_bytes(name));
    return out;
  }();

  const std::uint32_t nbuckets = static_cast<std::uint32_t>(std::max<std::size_t>(1, nsyms));
  std::vector<std::size_t> indices(nsyms);
  for (std::size_t i = 0; i < nsyms; ++i) indices[i] = i;
  std::sort(indices.begin(), indices.end(), [&](std::size_t lhs, std::size_t rhs) {
    return (hashes[lhs] % nbuckets) < (hashes[rhs] % nbuckets);
  });

  const std::uint32_t symoffset = 1;
  const std::uint32_t bloom_shift = 6;
  const std::uint32_t bloom_size = static_cast<std::uint32_t>(std::max<std::size_t>(1, (nsyms / 2)));
  std::vector<std::uint64_t> bloom(bloom_size, 0);
  for (const std::size_t idx : indices) {
    const std::uint32_t h = hashes[idx];
    const std::size_t word_idx = ((h / 64u) % bloom_size);
    bloom[word_idx] |= (1ull << (h % 64u)) | (1ull << ((h >> bloom_shift) % 64u));
  }

  std::vector<std::uint32_t> buckets(nbuckets, 0);
  std::vector<std::uint32_t> chain(nsyms, 0);
  for (std::size_t pos = 0; pos < indices.size(); ++pos) {
    const std::size_t orig_idx = indices[pos];
    const std::uint32_t h = hashes[orig_idx];
    const std::uint32_t bucket = h % nbuckets;
    const std::uint32_t dynsym_idx = static_cast<std::uint32_t>(pos) + symoffset;
    if (buckets[bucket] == 0) buckets[bucket] = dynsym_idx;
    chain[pos] = h & ~1u;
  }
  for (std::size_t pos = 0; pos < indices.size(); ++pos) {
    const std::uint32_t h = hashes[indices[pos]];
    const std::uint32_t bucket = h % nbuckets;
    const bool is_last = pos + 1 >= indices.size() || (hashes[indices[pos + 1]] % nbuckets) != bucket;
    if (is_last) chain[pos] |= 1u;
  }

  std::vector<std::uint8_t> data;
  data.reserve(16 + bloom.size() * 8 + buckets.size() * 4 + chain.size() * 4);
  append_u32(&data, nbuckets);
  append_u32(&data, symoffset);
  append_u32(&data, bloom_size);
  append_u32(&data, bloom_shift);
  for (const auto word : bloom) {
    append_u64(&data, word);
  }
  for (const auto bucket : buckets) {
    append_u32(&data, bucket);
  }
  for (const auto ch : chain) {
    append_u32(&data, ch);
  }
  return {data, indices};
}

std::uint64_t section_order(const std::string& name, std::uint64_t flags) {
  if (name == ".text") return 100;
  if (name == ".rodata") return 200;
  if (name == ".eh_frame_hdr") return 250;
  if (name == ".eh_frame") return 260;
  if (name == ".preinit_array") return 500;
  if (name == ".init_array") return 510;
  if (name == ".fini_array") return 520;
  if (name == ".data") return 600;
  if (name == ".sdata") return 650;
  if (name == ".bss" || name == ".sbss") return 700;
  if (flags & SHF_EXECINSTR) return 150;
  if ((flags & SHF_WRITE) == 0) return 300;
  return 600;
}

bool contains_section_name(const std::vector<MergedSection>& sections, std::string_view name) {
  return std::any_of(sections.begin(), sections.end(),
                     [&](const MergedSection& sec) { return sec.name == name; });
}

std::vector<std::string> collect_plt_symbols(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    const std::map<std::string, GlobalSym>& global_syms) {
  std::vector<std::string> plt_symbols;
  for (const auto& [_, obj] : input_objs) {
    for (const auto& sec_relocs : obj.relocations) {
      for (const auto& reloc : sec_relocs) {
        if (reloc.rela_type != R_RISCV_CALL_PLT || reloc.sym_idx >= obj.symbols.size()) continue;
        const Symbol& sym = obj.symbols[reloc.sym_idx];
        if (sym.name.empty() || sym.binding() == STB_LOCAL) continue;
        const auto it = global_syms.find(sym.name);
        if (it != global_syms.end() && !it->second.defined &&
            !contains(plt_symbols, sym.name)) {
          plt_symbols.push_back(sym.name);
        }
      }
    }
  }
  return plt_symbols;
}

}  // namespace

bool emit_shared_library(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    std::vector<MergedSection>* merged_sections,
    std::map<std::string, std::size_t>* merged_map,
    const std::map<std::pair<std::size_t, std::size_t>, std::pair<std::size_t, std::uint64_t>>& sec_mapping,
    std::map<std::string, GlobalSym>* global_syms,
    const std::vector<std::string>& got_symbols,
    const std::unordered_set<std::string>& tls_got_symbols,
    const std::map<std::string, std::tuple<std::size_t, std::size_t, std::int64_t>>& local_got_sym_info,
    const std::vector<std::string>& needed_sonames,
    const std::optional<std::string>& soname,
    const std::string& output_path,
    std::string* error) {
  if (error != nullptr) error->clear();

  // Concrete helper work that is independent from the rest of the shared
  // linker surface.
  const std::vector<std::string> plt_symbols = collect_plt_symbols(input_objs, *global_syms);
  const std::uint64_t plt_header_size = plt_symbols.empty() ? 0 : 32;
  const std::uint64_t plt_entry_size = 16;
  const std::uint64_t plt_size = plt_symbols.empty()
                                     ? 0
                                     : plt_header_size + plt_symbols.size() * plt_entry_size;
  const std::uint64_t got_plt_entries = plt_symbols.empty() ? 0 : 2 + plt_symbols.size();
  const std::uint64_t got_plt_size = got_plt_entries * 8;
  const std::uint64_t rela_plt_size = plt_symbols.size() * 24;

  std::vector<std::size_t> sec_indices(merged_sections->size());
  for (std::size_t i = 0; i < merged_sections->size(); ++i) sec_indices[i] = i;
  std::sort(sec_indices.begin(), sec_indices.end(), [&](std::size_t lhs, std::size_t rhs) {
    return section_order((*merged_sections)[lhs].name, (*merged_sections)[lhs].sh_flags) <
           section_order((*merged_sections)[rhs].name, (*merged_sections)[rhs].sh_flags);
  });

  const bool has_init_array = contains_section_name(*merged_sections, ".init_array");
  const bool has_fini_array = contains_section_name(*merged_sections, ".fini_array");
  const bool has_tls = std::any_of(
      merged_sections->begin(), merged_sections->end(),
      [](const MergedSection& sec) { return (sec.sh_flags & SHF_TLS) != 0; });
  const bool has_riscv_attrs = contains_section_name(*merged_sections, ".riscv.attributes");

  const std::uint64_t dynamic_size = (needed_sonames.size() + 10 +
                                      (soname.has_value() ? 1u : 0u) +
                                      (has_init_array ? 2u : 0u) +
                                      (has_fini_array ? 2u : 0u) +
                                      (!plt_symbols.empty() ? 4u : 0u)) *
                                     16u;
  const std::uint64_t phdr_count = 8 + (has_tls ? 1u : 0u) + (has_riscv_attrs ? 1u : 0u);
  const std::uint64_t phdr_total_size = phdr_count * 56u;

  // Build the dynamic string table and GNU hash metadata exactly as the Rust
  // source does, but keep the full ELF emission path as a separate phase.
  std::vector<std::string> dyn_sym_names;
  std::vector<std::string> exported;
  exported.reserve(global_syms->size());
  for (const auto& [name, gsym] : *global_syms) {
    if (gsym.defined && gsym.binding != STB_LOCAL) exported.push_back(name);
  }
  std::sort(exported.begin(), exported.end());
  dyn_sym_names.insert(dyn_sym_names.end(), exported.begin(), exported.end());
  for (const auto& [name, gsym] : *global_syms) {
    if (!gsym.defined && !contains(dyn_sym_names, name)) dyn_sym_names.push_back(name);
  }

  DynStrTab dynstr;
  for (const auto& lib : needed_sonames) dynstr.add(lib);
  if (soname.has_value()) dynstr.add(*soname);
  for (const auto& name : dyn_sym_names) dynstr.add(name);

  const auto [gnu_hash_data, gnu_hash_order] = build_gnu_hash(dyn_sym_names);
  (void)gnu_hash_order;
  (void)gnu_hash_data;
  (void)plt_size;
  (void)got_plt_size;
  (void)rela_plt_size;
  (void)dynamic_size;
  (void)phdr_total_size;
  (void)sec_mapping;
  (void)tls_got_symbols;
  (void)local_got_sym_info;

  // Phase inventory from the Rust source:
  // - compute PLT/GOT/GNU-hash layout
  // - assign section virtual addresses
  // - update global symbol values and GOT offsets
  // - emit ELF header, program headers, dynamic table, .gnu.hash, .dynsym,
  //   .dynstr, section payloads, relocation tables, and section headers
  //
  // The concrete machinery for those phases belongs to the shared RISC-V
  // linker surface and the other linker subfiles. This translation unit keeps
  // the reusable layout helpers and phase boundaries in C++.

  if (output_path.empty()) {
    if (error != nullptr) *error = "emit_shared_library: output path is empty";
    return false;
  }

  // The full file emission path is not wired in this slice yet.
  if (error != nullptr) {
    *error = "emit_shared_library: shared RISC-V linker emission is not fully translated yet";
  }
  return false;
}

}  // namespace c4c::backend::riscv::linker

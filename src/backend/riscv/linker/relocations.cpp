// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/relocations.rs
// Self-contained RISC-V linker helpers and data types.

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <unordered_set>

namespace c4c::backend::riscv::linker {

// ── RISC-V relocation constants ─────────────────────────────────────────

inline constexpr std::uint32_t R_RISCV_32 = 1;
inline constexpr std::uint32_t R_RISCV_64 = 2;
inline constexpr std::uint32_t R_RISCV_BRANCH = 16;
inline constexpr std::uint32_t R_RISCV_JAL = 17;
inline constexpr std::uint32_t R_RISCV_CALL_PLT = 19;
inline constexpr std::uint32_t R_RISCV_GOT_HI20 = 20;
inline constexpr std::uint32_t R_RISCV_TLS_GOT_HI20 = 21;
inline constexpr std::uint32_t R_RISCV_TLS_GD_HI20 = 22;
inline constexpr std::uint32_t R_RISCV_PCREL_HI20 = 23;
inline constexpr std::uint32_t R_RISCV_PCREL_LO12_I = 24;
inline constexpr std::uint32_t R_RISCV_PCREL_LO12_S = 25;
inline constexpr std::uint32_t R_RISCV_HI20 = 26;
inline constexpr std::uint32_t R_RISCV_LO12_I = 27;
inline constexpr std::uint32_t R_RISCV_LO12_S = 28;
inline constexpr std::uint32_t R_RISCV_TPREL_HI20 = 29;
inline constexpr std::uint32_t R_RISCV_TPREL_LO12_I = 30;
inline constexpr std::uint32_t R_RISCV_TPREL_LO12_S = 31;
inline constexpr std::uint32_t R_RISCV_TPREL_ADD = 32;
inline constexpr std::uint32_t R_RISCV_ADD8 = 33;
inline constexpr std::uint32_t R_RISCV_ADD16 = 34;
inline constexpr std::uint32_t R_RISCV_ADD32 = 35;
inline constexpr std::uint32_t R_RISCV_ADD64 = 36;
inline constexpr std::uint32_t R_RISCV_SUB8 = 37;
inline constexpr std::uint32_t R_RISCV_SUB16 = 38;
inline constexpr std::uint32_t R_RISCV_SUB32 = 39;
inline constexpr std::uint32_t R_RISCV_SUB64 = 40;
inline constexpr std::uint32_t R_RISCV_ALIGN = 43;
inline constexpr std::uint32_t R_RISCV_RVC_BRANCH = 44;
inline constexpr std::uint32_t R_RISCV_RVC_JUMP = 45;
inline constexpr std::uint32_t R_RISCV_SUB6 = 52;
inline constexpr std::uint32_t R_RISCV_RELAX = 51;
inline constexpr std::uint32_t R_RISCV_SET6 = 53;
inline constexpr std::uint32_t R_RISCV_SET8 = 54;
inline constexpr std::uint32_t R_RISCV_SET16 = 55;
inline constexpr std::uint32_t R_RISCV_SET32 = 56;
inline constexpr std::uint32_t R_RISCV_32_PCREL = 57;
inline constexpr std::uint32_t R_RISCV_SET_ULEB128 = 60;
inline constexpr std::uint32_t R_RISCV_SUB_ULEB128 = 61;

// ELF section / symbol constants used by the linker helpers.
inline constexpr std::uint32_t SHT_NULL = 0;
inline constexpr std::uint32_t SHT_PROGBITS = 1;
inline constexpr std::uint32_t SHT_SYMTAB = 2;
inline constexpr std::uint32_t SHT_STRTAB = 3;
inline constexpr std::uint32_t SHT_RELA = 4;
inline constexpr std::uint32_t SHT_NOBITS = 8;
inline constexpr std::uint32_t SHT_GROUP = 17;
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
inline constexpr std::uint16_t SHN_ABS = 0xfff1;
inline constexpr std::uint16_t SHN_COMMON = 0xfff2;

namespace {

std::uint32_t gnu_hash_bytes(std::string_view text) {
  std::uint32_t value = 5381;
  for (unsigned char ch : text) {
    value = value * 33u + ch;
  }
  return value;
}

std::size_t next_power_of_two(std::size_t value) {
  if (value <= 1) return 1;
  --value;
  for (std::size_t shift = 1; shift < sizeof(std::size_t) * 8; shift <<= 1) {
    value |= value >> shift;
  }
  return value + 1;
}

bool has_prefix(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

bool has_suffix(std::string_view value, std::string_view suffix) {
  return value.size() >= suffix.size() &&
         value.substr(value.size() - suffix.size()) == suffix;
}

std::uint16_t read_u16_le(const std::vector<std::uint8_t>& data, std::size_t off) {
  if (off + 2 > data.size()) return 0;
  return static_cast<std::uint16_t>(data[off]) |
         static_cast<std::uint16_t>(data[off + 1]) << 8;
}

std::uint32_t read_u32_le(const std::vector<std::uint8_t>& data, std::size_t off) {
  if (off + 4 > data.size()) return 0;
  std::uint32_t value = 0;
  for (std::size_t i = 0; i < 4; ++i) {
    value |= static_cast<std::uint32_t>(data[off + i]) << (8 * i);
  }
  return value;
}

void write_u16_le(std::vector<std::uint8_t>* out, std::size_t off, std::uint16_t value) {
  if (off + 2 > out->size()) out->resize(off + 2, 0);
  (*out)[off] = static_cast<std::uint8_t>(value & 0xff);
  (*out)[off + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write_u32_le(std::vector<std::uint8_t>* out, std::size_t off, std::uint32_t value) {
  if (off + 4 > out->size()) out->resize(off + 4, 0);
  for (std::size_t i = 0; i < 4; ++i) {
    (*out)[off + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_u64_le(std::vector<std::uint8_t>* out, std::size_t off, std::uint64_t value) {
  if (off + 8 > out->size()) out->resize(off + 8, 0);
  for (std::size_t i = 0; i < 8; ++i) {
    (*out)[off + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void append_u16_le(std::vector<std::uint8_t>* out, std::uint16_t value) {
  out->push_back(static_cast<std::uint8_t>(value & 0xff));
  out->push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32_le(std::vector<std::uint8_t>* out, std::uint32_t value) {
  for (std::size_t i = 0; i < 4; ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
  }
}

void append_u64_le(std::vector<std::uint8_t>* out, std::uint64_t value) {
  for (std::size_t i = 0; i < 8; ++i) {
    out->push_back(static_cast<std::uint8_t>((value >> (8 * i)) & 0xff));
  }
}

void append_i64_le(std::vector<std::uint8_t>* out, std::int64_t value) {
  append_u64_le(out, static_cast<std::uint64_t>(value));
}

void ensure_size(std::vector<std::uint8_t>* out, std::size_t size) {
  if (out->size() < size) out->resize(size, 0);
}

}  // namespace

// ── Shared types ────────────────────────────────────────────────────────

struct Section {
  std::string name;
  std::uint32_t sh_type = SHT_NULL;
  std::uint64_t flags = 0;
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
  std::uint8_t visibility = 0;
  std::uint16_t shndx = SHN_UNDEF;

  [[nodiscard]] std::uint8_t sym_type() const { return type_; }
  [[nodiscard]] std::uint8_t binding() const { return binding_; }
  [[nodiscard]] bool is_local() const { return binding_ == STB_LOCAL; }
  [[nodiscard]] bool is_global() const { return binding_ == STB_GLOBAL; }
  [[nodiscard]] bool is_weak() const { return binding_ == STB_WEAK; }
  [[nodiscard]] bool is_undefined() const { return shndx == SHN_UNDEF; }
};

struct ElfObject {
  std::vector<Section> sections;
  std::vector<std::vector<Relocation>> relocations;
  std::vector<Symbol> symbols;
};

struct GlobalSym {
  std::uint64_t value = 0;
  std::uint64_t size = 0;
  std::uint8_t binding = STB_LOCAL;
  std::uint8_t sym_type = STT_NOTYPE;
  std::uint8_t visibility = 0;
  bool defined = false;
  bool needs_plt = false;
  std::size_t plt_idx = 0;
  std::optional<std::uint64_t> got_offset;
  std::optional<std::size_t> section_idx;
};

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = SHT_NULL;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
  std::uint64_t align = 1;
};

struct InputSecRef {
  std::size_t obj_idx = 0;
  std::size_t sec_idx = 0;
  std::size_t merged_sec_idx = 0;
  std::uint64_t offset_in_merged = 0;
};

using SectionKey = std::pair<std::size_t, std::size_t>;
using SectionMap = std::map<SectionKey, std::pair<std::size_t, std::uint64_t>>;
using LocalSymVaddrs = std::vector<std::vector<std::uint64_t>>;
using SectionVaddrs = std::vector<std::uint64_t>;

// ── Instruction patching ────────────────────────────────────────────────

void patch_u_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t hi = (value + 0x800u) & 0xFFFFF000u;
  write_u32_le(&data, off, (insn & 0xFFFu) | hi);
}

void patch_i_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t imm = value & 0xFFFu;
  write_u32_le(&data, off, (insn & 0x000FFFFFu) | (imm << 20));
}

void patch_s_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t imm = value & 0xFFFu;
  const std::uint32_t imm_hi = (imm >> 5) & 0x7Fu;
  const std::uint32_t imm_lo = imm & 0x1Fu;
  write_u32_le(&data, off, (insn & 0x01FFF07Fu) | (imm_hi << 25) | (imm_lo << 7));
}

void patch_b_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t bit12 = (value >> 12) & 1u;
  const std::uint32_t bits10_5 = (value >> 5) & 0x3Fu;
  const std::uint32_t bits4_1 = (value >> 1) & 0xFu;
  const std::uint32_t bit11 = (value >> 11) & 1u;
  write_u32_le(&data, off, (insn & 0x01FFF07Fu) | (bit12 << 31) | (bits10_5 << 25) |
                               (bits4_1 << 8) | (bit11 << 7));
}

void patch_j_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t bit20 = (value >> 20) & 1u;
  const std::uint32_t bits10_1 = (value >> 1) & 0x3FFu;
  const std::uint32_t bit11 = (value >> 11) & 1u;
  const std::uint32_t bits19_12 = (value >> 12) & 0xFFu;
  write_u32_le(&data, off, (insn & 0xFFFu) | (bit20 << 31) | (bits10_1 << 21) |
                               (bit11 << 20) | (bits19_12 << 12));
}

void patch_cb_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 2 > data.size()) return;
  const std::uint16_t insn = read_u16_le(data, off);
  const std::uint32_t bit8 = (value >> 8) & 1u;
  const std::uint32_t bits4_3 = (value >> 3) & 0x3u;
  const std::uint32_t bits7_6 = (value >> 6) & 0x3u;
  const std::uint32_t bits2_1 = (value >> 1) & 0x3u;
  const std::uint32_t bit5 = (value >> 5) & 1u;
  const std::uint16_t encoded = static_cast<std::uint16_t>(
      ((bit8 & 1u) << 12) | ((bits4_3 & 0x3u) << 10) | ((bits7_6 & 0x3u) << 5) |
      ((bits2_1 & 0x3u) << 3) | ((bit5 & 1u) << 2));
  write_u16_le(&data, off, static_cast<std::uint16_t>((insn & 0xE383u) | encoded));
}

void patch_cj_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 2 > data.size()) return;
  const std::uint16_t insn = read_u16_le(data, off);
  const std::uint32_t bit11 = (value >> 11) & 1u;
  const std::uint32_t bit4 = (value >> 4) & 1u;
  const std::uint32_t bits9_8 = (value >> 8) & 0x3u;
  const std::uint32_t bit10 = (value >> 10) & 1u;
  const std::uint32_t bit6 = (value >> 6) & 1u;
  const std::uint32_t bit7 = (value >> 7) & 1u;
  const std::uint32_t bits3_1 = (value >> 1) & 0x7u;
  const std::uint32_t bit5 = (value >> 5) & 1u;
  const std::uint16_t encoded = static_cast<std::uint16_t>(
      (bit11 << 10) | (bit4 << 9) | (bits9_8 << 7) | (bit10 << 6) | (bit6 << 5) |
      (bit7 << 4) | (bits3_1 << 1) | bit5);
  write_u16_le(&data, off, static_cast<std::uint16_t>((insn & 0xE003u) | (encoded << 2)));
}

// ── Symbol resolution helpers ────────────────────────────────────────────

std::uint64_t resolve_symbol_value(const Symbol& sym,
                                   std::size_t sym_idx,
                                   const ElfObject& obj,
                                   std::size_t obj_idx,
                                   const SectionMap& sec_mapping,
                                   const SectionVaddrs& section_vaddrs,
                                   const LocalSymVaddrs& local_sym_vaddrs,
                                   const std::map<std::string, GlobalSym>& global_syms) {
  if (sym.sym_type() == STT_SECTION) {
    if (static_cast<std::size_t>(sym.shndx) < obj.sections.size()) {
      const auto it = sec_mapping.find({obj_idx, static_cast<std::size_t>(sym.shndx)});
      if (it != sec_mapping.end() && it->second.first < section_vaddrs.size()) {
        return section_vaddrs[it->second.first] + it->second.second;
      }
    }
    return 0;
  }

  if (!sym.name.empty() && sym.binding() != STB_LOCAL) {
    const auto it = global_syms.find(sym.name);
    return it != global_syms.end() ? it->second.value : 0;
  }

  if (obj_idx < local_sym_vaddrs.size() && sym_idx < local_sym_vaddrs[obj_idx].size()) {
    return local_sym_vaddrs[obj_idx][sym_idx];
  }
  return 0;
}

std::pair<std::string, bool> got_sym_key(std::size_t obj_idx, const Symbol& sym, std::int64_t addend) {
  if (sym.sym_type() == STT_SECTION) {
    return {"__local_sec_" + std::to_string(obj_idx) + "_" +
                std::to_string(sym.shndx) + "_" + std::to_string(addend),
            true};
  }
  if (sym.binding() == STB_LOCAL) {
    return {"__local_" + std::to_string(obj_idx) + "_" + sym.name + "_" +
                std::to_string(addend),
            true};
  }
  return {sym.name, false};
}

std::int64_t find_hi20_value_core(const ElfObject& obj,
                                  std::size_t obj_idx,
                                  std::size_t sec_idx,
                                  const SectionMap& sec_mapping,
                                  const SectionVaddrs& section_vaddrs,
                                  const LocalSymVaddrs& local_sym_vaddrs,
                                  const std::map<std::string, GlobalSym>& global_syms,
                                  std::uint64_t auipc_vaddr,
                                  std::uint64_t sec_offset,
                                  std::uint64_t got_vaddr,
                                  const std::vector<std::string>& got_symbols,
                                  std::optional<std::uint64_t> got_plt_vaddr) {
  if (sec_idx >= obj.relocations.size()) {
    return 0;
  }

  const auto map_it = sec_mapping.find({obj_idx, sec_idx});
  if (map_it == sec_mapping.end() || map_it->second.first >= section_vaddrs.size()) {
    return 0;
  }
  const std::uint64_t section_base = section_vaddrs[map_it->second.first];

  for (const auto& reloc : obj.relocations[sec_idx]) {
    const std::uint64_t reloc_vaddr = sec_offset + reloc.offset;
    const std::uint64_t this_vaddr = section_base + reloc_vaddr;
    if (this_vaddr != auipc_vaddr) continue;

    if (reloc.rela_type == R_RISCV_PCREL_HI20) {
      const std::size_t hi_sym_idx = static_cast<std::size_t>(reloc.sym_idx);
      if (hi_sym_idx >= obj.symbols.size()) return 0;
      const Symbol& sym = obj.symbols[hi_sym_idx];
      const std::uint64_t sym_value = resolve_symbol_value(
          sym, hi_sym_idx, obj, obj_idx, sec_mapping, section_vaddrs, local_sym_vaddrs, global_syms);
      const std::int64_t target = static_cast<std::int64_t>(sym_value) + reloc.addend;
      return (target - static_cast<std::int64_t>(auipc_vaddr)) & 0xFFF;
    }

    if (reloc.rela_type == R_RISCV_GOT_HI20 || reloc.rela_type == R_RISCV_TLS_GOT_HI20) {
      const std::size_t hi_sym_idx = static_cast<std::size_t>(reloc.sym_idx);
      if (hi_sym_idx >= obj.symbols.size()) return 0;
      const Symbol& sym = obj.symbols[hi_sym_idx];
      const auto [sym_name, _is_local] = got_sym_key(obj_idx, sym, reloc.addend);

      std::uint64_t got_entry_vaddr = 0;
      if (got_plt_vaddr.has_value()) {
        const auto global_it = global_syms.find(sym.name);
        if (global_it != global_syms.end()) {
          const GlobalSym& gs = global_it->second;
          if (gs.got_offset.has_value()) {
            got_entry_vaddr = got_vaddr + *gs.got_offset;
          } else {
            got_entry_vaddr = *got_plt_vaddr + (2 + gs.plt_idx) * 8;
          }
        } else {
          const auto got_it = std::find(got_symbols.begin(), got_symbols.end(), sym_name);
          if (got_it != got_symbols.end()) {
            got_entry_vaddr = got_vaddr + static_cast<std::uint64_t>(got_it - got_symbols.begin()) * 8;
          }
        }
      } else {
        const auto got_it = std::find(got_symbols.begin(), got_symbols.end(), sym_name);
        if (got_it != got_symbols.end()) {
          got_entry_vaddr = got_vaddr + static_cast<std::uint64_t>(got_it - got_symbols.begin()) * 8;
        } else {
          const auto global_it = global_syms.find(sym.name);
          if (global_it != global_syms.end() && global_it->second.got_offset.has_value()) {
            got_entry_vaddr = got_vaddr + *global_it->second.got_offset;
          }
        }
      }

      return (static_cast<std::int64_t>(got_entry_vaddr) + reloc.addend -
              static_cast<std::int64_t>(auipc_vaddr)) & 0xFFF;
    }
  }

  return 0;
}

std::int64_t find_hi20_value(const ElfObject& obj,
                             std::size_t obj_idx,
                             std::size_t sec_idx,
                             const SectionMap& sec_mapping,
                             const SectionVaddrs& section_vaddrs,
                             const LocalSymVaddrs& local_sym_vaddrs,
                             const std::map<std::string, GlobalSym>& global_syms,
                             std::uint64_t auipc_vaddr,
                             std::uint64_t sec_offset,
                             std::uint64_t got_vaddr,
                             const std::vector<std::string>& got_symbols,
                             std::uint64_t got_plt_vaddr,
                             const std::map<std::uint64_t, std::pair<std::uint64_t, std::int64_t>>&
                                 gd_tls_relax_info,
                             std::uint64_t tls_vaddr) {
  const auto relax_it = gd_tls_relax_info.find(auipc_vaddr);
  if (relax_it != gd_tls_relax_info.end()) {
    const std::int64_t tprel = static_cast<std::int64_t>(relax_it->second.first + relax_it->second.second -
                                                         tls_vaddr);
    return tprel & 0xFFF;
  }

  return find_hi20_value_core(obj, obj_idx, sec_idx, sec_mapping, section_vaddrs,
                              local_sym_vaddrs, global_syms, auipc_vaddr, sec_offset,
                              got_vaddr, got_symbols, got_plt_vaddr);
}

std::int64_t find_hi20_value_shared(const ElfObject& obj,
                                    std::size_t obj_idx,
                                    std::size_t sec_idx,
                                    const SectionMap& sec_mapping,
                                    const SectionVaddrs& section_vaddrs,
                                    const LocalSymVaddrs& local_sym_vaddrs,
                                    const std::map<std::string, GlobalSym>& global_syms,
                                    std::uint64_t auipc_vaddr,
                                    std::uint64_t sec_offset,
                                    std::uint64_t got_vaddr,
                                    const std::vector<std::string>& got_symbols) {
  return find_hi20_value_core(obj, obj_idx, sec_idx, sec_mapping, section_vaddrs,
                              local_sym_vaddrs, global_syms, auipc_vaddr, sec_offset,
                              got_vaddr, got_symbols, std::nullopt);
}

// ── Section / ELF helpers ───────────────────────────────────────────────

[[nodiscard]] std::optional<std::string> output_section_name(std::string_view name,
                                                             std::uint32_t sh_type,
                                                             std::uint64_t sh_flags) {
  if ((sh_flags & SHF_ALLOC) == 0 && sh_type != SHT_RISCV_ATTRIBUTES) {
    return std::nullopt;
  }
  if (sh_type == SHT_GROUP) {
    return std::nullopt;
  }

  const std::pair<std::string_view, std::string_view> prefixes[] = {
      {".text.", ".text"},       {".rodata.", ".rodata"}, {".data.rel.ro", ".data"},
      {".data.", ".data"},       {".bss.", ".bss"},       {".tdata.", ".tdata"},
      {".tbss.", ".tbss"},       {".sdata.", ".sdata"},   {".sbss.", ".sbss"},
  };
  for (const auto& [prefix, output] : prefixes) {
    if (has_prefix(name, prefix)) {
      return std::string(output);
    }
  }

  for (std::string_view arr : {".init_array", ".fini_array", ".preinit_array"}) {
    if (name == arr || has_prefix(name, std::string(arr) + ".")) {
      return std::string(arr);
    }
  }

  return std::string(name);
}

void align_up(std::vector<std::uint8_t>* out, std::size_t alignment) {
  if (alignment == 0) return;
  const std::size_t mask = alignment - 1;
  const std::size_t new_size = (out->size() + mask) & ~mask;
  if (new_size > out->size()) out->resize(new_size, 0);
}

void pad_to(std::vector<std::uint8_t>* out, std::size_t target) {
  if (out->size() < target) out->resize(target, 0);
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
  ensure_size(out, offset + 64);
  write_u32_le(out, offset + 0, sh_name);
  write_u32_le(out, offset + 4, sh_type);
  write_u64_le(out, offset + 8, sh_flags);
  write_u64_le(out, offset + 16, sh_addr);
  write_u64_le(out, offset + 24, sh_offset);
  write_u64_le(out, offset + 32, sh_size);
  write_u32_le(out, offset + 40, sh_link);
  write_u32_le(out, offset + 44, sh_info);
  write_u64_le(out, offset + 48, sh_addralign);
  write_u64_le(out, offset + 56, sh_entsize);
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
  ensure_size(out, offset + 56);
  write_u32_le(out, offset + 0, p_type);
  write_u32_le(out, offset + 4, p_flags);
  write_u64_le(out, offset + 8, p_offset);
  write_u64_le(out, offset + 16, p_vaddr);
  write_u64_le(out, offset + 24, p_vaddr);
  write_u64_le(out, offset + 32, p_filesz);
  write_u64_le(out, offset + 40, p_memsz);
  write_u64_le(out, offset + 48, p_align);
}

void write_phdr_at(std::vector<std::uint8_t>* out,
                   std::size_t offset,
                   std::uint32_t p_type,
                   std::uint32_t p_flags,
                   std::uint64_t p_offset,
                   std::uint64_t p_vaddr,
                   std::uint64_t p_filesz,
                   std::uint64_t p_memsz,
                   std::uint64_t p_align) {
  write_phdr(out, offset, p_type, p_flags, p_offset, p_vaddr, p_filesz, p_memsz, p_align);
}

// ── GNU hash / archive helpers ──────────────────────────────────────────

[[nodiscard]] std::pair<std::vector<std::uint8_t>, std::vector<std::size_t>> build_gnu_hash(
    const std::vector<std::string>& sym_names) {
  const std::size_t nsyms = sym_names.size();
  if (nsyms == 0) {
    std::vector<std::uint8_t> data;
    append_u32_le(&data, 1);
    append_u32_le(&data, 1);
    append_u32_le(&data, 1);
    append_u32_le(&data, 6);
    append_u64_le(&data, 0);
    append_u32_le(&data, 0);
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
  const std::size_t bloom_size =
      next_power_of_two(std::max<std::size_t>(1, nsyms / 2));
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
  append_u32_le(&data, nbuckets);
  append_u32_le(&data, symoffset);
  append_u32_le(&data, static_cast<std::uint32_t>(bloom.size()));
  append_u32_le(&data, bloom_shift);
  for (const auto word : bloom) append_u64_le(&data, word);
  for (const auto bucket : buckets) append_u32_le(&data, bucket);
  for (const auto value : chain) append_u32_le(&data, value);

  return {std::move(data), std::move(indices)};
}

[[nodiscard]] std::optional<std::string> find_versioned_soname(std::string_view dir,
                                                               std::string_view libname) {
  const std::string prefix = "lib" + std::string(libname) + ".so.";
  std::optional<std::string> best;

  std::error_code ec;
  std::filesystem::directory_iterator it(std::filesystem::path(dir), ec);
  if (ec) return std::nullopt;

  for (const auto& entry : it) {
    const std::string name = entry.path().filename().string();
    if (has_prefix(name, prefix) &&
        (!best.has_value() || name.size() < best->size())) {
      best = name;
    }
  }
  return best;
}

void resolve_archive_members(std::vector<std::pair<std::string, ElfObject>> members,
                             std::vector<std::pair<std::string, ElfObject>>* input_objs,
                             std::unordered_set<std::string>* defined_syms,
                             std::unordered_set<std::string>* undefined_syms) {
  std::vector<std::pair<std::string, ElfObject>> pool = std::move(members);
  while (true) {
    bool added_any = false;
    std::vector<std::pair<std::string, ElfObject>> remaining;
    for (const auto& member : pool) {
      const std::string& name = member.first;
      const ElfObject& obj = member.second;

      const bool needed = std::any_of(obj.symbols.begin(), obj.symbols.end(), [&](const Symbol& sym) {
        return sym.shndx != SHN_UNDEF && sym.binding() != STB_LOCAL && !sym.name.empty() &&
               undefined_syms->find(sym.name) != undefined_syms->end();
      });

      if (needed) {
        for (const Symbol& sym : obj.symbols) {
          if (sym.shndx != SHN_UNDEF && sym.binding() != STB_LOCAL && !sym.name.empty()) {
            defined_syms->insert(sym.name);
            undefined_syms->erase(sym.name);
          }
        }
        for (const Symbol& sym : obj.symbols) {
          if (sym.shndx == SHN_UNDEF && !sym.name.empty() && sym.binding() != STB_LOCAL &&
              defined_syms->find(sym.name) == defined_syms->end()) {
            undefined_syms->insert(sym.name);
          }
        }
        input_objs->emplace_back(name, obj);
        added_any = true;
      } else {
        remaining.emplace_back(name, obj);
      }
    }

    if (!added_any || remaining.empty()) break;
    pool = std::move(remaining);
  }
}

[[nodiscard]] std::uint64_t section_order(std::string_view name, std::uint64_t flags) {
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
  if ((flags & SHF_EXECINSTR) != 0) return 150;
  if ((flags & SHF_WRITE) == 0) return 300;
  return 600;
}

// ── ULEB128 helpers ─────────────────────────────────────────────────────

[[nodiscard]] std::uint64_t decode_uleb128(const std::vector<std::uint8_t>& data,
                                           std::size_t off) {
  std::uint64_t result = 0;
  std::uint32_t shift = 0;
  std::size_t i = off;
  while (i < data.size()) {
    const std::uint8_t byte = data[i];
    result |= static_cast<std::uint64_t>(byte & 0x7Fu) << shift;
    if ((byte & 0x80u) == 0) break;
    shift += 7;
    ++i;
  }
  return result;
}

void encode_uleb128_in_place(std::vector<std::uint8_t>* data,
                             std::size_t off,
                             std::uint64_t value) {
  std::size_t num_bytes = 0;
  std::size_t i = off;
  while (i < data->size()) {
    ++num_bytes;
    if (((*data)[i] & 0x80u) == 0) break;
    ++i;
  }

  std::uint64_t val = value;
  for (std::size_t j = 0; j < num_bytes; ++j) {
    const std::size_t idx = off + j;
    if (idx >= data->size()) break;
    std::uint8_t byte = static_cast<std::uint8_t>(val & 0x7Fu);
    val >>= 7;
    if (j + 1 < num_bytes) {
      byte |= 0x80u;
    }
    (*data)[idx] = byte;
  }
}

// The Rust module re-exports ELF writing helpers from linker_common:
// write_shdr, write_phdr, write_phdr_at, align_up, pad_to.
// In this translation unit we provide local C++ equivalents above so the
// linker helper surface remains self-contained.

}  // namespace c4c::backend::riscv::linker

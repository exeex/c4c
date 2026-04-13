// Translated from src/backend/riscv/linker/reloc.rs
// Self-contained relocation application pass for the RISC-V linker.

#include "mod.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::linker {

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
inline constexpr std::uint32_t R_RISCV_RELAX = 51;
inline constexpr std::uint32_t R_RISCV_SUB6 = 52;
inline constexpr std::uint32_t R_RISCV_SET6 = 53;
inline constexpr std::uint32_t R_RISCV_SET8 = 54;
inline constexpr std::uint32_t R_RISCV_SET16 = 55;
inline constexpr std::uint32_t R_RISCV_SET32 = 56;
inline constexpr std::uint32_t R_RISCV_32_PCREL = 57;
inline constexpr std::uint32_t R_RISCV_SET_ULEB128 = 60;
inline constexpr std::uint32_t R_RISCV_SUB_ULEB128 = 61;

inline constexpr std::uint8_t STB_LOCAL = 0;
inline constexpr std::uint8_t STB_GLOBAL = 1;
inline constexpr std::uint8_t STT_SECTION = 3;

struct Section {
  std::string name;
  std::uint32_t sh_type = 0;
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
  std::uint8_t type_ = 0;
  std::uint8_t visibility = 0;
  std::uint16_t shndx = 0;

  [[nodiscard]] std::uint8_t sym_type() const { return type_; }
  [[nodiscard]] std::uint8_t binding() const { return binding_; }
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
  std::uint8_t sym_type = 0;
  std::uint8_t visibility = 0;
  bool defined = false;
  bool needs_plt = false;
  std::size_t plt_idx = 0;
  std::optional<std::uint64_t> got_offset;
  std::optional<std::size_t> section_idx;
};

struct MergedSection {
  std::string name;
  std::uint32_t sh_type = 0;
  std::uint64_t sh_flags = 0;
  std::vector<std::uint8_t> data;
  std::uint64_t vaddr = 0;
};

using SectionKey = std::pair<std::size_t, std::size_t>;
using SectionMap = std::map<SectionKey, std::pair<std::size_t, std::uint64_t>>;
using SectionVaddrs = std::vector<std::uint64_t>;
using LocalSymVaddrs = std::vector<std::vector<std::uint64_t>>;
using GlobalSymMap = std::map<std::string, GlobalSym>;
using GotOffsetMap = std::map<std::string, std::uint64_t>;
using PltAddrMap = std::map<std::string, std::uint64_t>;
using GDRelaxMap = std::map<std::uint64_t, std::pair<std::uint64_t, std::int64_t>>;

struct RelocContext {
  const SectionMap& sec_mapping;
  const SectionVaddrs& section_vaddrs;
  const LocalSymVaddrs& local_sym_vaddrs;
  const GlobalSymMap& global_syms;
  std::uint64_t got_vaddr = 0;
  const std::vector<std::string>& got_symbols;
  std::uint64_t got_plt_vaddr = 0;
  std::uint64_t tls_vaddr = 0;
  const GDRelaxMap& gd_tls_relax_info;
  const std::unordered_set<std::uint64_t>& gd_tls_call_nop;
  bool collect_relatives = false;
  const GotOffsetMap& got_sym_offsets;
  const PltAddrMap& plt_sym_addrs;
};

struct RelocResult {
  std::vector<std::pair<std::uint64_t, std::uint64_t>> relative_entries;
};

[[nodiscard]] std::optional<RelocResult> apply_relocations(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    std::vector<MergedSection>* merged_sections,
    const RelocContext& ctx,
    std::string* error);

namespace {

bool has_prefix(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

std::optional<std::size_t> find_text_section_index(const linker_common::Elf64Object& object) {
  for (std::size_t index = 0; index < object.sections.size(); ++index) {
    if (object.sections[index].name == ".text") return index;
  }
  return std::nullopt;
}

Section convert_section(const linker_common::Elf64Section& section) {
  return Section{
      .name = section.name,
      .sh_type = section.sh_type,
      .flags = section.flags,
  };
}

Relocation convert_relocation(const linker_common::Elf64Rela& relocation) {
  return Relocation{
      .offset = relocation.offset,
      .rela_type = relocation.rela_type,
      .sym_idx = relocation.sym_idx,
      .addend = relocation.addend,
  };
}

Symbol convert_symbol(const linker_common::Elf64Symbol& symbol) {
  return Symbol{
      .name = symbol.name,
      .value = symbol.value,
      .size = symbol.size,
      .binding_ = symbol.binding(),
      .type_ = symbol.sym_type(),
      .visibility = symbol.visibility(),
      .shndx = symbol.shndx,
  };
}

ElfObject convert_object(const linker_common::Elf64Object& object) {
  ElfObject converted;
  converted.sections.reserve(object.sections.size());
  for (const auto& section : object.sections) {
    converted.sections.push_back(convert_section(section));
  }
  converted.relocations.reserve(object.relocations.size());
  for (const auto& section_relocations : object.relocations) {
    std::vector<Relocation> converted_relocations;
    converted_relocations.reserve(section_relocations.size());
    for (const auto& relocation : section_relocations) {
      converted_relocations.push_back(convert_relocation(relocation));
    }
    converted.relocations.push_back(std::move(converted_relocations));
  }
  converted.symbols.reserve(object.symbols.size());
  for (const auto& symbol : object.symbols) {
    converted.symbols.push_back(convert_symbol(symbol));
  }
  return converted;
}

LocalSymVaddrs build_local_sym_vaddrs(const std::vector<std::pair<std::string, ElfObject>>& input_objs,
                                      const SectionMap& sec_mapping,
                                      const SectionVaddrs& section_vaddrs) {
  LocalSymVaddrs local_sym_vaddrs(input_objs.size());
  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& object = input_objs[obj_idx].second;
    auto& symbol_vaddrs = local_sym_vaddrs[obj_idx];
    symbol_vaddrs.resize(object.symbols.size(), 0);
    for (std::size_t sym_idx = 0; sym_idx < object.symbols.size(); ++sym_idx) {
      const auto& symbol = object.symbols[sym_idx];
      const auto section_index = static_cast<std::size_t>(symbol.shndx);
      if (symbol.shndx == 0 || section_index >= object.sections.size()) continue;
      const auto mapping = sec_mapping.find({obj_idx, section_index});
      if (mapping == sec_mapping.end() || mapping->second.first >= section_vaddrs.size()) continue;
      symbol_vaddrs[sym_idx] =
          section_vaddrs[mapping->second.first] + mapping->second.second + symbol.value;
    }
  }
  return local_sym_vaddrs;
}

GlobalSymMap build_global_symbol_map(
    const std::vector<LoadedInputObject>& loaded_objects,
    const std::unordered_map<std::string, std::uint64_t>& symbol_addresses) {
  GlobalSymMap global_syms;
  for (const auto& loaded_object : loaded_objects) {
    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || symbol.is_undefined() ||
          (!symbol.is_global() && !symbol.is_weak())) {
        continue;
      }
      const auto address = symbol_addresses.find(symbol.name);
      if (address == symbol_addresses.end()) continue;
      global_syms.emplace(symbol.name, GlobalSym{
                                           .value = address->second,
                                           .size = symbol.size,
                                           .binding = symbol.binding(),
                                           .sym_type = symbol.sym_type(),
                                           .visibility = symbol.visibility(),
                                           .defined = true,
                                       });
    }
  }
  return global_syms;
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

void write_u16_le(std::vector<std::uint8_t>& data, std::size_t off, std::uint16_t value) {
  if (off + 2 > data.size()) return;
  data[off] = static_cast<std::uint8_t>(value & 0xff);
  data[off + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write_u32_le(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  for (std::size_t i = 0; i < 4; ++i) {
    data[off + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void write_u64_le(std::vector<std::uint8_t>& data, std::size_t off, std::uint64_t value) {
  if (off + 8 > data.size()) return;
  for (std::size_t i = 0; i < 8; ++i) {
    data[off + i] = static_cast<std::uint8_t>((value >> (8 * i)) & 0xff);
  }
}

void patch_u_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t hi = (value + 0x800u) & 0xFFFFF000u;
  write_u32_le(data, off, (insn & 0xFFFu) | hi);
}

void patch_i_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t imm = value & 0xFFFu;
  write_u32_le(data, off, (insn & 0x000FFFFFu) | (imm << 20));
}

void patch_s_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t imm = value & 0xFFFu;
  const std::uint32_t imm_hi = (imm >> 5) & 0x7Fu;
  const std::uint32_t imm_lo = imm & 0x1Fu;
  write_u32_le(data, off, (insn & 0x01FFF07Fu) | (imm_hi << 25) | (imm_lo << 7));
}

void patch_b_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t bit12 = (value >> 12) & 1u;
  const std::uint32_t bits10_5 = (value >> 5) & 0x3Fu;
  const std::uint32_t bits4_1 = (value >> 1) & 0xFu;
  const std::uint32_t bit11 = (value >> 11) & 1u;
  write_u32_le(data, off, (insn & 0x01FFF07Fu) | (bit12 << 31) | (bits10_5 << 25) |
                               (bits4_1 << 8) | (bit11 << 7));
}

void patch_j_type(std::vector<std::uint8_t>& data, std::size_t off, std::uint32_t value) {
  if (off + 4 > data.size()) return;
  const std::uint32_t insn = read_u32_le(data, off);
  const std::uint32_t bit20 = (value >> 20) & 1u;
  const std::uint32_t bits10_1 = (value >> 1) & 0x3FFu;
  const std::uint32_t bit11 = (value >> 11) & 1u;
  const std::uint32_t bits19_12 = (value >> 12) & 0xFFu;
  write_u32_le(data, off, (insn & 0xFFFu) | (bit20 << 31) | (bits10_1 << 21) |
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
  write_u16_le(data, off, static_cast<std::uint16_t>((insn & 0xE383u) | encoded));
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
  write_u16_le(data, off, static_cast<std::uint16_t>((insn & 0xE003u) | (encoded << 2)));
}

std::pair<std::string, bool> got_sym_key(std::size_t obj_idx, const Symbol& sym,
                                         std::int64_t addend) {
  if (sym.sym_type() == STT_SECTION) {
    return {"__local_sec_" + std::to_string(obj_idx) + "_" + std::to_string(sym.shndx) + "_" +
                std::to_string(addend),
            true};
  }
  if (sym.binding() == STB_LOCAL) {
    return {"__local_" + std::to_string(obj_idx) + "_" + sym.name + "_" +
                std::to_string(addend),
            true};
  }
  return {sym.name, false};
}

std::uint64_t resolve_symbol_value(
    const Symbol& sym,
    std::size_t sym_idx,
    std::size_t obj_idx,
    const ElfObject& obj,
    const SectionMap& sec_mapping,
    const SectionVaddrs& section_vaddrs,
    const LocalSymVaddrs& local_sym_vaddrs,
    const GlobalSymMap& global_syms) {
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
    if (it != global_syms.end()) {
      const GlobalSym& gs = it->second;
      if (gs.needs_plt || gs.defined) {
        return gs.value;
      }
    }
    return 0;
  }

  if (obj_idx < local_sym_vaddrs.size() && sym_idx < local_sym_vaddrs[obj_idx].size()) {
    return local_sym_vaddrs[obj_idx][sym_idx];
  }
  return 0;
}

std::uint64_t lookup_got_entry(const Symbol& sym,
                               std::size_t obj_idx,
                               std::int64_t addend,
                               const RelocContext& ctx) {
  const auto [sym_name, _] = got_sym_key(obj_idx, sym, addend);

  if (const auto it = ctx.got_sym_offsets.find(sym_name); it != ctx.got_sym_offsets.end()) {
    return ctx.got_vaddr + it->second;
  }

  if (const auto it = ctx.global_syms.find(sym.name); it != ctx.global_syms.end()) {
    const GlobalSym& gs = it->second;
    if (gs.got_offset.has_value()) {
      return ctx.got_vaddr + *gs.got_offset;
    }
    if (gs.needs_plt) {
      return ctx.got_plt_vaddr + (2 + gs.plt_idx) * 8;
    }
  }

  if (const auto it = std::find(ctx.got_symbols.begin(), ctx.got_symbols.end(), sym_name);
      it != ctx.got_symbols.end()) {
    return ctx.got_vaddr + static_cast<std::uint64_t>(it - ctx.got_symbols.begin()) * 8;
  }

  return 0;
}

std::int64_t find_hi20_value_core(
    const ElfObject& obj,
    std::size_t obj_idx,
    std::size_t sec_idx,
    const SectionMap& sec_mapping,
    const SectionVaddrs& section_vaddrs,
    const LocalSymVaddrs& local_sym_vaddrs,
    const GlobalSymMap& global_syms,
    std::uint64_t auipc_vaddr,
    std::uint64_t sec_offset,
    std::uint64_t got_vaddr,
    const std::vector<std::string>& got_symbols,
    std::optional<std::uint64_t> got_plt_vaddr) {
  if (sec_idx >= obj.relocations.size()) return 0;

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
          sym, hi_sym_idx, obj_idx, obj, sec_mapping, section_vaddrs, local_sym_vaddrs,
          global_syms);
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
            got_entry_vaddr =
                got_vaddr + static_cast<std::uint64_t>(got_it - got_symbols.begin()) * 8;
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

std::int64_t find_hi20_value_executable(
    const ElfObject& obj,
    std::size_t obj_idx,
    std::size_t sec_idx,
    const SectionMap& sec_mapping,
    const SectionVaddrs& section_vaddrs,
    const LocalSymVaddrs& local_sym_vaddrs,
    const GlobalSymMap& global_syms,
    std::uint64_t auipc_vaddr,
    std::uint64_t sec_offset,
    std::uint64_t got_vaddr,
    const std::vector<std::string>& got_symbols,
    std::uint64_t got_plt_vaddr,
    const GDRelaxMap& gd_tls_relax_info,
    std::uint64_t tls_vaddr) {
  if (const auto relax_it = gd_tls_relax_info.find(auipc_vaddr);
      relax_it != gd_tls_relax_info.end()) {
    const std::int64_t tprel =
        static_cast<std::int64_t>(relax_it->second.first + relax_it->second.second - tls_vaddr);
    return tprel & 0xFFF;
  }

  return find_hi20_value_core(obj, obj_idx, sec_idx, sec_mapping, section_vaddrs,
                              local_sym_vaddrs, global_syms, auipc_vaddr, sec_offset,
                              got_vaddr, got_symbols, got_plt_vaddr);
}

std::int64_t find_hi20_value_shared(
    const ElfObject& obj,
    std::size_t obj_idx,
    std::size_t sec_idx,
    const SectionMap& sec_mapping,
    const SectionVaddrs& section_vaddrs,
    const LocalSymVaddrs& local_sym_vaddrs,
    const GlobalSymMap& global_syms,
    std::uint64_t auipc_vaddr,
    std::uint64_t sec_offset,
    std::uint64_t got_vaddr,
    const std::vector<std::string>& got_symbols) {
  return find_hi20_value_core(obj, obj_idx, sec_idx, sec_mapping, section_vaddrs,
                              local_sym_vaddrs, global_syms, auipc_vaddr, sec_offset,
                              got_vaddr, got_symbols, std::nullopt);
}

std::int64_t find_hi20_value_for_reloc(const ElfObject& obj,
                                       std::size_t obj_idx,
                                       std::size_t sec_idx,
                                       const RelocContext& ctx,
                                       std::uint64_t auipc_vaddr,
                                       std::uint64_t sec_offset) {
  if (ctx.collect_relatives) {
    return find_hi20_value_shared(obj, obj_idx, sec_idx, ctx.sec_mapping, ctx.section_vaddrs,
                                  ctx.local_sym_vaddrs, ctx.global_syms, auipc_vaddr,
                                  sec_offset, ctx.got_vaddr, ctx.got_symbols);
  }

  return find_hi20_value_executable(obj, obj_idx, sec_idx, ctx.sec_mapping, ctx.section_vaddrs,
                                    ctx.local_sym_vaddrs, ctx.global_syms, auipc_vaddr,
                                    sec_offset, ctx.got_vaddr, ctx.got_symbols, ctx.got_plt_vaddr,
                                    ctx.gd_tls_relax_info, ctx.tls_vaddr);
}

void apply_set_uleb128(std::vector<std::uint8_t>& data, std::size_t off, std::uint64_t val) {
  std::uint64_t v = val;
  std::size_t i = off;
  while (i < data.size()) {
    const std::uint8_t byte = static_cast<std::uint8_t>(v & 0x7Fu);
    v >>= 7;
    if (v != 0) {
      data[i] = byte | 0x80u;
    } else {
      data[i] = byte;
      break;
    }
    ++i;
  }
}

void apply_sub_uleb128(std::vector<std::uint8_t>& data, std::size_t off, std::uint64_t sub_val) {
  std::uint64_t cur = 0;
  std::size_t shift = 0;
  std::size_t i = off;
  while (i < data.size()) {
    const std::uint8_t byte = data[i];
    cur |= static_cast<std::uint64_t>(byte & 0x7Fu) << shift;
    if ((byte & 0x80u) == 0) break;
    shift += 7;
    ++i;
  }

  std::uint64_t v = cur - sub_val;
  std::size_t j = off;
  while (j < data.size()) {
    const std::uint8_t byte = static_cast<std::uint8_t>(v & 0x7Fu);
    v >>= 7;
    if (v != 0) {
      data[j] = byte | 0x80u;
    } else {
      data[j] = byte;
      break;
    }
    ++j;
  }
}

bool apply_one_reloc(std::uint32_t rela_type,
                     std::vector<std::uint8_t>& data,
                     std::size_t off,
                     std::uint64_t s,
                     std::int64_t a,
                     std::uint64_t p,
                     const Symbol& sym,
                     std::size_t obj_idx,
                     const ElfObject& obj,
                     std::size_t sec_idx,
                     std::uint64_t sec_offset,
                     const std::string& obj_name,
                     const RelocContext& ctx,
                     RelocResult* result,
                     std::string* error) {
  const auto fail = [&](const std::string& message) -> bool {
    if (error != nullptr) *error = message;
    return false;
  };

  switch (rela_type) {
    case R_RISCV_RELAX:
    case R_RISCV_ALIGN:
      break;

    case R_RISCV_64: {
      const auto val = static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a);
      if (off + 8 <= data.size()) {
        write_u64_le(data, off, val);
        if (ctx.collect_relatives && s != 0 && result != nullptr) {
          result->relative_entries.emplace_back(p, val);
        }
      }
      break;
    }
    case R_RISCV_32: {
      const auto val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
      if (off + 4 <= data.size()) write_u32_le(data, off, val);
      break;
    }
    case R_RISCV_PCREL_HI20: {
      const std::int64_t target = static_cast<std::int64_t>(s) + a;
      const std::int64_t offset_val = target - static_cast<std::int64_t>(p);
      patch_u_type(data, off, static_cast<std::uint32_t>(offset_val));
      break;
    }
    case R_RISCV_PCREL_LO12_I: {
      const std::uint64_t auipc_addr = static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a);
      const std::int64_t hi_val =
          find_hi20_value_for_reloc(obj, obj_idx, sec_idx, ctx, auipc_addr, sec_offset);
      patch_i_type(data, off, static_cast<std::uint32_t>(hi_val));
      break;
    }
    case R_RISCV_PCREL_LO12_S: {
      const std::uint64_t auipc_addr = static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a);
      const std::int64_t hi_val =
          find_hi20_value_for_reloc(obj, obj_idx, sec_idx, ctx, auipc_addr, sec_offset);
      patch_s_type(data, off, static_cast<std::uint32_t>(hi_val));
      break;
    }
    case R_RISCV_GOT_HI20: {
      const std::uint64_t got_entry_vaddr = lookup_got_entry(sym, obj_idx, a, ctx);
      const std::int64_t offset_val =
          static_cast<std::int64_t>(got_entry_vaddr) + a - static_cast<std::int64_t>(p);
      patch_u_type(data, off, static_cast<std::uint32_t>(offset_val));
      break;
    }
    case R_RISCV_CALL_PLT: {
      if (ctx.gd_tls_call_nop.find(p) != ctx.gd_tls_call_nop.end()) {
        if (off + 8 <= data.size()) {
          write_u32_le(data, off, 0x00450533u);
          write_u32_le(data, off + 4, 0x00000013u);
        }
      } else {
        std::uint64_t target = s;
        if (!sym.name.empty() && sym.binding() != STB_LOCAL) {
          if (const auto plt_it = ctx.plt_sym_addrs.find(sym.name);
              plt_it != ctx.plt_sym_addrs.end()) {
            target = plt_it->second;
          } else if (const auto gs_it = ctx.global_syms.find(sym.name);
                     gs_it != ctx.global_syms.end()) {
            target = gs_it->second.value;
          }
        }

        const std::int64_t offset_val = static_cast<std::int64_t>(target) + a -
                                        static_cast<std::int64_t>(p);
        const std::uint32_t hi = static_cast<std::uint32_t>(((offset_val + 0x800) >> 12) & 0xFFFFF);
        const std::uint32_t lo = static_cast<std::uint32_t>(offset_val & 0xFFF);
        if (off + 8 <= data.size()) {
          const std::uint32_t auipc = read_u32_le(data, off);
          write_u32_le(data, off, (auipc & 0xFFFu) | (hi << 12));
          const std::uint32_t jalr = read_u32_le(data, off + 4);
          write_u32_le(data, off + 4, (jalr & 0x000FFFFFu) | (lo << 20));
        }
      }
      break;
    }
    case R_RISCV_BRANCH: {
      const auto offset_val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                         static_cast<std::int64_t>(p));
      patch_b_type(data, off, offset_val);
      break;
    }
    case R_RISCV_JAL: {
      const auto offset_val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                         static_cast<std::int64_t>(p));
      patch_j_type(data, off, offset_val);
      break;
    }
    case R_RISCV_RVC_BRANCH: {
      const auto offset_val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                         static_cast<std::int64_t>(p));
      patch_cb_type(data, off, offset_val);
      break;
    }
    case R_RISCV_RVC_JUMP: {
      const auto offset_val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                         static_cast<std::int64_t>(p));
      patch_cj_type(data, off, offset_val);
      break;
    }
    case R_RISCV_HI20: {
      const std::uint32_t val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
      const std::uint32_t hi = (val + 0x800u) & 0xFFFFF000u;
      if (off + 4 <= data.size()) {
        const std::uint32_t insn = read_u32_le(data, off);
        write_u32_le(data, off, (insn & 0xFFFu) | hi);
      }
      break;
    }
    case R_RISCV_LO12_I: {
      const auto val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
      patch_i_type(data, off, val & 0xFFFu);
      break;
    }
    case R_RISCV_LO12_S: {
      const auto val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
      patch_s_type(data, off, val & 0xFFFu);
      break;
    }
    case R_RISCV_TPREL_HI20: {
      const std::uint32_t val =
          static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                     static_cast<std::int64_t>(ctx.tls_vaddr));
      const std::uint32_t hi = (val + 0x800u) & 0xFFFFF000u;
      if (off + 4 <= data.size()) {
        const std::uint32_t insn = read_u32_le(data, off);
        write_u32_le(data, off, (insn & 0xFFFu) | hi);
      }
      break;
    }
    case R_RISCV_TPREL_LO12_I: {
      const auto val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                   static_cast<std::int64_t>(ctx.tls_vaddr));
      patch_i_type(data, off, val & 0xFFFu);
      break;
    }
    case R_RISCV_TPREL_LO12_S: {
      const auto val = static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                   static_cast<std::int64_t>(ctx.tls_vaddr));
      patch_s_type(data, off, val & 0xFFFu);
      break;
    }
    case R_RISCV_TPREL_ADD:
      break;

    case R_RISCV_ADD8: {
      if (off < data.size()) {
        const std::uint8_t delta = static_cast<std::uint8_t>(static_cast<std::int64_t>(s) + a);
        data[off] = static_cast<std::uint8_t>(data[off] + delta);
      }
      break;
    }
    case R_RISCV_ADD16: {
      if (off + 2 <= data.size()) {
        const std::uint16_t cur = read_u16_le(data, off);
        const std::uint16_t val =
            static_cast<std::uint16_t>(cur +
                                       static_cast<std::uint16_t>(static_cast<std::int64_t>(s) + a));
        write_u16_le(data, off, val);
      }
      break;
    }
    case R_RISCV_ADD32: {
      if (off + 4 <= data.size()) {
        const std::uint32_t cur = read_u32_le(data, off);
        const std::uint32_t val =
            cur + static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
        write_u32_le(data, off, val);
      }
      break;
    }
    case R_RISCV_ADD64: {
      if (off + 8 <= data.size()) {
        std::uint64_t cur = 0;
        for (std::size_t i = 0; i < 8; ++i) {
          cur |= static_cast<std::uint64_t>(data[off + i]) << (8 * i);
        }
        const std::uint64_t val =
            cur + static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a);
        write_u64_le(data, off, val);
      }
      break;
    }
    case R_RISCV_SUB8: {
      if (off < data.size()) {
        const std::uint8_t delta = static_cast<std::uint8_t>(static_cast<std::int64_t>(s) + a);
        data[off] = static_cast<std::uint8_t>(data[off] - delta);
      }
      break;
    }
    case R_RISCV_SUB16: {
      if (off + 2 <= data.size()) {
        const std::uint16_t cur = read_u16_le(data, off);
        const std::uint16_t val =
            static_cast<std::uint16_t>(cur -
                                       static_cast<std::uint16_t>(static_cast<std::int64_t>(s) + a));
        write_u16_le(data, off, val);
      }
      break;
    }
    case R_RISCV_SUB32: {
      if (off + 4 <= data.size()) {
        const std::uint32_t cur = read_u32_le(data, off);
        const std::uint32_t val =
            cur - static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a);
        write_u32_le(data, off, val);
      }
      break;
    }
    case R_RISCV_SUB64: {
      if (off + 8 <= data.size()) {
        std::uint64_t cur = 0;
        for (std::size_t i = 0; i < 8; ++i) {
          cur |= static_cast<std::uint64_t>(data[off + i]) << (8 * i);
        }
        const std::uint64_t val =
            cur - static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a);
        write_u64_le(data, off, val);
      }
      break;
    }
    case R_RISCV_SET6: {
      if (off < data.size()) {
        const std::uint8_t delta = static_cast<std::uint8_t>(static_cast<std::int64_t>(s) + a);
        data[off] = static_cast<std::uint8_t>((data[off] & 0xC0u) | (delta & 0x3Fu));
      }
      break;
    }
    case R_RISCV_SUB6: {
      if (off < data.size()) {
        const std::uint8_t delta = static_cast<std::uint8_t>(static_cast<std::int64_t>(s) + a);
        const std::uint8_t cur = data[off] & 0x3Fu;
        const std::uint8_t val = static_cast<std::uint8_t>(cur - delta);
        data[off] = static_cast<std::uint8_t>((data[off] & 0xC0u) | (val & 0x3Fu));
      }
      break;
    }
    case R_RISCV_SET8:
      if (off < data.size()) {
        data[off] = static_cast<std::uint8_t>(static_cast<std::int64_t>(s) + a);
      }
      break;
    case R_RISCV_SET16:
      if (off + 2 <= data.size()) {
        write_u16_le(data, off, static_cast<std::uint16_t>(static_cast<std::int64_t>(s) + a));
      }
      break;
    case R_RISCV_SET32:
      if (off + 4 <= data.size()) {
        write_u32_le(data, off, static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a));
      }
      break;
    case R_RISCV_32_PCREL:
      if (off + 4 <= data.size()) {
        write_u32_le(data, off, static_cast<std::uint32_t>(static_cast<std::int64_t>(s) + a -
                                                           static_cast<std::int64_t>(p)));
      }
      break;
    case R_RISCV_SET_ULEB128:
      apply_set_uleb128(data, off, static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a));
      break;
    case R_RISCV_SUB_ULEB128:
      apply_sub_uleb128(data, off, static_cast<std::uint64_t>(static_cast<std::int64_t>(s) + a));
      break;
    case R_RISCV_TLS_GD_HI20: {
      const std::uint32_t tprel = static_cast<std::uint32_t>(
          static_cast<std::int64_t>(s) + a - static_cast<std::int64_t>(ctx.tls_vaddr));
      const std::uint32_t hi = (tprel + 0x800u) & 0xFFFFF000u;
      if (off + 4 <= data.size()) {
        write_u32_le(data, off, 0x00000537u | hi);
      }
      break;
    }
    case R_RISCV_TLS_GOT_HI20: {
      const std::uint64_t got_entry_vaddr = lookup_got_entry(sym, obj_idx, a, ctx);
      const std::int64_t offset_val =
          static_cast<std::int64_t>(got_entry_vaddr) + a - static_cast<std::int64_t>(p);
      patch_u_type(data, off, static_cast<std::uint32_t>(offset_val));
      break;
    }
    default:
      if (!ctx.collect_relatives) {
        if (error != nullptr) {
          *error = "unsupported RISC-V relocation type " + std::to_string(rela_type) +
                   " for symbol '" + sym.name + "' in '" + obj_name + "'";
        }
        return false;
      }
      break;
  }

  return true;
}

}  // namespace

bool apply_first_static_text_relocations(
    const std::vector<LoadedInputObject>& loaded_objects,
    const std::unordered_map<std::string, std::uint64_t>& symbol_addresses,
    const std::unordered_map<std::string, std::uint64_t>& text_offsets,
    std::uint64_t text_virtual_address,
    std::vector<std::uint8_t>* merged_text,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (merged_text == nullptr) {
    if (error != nullptr) *error = "missing merged text buffer";
    return false;
  }

  std::vector<std::pair<std::string, ElfObject>> input_objs;
  input_objs.reserve(loaded_objects.size());
  SectionMap sec_mapping;

  for (std::size_t obj_idx = 0; obj_idx < loaded_objects.size(); ++obj_idx) {
    const auto& loaded_object = loaded_objects[obj_idx];
    const auto text_index = find_text_section_index(loaded_object.object);
    if (!text_index.has_value()) {
      if (error != nullptr) *error = loaded_object.path + ": missing .text section";
      return false;
    }
    const auto text_offset = text_offsets.find(loaded_object.path);
    if (text_offset == text_offsets.end()) {
      if (error != nullptr) *error = loaded_object.path + ": missing merged .text offset";
      return false;
    }

    for (std::size_t section_index = 0; section_index < loaded_object.object.sections.size();
         ++section_index) {
      if (section_index == *text_index) continue;
      if (!loaded_object.object.relocations[section_index].empty()) {
        if (error != nullptr) {
          *error = loaded_object.path +
                   ": first static executable currently supports relocations only in RV64 .text";
        }
        return false;
      }
    }

    sec_mapping.emplace(std::make_pair(obj_idx, *text_index),
                        std::make_pair(static_cast<std::size_t>(0), text_offset->second));
    input_objs.emplace_back(loaded_object.path, convert_object(loaded_object.object));
  }

  std::vector<MergedSection> merged_sections;
  merged_sections.push_back(MergedSection{
      .name = ".text",
      .sh_type = 1,
      .sh_flags = 0x6,
      .data = *merged_text,
      .vaddr = text_virtual_address,
  });

  const SectionVaddrs section_vaddrs = {text_virtual_address};
  const LocalSymVaddrs local_sym_vaddrs =
      build_local_sym_vaddrs(input_objs, sec_mapping, section_vaddrs);
  const GlobalSymMap global_syms = build_global_symbol_map(loaded_objects, symbol_addresses);
  const std::vector<std::string> got_symbols;
  const GDRelaxMap gd_tls_relax_info;
  const std::unordered_set<std::uint64_t> gd_tls_call_nop;
  const GotOffsetMap got_sym_offsets;
  const PltAddrMap plt_sym_addrs;
  const RelocContext ctx{
      .sec_mapping = sec_mapping,
      .section_vaddrs = section_vaddrs,
      .local_sym_vaddrs = local_sym_vaddrs,
      .global_syms = global_syms,
      .got_vaddr = 0,
      .got_symbols = got_symbols,
      .got_plt_vaddr = 0,
      .tls_vaddr = 0,
      .gd_tls_relax_info = gd_tls_relax_info,
      .gd_tls_call_nop = gd_tls_call_nop,
      .collect_relatives = false,
      .got_sym_offsets = got_sym_offsets,
      .plt_sym_addrs = plt_sym_addrs,
  };

  if (!apply_relocations(input_objs, &merged_sections, ctx, error).has_value()) {
    return false;
  }

  *merged_text = std::move(merged_sections.front().data);
  return true;
}

[[nodiscard]] std::optional<RelocResult> apply_relocations(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    std::vector<MergedSection>* merged_sections,
    const RelocContext& ctx,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (merged_sections == nullptr) {
    if (error != nullptr) *error = "null merged_sections";
    return std::nullopt;
  }

  RelocResult result;

  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& [obj_name, obj] = input_objs[obj_idx];
    for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
      const auto& relocs = obj.relocations[sec_idx];
      if (relocs.empty()) continue;

      const auto map_it = ctx.sec_mapping.find({obj_idx, sec_idx});
      if (map_it == ctx.sec_mapping.end()) continue;

      const std::size_t merged_idx = map_it->second.first;
      const std::uint64_t sec_offset = map_it->second.second;
      if (merged_idx >= merged_sections->size() || merged_idx >= ctx.section_vaddrs.size()) {
        continue;
      }

      const std::uint64_t ms_vaddr = ctx.section_vaddrs[merged_idx];
      auto& data = (*merged_sections)[merged_idx].data;

      for (const auto& reloc : relocs) {
        const std::uint64_t offset = sec_offset + reloc.offset;
        const std::uint64_t p = ms_vaddr + offset;
        const std::size_t sym_idx = static_cast<std::size_t>(reloc.sym_idx);
        if (sym_idx >= obj.symbols.size()) continue;

        const Symbol& sym = obj.symbols[sym_idx];
        const std::uint64_t s = resolve_symbol_value(sym, sym_idx, obj_idx, obj, ctx.sec_mapping,
                                                     ctx.section_vaddrs, ctx.local_sym_vaddrs,
                                                     ctx.global_syms);
        if (!apply_one_reloc(reloc.rela_type, data, static_cast<std::size_t>(offset), s,
                             reloc.addend, p, sym, obj_idx, obj, sec_idx, sec_offset, obj_name,
                             ctx, &result, error)) {
          return std::nullopt;
        }
      }
    }
  }

  return result;
}

void collect_gd_tls_relax_info(
    const std::vector<std::pair<std::string, ElfObject>>& input_objs,
    const SectionMap& sec_mapping,
    const SectionVaddrs& section_vaddrs,
    const LocalSymVaddrs& local_sym_vaddrs,
    const GlobalSymMap& global_syms,
    std::map<std::uint64_t, std::pair<std::uint64_t, std::int64_t>>* gd_tls_relax_info,
    std::unordered_set<std::uint64_t>* gd_tls_call_nop) {
  if (gd_tls_relax_info == nullptr || gd_tls_call_nop == nullptr) return;

  for (std::size_t obj_idx = 0; obj_idx < input_objs.size(); ++obj_idx) {
    const auto& obj = input_objs[obj_idx].second;
    for (std::size_t sec_idx = 0; sec_idx < obj.relocations.size(); ++sec_idx) {
      const auto& relocs = obj.relocations[sec_idx];
      if (relocs.empty()) continue;

      const auto map_it = sec_mapping.find({obj_idx, sec_idx});
      if (map_it == sec_mapping.end() || map_it->second.first >= section_vaddrs.size()) {
        continue;
      }

      const std::uint64_t ms_vaddr = section_vaddrs[map_it->second.first];
      const std::uint64_t sec_offset = map_it->second.second;

      for (std::size_t ri = 0; ri < relocs.size(); ++ri) {
        const auto& reloc = relocs[ri];
        if (reloc.rela_type != R_RISCV_TLS_GD_HI20) continue;

        const std::uint64_t offset = sec_offset + reloc.offset;
        const std::uint64_t auipc_vaddr = ms_vaddr + offset;
        const std::size_t sym_idx = static_cast<std::size_t>(reloc.sym_idx);
        if (sym_idx >= obj.symbols.size()) continue;

        const Symbol& sym = obj.symbols[sym_idx];
        const std::uint64_t sym_val = resolve_symbol_value(
            sym, sym_idx, obj_idx, obj, sec_mapping, section_vaddrs, local_sym_vaddrs,
            global_syms);
        gd_tls_relax_info->emplace(auipc_vaddr, std::make_pair(sym_val, reloc.addend));

        const std::size_t limit = std::min(relocs.size(), ri + 8);
        for (std::size_t j = ri + 1; j < limit; ++j) {
          const auto& call_reloc = relocs[j];
          if (call_reloc.rela_type != R_RISCV_CALL_PLT) continue;
          const std::size_t call_sym_idx = static_cast<std::size_t>(call_reloc.sym_idx);
          if (call_sym_idx >= obj.symbols.size()) continue;

          const Symbol& call_sym = obj.symbols[call_sym_idx];
          if (call_sym.name == "__tls_get_addr") {
            const std::uint64_t call_offset = sec_offset + call_reloc.offset;
            const std::uint64_t call_vaddr = ms_vaddr + call_offset;
            gd_tls_call_nop->insert(call_vaddr);
            break;
          }
        }
      }
    }
  }
}

}  // namespace c4c::backend::riscv::linker

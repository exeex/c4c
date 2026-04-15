#include "mod.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::linker {
namespace {

// Relocation constants (from AArch64 ELF ABI).
constexpr std::uint32_t kR_AARCH64_NONE = 0;
constexpr std::uint32_t kR_AARCH64_ABS64 = 257;
constexpr std::uint32_t kR_AARCH64_ABS32 = 258;
constexpr std::uint32_t kR_AARCH64_ABS16 = 259;
constexpr std::uint32_t kR_AARCH64_PREL64 = 260;
constexpr std::uint32_t kR_AARCH64_PREL32 = 261;
constexpr std::uint32_t kR_AARCH64_PREL16 = 262;
constexpr std::uint32_t kR_AARCH64_ADR_PREL_LO21 = 274;
constexpr std::uint32_t kR_AARCH64_ADR_PREL_PG_HI21 = 275;
constexpr std::uint32_t kR_AARCH64_ADD_ABS_LO12_NC = 277;
constexpr std::uint32_t kR_AARCH64_LDST8_ABS_LO12_NC = 278;
constexpr std::uint32_t kR_AARCH64_TSTBR14 = 279;
constexpr std::uint32_t kR_AARCH64_CONDBR19 = 280;
constexpr std::uint32_t kR_AARCH64_LDST16_ABS_LO12_NC = 284;
constexpr std::uint32_t kR_AARCH64_LDST32_ABS_LO12_NC = 285;
constexpr std::uint32_t kR_AARCH64_LDST64_ABS_LO12_NC = 286;
constexpr std::uint32_t kR_AARCH64_LDST128_ABS_LO12_NC = 299;
constexpr std::uint32_t kR_AARCH64_JUMP26 = 282;
constexpr std::uint32_t kR_AARCH64_CALL26 = 283;

constexpr std::uint32_t kBoundedFirstSliceCallReloc = 279;

bool has_room_for(const std::vector<std::uint8_t>& bytes, std::size_t offset, std::size_t bytes_needed) {
  return offset + bytes_needed <= bytes.size();
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

void write_u32(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint32_t value) {
  (*bytes)[offset] = static_cast<std::uint8_t>(value & 0xff);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
  (*bytes)[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
  (*bytes)[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
}

bool in_range(std::int64_t value, int bits) {
  if (bits <= 0 || bits >= 63) return true;
  const std::int64_t min = -(std::int64_t{1} << (bits - 1));
  const std::int64_t max = (std::int64_t{1} << (bits - 1)) - 1;
  return value >= min && value <= max;
}

void write_u64(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint64_t value) {
  if (offset + 8 > bytes->size()) return;
  (*bytes)[offset] = static_cast<std::uint8_t>(value & 0xff);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
  (*bytes)[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
  (*bytes)[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
  (*bytes)[offset + 4] = static_cast<std::uint8_t>((value >> 32) & 0xff);
  (*bytes)[offset + 5] = static_cast<std::uint8_t>((value >> 40) & 0xff);
  (*bytes)[offset + 6] = static_cast<std::uint8_t>((value >> 48) & 0xff);
  (*bytes)[offset + 7] = static_cast<std::uint8_t>((value >> 56) & 0xff);
}

void encode_adrp(std::vector<std::uint8_t>* bytes, std::size_t offset, std::int64_t imm) {
  auto insn = read_u32(*bytes, offset);
  const std::uint32_t immlo = static_cast<std::uint32_t>(imm & 0x3);
  const std::uint32_t immhi = static_cast<std::uint32_t>((imm >> 2) & 0x7ffff);
  insn = (insn & 0x9f00001fu) | (immlo << 29) | (immhi << 5);
  write_u32(bytes, offset, insn);
}

void encode_adr(std::vector<std::uint8_t>* bytes, std::size_t offset, std::int64_t offset_bytes) {
  auto insn = read_u32(*bytes, offset);
  const std::uint32_t imm = static_cast<std::uint32_t>(offset_bytes);
  const std::uint32_t immlo = imm & 0x3;
  const std::uint32_t immhi = (imm >> 2) & 0x7ffff;
  insn = (insn & 0x9f00001fu) | (immlo << 29) | (immhi << 5);
  write_u32(bytes, offset, insn);
}

void encode_add_imm12(std::vector<std::uint8_t>* bytes,
                     std::size_t offset,
                     std::uint32_t imm12) {
  auto insn = read_u32(*bytes, offset);
  insn = (insn & 0xffc003ffu) | ((imm12 & 0xfff) << 10);
  write_u32(bytes, offset, insn);
}

void encode_ldst_imm12(std::vector<std::uint8_t>* bytes,
                      std::size_t offset,
                      std::uint32_t lo12,
                      std::uint32_t shift) {
  auto insn = read_u32(*bytes, offset);
  const std::uint32_t imm12 = (lo12 >> shift) & 0xfff;
  insn = (insn & 0xffc003ffu) | (imm12 << 10);
  write_u32(bytes, offset, insn);
}

bool apply_relocation(std::vector<std::uint8_t>* merged_text,
                     std::size_t patch_offset,
                     std::uint32_t reloc_type,
                     const std::string& source_symbol,
                     std::uint64_t target_address,
                     std::uint64_t text_virtual_address,
                     std::int64_t addend,
                     std::string* error) {
  if (!has_room_for(*merged_text, patch_offset, 4)) {
    if (error != nullptr) {
      *error = source_symbol + ": relocation offset out of bounds";
    }
    return false;
  }

  const auto target = static_cast<std::int64_t>(target_address) + addend;
  const auto place = static_cast<std::int64_t>(text_virtual_address + patch_offset);
  const auto delta = target - place;
  auto insn = read_u32(*merged_text, patch_offset);

  switch (reloc_type) {
    case kR_AARCH64_NONE:
      return true;
    case kR_AARCH64_ABS64: {
      if (!has_room_for(*merged_text, patch_offset, 8)) return false;
      write_u64(merged_text, patch_offset, static_cast<std::uint64_t>(target));
      return true;
    }
    case kR_AARCH64_PREL64: {
      if (!has_room_for(*merged_text, patch_offset, 8)) return false;
      const auto value = static_cast<std::uint64_t>(target - place);
      write_u64(merged_text, patch_offset, value);
      return true;
    }
    case kR_AARCH64_ABS32: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      write_u32(merged_text, patch_offset, static_cast<std::uint32_t>(target));
      return true;
    }
    case kR_AARCH64_PREL32: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      write_u32(merged_text, patch_offset, static_cast<std::uint32_t>(delta));
      return true;
    }
    case kR_AARCH64_ABS16:
      if (!has_room_for(*merged_text, patch_offset, 2)) {
        if (error != nullptr) {
          *error = source_symbol + ": relocation offset out of bounds";
        }
        return false;
      }
      (*merged_text)[patch_offset] = static_cast<std::uint8_t>(target & 0xff);
      (*merged_text)[patch_offset + 1] = static_cast<std::uint8_t>((target >> 8) & 0xff);
      return true;
    case kR_AARCH64_PREL16: {
      if (!has_room_for(*merged_text, patch_offset, 2)) {
        if (error != nullptr) {
          *error = source_symbol + ": relocation offset out of bounds";
        }
        return false;
      }
      const auto value = static_cast<std::uint16_t>(static_cast<std::uint64_t>(delta));
      (*merged_text)[patch_offset] = static_cast<std::uint8_t>(value & 0xff);
      (*merged_text)[patch_offset + 1] = static_cast<std::uint8_t>(value >> 8);
      return true;
    }
    case kR_AARCH64_JUMP26:
    case kR_AARCH64_CALL26: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      if ((delta & 0x3) != 0) {
        if (error != nullptr) {
          *error = source_symbol + ": branch relocation target is not 4-byte aligned";
        }
        return false;
      }
      const std::int64_t imm26 = delta >> 2;
      if (!in_range(imm26, 26)) {
        if (error != nullptr) {
          *error = source_symbol + ": branch relocation out of range";
        }
        return false;
      }
      insn = (insn & 0xfc000000u) | (static_cast<std::uint32_t>(imm26) & 0x03ffffffu);
      write_u32(merged_text, patch_offset, insn);
      return true;
    }
    case kR_AARCH64_CONDBR19: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      const std::int64_t imm19 = delta >> 2;
      if ((delta & 0x3) != 0 || !in_range(imm19, 19)) {
        if (error != nullptr) {
          *error = source_symbol + ": conditional branch relocation out of range";
        }
        return false;
      }
      insn = (insn & 0xff00001fu) | (static_cast<std::uint32_t>(imm19 & 0x7ffffu) << 5);
      write_u32(merged_text, patch_offset, insn);
      return true;
    }
    case kR_AARCH64_TSTBR14: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      const std::int64_t imm14 = delta >> 2;
      if ((delta & 0x3) != 0 || !in_range(imm14, 14)) {
        if (error != nullptr) {
          *error = source_symbol + ": test-branch relocation out of range";
        }
        return false;
      }
      insn = (insn & 0xfff8001fu) | (static_cast<std::uint32_t>(imm14 & 0x3fffu) << 5);
      write_u32(merged_text, patch_offset, insn);
      return true;
    }
    case kR_AARCH64_ADR_PREL_PG_HI21: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      const auto sym_page = (target_address & ~0xfffULL);
      const auto pc_page = (place & ~0xfffLL);
      const std::int64_t page_delta =
          (static_cast<std::int64_t>(sym_page) - static_cast<std::int64_t>(pc_page)) >> 12;
      if (!in_range(page_delta, 21)) {
        if (error != nullptr) {
          *error = source_symbol + ": ADRP relocation out of range";
        }
        return false;
      }
      encode_adrp(merged_text, patch_offset, page_delta);
      return true;
    }
    case kR_AARCH64_ADR_PREL_LO21: {
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      if (!in_range(delta, 21)) {
        if (error != nullptr) {
          *error = source_symbol + ": ADR relocation out of range";
        }
        return false;
      }
      encode_adr(merged_text, patch_offset, delta);
      return true;
    }
    case kR_AARCH64_ADD_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_add_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff));
      return true;
    case kR_AARCH64_LDST8_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_ldst_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff), 0);
      return true;
    case kR_AARCH64_LDST16_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_ldst_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff), 1);
      return true;
    case kR_AARCH64_LDST32_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_ldst_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff), 2);
      return true;
    case kR_AARCH64_LDST64_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_ldst_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff), 3);
      return true;
    case kR_AARCH64_LDST128_ABS_LO12_NC:
      if (!has_room_for(*merged_text, patch_offset, 4)) return false;
      encode_ldst_imm12(merged_text, patch_offset, static_cast<std::uint32_t>(target & 0xfff), 4);
      return true;
    default:
      return false;
  }
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

  for (const auto& loaded_object : loaded_objects) {
    const auto offset_it = text_offsets.find(loaded_object.path);
    if (offset_it == text_offsets.end()) continue;

    const auto& object = loaded_object.object;
    for (std::size_t section_index = 0; section_index < object.sections.size(); ++section_index) {
      const auto& section = object.sections[section_index];
      if (section.name != ".text") continue;

      for (const auto& relocation : object.relocations[section_index]) {
        if (relocation.rela_type != kBoundedFirstSliceCallReloc &&
            relocation.rela_type != kR_AARCH64_CALL26 &&
            relocation.rela_type != kR_AARCH64_JUMP26 &&
            relocation.rela_type != kR_AARCH64_CONDBR19 &&
            relocation.rela_type != kR_AARCH64_TSTBR14 &&
            relocation.rela_type != kR_AARCH64_ADR_PREL_PG_HI21 &&
            relocation.rela_type != kR_AARCH64_ADR_PREL_LO21 &&
            relocation.rela_type != kR_AARCH64_ADD_ABS_LO12_NC &&
            relocation.rela_type != kR_AARCH64_LDST8_ABS_LO12_NC &&
            relocation.rela_type != kR_AARCH64_LDST16_ABS_LO12_NC &&
            relocation.rela_type != kR_AARCH64_LDST32_ABS_LO12_NC &&
            relocation.rela_type != kR_AARCH64_LDST64_ABS_LO12_NC &&
            relocation.rela_type != kR_AARCH64_LDST128_ABS_LO12_NC) {
          if (error != nullptr) {
            *error = loaded_object.path + ": unsupported relocation type in first static-link slice: " +
                     std::to_string(relocation.rela_type);
          }
          return false;
        }
        if (relocation.sym_idx >= object.symbols.size()) {
          if (error != nullptr) {
            *error = loaded_object.path + ": relocation symbol index out of bounds";
          }
          return false;
        }

        const auto& symbol = object.symbols[relocation.sym_idx];
        const auto symbol_it = symbol_addresses.find(symbol.name);
        if (symbol_it == symbol_addresses.end()) {
          if (error != nullptr) {
            *error = loaded_object.path + ": unresolved relocation symbol: " + symbol.name;
          }
          return false;
        }

        const auto patch_offset_u64 = offset_it->second + relocation.offset;
        if (patch_offset_u64 + 4 > merged_text->size()) {
          if (error != nullptr) {
            *error = loaded_object.path + ": relocation offset past merged .text";
          }
          return false;
        }
        const auto patch_offset = static_cast<std::size_t>(patch_offset_u64);

        if (!apply_relocation(
                merged_text, patch_offset, relocation.rela_type, symbol.name, symbol_it->second,
                text_virtual_address + offset_it->second, relocation.addend, error)) {
          return false;
        }
      }
    }
  }
  return true;
}

}  // namespace c4c::backend::aarch64::linker

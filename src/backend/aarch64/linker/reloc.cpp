#include "mod.hpp"

#include <limits>

namespace c4c::backend::aarch64::linker {
namespace {

constexpr std::uint32_t kBoundedFirstSliceCallReloc = 279;
constexpr std::uint32_t kAarch64Call26 = 283;

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
            relocation.rela_type != kAarch64Call26) {
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
        const std::int64_t target =
            static_cast<std::int64_t>(symbol_it->second) + relocation.addend;
        const std::int64_t place =
            static_cast<std::int64_t>(text_virtual_address + patch_offset_u64);
        const std::int64_t delta = target - place;
        if ((delta & 0x3) != 0) {
          if (error != nullptr) {
            *error = loaded_object.path + ": CALL26 target is not 4-byte aligned";
          }
          return false;
        }

        const std::int64_t imm26 = delta >> 2;
        if (imm26 < -(1LL << 25) || imm26 >= (1LL << 25)) {
          if (error != nullptr) {
            *error = loaded_object.path + ": CALL26 target out of range";
          }
          return false;
        }

        const auto original = read_u32(*merged_text, patch_offset);
        (void)original;
        const auto patched =
            0x94000000u | (static_cast<std::uint32_t>(imm26) & 0x03ffffffu);
        write_u32(merged_text, patch_offset, patched);
      }
    }
  }

  return true;
}

}  // namespace c4c::backend::aarch64::linker

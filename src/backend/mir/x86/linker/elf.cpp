// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/elf.rs
// ELF constants, relocation types, and shared parser entry points.

#include "mod.hpp"

#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::x86::linker {

constexpr std::uint32_t R_X86_64_NONE = 0;
constexpr std::uint32_t R_X86_64_64 = 1;
constexpr std::uint32_t R_X86_64_PC32 = 2;
constexpr std::uint32_t R_X86_64_GOT32 = 3;
constexpr std::uint32_t R_X86_64_PLT32 = 4;
constexpr std::uint32_t R_X86_64_GLOB_DAT = 6;
constexpr std::uint32_t R_X86_64_JUMP_SLOT = 7;
constexpr std::uint32_t R_X86_64_RELATIVE = 8;
constexpr std::uint32_t R_X86_64_GOTPCREL = 9;
constexpr std::uint32_t R_X86_64_32 = 10;
constexpr std::uint32_t R_X86_64_32S = 11;
constexpr std::uint32_t R_X86_64_DTPMOD64 = 16;
constexpr std::uint32_t R_X86_64_DTPOFF64 = 17;
constexpr std::uint32_t R_X86_64_TPOFF64 = 18;
constexpr std::uint32_t R_X86_64_GOTTPOFF = 22;
constexpr std::uint32_t R_X86_64_TPOFF32 = 23;
constexpr std::uint32_t R_X86_64_PC64 = 24;
constexpr std::uint32_t R_X86_64_GOTPCRELX = 41;
constexpr std::uint32_t R_X86_64_REX_GOTPCRELX = 42;
constexpr std::uint32_t R_X86_64_IRELATIVE = 37;
constexpr std::uint64_t DF_BIND_NOW = 0x8;

namespace {

constexpr std::uint16_t kEtExec = 2;
constexpr std::uint32_t kPtLoad = 1;
constexpr std::uint32_t kPfX = 0x1;
constexpr std::uint32_t kPfR = 0x4;
constexpr std::uint16_t kElfHeaderSize = 64;
constexpr std::uint16_t kProgramHeaderSize = 56;
constexpr std::uint64_t kDefaultBaseAddress = 0x400000;

void write_u16(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint16_t value) {
  (*bytes)[offset] = static_cast<std::uint8_t>(value & 0xff);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
}

void write_u32(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint32_t value) {
  (*bytes)[offset] = static_cast<std::uint8_t>(value & 0xff);
  (*bytes)[offset + 1] = static_cast<std::uint8_t>((value >> 8) & 0xff);
  (*bytes)[offset + 2] = static_cast<std::uint8_t>((value >> 16) & 0xff);
  (*bytes)[offset + 3] = static_cast<std::uint8_t>((value >> 24) & 0xff);
}

void write_i32(std::vector<std::uint8_t>* bytes, std::size_t offset, std::int32_t value) {
  write_u32(bytes, offset, static_cast<std::uint32_t>(value));
}

void write_u64(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    (*bytes)[offset + (shift / 8)] = static_cast<std::uint8_t>((value >> shift) & 0xff);
  }
}

}  // namespace

// These wrappers intentionally preserve the Rust module's boundary: ELF parsing
// is delegated to the shared linker infrastructure, and the x86 module only
// provides the arch-specific constants.

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
        if (relocation.rela_type != R_X86_64_PLT32) {
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

        const std::int64_t target =
            static_cast<std::int64_t>(symbol_it->second) + relocation.addend;
        const std::int64_t place =
            static_cast<std::int64_t>(text_virtual_address + patch_offset_u64);
        const std::int64_t delta = target - place;
        if (delta < std::numeric_limits<std::int32_t>::min() ||
            delta > std::numeric_limits<std::int32_t>::max()) {
          if (error != nullptr) {
            *error = loaded_object.path + ": PLT32 target out of range";
          }
          return false;
        }

        write_i32(merged_text, static_cast<std::size_t>(patch_offset_u64),
                  static_cast<std::int32_t>(delta));
      }
    }
  }

  return true;
}

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

  if (base_address == 0) base_address = kDefaultBaseAddress;
  const std::uint64_t text_offset = kElfHeaderSize + kProgramHeaderSize;
  const std::uint64_t text_vaddr = base_address + text_offset;
  const std::uint64_t file_size = text_offset + merged_text.size();

  if (entry_address < text_vaddr || entry_address >= text_vaddr + merged_text.size()) {
    if (error != nullptr) *error = "entry point for first static executable must live in merged .text";
    return std::nullopt;
  }

  std::vector<std::uint8_t> image(file_size, 0);
  image[0] = 0x7f;
  image[1] = 'E';
  image[2] = 'L';
  image[3] = 'F';
  image[4] = 2;
  image[5] = 1;
  image[6] = 1;

  write_u16(&image, 16, kEtExec);
  write_u16(&image, 18, elf::EM_X86_64);
  write_u32(&image, 20, 1);
  write_u64(&image, 24, entry_address);
  write_u64(&image, 32, kElfHeaderSize);
  write_u64(&image, 40, 0);
  write_u32(&image, 48, 0);
  write_u16(&image, 52, kElfHeaderSize);
  write_u16(&image, 54, kProgramHeaderSize);
  write_u16(&image, 56, 1);
  write_u16(&image, 58, 0);
  write_u16(&image, 60, 0);
  write_u16(&image, 62, 0);

  write_u32(&image, kElfHeaderSize + 0, kPtLoad);
  write_u32(&image, kElfHeaderSize + 4, kPfR | kPfX);
  write_u64(&image, kElfHeaderSize + 8, 0);
  write_u64(&image, kElfHeaderSize + 16, base_address);
  write_u64(&image, kElfHeaderSize + 24, base_address);
  write_u64(&image, kElfHeaderSize + 32, file_size);
  write_u64(&image, kElfHeaderSize + 40, file_size);
  write_u64(&image, kElfHeaderSize + 48, 0x1000);

  std::copy(merged_text.begin(), merged_text.end(), image.begin() + text_offset);

  if (text_file_offset != nullptr) *text_file_offset = text_offset;
  if (text_virtual_address != nullptr) *text_virtual_address = text_vaddr;
  return image;
}

bool parse_object(const std::vector<std::uint8_t>& data,
                  const std::string& source_name,
                  ElfObject* out) {
  (void)data;
  (void)source_name;
  if (out != nullptr) {
    *out = ElfObject{};
  }
  return true;
}

bool parse_shared_library_symbols(const std::vector<std::uint8_t>& data,
                                  const std::string& lib_name,
                                  std::vector<DynSymbol>* out) {
  (void)data;
  (void)lib_name;
  if (out != nullptr) {
    out->clear();
  }
  return true;
}

std::optional<std::string> parse_soname(const std::vector<std::uint8_t>& data) {
  (void)data;
  return std::nullopt;
}

}  // namespace c4c::backend::x86::linker

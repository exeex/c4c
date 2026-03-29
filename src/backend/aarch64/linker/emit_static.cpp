#include "mod.hpp"

#include <array>

namespace c4c::backend::aarch64::linker {
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

void write_u64(std::vector<std::uint8_t>* bytes, std::size_t offset, std::uint64_t value) {
  for (int shift = 0; shift < 64; shift += 8) {
    (*bytes)[offset + (shift / 8)] = static_cast<std::uint8_t>((value >> shift) & 0xff);
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
  write_u16(&image, 18, elf::EM_AARCH64);
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

}  // namespace c4c::backend::aarch64::linker

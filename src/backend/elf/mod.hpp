#pragma once

#include <cstddef>
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::elf {

// Marker type for the staged shared ELF contract surface.
struct ContractSurface final {};

inline constexpr std::array<std::uint8_t, 4> ELF_MAGIC = {0x7f, 'E', 'L', 'F'};

inline constexpr std::uint8_t ELFCLASS64 = 2;
inline constexpr std::uint8_t ELFDATA2LSB = 1;

inline constexpr std::uint16_t ET_REL = 1;
inline constexpr std::uint16_t EM_AARCH64 = 183;

inline constexpr std::uint32_t SHT_NULL = 0;
inline constexpr std::uint32_t SHT_PROGBITS = 1;
inline constexpr std::uint32_t SHT_SYMTAB = 2;
inline constexpr std::uint32_t SHT_STRTAB = 3;
inline constexpr std::uint32_t SHT_RELA = 4;
inline constexpr std::uint32_t SHT_NOBITS = 8;

inline constexpr std::uint64_t SHF_ALLOC = 0x2;
inline constexpr std::uint64_t SHF_EXECINSTR = 0x4;
inline constexpr std::uint64_t SHF_INFO_LINK = 0x40;

inline constexpr std::uint8_t STB_LOCAL = 0;
inline constexpr std::uint8_t STB_GLOBAL = 1;
inline constexpr std::uint8_t STB_WEAK = 2;

inline constexpr std::uint8_t STT_NOTYPE = 0;
inline constexpr std::uint8_t STT_FUNC = 2;
inline constexpr std::uint8_t STT_SECTION = 3;

inline constexpr std::uint16_t SHN_UNDEF = 0;

struct ArchiveMemberRef {
  std::string name;
  std::size_t data_offset = 0;
  std::size_t data_size = 0;
};

[[nodiscard]] bool is_archive_file(const std::vector<std::uint8_t>& data);

[[nodiscard]] std::optional<std::vector<ArchiveMemberRef>> parse_archive_members(
    const std::vector<std::uint8_t>& data,
    std::string* error = nullptr);

}  // namespace c4c::backend::elf

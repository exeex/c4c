#pragma once

#include "../elf/mod.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::linker_common {

// Marker type for the staged shared linker contract surface.
struct ContractSurface final {};

struct Elf64Section {
  std::uint32_t name_idx = 0;
  std::string name;
  std::uint32_t sh_type = 0;
  std::uint64_t flags = 0;
  std::uint64_t addr = 0;
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
  std::uint32_t link = 0;
  std::uint32_t info = 0;
  std::uint64_t addralign = 0;
  std::uint64_t entsize = 0;
};

struct Elf64Symbol {
  std::uint32_t name_idx = 0;
  std::string name;
  std::uint8_t info = 0;
  std::uint8_t other = 0;
  std::uint16_t shndx = 0;
  std::uint64_t value = 0;
  std::uint64_t size = 0;

  std::uint8_t binding() const { return static_cast<std::uint8_t>(info >> 4); }
  std::uint8_t sym_type() const { return static_cast<std::uint8_t>(info & 0x0f); }
  std::uint8_t visibility() const { return static_cast<std::uint8_t>(other & 0x03); }
  bool is_undefined() const { return shndx == elf::SHN_UNDEF; }
  bool is_global() const { return binding() == elf::STB_GLOBAL; }
  bool is_weak() const { return binding() == elf::STB_WEAK; }
  bool is_local() const { return binding() == elf::STB_LOCAL; }
};

struct Elf64Rela {
  std::uint64_t offset = 0;
  std::uint32_t sym_idx = 0;
  std::uint32_t rela_type = 0;
  std::int64_t addend = 0;
};

struct Elf64Object {
  std::vector<Elf64Section> sections;
  std::vector<Elf64Symbol> symbols;
  std::vector<std::vector<std::uint8_t>> section_data;
  std::vector<std::vector<Elf64Rela>> relocations;
  std::string source_name;
};

struct ArchiveMember {
  std::string name;
  std::size_t data_offset = 0;
  std::size_t data_size = 0;
  std::optional<Elf64Object> object;
  std::vector<std::string> defined_symbols;

  [[nodiscard]] bool defines_symbol(const std::string& symbol_name) const;
};

struct ElfArchive {
  std::vector<ArchiveMember> members;
  std::string source_name;

  [[nodiscard]] std::optional<std::size_t> find_member_index_for_symbol(
      const std::string& symbol_name) const;
};

[[nodiscard]] std::optional<Elf64Object> parse_elf64_object(
    const std::vector<std::uint8_t>& data,
    const std::string& source_name,
    std::uint16_t expected_machine,
    std::string* error = nullptr);

[[nodiscard]] std::optional<ElfArchive> parse_elf64_archive(
    const std::vector<std::uint8_t>& data,
    const std::string& source_name,
    std::uint16_t expected_machine,
    std::string* error = nullptr);

}  // namespace c4c::backend::linker_common

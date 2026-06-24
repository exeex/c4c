#pragma once

#include "mir/object/model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::mir::object {

enum class ElfClass {
  Elf64,
};

enum class ElfDataEncoding {
  LittleEndian,
};

struct RelocatableElfConfig {
  ElfClass elf_class = ElfClass::Elf64;
  ElfDataEncoding data_encoding = ElfDataEncoding::LittleEndian;
  std::uint16_t machine = 0;
  std::uint32_t flags = 0;
};

struct RelocatableElfImage {
  std::vector<std::uint8_t> bytes;
};

std::optional<RelocatableElfImage> write_relocatable_elf(
    const ObjectModule& module, const RelocatableElfConfig& config);

}  // namespace c4c::backend::mir::object

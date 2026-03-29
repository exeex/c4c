#pragma once

#include "../../elf/mod.hpp"
#include "../../linker_common/mod.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::linker {

// Marker type for the staged AArch64 linker contract surface.
struct ContractSurface final {};

struct InputObjectSummary {
  std::string path;
  std::vector<std::string> defined_symbols;
  std::vector<std::string> undefined_symbols;
};

struct LoadedInputObject {
  std::string path;
  linker_common::Elf64Object object;
};

struct InputRelocationSummary {
  std::string object_path;
  std::string section_name;
  std::string symbol_name;
  std::uint32_t relocation_type = 0;
  std::uint64_t offset = 0;
  std::int64_t addend = 0;
  bool resolved = false;
  std::string resolved_object_path;
};

struct FirstStaticLinkSlice {
  std::string case_name;
  std::vector<InputObjectSummary> objects;
  std::vector<InputRelocationSummary> relocations;
  std::vector<std::string> merged_output_sections;
  std::vector<std::string> unresolved_symbols;
};

[[nodiscard]] std::optional<FirstStaticLinkSlice> inspect_first_static_link_slice(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

[[nodiscard]] std::optional<std::vector<LoadedInputObject>> load_first_static_input_objects(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

}  // namespace c4c::backend::aarch64::linker

#pragma once

#include "../../elf/mod.hpp"
#include "../../linker_common/mod.hpp"

#include <cstdint>
#include <unordered_map>
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

struct FirstStaticExecutable {
  std::vector<std::uint8_t> image;
  std::uint64_t base_address = 0;
  std::uint64_t entry_address = 0;
  std::uint64_t text_file_offset = 0;
  std::uint64_t text_virtual_address = 0;
  std::uint64_t text_size = 0;
  std::unordered_map<std::string, std::uint64_t> symbol_addresses;
};

[[nodiscard]] std::optional<FirstStaticLinkSlice> inspect_first_static_link_slice(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

[[nodiscard]] std::optional<std::vector<LoadedInputObject>> load_first_static_input_objects(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

[[nodiscard]] bool apply_first_static_text_relocations(
    const std::vector<LoadedInputObject>& loaded_objects,
    const std::unordered_map<std::string, std::uint64_t>& symbol_addresses,
    const std::unordered_map<std::string, std::uint64_t>& text_offsets,
    std::uint64_t text_virtual_address,
    std::vector<std::uint8_t>* merged_text,
    std::string* error = nullptr);

[[nodiscard]] std::optional<std::vector<std::uint8_t>> emit_first_static_executable_image(
    const std::vector<std::uint8_t>& merged_text,
    std::uint64_t base_address,
    std::uint64_t entry_address,
    std::uint64_t* text_file_offset = nullptr,
    std::uint64_t* text_virtual_address = nullptr,
    std::string* error = nullptr);

[[nodiscard]] std::optional<FirstStaticExecutable> link_first_static_executable(
    const std::vector<std::string>& object_paths,
    std::string* error = nullptr);

}  // namespace c4c::backend::aarch64::linker

// Translated from /Users/chi-shengwu/c4c/ref/claudes-c-compiler/src/backend/arm/linker/input.rs
#include "mod.hpp"

#include <fstream>
#include <iterator>

namespace c4c::backend::aarch64::linker {
namespace {

std::vector<std::uint8_t> read_file_bytes(const std::string& path, std::string* error) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    if (error != nullptr) *error = "failed to open object file: " + path;
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

}  // namespace

std::optional<std::vector<LoadedInputObject>> load_first_static_input_objects(
    const std::vector<std::string>& object_paths,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (object_paths.size() < 2) {
    if (error != nullptr) {
      *error = "first static-link slice requires at least two object files";
    }
    return std::nullopt;
  }

  std::vector<LoadedInputObject> loaded_objects;
  loaded_objects.reserve(object_paths.size());

  for (const auto& object_path : object_paths) {
    std::string read_error;
    const auto bytes = read_file_bytes(object_path, &read_error);
    if (!read_error.empty()) {
      if (error != nullptr) *error = read_error;
      return std::nullopt;
    }

    std::string parse_error;
    auto parsed = linker_common::parse_elf64_object(
        bytes, object_path, elf::EM_AARCH64, &parse_error);
    if (!parsed.has_value()) {
      if (error != nullptr) *error = parse_error;
      return std::nullopt;
    }

    loaded_objects.push_back(LoadedInputObject{
        .path = object_path,
        .object = std::move(*parsed),
    });
  }

  return loaded_objects;
}

}  // namespace c4c::backend::aarch64::linker

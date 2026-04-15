// Translated from /Users/chi-shengwu/c4c/ref/claudes-c-compiler/src/backend/arm/linker/input.rs
#include "mod.hpp"

#include <fstream>
#include <iterator>
#include <unordered_set>

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

std::vector<std::string> collect_unresolved_symbols(
    const std::vector<LoadedInputObject>& loaded_objects) {
  std::unordered_set<std::string> defined_symbols;
  for (const auto& loaded_object : loaded_objects) {
    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || symbol.is_undefined() || symbol.sym_type() == elf::STT_SECTION) {
        continue;
      }
      if (symbol.is_global() || symbol.is_weak()) defined_symbols.insert(symbol.name);
    }
  }

  std::vector<std::string> unresolved_symbols;
  std::unordered_set<std::string> seen_symbols;
  for (const auto& loaded_object : loaded_objects) {
    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || !symbol.is_undefined() || symbol.is_weak()) continue;
      if (defined_symbols.find(symbol.name) != defined_symbols.end()) continue;
      if (seen_symbols.insert(symbol.name).second) unresolved_symbols.push_back(symbol.name);
    }
  }

  return unresolved_symbols;
}

bool append_archive_members_for_first_static_slice(const std::string& archive_path,
                                                   const std::vector<std::uint8_t>& bytes,
                                                   std::vector<LoadedInputObject>* loaded_objects,
                                                   std::string* error) {
  std::string parse_error;
  const auto parsed_archive = linker_common::parse_elf64_archive(
      bytes, archive_path, elf::EM_AARCH64, &parse_error);
  if (!parsed_archive.has_value()) {
    if (error != nullptr) *error = parse_error;
    return false;
  }

  const auto unresolved_symbols = collect_unresolved_symbols(*loaded_objects);
  std::unordered_set<std::size_t> selected_members;
  for (const auto& symbol_name : unresolved_symbols) {
    const auto member_index = parsed_archive->find_member_index_for_symbol(symbol_name);
    if (!member_index.has_value() || !selected_members.insert(*member_index).second) continue;

    const auto& member = parsed_archive->members[*member_index];
    if (!member.object.has_value()) continue;
    loaded_objects->push_back(LoadedInputObject{
        .path = member.object->source_name,
        .object = *member.object,
    });
  }

  if (loaded_objects->size() < 2) {
    if (error != nullptr) {
      *error = archive_path +
               ": archive did not provide a member for the first static-link slice";
    }
    return false;
  }

  return true;
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

    if (elf::is_archive_file(bytes)) {
      if (!append_archive_members_for_first_static_slice(
              object_path, bytes, &loaded_objects, error)) {
        return std::nullopt;
      }
      continue;
    }

    std::string parse_error;
    auto parsed = linker_common::parse_elf64_object(
        bytes, object_path, elf::EM_AARCH64, &parse_error);
    if (!parsed.has_value()) {
      if (error != nullptr) *error = parse_error;
      return std::nullopt;
    }

    loaded_objects.push_back(
        LoadedInputObject{.path = object_path, .object = std::move(*parsed)});
  }

  return loaded_objects;
}

}  // namespace c4c::backend::aarch64::linker

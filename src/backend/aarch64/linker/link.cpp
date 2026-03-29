// Translated from /Users/chi-shengwu/c4c/ref/claudes-c-compiler/src/backend/arm/linker/link.rs
#include "mod.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <unordered_map>

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

std::vector<std::string> sorted_symbols(std::vector<std::string> symbols) {
  std::sort(symbols.begin(), symbols.end());
  symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());
  return symbols;
}

}  // namespace

std::optional<FirstStaticLinkSlice> inspect_first_static_link_slice(
    const std::vector<std::string>& object_paths,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (object_paths.size() < 2) {
    if (error != nullptr) {
      *error = "first static-link slice requires at least two object files";
    }
    return std::nullopt;
  }

  std::vector<linker_common::Elf64Object> parsed_objects;
  parsed_objects.reserve(object_paths.size());

  std::unordered_map<std::string, std::string> symbol_provider;
  FirstStaticLinkSlice slice;
  slice.case_name = "aarch64-static-call26-two-object";
  slice.objects.reserve(object_paths.size());

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

    InputObjectSummary summary;
    summary.path = object_path;

    for (const auto& symbol : parsed->symbols) {
      if (symbol.name.empty() || symbol.sym_type() == elf::STT_SECTION) continue;
      if (symbol.is_undefined()) {
        if (!symbol.is_weak()) summary.undefined_symbols.push_back(symbol.name);
        continue;
      }
      if (symbol.is_global() || symbol.is_weak()) {
        summary.defined_symbols.push_back(symbol.name);
        symbol_provider.emplace(symbol.name, object_path);
      }
    }

    summary.defined_symbols = sorted_symbols(std::move(summary.defined_symbols));
    summary.undefined_symbols = sorted_symbols(std::move(summary.undefined_symbols));
    slice.objects.push_back(summary);
    parsed_objects.push_back(std::move(*parsed));
  }

  for (std::size_t object_index = 0; object_index < parsed_objects.size(); ++object_index) {
    const auto& object = parsed_objects[object_index];
    for (std::size_t section_index = 0; section_index < object.sections.size(); ++section_index) {
      const auto& section = object.sections[section_index];
      if ((section.flags & elf::SHF_ALLOC) != 0 && !section.name.empty()) {
        slice.merged_output_sections.push_back(section.name);
      }

      for (const auto& relocation : object.relocations[section_index]) {
        if (relocation.sym_idx >= object.symbols.size()) {
          if (error != nullptr) {
            *error = object.source_name + ": relocation symbol index out of bounds";
          }
          return std::nullopt;
        }

        const auto& symbol = object.symbols[relocation.sym_idx];
        InputRelocationSummary relocation_summary;
        relocation_summary.object_path = object_paths[object_index];
        relocation_summary.section_name = section.name;
        relocation_summary.symbol_name = symbol.name;
        relocation_summary.relocation_type = relocation.rela_type;
        relocation_summary.offset = relocation.offset;
        relocation_summary.addend = relocation.addend;

        if (!symbol.name.empty()) {
          const auto provider = symbol_provider.find(symbol.name);
          if (provider != symbol_provider.end()) {
            relocation_summary.resolved = true;
            relocation_summary.resolved_object_path = provider->second;
          }
        } else if (!symbol.is_undefined()) {
          relocation_summary.resolved = true;
          relocation_summary.resolved_object_path = object_paths[object_index];
        }

        slice.relocations.push_back(std::move(relocation_summary));
      }
    }
  }

  slice.merged_output_sections = sorted_symbols(std::move(slice.merged_output_sections));

  std::vector<std::string> unresolved_symbols;
  for (const auto& object : slice.objects) {
    for (const auto& symbol_name : object.undefined_symbols) {
      if (symbol_provider.find(symbol_name) == symbol_provider.end()) {
        unresolved_symbols.push_back(symbol_name);
      }
    }
  }
  slice.unresolved_symbols = sorted_symbols(std::move(unresolved_symbols));

  return slice;
}

}  // namespace c4c::backend::aarch64::linker

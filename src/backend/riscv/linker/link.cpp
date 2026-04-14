// Translated from ref/claudes-c-compiler/src/backend/riscv/linker/link.rs
// The first activation packet exposes only the minimal static-exec contract
// needed by the current return-add object lane.

#include "mod.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend::riscv::linker {
namespace {

constexpr std::uint64_t kFirstStaticTextFileOffset = 64 + 56;

std::vector<std::string> sorted_symbols(std::vector<std::string> symbols) {
  std::sort(symbols.begin(), symbols.end());
  symbols.erase(std::unique(symbols.begin(), symbols.end()), symbols.end());
  return symbols;
}

std::optional<std::size_t> find_text_section_index(const linker_common::Elf64Object& object) {
  for (std::size_t index = 0; index < object.sections.size(); ++index) {
    if (object.sections[index].name == ".text") return index;
  }
  return std::nullopt;
}

std::vector<std::uint8_t> read_file_bytes(const std::string& path, std::string* error) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    if (error != nullptr) *error = "failed to open object file: " + path;
    return {};
  }
  return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input),
                                   std::istreambuf_iterator<char>());
}

bool write_file_bytes(const std::string& path,
                      const std::vector<std::uint8_t>& bytes,
                      std::string* error) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) {
    if (error != nullptr) *error = "failed to open output file: " + path;
    return false;
  }
  output.write(reinterpret_cast<const char*>(bytes.data()),
               static_cast<std::streamsize>(bytes.size()));
  if (!output.good()) {
    if (error != nullptr) *error = "failed to write output file: " + path;
    return false;
  }
  return true;
}

std::vector<std::string> collect_object_paths(const std::vector<const char*>& paths) {
  std::vector<std::string> out;
  out.reserve(paths.size());
  for (const char* path : paths) {
    if (path != nullptr && *path != '\0') out.emplace_back(path);
  }
  return out;
}

bool any_named_inputs(const std::vector<const char*>& paths) {
  return std::any_of(paths.begin(), paths.end(), [](const char* path) {
    return path != nullptr && *path != '\0';
  });
}

bool is_supported_builtin_arg(const std::string& arg) {
  return arg == "-static" || arg == "-nostdlib";
}

}  // namespace

std::optional<std::vector<LoadedInputObject>> load_first_static_input_objects(
    const std::vector<std::string>& object_paths,
    std::string* error) {
  if (error != nullptr) error->clear();
  if (object_paths.empty()) {
    if (error != nullptr) *error = "first static executable requires at least one RV64 object file";
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
      if (error != nullptr) {
        *error = object_path +
                 ": first RV64 static executable contract only accepts relocatable object inputs";
      }
      return std::nullopt;
    }

    std::string parse_error;
    auto parsed = linker_common::parse_elf64_object(bytes, object_path, kEmRiscv, &parse_error);
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

std::optional<FirstStaticLinkSlice> inspect_first_static_link_slice(
    const std::vector<std::string>& object_paths,
    std::string* error) {
  if (error != nullptr) error->clear();
  const auto loaded_objects = load_first_static_input_objects(object_paths, error);
  if (!loaded_objects.has_value()) return std::nullopt;

  std::unordered_map<std::string, std::string> symbol_provider;
  FirstStaticLinkSlice slice;
  slice.case_name = "riscv64-static-reloc-free-single-object";
  slice.objects.reserve(loaded_objects->size());

  for (const auto& loaded_object : *loaded_objects) {
    InputObjectSummary summary;
    summary.path = loaded_object.path;

    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || symbol.sym_type() == elf::STT_SECTION) continue;
      if (symbol.is_undefined()) {
        if (!symbol.is_weak()) summary.undefined_symbols.push_back(symbol.name);
        continue;
      }
      if (symbol.is_global() || symbol.is_weak()) {
        summary.defined_symbols.push_back(symbol.name);
        symbol_provider.emplace(symbol.name, loaded_object.path);
      }
    }

    summary.defined_symbols = sorted_symbols(std::move(summary.defined_symbols));
    summary.undefined_symbols = sorted_symbols(std::move(summary.undefined_symbols));
    slice.objects.push_back(std::move(summary));
  }

  for (const auto& loaded_object : *loaded_objects) {
    const auto& object = loaded_object.object;
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
        relocation_summary.object_path = loaded_object.path;
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
          relocation_summary.resolved_object_path = loaded_object.path;
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

std::optional<FirstStaticExecutable> link_first_static_executable(
    const std::vector<std::string>& object_paths,
    std::string* error) {
  if (error != nullptr) error->clear();
  const auto loaded_objects = load_first_static_input_objects(object_paths, error);
  if (!loaded_objects.has_value()) return std::nullopt;

  std::unordered_map<std::string, std::uint64_t> text_offsets;
  std::unordered_map<std::string, std::uint64_t> symbol_addresses;
  std::unordered_set<std::string> unresolved_symbols;
  std::vector<std::uint8_t> merged_text;

  for (const auto& loaded_object : *loaded_objects) {
    const auto text_index = find_text_section_index(loaded_object.object);
    if (!text_index.has_value()) {
      if (error != nullptr) *error = loaded_object.path + ": missing .text section";
      return std::nullopt;
    }

    text_offsets.emplace(loaded_object.path, merged_text.size());
    const auto& text_bytes = loaded_object.object.section_data[*text_index];
    merged_text.insert(merged_text.end(), text_bytes.begin(), text_bytes.end());

    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || symbol.is_undefined() || symbol.sym_type() == elf::STT_SECTION ||
          symbol.shndx != *text_index) {
        continue;
      }
      if (symbol.is_global() || symbol.is_weak()) {
        symbol_addresses.emplace(symbol.name, symbol.value + text_offsets[loaded_object.path]);
      }
    }

    for (const auto& symbol : loaded_object.object.symbols) {
      if (symbol.name.empty() || !symbol.is_undefined() || symbol.is_weak()) continue;
      unresolved_symbols.insert(symbol.name);
    }
  }

  for (auto it = unresolved_symbols.begin(); it != unresolved_symbols.end();) {
    if (symbol_addresses.find(*it) != symbol_addresses.end()) {
      it = unresolved_symbols.erase(it);
    } else {
      ++it;
    }
  }
  if (!unresolved_symbols.empty()) {
    if (error != nullptr) {
      *error = "first static executable still has unresolved symbols: " + *unresolved_symbols.begin();
    }
    return std::nullopt;
  }

  const std::uint64_t text_virtual_address = kFirstStaticBaseAddress + kFirstStaticTextFileOffset;
  for (auto& [name, address] : symbol_addresses) {
    address += text_virtual_address;
  }

  if (!apply_first_static_text_relocations(*loaded_objects, symbol_addresses, text_offsets,
                                           text_virtual_address, &merged_text, error)) {
    return std::nullopt;
  }

  const auto entry_it = symbol_addresses.find("main");
  if (entry_it == symbol_addresses.end()) {
    if (error != nullptr) *error = "first static executable requires a global main symbol";
    return std::nullopt;
  }

  std::uint64_t emitted_text_file_offset = 0;
  std::uint64_t emitted_text_virtual_address = 0;
  const auto image = emit_first_static_executable_image(
      merged_text, kFirstStaticBaseAddress, entry_it->second, &emitted_text_file_offset,
      &emitted_text_virtual_address, error);
  if (!image.has_value()) return std::nullopt;

  return FirstStaticExecutable{
      .image = *image,
      .base_address = kFirstStaticBaseAddress,
      .entry_address = entry_it->second,
      .text_file_offset = emitted_text_file_offset,
      .text_virtual_address = emitted_text_virtual_address,
      .text_size = merged_text.size(),
      .symbol_addresses = std::move(symbol_addresses),
  };
}

bool link_builtin(const std::vector<const char*>& object_files,
                  const std::string& output_path,
                  const std::vector<std::string>& user_args,
                  const std::vector<const char*>& lib_paths,
                  const std::vector<const char*>& needed_libs,
                  const std::vector<const char*>& crt_objects_before,
                  const std::vector<const char*>& crt_objects_after,
                  std::string* error) {
  if (error != nullptr) error->clear();
  if (output_path.empty()) {
    if (error != nullptr) *error = "RV64 link_builtin requires a non-empty output path";
    return false;
  }
  if (any_named_inputs(lib_paths) || any_named_inputs(needed_libs) ||
      any_named_inputs(crt_objects_before) || any_named_inputs(crt_objects_after)) {
    if (error != nullptr) {
      *error = "RV64 first static executable contract does not yet support libraries or CRT inputs";
    }
    return false;
  }
  for (std::size_t i = 0; i < user_args.size(); ++i) {
    if (user_args[i] == "-o" && i + 1 < user_args.size()) {
      ++i;
      continue;
    }
    if (!is_supported_builtin_arg(user_args[i])) {
      if (error != nullptr) {
        *error = "RV64 first static executable contract does not yet support linker arg: " +
                 user_args[i];
      }
      return false;
    }
  }

  const auto linked = link_first_static_executable(collect_object_paths(object_files), error);
  if (!linked.has_value()) return false;
  return write_file_bytes(output_path, linked->image, error);
}

bool link_shared(const std::vector<const char*>& object_files,
                 const std::string& output_path,
                 const std::vector<std::string>& user_args,
                 const std::vector<const char*>& lib_paths,
                 const std::vector<const char*>& needed_libs,
                 std::string* error) {
  (void)object_files;
  (void)output_path;
  (void)user_args;
  (void)lib_paths;
  (void)needed_libs;
  if (error != nullptr) {
    *error =
        "RISC-V shared-library linking is not yet activated in the first static-executable contract surface";
  }
  return false;
}

}  // namespace c4c::backend::riscv::linker

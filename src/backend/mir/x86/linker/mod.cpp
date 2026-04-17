// Mechanical C++ translation of ref/claudes-c-compiler/src/backend/x86/linker/mod.rs
// Native x86-64 ELF linker module surface.

#include "mod.hpp"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace c4c::backend::x86::linker {

namespace {

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

  const auto loaded_objects = load_first_static_input_objects(object_paths, error);
  if (!loaded_objects.has_value()) return std::nullopt;

  std::unordered_map<std::string, std::string> symbol_provider;
  FirstStaticLinkSlice slice;
  slice.case_name = "x86_64-static-plt32-two-object";
  slice.objects.reserve(object_paths.size());

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
    slice.objects.push_back(summary);
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
      if (symbol.name.empty() || symbol.is_undefined() || symbol.sym_type() == elf::STT_SECTION) {
        continue;
      }
      if (symbol.shndx != elf::SHN_UNDEF && (symbol.is_global() || symbol.is_weak())) {
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
      *error = "first static-link slice still has unresolved symbols: " + *unresolved_symbols.begin();
    }
    return std::nullopt;
  }

  constexpr std::uint64_t kBaseAddress = 0x400000;
  constexpr std::uint64_t kTextFileOffset = 64 + 56;
  const std::uint64_t text_virtual_address = kBaseAddress + kTextFileOffset;
  for (auto& [name, address] : symbol_addresses) {
    address += text_virtual_address;
  }

  if (!apply_first_static_text_relocations(*loaded_objects, symbol_addresses, text_offsets,
                                           text_virtual_address, &merged_text, error)) {
    return std::nullopt;
  }

  const auto entry_it = symbol_addresses.find("main");
  if (entry_it == symbol_addresses.end()) {
    if (error != nullptr) *error = "first static-link slice requires a global main symbol";
    return std::nullopt;
  }

  std::uint64_t emitted_text_file_offset = 0;
  std::uint64_t emitted_text_virtual_address = 0;
  const auto image = emit_first_static_executable_image(
      merged_text, kBaseAddress, entry_it->second, &emitted_text_file_offset,
      &emitted_text_virtual_address, error);
  if (!image.has_value()) return std::nullopt;

  return FirstStaticExecutable{
      .image = *image,
      .base_address = kBaseAddress,
      .entry_address = entry_it->second,
      .text_file_offset = emitted_text_file_offset,
      .text_virtual_address = emitted_text_virtual_address,
      .text_size = merged_text.size(),
      .symbol_addresses = std::move(symbol_addresses),
  };
}

// The Rust module exposes the ELF helpers, the x86-specific symbol type, the
// input loader, the PLT/GOT builder, and the two public linker entry points.
// In C++ we keep the same module-level shape through declarations in mod.hpp
// and define the real logic in the sibling translation units.

}  // namespace c4c::backend::x86::linker

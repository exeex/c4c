#include "mir/object/model.hpp"

#include <algorithm>
#include <utility>

namespace c4c::backend::mir::object {

namespace {

bool name_equals(const std::string& lhs, std::string_view rhs) {
  return lhs.size() == rhs.size() &&
         lhs.compare(0, lhs.size(), rhs.data(), rhs.size()) == 0;
}

}  // namespace

std::optional<SectionId> find_section_id(const ObjectModule& module,
                                         std::string_view name) {
  for (const auto& section : module.sections) {
    if (name_equals(section.name, name)) {
      return section.id;
    }
  }
  return std::nullopt;
}

SectionRecord* find_section(ObjectModule& module, SectionId id) {
  if (id.value >= module.sections.size()) {
    return nullptr;
  }
  if (module.sections[id.value].id != id) {
    return nullptr;
  }
  return &module.sections[id.value];
}

const SectionRecord* find_section(const ObjectModule& module, SectionId id) {
  if (id.value >= module.sections.size()) {
    return nullptr;
  }
  if (module.sections[id.value].id != id) {
    return nullptr;
  }
  return &module.sections[id.value];
}

SectionRecord* find_section(ObjectModule& module, std::string_view name) {
  const auto id = find_section_id(module, name);
  return id.has_value() ? find_section(module, *id) : nullptr;
}

const SectionRecord* find_section(const ObjectModule& module,
                                  std::string_view name) {
  const auto id = find_section_id(module, name);
  return id.has_value() ? find_section(module, *id) : nullptr;
}

SectionRecord& create_section(ObjectModule& module, std::string name,
                              SectionKind kind, std::uint64_t align_bytes,
                              bool alloc, bool executable, bool writable) {
  const SectionId id{module.sections.size()};
  module.sections.push_back(SectionRecord{
      .id = id,
      .name = std::move(name),
      .kind = kind,
      .align_bytes = std::max<std::uint64_t>(1, align_bytes),
      .alloc = alloc,
      .executable = executable,
      .writable = writable,
  });
  return module.sections.back();
}

SectionRecord& get_or_create_section(ObjectModule& module, std::string name,
                                     SectionKind kind,
                                     std::uint64_t align_bytes, bool alloc,
                                     bool executable, bool writable) {
  if (auto* existing = find_section(module, name)) {
    existing->kind = kind;
    existing->align_bytes =
        std::max(existing->align_bytes, std::max<std::uint64_t>(1, align_bytes));
    existing->alloc = alloc;
    existing->executable = executable;
    existing->writable = writable;
    return *existing;
  }
  return create_section(module, std::move(name), kind, align_bytes, alloc,
                        executable, writable);
}

std::uint64_t append_section_bytes(SectionRecord& section,
                                   const std::vector<std::uint8_t>& bytes) {
  const auto offset = section.size_bytes;
  section.bytes.insert(section.bytes.end(), bytes.begin(), bytes.end());
  section.size_bytes += bytes.size();
  return offset;
}

std::uint64_t append_section_bytes(SectionRecord& section,
                                   std::initializer_list<std::uint8_t> bytes) {
  const auto offset = section.size_bytes;
  section.bytes.insert(section.bytes.end(), bytes.begin(), bytes.end());
  section.size_bytes += bytes.size();
  return offset;
}

std::uint64_t reserve_section_bytes(SectionRecord& section,
                                    std::uint64_t size_bytes) {
  const auto offset = section.size_bytes;
  section.size_bytes += size_bytes;
  return offset;
}

std::uint64_t align_section(SectionRecord& section, std::uint64_t align_bytes,
                            std::uint8_t fill_byte) {
  const auto normalized_align = std::max<std::uint64_t>(1, align_bytes);
  const auto offset = section.size_bytes;
  const auto remainder = offset % normalized_align;
  section.align_bytes = std::max(section.align_bytes, normalized_align);
  if (remainder == 0) {
    return offset;
  }
  const auto padding = normalized_align - remainder;
  if (section.bytes.size() == section.size_bytes) {
    section.bytes.insert(section.bytes.end(), padding, fill_byte);
  }
  section.size_bytes += padding;
  return offset;
}

std::optional<SymbolId> find_symbol_id(const ObjectModule& module,
                                       std::string_view name) {
  for (const auto& symbol : module.symbols) {
    if (name_equals(symbol.name, name)) {
      return symbol.id;
    }
  }
  return std::nullopt;
}

SymbolRecord* find_symbol(ObjectModule& module, SymbolId id) {
  if (id.value >= module.symbols.size()) {
    return nullptr;
  }
  if (module.symbols[id.value].id != id) {
    return nullptr;
  }
  return &module.symbols[id.value];
}

const SymbolRecord* find_symbol(const ObjectModule& module, SymbolId id) {
  if (id.value >= module.symbols.size()) {
    return nullptr;
  }
  if (module.symbols[id.value].id != id) {
    return nullptr;
  }
  return &module.symbols[id.value];
}

SymbolRecord* find_symbol(ObjectModule& module, std::string_view name) {
  const auto id = find_symbol_id(module, name);
  return id.has_value() ? find_symbol(module, *id) : nullptr;
}

const SymbolRecord* find_symbol(const ObjectModule& module,
                                std::string_view name) {
  const auto id = find_symbol_id(module, name);
  return id.has_value() ? find_symbol(module, *id) : nullptr;
}

SymbolRecord& define_symbol(ObjectModule& module, std::string name,
                            SymbolBinding binding, SymbolKind kind,
                            SectionId section, std::uint64_t value,
                            std::uint64_t size_bytes) {
  if (auto* existing = find_symbol(module, name)) {
    existing->binding = binding;
    existing->kind = kind;
    existing->section = section;
    existing->value = value;
    existing->size_bytes = size_bytes;
    return *existing;
  }
  const SymbolId id{module.symbols.size()};
  module.symbols.push_back(SymbolRecord{
      .id = id,
      .name = std::move(name),
      .binding = binding,
      .kind = kind,
      .section = section,
      .value = value,
      .size_bytes = size_bytes,
  });
  return module.symbols.back();
}

SymbolRecord& declare_undefined_symbol(ObjectModule& module, std::string name,
                                       SymbolBinding binding, SymbolKind kind,
                                       std::uint64_t size_bytes) {
  if (auto* existing = find_symbol(module, name)) {
    existing->binding = binding;
    existing->kind = kind;
    existing->section = std::nullopt;
    existing->value = 0;
    existing->size_bytes = size_bytes;
    return *existing;
  }
  const SymbolId id{module.symbols.size()};
  module.symbols.push_back(SymbolRecord{
      .id = id,
      .name = std::move(name),
      .binding = binding,
      .kind = kind,
      .size_bytes = size_bytes,
  });
  return module.symbols.back();
}

LabelRecord& bind_label(ObjectModule& module, std::string name,
                        SectionId section, std::uint64_t offset) {
  const LabelId id{module.labels.size()};
  module.labels.push_back(LabelRecord{
      .id = id,
      .name = std::move(name),
      .section = section,
      .offset = offset,
  });
  return module.labels.back();
}

LabelRecord& bind_label_at_current_offset(ObjectModule& module,
                                          std::string name,
                                          const SectionRecord& section) {
  return bind_label(module, std::move(name), section.id, section.size_bytes);
}

RelocationRecord& attach_relocation(ObjectModule& module, SectionId section,
                                    std::uint64_t offset, std::uint32_t type,
                                    SymbolId symbol, std::int64_t addend) {
  module.relocations.push_back(RelocationRecord{
      .section = section,
      .offset = offset,
      .type = type,
      .symbol = symbol,
      .addend = addend,
      .base = RelocationBase::Symbol,
  });
  return module.relocations.back();
}

RelocationRecord& attach_label_relocation(ObjectModule& module,
                                          SectionId section,
                                          std::uint64_t offset,
                                          std::uint32_t type, SymbolId symbol,
                                          LabelId label,
                                          std::int64_t addend) {
  module.relocations.push_back(RelocationRecord{
      .section = section,
      .offset = offset,
      .type = type,
      .symbol = symbol,
      .addend = addend,
      .base = RelocationBase::Label,
      .label = label,
  });
  return module.relocations.back();
}

const char* section_kind_name(SectionKind kind) {
  switch (kind) {
    case SectionKind::Text:
      return "text";
    case SectionKind::Data:
      return "data";
    case SectionKind::Bss:
      return "bss";
    case SectionKind::Custom:
      return "custom";
  }
  return "unknown";
}

const char* symbol_binding_name(SymbolBinding binding) {
  switch (binding) {
    case SymbolBinding::Local:
      return "local";
    case SymbolBinding::Global:
      return "global";
  }
  return "unknown";
}

const char* symbol_kind_name(SymbolKind kind) {
  switch (kind) {
    case SymbolKind::NoType:
      return "notype";
    case SymbolKind::Function:
      return "function";
    case SymbolKind::Object:
      return "object";
    case SymbolKind::Section:
      return "section";
  }
  return "unknown";
}

const char* relocation_base_name(RelocationBase base) {
  switch (base) {
    case RelocationBase::SectionOffset:
      return "section_offset";
    case RelocationBase::Label:
      return "label";
    case RelocationBase::Symbol:
      return "symbol";
  }
  return "unknown";
}

bool is_undefined_symbol(const SymbolRecord& symbol) {
  return !symbol.section.has_value();
}

bool is_section_local_label(const LabelRecord& label, const SectionRecord& section) {
  return label.section == section.id;
}

}  // namespace c4c::backend::mir::object

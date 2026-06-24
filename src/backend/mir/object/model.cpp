#include "mir/object/model.hpp"

namespace c4c::backend::mir::object {

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

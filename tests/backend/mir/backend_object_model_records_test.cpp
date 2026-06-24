#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>

namespace {

namespace object = c4c::backend::mir::object;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

object::ObjectModule sample_module() {
  object::ObjectModule module;

  module.sections.push_back(object::SectionRecord{
      .id = object::SectionId{0},
      .name = ".text",
      .kind = object::SectionKind::Text,
      .bytes = {0x13, 0x05, 0x00, 0x00},
      .size_bytes = 4,
      .align_bytes = 4,
      .alloc = true,
      .executable = true,
      .writable = false,
  });
  module.sections.push_back(object::SectionRecord{
      .id = object::SectionId{1},
      .name = ".data",
      .kind = object::SectionKind::Data,
      .bytes = {0x2a, 0x00, 0x00, 0x00},
      .size_bytes = 4,
      .align_bytes = 4,
      .alloc = true,
      .executable = false,
      .writable = true,
  });
  module.sections.push_back(object::SectionRecord{
      .id = object::SectionId{2},
      .name = ".bss",
      .kind = object::SectionKind::Bss,
      .size_bytes = 16,
      .align_bytes = 8,
      .alloc = true,
      .executable = false,
      .writable = true,
  });

  module.labels.push_back(object::LabelRecord{
      .id = object::LabelId{0},
      .name = ".Lentry",
      .section = object::SectionId{0},
      .offset = 0,
  });
  module.labels.push_back(object::LabelRecord{
      .id = object::LabelId{1},
      .name = ".Lliteral",
      .section = object::SectionId{1},
      .offset = 0,
  });

  module.symbols.push_back(object::SymbolRecord{
      .id = object::SymbolId{0},
      .name = "main",
      .binding = object::SymbolBinding::Global,
      .kind = object::SymbolKind::Function,
      .section = object::SectionId{0},
      .value = 0,
      .size_bytes = 4,
  });
  module.symbols.push_back(object::SymbolRecord{
      .id = object::SymbolId{1},
      .name = "helper",
      .binding = object::SymbolBinding::Local,
      .kind = object::SymbolKind::Function,
      .section = object::SectionId{0},
      .value = 4,
      .size_bytes = 0,
  });
  module.symbols.push_back(object::SymbolRecord{
      .id = object::SymbolId{2},
      .name = "extern_func",
      .binding = object::SymbolBinding::Global,
      .kind = object::SymbolKind::Function,
  });
  module.symbols.push_back(object::SymbolRecord{
      .id = object::SymbolId{3},
      .name = ".bss",
      .binding = object::SymbolBinding::Local,
      .kind = object::SymbolKind::Section,
      .section = object::SectionId{2},
  });

  module.relocations.push_back(object::RelocationRecord{
      .section = object::SectionId{0},
      .offset = 2,
      .type = 26,
      .symbol = object::SymbolId{2},
      .addend = -4,
      .base = object::RelocationBase::Symbol,
  });
  module.relocations.push_back(object::RelocationRecord{
      .section = object::SectionId{0},
      .offset = 8,
      .type = 17,
      .symbol = object::SymbolId{0},
      .addend = 12,
      .base = object::RelocationBase::Label,
      .label = object::LabelId{1},
  });

  return module;
}

int records_standard_section_shapes() {
  const auto module = sample_module();
  if (module.sections.size() != 3) {
    return fail("expected text, data, and bss sections");
  }
  if (module.sections[0].name != ".text" ||
      module.sections[0].kind != object::SectionKind::Text ||
      !module.sections[0].executable || module.sections[0].writable ||
      module.sections[0].bytes.size() != 4) {
    return fail("expected executable text section with bytes");
  }
  if (module.sections[1].name != ".data" ||
      module.sections[1].kind != object::SectionKind::Data ||
      module.sections[1].executable || !module.sections[1].writable ||
      module.sections[1].bytes.size() != 4) {
    return fail("expected writable data section with bytes");
  }
  if (module.sections[2].name != ".bss" ||
      module.sections[2].kind != object::SectionKind::Bss ||
      module.sections[2].executable || !module.sections[2].writable ||
      !module.sections[2].bytes.empty() || module.sections[2].size_bytes != 16) {
    return fail("expected sized bss section without bytes");
  }
  return 0;
}

int records_defined_and_undefined_symbols() {
  const auto module = sample_module();
  const auto& global_function = module.symbols[0];
  const auto& local_function = module.symbols[1];
  const auto& undefined_function = module.symbols[2];
  const auto& bss_section_symbol = module.symbols[3];

  if (global_function.binding != object::SymbolBinding::Global ||
      global_function.kind != object::SymbolKind::Function ||
      global_function.section != std::optional<object::SectionId>{object::SectionId{0}} ||
      object::is_undefined_symbol(global_function)) {
    return fail("expected defined global function symbol");
  }
  if (local_function.binding != object::SymbolBinding::Local ||
      local_function.kind != object::SymbolKind::Function ||
      local_function.section != std::optional<object::SectionId>{object::SectionId{0}}) {
    return fail("expected defined local function symbol");
  }
  if (undefined_function.binding != object::SymbolBinding::Global ||
      undefined_function.kind != object::SymbolKind::Function ||
      !object::is_undefined_symbol(undefined_function)) {
    return fail("expected undefined external function symbol");
  }
  if (bss_section_symbol.kind != object::SymbolKind::Section ||
      bss_section_symbol.section != std::optional<object::SectionId>{object::SectionId{2}}) {
    return fail("expected local section symbol");
  }
  return 0;
}

int records_section_local_labels() {
  const auto module = sample_module();
  if (!object::is_section_local_label(module.labels[0], module.sections[0]) ||
      object::is_section_local_label(module.labels[0], module.sections[1]) ||
      module.labels[1].section != object::SectionId{1}) {
    return fail("expected labels to stay scoped to their section ids");
  }
  return 0;
}

int records_numeric_relocations_with_addends() {
  const auto module = sample_module();
  if (module.relocations.size() != 2) {
    return fail("expected symbol and label relocations");
  }
  const auto& symbol_reloc = module.relocations[0];
  if (symbol_reloc.section != object::SectionId{0} || symbol_reloc.offset != 2 ||
      symbol_reloc.type != 26 || symbol_reloc.symbol != object::SymbolId{2} ||
      symbol_reloc.addend != -4 ||
      symbol_reloc.base != object::RelocationBase::Symbol ||
      symbol_reloc.label.has_value()) {
    return fail("expected numeric symbol relocation with signed addend");
  }
  const auto& label_reloc = module.relocations[1];
  if (label_reloc.section != object::SectionId{0} || label_reloc.offset != 8 ||
      label_reloc.type != 17 || label_reloc.symbol != object::SymbolId{0} ||
      label_reloc.addend != 12 || label_reloc.base != object::RelocationBase::Label ||
      label_reloc.label != std::optional<object::LabelId>{object::LabelId{1}}) {
    return fail("expected numeric label relocation with signed addend");
  }
  return 0;
}

int records_target_neutral_names() {
  if (object::section_kind_name(object::SectionKind::Text) != std::string{"text"} ||
      object::section_kind_name(object::SectionKind::Data) != std::string{"data"} ||
      object::section_kind_name(object::SectionKind::Bss) != std::string{"bss"} ||
      object::symbol_binding_name(object::SymbolBinding::Global) !=
          std::string{"global"} ||
      object::symbol_kind_name(object::SymbolKind::Function) !=
          std::string{"function"} ||
      object::relocation_base_name(object::RelocationBase::Symbol) !=
          std::string{"symbol"}) {
    return fail("expected target-neutral record names");
  }
  return 0;
}

}  // namespace

int main() {
  if (int rc = records_standard_section_shapes(); rc != 0) {
    return rc;
  }
  if (int rc = records_defined_and_undefined_symbols(); rc != 0) {
    return rc;
  }
  if (int rc = records_section_local_labels(); rc != 0) {
    return rc;
  }
  if (int rc = records_numeric_relocations_with_addends(); rc != 0) {
    return rc;
  }
  return records_target_neutral_names();
}

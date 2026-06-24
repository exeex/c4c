#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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

int helpers_construct_target_like_module() {
  object::ObjectModule module;

  const auto text_id =
      object::get_or_create_section(module, ".text", object::SectionKind::Text,
                                    4, true, true, false)
          .id;
  const auto duplicate_text_id =
      object::get_or_create_section(module, ".text", object::SectionKind::Text,
                                    16, true, true, false)
          .id;
  const auto data_id =
      object::create_section(module, ".data", object::SectionKind::Data, 4,
                             true, false, true)
          .id;
  const auto bss_id =
      object::create_section(module, ".bss", object::SectionKind::Bss, 8, true,
                             false, true)
          .id;

  if (module.sections.size() != 3 || duplicate_text_id != text_id) {
    return fail("expected section lookup to avoid duplicate standard sections");
  }

  auto* text = object::find_section(module, text_id);
  auto* data = object::find_section(module, ".data");
  auto* bss = object::find_section(module, bss_id);
  if (text == nullptr || data == nullptr || bss == nullptr ||
      object::find_section_id(module, ".text") !=
          std::optional<object::SectionId>{text_id}) {
    return fail("expected created sections to be findable by id and name");
  }

  const auto entry_offset = object::append_section_bytes(*text, {0x13, 0x05});
  const auto text_align_offset = object::align_section(*text, 4, 0);
  const auto entry_label =
      object::bind_label(module, ".Lentry", text->id, entry_offset).id;
  const auto call_offset = object::append_section_bytes(*text, {0, 0, 0, 0});
  const auto data_offset =
      object::append_section_bytes(*data, std::vector<std::uint8_t>{0x2a});
  object::align_section(*data, 4, 0);
  const auto bss_offset = object::reserve_section_bytes(*bss, 5);
  object::align_section(*bss, 8);

  if (entry_offset != 0 || text_align_offset != 2 || call_offset != 4 ||
      text->size_bytes != 8 || text->bytes.size() != 8 ||
      text->align_bytes != 16 || data_offset != 0 || data->size_bytes != 4 ||
      data->bytes.size() != 4 || bss_offset != 0 || bss->size_bytes != 8 ||
      !bss->bytes.empty()) {
    return fail(
        "expected append, reserve, and alignment helpers to update sections");
  }

  const auto main_symbol =
      object::define_symbol(module, "main", object::SymbolBinding::Global,
                            object::SymbolKind::Function, text->id,
                            entry_offset, text->size_bytes)
          .id;
  const auto extern_symbol =
      object::declare_undefined_symbol(module, "extern_func",
                                       object::SymbolBinding::Global,
                                       object::SymbolKind::Function)
          .id;
  const auto section_symbol =
      object::define_symbol(module, ".bss", object::SymbolBinding::Local,
                            object::SymbolKind::Section, bss->id, 0,
                            bss->size_bytes)
          .id;
  object::declare_undefined_symbol(module, "late_bound",
                                   object::SymbolBinding::Global,
                                   object::SymbolKind::Object, 4);
  const auto late_bound =
      object::define_symbol(module, "late_bound", object::SymbolBinding::Global,
                            object::SymbolKind::Object, data->id, data_offset,
                            4)
          .id;

  const auto* late_bound_record = object::find_symbol(module, "late_bound");
  if (module.symbols.size() != 4 ||
      object::find_symbol_id(module, "main") !=
          std::optional<object::SymbolId>{main_symbol} ||
      object::find_symbol(module, extern_symbol) == nullptr ||
      object::find_symbol(module, ".bss") == nullptr ||
      late_bound_record == nullptr || late_bound_record->section !=
          std::optional<object::SectionId>{data->id} ||
      late_bound.value != 3) {
    return fail("expected symbol helpers to publish and find symbols");
  }

  const auto current_label =
      object::bind_label_at_current_offset(module, ".Lafter_call", *text).id;
  object::attach_relocation(module, text->id, call_offset, 26, extern_symbol,
                            -4);
  object::attach_label_relocation(module, text->id, call_offset + 2, 17,
                                  main_symbol, entry_label, 12);

  if (module.labels.size() != 2 ||
      module.labels[current_label.value].offset != 8 ||
      module.relocations.size() != 2 ||
      module.relocations[0].base != object::RelocationBase::Symbol ||
      module.relocations[0].symbol != extern_symbol ||
      module.relocations[0].addend != -4 ||
      module.relocations[1].base != object::RelocationBase::Label ||
      module.relocations[1].label !=
          std::optional<object::LabelId>{entry_label} ||
      section_symbol.value != 2) {
    return fail("expected label and relocation helpers to record structured fixups");
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
  if (int rc = records_target_neutral_names(); rc != 0) {
    return rc;
  }
  return helpers_construct_target_like_module();
}

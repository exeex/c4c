#include "src/backend/backend.hpp"
#include "src/backend/mir/object/elf_writer.hpp"
#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace backend = c4c::backend;
namespace bir = c4c::backend::bir;
namespace object = c4c::backend::mir::object;

constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t SHT_STRTAB = 3;
constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_NOBITS = 8;

std::uint16_t read_u16(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint16_t>(bytes[offset]) |
         (static_cast<std::uint16_t>(bytes[offset + 1]) << 8);
}

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return static_cast<std::uint32_t>(bytes[offset]) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
}

std::uint64_t read_u64(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  std::uint64_t value = 0;
  for (int shift = 0; shift < 64; shift += 8) {
    value |= static_cast<std::uint64_t>(bytes[offset + shift / 8]) << shift;
  }
  return value;
}

std::int64_t read_i64(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset) {
  return static_cast<std::int64_t>(read_u64(bytes, offset));
}

std::string read_c_string(const std::vector<std::uint8_t>& bytes,
                          std::size_t offset) {
  std::string value;
  while (offset < bytes.size() && bytes[offset] != 0) {
    value.push_back(static_cast<char>(bytes[offset]));
    ++offset;
  }
  return value;
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool has_elf_header_for_machine(const std::vector<std::uint8_t>& bytes,
                                std::uint16_t machine) {
  return bytes.size() >= 64 && bytes[0] == 0x7f && bytes[1] == 'E' &&
         bytes[2] == 'L' && bytes[3] == 'F' && read_u16(bytes, 18) == machine;
}

bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

bir::Module make_return_zero_module(std::string target_triple) {
  bir::Block entry{
      .label = "entry",
      .terminator = bir::Terminator{},
  };
  entry.terminator.value = bir::Value::immediate_i32(0);

  bir::Function function{
      .name = "main",
      .return_type = bir::TypeKind::I32,
      .return_size_bytes = 4,
      .return_align_bytes = 4,
      .blocks = {std::move(entry)},
  };

  bir::Module module;
  module.target_triple = std::move(target_triple);
  module.functions.push_back(std::move(function));
  return module;
}

bir::Module make_rv64_global_object_unsupported_module() {
  auto module = make_return_zero_module("riscv64-linux-gnu");
  const auto link_name = module.names.link_names.intern("global_i32");
  module.globals.push_back(bir::Global{
      .name = "global_i32",
      .link_name_id = link_name,
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .initializer = bir::Value::immediate_i32(7),
      .address_materialization_policy =
          bir::GlobalAddressMaterializationPolicy::Direct,
  });
  return module;
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

int serializes_minimal_relocatable_elf() {
  const auto module = sample_module();
  const auto image = object::write_relocatable_elf(
      module, object::RelocatableElfConfig{.machine = 243, .flags = 0x5});
  if (!image.has_value()) {
    return fail("expected ELF writer to accept explicit ELF64 little-endian config");
  }

  const auto& bytes = image->bytes;
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F' || bytes[4] != 2 ||
      bytes[5] != 1 || read_u16(bytes, 16) != 1 ||
      read_u16(bytes, 18) != 243 || read_u32(bytes, 48) != 0x5) {
    return fail("expected ELF64 relocatable header with target config");
  }

  const auto section_header_offset =
      static_cast<std::size_t>(read_u64(bytes, 40));
  const auto section_entry_size = read_u16(bytes, 58);
  const auto section_count = read_u16(bytes, 60);
  const auto shstr_index = read_u16(bytes, 62);
  if (section_entry_size != 64 || section_count != 8 || shstr_index != 7) {
    return fail("expected standard section header table shape");
  }

  const auto shstr_header = section_header_offset + shstr_index * 64;
  const auto shstr_offset = static_cast<std::size_t>(read_u64(bytes, shstr_header + 24));

  auto section_name = [&](std::uint16_t index) {
    const auto header = section_header_offset + index * 64;
    return read_c_string(bytes, shstr_offset + read_u32(bytes, header));
  };
  auto section_type_at = [&](std::uint16_t index) {
    return read_u32(bytes, section_header_offset + index * 64 + 4);
  };
  auto section_offset_at = [&](std::uint16_t index) {
    return static_cast<std::size_t>(
        read_u64(bytes, section_header_offset + index * 64 + 24));
  };
  auto section_size_at = [&](std::uint16_t index) {
    return read_u64(bytes, section_header_offset + index * 64 + 32);
  };

  if (section_name(1) != ".text" || section_name(2) != ".data" ||
      section_name(3) != ".bss" || section_name(4) != ".rela.text" ||
      section_name(5) != ".symtab" || section_name(6) != ".strtab" ||
      section_name(7) != ".shstrtab") {
    return fail("expected ELF writer to publish standard and relocation sections");
  }
  if (section_type_at(3) != SHT_NOBITS || section_size_at(3) != 16 ||
      section_type_at(4) != SHT_RELA || section_size_at(4) != 48 ||
      section_type_at(5) != SHT_SYMTAB || section_size_at(5) != 120 ||
      section_type_at(6) != SHT_STRTAB) {
    return fail("expected bss, rela, symtab, and string-table section metadata");
  }

  const auto rela_header = section_header_offset + 4 * 64;
  if (read_u32(bytes, rela_header + 40) != 5 ||
      read_u32(bytes, rela_header + 44) != 1 || read_u64(bytes, rela_header + 56) != 24) {
    return fail("expected relocation section to link symtab and target text");
  }

  const auto symtab_header = section_header_offset + 5 * 64;
  if (read_u32(bytes, symtab_header + 40) != 6 ||
      read_u32(bytes, symtab_header + 44) != 3 || read_u64(bytes, symtab_header + 56) != 24) {
    return fail("expected symtab to link strtab and mark first global symbol");
  }

  const auto strtab_offset = section_offset_at(6);
  const auto symtab_offset = section_offset_at(5);
  const auto helper_name = read_c_string(bytes, strtab_offset + read_u32(bytes, symtab_offset + 24));
  const auto bss_name = read_c_string(bytes, strtab_offset + read_u32(bytes, symtab_offset + 48));
  const auto main_name = read_c_string(bytes, strtab_offset + read_u32(bytes, symtab_offset + 72));
  const auto extern_name =
      read_c_string(bytes, strtab_offset + read_u32(bytes, symtab_offset + 96));
  if (helper_name != "helper" || bss_name != ".bss" || main_name != "main" ||
      extern_name != "extern_func" || read_u16(bytes, symtab_offset + 96 + 6) != 0) {
    return fail("expected local symbols before globals and undefined extern");
  }

  const auto rela_offset = section_offset_at(4);
  if (read_u64(bytes, rela_offset) != 2 ||
      read_u64(bytes, rela_offset + 8) != ((std::uint64_t{4} << 32) | 26) ||
      read_i64(bytes, rela_offset + 16) != -4 ||
      read_u64(bytes, rela_offset + 24) != 8 ||
      read_u64(bytes, rela_offset + 32) != ((std::uint64_t{3} << 32) | 17) ||
      read_i64(bytes, rela_offset + 40) != 12) {
    return fail("expected numeric relocation entries with ELF symbol indices");
  }

  return 0;
}

int backend_facade_emits_rv64_object_bytes() {
  const auto module = make_return_zero_module("riscv64-linux-gnu");
  const auto result = backend::emit_module_object(
      backend::BackendModuleInput(module),
      backend::BackendOptions{});
  if (!result.ok() || !result.diagnostic.empty()) {
    return fail("expected backend facade to emit RV64 object bytes");
  }
  if (!has_elf_header_for_machine(result.bytes, 243)) {
    return fail("expected backend facade RV64 object bytes to be ELF64/RISC-V");
  }
  return 0;
}

int backend_facade_emits_aarch64_object_bytes() {
  const auto module = make_return_zero_module("aarch64-linux-gnu");
  const auto result = backend::emit_module_object(
      backend::BackendModuleInput(module),
      backend::BackendOptions{});
  if (!result.ok() || !result.diagnostic.empty()) {
    return fail("expected backend facade to emit AArch64 object bytes");
  }
  if (!has_elf_header_for_machine(result.bytes, 183)) {
    return fail("expected backend facade AArch64 object bytes to be ELF64/AArch64");
  }
  return 0;
}

int backend_facade_rejects_unsupported_object_target() {
  const auto module = make_return_zero_module("x86_64-linux-gnu");
  const auto result = backend::emit_module_object(
      backend::BackendModuleInput(module),
      backend::BackendOptions{});
  if (result.ok() || !result.bytes.empty() ||
      !contains(result.diagnostic, "backend object route unsupported target") ||
      !contains(result.diagnostic, "x86_64")) {
    return fail("expected backend object facade to reject unsupported x86 target");
  }
  return 0;
}

int backend_facade_rejects_unsupported_rv64_object_feature_without_asm_fallback() {
  const auto module = make_rv64_global_object_unsupported_module();
  const auto result = backend::emit_module_object(
      backend::BackendModuleInput(module),
      backend::BackendOptions{});
  if (result.ok() || !result.bytes.empty() ||
      !contains(result.diagnostic,
                "RISC-V backend object route unsupported prepared module shape")) {
    return fail("expected RV64 object facade to reject globals without asm fallback");
  }
  return 0;
}

int backend_facade_preserves_existing_asm_text_api() {
  const auto module = make_return_zero_module("riscv64-linux-gnu");
  const auto text = backend::emit_module(
      backend::BackendModuleInput(module),
      backend::BackendOptions{});
  if (!contains(text, ".text") || !contains(text, "main:") ||
      !contains(text, "ret")) {
    return fail("expected existing backend asm text API to remain available");
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
  if (int rc = helpers_construct_target_like_module(); rc != 0) {
    return rc;
  }
  if (int rc = serializes_minimal_relocatable_elf(); rc != 0) {
    return rc;
  }
  if (int rc = backend_facade_emits_rv64_object_bytes(); rc != 0) {
    return rc;
  }
  if (int rc = backend_facade_emits_aarch64_object_bytes(); rc != 0) {
    return rc;
  }
  if (int rc = backend_facade_rejects_unsupported_object_target(); rc != 0) {
    return rc;
  }
  if (int rc = backend_facade_rejects_unsupported_rv64_object_feature_without_asm_fallback();
      rc != 0) {
    return rc;
  }
  return backend_facade_preserves_existing_asm_text_api();
}

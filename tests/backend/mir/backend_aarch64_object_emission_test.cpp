#include "src/backend/mir/aarch64/codegen/object_emission.hpp"
#include "src/backend/mir/aarch64/codegen/returns.hpp"
#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace {

namespace aarch64 = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace object = c4c::backend::mir::object;

constexpr std::uint32_t R_AARCH64_ADR_PREL_PG_HI21 = 275;
constexpr std::uint32_t R_AARCH64_ADD_ABS_LO12_NC = 277;
constexpr std::uint32_t R_AARCH64_CALL26 = 283;
constexpr std::uint16_t EM_AARCH64 = 183;
constexpr std::uint32_t SHT_RELA = 4;
constexpr std::uint32_t SHT_SYMTAB = 2;
constexpr std::uint32_t SHT_STRTAB = 3;
constexpr std::uint16_t SHN_UNDEF = 0;

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

bool has_bytes(const object::SectionRecord& section,
               std::initializer_list<std::uint8_t> expected) {
  return section.bytes == std::vector<std::uint8_t>(expected);
}

struct ElfSections {
  const std::vector<std::uint8_t>& bytes;
  std::size_t shoff = 0;
  std::size_t shentsize = 0;
  std::size_t shnum = 0;
  std::size_t shstr_offset = 0;

  [[nodiscard]] std::size_t header(std::size_t index) const {
    return shoff + index * shentsize;
  }

  [[nodiscard]] std::string name(std::size_t index) const {
    const auto header_offset = header(index);
    return read_c_string(bytes, shstr_offset + read_u32(bytes, header_offset));
  }

  [[nodiscard]] std::size_t find(std::string_view wanted) const {
    for (std::size_t index = 1; index < shnum; ++index) {
      if (name(index) == wanted) {
        return header(index);
      }
    }
    return 0;
  }
};

std::optional<ElfSections> inspect_elf_sections(
    const std::vector<std::uint8_t>& bytes) {
  if (bytes.size() < 64 || bytes[0] != 0x7f || bytes[1] != 'E' ||
      bytes[2] != 'L' || bytes[3] != 'F') {
    return std::nullopt;
  }
  ElfSections sections{
      .bytes = bytes,
      .shoff = static_cast<std::size_t>(read_u64(bytes, 40)),
      .shentsize = read_u16(bytes, 58),
      .shnum = read_u16(bytes, 60),
  };
  const auto shstrndx = read_u16(bytes, 62);
  if (sections.shoff == 0 || sections.shentsize != 64 ||
      sections.shnum == 0 || shstrndx >= sections.shnum) {
    return std::nullopt;
  }
  const auto shstr_header = sections.header(shstrndx);
  sections.shstr_offset =
      static_cast<std::size_t>(read_u64(bytes, shstr_header + 24));
  return sections;
}

aarch64_module::MachineInstruction machine_instruction(
    aarch64::InstructionRecord target) {
  return aarch64_module::MachineInstruction{.target = std::move(target)};
}

aarch64_module::MachineFunction machine_function(
    std::initializer_list<aarch64_module::MachineInstruction> instructions) {
  aarch64_module::MachineBlock block;
  block.index = 0;
  block.instructions = instructions;

  aarch64_module::MachineFunction function;
  function.blocks.push_back(std::move(block));
  return function;
}

aarch64::InstructionRecord machine_return() {
  return aarch64::make_return_instruction(aarch64::ReturnInstructionRecord{});
}

aarch64::InstructionRecord machine_direct_call(std::string_view callee) {
  return aarch64::make_call_instruction(aarch64::CallInstructionRecord{
      .direct_callee_label = callee,
  });
}

aarch64::InstructionRecord unsupported_machine_scalar() {
  return aarch64::make_unsupported_machine_instruction(
      aarch64::InstructionFamily::Scalar,
      aarch64::MachineNodeSelectionStatus::DeferredUnsupported,
      "unsupported for object emission");
}

int maps_aarch64_relocation_kinds_locally() {
  if (aarch64::aarch64_elf_relocation_type(
          aarch64::Aarch64ObjectFixupKind::Call26) != R_AARCH64_CALL26 ||
      aarch64::aarch64_elf_relocation_type(
          aarch64::Aarch64ObjectFixupKind::AdrPrelPgHi21) !=
          R_AARCH64_ADR_PREL_PG_HI21 ||
      aarch64::aarch64_elf_relocation_type(
          aarch64::Aarch64ObjectFixupKind::AddAbsLo12Nc) !=
          R_AARCH64_ADD_ABS_LO12_NC) {
    return fail("expected AArch64 fixup kinds to map to AArch64 ELF relocations");
  }
  const auto config = aarch64::aarch64_relocatable_elf_config();
  if (config.machine != EM_AARCH64 || config.flags != 0) {
    return fail("expected AArch64 relocatable ELF metadata");
  }
  return 0;
}

int records_undefined_direct_call_symbol() {
  const auto module = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64ObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              aarch64::make_aarch64_direct_call_fragment("callee"),
              aarch64::make_aarch64_return_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected AArch64 direct-call object module construction");
  }

  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->size_bytes != 8 ||
      !has_bytes(*text, {0x00, 0x00, 0x00, 0x94, 0xc0, 0x03, 0x5f, 0xd6}) ||
      !text->executable || text->writable) {
    return fail("expected .text to contain BL placeholder followed by RET");
  }

  const auto* caller = object::find_symbol(*module, "caller");
  const auto* callee = object::find_symbol(*module, "callee");
  if (caller == nullptr || caller->binding != object::SymbolBinding::Global ||
      caller->kind != object::SymbolKind::Function ||
      caller->section != std::optional<object::SectionId>{text->id} ||
      caller->value != 0 || caller->size_bytes != 8) {
    return fail("expected defined caller function symbol");
  }
  if (callee == nullptr || callee->binding != object::SymbolBinding::Global ||
      callee->kind != object::SymbolKind::Function ||
      !object::is_undefined_symbol(*callee)) {
    return fail("expected undefined direct-call target symbol");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_AARCH64_CALL26 ||
      module->relocations[0].symbol != callee->id ||
      module->relocations[0].addend != 0) {
    return fail("expected R_AARCH64_CALL26 relocation at BL placeholder");
  }
  return 0;
}

int records_same_module_direct_call_symbol() {
  const auto module = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64ObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {
              aarch64::make_aarch64_direct_call_fragment("callee"),
              aarch64::make_aarch64_return_fragment(),
          },
      },
      aarch64::Aarch64ObjectFunction{
          .name = "callee",
          .global = true,
          .fragments = {
              aarch64::make_aarch64_return_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected same-module AArch64 object module construction");
  }

  const auto* text = object::find_section(*module, ".text");
  const auto* callee = object::find_symbol(*module, "callee");
  if (text == nullptr || callee == nullptr ||
      callee->section != std::optional<object::SectionId>{text->id} ||
      callee->value != 8 || callee->size_bytes != 4 ||
      object::is_undefined_symbol(*callee)) {
    return fail("expected same-module call target to resolve as defined");
  }
  if (module->relocations.size() != 1 ||
      module->relocations[0].type != R_AARCH64_CALL26 ||
      module->relocations[0].symbol != callee->id) {
    return fail("expected same-module direct call relocation to target callee");
  }
  return 0;
}

int records_adrp_add_address_materialization_fixups() {
  const auto module = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64ObjectFunction{
          .name = "load_address",
          .global = true,
          .fragments = {
              aarch64::make_aarch64_address_materialization_fragment(
                  "global_value",
                  aarch64::Aarch64ObjectSymbolKind::Object),
              aarch64::make_aarch64_return_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected AArch64 address materialization object module");
  }

  const auto* text = object::find_section(*module, ".text");
  if (text == nullptr || text->size_bytes != 12 ||
      !has_bytes(*text,
                 {0x00, 0x00, 0x00, 0x90, 0x00, 0x00,
                  0x00, 0x91, 0xc0, 0x03, 0x5f, 0xd6})) {
    return fail("expected ADRP/ADD placeholders followed by RET");
  }

  const auto* symbol = object::find_symbol(*module, "global_value");
  if (symbol == nullptr || symbol->binding != object::SymbolBinding::Global ||
      symbol->kind != object::SymbolKind::Object ||
      !object::is_undefined_symbol(*symbol)) {
    return fail("expected address materialization target as undefined object symbol");
  }
  if (module->relocations.size() != 2 ||
      module->relocations[0].section != text->id ||
      module->relocations[0].offset != 0 ||
      module->relocations[0].type != R_AARCH64_ADR_PREL_PG_HI21 ||
      module->relocations[0].symbol != symbol->id ||
      module->relocations[1].section != text->id ||
      module->relocations[1].offset != 4 ||
      module->relocations[1].type != R_AARCH64_ADD_ABS_LO12_NC ||
      module->relocations[1].symbol != symbol->id) {
    return fail("expected ADRP/ADD relocation pair against one target symbol");
  }
  return 0;
}

int rejects_fixups_without_instruction_slots() {
  aarch64::Aarch64EncodedFragment fragment;
  fragment.bytes = {0x00, 0x00, 0x00, 0x94};
  fragment.fixups.push_back(aarch64::Aarch64ObjectFixup{
      .offset_bytes = fragment.bytes.size(),
      .kind = aarch64::Aarch64ObjectFixupKind::Call26,
      .symbol_name = "callee",
      .symbol_kind = aarch64::Aarch64ObjectSymbolKind::Function,
      .addend = 0,
  });

  const auto module = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64ObjectFunction{
          .name = "caller",
          .global = true,
          .fragments = {fragment},
      },
  });
  if (module.has_value()) {
    return fail("expected AArch64 fixup offsets to require instruction slots");
  }
  return 0;
}

int builds_return_only_machine_function_object() {
  const auto function = machine_function({machine_instruction(machine_return())});

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "return_only",
          .global = true,
          .function = &function,
      },
  });
  if (!result.ok()) {
    return fail("expected return-only machine function to emit object module");
  }

  const auto* text = object::find_section(*result.module, ".text");
  const auto* symbol = object::find_symbol(*result.module, "return_only");
  if (text == nullptr || text->size_bytes != 4 ||
      !has_bytes(*text, {0xc0, 0x03, 0x5f, 0xd6}) ||
      symbol == nullptr ||
      symbol->section != std::optional<object::SectionId>{text->id} ||
      symbol->value != 0 || symbol->size_bytes != 4 ||
      !result.module->relocations.empty()) {
    return fail("expected structured return machine record to emit one RET");
  }
  return 0;
}

int builds_same_module_machine_direct_call_object() {
  const auto caller = machine_function({
      machine_instruction(machine_direct_call("callee")),
      machine_instruction(machine_return()),
  });
  const auto callee = machine_function({machine_instruction(machine_return())});

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "caller",
          .global = true,
          .function = &caller,
      },
      aarch64::Aarch64MachineObjectFunction{
          .name = "callee",
          .global = true,
          .function = &callee,
      },
  });
  if (!result.ok()) {
    return fail("expected same-module machine direct call object module");
  }

  const auto* text = object::find_section(*result.module, ".text");
  const auto* callee_symbol = object::find_symbol(*result.module, "callee");
  if (text == nullptr || callee_symbol == nullptr ||
      callee_symbol->section != std::optional<object::SectionId>{text->id} ||
      callee_symbol->value != 8 || callee_symbol->size_bytes != 4 ||
      result.module->relocations.size() != 1 ||
      result.module->relocations[0].type != R_AARCH64_CALL26 ||
      result.module->relocations[0].offset != 0 ||
      result.module->relocations[0].symbol != callee_symbol->id) {
    return fail("expected structured same-module direct call relocation");
  }
  return 0;
}

int builds_external_machine_direct_call_object() {
  const auto caller = machine_function({
      machine_instruction(machine_direct_call("external_callee")),
      machine_instruction(machine_return()),
  });

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "caller",
          .global = true,
          .function = &caller,
      },
  });
  if (!result.ok()) {
    return fail("expected external machine direct call object module");
  }

  const auto* text = object::find_section(*result.module, ".text");
  const auto* callee = object::find_symbol(*result.module, "external_callee");
  if (text == nullptr || callee == nullptr || !object::is_undefined_symbol(*callee) ||
      result.module->relocations.size() != 1 ||
      result.module->relocations[0].section != text->id ||
      result.module->relocations[0].type != R_AARCH64_CALL26 ||
      result.module->relocations[0].symbol != callee->id) {
    return fail("expected external direct call to remain an undefined function relocation");
  }
  return 0;
}

int rejects_unsupported_machine_records_without_text_fallback() {
  const auto function = machine_function({
      machine_instruction(unsupported_machine_scalar()),
      machine_instruction(machine_return()),
  });

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "unsupported",
          .global = true,
          .function = &function,
      },
  });
  if (result.module.has_value() || result.diagnostics.size() != 1 ||
      result.diagnostics[0].kind !=
          aarch64::Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction ||
      result.diagnostics[0].function_name != "unsupported" ||
      result.diagnostics[0].block_index != 0 ||
      result.diagnostics[0].instruction_index != 0) {
    return fail("expected unsupported machine record to produce explicit diagnostic");
  }
  return 0;
}

int rejects_unselected_machine_records_without_text_fallback() {
  auto deferred_return = machine_return();
  deferred_return.selection.status =
      aarch64::MachineNodeSelectionStatus::DeferredUnsupported;
  deferred_return.selection.diagnostic = "deferred return";

  const auto function = machine_function({
      machine_instruction(deferred_return),
  });

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "deferred_return",
          .global = true,
          .function = &function,
      },
  });
  if (result.module.has_value() || result.diagnostics.size() != 1 ||
      result.diagnostics[0].kind !=
          aarch64::Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction ||
      result.diagnostics[0].function_name != "deferred_return") {
    return fail("expected unselected machine record to produce explicit diagnostic");
  }
  return 0;
}

int serializes_external_call_machine_object_as_aarch64_elf() {
  const auto caller = machine_function({
      machine_instruction(machine_direct_call("external_callee")),
      machine_instruction(machine_return()),
  });

  const auto result = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64MachineObjectFunction{
          .name = "caller",
          .global = true,
          .function = &caller,
      },
  });
  if (!result.ok()) {
    return fail("expected external-call machine records to produce object module");
  }

  const auto image = aarch64::write_aarch64_relocatable_elf_object(*result.module);
  if (!image.has_value()) {
    return fail("expected AArch64 ELF writer to serialize machine object module");
  }
  const auto& bytes = image->bytes;
  const auto sections = inspect_elf_sections(bytes);
  if (!sections.has_value() || bytes[4] != 2 || bytes[5] != 1 ||
      read_u16(bytes, 16) != 1 || read_u16(bytes, 18) != EM_AARCH64 ||
      read_u32(bytes, 48) != 0) {
    return fail("expected ELF64 little-endian relocatable AArch64 header");
  }

  const auto text_header = sections->find(".text");
  const auto rela_text_header = sections->find(".rela.text");
  const auto symtab_header = sections->find(".symtab");
  const auto strtab_header = sections->find(".strtab");
  const auto shstrtab_header = sections->find(".shstrtab");
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0 ||
      strtab_header == 0 || shstrtab_header == 0) {
    return fail("expected AArch64 ELF object section set");
  }
  if (read_u64(bytes, text_header + 32) != 8 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA ||
      read_u32(bytes, symtab_header + 4) != SHT_SYMTAB ||
      read_u32(bytes, strtab_header + 4) != SHT_STRTAB ||
      read_u32(bytes, shstrtab_header + 4) != SHT_STRTAB ||
      read_u32(bytes, rela_text_header + 40) !=
          static_cast<std::uint32_t>(
              (symtab_header - sections->shoff) / sections->shentsize) ||
      read_u32(bytes, rela_text_header + 44) !=
          static_cast<std::uint32_t>(
              (text_header - sections->shoff) / sections->shentsize)) {
    return fail("expected AArch64 text, rela, symtab, and string-table metadata");
  }

  const auto text_offset = static_cast<std::size_t>(read_u64(bytes, text_header + 24));
  if (text_offset + 8 > bytes.size() ||
      bytes[text_offset] != 0x00 || bytes[text_offset + 1] != 0x00 ||
      bytes[text_offset + 2] != 0x00 || bytes[text_offset + 3] != 0x94 ||
      bytes[text_offset + 4] != 0xc0 || bytes[text_offset + 5] != 0x03 ||
      bytes[text_offset + 6] != 0x5f || bytes[text_offset + 7] != 0xd6) {
    return fail("expected serialized AArch64 BL placeholder followed by RET");
  }

  const auto rela_offset =
      static_cast<std::size_t>(read_u64(bytes, rela_text_header + 24));
  const auto rela_size = read_u64(bytes, rela_text_header + 32);
  const auto rela_entsize = read_u64(bytes, rela_text_header + 56);
  if (rela_size != 24 || rela_entsize != 24 || read_u64(bytes, rela_offset) != 0) {
    return fail("expected one serialized AArch64 call relocation");
  }
  const auto r_info = read_u64(bytes, rela_offset + 8);
  if ((r_info & 0xffffffffull) != R_AARCH64_CALL26 ||
      read_u64(bytes, rela_offset + 16) != 0) {
    return fail("expected serialized R_AARCH64_CALL26 relocation encoding");
  }

  const auto symtab_offset =
      static_cast<std::size_t>(read_u64(bytes, symtab_header + 24));
  const auto strtab_offset =
      static_cast<std::size_t>(read_u64(bytes, strtab_header + 24));
  const auto symbol_name_at = [&](std::size_t symbol_index) {
    const auto symbol_offset = symtab_offset + symbol_index * 24;
    return read_c_string(bytes, strtab_offset + read_u32(bytes, symbol_offset));
  };
  const auto symbol_section_at = [&](std::size_t symbol_index) {
    const auto symbol_offset = symtab_offset + symbol_index * 24;
    return read_u16(bytes, symbol_offset + 6);
  };
  const auto symbol_value_at = [&](std::size_t symbol_index) {
    const auto symbol_offset = symtab_offset + symbol_index * 24;
    return read_u64(bytes, symbol_offset + 8);
  };
  const auto symbol_size_at = [&](std::size_t symbol_index) {
    const auto symbol_offset = symtab_offset + symbol_index * 24;
    return read_u64(bytes, symbol_offset + 16);
  };
  const auto relocated_symbol = static_cast<std::size_t>(r_info >> 32);
  if (symbol_name_at(relocated_symbol) != "external_callee" ||
      symbol_section_at(relocated_symbol) != SHN_UNDEF) {
    return fail("expected call relocation to reference undefined function symbol");
  }

  bool saw_caller = false;
  const auto symbol_count = read_u64(bytes, symtab_header + 32) / 24;
  for (std::size_t index = 1; index < symbol_count; ++index) {
    if (symbol_name_at(index) == "caller") {
      saw_caller = symbol_section_at(index) != SHN_UNDEF &&
                   symbol_value_at(index) == 0 && symbol_size_at(index) == 8;
    }
  }
  if (!saw_caller) {
    return fail("expected serialized caller function symbol");
  }
  return 0;
}

int serializes_adrp_add_aarch64_relocation_pair() {
  const auto module = aarch64::build_aarch64_text_object_module({
      aarch64::Aarch64ObjectFunction{
          .name = "load_address",
          .global = true,
          .fragments = {
              aarch64::make_aarch64_address_materialization_fragment(
                  "global_value",
                  aarch64::Aarch64ObjectSymbolKind::Object),
              aarch64::make_aarch64_return_fragment(),
          },
      },
  });
  if (!module.has_value()) {
    return fail("expected AArch64 ADRP/ADD object module");
  }
  const auto image = aarch64::write_aarch64_relocatable_elf_object(*module);
  if (!image.has_value()) {
    return fail("expected AArch64 ELF writer to serialize ADRP/ADD module");
  }
  const auto sections = inspect_elf_sections(image->bytes);
  if (!sections.has_value()) {
    return fail("expected ADRP/ADD ELF section headers");
  }
  const auto& bytes = image->bytes;
  const auto text_header = sections->find(".text");
  const auto rela_text_header = sections->find(".rela.text");
  const auto symtab_header = sections->find(".symtab");
  if (text_header == 0 || rela_text_header == 0 || symtab_header == 0 ||
      read_u64(bytes, text_header + 32) != 12 ||
      read_u32(bytes, rela_text_header + 4) != SHT_RELA) {
    return fail("expected ADRP/ADD ELF text and relocation sections");
  }

  const auto rela_offset =
      static_cast<std::size_t>(read_u64(bytes, rela_text_header + 24));
  if (read_u64(bytes, rela_text_header + 32) != 48 ||
      read_u64(bytes, rela_text_header + 56) != 24) {
    return fail("expected two serialized ADRP/ADD relocation entries");
  }
  const auto hi_info = read_u64(bytes, rela_offset + 8);
  const auto lo_info = read_u64(bytes, rela_offset + 32);
  if (read_u64(bytes, rela_offset) != 0 ||
      (hi_info & 0xffffffffull) != R_AARCH64_ADR_PREL_PG_HI21 ||
      read_u64(bytes, rela_offset + 24) != 4 ||
      (lo_info & 0xffffffffull) != R_AARCH64_ADD_ABS_LO12_NC ||
      (hi_info >> 32) != (lo_info >> 32)) {
    return fail("expected serialized AArch64 ADRP/ADD relocation pair");
  }

  const auto symtab_offset =
      static_cast<std::size_t>(read_u64(bytes, symtab_header + 24));
  const auto strtab_header =
      sections->shoff + read_u32(bytes, symtab_header + 40) * sections->shentsize;
  const auto strtab_offset =
      static_cast<std::size_t>(read_u64(bytes, strtab_header + 24));
  const auto symbol_index = static_cast<std::size_t>(hi_info >> 32);
  const auto symbol_offset = symtab_offset + symbol_index * 24;
  if (read_c_string(bytes,
                    strtab_offset + read_u32(bytes, symbol_offset)) !=
          "global_value" ||
      read_u16(bytes, symbol_offset + 6) != SHN_UNDEF) {
    return fail("expected ADRP/ADD relocations to reference undefined object symbol");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = maps_aarch64_relocation_kinds_locally(); status != 0) {
    return status;
  }
  if (const int status = records_undefined_direct_call_symbol(); status != 0) {
    return status;
  }
  if (const int status = records_same_module_direct_call_symbol(); status != 0) {
    return status;
  }
  if (const int status = records_adrp_add_address_materialization_fixups();
      status != 0) {
    return status;
  }
  if (const int status = rejects_fixups_without_instruction_slots(); status != 0) {
    return status;
  }
  if (const int status = builds_return_only_machine_function_object(); status != 0) {
    return status;
  }
  if (const int status = builds_same_module_machine_direct_call_object();
      status != 0) {
    return status;
  }
  if (const int status = builds_external_machine_direct_call_object();
      status != 0) {
    return status;
  }
  if (const int status = rejects_unsupported_machine_records_without_text_fallback();
      status != 0) {
    return status;
  }
  if (const int status = rejects_unselected_machine_records_without_text_fallback();
      status != 0) {
    return status;
  }
  if (const int status = serializes_external_call_machine_object_as_aarch64_elf();
      status != 0) {
    return status;
  }
  if (const int status = serializes_adrp_add_aarch64_relocation_pair();
      status != 0) {
    return status;
  }
  return 0;
}

#include "src/backend/mir/aarch64/codegen/object_emission.hpp"
#include "src/backend/mir/aarch64/codegen/returns.hpp"
#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
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

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bool has_bytes(const object::SectionRecord& section,
               std::initializer_list<std::uint8_t> expected) {
  return section.bytes == std::vector<std::uint8_t>(expected);
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
  return 0;
}

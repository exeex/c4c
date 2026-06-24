#include "src/backend/mir/aarch64/codegen/object_emission.hpp"
#include "src/backend/mir/object/model.hpp"

#include <cstdint>
#include <iostream>
#include <optional>
#include <vector>

namespace {

namespace aarch64 = c4c::backend::aarch64::codegen;
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
  return 0;
}

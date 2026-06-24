#include "object_emission.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace object = c4c::backend::mir::object;

constexpr std::uint16_t kElfMachineAarch64 = 183;
constexpr std::uint32_t kAarch64ElfFlagsNone = 0;
constexpr std::uint32_t kAarch64RelocAdrPrelPgHi21 = 275;
constexpr std::uint32_t kAarch64RelocAddAbsLo12Nc = 277;
constexpr std::uint32_t kAarch64RelocCall26 = 283;

void append_le32(std::vector<std::uint8_t>& bytes, std::uint32_t word) {
  bytes.push_back(static_cast<std::uint8_t>(word & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 8) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 16) & 0xffu));
  bytes.push_back(static_cast<std::uint8_t>((word >> 24) & 0xffu));
}

object::SymbolBinding binding_for_function(const Aarch64ObjectFunction& function) {
  return function.global ? object::SymbolBinding::Global
                         : object::SymbolBinding::Local;
}

object::SymbolKind object_symbol_kind(Aarch64ObjectSymbolKind kind) {
  switch (kind) {
    case Aarch64ObjectSymbolKind::Function:
      return object::SymbolKind::Function;
    case Aarch64ObjectSymbolKind::Object:
      return object::SymbolKind::Object;
  }
  return object::SymbolKind::NoType;
}

object::SymbolId get_or_declare_symbol(
    object::ObjectModule& module,
    std::unordered_map<std::string, object::SymbolId>& symbols_by_name,
    const Aarch64ObjectFixup& fixup) {
  const auto existing = symbols_by_name.find(fixup.symbol_name);
  if (existing != symbols_by_name.end()) {
    return existing->second;
  }
  auto& undefined = object::declare_undefined_symbol(
      module,
      fixup.symbol_name,
      object::SymbolBinding::Global,
      object_symbol_kind(fixup.symbol_kind));
  symbols_by_name.emplace(undefined.name, undefined.id);
  return undefined.id;
}

bool is_selected_machine_instruction(const InstructionRecord& instruction) {
  return instruction.surface == RecordSurfaceKind::MachineInstructionNode &&
         instruction.selection.status == MachineNodeSelectionStatus::Selected;
}

void add_diagnostic(std::vector<Aarch64ObjectEmissionDiagnostic>& diagnostics,
                    Aarch64ObjectEmissionDiagnosticKind kind,
                    std::string function_name,
                    std::size_t block_index,
                    std::size_t instruction_index,
                    std::string message) {
  diagnostics.push_back(Aarch64ObjectEmissionDiagnostic{
      .kind = kind,
      .function_name = std::move(function_name),
      .block_index = block_index,
      .instruction_index = instruction_index,
      .message = std::move(message),
  });
}

std::optional<Aarch64EncodedFragment> fragment_for_machine_instruction(
    const InstructionRecord& instruction,
    std::vector<Aarch64ObjectEmissionDiagnostic>& diagnostics,
    const std::string& function_name,
    std::size_t block_index,
    std::size_t instruction_index) {
  if (!is_selected_machine_instruction(instruction)) {
    add_diagnostic(diagnostics,
                   Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction,
                   function_name,
                   block_index,
                   instruction_index,
                   "AArch64 object emission supports only selected machine instructions");
    return std::nullopt;
  }

  if (std::holds_alternative<ReturnInstructionRecord>(instruction.payload)) {
    return make_aarch64_return_fragment();
  }

  if (const auto* call = std::get_if<CallInstructionRecord>(&instruction.payload)) {
    if (instruction.opcode == MachineOpcode::DirectCall && !call->is_indirect &&
        !call->direct_callee_label.empty()) {
      return make_aarch64_direct_call_fragment(
          std::string{call->direct_callee_label});
    }
    add_diagnostic(diagnostics,
                   Aarch64ObjectEmissionDiagnosticKind::UnsupportedCall,
                   function_name,
                   block_index,
                   instruction_index,
                   "AArch64 object emission supports only selected direct calls");
    return std::nullopt;
  }

  add_diagnostic(diagnostics,
                 Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction,
                 function_name,
                 block_index,
                 instruction_index,
                 "unsupported AArch64 machine instruction for object emission");
  return std::nullopt;
}

std::optional<Aarch64ObjectFunction> object_function_from_machine(
    const Aarch64MachineObjectFunction& input,
    std::vector<Aarch64ObjectEmissionDiagnostic>& diagnostics) {
  if (input.function == nullptr) {
    add_diagnostic(diagnostics,
                   Aarch64ObjectEmissionDiagnosticKind::MissingFunction,
                   input.name,
                   0,
                   0,
                   "missing AArch64 machine function");
    return std::nullopt;
  }
  if (input.name.empty()) {
    add_diagnostic(diagnostics,
                   Aarch64ObjectEmissionDiagnosticKind::MissingFunctionName,
                   input.name,
                   0,
                   0,
                   "missing AArch64 object function name");
    return std::nullopt;
  }

  Aarch64ObjectFunction object_function{
      .name = input.name,
      .global = input.global,
  };
  for (const auto& block : input.function->blocks) {
    for (std::size_t index = 0; index < block.instructions.size(); ++index) {
      const auto fragment = fragment_for_machine_instruction(
          block.instructions[index].target,
          diagnostics,
          input.name,
          block.index,
          index);
      if (!fragment.has_value()) {
        return std::nullopt;
      }
      object_function.fragments.push_back(*fragment);
    }
  }
  return object_function;
}

}  // namespace

std::optional<std::uint32_t> aarch64_elf_relocation_type(
    Aarch64ObjectFixupKind kind) {
  switch (kind) {
    case Aarch64ObjectFixupKind::Call26:
      return kAarch64RelocCall26;
    case Aarch64ObjectFixupKind::AdrPrelPgHi21:
      return kAarch64RelocAdrPrelPgHi21;
    case Aarch64ObjectFixupKind::AddAbsLo12Nc:
      return kAarch64RelocAddAbsLo12Nc;
  }
  return std::nullopt;
}

object::RelocatableElfConfig aarch64_relocatable_elf_config() {
  return object::RelocatableElfConfig{
      .elf_class = object::ElfClass::Elf64,
      .data_encoding = object::ElfDataEncoding::LittleEndian,
      .machine = kElfMachineAarch64,
      .flags = kAarch64ElfFlagsNone,
  };
}

Aarch64EncodedFragment make_aarch64_return_fragment() {
  Aarch64EncodedFragment fragment;
  append_le32(fragment.bytes, 0xd65f03c0u);  // ret
  return fragment;
}

Aarch64EncodedFragment make_aarch64_direct_call_fragment(std::string callee_name) {
  Aarch64EncodedFragment fragment;
  append_le32(fragment.bytes, 0x94000000u);  // bl imm26, relocation-filled
  fragment.fixups.push_back(Aarch64ObjectFixup{
      .offset_bytes = 0,
      .kind = Aarch64ObjectFixupKind::Call26,
      .symbol_name = std::move(callee_name),
      .symbol_kind = Aarch64ObjectSymbolKind::Function,
      .addend = 0,
  });
  return fragment;
}

Aarch64EncodedFragment make_aarch64_address_materialization_fragment(
    std::string symbol_name, Aarch64ObjectSymbolKind symbol_kind) {
  Aarch64EncodedFragment fragment;
  append_le32(fragment.bytes, 0x90000000u);  // adrp x0, symbol@page
  append_le32(fragment.bytes, 0x91000000u);  // add x0, x0, symbol@pageoff
  fragment.fixups.push_back(Aarch64ObjectFixup{
      .offset_bytes = 0,
      .kind = Aarch64ObjectFixupKind::AdrPrelPgHi21,
      .symbol_name = symbol_name,
      .symbol_kind = symbol_kind,
      .addend = 0,
  });
  fragment.fixups.push_back(Aarch64ObjectFixup{
      .offset_bytes = 4,
      .kind = Aarch64ObjectFixupKind::AddAbsLo12Nc,
      .symbol_name = std::move(symbol_name),
      .symbol_kind = symbol_kind,
      .addend = 0,
  });
  return fragment;
}

std::optional<object::ObjectModule> build_aarch64_text_object_module(
    const std::vector<Aarch64ObjectFunction>& functions) {
  object::ObjectModule module;
  auto& text = object::create_section(module,
                                      ".text",
                                      object::SectionKind::Text,
                                      4,
                                      true,
                                      true,
                                      false);

  std::unordered_map<std::string, object::SymbolId> symbols_by_name;
  std::unordered_set<std::string> defined_function_names;
  for (const auto& function : functions) {
    if (function.name.empty() ||
        !defined_function_names.insert(function.name).second) {
      return std::nullopt;
    }
  }

  for (const auto& function : functions) {
    const auto start_offset = text.size_bytes;
    for (const auto& fragment : function.fragments) {
      const auto fragment_offset = object::append_section_bytes(text, fragment.bytes);
      for (const auto& fixup : fragment.fixups) {
        const auto reloc_type = aarch64_elf_relocation_type(fixup.kind);
        if (!reloc_type.has_value() || fixup.symbol_name.empty() ||
            fixup.offset_bytes >= fragment.bytes.size() ||
            (fixup.offset_bytes % 4) != 0) {
          return std::nullopt;
        }
        const auto target_symbol = get_or_declare_symbol(
            module,
            symbols_by_name,
            fixup);
        object::attach_relocation(module,
                                  text.id,
                                  fragment_offset + fixup.offset_bytes,
                                  *reloc_type,
                                  target_symbol,
                                  fixup.addend);
      }
    }
    auto& symbol = object::define_symbol(module,
                                         function.name,
                                         binding_for_function(function),
                                         object::SymbolKind::Function,
                                         text.id,
                                         start_offset,
                                         text.size_bytes - start_offset);
    symbols_by_name[symbol.name] = symbol.id;
  }

  return module;
}

Aarch64ObjectEmissionResult build_aarch64_text_object_module(
    const std::vector<Aarch64MachineObjectFunction>& functions) {
  Aarch64ObjectEmissionResult result;
  std::vector<Aarch64ObjectFunction> object_functions;
  object_functions.reserve(functions.size());

  for (const auto& function : functions) {
    auto object_function =
        object_function_from_machine(function, result.diagnostics);
    if (!object_function.has_value()) {
      return result;
    }
    object_functions.push_back(std::move(*object_function));
  }

  result.module = build_aarch64_text_object_module(object_functions);
  if (!result.module.has_value() && result.diagnostics.empty()) {
    add_diagnostic(result.diagnostics,
                   Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction,
                   "",
                   0,
                   0,
                   "AArch64 object module construction failed");
  }
  return result;
}

std::optional<object::RelocatableElfImage> write_aarch64_relocatable_elf_object(
    const object::ObjectModule& module) {
  return object::write_relocatable_elf(module, aarch64_relocatable_elf_config());
}

}  // namespace c4c::backend::aarch64::codegen

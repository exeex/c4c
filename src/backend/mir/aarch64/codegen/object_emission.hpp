#pragma once

#include "mir/aarch64/module/module.hpp"
#include "mir/object/elf_writer.hpp"
#include "mir/object/model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

enum class Aarch64ObjectFixupKind {
  Call26,
  AdrPrelPgHi21,
  AddAbsLo12Nc,
};

enum class Aarch64ObjectSymbolKind {
  Function,
  Object,
};

struct Aarch64ObjectFixup {
  std::uint64_t offset_bytes = 0;
  Aarch64ObjectFixupKind kind = Aarch64ObjectFixupKind::Call26;
  std::string symbol_name;
  Aarch64ObjectSymbolKind symbol_kind = Aarch64ObjectSymbolKind::Function;
  std::int64_t addend = 0;
};

struct Aarch64EncodedFragment {
  std::vector<std::uint8_t> bytes;
  std::vector<Aarch64ObjectFixup> fixups;
};

struct Aarch64ObjectFunction {
  std::string name;
  bool global = true;
  std::vector<Aarch64EncodedFragment> fragments;
};

enum class Aarch64ObjectEmissionDiagnosticKind {
  MissingFunction,
  MissingFunctionName,
  UnsupportedInstruction,
  UnsupportedCall,
};

struct Aarch64ObjectEmissionDiagnostic {
  Aarch64ObjectEmissionDiagnosticKind kind =
      Aarch64ObjectEmissionDiagnosticKind::UnsupportedInstruction;
  std::string function_name;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::string message;
};

struct Aarch64MachineObjectFunction {
  std::string name;
  bool global = true;
  const c4c::backend::aarch64::module::MachineFunction* function = nullptr;
};

struct Aarch64ObjectEmissionResult {
  std::optional<c4c::backend::mir::object::ObjectModule> module;
  std::vector<Aarch64ObjectEmissionDiagnostic> diagnostics;

  [[nodiscard]] bool ok() const {
    return module.has_value() && diagnostics.empty();
  }
};

[[nodiscard]] std::optional<std::uint32_t> aarch64_elf_relocation_type(
    Aarch64ObjectFixupKind kind);

[[nodiscard]] c4c::backend::mir::object::RelocatableElfConfig
aarch64_relocatable_elf_config();

[[nodiscard]] Aarch64EncodedFragment make_aarch64_return_fragment();

[[nodiscard]] Aarch64EncodedFragment make_aarch64_direct_call_fragment(
    std::string callee_name);

[[nodiscard]] Aarch64EncodedFragment make_aarch64_address_materialization_fragment(
    std::string symbol_name, Aarch64ObjectSymbolKind symbol_kind);

[[nodiscard]] std::optional<c4c::backend::mir::object::ObjectModule>
build_aarch64_text_object_module(
    const std::vector<Aarch64ObjectFunction>& functions);

[[nodiscard]] Aarch64ObjectEmissionResult build_aarch64_text_object_module(
    const std::vector<Aarch64MachineObjectFunction>& functions);

[[nodiscard]] std::optional<c4c::backend::mir::object::RelocatableElfImage>
write_aarch64_relocatable_elf_object(
    const c4c::backend::mir::object::ObjectModule& module);

}  // namespace c4c::backend::aarch64::codegen

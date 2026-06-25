#pragma once

#include "mir/object/elf_writer.hpp"
#include "mir/object/model.hpp"
#include "../../../prealloc/module.hpp"
#include "../../../prealloc/prepared_object_traversal.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::riscv::codegen {

enum class RiscvObjectFixupKind {
  CallPlt,
  PcrelHi20,
  PcrelLo12I,
  Branch,
  Jal,
};

struct RiscvObjectFixup {
  std::uint64_t offset_bytes = 0;
  RiscvObjectFixupKind kind = RiscvObjectFixupKind::CallPlt;
  std::string symbol_name;
  std::int64_t addend = 0;
};

struct RiscvObjectLabel {
  std::uint64_t offset_bytes = 0;
  std::string name;
};

struct RiscvEncodedFragment {
  std::vector<std::uint8_t> bytes;
  std::vector<RiscvObjectLabel> labels;
  std::vector<RiscvObjectFixup> fixups;
};

struct RiscvObjectFunction {
  std::string name;
  bool global = true;
  std::vector<RiscvEncodedFragment> fragments;
};

struct RiscvPreparedObjectModuleResult {
  std::optional<c4c::backend::mir::object::ObjectModule> module;
  std::optional<c4c::backend::prepare::PreparedObjectConsumerDiagnosticCategory>
      prepared_consumer_category;
  std::string diagnostic;

  [[nodiscard]] bool ok() const {
    return module.has_value() && !prepared_consumer_category.has_value() &&
           diagnostic.empty();
  }
};

struct RiscvPreparedObjectImageResult {
  std::optional<c4c::backend::mir::object::RelocatableElfImage> image;
  std::optional<c4c::backend::prepare::PreparedObjectConsumerDiagnosticCategory>
      prepared_consumer_category;
  std::string diagnostic;

  [[nodiscard]] bool ok() const {
    return image.has_value() && !prepared_consumer_category.has_value() &&
           diagnostic.empty();
  }
};

enum class RiscvInsnDInlineAsmRegisterBank {
  Gpr,
  Vector,
};

struct RiscvInsnDInlineAsmRegister {
  RiscvInsnDInlineAsmRegisterBank bank = RiscvInsnDInlineAsmRegisterBank::Gpr;
  std::uint32_t physical_index = 0;
  std::size_t group_width = 1;
};

struct RiscvInsnDInlineAsmShape {
  std::int64_t major = 0;
  std::int64_t operation = 0;
  RiscvInsnDInlineAsmRegister destination;
  RiscvInsnDInlineAsmRegister lhs;
  RiscvInsnDInlineAsmRegister rhs;
  RiscvInsnDInlineAsmRegister accumulator;
  std::int64_t dtype = 0;
};

[[nodiscard]] std::optional<std::uint32_t> rv64_elf_relocation_type(
    RiscvObjectFixupKind kind);

[[nodiscard]] c4c::backend::mir::object::RelocatableElfConfig
rv64_relocatable_elf_config();

[[nodiscard]] RiscvEncodedFragment make_rv64_return_zero_fragment();

[[nodiscard]] RiscvEncodedFragment make_rv64_direct_call_fragment(
    std::string callee_name);

[[nodiscard]] RiscvEncodedFragment make_rv64_pcrel_address_fragment(
    std::string symbol_name, std::string auipc_label_name);

[[nodiscard]] std::optional<c4c::backend::mir::object::ObjectModule>
build_rv64_text_object_module(const std::vector<RiscvObjectFunction>& functions);

[[nodiscard]] std::optional<c4c::backend::mir::object::ObjectModule>
build_rv64_prepared_text_object_module(
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] RiscvPreparedObjectModuleResult
build_rv64_prepared_text_object_module_with_diagnostics(
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] std::optional<c4c::backend::mir::object::RelocatableElfImage>
write_rv64_relocatable_elf_object(
    const c4c::backend::mir::object::ObjectModule& module);

[[nodiscard]] RiscvPreparedObjectImageResult
write_rv64_prepared_relocatable_elf_object_with_diagnostics(
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] std::optional<c4c::backend::mir::object::RelocatableElfImage>
write_rv64_prepared_relocatable_elf_object(
    const c4c::backend::prepare::PreparedBirModule& prepared);

[[nodiscard]] std::optional<std::string> substitute_prepared_riscv_inline_asm_operands(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier);

[[nodiscard]] std::optional<RiscvInsnDInlineAsmShape>
classify_prepared_rv64_insn_d_inline_asm(
    const c4c::backend::prepare::PreparedInlineAsmCarrier& carrier);

[[nodiscard]] std::optional<std::uint64_t> encode_rv64_ev_insn_d_inline_asm(
    const RiscvInsnDInlineAsmShape& shape);

}  // namespace c4c::backend::riscv::codegen

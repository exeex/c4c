#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <optional>

namespace c4c::backend::aarch64::codegen {

enum class ReturnValuePrintForm {
  NoValue,
  PrimaryReturn,
  ImmediateMaterialization,
  Unsupported,
};

[[nodiscard]] InstructionRecord make_return_instruction(ReturnInstructionRecord instruction);
[[nodiscard]] ReturnValuePrintForm classify_return_value_print_form(
    const ReturnInstructionRecord& instruction);
[[nodiscard]] bool is_printable_return_immediate_materialization_value(
    const ImmediateOperand& immediate);
[[nodiscard]] std::optional<abi::RegisterReference> return_immediate_materialization_register(
    bir::TypeKind type);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_return_terminator(const module::BlockLoweringContext& context,
                                 const BlockScalarLoweringState& scalar_state,
                                 module::ModuleLoweringDiagnostics& diagnostics);

}  // namespace c4c::backend::aarch64::codegen

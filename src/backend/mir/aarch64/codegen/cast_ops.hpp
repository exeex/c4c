#pragma once

#include "alu.hpp"
#include "instruction.hpp"
#include "../module/module.hpp"
#include "mir/printer.hpp"

#include <string_view>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] bool is_simple_integer_cast_opcode(bir::CastOpcode opcode);
[[nodiscard]] bool is_supported_scalar_conversion_cast_opcode(bir::CastOpcode opcode);
[[nodiscard]] ScalarCastOperationKind scalar_cast_operation_from_cast_opcode(
    bir::CastOpcode opcode);
[[nodiscard]] MachineOpcode machine_opcode_from_scalar_cast(
    ScalarCastOperationKind operation);
[[nodiscard]] ScalarInstructionRecord make_scalar_cast_instruction_record(
    ScalarCastRecord cast);
[[nodiscard]] PreparedScalarCastRecordResult make_prepared_scalar_cast_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast);
[[nodiscard]] PreparedScalarCastInstructionRecordResult
make_prepared_scalar_cast_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast);
[[nodiscard]] std::optional<module::MachineInstruction> lower_scalar_cast_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);
[[nodiscard]] mir::TargetInstructionPrintResult print_scalar_cast_instruction(
    const InstructionRecord& instruction,
    const ScalarInstructionRecord& scalar,
    std::string_view diagnostic_prefix);

}  // namespace c4c::backend::aarch64::codegen

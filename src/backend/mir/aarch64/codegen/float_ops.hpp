#pragma once

#include "alu.hpp"

#include <optional>
#include <string>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_register_view(
    bir::TypeKind type);
[[nodiscard]] bool is_prepared_scalar_float_alu_operation(
    const bir::BinaryInst& binary);
[[nodiscard]] ScalarAluPrintResult make_scalar_float_alu_print_lines(
    const ScalarInstructionRecord& scalar);
[[nodiscard]] PreparedScalarAluRecordResult make_prepared_scalar_float_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_prepared_scalar_float_alu_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen

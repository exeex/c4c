#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::optional<module::MachineInstruction> lower_intrinsic_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] bool has_prepared_intrinsic_carrier(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] InstructionRecord make_scalar_fp_unary_intrinsic_instruction(
    ScalarFpUnaryIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_crc32w_intrinsic_instruction(
    Crc32WIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_vector_load_intrinsic_instruction(
    VectorLoadIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_vector_add_intrinsic_instruction(
    VectorAddIntrinsicRecord instruction);

[[nodiscard]] PreparedScalarFpUnaryIntrinsicInstructionRecordResult
make_prepared_scalar_fp_unary_intrinsic_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier);

[[nodiscard]] mir::TargetInstructionPrintResult print_intrinsic_instruction(
    const InstructionRecord& instruction);

}  // namespace c4c::backend::aarch64::codegen

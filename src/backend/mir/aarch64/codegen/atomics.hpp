#pragma once

#include "instruction.hpp"
#include "../module/module.hpp"

#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string_view prepared_atomic_operation_record_error_name(
    PreparedAtomicOperationRecordError error);
[[nodiscard]] std::string_view atomic_memory_instruction_kind_name(
    AtomicMemoryInstructionKind kind);

[[nodiscard]] InstructionRecord make_atomic_memory_instruction(
    AtomicMemoryInstructionRecord instruction);

[[nodiscard]] std::vector<module::MachineInstruction> lower_atomic_memory_operations_for_block(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics);

[[nodiscard]] PreparedAtomicOperationInstructionRecordResult
make_prepared_atomic_operation_instruction_record(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAtomicOperationCarrier& operation);

}  // namespace c4c::backend::aarch64::codegen

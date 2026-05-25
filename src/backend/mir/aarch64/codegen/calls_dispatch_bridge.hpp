#pragma once

#include "../module/module.hpp"
#include "alu.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    const std::vector<bir::Value>& arguments,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics);

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_call_boundary_source_to_destination(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_indirect_call_callee_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

void record_call_result_source_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::vector<module::MachineInstruction>
materialize_missing_frame_slot_call_arguments(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen

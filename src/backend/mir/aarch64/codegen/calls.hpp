#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "alu.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

// Call-boundary move API

[[nodiscard]] std::vector<module::MachineInstruction> lower_before_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_after_call_moves(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] bool call_boundary_move_reloads_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses);
[[nodiscard]] std::vector<module::MachineInstruction>
order_before_call_moves_for_source_preservation(
    std::vector<module::MachineInstruction> moves);
[[nodiscard]] std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);

// Dispatch-to-calls API

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

void retarget_call_boundary_source_to_emitted_scalar(
    module::MachineInstruction& instruction,
    const BlockScalarLoweringState& scalar_state);

void record_call_boundary_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state);

void record_call_boundary_source_in_destination(
    const module::MachineInstruction& instruction,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool call_boundary_move_reloads_prepared_stack_source(
    const module::MachineInstruction& instruction);

[[nodiscard]] bool source_register_conflicts_with_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses);

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

void retarget_fpr_call_result_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction);

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

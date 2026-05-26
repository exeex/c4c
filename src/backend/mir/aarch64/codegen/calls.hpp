#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::size_t outgoing_stack_argument_bytes(
    const prepare::PreparedCallPlan& call_plan);
[[nodiscard]] abi::RegisterReference outgoing_stack_argument_base_register();

[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes);

// calls_moves / calls_dispatch_bridge

// Shared selection-driven call argument source conversion. Source choice is
// prepared by call plans; this helper only translates complete prepared facts
// into AArch64 operands.
[[nodiscard]] std::optional<MemoryOperand> make_selected_call_argument_source(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallArgumentPlan& argument,
    const prepare::PreparedValueHome* source_home,
    const prepare::PreparedCallArgumentSourceSelection& selection,
    prepare::PreparedCallArgumentSourceSelectionKind expected_kind,
    std::size_t instruction_index);

[[nodiscard]] const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::optional<module::MachineInstruction> lower_prepared_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan,
    const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes,
    std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_helper,
    module::ModuleLoweringDiagnostics& diagnostics);

// calls_moves

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

// calls_moves

[[nodiscard]] const prepare::PreparedMoveBundle* find_move_bundle(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);

}  // namespace c4c::backend::aarch64::codegen

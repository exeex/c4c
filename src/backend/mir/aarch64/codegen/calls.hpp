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

// Target operand adapters

// Target-only operand adapters. These translate prepared register, view,
// immediate, and F128 facts into AArch64 operands; they do not choose semantic
// call-argument sources.

[[nodiscard]] bool complete_full_width_f128_carrier(
    const prepare::PreparedF128Carrier* carrier);
[[nodiscard]] bool complete_f128_constant_carrier(
    const prepare::PreparedF128Carrier* carrier);
[[nodiscard]] const prepare::PreparedF128Carrier*
find_prepared_f128_carrier_in_module(const prepare::PreparedBirModule& prepared,
                                     prepare::PreparedValueId value_id);
[[nodiscard]] std::optional<abi::RegisterView> scalar_fp_view_from_register_name(
    const std::optional<std::string>& register_name);
[[nodiscard]] std::optional<abi::RegisterView> scalar_view_from_register_name(
    const std::optional<std::string>& register_name);
[[nodiscard]] std::size_t scalar_size_from_register_view(
    std::optional<abi::RegisterView> view);
[[nodiscard]] std::optional<std::string> register_name_with_expected_view(
    const std::optional<std::string>& register_name,
    std::optional<abi::RegisterView> expected_view);
[[nodiscard]] std::optional<RegisterOperand> make_register_operand_from_prepared_authority(
    const std::optional<std::string>& register_name,
    const std::optional<prepare::PreparedRegisterPlacement>& placement,
    const std::optional<prepare::PreparedRegisterBank>& bank,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    std::size_t contiguous_width,
    const std::vector<std::string>& occupied_registers,
    std::optional<abi::RegisterView> expected_view,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<RegisterOperand>
make_f128_q_register_operand_from_carrier(
    const prepare::PreparedF128Carrier& carrier,
    RegisterOperandRole role,
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name,
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] std::optional<ImmediateOperand> make_scalar_call_argument_immediate(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id);
[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view(
    bir::TypeKind type);
[[nodiscard]] std::optional<abi::RegisterView> scalar_integer_register_view_from_size(
    std::size_t size_bytes);
[[nodiscard]] std::optional<bir::TypeKind> scalar_integer_type_from_size(
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

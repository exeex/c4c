#pragma once

#include "../module/module.hpp"
#include "instruction.hpp"
#include "mir/printer.hpp"

#include <cstddef>
#include <optional>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::optional<MachineEffectResource> effect_from_prepared_call_clobber(
    const prepare::PreparedClobberedRegister& clobber);
[[nodiscard]] std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers);
[[nodiscard]] std::optional<MachineEffectResource>
effect_from_prepared_call_preserved_value(
    const prepare::PreparedCallPreservedValue& preserved);
[[nodiscard]] std::vector<MachineEffectResource>
effects_from_prepared_call_preserved_values(
    const std::vector<prepare::PreparedCallPreservedValue>& preserved_values);
[[nodiscard]] const prepare::PreparedCallPlan* find_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);
[[nodiscard]] const prepare::PreparedCallPlan* require_prepared_call_plan(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
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
[[nodiscard]] std::vector<module::MachineInstruction> lower_before_return_moves(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics);
[[nodiscard]] std::vector<module::MachineInstruction> lower_value_moves(
    const module::BlockLoweringContext& context,
    prepare::PreparedMovePhase phase,
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
[[nodiscard]] InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);
[[nodiscard]] mir::TargetInstructionPrintResult print_call(
    const InstructionRecord& instruction,
    const CallInstructionRecord& call);
[[nodiscard]] mir::TargetInstructionPrintResult print_call_boundary_move(
    const InstructionRecord& instruction,
    const CallBoundaryMoveInstructionRecord& move);
[[nodiscard]] mir::TargetInstructionPrintResult print_call_boundary_abi_binding(
    const InstructionRecord& instruction,
    const CallBoundaryAbiBindingInstructionRecord& binding);

}  // namespace c4c::backend::aarch64::codegen

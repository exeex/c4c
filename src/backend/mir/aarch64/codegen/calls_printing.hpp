#pragma once

#include "calls.hpp"

namespace c4c::backend::aarch64::codegen {

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

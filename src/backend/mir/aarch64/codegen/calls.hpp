#pragma once

#include "instruction.hpp"

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
[[nodiscard]] InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);

}  // namespace c4c::backend::aarch64::codegen

#pragma once

#include "instruction.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] std::string_view machine_effect_resource_kind_name(
    MachineEffectResourceKind kind);
[[nodiscard]] std::string_view machine_side_effect_kind_name(MachineSideEffectKind kind);

[[nodiscard]] std::string_view register_display_name(abi::RegisterReference reg);
[[nodiscard]] std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg);
[[nodiscard]] std::vector<std::string_view> occupied_register_views(
    const std::vector<abi::RegisterReference>& regs);
[[nodiscard]] std::optional<abi::RegisterView> prepared_clobber_expected_view(
    prepare::PreparedRegisterBank bank);

[[nodiscard]] MachineEffectResource machine_effect_from_operand(
    const OperandRecord& operand);
[[nodiscard]] MachineEffectResource machine_prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name);
[[nodiscard]] std::vector<MachineEffectResource> machine_effects_from_operands(
    const std::vector<OperandRecord>& operands);
[[nodiscard]] std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers);
[[nodiscard]] std::vector<MachineEffectResource> effects_from_prepared_call_preserved_values(
    const std::vector<prepare::PreparedCallPreservedValue>& preserved_values);

}  // namespace c4c::backend::aarch64::codegen

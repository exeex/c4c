#pragma once

#include "calls.hpp"

#include <optional>
#include <vector>

namespace c4c::backend::aarch64::codegen {

[[nodiscard]] MachineEffectResource effect_from_operand(const OperandRecord& operand);
[[nodiscard]] MachineEffectResource prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name);
[[nodiscard]] std::vector<MachineEffectResource> effects_from_operands(
    const std::vector<OperandRecord>& operands);

}  // namespace c4c::backend::aarch64::codegen

#pragma once

#include "alu.hpp"

#include <optional>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;

[[nodiscard]] std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value);
[[nodiscard]] bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state);

}  // namespace c4c::backend::aarch64::codegen

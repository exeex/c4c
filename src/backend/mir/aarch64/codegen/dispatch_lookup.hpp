#pragma once

#include "alu.hpp"

#include <cstddef>
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
[[nodiscard]] bool is_scalar_call_argument_producer_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] std::optional<std::size_t> find_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] bool has_same_block_load_local_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::aarch64::codegen

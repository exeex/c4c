#pragma once

#include "alu.hpp"
#include "../module/module.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace c4c::backend::aarch64::codegen {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value);
[[nodiscard]] const prepare::PreparedValueHome* find_value_home(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id);
[[nodiscard]] std::optional<prepare::PreparedValueId> prepared_value_id(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);
[[nodiscard]] const prepare::PreparedValueHome* find_value_home(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name);
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
    std::string_view value_name,
    std::size_t before_instruction_index);
[[nodiscard]] bool has_same_block_load_local_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::aarch64::codegen

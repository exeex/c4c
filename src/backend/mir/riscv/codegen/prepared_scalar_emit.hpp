#pragma once

#include "../../../bir/bir.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

struct SimpleCompare {
  c4c::backend::bir::BinaryOpcode opcode = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::Value lhs;
  c4c::backend::bir::Value rhs;
};

[[nodiscard]] std::optional<std::int64_t> simple_integer_immediate(
    const c4c::backend::bir::Value& value);

[[nodiscard]] bool emit_move_to_register(
    std::string& out,
    std::string_view destination_register,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_compare_branch(
    const SimpleCompare& compare,
    std::string_view true_label,
    std::string_view false_label);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_cast(
    const c4c::backend::bir::CastInst& cast,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_select(
    const c4c::backend::bir::SelectInst& select,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_prepared_pointer_add(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_binary(
    const c4c::backend::bir::BinaryInst& binary,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::optional<std::size_t> return_address_stack_offset,
    std::size_t stack_frame_bytes);

}  // namespace c4c::backend::riscv::codegen

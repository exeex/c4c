#pragma once

#include "object_emission.hpp"
#include "../../../bir/bir.hpp"
#include "../../../prealloc/names.hpp"
#include "../../../prealloc/prepared_lookups.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

namespace c4c::backend::riscv::codegen {

struct PreparedCurrentInstructionContext;

struct PreparedBeforeReturnStackToRegisterKey {
  std::size_t block_index = 0;
  c4c::backend::prepare::PreparedValueId value_id = 0;

  bool operator==(const PreparedBeforeReturnStackToRegisterKey& other) const {
    return block_index == other.block_index && value_id == other.value_id;
  }
};

struct PreparedBeforeReturnStackToRegisterKeyHash {
  std::size_t operator()(
      const PreparedBeforeReturnStackToRegisterKey& key) const {
    return (std::hash<std::size_t>{}(key.block_index) << 1) ^
           std::hash<c4c::backend::prepare::PreparedValueId>{}(key.value_id);
  }
};

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


[[nodiscard]] bool append_rv64_move_value_to_register(
    RiscvEncodedFragment& fragment,
    std::uint32_t destination,
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Value& value,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_binary(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_cast(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::CastInst& cast,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_compare_branch(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    c4c::backend::bir::BinaryOpcode opcode,
    const c4c::backend::bir::Value& lhs,
    const c4c::backend::bir::Value& rhs,
    std::string true_label,
    std::string false_label,
    std::size_t stack_frame_bytes);

[[nodiscard]] bool prepared_compare_feeds_supported_scalar_trunc_publication(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment>
fragment_for_prepared_scalar_compare_trunc_source(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::bir::Block& block,
    std::size_t instruction_index,
    const c4c::backend::bir::BinaryInst& binary,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<RiscvEncodedFragment> fragment_for_prepared_return(
    const c4c::backend::prepare::PreparedStackLayout& stack_layout,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    const c4c::backend::prepare::PreparedFramePlanFunction* frame_plan,
    const c4c::backend::bir::Terminator& terminator,
    std::size_t block_index,
    std::size_t terminator_instruction_index,
    const std::unordered_set<PreparedBeforeReturnStackToRegisterKey,
                             PreparedBeforeReturnStackToRegisterKeyHash>*
        prepared_before_return_stack_to_register_values,
    bool restore_return_address,
    std::size_t stack_frame_bytes);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_compare_branch(
    const SimpleCompare& compare,
    std::string_view true_label,
    std::string_view false_label);

[[nodiscard]] std::optional<std::string> emit_riscv_prepared_fused_compare_branch(
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::string_view true_label,
    std::string_view false_label);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_cast(
    const c4c::backend::bir::CastInst& cast,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_select(
    const c4c::backend::bir::SelectInst& select,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    std::string_view function_name,
    const PreparedCurrentInstructionContext& context,
    const c4c::backend::bir::Block* block);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_prepared_pointer_add(
    const c4c::backend::bir::BinaryInst& binary,
    const PreparedCurrentInstructionContext& context);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_binary(
    const c4c::backend::bir::BinaryInst& binary,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups);

[[nodiscard]] std::optional<std::string> emit_riscv_simple_return(
    const c4c::backend::bir::Terminator& terminator,
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedFunctionLookups* lookups,
    std::optional<std::size_t> return_address_stack_offset,
    std::size_t stack_frame_bytes);

}  // namespace c4c::backend::riscv::codegen

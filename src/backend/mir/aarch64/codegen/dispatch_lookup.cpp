#include "dispatch_lookup.hpp"

#include "../abi/abi.hpp"

#include <algorithm>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace abi = c4c::backend::aarch64::abi;

bool is_scalar_call_argument_producer_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::SRem:
      return true;
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

std::optional<c4c::ValueNameId> prepared_named_value_id(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.prepared == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return std::nullopt;
  }
  return prepare::resolve_prepared_value_name_id(context.function.prepared->names,
                                                 value.name);
}

std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = prepare::find_indexed_prepared_value_home(
      context.function.value_home_lookups,
      context.function.regalloc,
      context.function.value_locations,
      *value_name);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(parsed->index, *expected_view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *viewed,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
}

bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value() &&
         find_emitted_scalar_register(scalar_state, *value_name).has_value();
}

std::optional<std::size_t> find_same_block_scalar_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto* binary =
        std::get_if<bir::BinaryInst>(&context.bir_block->insts[candidate_index]);
    if (binary == nullptr) {
      continue;
    }
    if (binary->result.kind == bir::Value::Kind::Named &&
        binary->result.name == value_name &&
        is_scalar_call_argument_producer_opcode(binary->opcode)) {
      return candidate_index;
    }
  }
  return std::nullopt;
}

bool has_same_block_load_local_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return false;
  }
  const std::size_t limit =
      std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto* load =
        std::get_if<bir::LoadLocalInst>(&context.bir_block->insts[index - 1]);
    if (load != nullptr &&
        load->result.kind == bir::Value::Kind::Named &&
        load->result.name == value.name) {
      return true;
    }
  }
  return false;
}

}  // namespace c4c::backend::aarch64::codegen

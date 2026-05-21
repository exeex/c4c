#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "f128.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "operands.hpp"
#include "returns.hpp"
#include "variadic.hpp"
#include "../../../prealloc/target_register_profile.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace abi = c4c::backend::aarch64::abi;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

constexpr std::size_t kStackPointerAlignmentBytes = 16;

[[nodiscard]] std::size_t align_to(std::size_t value, std::size_t alignment) {
  if (alignment == 0) {
    return value;
  }
  const std::size_t remainder = value % alignment;
  return remainder == 0 ? value : value + (alignment - remainder);
}

[[nodiscard]] bool registers_alias(const RegisterOperand& lhs,
                                   const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}

void append_block_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                             module::ModuleLoweringDiagnosticKind kind,
                             const module::BlockLoweringContext& context,
                             std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string unsupported_terminator_message(
    c4c::backend::bir::TerminatorKind kind) {
  switch (kind) {
    case c4c::backend::bir::TerminatorKind::Return:
      return "AArch64 block dispatch visited unsupported prepared return terminator";
    case c4c::backend::bir::TerminatorKind::Branch:
      return "AArch64 block dispatch visited prepared branch terminator; semantic lowering is not implemented";
    case c4c::backend::bir::TerminatorKind::CondBranch:
      return "AArch64 block dispatch visited prepared conditional branch terminator; semantic lowering is not implemented";
  }
  return "AArch64 block dispatch visited unsupported prepared terminator";
}

[[nodiscard]] const bir::Block* find_bir_block(
    const module::FunctionLoweringContext& function,
    const prepare::PreparedControlFlowBlock& block) {
  if (function.bir_function == nullptr) {
    return nullptr;
  }

  if (function.prepared == nullptr || block.block_label == c4c::kInvalidBlockLabel) {
    return nullptr;
  }

  const std::string_view prepared_block_label =
      prepare::prepared_block_label(function.prepared->names, block.block_label);
  if (prepared_block_label.empty()) {
    return nullptr;
  }

  for (const auto& bir_block : function.bir_function->blocks) {
    if (bir_block.label_id != c4c::kInvalidBlockLabel &&
        function.prepared->module.names.block_labels.spelling(bir_block.label_id) ==
            prepared_block_label) {
      return &bir_block;
    }
    if (std::string_view{bir_block.label} == prepared_block_label) {
      return &bir_block;
    }
  }
  return nullptr;
}

[[nodiscard]] bool binary_result_matches_value(const bir::Inst& inst,
                                               std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  return binary != nullptr &&
         binary->result.kind == bir::Value::Kind::Named &&
         binary->result.name == value_name;
}

[[nodiscard]] bool binary_uses_named_value(const bir::Inst& inst,
                                           std::string_view value_name) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr) {
    return false;
  }
  const auto matches = [&](const bir::Value& value) {
    return value.kind == bir::Value::Kind::Named && value.name == value_name;
  };
  return matches(binary->lhs) || matches(binary->rhs);
}

[[nodiscard]] std::optional<unsigned> integer_bit_width(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return 1U;
    case bir::TypeKind::I8:
      return 8U;
    case bir::TypeKind::I16:
      return 16U;
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<unsigned> power_of_two_shift(std::uint64_t value) {
  if (value == 0 || (value & (value - 1U)) != 0) {
    return std::nullopt;
  }
  unsigned shift = 0;
  while (value > 1U) {
    value >>= 1U;
    ++shift;
  }
  return shift;
}

[[nodiscard]] std::optional<std::string_view> branch_condition_suffix(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Eq:
      return std::string_view{"eq"};
    case bir::BinaryOpcode::Ne:
      return std::string_view{"ne"};
    case bir::BinaryOpcode::Slt:
      return std::string_view{"lt"};
    case bir::BinaryOpcode::Sle:
      return std::string_view{"le"};
    case bir::BinaryOpcode::Sgt:
      return std::string_view{"gt"};
    case bir::BinaryOpcode::Sge:
      return std::string_view{"ge"};
    case bir::BinaryOpcode::Ult:
      return std::string_view{"lo"};
    case bir::BinaryOpcode::Ule:
      return std::string_view{"ls"};
    case bir::BinaryOpcode::Ugt:
      return std::string_view{"hi"};
    case bir::BinaryOpcode::Uge:
      return std::string_view{"hs"};
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::string machine_block_label(c4c::FunctionNameId function_name,
                                              c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
}

[[nodiscard]] std::optional<c4c::ValueNameId> prepared_named_value_id(
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

[[nodiscard]] std::optional<RegisterOperand> make_named_prepared_result_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home =
      prepare::find_prepared_value_home(*context.function.value_locations, *value_name);
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

[[nodiscard]] bool emitted_scalar_value_available(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value() &&
         find_emitted_scalar_register(scalar_state, *value_name).has_value();
}

[[nodiscard]] bool is_scalar_call_argument_producer_opcode(bir::BinaryOpcode opcode) {
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

[[nodiscard]] std::optional<std::size_t> find_same_block_scalar_producer(
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

[[nodiscard]] bool has_same_block_load_local_producer(
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

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_direct_global_select_chain_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty() ||
      emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  const auto producer_index =
      find_same_block_scalar_producer(context, value.name, before_instruction_index);
  if (!producer_index.has_value() || context.bir_block == nullptr) {
    if (has_same_block_load_local_producer(context, value, before_instruction_index)) {
      return true;
    }
    if (auto prepared_register = make_named_prepared_result_register(context, value);
        prepared_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     prepared_register->value_name,
                                     *prepared_register);
    }
    return true;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(&context.bir_block->insts[*producer_index]);
  if (binary == nullptr ||
      !is_scalar_call_argument_producer_opcode(binary->opcode)) {
    return true;
  }

  active_values.push_back(value.name);
  const bool lhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->lhs,
                                             *producer_index,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
                                             binary->rhs,
                                             *producer_index,
                                             scalar_state,
                                             diagnostics,
                                             lowered,
                                             active_values);
  active_values.pop_back();
  if (!lhs_ready || !rhs_ready) {
    return false;
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
  }
  if (auto instruction = lower_scalar_instruction(context,
                                                  context.bir_block->insts[*producer_index],
                                                  *producer_index,
                                                  scalar_state,
                                                  diagnostics)) {
    const auto expected_result =
        make_named_prepared_result_register(context, binary->result);
    if (expected_result.has_value()) {
      if (auto* scalar =
              std::get_if<ScalarInstructionRecord>(&instruction->target.payload)) {
        scalar->result_register = *expected_result;
        if (scalar->scalar_alu.has_value()) {
          scalar->scalar_alu->result_register = *expected_result;
          if (const auto lhs_name = prepared_named_value_id(context, binary->lhs);
              lhs_name.has_value()) {
            if (const auto lhs_register =
                find_emitted_scalar_register(scalar_state, *lhs_name);
                lhs_register.has_value()) {
              auto lhs_operand = make_register_operand(*lhs_register);
              scalar->scalar_alu->lhs = lhs_operand;
              if (!scalar->inputs.empty()) {
                scalar->inputs[0] = std::move(lhs_operand);
              }
            }
          }
          if (const auto rhs_name = prepared_named_value_id(context, binary->rhs);
              rhs_name.has_value()) {
            if (const auto rhs_register =
                    find_emitted_scalar_register(scalar_state, *rhs_name);
                rhs_register.has_value()) {
              auto rhs_operand = make_register_operand(*rhs_register);
              scalar->scalar_alu->rhs = rhs_operand;
              if (scalar->inputs.size() > 1) {
                scalar->inputs[1] = std::move(rhs_operand);
              }
            }
          }
        }
        record_emitted_scalar_register(scalar_state,
                                       expected_result->value_name,
                                       *expected_result);
      }
    }
    lowered.push_back(std::move(*instruction));
    return true;
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_scalar_call_argument_producers(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  for (const auto& argument : call.args) {
    if (auto select_chain =
            materialize_direct_global_select_chain_call_argument(context,
                                                                 argument,
                                                                 instruction_index,
                                                                 scalar_state)) {
      lowered.push_back(std::move(*select_chain));
      continue;
    }
    std::vector<std::string_view> active_values;
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}

[[nodiscard]] bool prepared_edge_select_source_is_destination_register(
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home) {
  return source_home.kind == prepare::PreparedValueHomeKind::Register &&
         destination_home.kind == prepare::PreparedValueHomeKind::Register &&
         source_home.register_name.has_value() &&
         destination_home.register_name.has_value() &&
         *source_home.register_name == *destination_home.register_name;
}

[[nodiscard]] std::optional<bir::Value> instruction_result_value(const bir::Inst& inst);

[[nodiscard]] bool is_current_block_join_parallel_copy_source(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto result_value = instruction_result_value(inst);
  if (!result_value.has_value() ||
      result_value->kind != bir::Value::Kind::Named ||
      result_value->name.empty()) {
    return false;
  }
  const auto result_value_name = prepared_named_value_id(context, *result_value);
  if (!result_value_name.has_value()) {
    return false;
  }
  const auto* result_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      if (move.to_value_id == result_home->value_id) {
        return true;
      }
      if (move.source_immediate_i32.has_value() ||
          move.from_value_id != result_home->value_id ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* destination_home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            move.to_value_id);
      if (destination_home != nullptr &&
          (prepared_edge_select_source_is_destination_register(*result_home,
                                                              *destination_home) ||
           result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::vector<std::string_view> named_operands_of_instruction(
    const bir::Inst& inst) {
  std::vector<std::string_view> operands;
  auto append_named = [&operands](const bir::Value& value) {
    if (value.kind == bir::Value::Kind::Named && !value.name.empty()) {
      operands.push_back(value.name);
    }
  };
  std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          append_named(typed_inst.lhs);
          append_named(typed_inst.rhs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          append_named(typed_inst.operand);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          append_named(typed_inst.lhs);
          append_named(typed_inst.rhs);
          append_named(typed_inst.true_value);
          append_named(typed_inst.false_value);
        }
      },
      inst);
  return operands;
}

[[nodiscard]] bool is_join_parallel_copy_expression_instruction(
    const bir::Inst& inst) {
  return std::get_if<bir::BinaryInst>(&inst) != nullptr ||
         std::get_if<bir::CastInst>(&inst) != nullptr ||
         std::get_if<bir::SelectInst>(&inst) != nullptr;
}

[[nodiscard]] std::optional<std::size_t> find_same_block_result_index(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || value_name.empty()) {
    return std::nullopt;
  }
  const auto limit =
      std::min(before_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto result = instruction_result_value(context.bir_block->insts[index - 1]);
    if (result.has_value() && result->kind == bir::Value::Kind::Named &&
        result->name == value_name) {
      return index - 1;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool same_block_result_depends_on_value(
    const module::BlockLoweringContext& context,
    std::string_view source_name,
    std::string_view dependency_name,
    std::size_t before_instruction_index,
    std::size_t depth = 0) {
  if (source_name.empty() || dependency_name.empty() || depth > 16U) {
    return false;
  }
  if (source_name == dependency_name) {
    return true;
  }
  const auto producer_index =
      find_same_block_result_index(context, source_name, before_instruction_index);
  if (!producer_index.has_value()) {
    return false;
  }
  const auto& producer = context.bir_block->insts[*producer_index];
  if (!is_join_parallel_copy_expression_instruction(producer)) {
    return false;
  }
  for (const auto operand_name : named_operands_of_instruction(producer)) {
    if (operand_name == dependency_name ||
        same_block_result_depends_on_value(context,
                                           operand_name,
                                           dependency_name,
                                           *producer_index,
                                           depth + 1U)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool is_current_block_join_parallel_copy_incoming_expression(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      !is_join_parallel_copy_expression_instruction(inst)) {
    return false;
  }
  const auto result_value = instruction_result_value(inst);
  if (!result_value.has_value() ||
      result_value->kind != bir::Value::Kind::Named ||
      result_value->name.empty()) {
    return false;
  }
  const auto result_value_name = prepared_named_value_id(context, *result_value);
  if (!result_value_name.has_value()) {
    return false;
  }
  const auto* result_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *result_value_name);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.source_immediate_i32.has_value()) {
        continue;
      }
      const auto* source_home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            move.from_value_id);
      if (source_home == nullptr ||
          source_home->value_name == c4c::kInvalidValueName) {
        continue;
      }
      if (source_home->value_id == result_home->value_id) {
        return true;
      }
      const auto source_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       source_home->value_name);
      if (!source_name.empty() &&
          same_block_result_depends_on_value(context,
                                             source_name,
                                             result_value->name,
                                             context.bir_block->insts.size())) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_predecessor_join_source_publication(
    const module::BlockLoweringContext& context,
    const bir::Block& successor,
    std::size_t source_index,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home,
    BlockScalarLoweringState& scalar_state);

[[nodiscard]] std::optional<bir::Value> instruction_result_value(const bir::Inst& inst);

[[nodiscard]] std::vector<module::MachineInstruction>
lower_predecessor_select_parallel_copy_sources(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    return lowered;
  }

  const auto* bundle = prepare::find_prepared_move_bundle(
      *context.function.value_locations,
      prepare::PreparedMovePhase::BlockEntry,
      context.block_index,
      0);
  if (bundle == nullptr ||
      bundle->authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      bundle->source_parallel_copy_predecessor_label !=
          std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} ||
      !bundle->source_parallel_copy_successor_label.has_value()) {
    return lowered;
  }

  const auto successor_label = prepare::prepared_block_label(
      context.function.prepared->names,
      *bundle->source_parallel_copy_successor_label);
  if (successor_label.empty() ||
      successor_label != context.bir_block->terminator.target_label) {
    return lowered;
  }
  const auto* successor =
      prepare::find_block_in_function(*context.function.bir_function, successor_label);
  if (successor == nullptr) {
    return lowered;
  }

  for (const auto& move : bundle->moves) {
    if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
        move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.source_immediate_i32.has_value() ||
        move.from_value_id == move.to_value_id) {
      continue;
    }
    const auto* source_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.from_value_id);
    const auto* destination_home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          move.to_value_id);
    if (source_home == nullptr ||
        destination_home == nullptr ||
        source_home->value_name == c4c::kInvalidValueName ||
        destination_home->kind != prepare::PreparedValueHomeKind::Register) {
      continue;
    }
    const auto source_name =
        prepare::prepared_value_name(context.function.prepared->names, source_home->value_name);
    if (source_name.empty()) {
      continue;
    }
    if (!prepared_edge_select_source_is_destination_register(*source_home,
                                                            *destination_home) &&
        source_home->kind != prepare::PreparedValueHomeKind::StackSlot) {
      continue;
    }
    for (std::size_t source_index = 0; source_index < successor->insts.size(); ++source_index) {
      if (!binary_result_matches_value(successor->insts[source_index], source_name)) {
        continue;
      }
      auto source_lowered = lower_predecessor_join_source_publication(
          context,
          *successor,
          source_index,
          *source_home,
          *destination_home,
          scalar_state);
      if (!source_lowered.has_value()) {
        lowered.clear();
        return lowered;
      }
      lowered.push_back(std::move(*source_lowered));
      return lowered;
    }
  }
  return lowered;
}

[[nodiscard]] module::MachineInstruction make_branch_compare_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::vector<std::string> lines);

[[nodiscard]] const bir::CastInst* find_same_block_cast_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  for (const auto& inst : context.bir_block->insts) {
    const auto* cast = std::get_if<bir::CastInst>(&inst);
    if (cast != nullptr &&
        cast->result.kind == bir::Value::Kind::Named &&
        cast->result.name == value.name) {
      return cast;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> emitted_register_name(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state,
    abi::RegisterView view) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value() || !abi::is_gp_register(emitted->reg)) {
    return std::nullopt;
  }
  const auto viewed = abi::gp_register(emitted->reg.index, view);
  if (!viewed.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*viewed);
}

[[nodiscard]] std::optional<std::string> compare_operand_name(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state,
    abi::RegisterView view) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return "#" + std::to_string(value.immediate);
  }
  return emitted_register_name(context, value, scalar_state, view);
}

struct SameBlockLoadProducer {
  const bir::LoadLocalInst* load = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] std::optional<module::MachineInstruction>
make_select_chain_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines);

[[nodiscard]] SameBlockLoadProducer find_same_block_load_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    const auto& inst = context.bir_block->insts[index];
    const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
    if (load != nullptr &&
        load->result.kind == bir::Value::Kind::Named &&
        load->result.name == value.name) {
      return SameBlockLoadProducer{.load = load, .instruction_index = index};
    }
  }
  return {};
}

[[nodiscard]] const bir::BinaryInst* find_same_block_binary_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return nullptr;
  }
  for (const auto& inst : context.bir_block->insts) {
    const auto* binary = std::get_if<bir::BinaryInst>(&inst);
    if (binary != nullptr &&
        binary->result.kind == bir::Value::Kind::Named &&
        binary->result.name == value.name) {
      return binary;
    }
  }
  return nullptr;
}

struct SameBlockSelectProducer {
  const bir::SelectInst* select = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr ||
      value.kind != bir::Value::Kind::Named ||
      value.name.empty()) {
    return {};
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const std::size_t candidate_index = index - 1;
    const auto* select =
        std::get_if<bir::SelectInst>(&context.bir_block->insts[candidate_index]);
    if (select != nullptr &&
        select->result.kind == bir::Value::Kind::Named &&
        select->result.name == value.name) {
      return SameBlockSelectProducer{.select = select,
                                     .instruction_index = candidate_index};
    }
  }
  return {};
}

[[nodiscard]] std::optional<std::int64_t> evaluate_same_block_integer_constant(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    unsigned depth = 0) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return value.immediate;
  }
  if (depth > 4U) {
    return std::nullopt;
  }
  const auto* binary = find_same_block_binary_producer(context, value);
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto lhs = evaluate_same_block_integer_constant(context, binary->lhs, depth + 1);
  const auto rhs = evaluate_same_block_integer_constant(context, binary->rhs, depth + 1);
  if (!lhs.has_value() || !rhs.has_value()) {
    return std::nullopt;
  }
  switch (binary->opcode) {
    case bir::BinaryOpcode::SDiv:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return *lhs / *rhs;
    case bir::BinaryOpcode::UDiv:
      if (*rhs == 0) {
        return std::nullopt;
      }
      return static_cast<std::int64_t>(
          static_cast<std::uint64_t>(*lhs) / static_cast<std::uint64_t>(*rhs));
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
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
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool is_cmp_immediate_encodable(std::int64_t value) {
  return value >= 0 && value <= 4095;
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer);

[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_memory_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index);

[[nodiscard]] bool select_chain_contains_direct_global_load(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (producer == nullptr) {
    return false;
  }
  if (std::get_if<bir::LoadGlobalInst>(producer) != nullptr) {
    return true;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  const auto nested_before = producer_index.value_or(before_instruction_index);
  if (const auto* select = std::get_if<bir::SelectInst>(producer);
      select != nullptr) {
    return select_chain_contains_direct_global_load(
               context, select->true_value, nested_before, depth + 1) ||
           select_chain_contains_direct_global_load(
               context, select->false_value, nested_before, depth + 1);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    return select_chain_contains_direct_global_load(
        context, cast->operand, nested_before, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer);
      binary != nullptr) {
    return select_chain_contains_direct_global_load(
               context, binary->lhs, nested_before, depth + 1) ||
           select_chain_contains_direct_global_load(
               context, binary->rhs, nested_before, depth + 1);
  }
  return false;
}

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  for (const auto& slot : stack_layout.frame_slots) {
    if (slot.slot_id == slot_id) {
      return &slot;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedStackObject* find_stack_object(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedObjectId object_id) {
  for (const auto& object : stack_layout.objects) {
    if (object.object_id == object_id) {
      return &object;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::string> prepared_frame_slot_load_address(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout, *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  const auto offset =
      static_cast<std::int64_t>(slot->offset_bytes) + access->address.byte_offset;
  std::string address = "[sp";
  if (offset != 0) {
    address += ", #";
    address += std::to_string(offset);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fused_compare_branch_from_emitted_cast(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch ||
      !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value()) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(*branch_condition->predicate);
  if (!condition.has_value()) {
    return std::nullopt;
  }

  const bir::Value* cast_value = &*branch_condition->lhs;
  const bir::Value* other_value = &*branch_condition->rhs;
  const bir::CastInst* cast = find_same_block_cast_producer(context, *cast_value);
  if (cast == nullptr) {
    cast_value = &*branch_condition->rhs;
    other_value = &*branch_condition->lhs;
    cast = find_same_block_cast_producer(context, *cast_value);
  }
  if (cast == nullptr ||
      cast->opcode != bir::CastOpcode::SExt ||
      cast->operand.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto source_bits = integer_bit_width(cast->operand.type);
  const auto result_bits = integer_bit_width(cast->result.type);
  const auto result_view = scalar_register_view(cast->result.type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      !result_view.has_value() || *source_bits >= *result_bits) {
    return std::nullopt;
  }
  auto source_name =
      emitted_register_name(context, cast->operand, scalar_state, abi::RegisterView::W);
  auto rhs_name = compare_operand_name(context, *other_value, scalar_state, *result_view);
  if (!rhs_name.has_value()) {
    const auto constant = evaluate_same_block_integer_constant(context, *other_value);
    if (constant.has_value() && is_cmp_immediate_encodable(*constant)) {
      rhs_name = "#" + std::to_string(*constant);
    }
  }
  std::vector<std::string> lines;
  if (!source_name.has_value()) {
    const auto load_producer = find_same_block_load_producer(context, cast->operand);
    const auto load_address =
        load_producer.load != nullptr
            ? prepared_frame_slot_load_address(context, load_producer.instruction_index)
            : std::optional<std::string>{};
    if (!load_producer.load ||
        load_producer.load->result.type != bir::TypeKind::I8 ||
        !load_address.has_value()) {
      return std::nullopt;
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.size() < 2U) {
      return std::nullopt;
    }
    source_name = abi::register_name(abi::w_register(scratches[1].index));
    lines.push_back("ldrb " + *source_name + ", " + *load_address);
  }
  if (!rhs_name.has_value()) {
    return std::nullopt;
  }

  std::string scratch_name;
  if (auto result_register = make_named_prepared_result_register(context, cast->result);
      result_register.has_value()) {
    const auto scratch = abi::gp_register(result_register->reg.index, *result_view);
    if (scratch.has_value() && abi::register_name(*scratch) != *rhs_name) {
      scratch_name = abi::register_name(*scratch);
    }
  }
  if (scratch_name.empty()) {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    for (const auto& scratch_reg : scratches) {
      const auto scratch = abi::gp_register(scratch_reg.index, *result_view);
      if (!scratch.has_value()) {
        continue;
      }
      const auto candidate = abi::register_name(*scratch);
      if (candidate != *rhs_name) {
        scratch_name = candidate;
        break;
      }
    }
    if (scratch_name.empty()) {
      return std::nullopt;
    }
  }

  if (*source_bits == 8U) {
    lines.push_back("sxtb " + scratch_name + ", " + *source_name);
  } else if (*source_bits == 16U) {
    lines.push_back("sxth " + scratch_name + ", " + *source_name);
  } else if (*source_bits == 32U && *result_bits == 64U) {
    lines.push_back("sxtw " + scratch_name + ", " + *source_name);
  } else {
    return std::nullopt;
  }
  lines.push_back("cmp " + scratch_name + ", " + *rhs_name);
  lines.push_back("b." + std::string{*condition} + " " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type);

[[nodiscard]] bool emit_value_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    bool reload_current_memory_loads = false);
[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);
[[nodiscard]] bool emit_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::size_t root_instruction_index,
    c4c::ValueNameId root_value_name,
    std::vector<std::string>& lines,
    std::size_t& label_index,
    std::vector<std::string_view>& active_values,
    bool reload_current_memory_loads = false);

[[nodiscard]] std::optional<RegisterOperand> current_block_entry_publication_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    abi::RegisterView expected_view);

[[nodiscard]] const bir::Global* find_load_global_target(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load);

[[nodiscard]] std::string load_global_symbol_label(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load,
    const bir::Global* target_global);

[[nodiscard]] std::optional<module::MachineInstruction>
lower_materialized_compare_condition_branch(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      branch_condition->condition_value != context.bir_block->terminator.condition) {
    return std::nullopt;
  }
  const auto condition_name =
      prepared_named_value_id(context, branch_condition->condition_value);
  const auto* condition_home =
      condition_name.has_value() && context.function.value_locations != nullptr
          ? prepare::find_prepared_value_home(*context.function.value_locations,
                                              *condition_name)
          : nullptr;
  if (condition_home == nullptr) {
    return std::nullopt;
  }
  const auto* producer =
      find_same_block_named_producer(context,
                                     branch_condition->condition_value.name,
                                     context.bir_block->insts.size());
  const auto producer_index = producer_instruction_index(context, producer);
  const auto* binary =
      producer != nullptr ? std::get_if<bir::BinaryInst>(producer) : nullptr;
  if (binary == nullptr || !producer_index.has_value()) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(binary->opcode);
  const auto operand_view = scalar_view_for_type(binary->operand_type);
  if (!condition.has_value() || !operand_view.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto lhs_reg = abi::gp_register(scratches[0].index, *operand_view);
  const auto rhs_reg = abi::gp_register(scratches[1].index, *operand_view);
  const auto lhs_scratch_name =
      lhs_reg.has_value() ? std::optional<std::string>{abi::register_name(*lhs_reg)}
                          : std::nullopt;
  const auto rhs_scratch_name =
      rhs_reg.has_value() ? std::optional<std::string>{abi::register_name(*rhs_reg)}
                          : std::nullopt;
  if (!lhs_scratch_name.has_value() || !rhs_scratch_name.has_value() ||
      branch_condition->true_label == c4c::kInvalidBlockLabel ||
      branch_condition->false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  std::optional<std::uint8_t> lhs_register_index;
  std::optional<RegisterOperand> lhs_publication;
  auto lhs_name = emitted_register_name(context, lhs, scalar_state, *operand_view);
  if (!lhs_name.has_value()) {
    if (auto published =
            current_block_entry_publication_register(context, lhs, *operand_view)) {
      lhs_publication = *published;
      lhs_name = std::string{abi::register_name(published->reg)};
      lhs_register_index = published->reg.index;
    }
  }
  if (lhs_name.has_value()) {
    if (!lhs_publication.has_value()) {
      const auto value_name = prepared_named_value_id(context, lhs);
      if (!value_name.has_value()) {
        return std::nullopt;
      }
      const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
      if (!emitted.has_value() ||
          emitted->reg.bank != abi::RegisterBank::GeneralPurpose) {
        return std::nullopt;
      }
      lhs_register_index = emitted->reg.index;
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            lhs,
                                            *producer_index,
                                            scratches[0].index,
                                            scratches[1].index,
                                            lines)) {
      return std::nullopt;
    }
    lhs_name = *lhs_scratch_name;
  }

  std::string rhs;
  if (binary->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(binary->rhs.immediate)) {
    rhs = "#" + std::to_string(binary->rhs.immediate);
  } else {
    auto rhs_value = binary->rhs;
    rhs_value.type = binary->operand_type;
    if (auto emitted_rhs =
            emitted_register_name(context, rhs_value, scalar_state, *operand_view);
        emitted_rhs.has_value()) {
      rhs = *emitted_rhs;
    } else {
      const auto rhs_target_index =
          lhs_register_index == std::optional<std::uint8_t>{scratches[1].index}
              ? scratches[0].index
              : scratches[1].index;
      const auto rhs_scratch_index =
          rhs_target_index == scratches[0].index ? scratches[1].index
                                                 : scratches[0].index;
      if (lhs_register_index ==
          std::optional<std::uint8_t>{rhs_scratch_index}) {
        return std::nullopt;
      }
      if (!emit_value_publication_to_register(context,
                                              rhs_value,
                                              *producer_index,
                                              rhs_target_index,
                                              rhs_scratch_index,
                                              lines)) {
        return std::nullopt;
      }
      const auto rhs_reg = abi::gp_register(rhs_target_index, *operand_view);
      if (!rhs_reg.has_value()) {
        return std::nullopt;
      }
      rhs = abi::register_name(*rhs_reg);
    }
  }

  lines.push_back("cmp " + *lhs_name + ", " + rhs);
  lines.push_back("b." + std::string{*condition} + " " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_current_block_entry_fused_compare_branch(
    const module::BlockLoweringContext& context) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch ||
      !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value()) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(*branch_condition->predicate);
  const auto operand_view = scalar_view_for_type(*branch_condition->compare_type);
  if (!condition.has_value() || !operand_view.has_value() ||
      branch_condition->true_label == c4c::kInvalidBlockLabel ||
      branch_condition->false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  auto lhs = *branch_condition->lhs;
  lhs.type = *branch_condition->compare_type;
  const auto published_lhs =
      current_block_entry_publication_register(context, lhs, *operand_view);
  if (!published_lhs.has_value() ||
      published_lhs->reg.bank != abi::RegisterBank::GeneralPurpose ||
      branch_condition->rhs->kind != bir::Value::Kind::Immediate ||
      !is_cmp_immediate_encodable(branch_condition->rhs->immediate)) {
    return std::nullopt;
  }
  const auto lhs_reg = abi::gp_register(published_lhs->reg.index, *operand_view);
  if (!lhs_reg.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_reg)} + ", #" +
                  std::to_string(branch_condition->rhs->immediate));
  lines.push_back("b." + std::string{*condition} + " " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_conditional_branch_from_emitted_condition(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->condition_value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto condition_name =
      prepared_named_value_id(context, branch_condition->condition_value);
  if (!condition_name.has_value()) {
    return std::nullopt;
  }
  auto condition_register =
      find_emitted_scalar_register(scalar_state, *condition_name);
  std::vector<std::string> lines;
  if (!condition_register.has_value()) {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.size() < 2U) {
      return std::nullopt;
    }
    const auto expected_view =
        scalar_view_for_type(branch_condition->condition_value.type)
            .value_or(abi::RegisterView::W);
    if (!emit_value_publication_to_register(context,
                                            branch_condition->condition_value,
                                            context.bir_block != nullptr
                                                ? context.bir_block->insts.size()
                                                : 0U,
                                            scratches[0].index,
                                            scratches[1].index,
                                            lines)) {
      return std::nullopt;
    }
    auto reg = abi::gp_register(scratches[0].index, expected_view);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    const auto* home =
        context.function.value_locations != nullptr
            ? prepare::find_prepared_value_home(*context.function.value_locations,
                                                *condition_name)
            : nullptr;
    condition_register = RegisterOperand{
        .reg = *reg,
        .role = RegisterOperandRole::ReservedMirScratch,
        .value_id = home != nullptr ? std::optional<prepare::PreparedValueId>{home->value_id}
                                    : std::nullopt,
        .value_name = *condition_name,
        .expected_view = expected_view,
    };
    record_emitted_scalar_register(scalar_state, *condition_name, *condition_register);
  }
  condition_register->reg.view =
      condition_register->expected_view.value_or(abi::RegisterView::W);
  const auto condition = std::string{abi::register_name(condition_register->reg)};
  if (condition.empty() ||
      branch_condition->true_label == c4c::kInvalidBlockLabel ||
      branch_condition->false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  lines.push_back("cbnz " + condition + ", " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] bool is_fused_compare_branch_support_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value()) {
    return false;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst);
      cast != nullptr &&
      cast->result.kind == bir::Value::Kind::Named) {
    if (!lower_fused_compare_branch_from_emitted_cast(context, scalar_state).has_value()) {
      return false;
    }
    const auto matches = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named &&
             value.name == cast->result.name;
    };
    return matches(*branch_condition->lhs) || matches(*branch_condition->rhs);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr &&
      binary->result.kind == bir::Value::Kind::Named &&
      branch_condition->condition_value.kind == bir::Value::Kind::Named) {
    if (binary->result.name == branch_condition->condition_value.name) {
      return true;
    }
    const auto matches_binary_result = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named &&
             value.name == binary->result.name;
    };
    if ((matches_binary_result(*branch_condition->lhs) ||
         matches_binary_result(*branch_condition->rhs)) &&
        evaluate_same_block_integer_constant(context, binary->result).has_value()) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] module::InstructionLoweringFamily classify_instruction(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> module::InstructionLoweringFamily {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::PhiInst>) {
          return module::InstructionLoweringFamily::Phi;
        } else if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                             std::is_same_v<T, bir::CastInst>) {
          return module::InstructionLoweringFamily::Scalar;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return module::InstructionLoweringFamily::Select;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return module::InstructionLoweringFamily::Call;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                             std::is_same_v<T, bir::LoadGlobalInst> ||
                             std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          return module::InstructionLoweringFamily::Memory;
        }
        return module::InstructionLoweringFamily::Unknown;
      },
      inst);
}

[[nodiscard]] bool is_store_local_instruction(const bir::Inst& inst) {
  return std::get_if<bir::StoreLocalInst>(&inst) != nullptr;
}

void record_address_materialization_result(
    BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* address_record =
      std::get_if<AddressMaterializationRecord>(&instruction.target.payload);
  if (address_record == nullptr || !address_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 address_record->result_value_name,
                                 *address_record->result_register);
}

void record_memory_result(BlockScalarLoweringState& scalar_state,
                          const module::MachineInstruction& instruction) {
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->result_stack_offset_bytes.has_value() ||
      !memory_record->result_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 memory_record->result_value_name,
                                 *memory_record->result_register);
}

[[nodiscard]] bool before_return_publication_already_emitted(
    const BlockScalarLoweringState& scalar_state,
    const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      move_record->phase != prepare::PreparedMovePhase::BeforeReturn ||
      move_record->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      !move_record->destination_register.has_value()) {
    return false;
  }
  const auto emitted =
      find_emitted_scalar_register(scalar_state,
                                   move_record->destination_register->value_name);
  return emitted.has_value() &&
         registers_alias(*emitted, *move_record->destination_register);
}

void record_before_return_publication(BlockScalarLoweringState& scalar_state,
                                      const module::MachineInstruction& instruction) {
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      move_record->phase != prepare::PreparedMovePhase::BeforeReturn ||
      move_record->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      !move_record->destination_register.has_value()) {
    return;
  }
  record_emitted_scalar_register(scalar_state,
                                 move_record->destination_register->value_name,
                                 *move_record->destination_register);
}

[[nodiscard]] bool before_return_move_targets_fpr_abi(
    const prepare::PreparedMoveResolution& move) {
  if (move.destination_register_placement.has_value()) {
    return move.destination_register_placement->bank ==
           prepare::PreparedRegisterBank::Fpr;
  }
  if (!move.destination_register_name.has_value()) {
    return false;
  }
  const auto parsed =
      abi::parse_aarch64_register_name(*move.destination_register_name);
  return parsed.has_value() && parsed->bank == abi::RegisterBank::FpSimd;
}

bool memory_load_result_feeds_before_return_fpr_abi(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId value_id) {
  if (context.function.value_locations == nullptr) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BeforeReturn ||
        bundle.block_index != context.block_index) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.from_value_id == value_id &&
          move.destination_kind ==
              prepare::PreparedMoveDestinationKind::FunctionReturnAbi &&
          move.destination_storage_kind ==
              prepare::PreparedMoveStorageKind::Register &&
          move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
          before_return_move_targets_fpr_abi(move)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

[[nodiscard]] bool symbol_fp_load_has_explicit_storage_placement(
    const module::BlockLoweringContext& context,
    const MemoryInstructionRecord& memory_record) {
  if (memory_record.address.base_kind != MemoryBaseKind::Symbol ||
      (memory_record.value_type != bir::TypeKind::F32 &&
       memory_record.value_type != bir::TypeKind::F64) ||
      !memory_record.result_value_id.has_value() ||
      !memory_record.result_register.has_value() ||
      !abi::is_fp_simd_register(memory_record.result_register->reg) ||
      context.function.storage_plan == nullptr) {
    return false;
  }
  const auto* storage =
      find_storage_plan_value(*context.function.storage_plan,
                              *memory_record.result_value_id);
  return storage != nullptr &&
         storage->encoding == prepare::PreparedStorageEncodingKind::Register &&
         storage->bank == prepare::PreparedRegisterBank::Fpr &&
         storage->register_placement.has_value();
}

void retarget_memory_result_to_prepared_home(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  const bool frame_slot_return_publication =
      memory_record != nullptr &&
      memory_record->address.base_kind == MemoryBaseKind::FrameSlot &&
      memory_record->result_value_id.has_value() &&
      memory_load_result_feeds_before_return_fpr_abi(
          context, *memory_record->result_value_id);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Load ||
      (memory_record->address.base_kind != MemoryBaseKind::Symbol &&
       !frame_slot_return_publication) ||
      !memory_record->result_value_id.has_value() ||
      !memory_record->result_register.has_value() ||
      context.function.value_locations == nullptr) {
    return;
  }
  if (symbol_fp_load_has_explicit_storage_placement(context, *memory_record)) {
    return;
  }

  const auto* home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *memory_record->result_value_id);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return;
  }

  const auto expected_view = memory_record->result_register->expected_view;
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() ||
      parsed->bank != memory_record->result_register->reg.bank) {
    return;
  }
  auto viewed = *parsed;
  if (expected_view.has_value()) {
    viewed.view = *expected_view;
  }
  memory_record->result_register->reg = viewed;
  memory_record->result_register->value_id = home->value_id;
  memory_record->result_register->value_name = home->value_name;
  memory_record->result_register->occupied_register_references = {viewed};
  memory_record->result_register->occupied_registers = {abi::register_name(viewed)};
}

void retarget_pointer_store_value_to_materialized_address(
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  memory_record->value = make_register_operand(materialized_address);
}

[[nodiscard]] bool register_operands_share_physical_register(
    const RegisterOperand& lhs,
    const RegisterOperand& rhs) {
  return lhs.reg.bank == rhs.reg.bank && lhs.reg.index == rhs.reg.index;
}

[[nodiscard]] bool store_local_uses_pointer_value_address(
    const bir::StoreLocalInst& store) {
  return store.address.has_value() &&
         store.address->base_kind == bir::MemoryAddress::BaseKind::PointerValue;
}

[[nodiscard]] std::optional<RegisterOperand> prepared_or_emitted_store_value_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    const BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  if (value_name.has_value()) {
    if (const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
        emitted.has_value()) {
      return emitted;
    }
  }
  auto prepared = make_named_prepared_result_register(context, value);
  if (prepared.has_value()) {
    prepared->occupied_register_references = {prepared->reg};
    prepared->occupied_registers = {abi::register_name(prepared->reg)};
  }
  return prepared;
}

void retarget_store_address_to_materialized_pointer(
    const bir::StoreLocalInst& store,
    module::MachineInstruction& instruction,
    const RegisterOperand& materialized_address) {
  if (!store.address.has_value() ||
      store.address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }

  memory_record->address.base_kind = MemoryBaseKind::Register;
  memory_record->address.base_register = materialized_address;
  memory_record->address.frame_slot_id.reset();
  memory_record->address.symbol_name.reset();
  memory_record->address.symbol_label.clear();
  memory_record->address.pointer_value_name.reset();
  memory_record->address.pointer_value_id.reset();
  memory_record->address.byte_offset = store.address->byte_offset;
  memory_record->address.byte_offset_is_prepared_snapshot = false;
  memory_record->address.size_bytes = store.address->size_bytes;
  memory_record->address.align_bytes = store.address->align_bytes;
  memory_record->address.address_space = store.address->address_space;
  memory_record->address.is_volatile = store.address->is_volatile;
  memory_record->address.can_use_base_plus_offset = true;
}

void retarget_pointer_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      store->value.type != bir::TypeKind::Ptr) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != bir::TypeKind::Ptr) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value()) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

void retarget_store_local_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store_local_uses_pointer_value_address(*store) ||
      store->value.kind != bir::Value::Kind::Named) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store) {
    return;
  }
  const auto* current_register =
      memory_record->value.has_value()
          ? std::get_if<RegisterOperand>(&memory_record->value->payload)
          : nullptr;
  if (current_register == nullptr) {
    return;
  }
  const auto value_register =
      prepared_or_emitted_store_value_register(context, store->value, scalar_state);
  if (!value_register.has_value()) {
    return;
  }
  if (register_operands_share_physical_register(*current_register, *value_register)) {
    return;
  }
  memory_record->value = make_register_operand(*value_register);
}

void retarget_fpr_call_result_store_value_to_emitted_scalar(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    module::MachineInstruction& instruction) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named ||
      (store->value.type != bir::TypeKind::F32 &&
       store->value.type != bir::TypeKind::F64)) {
    return;
  }
  auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&instruction.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      memory_record->value_type != store->value.type) {
    return;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value()) {
    return;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  if (!emitted.has_value() ||
      emitted->role != RegisterOperandRole::CallAbi ||
      emitted->prepared_bank != prepare::PreparedRegisterBank::Fpr ||
      emitted->reg.bank != abi::RegisterBank::FpSimd) {
    return;
  }
  memory_record->value = make_register_operand(*emitted);
}

[[nodiscard]] std::optional<std::string_view> fixed_formal_scalar_store_mnemonic(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool store_local_value_is_fixed_formal(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    c4c::ValueNameId value_name) {
  if (context.function.prepared == nullptr ||
      context.function.bir_function == nullptr ||
      store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  for (const auto& param : context.function.bir_function->params) {
    if (param.is_varargs || param.is_sret || param.is_byval ||
        param.type != store.value.type) {
      continue;
    }
    const auto param_name = prepare::resolve_prepared_value_name_id(
        context.function.prepared->names, param.name);
    if (param_name.has_value() && *param_name == value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_fixed_formal_store_local_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, store->value);
  if (!value_name.has_value() ||
      !store_local_value_is_fixed_formal(context, *store, *value_name)) {
    return std::nullopt;
  }
  const auto emitted = find_emitted_scalar_register(scalar_state, *value_name);
  const auto address = prepared_frame_slot_load_address(context, instruction_index);
  const auto mnemonic = fixed_formal_scalar_store_mnemonic(store->value.type);
  if (!emitted.has_value() || !address.has_value() || !mnemonic.has_value() ||
      !abi::is_gp_register(emitted->reg)) {
    return std::nullopt;
  }
  auto store_reg = emitted->reg;
  if (const auto expected_view = scalar_register_view(store->value.type);
      expected_view.has_value()) {
    const auto resized = abi::gp_register(emitted->reg.index, *expected_view);
    if (!resized.has_value()) {
      return std::nullopt;
    }
    store_reg = *resized;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::string{*mnemonic} + " " +
                                 std::string{abi::register_name(store_reg)} +
                                 ", " + *address,
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::string relocation_operand(std::string_view label,
                                             std::size_t byte_offset) {
  std::string operand{label};
  if (byte_offset != 0) {
    operand += "+";
    operand += std::to_string(byte_offset);
  }
  return operand;
}

[[nodiscard]] std::string register_indirect_address(std::string_view base,
                                                    std::size_t byte_offset) {
  std::string address{"["};
  address += base;
  if (byte_offset != 0) {
    address += ", #";
    address += std::to_string(byte_offset);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::string frame_slot_address(std::size_t offset_bytes) {
  std::string address{"[sp"};
  if (offset_bytes != 0) {
    address += ", #";
    address += std::to_string(offset_bytes);
  }
  address += "]";
  return address;
}

[[nodiscard]] std::optional<abi::RegisterView> scalar_view_for_type(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type);

[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value);

[[nodiscard]] std::optional<std::string> gp_register_name(std::uint8_t index,
                                                          abi::RegisterView view) {
  auto reg = abi::gp_register(index, view);
  if (!reg.has_value()) {
    return std::nullopt;
  }
  return abi::register_name(*reg);
}

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const module::BlockLoweringContext& context,
    std::string_view value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr || value_name.empty()) {
    return nullptr;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto& candidate = context.bir_block->insts[index - 1];
    bool matches = false;
    std::visit(
        [&](const auto& typed_inst) {
          using T = std::decay_t<decltype(typed_inst)>;
          if constexpr (std::is_same_v<T, bir::BinaryInst> ||
                        std::is_same_v<T, bir::CastInst> ||
                        std::is_same_v<T, bir::SelectInst> ||
                        std::is_same_v<T, bir::LoadLocalInst> ||
                        std::is_same_v<T, bir::LoadGlobalInst>) {
            matches = typed_inst.result.kind == bir::Value::Kind::Named &&
                      typed_inst.result.name == value_name;
          }
        },
        candidate);
    if (matches) {
      return &candidate;
    }
  }
  return nullptr;
}

[[nodiscard]] std::optional<std::size_t> prepared_local_load_offset(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return std::nullopt;
  }
  const auto* slot =
      find_frame_slot(context.function.prepared->stack_layout,
                      *access->address.frame_slot_id);
  if (slot == nullptr) {
    return std::nullopt;
  }
  return slot->offset_bytes + static_cast<std::size_t>(access->address.byte_offset);
}

[[nodiscard]] bool emit_prepared_global_symbol_load_to_register(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr) {
    return false;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::GlobalSymbol ||
      !access->address.symbol_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto symbol_name =
      prepare::prepared_link_name(context.function.prepared->names,
                                  *access->address.symbol_name);
  const auto mnemonic = scalar_load_mnemonic(type);
  const auto target_view = scalar_view_for_type(type);
  const auto target = target_view.has_value()
                          ? gp_register_name(target_index, *target_view)
                          : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (symbol_name.empty() || !mnemonic.has_value() || !target.has_value() ||
      !address.has_value()) {
    return false;
  }
  const auto symbol =
      relocation_operand(symbol_name,
                         static_cast<std::size_t>(access->address.byte_offset));
  lines.push_back("adrp " + *address + ", " + symbol);
  lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
  lines.push_back(std::string{*mnemonic} + " " + *target + ", [" + *address + "]");
  return true;
}

[[nodiscard]] std::optional<std::size_t> parse_va_list_field_suffix(
    std::string_view base,
    std::string_view slot_name) {
  if (slot_name.size() <= base.size() + 1 ||
      slot_name.substr(0, base.size()) != base ||
      slot_name[base.size()] != '.') {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = base.size() + 1; index < slot_name.size(); ++index) {
    const char ch = slot_name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

[[nodiscard]] std::optional<std::string> prepared_va_list_field_address(
    const module::BlockLoweringContext& context,
    std::string_view slot_name) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  const auto* entry_plan =
      prepare::find_prepared_variadic_entry_plan(
          *context.function.prepared,
          context.function.control_flow->function_name);
  if (entry_plan == nullptr) {
    return std::nullopt;
  }
  for (const auto& homes : entry_plan->helper_operand_homes) {
    if (homes.helper != prepare::PreparedVariadicEntryHelperKind::VaStart ||
        !homes.destination_va_list.has_value()) {
      continue;
    }
    const auto& va_list_home = *homes.destination_va_list;
    const auto base_name =
        prepare::prepared_value_name(context.function.prepared->names,
                                     va_list_home.value_name);
    const auto field_offset = parse_va_list_field_suffix(base_name, slot_name);
    if (!field_offset.has_value() ||
        va_list_home.kind != prepare::PreparedValueHomeKind::Register ||
        !va_list_home.register_name.has_value()) {
      continue;
    }
    const auto parsed = abi::parse_aarch64_register_name(*va_list_home.register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      continue;
    }
    const auto base = abi::gp_register(parsed->index, abi::RegisterView::X);
    if (!base.has_value()) {
      continue;
    }
    return register_indirect_address(abi::register_name(*base), *field_offset);
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"ldrb"};
    case bir::TypeKind::I16:
      return std::string_view{"ldrh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"ldr"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> scalar_type_size_bytes(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::size_t{1};
    case bir::TypeKind::I16:
      return std::size_t{2};
    case bir::TypeKind::I32:
      return std::size_t{4};
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::size_t{8};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> scalar_load_mnemonic_for_width(
    std::size_t width_bytes) {
  switch (width_bytes) {
    case 1:
      return std::string_view{"ldrb"};
    case 2:
      return std::string_view{"ldrh"};
    case 4:
    case 8:
      return std::string_view{"ldr"};
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> scalar_store_mnemonic(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return std::string_view{"strb"};
    case bir::TypeKind::I16:
      return std::string_view{"strh"};
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return std::string_view{"str"};
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<unsigned> scalar_integer_width_bits(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return 32U;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return 64U;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_gp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  const auto width_bits = scalar_integer_width_bits(type);
  if (!width_bits.has_value()) {
    return std::nullopt;
  }
  if (*width_bits == 32U) {
    return abi::gp_register(reg.index, abi::RegisterView::W);
  }
  return abi::gp_register(reg.index, abi::RegisterView::X);
}

[[nodiscard]] std::optional<abi::RegisterReference> scalar_fp_register_view(
    abi::RegisterReference reg,
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::fp_simd_register(reg.index, abi::RegisterView::S);
    case bir::TypeKind::F64:
      return abi::fp_simd_register(reg.index, abi::RegisterView::D);
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::uint64_t immediate_integer_bits(const bir::Value& value,
                                                   unsigned width_bits) {
  if (value.immediate_bits != 0U) {
    return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate_bits)
                             : value.immediate_bits;
  }
  return width_bits == 32U ? static_cast<std::uint32_t>(value.immediate)
                           : static_cast<std::uint64_t>(value.immediate);
}

[[nodiscard]] bool emit_fp_immediate_to_register(std::vector<std::string>& lines,
                                                 const bir::Value& value,
                                                 abi::RegisterReference fp_destination,
                                                 std::uint8_t gp_scratch_index) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return false;
  }
  const auto fp_view = scalar_fp_register_view(fp_destination, value.type);
  if (!fp_view.has_value()) {
    return false;
  }
  if (value.type == bir::TypeKind::F32) {
    const auto gp_scratch = abi::gp_register(gp_scratch_index, abi::RegisterView::W);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 32U);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  if (value.type == bir::TypeKind::F64) {
    const auto gp_scratch = abi::gp_register(gp_scratch_index, abi::RegisterView::X);
    if (!gp_scratch.has_value()) {
      return false;
    }
    auto materialize =
        materialize_integer_constant_lines(*gp_scratch, value.immediate_bits, 64U);
    if (materialize.empty()) {
      return false;
    }
    lines.insert(lines.end(), materialize.begin(), materialize.end());
    lines.push_back("fmov " + std::string{abi::register_name(*fp_view)} + ", " +
                    std::string{abi::register_name(*gp_scratch)});
    return true;
  }
  return false;
}

[[nodiscard]] bool emit_fp_value_to_register(const module::BlockLoweringContext& context,
                                             const bir::Value& value,
                                             std::size_t before_instruction_index,
                                             abi::RegisterReference destination,
                                             std::uint8_t gp_scratch_index,
                                             std::vector<std::string>& lines) {
  const auto destination_view = scalar_fp_register_view(destination, value.type);
  if (!destination_view.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return emit_fp_immediate_to_register(
        lines, value, *destination_view, gp_scratch_index);
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (const auto* select =
          producer != nullptr ? std::get_if<bir::SelectInst>(producer) : nullptr;
      select != nullptr) {
    const auto condition = branch_condition_suffix(select->predicate);
    const auto compare_view = scalar_view_for_type(select->compare_type);
    if (!condition.has_value() || !compare_view.has_value()) {
      return false;
    }
    const auto producer_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
    const auto gp_lhs = abi::gp_register(gp_scratch_index, *compare_view);
    const std::uint8_t gp_rhs_index = gp_scratch_index == 9 ? 10 : 9;
    const auto gp_rhs = abi::gp_register(gp_rhs_index, *compare_view);
    if (!gp_lhs.has_value() || !gp_rhs.has_value()) {
      return false;
    }
    std::optional<abi::RegisterReference> true_scratch;
    for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
      if (scratch.index == destination.index) {
        continue;
      }
      true_scratch = scalar_fp_register_view(scratch, value.type);
      if (true_scratch.has_value()) {
        break;
      }
    }
    if (!true_scratch.has_value()) {
      return false;
    }
    auto true_value = select->true_value;
    true_value.type = value.type;
    auto false_value = select->false_value;
    false_value.type = value.type;
    if (!emit_fp_value_to_register(
            context, false_value, producer_index, *destination_view, gp_scratch_index, lines) ||
        !emit_fp_value_to_register(
            context, true_value, producer_index, *true_scratch, gp_scratch_index, lines)) {
      return false;
    }
    auto lhs = select->lhs;
    lhs.type = select->compare_type;
    if (!emit_value_publication_to_register(
            context, lhs, producer_index, gp_scratch_index, gp_rhs_index, lines)) {
      return false;
    }
    std::string rhs_name;
    if (select->rhs.kind == bir::Value::Kind::Immediate &&
        is_cmp_immediate_encodable(select->rhs.immediate)) {
      rhs_name = "#" + std::to_string(select->rhs.immediate);
    } else {
      auto rhs = select->rhs;
      rhs.type = select->compare_type;
      if (!emit_value_publication_to_register(
              context, rhs, producer_index, gp_rhs_index, gp_scratch_index, lines)) {
        return false;
      }
      rhs_name = abi::register_name(*gp_rhs);
    }
    lines.push_back("cmp " + std::string{abi::register_name(*gp_lhs)} + ", " + rhs_name);
    lines.push_back("fcsel " + std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{abi::register_name(*true_scratch)} + ", " +
                    std::string{abi::register_name(*destination_view)} + ", " +
                    std::string{*condition});
    return true;
  }
  if (const auto* load_local =
          producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
      load_local != nullptr) {
    const auto producer_index = producer_instruction_index(context, producer);
    const auto offset = producer_index.has_value()
                            ? prepared_local_load_offset(context, *producer_index)
                            : std::nullopt;
    if (offset.has_value()) {
      lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} +
                      ", " + frame_slot_address(*offset));
      return true;
    }
  }
  if (const auto* load_global =
          producer != nullptr ? std::get_if<bir::LoadGlobalInst>(producer) : nullptr;
      load_global != nullptr) {
    const auto address = abi::gp_register(gp_scratch_index, abi::RegisterView::X);
    if (!address.has_value() || load_global->global_name.empty()) {
      return false;
    }
    const bir::Global* target_global = find_load_global_target(context, *load_global);
    const auto symbol_label = load_global_symbol_label(context, *load_global, target_global);
    if (symbol_label.empty()) {
      return false;
    }
    if (target_global != nullptr &&
        target_global->address_materialization_policy ==
            bir::GlobalAddressMaterializationPolicy::GotRequired) {
      lines.push_back("adrp " + std::string{abi::register_name(*address)} + ", :got:" +
                      symbol_label);
      lines.push_back("ldr " + std::string{abi::register_name(*address)} + ", [" +
                      std::string{abi::register_name(*address)} + ", :got_lo12:" +
                      symbol_label + "]");
      lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} + ", " +
                      register_indirect_address(abi::register_name(*address),
                                                load_global->byte_offset));
      return true;
    }
    const auto symbol = relocation_operand(symbol_label, load_global->byte_offset);
    lines.push_back("adrp " + std::string{abi::register_name(*address)} + ", " + symbol);
    lines.push_back("add " + std::string{abi::register_name(*address)} + ", " +
                    std::string{abi::register_name(*address)} + ", :lo12:" + symbol);
    lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} + ", [" +
                    std::string{abi::register_name(*address)} + "]");
    return true;
  }
  if (const auto* cast =
          producer != nullptr ? std::get_if<bir::CastInst>(producer) : nullptr;
      cast != nullptr) {
    const auto producer_index =
        producer_instruction_index(context, producer).value_or(before_instruction_index);
    switch (cast->opcode) {
      case bir::CastOpcode::SIToFP:
      case bir::CastOpcode::UIToFP: {
        const auto source_width = scalar_integer_width_bits(cast->operand.type);
        const auto gp_source =
            source_width.has_value()
                ? abi::gp_register(gp_scratch_index,
                                   *source_width == 64U ? abi::RegisterView::X
                                                        : abi::RegisterView::W)
                : std::nullopt;
        if (!source_width.has_value() || !gp_source.has_value()) {
          return false;
        }
        if (cast->operand.kind == bir::Value::Kind::Immediate) {
          auto materialize = materialize_integer_constant_lines(
              *gp_source, immediate_integer_bits(cast->operand, *source_width), *source_width);
          if (materialize.empty()) {
            return false;
          }
          lines.insert(lines.end(), materialize.begin(), materialize.end());
        } else if (!emit_value_publication_to_register(context,
                                                       cast->operand,
                                                       producer_index,
                                                       gp_scratch_index,
                                                       destination.index,
                                                       lines)) {
          return false;
        }
        lines.push_back(std::string{cast->opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                            : "ucvtf "} +
                        std::string{abi::register_name(*destination_view)} + ", " +
                        std::string{abi::register_name(*gp_source)});
        return true;
      }
      case bir::CastOpcode::FPExt:
      case bir::CastOpcode::FPTrunc: {
        std::optional<abi::RegisterReference> fp_source_base;
        for (const auto scratch : abi::reserved_mir_scratch_fp_simd_registers()) {
          if (scratch.index != destination.index) {
            fp_source_base = abi::fp_simd_register(scratch.index, abi::RegisterView::D);
            break;
          }
        }
        if (!fp_source_base.has_value() ||
            !emit_fp_value_to_register(context,
                                       cast->operand,
                                       producer_index,
                                       *fp_source_base,
                                       gp_scratch_index,
                                       lines)) {
          return false;
        }
        const auto fp_source = scalar_fp_register_view(*fp_source_base, cast->operand.type);
        if (!fp_source.has_value()) {
          return false;
        }
        lines.push_back("fcvt " + std::string{abi::register_name(*destination_view)} +
                        ", " + std::string{abi::register_name(*fp_source)});
        return true;
      }
      case bir::CastOpcode::FPToSI:
      case bir::CastOpcode::FPToUI:
      case bir::CastOpcode::SExt:
      case bir::CastOpcode::ZExt:
      case bir::CastOpcode::Trunc:
      case bir::CastOpcode::PtrToInt:
      case bir::CastOpcode::IntToPtr:
      case bir::CastOpcode::Bitcast:
        break;
    }
  }
  const auto* home = prepared_value_home_for_value(context, value);
  if (home == nullptr) {
    return false;
  }
  if (home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    lines.push_back("ldr " + std::string{abi::register_name(*destination_view)} +
                    ", " + frame_slot_address(*home->offset_bytes));
    return true;
  }
  if (home->kind == prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return false;
    }
    const auto source = scalar_fp_register_view(*parsed, value.type);
    if (!source.has_value()) {
      return false;
    }
    const std::string source_name = abi::register_name(*source);
    const std::string destination_name = abi::register_name(*destination_view);
    if (source_name != destination_name) {
      lines.push_back("fmov " + destination_name + ", " + source_name);
    }
    return true;
  }
  return false;
}

[[nodiscard]] bool emit_prepared_va_list_field_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto address = prepared_va_list_field_address(context, load_local.slot_name);
  if (!address.has_value()) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!mnemonic.has_value() || !target.has_value()) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " + *address);
  return true;
}

[[nodiscard]] bool is_byval_formal_value_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.function.prepared == nullptr ||
      context.function.bir_function == nullptr) {
    return false;
  }
  const std::string_view name =
      prepare::prepared_value_name(context.function.prepared->names, value_name);
  if (name.empty()) {
    return false;
  }
  for (const auto& param : context.function.bir_function->params) {
    if (param.is_byval && param.name == name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool emit_prepared_pointer_value_load_to_register(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load_local,
    std::size_t instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  const auto* access =
      addressing != nullptr
          ? prepare::find_prepared_memory_access(
                *addressing, context.control_flow_block->block_label, instruction_index)
          : nullptr;
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto mnemonic = scalar_load_mnemonic(load_local.result.type);
  const auto target_view = scalar_view_for_type(load_local.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!mnemonic.has_value() || !target.has_value() || !address.has_value()) {
    return false;
  }
  const auto* pointer_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *access->address.pointer_value_name);
  if (pointer_home == nullptr) {
    return false;
  }
  if (pointer_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      pointer_home->offset_bytes.has_value()) {
    if (is_byval_formal_value_name(context, *access->address.pointer_value_name)) {
      const auto offset =
          static_cast<std::int64_t>(*pointer_home->offset_bytes) +
          access->address.byte_offset;
      if (offset < 0) {
        return false;
      }
      lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                      frame_slot_address(static_cast<std::size_t>(offset)));
      return true;
    }
    lines.push_back("ldr " + *address + ", " +
                    frame_slot_address(*pointer_home->offset_bytes));
  } else if (pointer_home->kind == prepare::PreparedValueHomeKind::Register &&
             pointer_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*pointer_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto viewed = abi::gp_register(parsed->index, abi::RegisterView::X);
    if (!viewed.has_value()) {
      return false;
    }
    const auto source = abi::register_name(*viewed);
    if (source != *address) {
      lines.push_back("mov " + *address + ", " + std::string{source});
    }
  } else {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  register_indirect_address(*address,
                                            static_cast<std::size_t>(
                                                access->address.byte_offset)));
  return true;
}

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const module::BlockLoweringContext& context,
    const bir::Inst* producer) {
  if (context.bir_block == nullptr || producer == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    if (&context.bir_block->insts[index] == producer) {
      return index;
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::string_view local_slot_reference_name(
    const module::BlockLoweringContext& context,
    std::string_view raw_name,
    SlotNameId slot_id) {
  if (!raw_name.empty()) {
    return raw_name;
  }
  if (context.function.prepared != nullptr && slot_id != c4c::kInvalidSlotName) {
    return context.function.prepared->module.names.slot_names.spelling(slot_id);
  }
  return {};
}

[[nodiscard]] bool local_slot_reference_matches(const module::BlockLoweringContext& context,
                                                const bir::LoadLocalInst& load,
                                                const bir::StoreLocalInst& store) {
  if (load.slot_id != c4c::kInvalidSlotName &&
      store.slot_id != c4c::kInvalidSlotName) {
    if (load.slot_id == store.slot_id) {
      return true;
    }
  }
  const auto load_name =
      local_slot_reference_name(context, load.slot_name, load.slot_id);
  const auto store_name =
      local_slot_reference_name(context, store.slot_name, store.slot_id);
  return !load_name.empty() && load_name == store_name;
}

[[nodiscard]] std::string_view prepared_frame_slot_object_name(
    const module::BlockLoweringContext& context,
    prepare::PreparedFrameSlotId slot_id) {
  if (context.function.prepared == nullptr) {
    return {};
  }
  const auto* slot = find_frame_slot(context.function.prepared->stack_layout, slot_id);
  const auto* object =
      slot != nullptr
          ? find_stack_object(context.function.prepared->stack_layout, slot->object_id)
          : nullptr;
  return object != nullptr
             ? prepare::prepared_stack_object_name(context.function.prepared->names,
                                                   *object)
             : std::string_view{};
}

[[nodiscard]] std::string_view prepared_load_local_frame_object_name(
    const module::BlockLoweringContext& context,
    std::size_t load_instruction_index) {
  const auto* access = prepared_memory_access(context, load_instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::FrameSlot ||
      !access->address.frame_slot_id.has_value()) {
    return {};
  }
  return prepared_frame_slot_object_name(context, *access->address.frame_slot_id);
}

[[nodiscard]] bool value_name_has_slot_prefix(std::string_view value_name,
                                              std::string_view slot_name) {
  return !slot_name.empty() && value_name.size() > slot_name.size() &&
         value_name.substr(0, slot_name.size()) == slot_name &&
         value_name[slot_name.size()] == '.';
}

[[nodiscard]] std::optional<std::size_t> parse_trailing_dot_offset(
    std::string_view name) {
  const auto dot = name.rfind('.');
  if (dot == std::string_view::npos || dot + 1 >= name.size()) {
    return std::nullopt;
  }
  std::size_t value = 0;
  for (std::size_t index = dot + 1; index < name.size(); ++index) {
    const char ch = name[index];
    if (ch < '0' || ch > '9') {
      return std::nullopt;
    }
    value = value * 10U + static_cast<std::size_t>(ch - '0');
  }
  return value;
}

[[nodiscard]] bool store_local_targets_logical_slot(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::string_view slot_name) {
  const auto store_name =
      local_slot_reference_name(context, store.slot_name, store.slot_id);
  if (!store_name.empty() && store_name == slot_name) {
    return true;
  }
  return store.value.kind == bir::Value::Kind::Named &&
         value_name_has_slot_prefix(store.value.name, slot_name);
}

struct NarrowLocalStorePublication {
  bir::Value stored_value;
  std::size_t instruction_index = 0;
};

[[nodiscard]] std::optional<NarrowLocalStorePublication>
find_latest_narrow_store_for_wide_local_load(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const auto load_bits = integer_bit_width(load.result.type);
  if (!load_bits.has_value()) {
    return std::nullopt;
  }
  const auto load_slot_name =
      local_slot_reference_name(context, load.slot_name, load.slot_id);
  const auto load_frame_object_name =
      prepared_load_local_frame_object_name(context, load_instruction_index);
  const auto load_lane_offset =
      load.result.kind == bir::Value::Kind::Named
          ? parse_trailing_dot_offset(load.result.name)
          : std::nullopt;
  for (std::size_t index = load_instruction_index; index > 0; --index) {
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[index - 1]);
    if (store == nullptr ||
        (!local_slot_reference_matches(context, load, *store) &&
         !store_local_targets_logical_slot(context, *store, load_slot_name) &&
         !store_local_targets_logical_slot(context, *store, load_frame_object_name) &&
         !(load_lane_offset.has_value() &&
           store->value.kind == bir::Value::Kind::Named &&
           parse_trailing_dot_offset(store->value.name) == load_lane_offset))) {
      continue;
    }
    const auto store_bits = integer_bit_width(store->value.type);
    if (!store_bits.has_value() || *store_bits != 8U || *store_bits >= *load_bits) {
      return std::nullopt;
    }
    return NarrowLocalStorePublication{
        .stored_value = store->value,
        .instruction_index = index - 1,
    };
  }
  return std::nullopt;
}

[[nodiscard]] const bir::Global* find_load_global_target(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global) {
  if (context.function.prepared == nullptr) {
    return nullptr;
  }
  const auto& globals = context.function.prepared->module.globals;
  if (load_global.global_name_id != c4c::kInvalidLinkName) {
    const auto it = std::find_if(
        globals.begin(),
        globals.end(),
        [&](const bir::Global& global) {
          return global.link_name_id == load_global.global_name_id;
        });
    if (it != globals.end()) {
      return &*it;
    }
  }
  if (load_global.global_name.empty()) {
    return nullptr;
  }
  const auto it = std::find_if(
      globals.begin(),
      globals.end(),
      [&](const bir::Global& global) {
        return global.name == load_global.global_name;
      });
  return it == globals.end() ? nullptr : &*it;
}

[[nodiscard]] std::string load_global_symbol_label(
    const module::BlockLoweringContext& context,
    const bir::LoadGlobalInst& load_global,
    const bir::Global* target_global) {
  if (context.function.prepared != nullptr &&
      load_global.global_name_id != c4c::kInvalidLinkName) {
    const std::string_view semantic_name =
        context.function.prepared->module.names.link_names.spelling(
            load_global.global_name_id);
    if (!semantic_name.empty()) {
      return std::string{semantic_name};
    }
  }
  if (target_global != nullptr && !target_global->name.empty()) {
    return target_global->name;
  }
  return load_global.global_name;
}

[[nodiscard]] const prepare::PreparedValueHome* prepared_value_home_for_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.function.value_locations == nullptr) {
    return nullptr;
  }
  const auto value_name = prepared_named_value_id(context, value);
  return value_name.has_value()
             ? prepare::find_prepared_value_home(*context.function.value_locations,
                                                 *value_name)
             : nullptr;
}

[[nodiscard]] bool value_has_current_block_entry_publication(
    const module::BlockLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
          move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
          move.to_value_id == home.value_id) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<RegisterOperand> current_block_entry_publication_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    abi::RegisterView expected_view) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr ||
      value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  auto value_name = prepared_named_value_id(context, value);
  const prepare::PreparedValueHome* home =
      value_name.has_value()
          ? prepare::find_prepared_value_home(*context.function.value_locations,
                                              *value_name)
          : nullptr;
  if (home == nullptr && context.function.prepared != nullptr && !value.name.empty()) {
    for (const auto& candidate : context.function.value_locations->value_homes) {
      if (candidate.value_name == c4c::kInvalidValueName) {
        continue;
      }
      const auto candidate_name =
          prepare::prepared_value_name(context.function.prepared->names,
                                       candidate.value_name);
      if (candidate_name == value.name) {
        home = &candidate;
        value_name = candidate.value_name;
        break;
      }
    }
  }
  if (home == nullptr) {
    return std::nullopt;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
          move.to_value_id != home->value_id) {
        continue;
      }
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
      if (!parsed.has_value() ||
          parsed->bank != abi::RegisterBank::GeneralPurpose) {
        continue;
      }
      auto reg = abi::gp_register(parsed->index, expected_view).value_or(*parsed);
      reg.view = expected_view;
      return RegisterOperand{
          .reg = reg,
          .role = RegisterOperandRole::StoragePlan,
          .value_id = home->value_id,
          .value_name = home->value_name,
          .expected_view = expected_view,
      };
    }
  }
  return std::nullopt;
}

void record_current_block_entry_publication_registers(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      const auto* home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            move.to_value_id);
      if (home == nullptr) {
        continue;
      }
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
      if (!parsed.has_value() ||
          parsed->bank != abi::RegisterBank::GeneralPurpose) {
        continue;
      }
      record_emitted_scalar_register(
          scalar_state,
          home->value_name,
          RegisterOperand{
              .reg = *parsed,
              .role = RegisterOperandRole::StoragePlan,
              .value_id = home->value_id,
              .value_name = home->value_name,
              .expected_view = parsed->view,
          });
    }
  }
}

[[nodiscard]] bool block_entry_move_clobbers_current_join_publication(
    const module::BlockLoweringContext& context,
    const module::MachineInstruction& instruction) {
  if (context.function.value_locations == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr || !move_record->destination_register.has_value()) {
    return false;
  }
  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value ||
          move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      const auto* home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            move.to_value_id);
      std::optional<std::string> register_name = move.destination_register_name;
      if (!register_name.has_value() &&
          home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        register_name = home->register_name;
      }
      if (!register_name.has_value()) {
        continue;
      }
      const auto parsed = abi::parse_aarch64_register_name(*register_name);
      if (!parsed.has_value()) {
        continue;
      }
      RegisterOperand published{
          .reg = *parsed,
          .role = RegisterOperandRole::StoragePlan,
          .value_id = home != nullptr
                          ? std::optional<prepare::PreparedValueId>{home->value_id}
                          : std::nullopt,
          .value_name = home != nullptr ? home->value_name : c4c::kInvalidValueName,
          .expected_view = parsed->view,
      };
      if (registers_alias(published, *move_record->destination_register)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] bool emit_prepared_value_home_to_register(
    const prepare::PreparedStackLayout* stack_layout,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type,
    std::uint8_t target_index,
    std::vector<std::string>& lines) {
  const auto target_view = scalar_view_for_type(type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (home.kind == prepare::PreparedValueHomeKind::StackSlot &&
      home.offset_bytes.has_value()) {
    const auto type_width = scalar_type_size_bytes(type);
    if (!type_width.has_value()) {
      return false;
    }
    std::optional<std::size_t> home_width = home.size_bytes;
    if (stack_layout != nullptr && home.slot_id.has_value()) {
      if (const auto* slot = find_frame_slot(*stack_layout, *home.slot_id); slot != nullptr) {
        home_width = home_width.has_value()
                         ? std::min(*home_width, slot->size_bytes)
                         : std::optional<std::size_t>{slot->size_bytes};
      }
    }
    const auto load_width =
        home_width.has_value()
            ? std::min(*home_width, *type_width)
            : *type_width;
    const auto mnemonic = scalar_load_mnemonic_for_width(load_width);
    const auto load_view = load_width == 8 ? abi::RegisterView::X
                                           : abi::RegisterView::W;
    const auto load_target = gp_register_name(target_index, load_view);
    if (!mnemonic.has_value() || !load_target.has_value()) {
      return false;
    }
    lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                    frame_slot_address(*home.offset_bytes));
    return true;
  }
  if (home.kind == prepare::PreparedValueHomeKind::Register &&
      home.register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return false;
    }
    const auto source = abi::gp_register(parsed->index, *target_view);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = std::string{abi::register_name(*source)};
    if (source_name != *target) {
      lines.push_back("mov " + *target + ", " + source_name);
    }
    return true;
  }
  return false;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_home_fused_compare_branch(
    const module::BlockLoweringContext& context) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch ||
      !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() ||
      !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value() ||
      branch_condition->rhs->kind != bir::Value::Kind::Immediate ||
      !is_cmp_immediate_encodable(branch_condition->rhs->immediate)) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(*branch_condition->predicate);
  const auto operand_view = scalar_view_for_type(*branch_condition->compare_type);
  if (!condition.has_value() || !operand_view.has_value() ||
      branch_condition->true_label == c4c::kInvalidBlockLabel ||
      branch_condition->false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  auto lhs = *branch_condition->lhs;
  lhs.type = *branch_condition->compare_type;
  if (lhs.kind == bir::Value::Kind::Named && context.bir_block != nullptr) {
    const auto* producer =
        find_same_block_named_producer(context, lhs.name, context.bir_block->insts.size());
    if (producer != nullptr &&
        std::get_if<bir::LoadLocalInst>(producer) == nullptr &&
        std::get_if<bir::LoadGlobalInst>(producer) == nullptr) {
      return std::nullopt;
    }
  }
  const auto* home = prepared_value_home_for_value(context, lhs);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const auto compare_reg = abi::gp_register(scratches.front().index, *operand_view);
  if (!compare_reg.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_prepared_value_home_to_register(&context.function.prepared->stack_layout,
                                            *home,
                                            *branch_condition->compare_type,
                                            scratches.front().index,
                                            lines)) {
    return std::nullopt;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*compare_reg)} + ", #" +
                  std::to_string(branch_condition->rhs->immediate));
  lines.push_back("b." + std::string{*condition} + " " +
                  machine_block_label(branch_condition->function_name,
                                      branch_condition->true_label));
  lines.push_back("b " + machine_block_label(branch_condition->function_name,
                                             branch_condition->false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

[[nodiscard]] bool prepared_value_home_reads_register_index(
    const prepare::PreparedValueHome& home,
    std::uint8_t register_index) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return false;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  return parsed.has_value() && parsed->bank == abi::RegisterBank::GeneralPurpose &&
         parsed->index == register_index;
}

[[nodiscard]] bool value_publication_may_read_register_index(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  const auto* home = prepared_value_home_for_value(context, value);
  if (producer == nullptr ||
      (home != nullptr && value_has_current_block_entry_publication(context, *home))) {
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }

  const auto producer_index =
      producer_instruction_index(context, producer).value_or(before_instruction_index);
  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return value_publication_may_read_register_index(
        context, operand, producer_index, register_index, depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer); binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    return value_publication_may_read_register_index(
               context, lhs, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, rhs, producer_index, register_index, depth + 1);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer); select != nullptr) {
    return value_publication_may_read_register_index(
               context, select->true_value, producer_index, register_index, depth + 1) ||
           value_publication_may_read_register_index(
               context, select->false_value, producer_index, register_index, depth + 1);
  }
  return false;
}

[[nodiscard]] bool emit_value_publication_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines,
    bool reload_current_memory_loads) {
  const auto target_view = scalar_view_for_type(value.type);
  if (!target_view.has_value()) {
    return false;
  }
  const auto target = gp_register_name(target_index, *target_view);
  if (!target.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  if (auto published =
          current_block_entry_publication_register(context, value, *target_view)) {
    const auto source = abi::gp_register(published->reg.index, *target_view);
    if (!source.has_value()) {
      return false;
    }
    const auto source_name = std::string{abi::register_name(*source)};
    if (source_name != *target) {
      lines.push_back("mov " + *target + ", " + source_name);
    }
    return true;
  }
  const auto* producer =
      find_same_block_named_producer(context, value.name, before_instruction_index);
  if (producer != nullptr &&
      is_current_block_join_parallel_copy_source(context, *producer)) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines);
    }
  }
  if (producer == nullptr) {
    const auto* home = prepared_value_home_for_value(context, value);
    if (home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines);
    }
    return false;
  }
  if (const auto* home = prepared_value_home_for_value(context, value);
      home != nullptr && value_has_current_block_entry_publication(context, *home)) {
    return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                   ? &context.function.prepared->stack_layout
                                                   : nullptr,
                                               *home,
                                               value.type,
                                               target_index,
                                               lines);
  }

  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(producer);
      load_local != nullptr) {
    const auto index = producer_instruction_index(context, producer);
    if (const auto* home = prepared_value_home_for_value(context, value);
        !reload_current_memory_loads && home != nullptr) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines);
    }
    if (emit_prepared_va_list_field_load_to_register(
            context, *load_local, target_index, lines)) {
      return true;
    }
    if (index.has_value() &&
        emit_prepared_global_symbol_load_to_register(context,
                                                     *index,
                                                     load_local->result.type,
                                                     target_index,
                                                     scratch_index,
                                                     lines)) {
      return true;
    }
    if (index.has_value()) {
      const auto narrow_store =
          find_latest_narrow_store_for_wide_local_load(context, *load_local, *index);
      if (narrow_store.has_value() &&
          emit_value_publication_to_register(context,
                                             narrow_store->stored_value,
                                             narrow_store->instruction_index,
                                             target_index,
                                             scratch_index,
                                             lines,
                                             reload_current_memory_loads)) {
        return true;
      }
    }
    const auto offset =
        index.has_value() ? prepared_local_load_offset(context, *index) : std::nullopt;
    if (offset.has_value()) {
      const auto mnemonic = scalar_load_mnemonic(load_local->result.type);
      const auto load_view = scalar_view_for_type(load_local->result.type);
      const auto load_target =
          load_view.has_value() ? gp_register_name(target_index, *load_view) : std::nullopt;
      if (!mnemonic.has_value() || !load_target.has_value()) {
        return false;
      }
      lines.push_back(std::string{*mnemonic} + " " + *load_target + ", " +
                      frame_slot_address(*offset));
      return true;
    }
    return index.has_value() &&
           emit_prepared_pointer_value_load_to_register(
               context, *load_local, *index, target_index, scratch_index, lines);
  }

  if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(producer);
      load_global != nullptr) {
    if (const auto* home = prepared_value_home_for_value(context, value);
        !reload_current_memory_loads && home != nullptr &&
        home->kind == prepare::PreparedValueHomeKind::StackSlot &&
        home->offset_bytes.has_value()) {
      return emit_prepared_value_home_to_register(context.function.prepared != nullptr
                                                     ? &context.function.prepared->stack_layout
                                                     : nullptr,
                                                 *home,
                                                 value.type,
                                                 target_index,
                                                 lines);
    }
    const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
    if (!address.has_value() || load_global->global_name.empty()) {
      return false;
    }
    const bir::Global* target_global = find_load_global_target(context, *load_global);
    const auto symbol_label = load_global_symbol_label(context, *load_global, target_global);
    if (symbol_label.empty()) {
      return false;
    }
    if (target_global != nullptr &&
        target_global->address_materialization_policy ==
            bir::GlobalAddressMaterializationPolicy::GotRequired) {
      lines.push_back("adrp " + *address + ", :got:" + symbol_label);
      lines.push_back("ldr " + *address + ", [" + *address + ", :got_lo12:" +
                      symbol_label + "]");
      lines.push_back("ldr " + *target + ", " +
                      register_indirect_address(*address, load_global->byte_offset));
      return true;
    }
    const auto symbol = relocation_operand(symbol_label, load_global->byte_offset);
    lines.push_back("adrp " + *address + ", " + symbol);
    lines.push_back("add " + *address + ", " + *address + ", :lo12:" + symbol);
    lines.push_back("ldr " + *target + ", [" + *address + "]");
    return true;
  }

  if (const auto* cast = std::get_if<bir::CastInst>(producer); cast != nullptr) {
    if (cast->opcode == bir::CastOpcode::SExt) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      if (!emit_value_publication_to_register(
              context,
              source,
              before_instruction_index,
              target_index,
              scratch_index,
              lines,
              reload_current_memory_loads)) {
        return false;
      }
      const auto source_bits = integer_bit_width(cast->operand.type);
      const auto result_bits = integer_bit_width(cast->result.type);
      const auto source_name = gp_register_name(target_index, abi::RegisterView::W);
      const auto target_name = gp_register_name(target_index, *target_view);
      if (!source_bits.has_value() || !result_bits.has_value() ||
          !source_name.has_value() || !target_name.has_value() ||
          *source_bits >= *result_bits) {
        return false;
      }
      if (*source_bits == 8U) {
        lines.push_back("sxtb " + *target_name + ", " + *source_name);
      } else if (*source_bits == 16U) {
        lines.push_back("sxth " + *target_name + ", " + *source_name);
      } else if (*source_bits == 32U && *result_bits == 64U) {
        lines.push_back("sxtw " + *target_name + ", " + *source_name);
      } else {
        return false;
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::ZExt) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      if (!emit_value_publication_to_register(
              context,
              source,
              before_instruction_index,
              target_index,
              scratch_index,
              lines,
              reload_current_memory_loads)) {
        return false;
      }
      const auto source_bits = integer_bit_width(cast->operand.type);
      const auto result_bits = integer_bit_width(cast->result.type);
      if (!source_bits.has_value() || !result_bits.has_value()) {
        return false;
      }
      if (*source_bits < 32U || *source_bits < *result_bits) {
        const auto widened = gp_register_name(target_index, abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        if (*source_bits < 32U) {
          lines.push_back("ubfx " + *widened + ", " + *widened +
                          ", #0, #" + std::to_string(*source_bits));
        }
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::Trunc) {
      bir::Value source = cast->operand;
      source.type = cast->operand.type;
      return emit_value_publication_to_register(
          context,
          source,
          before_instruction_index,
          target_index,
          scratch_index,
          lines,
          reload_current_memory_loads);
    }
    return false;
  }

  if (std::get_if<bir::SelectInst>(producer) != nullptr) {
    std::size_t label_index = 0;
    std::vector<std::string_view> active_values;
    return emit_select_chain_value_to_register(context,
                                               value,
                                               before_instruction_index,
                                               target_index,
                                               scratch_index,
                                               before_instruction_index,
                                               prepared_named_value_id(context, value)
                                                   .value_or(c4c::kInvalidValueName),
                                               lines,
                                               label_index,
                                               active_values,
                                               reload_current_memory_loads);
  }

  const auto* binary = std::get_if<bir::BinaryInst>(producer);
  if (binary == nullptr) {
    return false;
  }

  if (const auto condition = branch_condition_suffix(binary->opcode);
      condition.has_value()) {
    if (binary->operand_type == bir::TypeKind::F32 ||
        binary->operand_type == bir::TypeKind::F64) {
      const auto result_name =
          gp_register_name(target_index, scalar_view_for_type(binary->result.type)
                                             .value_or(abi::RegisterView::W));
      const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
      if (!result_name.has_value() || fp_scratches.size() < 2U) {
        return false;
      }
      const auto lhs_fp = scalar_fp_register_view(fp_scratches[0], binary->operand_type);
      const auto rhs_fp = scalar_fp_register_view(fp_scratches[1], binary->operand_type);
      if (!lhs_fp.has_value() || !rhs_fp.has_value()) {
        return false;
      }
      auto lhs = binary->lhs;
      lhs.type = binary->operand_type;
      auto rhs = binary->rhs;
      rhs.type = binary->operand_type;
      if (!emit_fp_value_to_register(context,
                                     lhs,
                                     before_instruction_index,
                                     *lhs_fp,
                                     scratch_index,
                                     lines) ||
          !emit_fp_value_to_register(context,
                                     rhs,
                                     before_instruction_index,
                                     *rhs_fp,
                                     target_index,
                                     lines)) {
        return false;
      }
      lines.push_back("fcmp " + std::string{abi::register_name(*lhs_fp)} + ", " +
                      std::string{abi::register_name(*rhs_fp)});
      lines.push_back("cset " + *result_name + ", " + std::string{*condition});
      return true;
    }
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto lhs_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto result_name =
        gp_register_name(target_index, scalar_view_for_type(binary->result.type)
                                           .value_or(abi::RegisterView::W));
    if (!operand_view.has_value() || !lhs_name.has_value() ||
        !result_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    if (!emit_value_publication_to_register(
            context,
            lhs,
            before_instruction_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads)) {
      return false;
    }
    std::optional<std::string> rhs_name;
    if (binary->rhs.kind == bir::Value::Kind::Immediate &&
        is_cmp_immediate_encodable(binary->rhs.immediate)) {
      rhs_name = "#" + std::to_string(binary->rhs.immediate);
    } else {
      rhs_name = gp_register_name(scratch_index, *operand_view);
      auto rhs = binary->rhs;
      rhs.type = binary->operand_type;
      const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
      if (!rhs_name.has_value() ||
          !emit_value_publication_to_register(context,
                                              rhs,
                                              before_instruction_index,
                                              scratch_index,
                                              nested_scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
    }
    lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
    lines.push_back("cset " + *result_name + ", " + std::string{*condition});
    return true;
  }

  if (binary->opcode == bir::BinaryOpcode::And ||
      binary->opcode == bir::BinaryOpcode::Or ||
      binary->opcode == bir::BinaryOpcode::Xor) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto result_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto rhs_name =
        operand_view.has_value() ? gp_register_name(scratch_index, *operand_view)
                                 : std::nullopt;
    if (!operand_view.has_value() || !result_name.has_value() ||
        !rhs_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (!emit_value_publication_to_register(
            context,
            lhs,
            before_instruction_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads)) {
      return false;
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
    std::string_view opcode = "and";
    if (binary->opcode == bir::BinaryOpcode::Or) {
      opcode = "orr";
    } else if (binary->opcode == bir::BinaryOpcode::Xor) {
      opcode = "eor";
    }
    lines.push_back(std::string{opcode} + " " + *result_name + ", " +
                    *result_name + ", " + *rhs_name);
    return true;
  }

  if (binary->opcode == bir::BinaryOpcode::Mul) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto result_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto rhs_name =
        operand_view.has_value() ? gp_register_name(scratch_index, *operand_view)
                                 : std::nullopt;
    if (!operand_view.has_value() || !result_name.has_value() ||
        !rhs_name.has_value()) {
      return false;
    }
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    if (rhs.kind == bir::Value::Kind::Immediate && rhs.immediate >= 0) {
      if (const auto shift =
              power_of_two_shift(static_cast<std::uint64_t>(rhs.immediate))) {
        if (!emit_value_publication_to_register(context,
                                                lhs,
                                                before_instruction_index,
                                                target_index,
                                                scratch_index,
                                                lines,
                                                reload_current_memory_loads)) {
          return false;
        }
        if (*shift != 0U) {
          lines.push_back("lsl " + *result_name + ", " + *result_name +
                          ", #" + std::to_string(*shift));
        }
        return true;
      }
    }
    const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
    const bool rhs_reads_target = value_publication_may_read_register_index(
        context, rhs, before_instruction_index, target_index);
    const bool lhs_reads_scratch = value_publication_may_read_register_index(
        context, lhs, before_instruction_index, scratch_index);
    if (rhs_reads_target && !lhs_reads_scratch) {
      if (!emit_value_publication_to_register(context,
                                              rhs,
                                              before_instruction_index,
                                              scratch_index,
                                              nested_scratch_index,
                                              lines,
                                              reload_current_memory_loads) ||
          !emit_value_publication_to_register(context,
                                              lhs,
                                              before_instruction_index,
                                              target_index,
                                              scratch_index,
                                              lines,
                                              reload_current_memory_loads)) {
        return false;
      }
    } else if (!emit_value_publication_to_register(context,
                                                   lhs,
                                                   before_instruction_index,
                                                   target_index,
                                                   scratch_index,
                                                   lines,
                                                   reload_current_memory_loads) ||
               !emit_value_publication_to_register(context,
                                                   rhs,
                                                   before_instruction_index,
                                                   scratch_index,
                                                   nested_scratch_index,
                                                   lines,
                                                   reload_current_memory_loads)) {
      return false;
    }
    lines.push_back("mul " + *result_name + ", " + *result_name + ", " + *rhs_name);
    return true;
  }

  if (binary->opcode != bir::BinaryOpcode::Add &&
      binary->opcode != bir::BinaryOpcode::Sub) {
    return false;
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  const std::uint8_t nested_scratch_index = scratch_index == 9 ? 10 : 9;
  const bool rhs_reads_target = value_publication_may_read_register_index(
      context, rhs, before_instruction_index, target_index);
  const bool lhs_reads_scratch = value_publication_may_read_register_index(
      context, lhs, before_instruction_index, scratch_index);
  if (rhs_reads_target && !lhs_reads_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            before_instruction_index,
                                            target_index,
                                            scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
  } else {
    if (!emit_value_publication_to_register(
            context,
            lhs,
            before_instruction_index,
            target_index,
            scratch_index,
            lines,
            reload_current_memory_loads) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            before_instruction_index,
                                            scratch_index,
                                            nested_scratch_index,
                                            lines,
                                            reload_current_memory_loads)) {
      return false;
    }
  }
  const auto result = gp_register_name(target_index, abi::RegisterView::X);
  const auto rhs_name = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!result.has_value() || !rhs_name.has_value()) {
    return false;
  }
  lines.push_back(std::string{binary->opcode == bir::BinaryOpcode::Add ? "add " : "sub "} +
                  *result + ", " + *result + ", " + *rhs_name);
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->opcode != bir::BinaryOpcode::Mul ||
      binary->result.kind != bir::Value::Kind::Named ||
      binary->result.name.empty()) {
    return std::nullopt;
  }
  if (binary->lhs.kind != bir::Value::Kind::Immediate &&
      binary->rhs.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }
  const auto result_view = scalar_view_for_type(binary->result.type);
  const auto operand_view = scalar_view_for_type(binary->operand_type);
  if (!result_view.has_value() || !operand_view.has_value() ||
      result_view != operand_view) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, binary->result);
  std::optional<std::size_t> result_stack_offset_bytes;
  if (!result_register.has_value()) {
    const auto value_name = prepared_named_value_id(context, binary->result);
    const auto* home =
        value_name.has_value() && context.function.value_locations != nullptr
            ? prepare::find_prepared_value_home(*context.function.value_locations,
                                                *value_name)
            : nullptr;
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::StackSlot ||
        !home->offset_bytes.has_value() ||
        scratches.empty()) {
      return std::nullopt;
    }
    const auto scratch = abi::gp_register(scratches.front().index, *result_view);
    if (!scratch.has_value()) {
      return std::nullopt;
    }
    result_register = RegisterOperand{
        .reg = *scratch,
        .role = RegisterOperandRole::SpillAuthority,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = result_view,
    };
    result_stack_offset_bytes = *home->offset_bytes;
  }
  if (!abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }
  const auto result_name =
      gp_register_name(result_register->reg.index, *result_view);
  if (!result_name.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> rhs_scratch_index;
  std::optional<std::uint8_t> nested_scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index) {
      rhs_scratch_index = scratch.index;
      break;
    }
  }
  if (!rhs_scratch_index.has_value()) {
    return std::nullopt;
  }
  for (const auto& scratch : scratches) {
    if (scratch.index != result_register->reg.index &&
        scratch.index != *rhs_scratch_index) {
      nested_scratch_index = scratch.index;
      break;
    }
  }
  if (!nested_scratch_index.has_value()) {
    nested_scratch_index = result_register->reg.index;
  }
  const auto rhs_name = gp_register_name(*rhs_scratch_index, *operand_view);
  if (!rhs_name.has_value()) {
    return std::nullopt;
  }

  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  std::vector<std::string> lines;
  const auto emit_shifted_power_of_two_operand =
      [&](const bir::Value& value, std::uint64_t scale) -> bool {
    const auto shift = power_of_two_shift(scale);
    if (!shift.has_value() ||
        !emit_value_publication_to_register(context,
                                            value,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return false;
    }
    if (*shift != 0U) {
      lines.push_back("lsl " + *result_name + ", " + *result_name +
                      ", #" + std::to_string(*shift));
    }
    return true;
  };
  bool emitted_power_of_two_scale = false;
  if (rhs.kind == bir::Value::Kind::Immediate && rhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(lhs,
                                          static_cast<std::uint64_t>(rhs.immediate));
  } else if (lhs.kind == bir::Value::Kind::Immediate && lhs.immediate >= 0) {
    emitted_power_of_two_scale =
        emit_shifted_power_of_two_operand(rhs,
                                          static_cast<std::uint64_t>(lhs.immediate));
  }
  if (emitted_power_of_two_scale) {
    if (result_stack_offset_bytes.has_value()) {
      lines.push_back("str " + *result_name + ", " +
                      frame_slot_address(*result_stack_offset_bytes));
    }
    std::string asm_text;
    for (std::size_t index = 0; index < lines.size(); ++index) {
      if (index != 0) {
        asm_text += '\n';
      }
      asm_text += lines[index];
    }
    InstructionRecord target{
        .family = InstructionFamily::Assembler,
        .surface = RecordSurfaceKind::MachineInstructionNode,
        .opcode = MachineOpcode::Unspecified,
        .selection =
            MachineNodeStatusRecord{
                .status = MachineNodeSelectionStatus::Selected,
            },
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .block_index = context.block_index,
        .instruction_index = instruction_index,
        .payload =
            AssemblerInstructionRecord{
                .has_inline_asm_payload = true,
                .side_effects = false,
                .inline_asm_template = std::move(asm_text),
            },
    };
    record_emitted_scalar_register(scalar_state,
                                   result_register->value_name,
                                   *result_register);
    return module::MachineInstruction{
        .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
        .operands = {},
        .target = std::move(target),
        .origin =
            c4c::backend::mir::MachineOrigin{
                .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
                .function_name = context.function.control_flow != nullptr
                                     ? context.function.control_flow->function_name
                                     : c4c::kInvalidFunctionName,
                .block_label = context.control_flow_block != nullptr
                                   ? context.control_flow_block->block_label
                                   : c4c::kInvalidBlockLabel,
                .instruction_index = instruction_index,
            },
    };
  }
  const bool rhs_reads_result = value_publication_may_read_register_index(
      context, rhs, instruction_index, result_register->reg.index);
  const bool lhs_reads_rhs_scratch = value_publication_may_read_register_index(
      context, lhs, instruction_index, *rhs_scratch_index);

  if (rhs_reads_result && !lhs_reads_rhs_scratch) {
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            lhs,
                                            instruction_index,
                                            result_register->reg.index,
                                            *rhs_scratch_index,
                                            lines) ||
        !emit_value_publication_to_register(context,
                                            rhs,
                                            instruction_index,
                                            *rhs_scratch_index,
                                            *nested_scratch_index,
                                            lines)) {
      return std::nullopt;
    }
  }
  lines.push_back("mul " + *result_name + ", " + *result_name + ", " + *rhs_name);
  if (result_stack_offset_bytes.has_value()) {
    lines.push_back("str " + *result_name + ", " +
                    frame_slot_address(*result_stack_offset_bytes));
  }

  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = false,
              .inline_asm_template = std::move(asm_text),
          },
  };
  record_emitted_scalar_register(scalar_state,
                                 result_register->value_name,
                                 *result_register);
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_value_load_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* load = std::get_if<bir::LoadLocalInst>(&inst);
  if (load == nullptr || context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value()) {
    return std::nullopt;
  }
  const auto* pointer_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *access->address.pointer_value_name);
  if (pointer_home == nullptr ||
      pointer_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !pointer_home->offset_bytes.has_value()) {
    return std::nullopt;
  }

  const auto result_name = prepared_named_value_id(context, load->result);
  if (!result_name.has_value()) {
    return std::nullopt;
  }
  const auto* result_home =
      prepare::find_prepared_value_home(*context.function.value_locations, *result_name);
  if (result_home == nullptr) {
    return std::nullopt;
  }
  const auto target_view = scalar_view_for_type(load->result.type);
  if (!target_view.has_value()) {
    return std::nullopt;
  }

  std::optional<std::uint8_t> target_index;
  if (result_home->kind == prepare::PreparedValueHomeKind::Register &&
      result_home->register_name.has_value()) {
    const auto parsed = abi::parse_aarch64_register_name(*result_home->register_name);
    if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
      return std::nullopt;
    }
    target_index = parsed->index;
  } else if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot &&
             result_home->offset_bytes.has_value()) {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      return std::nullopt;
    }
    target_index = scratches.front().index;
  } else {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != *target_index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_prepared_pointer_value_load_to_register(
          context, *load, instruction_index, *target_index, *scratch_index, lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  if (result_home->kind == prepare::PreparedValueHomeKind::StackSlot) {
    const auto mnemonic = scalar_store_mnemonic(load->result.type);
    const auto target = gp_register_name(*target_index, *target_view);
    if (!mnemonic.has_value() || !target.has_value() ||
        !result_home->offset_bytes.has_value()) {
      return std::nullopt;
    }
    lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                    frame_slot_address(*result_home->offset_bytes));
  } else if (result_home->kind == prepare::PreparedValueHomeKind::Register) {
    const auto reg = abi::gp_register(*target_index, *target_view);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    RegisterOperand emitted{
        .reg = *reg,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = result_home->value_id,
        .value_name = result_home->value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = target_view,
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  }

  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::BlockLoweringContext> block_context_for_label(
    const module::BlockLoweringContext& context,
    c4c::BlockLabelId label) {
  if (context.function.control_flow == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = 0; index < context.function.control_flow->blocks.size(); ++index) {
    const auto& block = context.function.control_flow->blocks[index];
    if (block.block_label == label) {
      return module::BlockLoweringContext{
          .function = context.function,
          .control_flow_block = &block,
          .bir_block = find_bir_block(context.function, block),
          .block_index = index,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<module::BlockLoweringContext> unique_branch_predecessor_context(
    const module::BlockLoweringContext& context) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return std::nullopt;
  }
  const prepare::PreparedControlFlowBlock* predecessor = nullptr;
  std::size_t predecessor_index = 0;
  for (std::size_t index = 0; index < context.function.control_flow->blocks.size(); ++index) {
    const auto& candidate = context.function.control_flow->blocks[index];
    if (candidate.terminator_kind == bir::TerminatorKind::Branch &&
        candidate.branch_target_label == context.control_flow_block->block_label) {
      if (predecessor != nullptr) {
        return std::nullopt;
      }
      predecessor = &candidate;
      predecessor_index = index;
    }
  }
  if (predecessor == nullptr) {
    return std::nullopt;
  }
  return module::BlockLoweringContext{
      .function = context.function,
      .control_flow_block = predecessor,
      .bir_block = find_bir_block(context.function, *predecessor),
      .block_index = predecessor_index,
  };
}

struct EdgeProducerContext {
  module::BlockLoweringContext context;
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
};

[[nodiscard]] std::optional<EdgeProducerContext> find_edge_named_producer(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    std::string_view value_name,
    std::size_t successor_before_instruction_index) {
  if (value_name.empty()) {
    return std::nullopt;
  }
  const auto try_context =
      [&](const module::BlockLoweringContext& candidate,
          std::size_t before_instruction_index) -> std::optional<EdgeProducerContext> {
    const auto* producer =
        find_same_block_named_producer(candidate, value_name, before_instruction_index);
    const auto index = producer_instruction_index(candidate, producer);
    if (producer == nullptr || !index.has_value()) {
      return std::nullopt;
    }
    return EdgeProducerContext{
        .context = candidate,
        .producer = producer,
        .instruction_index = *index,
    };
  };

  if (auto producer = try_context(successor_context, successor_before_instruction_index)) {
    return producer;
  }
  if (edge_context.bir_block != nullptr) {
    if (auto producer = try_context(edge_context, edge_context.bir_block->insts.size())) {
      return producer;
    }
  }

  auto cursor = edge_context;
  for (unsigned depth = 0; depth < 4U; ++depth) {
    auto predecessor = unique_branch_predecessor_context(cursor);
    if (!predecessor.has_value() || predecessor->bir_block == nullptr) {
      return std::nullopt;
    }
    if (auto producer = try_context(*predecessor, predecessor->bir_block->insts.size())) {
      return producer;
    }
    cursor = *predecessor;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedMemoryAccess* prepared_memory_access(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return nullptr;
  }
  const auto* addressing =
      prepare::find_prepared_addressing(*context.function.prepared,
                                        context.function.control_flow->function_name);
  return addressing != nullptr
             ? prepare::find_prepared_memory_access(
                   *addressing, context.control_flow_block->block_label, instruction_index)
             : nullptr;
}

[[nodiscard]] bool prepared_memory_access_matches_instruction(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMemoryAccess* access,
    const bir::Inst& inst) {
  if (access == nullptr) {
    return false;
  }
  return std::visit(
      [&](const auto& op) {
        using T = std::decay_t<decltype(op)>;
        if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                      std::is_same_v<T, bir::LoadGlobalInst>) {
          const auto result_name = prepared_named_value_id(context, op.result);
          return result_name.has_value() &&
                 access->result_value_name.has_value() &&
                 *access->result_value_name == *result_name;
        } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                             std::is_same_v<T, bir::StoreGlobalInst>) {
          if (op.value.kind != bir::Value::Kind::Named) {
            return !access->stored_value_name.has_value();
          }
          const auto stored_name = prepared_named_value_id(context, op.value);
          return stored_name.has_value() &&
                 access->stored_value_name.has_value() &&
                 *access->stored_value_name == *stored_name;
        }
        return false;
      },
      inst);
}

[[nodiscard]] bool emit_edge_value_publication_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines);

[[nodiscard]] bool edge_value_publication_may_read_register_index(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t register_index,
    unsigned depth = 0) {
  if (depth > 64U || value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  const auto producer = find_edge_named_producer(
      edge_context, successor_context, value.name, successor_before_instruction_index);
  if (!producer.has_value() || producer->producer == nullptr) {
    const auto* home = prepared_value_home_for_value(successor_context, value);
    return home != nullptr &&
           prepared_value_home_reads_register_index(*home, register_index);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer->producer);
      cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    return edge_value_publication_may_read_register_index(edge_context,
                                                          successor_context,
                                                          operand,
                                                          producer->instruction_index,
                                                          register_index,
                                                          depth + 1);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(producer->producer);
      binary != nullptr) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    return edge_value_publication_may_read_register_index(edge_context,
                                                          successor_context,
                                                          lhs,
                                                          producer->instruction_index,
                                                          register_index,
                                                          depth + 1) ||
           edge_value_publication_may_read_register_index(edge_context,
                                                          successor_context,
                                                          rhs,
                                                          producer->instruction_index,
                                                          register_index,
                                                          depth + 1);
  }
  if (const auto* select = std::get_if<bir::SelectInst>(producer->producer);
      select != nullptr) {
    return edge_value_publication_may_read_register_index(edge_context,
                                                          successor_context,
                                                          select->true_value,
                                                          producer->instruction_index,
                                                          register_index,
                                                          depth + 1) ||
           edge_value_publication_may_read_register_index(edge_context,
                                                          successor_context,
                                                          select->false_value,
                                                          producer->instruction_index,
                                                          register_index,
                                                          depth + 1);
  }
  return false;
}

[[nodiscard]] bool emit_edge_load_local_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const EdgeProducerContext& producer,
    const bir::LoadLocalInst& load,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  const auto mnemonic = scalar_load_mnemonic(load.result.type);
  const auto target_view = scalar_view_for_type(load.result.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!mnemonic.has_value() || !target.has_value()) {
    return false;
  }
  if (emit_prepared_va_list_field_load_to_register(
          producer.context, load, target_index, lines)) {
    return true;
  }
  const auto* access = prepared_memory_access(producer.context, producer.instruction_index);
  if (access == nullptr) {
    return emit_value_publication_to_register(producer.context,
                                              load.result,
                                              producer.instruction_index + 1,
                                              target_index,
                                              scratch_index,
                                              lines);
  }
  if (access->address.base_kind == prepare::PreparedAddressBaseKind::FrameSlot &&
      access->address.can_use_base_plus_offset &&
      access->address.frame_slot_id.has_value()) {
    const auto* slot =
        find_frame_slot(producer.context.function.prepared->stack_layout,
                        *access->address.frame_slot_id);
    if (slot == nullptr) {
      return false;
    }
    lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                    frame_slot_address(slot->offset_bytes +
                                       static_cast<std::size_t>(
                                           access->address.byte_offset)));
    return true;
  }
  if (access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset) {
    return false;
  }
  const auto pointer_name = prepare::prepared_value_name(
      producer.context.function.prepared->names, *access->address.pointer_value_name);
  if (pointer_name.empty()) {
    return false;
  }
  const auto address = gp_register_name(scratch_index, abi::RegisterView::X);
  if (!address.has_value()) {
    return false;
  }
  const auto nested_scratch = scratch_index == 9 ? 10 : 9;
  if (!emit_edge_value_publication_to_register(
          edge_context,
          successor_context,
          bir::Value::named(bir::TypeKind::Ptr, std::string{pointer_name}),
          successor_before_instruction_index,
          scratch_index,
          nested_scratch,
          lines)) {
    return false;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  register_indirect_address(*address,
                                            static_cast<std::size_t>(
                                                access->address.byte_offset)));
  return true;
}

[[nodiscard]] bool emit_edge_value_publication_to_register(
    const module::BlockLoweringContext& edge_context,
    const module::BlockLoweringContext& successor_context,
    const bir::Value& value,
    std::size_t successor_before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::vector<std::string>& lines) {
  const auto target_view = scalar_view_for_type(value.type);
  const auto target =
      target_view.has_value() ? gp_register_name(target_index, *target_view) : std::nullopt;
  if (!target_view.has_value() || !target.has_value()) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto producer = find_edge_named_producer(
      edge_context, successor_context, value.name, successor_before_instruction_index);
  if (!producer.has_value() || producer->producer == nullptr) {
    const auto* home = prepared_value_home_for_value(successor_context, value);
    return home != nullptr &&
           emit_prepared_value_home_to_register(
               successor_context.function.prepared != nullptr
                   ? &successor_context.function.prepared->stack_layout
                   : nullptr,
               *home,
               value.type,
               target_index,
               lines);
  }
  if (const auto* load = std::get_if<bir::LoadLocalInst>(producer->producer);
      load != nullptr) {
    std::vector<std::string> load_lines;
    if (emit_edge_load_local_to_register(edge_context,
                                         successor_context,
                                         *producer,
                                         *load,
                                         successor_before_instruction_index,
                                         target_index,
                                         scratch_index,
                                         load_lines)) {
      lines.insert(lines.end(), load_lines.begin(), load_lines.end());
      return true;
    }
    const auto* home = prepared_value_home_for_value(successor_context, value);
    return home != nullptr &&
           emit_prepared_value_home_to_register(
               successor_context.function.prepared != nullptr
                   ? &successor_context.function.prepared->stack_layout
                   : nullptr,
               *home,
               value.type,
               target_index,
               lines);
  }
  if (const auto* cast = std::get_if<bir::CastInst>(producer->producer);
      cast != nullptr) {
    auto operand = cast->operand;
    operand.type = cast->operand.type;
    if (!emit_edge_value_publication_to_register(edge_context,
                                                 successor_context,
                                                 operand,
                                                 producer->instruction_index,
                                                 target_index,
                                                 scratch_index,
                                                 lines)) {
      return false;
    }
    const auto source_bits = integer_bit_width(cast->operand.type);
    const auto result_bits = integer_bit_width(cast->result.type);
    const auto source_name = gp_register_name(target_index, abi::RegisterView::W);
    const auto target_name = gp_register_name(target_index, *target_view);
    if (!source_bits.has_value() || !result_bits.has_value() ||
        !source_name.has_value() || !target_name.has_value()) {
      return false;
    }
    if (cast->opcode == bir::CastOpcode::SExt) {
      if (*source_bits == 8U) {
        lines.push_back("sxtb " + *target_name + ", " + *source_name);
      } else if (*source_bits == 16U) {
        lines.push_back("sxth " + *target_name + ", " + *source_name);
      } else if (*source_bits == 32U && *result_bits == 64U) {
        lines.push_back("sxtw " + *target_name + ", " + *source_name);
      } else if (*source_bits != *result_bits) {
        return false;
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::ZExt) {
      if (*source_bits < 32U || *source_bits < *result_bits) {
        const auto widened = gp_register_name(target_index, abi::RegisterView::X);
        if (!widened.has_value()) {
          return false;
        }
        if (*source_bits < 32U) {
          lines.push_back("ubfx " + *widened + ", " + *widened +
                          ", #0, #" + std::to_string(*source_bits));
        }
      }
      return true;
    }
    if (cast->opcode == bir::CastOpcode::Trunc) {
      return true;
    }
    return false;
  }
  const auto* binary = std::get_if<bir::BinaryInst>(producer->producer);
  if (binary == nullptr) {
    return emit_value_publication_to_register(producer->context,
                                              value,
                                              producer->instruction_index + 1,
                                              target_index,
                                              scratch_index,
                                              lines);
  }
  auto lhs = binary->lhs;
  lhs.type = binary->operand_type;
  auto rhs = binary->rhs;
  rhs.type = binary->operand_type;
  if (const auto condition = branch_condition_suffix(binary->opcode);
      condition.has_value()) {
    const auto operand_view = scalar_view_for_type(binary->operand_type);
    const auto lhs_name =
        operand_view.has_value() ? gp_register_name(target_index, *operand_view)
                                 : std::nullopt;
    const auto result_name = gp_register_name(target_index, *target_view);
    if (!operand_view.has_value() || !lhs_name.has_value() ||
        !result_name.has_value()) {
      return false;
    }
    if (!emit_edge_value_publication_to_register(edge_context,
                                                 successor_context,
                                                 lhs,
                                                 producer->instruction_index,
                                                 target_index,
                                                 scratch_index,
                                                 lines)) {
      return false;
    }
    std::optional<std::string> rhs_name;
    if (rhs.kind == bir::Value::Kind::Immediate && is_cmp_immediate_encodable(rhs.immediate)) {
      rhs_name = "#" + std::to_string(rhs.immediate);
    } else {
      rhs_name = gp_register_name(scratch_index, *operand_view);
      const auto nested_scratch = scratch_index == 9 ? 10 : 9;
      if (!rhs_name.has_value() ||
          !emit_edge_value_publication_to_register(edge_context,
                                                   successor_context,
                                                   rhs,
                                                   producer->instruction_index,
                                                   scratch_index,
                                                   nested_scratch,
                                                   lines)) {
        return false;
      }
    }
    lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
    lines.push_back("cset " + *result_name + ", " + std::string{*condition});
    return true;
  }
  if (binary->opcode == bir::BinaryOpcode::Add ||
      binary->opcode == bir::BinaryOpcode::Sub) {
    auto lhs = binary->lhs;
    lhs.type = binary->operand_type;
    auto rhs = binary->rhs;
    rhs.type = binary->operand_type;
    const std::uint8_t nested_scratch = scratch_index == 9 ? 10 : 9;
    const bool rhs_reads_target = edge_value_publication_may_read_register_index(
        edge_context, successor_context, rhs, producer->instruction_index, target_index);
    const bool lhs_reads_scratch = edge_value_publication_may_read_register_index(
        edge_context, successor_context, lhs, producer->instruction_index, scratch_index);
    if (rhs_reads_target && !lhs_reads_scratch) {
      if (!emit_edge_value_publication_to_register(edge_context,
                                                   successor_context,
                                                   rhs,
                                                   producer->instruction_index,
                                                   scratch_index,
                                                   nested_scratch,
                                                   lines) ||
          !emit_edge_value_publication_to_register(edge_context,
                                                   successor_context,
                                                   lhs,
                                                   producer->instruction_index,
                                                   target_index,
                                                   scratch_index,
                                                   lines)) {
        return false;
      }
    } else {
      if (!emit_edge_value_publication_to_register(edge_context,
                                                   successor_context,
                                                   lhs,
                                                   producer->instruction_index,
                                                   target_index,
                                                   scratch_index,
                                                   lines) ||
          !emit_edge_value_publication_to_register(edge_context,
                                                   successor_context,
                                                   rhs,
                                                   producer->instruction_index,
                                                   scratch_index,
                                                   nested_scratch,
                                                   lines)) {
        return false;
      }
    }
    const auto result = gp_register_name(target_index, abi::RegisterView::X);
    const auto rhs_name = gp_register_name(scratch_index, abi::RegisterView::X);
    if (!result.has_value() || !rhs_name.has_value()) {
      return false;
    }
    lines.push_back(std::string{binary->opcode == bir::BinaryOpcode::Add
                                    ? "add "
                                    : "sub "} +
                    *result + ", " + *result + ", " + *rhs_name);
    return true;
  }
  return emit_value_publication_to_register(producer->context,
                                            value,
                                            producer->instruction_index + 1,
                                            target_index,
                                            scratch_index,
                                            lines);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_predecessor_join_source_publication(
    const module::BlockLoweringContext& context,
    const bir::Block& successor,
    std::size_t source_index,
    const prepare::PreparedValueHome& source_home,
    const prepare::PreparedValueHome& destination_home,
    BlockScalarLoweringState& scalar_state) {
  if (source_index >= successor.insts.size() ||
      source_home.value_name == c4c::kInvalidValueName ||
      destination_home.kind != prepare::PreparedValueHomeKind::Register ||
      !destination_home.register_name.has_value()) {
    return std::nullopt;
  }
  const auto source_value = instruction_result_value(successor.insts[source_index]);
  if (!source_value.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*destination_home.register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  std::uint8_t scratch_index = scratches.front().index;
  for (const auto scratch : scratches) {
    if (scratch.index != parsed->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (scratch_index == parsed->index) {
    return std::nullopt;
  }
  auto successor_context = context;
  successor_context.bir_block = &successor;
  std::vector<std::string> lines;
  if (!emit_edge_value_publication_to_register(context,
                                               successor_context,
                                               *source_value,
                                               source_index + 1,
                                               parsed->index,
                                               scratch_index,
                                               lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  const auto expected_view =
      scalar_view_for_type(source_value->type).value_or(parsed->view);
  auto source_reg = abi::gp_register(parsed->index, expected_view).value_or(*parsed);
  RegisterOperand emitted{
      .reg = source_reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = source_home.value_id,
      .value_name = source_home.value_name,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  RegisterOperand destination = emitted;
  destination.value_id = destination_home.value_id;
  destination.value_name = destination_home.value_name;
  record_emitted_scalar_register(scalar_state, destination.value_name, destination);
  return make_select_chain_materialization_instruction(
      context, context.bir_block != nullptr ? context.bir_block->insts.size() : 0U,
      std::move(lines));
}

[[nodiscard]] std::string select_chain_label(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    c4c::ValueNameId root_value_name,
    std::uint8_t target_index,
    std::size_t label_index,
    std::string_view suffix) {
  const auto function_name = context.function.control_flow != nullptr
                                 ? context.function.control_flow->function_name
                                 : c4c::kInvalidFunctionName;
  const auto block_label = context.control_flow_block != nullptr
                               ? context.control_flow_block->block_label
                               : c4c::kInvalidBlockLabel;
  return ".Lselect_mat_" + std::to_string(function_name) + "_" +
         std::to_string(block_label) + "_" + std::to_string(instruction_index) +
         "_" + std::to_string(root_value_name) + "_" + std::to_string(target_index) +
         "_" + std::to_string(label_index) + "_" + std::string{suffix};
}

[[nodiscard]] bool emit_select_chain_value_to_register(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    std::uint8_t scratch_index,
    std::size_t root_instruction_index,
    c4c::ValueNameId root_value_name,
    std::vector<std::string>& lines,
    std::size_t& label_index,
    std::vector<std::string_view>& active_values,
    bool reload_current_memory_loads) {
  if (value.kind == bir::Value::Kind::Immediate) {
    const auto target_view = scalar_view_for_type(value.type);
    const auto target = target_view.has_value()
                            ? gp_register_name(target_index, *target_view)
                            : std::nullopt;
    if (!target.has_value()) {
      return false;
    }
    lines.push_back("mov " + *target + ", #" + std::to_string(value.immediate));
    return true;
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return false;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }

  const auto producer =
      find_same_block_select_producer(context, value, before_instruction_index);
  if (producer.select == nullptr) {
    return emit_value_publication_to_register(
        context,
        value,
        before_instruction_index,
        target_index,
        scratch_index,
        lines,
        reload_current_memory_loads);
  }

  const auto condition = branch_condition_suffix(producer.select->predicate);
  const auto compare_view = scalar_view_for_type(producer.select->compare_type);
  if (!condition.has_value() || !compare_view.has_value()) {
    return false;
  }

  active_values.push_back(value.name);
  const auto current_label = label_index++;
  const auto true_label =
      select_chain_label(
          context, root_instruction_index, root_value_name, target_index, current_label, "true");
  const auto end_label =
      select_chain_label(
          context, root_instruction_index, root_value_name, target_index, current_label, "end");

  const auto lhs_name = gp_register_name(target_index, *compare_view);
  if (!lhs_name.has_value() ||
      !emit_value_publication_to_register(context,
                                          producer.select->lhs,
                                          producer.instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines,
                                          reload_current_memory_loads)) {
    active_values.pop_back();
    return false;
  }
  std::optional<std::string> rhs_name;
  if (producer.select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(producer.select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(producer.select->rhs.immediate);
  } else {
    rhs_name = gp_register_name(scratch_index, *compare_view);
    if (!rhs_name.has_value() ||
        !emit_value_publication_to_register(context,
                                            producer.select->rhs,
                                            producer.instruction_index,
                                            scratch_index,
                                            target_index,
                                            lines,
                                            reload_current_memory_loads)) {
      active_values.pop_back();
      return false;
    }
  }
  lines.push_back("cmp " + *lhs_name + ", " + *rhs_name);
  lines.push_back("b." + std::string{*condition} + " " + true_label);

  if (!emit_select_chain_value_to_register(context,
                                           producer.select->false_value,
                                           producer.instruction_index,
                                           target_index,
                                           scratch_index,
                                           root_instruction_index,
                                           root_value_name,
                                           lines,
                                           label_index,
                                           active_values,
                                           reload_current_memory_loads)) {
    active_values.pop_back();
    return false;
  }
  lines.push_back("b " + end_label);
  lines.push_back(true_label + ":");
  if (!emit_select_chain_value_to_register(context,
                                           producer.select->true_value,
                                           producer.instruction_index,
                                           target_index,
                                           scratch_index,
                                           root_instruction_index,
                                           root_value_name,
                                           lines,
                                           label_index,
                                           active_values,
                                           reload_current_memory_loads)) {
    active_values.pop_back();
    return false;
  }
  lines.push_back(end_label + ":");
  active_values.pop_back();
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_select_chain_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
make_load_global_got_materialization_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load_global,
    BlockScalarLoweringState& scalar_state) {
  const bir::Global* target_global = find_load_global_target(context, load_global);
  if (target_global == nullptr ||
      target_global->address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, load_global.result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto result_view = scalar_view_for_type(load_global.result.type);
  if (!result_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_result = make_named_prepared_result_register(context, load_global.result);
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const auto result_register =
      prepared_result.has_value()
          ? std::optional<abi::RegisterReference>{prepared_result->reg}
          : abi::gp_register(scratches.front().index, *result_view);
  if (!result_register.has_value() ||
      result_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto address_register =
      abi::gp_register(result_register->index, abi::RegisterView::X);
  const auto target_register =
      abi::gp_register(result_register->index, *result_view);
  if (!address_register.has_value() || !target_register.has_value()) {
    return std::nullopt;
  }
  const std::string address = abi::register_name(*address_register);
  const std::string target = abi::register_name(*target_register);
  const auto symbol_label = load_global_symbol_label(context, load_global, target_global);
  if (symbol_label.empty()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  lines.push_back("adrp " + address + ", :got:" + symbol_label);
  lines.push_back("ldr " + address + ", [" + address + ", :got_lo12:" +
                  symbol_label + "]");
  lines.push_back("ldr " + target + ", " +
                  register_indirect_address(address, load_global.byte_offset));

  if (const auto* home = prepared_value_home_for_value(context, load_global.result);
      home != nullptr &&
      home->kind == prepare::PreparedValueHomeKind::StackSlot &&
      home->offset_bytes.has_value()) {
    lines.push_back("str " + target + ", " + frame_slot_address(*home->offset_bytes));
    return make_select_chain_materialization_instruction(
        context, instruction_index, std::move(lines));
  }

  RegisterOperand emitted{
      .reg = *target_register,
      .role = prepared_result.has_value() ? prepared_result->role
                                          : RegisterOperandRole::StoragePlan,
      .value_name = *value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = result_view,
  };
  record_emitted_scalar_register(scalar_state, *value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_direct_global_select_chain_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value() ||
      !select_chain_contains_direct_global_load(context,
                                                value,
                                                before_instruction_index)) {
    return std::nullopt;
  }
  if (context.function.value_locations != nullptr) {
    const auto* home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          *value_name);
    if (home != nullptr && home->kind == prepare::PreparedValueHomeKind::StackSlot) {
      return std::nullopt;
    }
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  if (context.function.value_locations != nullptr &&
      (value.type == bir::TypeKind::F32 || value.type == bir::TypeKind::F64)) {
    const auto* home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          *value_name);
    if (home == nullptr ||
        home->kind != prepare::PreparedValueHomeKind::Register ||
        !home->register_name.has_value()) {
      return std::nullopt;
    }
    const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
    if (!parsed.has_value() || !abi::is_fp_simd_register(*parsed)) {
      return std::nullopt;
    }
    const auto result_register = scalar_fp_register_view(*parsed, value.type);
    if (!result_register.has_value()) {
      return std::nullopt;
    }
    std::vector<std::string> lines;
    if (!emit_fp_value_to_register(context,
                                   value,
                                   before_instruction_index,
                                   *result_register,
                                   scratches.front().index,
                                   lines) ||
        lines.empty()) {
      return std::nullopt;
    }
    RegisterOperand emitted{
        .reg = *result_register,
        .role = RegisterOperandRole::StoragePlan,
        .value_id = home->value_id,
        .value_name = *value_name,
        .prepared_bank = prepare::PreparedRegisterBank::Fpr,
        .expected_view = result_register->view,
    };
    record_emitted_scalar_register(scalar_state, *value_name, emitted);
    return make_select_chain_materialization_instruction(
        context, before_instruction_index, std::move(lines));
  }
  const auto result_view = scalar_view_for_type(value.type);
  const auto result_register =
      result_view.has_value()
          ? abi::gp_register(scratches[0].index, *result_view)
          : std::nullopt;
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  std::size_t label_index = 0;
  std::vector<std::string_view> active_values;
  if (!emit_select_chain_value_to_register(context,
                                           value,
                                           before_instruction_index,
                                           scratches[0].index,
                                           scratches[1].index,
                                           before_instruction_index,
                                           *value_name,
                                           lines,
                                           label_index,
                                           active_values) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_name = *value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = result_view,
  };
  record_emitted_scalar_register(scalar_state, *value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, before_instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_global_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    std::unordered_set<c4c::ValueNameId>* published_stack_values = nullptr,
    bool stack_homes_only = false) {
  const auto* store = std::get_if<bir::StoreGlobalInst>(&inst);
  if (store == nullptr || store->value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto store_value_name = prepared_named_value_id(context, store->value);
  if (store_value_name.has_value() && published_stack_values != nullptr &&
      published_stack_values->find(*store_value_name) != published_stack_values->end()) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value() ||
      (memory_record->value->kind != OperandKind::Register &&
       memory_record->value->kind != OperandKind::Memory)) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store->value;
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (stack_homes_only) {
      return std::nullopt;
    }
    if (abi::is_reserved_mir_scratch(target_register->reg) || scratches.empty()) {
      return std::nullopt;
    }
    if (abi::is_gp_register(target_register->reg)) {
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   scratches.front().index,
                                                   lines);
    } else if (abi::is_fp_simd_register(target_register->reg)) {
      emitted = emit_fp_value_to_register(context,
                                          value,
                                          instruction_index,
                                          target_register->reg,
                                          scratches.front().index,
                                          lines);
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr) {
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    const auto stack_home = memory_address(*target_memory);
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U || !store_mnemonic.has_value() ||
        !store_register.has_value() || stack_home.empty()) {
      return std::nullopt;
    }
    emitted = emit_value_publication_to_register(context,
                                                 value,
                                                 instruction_index,
                                                 scratches.front().index,
                                                 scratches[1].index,
                                                 lines,
                                                 stack_homes_only);
    if (emitted) {
      lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                      ", " + stack_home);
      published_stack_home = true;
      if (store_value_name.has_value() && published_stack_values != nullptr) {
        published_stack_values->insert(*store_value_name);
      }
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] bool store_local_value_is_byval_frame_slot_load(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  if (load == nullptr) {
    return false;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  const auto* access = producer_index.has_value()
                           ? prepared_memory_access(context, *producer_index)
                           : nullptr;
  return access != nullptr &&
         access->address.base_kind == prepare::PreparedAddressBaseKind::PointerValue &&
         access->address.pointer_value_name.has_value() &&
         access->address.can_use_base_plus_offset &&
         is_byval_formal_value_name(context, *access->address.pointer_value_name);
}

[[nodiscard]] bool store_local_value_is_wide_load_from_narrow_local_store(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer) : nullptr;
  if (load == nullptr) {
    return false;
  }
  const auto producer_index = producer_instruction_index(context, producer);
  return producer_index.has_value() &&
         find_latest_narrow_store_for_wide_local_load(context, *load, *producer_index)
             .has_value();
}

[[nodiscard]] const bir::CastInst* store_local_value_cast_producer(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  return producer != nullptr ? std::get_if<bir::CastInst>(producer) : nullptr;
}

[[nodiscard]] bool store_local_value_has_select_producer(
    const module::BlockLoweringContext& context,
    const bir::StoreLocalInst& store,
    std::size_t instruction_index) {
  if (store.value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer =
      find_same_block_named_producer(context, store.value.name, instruction_index);
  return producer != nullptr && std::get_if<bir::SelectInst>(producer) != nullptr;
}

[[nodiscard]] bool emit_scalar_conversion_cast_to_register(
    const module::BlockLoweringContext& context,
    const bir::CastInst& cast,
    std::size_t cast_instruction_index,
    const RegisterOperand& target_register,
    std::vector<std::string>& lines) {
  const auto gp_scratches = abi::reserved_mir_scratch_gp_registers();
  const auto fp_scratches = abi::reserved_mir_scratch_fp_simd_registers();
  if (gp_scratches.empty() || fp_scratches.empty()) {
    return false;
  }
  const std::uint8_t gp_scratch_index =
      abi::is_gp_register(target_register.reg) &&
              target_register.reg.index == gp_scratches.front().index
          ? gp_scratches.back().index
          : gp_scratches.front().index;
  const std::uint8_t fp_scratch_index =
      abi::is_fp_simd_register(target_register.reg) &&
              target_register.reg.index == fp_scratches.front().index
          ? fp_scratches.back().index
          : fp_scratches.front().index;
  switch (cast.opcode) {
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI: {
      if (!abi::is_gp_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_gp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::FPToSI ? "fcvtzs "
                                                                         : "fcvtzu "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto source_width = scalar_integer_width_bits(cast.operand.type);
      const auto gp_source = source_width.has_value()
                                 ? abi::gp_register(gp_scratch_index,
                                                    *source_width == 64U
                                                        ? abi::RegisterView::X
                                                        : abi::RegisterView::W)
                                 : std::nullopt;
      if (!destination.has_value() || !source_width.has_value() || !gp_source.has_value()) {
        return false;
      }
      if (cast.operand.kind == bir::Value::Kind::Immediate) {
        auto materialize = materialize_integer_constant_lines(
            *gp_source, immediate_integer_bits(cast.operand, *source_width), *source_width);
        if (materialize.empty()) {
          return false;
        }
        lines.insert(lines.end(), materialize.begin(), materialize.end());
      } else if (!emit_value_publication_to_register(context,
                                                     cast.operand,
                                                     cast_instruction_index,
                                                     gp_scratch_index,
                                                     target_register.reg.index,
                                                     lines)) {
        return false;
      }
      lines.push_back(std::string{cast.opcode == bir::CastOpcode::SIToFP ? "scvtf "
                                                                         : "ucvtf "} +
                      std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*gp_source)});
      return true;
    }
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPTrunc: {
      if (!abi::is_fp_simd_register(target_register.reg)) {
        return false;
      }
      const auto destination = scalar_fp_register_view(target_register.reg, cast.result.type);
      const auto fp_source_base = abi::fp_simd_register(fp_scratch_index, abi::RegisterView::D);
      if (!destination.has_value() || !fp_source_base.has_value() ||
          !emit_fp_value_to_register(context,
                                     cast.operand,
                                     cast_instruction_index,
                                     *fp_source_base,
                                     gp_scratch_index,
                                     lines)) {
        return false;
      }
      const auto fp_source = scalar_fp_register_view(*fp_source_base, cast.operand.type);
      if (!fp_source.has_value()) {
        return false;
      }
      lines.push_back("fcvt " + std::string{abi::register_name(*destination)} + ", " +
                      std::string{abi::register_name(*fp_source)});
      return true;
    }
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_store_local_value_publication(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    const module::MachineInstruction& lowered_memory,
    const BlockScalarLoweringState& scalar_state,
    const module::MachineBlock& block) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  const auto* cast_producer =
      store != nullptr ? store_local_value_cast_producer(context, *store, instruction_index)
                       : nullptr;
  if (store == nullptr ||
      (!store_local_value_is_byval_frame_slot_load(context, *store, instruction_index) &&
       !store_local_value_is_wide_load_from_narrow_local_store(
           context, *store, instruction_index) &&
       !store_local_value_has_select_producer(context, *store, instruction_index) &&
       cast_producer == nullptr &&
       !select_chain_contains_direct_global_load(context, store->value, instruction_index))) {
    return std::nullopt;
  }
  const auto* memory_record =
      std::get_if<MemoryInstructionRecord>(&lowered_memory.target.payload);
  if (memory_record == nullptr ||
      memory_record->memory_kind != MemoryInstructionKind::Store ||
      !memory_record->value.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::vector<std::string> lines;
  auto value = store->value;
  const auto* producer_inst =
      value.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(context, value.name, instruction_index)
          : nullptr;
  const auto producer_index = producer_instruction_index(context, producer_inst);
  if (producer_index.has_value() &&
      std::get_if<bir::SelectInst>(producer_inst) != nullptr &&
      !block.instructions.empty() &&
      block.instructions.back().origin.has_value() &&
      block.instructions.back().origin->instruction_index == *producer_index) {
    return std::nullopt;
  }
  bool emitted = false;
  bool published_stack_home = false;
  if (const auto* target_register =
          std::get_if<RegisterOperand>(&memory_record->value->payload);
      target_register != nullptr) {
    if (store_local_uses_pointer_value_address(*store)) {
      const auto value_register =
          prepared_or_emitted_store_value_register(context, value, scalar_state);
      if (value_register.has_value() &&
          register_operands_share_physical_register(*value_register, *target_register)) {
        return std::nullopt;
      }
    }
    if (abi::is_reserved_mir_scratch(target_register->reg)) {
      const auto* value_home = prepared_value_home_for_value(context, value);
      const auto store_mnemonic = scalar_store_mnemonic(value.type);
      const auto store_view = scalar_view_for_type(value.type);
      const auto store_register =
          store_view.has_value()
              ? gp_register_name(target_register->reg.index, *store_view)
              : std::nullopt;
      std::optional<std::uint8_t> alternate_scratch;
      for (const auto& scratch : scratches) {
        if (scratch.index != target_register->reg.index) {
          alternate_scratch = scratch.index;
          break;
        }
      }
      if (!store_local_value_has_select_producer(context, *store, instruction_index) ||
          value_home == nullptr ||
          value_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
          !value_home->offset_bytes.has_value() ||
          !store_mnemonic.has_value() ||
          !store_register.has_value() ||
          !alternate_scratch.has_value()) {
        return std::nullopt;
      }
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   *alternate_scratch,
                                                   lines,
                                                   true);
      if (emitted) {
        lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                        ", " + frame_slot_address(*value_home->offset_bytes));
        published_stack_home = true;
      }
    } else if (cast_producer != nullptr) {
      emitted = emit_scalar_conversion_cast_to_register(
          context,
          *cast_producer,
          producer_instruction_index(context, producer_inst).value_or(instruction_index),
          *target_register,
          lines);
    }
    if (!emitted && abi::is_gp_register(target_register->reg) && !scratches.empty()) {
      lines.clear();
      emitted = emit_value_publication_to_register(context,
                                                   value,
                                                   instruction_index,
                                                   target_register->reg.index,
                                                   scratches.front().index,
                                                   lines);
    }
  } else if (const auto* target_memory =
                 std::get_if<MemoryOperand>(&memory_record->value->payload);
             target_memory != nullptr &&
             (store_local_value_has_select_producer(context, *store, instruction_index) ||
              select_chain_contains_direct_global_load(context, value, instruction_index))) {
    const auto store_mnemonic = scalar_store_mnemonic(value.type);
    const auto store_view = scalar_view_for_type(value.type);
    const auto store_register =
        store_view.has_value() && !scratches.empty()
            ? gp_register_name(scratches.front().index, *store_view)
            : std::nullopt;
    const auto stack_home = memory_address(*target_memory);
    if (target_memory->support != MemoryOperandSupportKind::Prepared ||
        target_memory->base_kind != MemoryBaseKind::FrameSlot ||
        !target_memory->byte_offset_is_prepared_snapshot ||
        !target_memory->can_use_base_plus_offset ||
        scratches.size() < 2U ||
        !store_mnemonic.has_value() ||
        !store_register.has_value() ||
        stack_home.empty()) {
      return std::nullopt;
    }
    emitted = emit_value_publication_to_register(context,
                                                 value,
                                                 instruction_index,
                                                 scratches.front().index,
                                                 scratches[1].index,
                                                 lines);
    if (emitted) {
      lines.push_back(std::string{*store_mnemonic} + " " + *store_register +
                      ", " + stack_home);
      published_stack_home = true;
    }
  }
  if (!emitted || lines.empty()) {
    return std::nullopt;
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = published_stack_home
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead,
                                MachineSideEffectKind::MemoryWrite}
                          : std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::MemoryRead},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_stack_homed_pointer_store_writeback(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr ||
      !store->address.has_value() ||
      store->address->base_kind != bir::MemoryAddress::BaseKind::PointerValue ||
      context.function.value_locations == nullptr) {
    return std::nullopt;
  }
  const auto* access = prepared_memory_access(context, instruction_index);
  if (access == nullptr ||
      access->address.base_kind != prepare::PreparedAddressBaseKind::PointerValue ||
      !access->address.pointer_value_name.has_value() ||
      !access->address.can_use_base_plus_offset ||
      access->address.byte_offset < 0) {
    return std::nullopt;
  }
  const auto* pointer_home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *access->address.pointer_value_name);
  if (pointer_home == nullptr ||
      pointer_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !pointer_home->offset_bytes.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  const auto value_view = scalar_view_for_type(store->value.type);
  const auto store_mnemonic = scalar_store_mnemonic(store->value.type);
  if (scratches.size() < 2U || !value_view.has_value() ||
      !store_mnemonic.has_value()) {
    return std::nullopt;
  }
  const auto value_register = gp_register_name(scratches[0].index, *value_view);
  const auto address_register_ref =
      abi::gp_register(scratches[1].index, abi::RegisterView::X);
  const auto address_register = gp_register_name(scratches[1].index, abi::RegisterView::X);
  if (!value_register.has_value() || !address_register_ref.has_value() ||
      !address_register.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  std::string stored_register = *value_register;
  if (auto prepared_store_value =
          make_named_prepared_result_register(context, store->value);
      prepared_store_value.has_value() &&
      !register_operands_share_physical_register(
          *prepared_store_value,
          RegisterOperand{.reg = *address_register_ref})) {
    const auto source_register = std::string{abi::register_name(prepared_store_value->reg)};
    if (source_register != stored_register) {
      lines.push_back("mov " + stored_register + ", " + source_register);
    }
  } else {
    if (!emit_value_publication_to_register(context,
                                            store->value,
                                            instruction_index,
                                            scratches[0].index,
                                            scratches[1].index,
                                            lines,
                                            true)) {
      return std::nullopt;
    }
  }
  lines.push_back("ldr " + *address_register + ", " +
                  frame_slot_address(*pointer_home->offset_bytes));
  lines.push_back(std::string{*store_mnemonic} + " " + stored_register +
                  ", " + register_indirect_address(
                              *address_register,
                              static_cast<std::size_t>(access->address.byte_offset)));

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects =
          {MachineSideEffectKind::MemoryRead, MachineSideEffectKind::MemoryWrite},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = [&] {
            std::string text;
            for (std::size_t index = 0; index < lines.size(); ++index) {
              if (index != 0) {
                text += '\n';
              }
              text += lines[index];
            }
            return text;
          }(),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] bool is_store_global_select_snapshot_run_instruction(const bir::Inst& inst) {
  return std::get_if<bir::SelectInst>(&inst) != nullptr ||
         std::get_if<bir::LoadGlobalInst>(&inst) != nullptr ||
         std::get_if<bir::BinaryInst>(&inst) != nullptr ||
         std::get_if<bir::CastInst>(&inst) != nullptr ||
         std::get_if<bir::StoreGlobalInst>(&inst) != nullptr;
}

void lower_pending_store_global_stack_value_publications(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::unordered_set<c4c::ValueNameId>& published_stack_values,
    module::MachineBlock& block) {
  if (context.bir_block == nullptr) {
    return;
  }
  for (std::size_t index = instruction_index; index < context.bir_block->insts.size();
       ++index) {
    const auto& candidate = context.bir_block->insts[index];
    if (!is_store_global_select_snapshot_run_instruction(candidate)) {
      break;
    }
    if (std::get_if<bir::StoreGlobalInst>(&candidate) == nullptr) {
      continue;
    }
    module::ModuleLoweringDiagnostics ignored_diagnostics;
    auto lowered_memory =
        lower_memory_instruction(context, candidate, index, ignored_diagnostics);
    if (!lowered_memory.handled || !lowered_memory.instruction.has_value()) {
      continue;
    }
    if (auto publication =
            lower_store_global_value_publication(context,
                                                 candidate,
                                                 index,
                                                 *lowered_memory.instruction,
                                                 &published_stack_values,
                                                 true)) {
      block.instructions.push_back(std::move(*publication));
    }
  }
}

[[nodiscard]] bool lower_store_local_with_address_materialization(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* store = std::get_if<bir::StoreLocalInst>(&inst);
  if (store == nullptr) {
    return false;
  }

  auto materialized = lower_address_materialization(context, instruction_index, diagnostics);
  if (!materialized.has_value()) {
    return false;
  }
  std::optional<RegisterOperand> materialized_address;
  if (const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized->target.payload);
      address_record != nullptr && address_record->result_register.has_value()) {
    materialized_address = *address_record->result_register;
  }

  record_address_materialization_result(scalar_state, *materialized);
  block.instructions.push_back(std::move(*materialized));

  const auto diagnostic_count = diagnostics.entries.size();
  auto lowered_memory =
      lower_memory_instruction(context, inst, instruction_index, diagnostics);
  if (lowered_memory.handled && lowered_memory.instruction.has_value()) {
    if (materialized_address.has_value()) {
      retarget_pointer_store_value_to_materialized_address(
          *lowered_memory.instruction, *materialized_address);
      retarget_store_address_to_materialized_pointer(
          *store, *lowered_memory.instruction, *materialized_address);
    }
    if (auto value_publication =
            lower_store_local_value_publication(context,
                                                inst,
                                                instruction_index,
                                                *lowered_memory.instruction,
                                                scalar_state,
                                                block)) {
      block.instructions.push_back(std::move(*value_publication));
    }
    record_memory_result(scalar_state, *lowered_memory.instruction);
    block.instructions.push_back(std::move(*lowered_memory.instruction));
  } else if (lowered_memory.handled) {
    if (auto pointer_store =
            lower_stack_homed_pointer_store_writeback(context, inst, instruction_index)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*pointer_store));
      return lowered_memory.handled;
    }
    if (auto formal_store =
            lower_fixed_formal_store_local_publication(
                context, inst, instruction_index, scalar_state)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*formal_store));
    }
  }
  return lowered_memory.handled;
}

[[nodiscard]] bool lower_scalar_with_address_materialization(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  if (binary == nullptr ||
      binary->result.kind != bir::Value::Kind::Named ||
      context.bir_block == nullptr) {
    return false;
  }

  auto materialized_addresses =
      lower_address_materializations(context, instruction_index, diagnostics);
  if (materialized_addresses.empty()) {
    return false;
  }

  auto trial_scalar_state = scalar_state;
  for (const auto& materialized : materialized_addresses) {
    record_address_materialization_result(trial_scalar_state, materialized);
  }
  std::optional<module::MachineInstruction> lowered_scalar;
  if (binary->result.type == bir::TypeKind::Ptr &&
      instruction_index + 1 < context.bir_block->insts.size()) {
    const auto* next_store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[instruction_index + 1]);
    if (next_store != nullptr &&
        next_store->value.kind == bir::Value::Kind::Named &&
        next_store->value.type == bir::TypeKind::Ptr &&
        next_store->value.name == binary->result.name) {
      lowered_scalar =
          lower_scalar_instruction(context, inst, instruction_index, trial_scalar_state, diagnostics);
    }
  } else if (bir::is_compare_opcode(binary->opcode)) {
    lowered_scalar = lower_scalar_control_value_instruction(
        context, inst, instruction_index, trial_scalar_state, diagnostics, true);
  }
  if (!lowered_scalar.has_value()) {
    return false;
  }

  scalar_state = std::move(trial_scalar_state);
  for (auto& materialized : materialized_addresses) {
    block.instructions.push_back(std::move(materialized));
  }
  block.instructions.push_back(std::move(*lowered_scalar));
  return true;
}

void append_unsupported_instruction_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = classify_instruction(inst),
      .message =
          "AArch64 block dispatch visited unsupported prepared BIR instruction family",
  });
}

void append_call_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                            module::ModuleLoweringDiagnosticKind kind,
                            const module::BlockLoweringContext& context,
                            std::size_t instruction_index,
                            std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = kind,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_index = instruction_index,
      .instruction_family = module::InstructionLoweringFamily::Call,
      .message = std::move(message),
  });
}

[[nodiscard]] bool is_dynamic_alloca_helper(std::string_view callee) {
  constexpr std::string_view kPrefix = "llvm.dynamic_alloca.";
  return callee.substr(0, kPrefix.size()) == kPrefix;
}

[[nodiscard]] std::optional<prepare::PreparedDynamicStackOpKind>
dynamic_stack_helper_kind(std::string_view callee) {
  if (callee == "llvm.stacksave") {
    return prepare::PreparedDynamicStackOpKind::StackSave;
  }
  if (is_dynamic_alloca_helper(callee)) {
    return prepare::PreparedDynamicStackOpKind::DynamicAlloca;
  }
  if (callee == "llvm.stackrestore") {
    return prepare::PreparedDynamicStackOpKind::StackRestore;
  }
  return std::nullopt;
}

[[nodiscard]] const prepare::PreparedDynamicStackOp* find_dynamic_stack_op(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    prepare::PreparedDynamicStackOpKind kind) {
  if (context.function.dynamic_stack_plan == nullptr) {
    return nullptr;
  }
  const c4c::BlockLabelId block_label =
      context.control_flow_block != nullptr ? context.control_flow_block->block_label
                                            : c4c::kInvalidBlockLabel;
  for (const auto& op : context.function.dynamic_stack_plan->operations) {
    if (op.kind == kind && op.instruction_index == instruction_index &&
        (op.block_label == block_label || op.block_label == c4c::kInvalidBlockLabel ||
         block_label == c4c::kInvalidBlockLabel)) {
      return &op;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    c4c::ValueNameId value_name) {
  if (context.value_locations == nullptr || value_name == c4c::kInvalidValueName) {
    return nullptr;
  }
  for (const auto& home : context.value_locations->value_homes) {
    if (home.value_name == value_name) {
      return &home;
    }
  }
  return nullptr;
}

[[nodiscard]] const prepare::PreparedValueHome* find_dynamic_stack_value_home(
    const module::FunctionLoweringContext& context,
    const std::optional<c4c::ValueNameId>& value_name) {
  if (!value_name.has_value()) {
    return nullptr;
  }
  return find_dynamic_stack_value_home(context, *value_name);
}

[[nodiscard]] std::optional<std::string> dynamic_stack_register_home_name(
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value() || home.register_name->empty()) {
    return std::nullopt;
  }
  return home.register_name;
}

[[nodiscard]] std::optional<std::string> dynamic_stack_stack_home_address(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return std::nullopt;
  }
  if (context.frame_plan == nullptr ||
      !context.frame_plan->uses_frame_pointer_for_fixed_slots) {
    return std::nullopt;
  }
  return std::string{"[x29, #"} + std::to_string(*home.offset_bytes) + "]";
}

[[nodiscard]] bool dynamic_stack_home_requires_stable_frame_pointer(
    const module::FunctionLoweringContext& context,
    const prepare::PreparedValueHome& home) {
  return home.kind == prepare::PreparedValueHomeKind::StackSlot &&
         (context.frame_plan == nullptr ||
          !context.frame_plan->uses_frame_pointer_for_fixed_slots);
}

[[nodiscard]] std::optional<std::size_t> dynamic_stack_element_size_bytes(
    const prepare::PreparedDynamicStackOp& op) {
  if (op.element_size_bytes != 0) {
    return op.element_size_bytes;
  }
  if (op.allocation_type_text == "i8") {
    return std::size_t{1};
  }
  if (op.allocation_type_text == "i16") {
    return std::size_t{2};
  }
  if (op.allocation_type_text == "i32") {
    return std::size_t{4};
  }
  if (op.allocation_type_text == "i64" || op.allocation_type_text == "ptr") {
    return std::size_t{8};
  }
  return std::nullopt;
}

[[nodiscard]] module::MachineInstruction make_bir_machine_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    InstructionRecord target) {
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = instruction_index,
          },
  };
}

[[nodiscard]] module::MachineInstruction make_branch_compare_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  const std::size_t instruction_index =
      context.bir_block != nullptr ? context.bir_block->insts.size() : 0;
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::Selected,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::ControlFlowTransfer},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] InstructionRecord make_dynamic_stack_rejection_record(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view diagnostic) {
  return InstructionRecord{
      .family = InstructionFamily::Frame,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::FrameSetup,
      .selection =
          MachineNodeStatusRecord{
              .status = MachineNodeSelectionStatus::DeferredUnsupported,
              .diagnostic = diagnostic,
          },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload = FrameInstructionRecord{},
  };
}

[[nodiscard]] module::MachineInstruction make_dynamic_stack_assembler_instruction(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }

  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::Selected,
      },
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = instruction_index,
      .side_effects = {MachineSideEffectKind::FrameSetup},
      .payload =
          AssemblerInstructionRecord{
              .has_inline_asm_payload = true,
              .side_effects = true,
              .inline_asm_template = std::move(asm_text),
          },
  };
  return make_bir_machine_instruction(context, instruction_index, std::move(target));
}

[[nodiscard]] std::optional<module::MachineInstruction> make_dynamic_stack_failure(
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    std::string_view message,
    module::ModuleLoweringDiagnostics& diagnostics) {
  append_call_diagnostic(diagnostics,
                         module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
                         context,
                         instruction_index,
                         std::string{message});
  return make_bir_machine_instruction(
      context, instruction_index, make_dynamic_stack_rejection_record(
                                      context, instruction_index, message));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_save(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires a prepared result home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov " + *reg + ", sp"});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *result_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"mov x16, sp", "str x16, " + *address});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack stack-save lowering requires stack-slot homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack stack-save lowering requires a register or stack-slot result home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_alloca(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto element_size = dynamic_stack_element_size_bytes(op);
  if (!element_size.has_value() || *element_size > 65535) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a printable nonzero element size",
        diagnostics);
  }

  const auto* count_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  const auto* result_home =
      find_dynamic_stack_value_home(context.function, op.result_value_name);
  if (count_home == nullptr || result_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires prepared operand and result homes",
        diagnostics);
  }

  std::vector<std::string> lines;
  std::string count_register;
  if (const auto reg = dynamic_stack_register_home_name(*count_home); reg.has_value()) {
    count_register = *reg;
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *count_home);
             address.has_value()) {
    lines.push_back("ldr x16, " + *address);
    count_register = "x16";
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *count_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot count homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot count home",
        diagnostics);
  }

  std::string byte_count_register = count_register;
  if (byte_count_register == "x17") {
    lines.push_back("mov x16, x17");
    byte_count_register = "x16";
  }
  if (*element_size != 1) {
    if (byte_count_register != "x16") {
      lines.push_back("mov x16, " + byte_count_register);
      byte_count_register = "x16";
    }
    lines.push_back("mov x17, #" + std::to_string(*element_size));
    lines.push_back("mul x16, x16, x17");
  }

  lines.push_back("mov x17, sp");
  lines.push_back("sub x17, x17, " + byte_count_register);
  lines.push_back("and x17, x17, #-16");
  lines.push_back("mov sp, x17");

  if (const auto reg = dynamic_stack_register_home_name(*result_home); reg.has_value()) {
    lines.push_back("mov " + *reg + ", sp");
  } else if (const auto address =
                 dynamic_stack_stack_home_address(context.function, *result_home);
             address.has_value()) {
    lines.push_back("str x17, " + *address);
  } else if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *result_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires stack-slot result homes to use a stable frame-pointer base",
        diagnostics);
  } else {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack allocation lowering requires a register or stack-slot result home",
        diagnostics);
  }

  return make_dynamic_stack_assembler_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_restore(
    const module::BlockLoweringContext& context,
    const prepare::PreparedDynamicStackOp& op,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto* operand_home =
      find_dynamic_stack_value_home(context.function, op.operand_value_name);
  if (operand_home == nullptr) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires a prepared operand home",
        diagnostics);
  }
  if (const auto reg = dynamic_stack_register_home_name(*operand_home); reg.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context, instruction_index, std::vector<std::string>{"mov sp, " + *reg});
  }
  if (const auto address =
          dynamic_stack_stack_home_address(context.function, *operand_home);
      address.has_value()) {
    return make_dynamic_stack_assembler_instruction(
        context,
        instruction_index,
        std::vector<std::string>{"ldr x16, " + *address, "mov sp, x16"});
  }
  if (dynamic_stack_home_requires_stable_frame_pointer(context.function, *operand_home)) {
    return make_dynamic_stack_failure(
        context,
        instruction_index,
        "AArch64 dynamic-stack restore lowering requires stack-slot operand homes to use a stable frame-pointer base",
        diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack restore lowering requires a register or stack-slot operand home",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_dynamic_stack_helper_call(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  const auto kind = dynamic_stack_helper_kind(call_inst.callee);
  if (!kind.has_value()) {
    return std::nullopt;
  }

  const auto* op = find_dynamic_stack_op(context, instruction_index, *kind);
  if (op == nullptr) {
    constexpr std::string_view message =
        "AArch64 dynamic-stack helper lowering requires prepared dynamic-stack operation authority";
    append_call_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        std::string{message});
    return make_bir_machine_instruction(
        context, instruction_index, make_dynamic_stack_rejection_record(
                                        context, instruction_index, message));
  }

  switch (*kind) {
    case prepare::PreparedDynamicStackOpKind::StackSave:
      return lower_dynamic_stack_save(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::DynamicAlloca:
      return lower_dynamic_alloca(context, *op, instruction_index, diagnostics);
    case prepare::PreparedDynamicStackOpKind::StackRestore:
      return lower_dynamic_stack_restore(context, *op, instruction_index, diagnostics);
  }
  return make_dynamic_stack_failure(
      context,
      instruction_index,
      "AArch64 dynamic-stack helper lowering visited an unsupported operation kind",
      diagnostics);
}

[[nodiscard]] std::optional<module::MachineInstruction> lower_call_instruction(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call_inst,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (auto dynamic_stack = lower_dynamic_stack_helper_call(
          context, call_inst, instruction_index, diagnostics);
      dynamic_stack.has_value()) {
    return dynamic_stack;
  }

  const auto variadic_helper = variadic_entry_helper_kind(call_inst.callee);
  const prepare::PreparedVariadicEntryPlanFunction* variadic_entry_plan = nullptr;
  if (variadic_helper.has_value()) {
    variadic_entry_plan =
        require_prepared_variadic_entry_plan(context, instruction_index, diagnostics);
    if (variadic_entry_plan == nullptr) {
      return std::nullopt;
    }
  }

  const auto* call_plan =
      require_prepared_call_plan(context, instruction_index, diagnostics);
  if (call_plan == nullptr) {
    return std::nullopt;
  }

  const prepare::PreparedVariadicEntryHelperOperandHomes* variadic_helper_operand_homes =
      nullptr;
  if (variadic_entry_plan != nullptr) {
    variadic_helper_operand_homes =
        prepare::find_prepared_variadic_entry_helper_operand_homes(
            *variadic_entry_plan, context.block_index, instruction_index);
    if (variadic_helper_operand_homes == nullptr ||
        !variadic_helper_operand_homes_complete(*variadic_helper_operand_homes)) {
      std::string message =
          "AArch64 variadic entry helper lowering requires prepared helper operand-home facts";
      const auto missing_consumption_fact =
          variadic_helper_missing_consumption_fact_message(*variadic_helper);
      if (!missing_consumption_fact.empty()) {
        message = missing_consumption_fact;
      }
      append_call_diagnostic(
          diagnostics,
          module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          context,
          instruction_index,
          std::move(message));
      return std::nullopt;
    }
  }

  return lower_prepared_call_instruction(context,
                                         call_inst,
                                         *call_plan,
                                         instruction_index,
                                         variadic_entry_plan,
                                         variadic_helper_operand_homes,
                                         variadic_helper,
                                         diagnostics);
}

[[nodiscard]] std::optional<bir::Value> instruction_result_value(
    const bir::Inst& inst) {
  return std::visit(
      [](const auto& typed_inst) -> std::optional<bir::Value> {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::LoadLocalInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::LoadGlobalInst>) {
          return typed_inst.result;
        } else if constexpr (std::is_same_v<T, bir::CallInst>) {
          return typed_inst.result;
        }
        return std::nullopt;
      },
      inst);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto* binary = std::get_if<bir::BinaryInst>(&inst);
  const auto result = instruction_result_value(inst);
  const bool supported_binary =
      binary != nullptr &&
      branch_condition_suffix(binary->opcode).has_value() &&
      (binary->operand_type == bir::TypeKind::F32 ||
       binary->operand_type == bir::TypeKind::F64);
  if ((cast == nullptr && !supported_binary) || !result.has_value()) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, *result);
  if (!value_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *value_name).has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register ||
      !home->register_name.has_value()) {
    return std::nullopt;
  }
  const auto* storage = [&]() -> const prepare::PreparedStoragePlanValue* {
    if (context.function.storage_plan == nullptr) {
      return nullptr;
    }
    for (const auto& value : context.function.storage_plan->values) {
      if (value.value_id == home->value_id) {
        return &value;
      }
    }
    return nullptr;
  }();
  if (storage == nullptr ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      storage->register_name != home->register_name) {
    return std::nullopt;
  }
  const auto expected_view = scalar_view_for_type(result->type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto target_register = abi::gp_register(parsed->index, *expected_view);
  if (!target_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  std::optional<std::uint8_t> scratch_index;
  for (const auto& scratch : scratches) {
    if (scratch.index != parsed->index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (!scratch_index.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          parsed->index,
                                          *scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted{
      .reg = *target_register,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_cast_publication_to_prepared_stack(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index) {
  const auto* cast = std::get_if<bir::CastInst>(&inst);
  const auto result = instruction_result_value(inst);
  if (cast == nullptr || !result.has_value()) {
    return std::nullopt;
  }
  const auto* home = prepared_value_home_for_value(context, *result);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto mnemonic = scalar_store_mnemonic(result->type);
  const auto value_view = scalar_view_for_type(result->type);
  if (!mnemonic.has_value() || !value_view.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  const auto target = gp_register_name(scratches.front().index, *value_view);
  if (!target.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *result,
                                          instruction_index + 1,
                                          scratches.front().index,
                                          scratches[1].index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  lines.push_back(std::string{*mnemonic} + " " + *target + ", " +
                  frame_slot_address(*home->offset_bytes));
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

[[nodiscard]] std::optional<bir::Value> find_bir_value_for_prepared_name(
    const module::BlockLoweringContext& context,
    c4c::ValueNameId value_name,
    std::size_t before_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  for (std::size_t index = before_instruction_index; index > 0; --index) {
    const auto result = instruction_result_value(context.bir_block->insts[index - 1]);
    if (!result.has_value()) {
      continue;
    }
    const auto prepared_name = prepared_named_value_id(context, *result);
    if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
      return result;
    }
  }
  if (before_instruction_index >= context.bir_block->insts.size()) {
    return std::nullopt;
  }
  if (const auto* call =
          std::get_if<bir::CallInst>(&context.bir_block->insts[before_instruction_index]);
      call != nullptr) {
    for (const auto& argument : call->args) {
      const auto prepared_name = prepared_named_value_id(context, argument);
      if (prepared_name == std::optional<c4c::ValueNameId>{value_name}) {
        return argument;
      }
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_call_boundary_source_to_destination(
    const module::BlockLoweringContext& context,
    module::MachineInstruction& instruction,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  auto* move_record =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move_record == nullptr ||
      !move_record->source_memory.has_value() ||
      move_record->source_memory_materializes_address ||
      !move_record->destination_register.has_value() ||
      !move_record->source_memory->result_value_name.has_value() ||
      move_record->source_memory->base_kind != MemoryBaseKind::FrameSlot ||
      find_emitted_scalar_register(
          scalar_state, *move_record->source_memory->result_value_name).has_value() ||
      !abi::is_gp_register(move_record->destination_register->reg)) {
    return std::nullopt;
  }

  const auto source_value =
      find_bir_value_for_prepared_name(
          context, *move_record->source_memory->result_value_name, instruction_index);
  if (!source_value.has_value()) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const std::uint8_t target_index = move_record->destination_register->reg.index;
  const std::uint8_t scratch_index =
      scratches.front().index == target_index && scratches.size() > 1
          ? scratches[1].index
          : scratches.front().index;
  if (scratch_index == target_index) {
    return std::nullopt;
  }

  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          *source_value,
                                          instruction_index,
                                          target_index,
                                          scratch_index,
                                          lines) ||
      lines.empty()) {
    return std::nullopt;
  }
  RegisterOperand emitted = *move_record->destination_register;
  emitted.value_id = move_record->source_memory->result_value_id;
  emitted.value_name = *move_record->source_memory->result_value_name;
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, instruction_index, std::move(lines));
}

void record_call_result_source_register(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    BlockScalarLoweringState& scalar_state) {
  if (context.function.value_locations != nullptr) {
    const auto* bundle = prepare::find_prepared_move_bundle(
        *context.function.value_locations,
        prepare::PreparedMovePhase::AfterCall,
        call_plan.block_index,
        call_plan.instruction_index);
    if (bundle != nullptr) {
      for (const auto& move : bundle->moves) {
        if (move.destination_kind !=
                prepare::PreparedMoveDestinationKind::CallResultAbi ||
            move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
            move.op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
          continue;
        }
        const auto* home =
            prepare::find_prepared_value_home(*context.function.value_locations,
                                              move.to_value_id);
        if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
          continue;
        }
        const auto expected_view =
            move.destination_register_name.has_value()
                ? abi::parse_aarch64_register_name(*move.destination_register_name)
                : std::nullopt;
        if (!expected_view.has_value() ||
            expected_view->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        abi::PreparedRegisterConversionResult converted;
        if (move.destination_register_placement.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_placement,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else if (move.destination_register_name.has_value()) {
          converted = abi::convert_prepared_register(
              *move.destination_register_name,
              prepare::PreparedRegisterBank::Fpr,
              prepare::PreparedRegisterClass::Float,
              expected_view->view);
        } else {
          continue;
        }
        if (!converted.reg.has_value() ||
            converted.reg->bank != abi::RegisterBank::FpSimd) {
          continue;
        }
        record_emitted_scalar_register(
            scalar_state,
            home->value_name,
            RegisterOperand{
                .reg = *converted.reg,
                .role = RegisterOperandRole::CallAbi,
                .value_id = home->value_id,
                .value_name = home->value_name,
                .prepared_class = prepare::PreparedRegisterClass::Float,
                .prepared_bank = prepare::PreparedRegisterBank::Fpr,
                .expected_view = converted.reg->view,
                .contiguous_width = move.destination_contiguous_width,
                .occupied_register_references = {*converted.reg},
                .occupied_registers = {abi::register_name(*converted.reg)},
            });
      }
    }
  }

  if (!call_plan.result.has_value() ||
      !call_plan.result->destination_value_id.has_value() ||
      !call_plan.result->source_register_name.has_value() ||
      call_plan.result->source_register_bank != prepare::PreparedRegisterBank::Gpr ||
      context.function.value_locations == nullptr) {
    return;
  }
  const auto* home =
      prepare::find_prepared_value_home(*context.function.value_locations,
                                        *call_plan.result->destination_value_id);
  if (home == nullptr) {
    return;
  }
  const auto parsed = abi::parse_aarch64_register_name(
      *call_plan.result->source_register_name);
  if (!parsed.has_value() || parsed->bank != abi::RegisterBank::GeneralPurpose) {
    return;
  }
  auto viewed = *parsed;
  if (call_plan.result->source_register_placement.has_value()) {
    if (const auto converted =
            abi::convert_prepared_register(*call_plan.result->source_register_placement,
                                           prepare::PreparedRegisterClass::General,
                                           parsed->view);
        converted.reg.has_value()) {
      viewed = *converted.reg;
    }
  }
  record_emitted_scalar_register(
      scalar_state,
      home->value_name,
      RegisterOperand{
          .reg = viewed,
          .role = RegisterOperandRole::CallAbi,
          .value_id = home->value_id,
          .value_name = home->value_name,
          .prepared_class = prepare::PreparedRegisterClass::General,
          .prepared_bank = prepare::PreparedRegisterBank::Gpr,
          .expected_view = viewed.view,
          .contiguous_width = call_plan.result->source_contiguous_width,
          .occupied_register_references = {viewed},
          .occupied_registers = {abi::register_name(viewed)},
      });
}

[[nodiscard]] bool move_source_aliases_destination(
    const module::MachineInstruction& source_instruction,
    const module::MachineInstruction& destination_instruction) {
  const auto* source =
      std::get_if<CallBoundaryMoveInstructionRecord>(&source_instruction.target.payload);
  const auto* destination =
      std::get_if<CallBoundaryMoveInstructionRecord>(&destination_instruction.target.payload);
  return source != nullptr &&
         destination != nullptr &&
         destination->move.reason != "callee_saved_preservation_home_population" &&
         source->source_register.has_value() &&
         destination->destination_register.has_value() &&
         registers_alias(*source->source_register, *destination->destination_register);
}

[[nodiscard]] bool call_boundary_move_reloads_materialized_address(
    const module::MachineInstruction& instruction,
    const std::vector<module::MachineInstruction>& materialized_addresses) {
  const auto* move =
      std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
  if (move == nullptr ||
      move->phase != prepare::PreparedMovePhase::BeforeCall ||
      move->move.destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      move->move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      !move->source_memory.has_value() ||
      !move->destination_register.has_value() ||
      (!move->source_memory->result_value_id.has_value() &&
       !move->source_memory->result_value_name.has_value())) {
    return false;
  }
  for (const auto& materialized : materialized_addresses) {
    const auto* address =
        std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
    if (address == nullptr || !address->result_register.has_value() ||
        !registers_alias(*address->result_register, *move->destination_register)) {
      continue;
    }
    const bool same_value_id =
        move->source_memory->result_value_id.has_value() &&
        address->result_value_id == move->source_memory->result_value_id;
    const bool same_value_name =
        move->source_memory->result_value_name.has_value() &&
        address->result_value_name == *move->source_memory->result_value_name;
    if (same_value_id || same_value_name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] std::vector<module::MachineInstruction>
order_before_call_moves_for_source_preservation(
    std::vector<module::MachineInstruction> moves) {
  std::vector<module::MachineInstruction> ordered;
  ordered.reserve(moves.size());
  while (!moves.empty()) {
    auto selected = moves.begin();
    for (auto candidate = moves.begin(); candidate != moves.end(); ++candidate) {
      const bool protects_live_source =
          std::any_of(moves.begin(), moves.end(), [&](const auto& other) {
            return &other != &*candidate &&
                   move_source_aliases_destination(*candidate, other);
          });
      if (protects_live_source) {
        selected = candidate;
        break;
      }
    }
    ordered.push_back(std::move(*selected));
    moves.erase(selected);
  }
  return ordered;
}

[[nodiscard]] std::vector<module::MachineInstruction>
materialize_missing_frame_slot_call_arguments(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.value_locations == nullptr) {
    return lowered;
  }
  for (const auto& argument : call_plan.arguments) {
    if (argument.source_encoding != prepare::PreparedStorageEncodingKind::FrameSlot ||
        !argument.source_value_id.has_value() ||
        argument.destination_register_bank != prepare::PreparedRegisterBank::Gpr ||
        argument.destination_contiguous_width > 1) {
      continue;
    }
    const auto* home =
        prepare::find_prepared_value_home(*context.function.value_locations,
                                          *argument.source_value_id);
    if (home == nullptr ||
        find_emitted_scalar_register(scalar_state, home->value_name).has_value()) {
      continue;
    }
    const auto source_value =
        find_bir_value_for_prepared_name(context, home->value_name, instruction_index);
    if (!source_value.has_value()) {
      continue;
    }
    const auto expected_view = scalar_view_for_type(source_value->type);
    if (!expected_view.has_value()) {
      continue;
    }
    const auto prepared_class = prepare::PreparedRegisterClass::General;
    abi::PreparedRegisterConversionResult converted;
    if (argument.destination_register_placement.has_value()) {
      converted =
          abi::convert_prepared_register(*argument.destination_register_placement,
                                         prepared_class,
                                         expected_view);
    } else if (argument.destination_register_name.has_value()) {
      converted =
          abi::convert_prepared_register(*argument.destination_register_name,
                                         argument.destination_register_bank,
                                         prepared_class,
                                         expected_view);
    } else {
      continue;
    }
    if (!converted.reg.has_value()) {
      continue;
    }
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    if (scratches.empty()) {
      continue;
    }
    const std::uint8_t target_index = converted.reg->index;
    const std::uint8_t scratch_index =
        scratches.front().index == target_index && scratches.size() > 1
            ? scratches[1].index
            : scratches.front().index;
    if (scratch_index == target_index) {
      continue;
    }
    std::vector<std::string> lines;
    if (!emit_value_publication_to_register(context,
                                            *source_value,
                                            instruction_index,
                                            target_index,
                                            scratch_index,
                                            lines) ||
        lines.empty()) {
      continue;
    }
    RegisterOperand emitted{
        .reg = *converted.reg,
        .role = RegisterOperandRole::CallAbi,
        .value_id = home->value_id,
        .value_name = home->value_name,
        .prepared_class = prepared_class,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = expected_view,
        .contiguous_width = argument.destination_contiguous_width,
        .occupied_register_references = {*converted.reg},
        .occupied_registers = {abi::register_name(*converted.reg)},
    };
    record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
    if (auto materialized =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*materialized));
    }
  }
  return lowered;
}

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  for (const auto& preserved : call_plan.preserved_values) {
    if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
        preserved.value_name == c4c::kInvalidValueName ||
        !preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0) {
      continue;
    }
    const auto emitted =
        find_emitted_scalar_register(scalar_state, preserved.value_name);
    std::optional<abi::RegisterView> expected_view;
    if (*preserved.stack_size_bytes == 8) {
      expected_view = abi::RegisterView::X;
    } else if (*preserved.stack_size_bytes == 4 ||
               *preserved.stack_size_bytes == 2 ||
               *preserved.stack_size_bytes == 1) {
      expected_view = abi::RegisterView::W;
    } else {
      continue;
    }
    const bool has_prior_stack_preservation = [&]() {
      const auto* call_plans =
          context.function.call_plans != nullptr
              ? context.function.call_plans
              : (context.function.prepared != nullptr &&
                         context.function.control_flow != nullptr
                     ? prepare::find_prepared_call_plans(
                           *context.function.prepared,
                           context.function.control_flow->function_name)
                     : nullptr);
      if (call_plans == nullptr) {
        return false;
      }
      for (const auto& call : call_plans->calls) {
        if (call.block_index != context.block_index ||
            call.instruction_index >= instruction_index) {
          continue;
        }
        for (const auto& prior : call.preserved_values) {
          if (prior.value_id == preserved.value_id &&
              prior.route == prepare::PreparedCallPreservationRoute::StackSlot) {
            return true;
          }
        }
      }
      return false;
    }();
    if (has_prior_stack_preservation) {
      continue;
    }
    std::optional<abi::RegisterReference> source_register;
    if (emitted.has_value() && abi::is_gp_register(emitted->reg)) {
      source_register = emitted->reg;
    } else if (context.function.value_locations != nullptr) {
      const auto* home =
          prepare::find_prepared_value_home(*context.function.value_locations,
                                            preserved.value_id);
      if (home != nullptr &&
          home->kind == prepare::PreparedValueHomeKind::Register &&
          home->register_name.has_value()) {
        const auto parsed = abi::parse_aarch64_register_name(*home->register_name);
        if (parsed.has_value() &&
            parsed->bank == abi::RegisterBank::GeneralPurpose) {
          source_register = *parsed;
        }
      }
    }
    if (!source_register.has_value()) {
      continue;
    }
    const auto source_reg = abi::gp_register(source_register->index, *expected_view);
    if (!source_reg.has_value()) {
      continue;
    }

    std::vector<std::string> lines;
    lines.push_back("str " + std::string(abi::register_name(*source_reg)) + ", " +
                    frame_slot_address(*preserved.stack_offset_bytes));
    if (auto published =
            make_select_chain_materialization_instruction(
                context, instruction_index, std::move(lines))) {
      lowered.push_back(std::move(*published));
    }
  }
  return lowered;
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_conditional_branch_condition_publication(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.bir_block == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto& condition = context.bir_block->terminator.condition;
  const auto condition_name = prepared_named_value_id(context, condition);
  if (!condition_name.has_value() ||
      find_emitted_scalar_register(scalar_state, *condition_name).has_value()) {
    return std::nullopt;
  }
  const auto* producer =
      condition.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(
                context, condition.name, context.bir_block->insts.size())
          : nullptr;
  const auto producer_index = producer_instruction_index(context, producer);
  if (producer == nullptr || !producer_index.has_value()) {
    return std::nullopt;
  }
  return lower_scalar_control_value_instruction(context,
                                                *producer,
                                                *producer_index,
                                                scalar_state,
                                                diagnostics,
                                                true);
}

[[nodiscard]] std::optional<module::MachineInstruction>
lower_missing_fused_compare_operand_publication(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.bir_block == nullptr || value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home =
      context.function.value_locations != nullptr
          ? prepare::find_prepared_value_home(*context.function.value_locations, *value_name)
          : nullptr;
  if (home == nullptr) {
    return std::nullopt;
  }
  if (find_emitted_scalar_register(scalar_state, *value_name).has_value() &&
      home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  auto resolved =
      resolve_value_operand(home->value_id, context.function, diagnostics);
  const auto expected_view = scalar_view_for_type(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  if (auto published =
          current_block_entry_publication_register(context, value, *expected_view)) {
    record_emitted_scalar_register(scalar_state,
                                   published->value_name,
                                   *published);
    return std::nullopt;
  }

  std::uint8_t target_index = 0;
  std::uint8_t scratch_index = 0;
  bool has_target = false;
  if (resolved.has_value() && resolved->register_reference.has_value() &&
      abi::is_gp_register(*resolved->register_reference)) {
    target_index = resolved->register_reference->index;
    has_target = true;
  } else {
    const auto scratches = abi::reserved_mir_scratch_gp_registers();
    for (const auto scratch : scratches) {
      bool occupied = false;
      for (const auto& [_, emitted] : scalar_state.emitted_registers) {
        if (emitted.reg.bank == scratch.bank && emitted.reg.index == scratch.index) {
          occupied = true;
          break;
        }
      }
      if (!occupied) {
        target_index = scratch.index;
        has_target = true;
        break;
      }
    }
  }
  if (!has_target) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  for (const auto scratch : scratches) {
    if (scratch.index != target_index) {
      scratch_index = scratch.index;
      break;
    }
  }
  if (scratch_index == target_index) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!emit_value_publication_to_register(context,
                                          value,
                                          context.bir_block->insts.size(),
                                          target_index,
                                          scratch_index,
                                          lines,
                                          true) ||
      lines.empty()) {
    return std::nullopt;
  }
  auto reg = abi::x_register(target_index);
  if (resolved.has_value() && resolved->register_reference.has_value()) {
    reg = *resolved->register_reference;
  } else {
    reg = abi::gp_register(target_index, *expected_view).value_or(reg);
  }
  reg.view = *expected_view;
  RegisterOperand emitted{
      .reg = reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .expected_view = expected_view,
  };
  record_emitted_scalar_register(scalar_state, emitted.value_name, emitted);
  return make_select_chain_materialization_instruction(
      context, context.bir_block->insts.size(), std::move(lines));
}

[[nodiscard]] std::vector<module::MachineInstruction>
lower_missing_fused_compare_operand_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return lowered;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch) {
    return lowered;
  }
  if (branch_condition->lhs.has_value()) {
    if (auto lhs = lower_missing_fused_compare_operand_publication(
            context, *branch_condition->lhs, scalar_state, diagnostics)) {
      lowered.push_back(std::move(*lhs));
    }
  }
  if (branch_condition->rhs.has_value()) {
    if (auto rhs = lower_missing_fused_compare_operand_publication(
            context, *branch_condition->rhs, scalar_state, diagnostics)) {
      lowered.push_back(std::move(*rhs));
    }
  }
  return lowered;
}

[[nodiscard]] bool fused_compare_operand_has_select_producer(
    const module::BlockLoweringContext& context,
    const bir::Value& value) {
  if (context.bir_block == nullptr || value.kind != bir::Value::Kind::Named) {
    return false;
  }
  const auto* producer = find_same_block_named_producer(
      context, value.name, context.bir_block->insts.size());
  return producer != nullptr && std::get_if<bir::SelectInst>(producer) != nullptr;
}

[[nodiscard]] bool fused_compare_uses_selected_operand(
    const module::BlockLoweringContext& context) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return false;
  }
  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition->can_fuse_with_branch) {
    return false;
  }
  return (branch_condition->lhs.has_value() &&
          fused_compare_operand_has_select_producer(context, *branch_condition->lhs)) ||
         (branch_condition->rhs.has_value() &&
          fused_compare_operand_has_select_producer(context, *branch_condition->rhs));
}

[[nodiscard]] std::optional<abi::RegisterView> entry_formal_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::F128:
      return abi::RegisterView::Q;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> entry_formal_load_opcode(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return "ldrb";
    case bir::TypeKind::I16:
      return "ldrh";
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return "ldr";
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> entry_formal_store_opcode(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
      return "strb";
    case bir::TypeKind::I16:
      return "strh";
    case bir::TypeKind::I32:
    case bir::TypeKind::F32:
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return "str";
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] bool entry_formal_same_aarch64_register_bank(
    const bir::CallArgAbiInfo& lhs,
    const bir::CallArgAbiInfo& rhs) {
  const auto is_float_bank = [](const bir::CallArgAbiInfo& abi) {
    return abi.primary_class == bir::AbiValueClass::Sse ||
           abi.primary_class == bir::AbiValueClass::X87;
  };
  if (is_float_bank(lhs) || is_float_bank(rhs)) {
    return is_float_bank(lhs) == is_float_bank(rhs);
  }
  return lhs.primary_class == bir::AbiValueClass::Integer &&
         rhs.primary_class == bir::AbiValueClass::Integer;
}

[[nodiscard]] std::size_t entry_formal_aarch64_register_slot_count(
    const bir::CallArgAbiInfo& abi) {
  if (abi.type == bir::TypeKind::Ptr && abi.byval_copy &&
      !abi.sret_pointer &&
      abi.primary_class == bir::AbiValueClass::Integer &&
      abi.size_bytes > 0 && abi.size_bytes <= 16) {
    return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8) / 8;
  }
  return 1;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_aarch64_register_slot_start(
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size() ||
      !function.params[param_index].abi.has_value()) {
    return std::nullopt;
  }
  const auto& param_abi = *function.params[param_index].abi;
  if (!param_abi.passed_in_register ||
      (param_abi.type == bir::TypeKind::Ptr && param_abi.sret_pointer)) {
    return std::nullopt;
  }

  std::size_t register_index = 0;
  for (std::size_t candidate_index = 0; candidate_index < param_index; ++candidate_index) {
    const auto& candidate = function.params[candidate_index];
    if (!candidate.abi.has_value() || !candidate.abi->passed_in_register) {
      continue;
    }
    if (candidate.abi->type == bir::TypeKind::Ptr && candidate.abi->sret_pointer) {
      continue;
    }
    if (entry_formal_same_aarch64_register_bank(*candidate.abi, param_abi)) {
      register_index += entry_formal_aarch64_register_slot_count(*candidate.abi);
    }
  }

  const auto required_slots = entry_formal_aarch64_register_slot_count(param_abi);
  if (param_abi.primary_class == bir::AbiValueClass::Integer &&
      register_index + required_slots > 8U) {
    return std::nullopt;
  }
  return register_index;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_abi_register_index(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value() || !param.abi->passed_in_register) {
    if (target_profile.arch == c4c::TargetArch::Aarch64 &&
        param.abi.has_value() &&
        param.abi->type == bir::TypeKind::Ptr &&
        param.abi->sret_pointer) {
      return 0;
    }
    return std::nullopt;
  }
  if (target_profile.arch != c4c::TargetArch::Aarch64) {
    return param_index;
  }
  if (param.abi->type == bir::TypeKind::Ptr && param.abi->sret_pointer) {
    return 0;
  }

  return entry_formal_aarch64_register_slot_start(function, param_index);
}

[[nodiscard]] std::optional<abi::RegisterReference> entry_formal_source_register(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return std::nullopt;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value()) {
    return std::nullopt;
  }
  const auto abi_register_index =
      entry_formal_abi_register_index(target_profile, function, param_index);
  const auto register_name =
      abi_register_index.has_value()
          ? prepare::call_arg_destination_register_name(
                target_profile, *param.abi, *abi_register_index)
          : std::nullopt;
  const auto expected_view = entry_formal_register_view(param.type);
  if (!register_name.has_value() || !expected_view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*register_name);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
    return abi::gp_register(parsed->index, *expected_view);
  }
  if (parsed->bank == abi::RegisterBank::FpSimd) {
    return abi::fp_simd_register(parsed->index, *expected_view);
  }
  return std::nullopt;
}

[[nodiscard]] std::vector<std::string> entry_formal_store_lines(
    abi::RegisterReference source,
    bir::TypeKind type,
    std::size_t stack_offset_bytes) {
  const auto opcode = entry_formal_store_opcode(type);
  if (!opcode.has_value()) {
    return {};
  }
  const std::string source_name{abi::register_name(source)};
  std::vector<std::string> lines;
  if (stack_offset_bytes <= 4095U) {
    std::string line = std::string{*opcode} + " " + source_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return lines;
  }
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  lines = materialize_integer_constant_lines(scratch, stack_offset_bytes, 64);
  lines.push_back("add " + std::string{abi::register_name(scratch)} +
                  ", sp, " + std::string{abi::register_name(scratch)});
  lines.push_back(std::string{*opcode} + " " + source_name + ", [" +
                  std::string{abi::register_name(scratch)} + "]");
  return lines;
}

[[nodiscard]] std::vector<std::string> entry_formal_load_lines(
    abi::RegisterReference destination,
    bir::TypeKind type,
    std::size_t stack_offset_bytes) {
  const auto opcode = entry_formal_load_opcode(type);
  if (!opcode.has_value()) {
    return {};
  }
  const std::string destination_name{abi::register_name(destination)};
  std::vector<std::string> lines;
  if (stack_offset_bytes <= 4095U) {
    std::string line = std::string{*opcode} + " " + destination_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return lines;
  }
  const auto scratch = abi::reserved_mir_scratch_gp_registers()[1];
  lines = materialize_integer_constant_lines(scratch, stack_offset_bytes, 64);
  lines.push_back("add " + std::string{abi::register_name(scratch)} +
                  ", sp, " + std::string{abi::register_name(scratch)});
  lines.push_back(std::string{*opcode} + " " + destination_name + ", [" +
                  std::string{abi::register_name(scratch)} + "]");
  return lines;
}

[[nodiscard]] std::size_t entry_formal_stack_argument_size_bytes(
    const bir::CallArgAbiInfo& abi) {
  return align_to(std::max<std::size_t>(abi.size_bytes, 8), 8);
}

[[nodiscard]] std::size_t entry_formal_stack_argument_alignment_bytes(
    const bir::CallArgAbiInfo& abi) {
  const std::size_t abi_alignment = abi.align_bytes == 0 ? abi.size_bytes : abi.align_bytes;
  return std::min<std::size_t>(std::max<std::size_t>(abi_alignment, 8), 16);
}

[[nodiscard]] bool entry_formal_uses_incoming_stack(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size()) {
    return false;
  }
  const auto& param = function.params[param_index];
  if (!param.abi.has_value()) {
    return false;
  }
  if (param.abi->passed_on_stack) {
    return true;
  }
  return target_profile.arch == c4c::TargetArch::Aarch64 &&
         param.abi->passed_in_register &&
         entry_formal_aarch64_register_slot_start(function, param_index) == std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_incoming_stack_offset(
    const c4c::TargetProfile& target_profile,
    const bir::Function& function,
    std::size_t param_index) {
  if (param_index >= function.params.size() ||
      !entry_formal_uses_incoming_stack(target_profile, function, param_index)) {
    return std::nullopt;
  }

  std::size_t next_offset = 0;
  for (std::size_t index = 0; index < function.params.size(); ++index) {
    const auto& param = function.params[index];
    if (!entry_formal_uses_incoming_stack(target_profile, function, index)) {
      continue;
    }
    next_offset = align_to(next_offset, entry_formal_stack_argument_alignment_bytes(*param.abi));
    if (index == param_index) {
      return next_offset;
    }
    next_offset += entry_formal_stack_argument_size_bytes(*param.abi);
  }
  return std::nullopt;
}

[[nodiscard]] bool function_has_call(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::CallInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

[[nodiscard]] std::optional<std::size_t> entry_formal_frame_size_bytes(
    const module::FunctionLoweringContext& context) {
  const auto* frame = context.frame_plan;
  const auto* function = context.bir_function;
  if (frame == nullptr || function == nullptr ||
      frame->has_dynamic_stack || context.dynamic_stack_plan != nullptr) {
    return std::nullopt;
  }

  std::size_t frame_alignment =
      std::max<std::size_t>(frame->frame_alignment_bytes, kStackPointerAlignmentBytes);
  std::size_t prepared_frame_size = frame->frame_size_bytes;
  for (const auto& saved : frame->saved_callee_registers) {
    if (!saved.slot_placement.has_value() ||
        !saved.slot_placement->stack_offset_bytes.has_value() ||
        !saved.slot_placement->size_bytes.has_value() ||
        !saved.slot_placement->align_bytes.has_value()) {
      return std::nullopt;
    }
    prepared_frame_size = std::max(
        prepared_frame_size,
        *saved.slot_placement->stack_offset_bytes + *saved.slot_placement->size_bytes);
    frame_alignment = std::max(frame_alignment, *saved.slot_placement->align_bytes);
  }

  std::size_t frame_size = align_to(prepared_frame_size, frame_alignment);
  if (function_has_call(*function)) {
    frame_size = align_to(prepared_frame_size + 16, frame_alignment);
  }
  return frame_size == 0 ? std::optional<std::size_t>{}
                         : std::optional<std::size_t>{frame_size};
}

void append_entry_formal_byte_store(std::vector<std::string>& lines,
                                    abi::RegisterReference byte_source,
                                    std::size_t stack_offset_bytes) {
  const std::string source_name{abi::register_name(byte_source)};
  if (stack_offset_bytes <= 4095U) {
    std::string line = "strb " + source_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return;
  }

  const auto address_scratch = abi::reserved_mir_scratch_gp_registers()[1];
  const std::string address_name{abi::register_name(address_scratch)};
  auto address_lines =
      materialize_integer_constant_lines(address_scratch, stack_offset_bytes, 64);
  lines.insert(lines.end(), address_lines.begin(), address_lines.end());
  lines.push_back("add " + address_name + ", sp, " + address_name);
  lines.push_back("strb " + source_name + ", [" + address_name + "]");
}

void append_entry_formal_byte_load(std::vector<std::string>& lines,
                                   abi::RegisterReference byte_destination,
                                   std::size_t stack_offset_bytes) {
  const std::string destination_name{abi::register_name(byte_destination)};
  if (stack_offset_bytes <= 4095U) {
    std::string line = "ldrb " + destination_name + ", [sp";
    if (stack_offset_bytes != 0U) {
      line += ", #";
      line += std::to_string(stack_offset_bytes);
    }
    line += "]";
    lines.push_back(std::move(line));
    return;
  }

  const auto address_scratch = abi::reserved_mir_scratch_gp_registers()[1];
  const std::string address_name{abi::register_name(address_scratch)};
  auto address_lines =
      materialize_integer_constant_lines(address_scratch, stack_offset_bytes, 64);
  lines.insert(lines.end(), address_lines.begin(), address_lines.end());
  lines.push_back("add " + address_name + ", sp, " + address_name);
  lines.push_back("ldrb " + destination_name + ", [" + address_name + "]");
}

[[nodiscard]] std::vector<std::string> entry_formal_byval_aggregate_store_lines(
    const bir::Param& param,
    abi::RegisterReference source,
    std::size_t stack_offset_bytes) {
  if (!param.abi.has_value() || param.type != bir::TypeKind::Ptr ||
      !param.abi->byval_copy || !param.abi->passed_in_register ||
      param.abi->primary_class != bir::AbiValueClass::Integer ||
      !abi::is_gp_register(source) || param.size_bytes == 0U) {
    return {};
  }

  const std::size_t lane_count = (param.size_bytes + 7U) / 8U;
  if (lane_count == 0U || lane_count > 2U ||
      source.index + lane_count > 8U) {
    return {};
  }

  const auto value_scratch = abi::reserved_mir_scratch_gp_registers()[0];
  const auto value_scratch_w =
      abi::gp_register(value_scratch.index, abi::RegisterView::W);
  if (!value_scratch_w.has_value()) {
    return {};
  }

  std::vector<std::string> lines;
  std::size_t byte_index = 0;
  while (byte_index + 8U <= param.size_bytes) {
    const std::size_t lane_index = byte_index / 8U;
    const auto lane_x =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::X);
    if (!lane_x.has_value()) {
      return {};
    }
    auto lane_lines = entry_formal_store_lines(*lane_x,
                                               bir::TypeKind::I64,
                                               stack_offset_bytes + byte_index);
    lines.insert(lines.end(), lane_lines.begin(), lane_lines.end());
    byte_index += 8U;
  }
  for (; byte_index < param.size_bytes; ++byte_index) {
    const std::size_t lane_index = byte_index / 8U;
    const std::size_t lane_byte = byte_index % 8U;
    const auto lane_x =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::X);
    const auto lane_w =
        abi::gp_register(static_cast<std::uint8_t>(source.index + lane_index),
                         abi::RegisterView::W);
    if (!lane_x.has_value() || !lane_w.has_value()) {
      return {};
    }

    abi::RegisterReference byte_source = *lane_w;
    if (lane_byte != 0U) {
      lines.push_back("lsr " + std::string{abi::register_name(value_scratch)} +
                      ", " + std::string{abi::register_name(*lane_x)} +
                      ", #" + std::to_string(lane_byte * 8U));
      byte_source = *value_scratch_w;
    }
    append_entry_formal_byte_store(lines,
                                   byte_source,
                                   stack_offset_bytes + byte_index);
  }
  return lines;
}

[[nodiscard]] std::vector<std::string>
entry_formal_byval_aggregate_stack_source_publication_lines(
    const bir::Param& param,
    const prepare::PreparedValueHome& home,
    std::size_t source_stack_offset_bytes) {
  if (!param.abi.has_value() || param.type != bir::TypeKind::Ptr ||
      !param.abi->byval_copy ||
      param.abi->primary_class != bir::AbiValueClass::Integer ||
      home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value() || param.size_bytes == 0U) {
    return {};
  }

  const auto value_scratch = abi::reserved_mir_scratch_gp_registers()[0];
  const auto value_scratch_w =
      abi::gp_register(value_scratch.index, abi::RegisterView::W);
  if (!value_scratch_w.has_value()) {
    return {};
  }

  std::vector<std::string> lines;
  for (std::size_t byte_index = 0; byte_index < param.size_bytes; ++byte_index) {
    append_entry_formal_byte_load(lines,
                                  *value_scratch_w,
                                  source_stack_offset_bytes + byte_index);
    append_entry_formal_byte_store(lines,
                                   *value_scratch_w,
                                   *home.offset_bytes + byte_index);
  }
  return lines;
}

[[nodiscard]] std::optional<abi::RegisterReference> entry_formal_destination_register(
    const prepare::PreparedValueHome& home,
    bir::TypeKind type) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      !home.register_name.has_value()) {
    return std::nullopt;
  }
  const auto view = entry_formal_register_view(type);
  if (!view.has_value()) {
    return std::nullopt;
  }
  const auto parsed = abi::parse_aarch64_register_name(*home.register_name);
  if (!parsed.has_value()) {
    return std::nullopt;
  }
  if (parsed->bank == abi::RegisterBank::GeneralPurpose) {
    return abi::gp_register(parsed->index, *view);
  }
  if (parsed->bank == abi::RegisterBank::FpSimd) {
    return abi::fp_simd_register(parsed->index, *view);
  }
  return std::nullopt;
}

void record_entry_formal_register_home(
    BlockScalarLoweringState& scalar_state,
    const prepare::PreparedValueHome& home,
    bir::TypeKind type) {
  const auto destination = entry_formal_destination_register(home, type);
  if (!destination.has_value()) {
    return;
  }
  prepare::PreparedRegisterClass prepared_class = prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank prepared_bank = prepare::PreparedRegisterBank::None;
  if (destination->bank == abi::RegisterBank::GeneralPurpose) {
    prepared_class = prepare::PreparedRegisterClass::General;
    prepared_bank = prepare::PreparedRegisterBank::Gpr;
  } else if (destination->bank == abi::RegisterBank::FpSimd) {
    prepared_class = (destination->view == abi::RegisterView::Q)
                         ? prepare::PreparedRegisterClass::Vector
                         : prepare::PreparedRegisterClass::Float;
    prepared_bank = (destination->view == abi::RegisterView::Q)
                        ? prepare::PreparedRegisterBank::Vreg
                        : prepare::PreparedRegisterBank::Fpr;
  }
  record_emitted_scalar_register(
      scalar_state,
      home.value_name,
      RegisterOperand{
          .reg = *destination,
          .role = RegisterOperandRole::StoragePlan,
          .value_id = home.value_id,
          .value_name = home.value_name,
          .prepared_class = prepared_class,
          .prepared_bank = prepared_bank,
          .expected_view = destination->view,
      });
}

[[nodiscard]] std::vector<std::string> entry_formal_stack_source_publication_lines(
    const module::BlockLoweringContext& context,
    const bir::Param& param,
    const prepare::PreparedValueHome& home,
    std::size_t param_index) {
  if (context.function.bir_function == nullptr) {
    return {};
  }
  const auto incoming_offset =
      entry_formal_incoming_stack_offset(context.function.prepared->target_profile,
                                         *context.function.bir_function,
                                         param_index);
  const auto frame_size = entry_formal_frame_size_bytes(context.function);
  if (!incoming_offset.has_value() || !frame_size.has_value()) {
    return {};
  }
  if (param.is_byval) {
    return entry_formal_byval_aggregate_stack_source_publication_lines(
        param, home, *frame_size + *incoming_offset);
  }
  if (home.kind == prepare::PreparedValueHomeKind::None &&
      param.type == bir::TypeKind::F128) {
    const auto scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
    const auto scratch = abi::fp_simd_register(scratch_base.index, abi::RegisterView::Q);
    if (!scratch.has_value()) {
      return {};
    }
    auto lines = entry_formal_load_lines(*scratch,
                                         param.type,
                                         *frame_size + *incoming_offset);
    auto stores = entry_formal_store_lines(*scratch, param.type, *incoming_offset);
    lines.insert(lines.end(), stores.begin(), stores.end());
    return lines;
  }
  if (home.kind == prepare::PreparedValueHomeKind::Register) {
    const auto destination = entry_formal_destination_register(home, param.type);
    if (!destination.has_value()) {
      return {};
    }
    return entry_formal_load_lines(*destination, param.type, *frame_size + *incoming_offset);
  }
  if (home.kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home.offset_bytes.has_value()) {
    return {};
  }
  const auto view = entry_formal_register_view(param.type);
  if (!view.has_value()) {
    return {};
  }
  const auto scratch_base = abi::reserved_mir_scratch_fp_simd_registers().front();
  const auto scratch = abi::fp_simd_register(scratch_base.index, *view);
  if (!scratch.has_value()) {
    return {};
  }
  auto lines = entry_formal_load_lines(*scratch, param.type, *frame_size + *incoming_offset);
  auto stores = entry_formal_store_lines(*scratch, param.type, *home.offset_bytes);
  lines.insert(lines.end(), stores.begin(), stores.end());
  return lines;
}

[[nodiscard]] std::vector<std::string> entry_formal_register_move_lines(
    abi::RegisterReference source,
    abi::RegisterReference destination) {
  if (source.bank != destination.bank) {
    return {};
  }
  if (source.index == destination.index) {
    return {};
  }
  const std::string source_name{abi::register_name(source)};
  const std::string destination_name{abi::register_name(destination)};
  const bool scalar_fp_register_move =
      source.bank == abi::RegisterBank::FpSimd &&
      (source.view == abi::RegisterView::S || source.view == abi::RegisterView::D) &&
      source.view == destination.view;
  return {std::string{scalar_fp_register_move ? "fmov" : "mov"} + " " +
          destination_name + ", " + source_name};
}

[[nodiscard]] module::MachineInstruction make_entry_formal_publication_instruction(
    const module::BlockLoweringContext& context,
    std::size_t param_index,
    std::vector<std::string> lines) {
  std::string asm_text;
  for (std::size_t index = 0; index < lines.size(); ++index) {
    if (index != 0) {
      asm_text += '\n';
    }
    asm_text += lines[index];
  }
  InstructionRecord target{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::Unspecified,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .block_index = context.block_index,
      .instruction_index = param_index,
      .side_effects = {MachineSideEffectKind::MemoryWrite,
                       MachineSideEffectKind::InlineAssembly},
      .payload = AssemblerInstructionRecord{
          .has_inline_asm_payload = true,
          .side_effects = true,
          .inline_asm_template = std::move(asm_text),
      },
  };
  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirInstruction,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
              .instruction_index = param_index,
          },
  };
}

[[nodiscard]] std::vector<module::MachineInstruction> lower_entry_formal_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  if (context.block_index != 0 || context.function.prepared == nullptr ||
      context.function.value_locations == nullptr ||
      context.function.bir_function == nullptr) {
    return lowered;
  }
  for (std::size_t param_index = 0;
       param_index < context.function.bir_function->params.size();
       ++param_index) {
    const auto& param = context.function.bir_function->params[param_index];
    if (param.is_varargs || param.is_sret) {
      continue;
    }
    const auto value_name = prepare::resolve_prepared_value_name_id(
        context.function.prepared->names, param.name);
    if (!value_name.has_value()) {
      continue;
    }
    const auto* home =
        prepare::find_prepared_value_home(*context.function.value_locations, *value_name);
    if (home == nullptr) {
      continue;
    }
    std::vector<std::string> lines;
    if (entry_formal_uses_incoming_stack(context.function.prepared->target_profile,
                                         *context.function.bir_function,
                                         param_index)) {
      lines = entry_formal_stack_source_publication_lines(context, param, *home, param_index);
      if (!lines.empty() && !param.is_byval &&
          home->kind == prepare::PreparedValueHomeKind::Register) {
        record_entry_formal_register_home(scalar_state, *home, param.type);
      }
    } else {
      const auto source = entry_formal_source_register(
          context.function.prepared->target_profile,
          *context.function.bir_function,
          param_index);
      if (!source.has_value()) {
        continue;
      }
      if (home->kind == prepare::PreparedValueHomeKind::StackSlot &&
          home->offset_bytes.has_value()) {
        if (param.is_byval) {
          lines = entry_formal_byval_aggregate_store_lines(param,
                                                           *source,
                                                           *home->offset_bytes);
        } else {
          lines = entry_formal_store_lines(*source, param.type, *home->offset_bytes);
        }
      } else if (!param.is_byval &&
                 home->kind == prepare::PreparedValueHomeKind::Register) {
        const auto destination = entry_formal_destination_register(*home, param.type);
        if (destination.has_value()) {
          lines = entry_formal_register_move_lines(*source, *destination);
          record_entry_formal_register_home(scalar_state, *home, param.type);
        }
      }
    }
    if (lines.empty()) {
      continue;
    }
    lowered.push_back(make_entry_formal_publication_instruction(
        context, param_index, std::move(lines)));
  }
  return lowered;
}

}  // namespace

module::BlockLoweringContext make_block_lowering_context(
    module::FunctionLoweringContext function,
    const prepare::PreparedControlFlowBlock& block,
    std::size_t block_index) {
  return module::BlockLoweringContext{
      .function = function,
      .control_flow_block = &block,
      .bir_block = find_bir_block(function, block),
      .block_index = block_index,
  };
}

InstructionDispatchResult dispatch_prepared_block(
    const module::BlockLoweringContext& context,
    module::MachineBlock& block,
    module::ModuleLoweringDiagnostics& diagnostics) {
  InstructionDispatchResult result;

  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr) {
    append_block_diagnostic(diagnostics,
                            module::ModuleLoweringDiagnosticKind::MissingBlockContext,
                            context,
                            "AArch64 block dispatch requires prepared function and block context");
    return result;
  }

  block.block_label = context.control_flow_block->block_label;
  block.index = context.block_index;
  block.successors.clear();

  if (context.bir_block == nullptr && context.function.bir_function != nullptr) {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingInstructionBlockMapping,
        context,
        "AArch64 block dispatch could not map prepared block to retained BIR instructions");
  }

  BlockScalarLoweringState scalar_state;
  record_current_block_entry_publication_registers(context, scalar_state);
  std::unordered_set<c4c::ValueNameId> published_store_global_stack_values;
  auto record_call_boundary_destination =
      [&](const module::MachineInstruction& instruction) {
    const auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record != nullptr && move_record->destination_register.has_value()) {
      record_emitted_scalar_register(scalar_state,
                                     move_record->destination_register->value_name,
                                     *move_record->destination_register);
    }
  };
  auto record_call_boundary_source_in_destination =
      [&](const module::MachineInstruction& instruction) {
    const auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record == nullptr || !move_record->destination_register.has_value()) {
      return;
    }
    std::optional<prepare::PreparedValueId> source_value_id;
    c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
    if (move_record->source_memory.has_value() &&
        move_record->source_memory->result_value_name.has_value()) {
      source_value_id = move_record->source_memory->result_value_id;
      source_value_name = *move_record->source_memory->result_value_name;
    } else if (move_record->source_register.has_value() &&
               move_record->source_register->value_name != c4c::kInvalidValueName) {
      source_value_id = move_record->source_register->value_id;
      source_value_name = move_record->source_register->value_name;
    }
    if (source_value_name == c4c::kInvalidValueName) {
      return;
    }
    auto source_alias = *move_record->destination_register;
    source_alias.value_id = source_value_id;
    source_alias.value_name = source_value_name;
    record_emitted_scalar_register(scalar_state, source_value_name, source_alias);
  };
  auto call_boundary_move_reloads_prepared_stack_source =
      [](const module::MachineInstruction& instruction) {
    const auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    return move_record != nullptr &&
           move_record->phase == prepare::PreparedMovePhase::BeforeInstruction &&
           move_record->source_memory.has_value() &&
           move_record->source_memory->support == MemoryOperandSupportKind::Prepared &&
           move_record->source_memory->base_kind == MemoryBaseKind::FrameSlot &&
           move_record->source_memory->byte_offset_is_prepared_snapshot;
  };

  auto retarget_call_boundary_source_to_emitted_scalar =
      [&](module::MachineInstruction& instruction) {
    auto* move_record =
        std::get_if<CallBoundaryMoveInstructionRecord>(&instruction.target.payload);
    if (move_record == nullptr) {
      return;
    }
    std::optional<c4c::ValueNameId> source_value_name;
    std::optional<prepare::PreparedValueId> source_value_id;
    if (move_record->source_memory.has_value() &&
        !move_record->source_memory_materializes_address &&
        move_record->source_memory->result_value_name.has_value()) {
      source_value_name = *move_record->source_memory->result_value_name;
      source_value_id = move_record->source_memory->result_value_id;
    } else if (move_record->source_register.has_value() &&
               move_record->source_register->value_name != c4c::kInvalidValueName) {
      source_value_name = move_record->source_register->value_name;
      source_value_id = move_record->source_register->value_id;
    } else if (move_record->source_register.has_value()) {
      source_value_id = move_record->source_register->value_id;
    }
    if (!source_value_name.has_value() && !source_value_id.has_value()) {
      return;
    }
    auto emitted = source_value_name.has_value()
                       ? find_emitted_scalar_register(scalar_state, *source_value_name)
                       : std::nullopt;
    if (!emitted.has_value() && source_value_id.has_value()) {
      const bool floating_preserved_source =
          move_record->source_register.has_value() &&
          move_record->source_register->reg.bank == abi::RegisterBank::FpSimd;
      if (floating_preserved_source) {
        for (const auto& [_, candidate] : scalar_state.emitted_registers) {
          if (candidate.value_id == source_value_id &&
              candidate.reg.bank == abi::RegisterBank::FpSimd) {
            emitted = candidate;
            break;
          }
        }
      }
    }
    if (!emitted.has_value()) {
      return;
    }
    move_record->source_register = *emitted;
    move_record->source_memory.reset();
    if (move_record->destination_register.has_value() &&
        emitted->reg.bank == abi::RegisterBank::GeneralPurpose &&
        move_record->destination_register->reg.bank == abi::RegisterBank::GeneralPurpose &&
        emitted->expected_view.has_value()) {
      const auto retargeted_destination =
          abi::gp_register(move_record->destination_register->reg.index,
                           *emitted->expected_view);
      if (retargeted_destination.has_value()) {
        move_record->destination_register->reg = *retargeted_destination;
        move_record->destination_register->expected_view = emitted->expected_view;
      }
    }
  };
  auto source_value_is_materialized_address =
      [](const CallBoundaryMoveInstructionRecord& move_record,
         const std::vector<module::MachineInstruction>& materialized_addresses) {
    for (const auto& materialized : materialized_addresses) {
      const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
      if (address_record == nullptr) {
        continue;
      }
      if (address_record->result_value_id.has_value() &&
          *address_record->result_value_id == move_record.move.from_value_id) {
        return true;
      }
      if (move_record.source_register.has_value() &&
          address_record->result_value_name != c4c::kInvalidValueName &&
          move_record.source_register->value_name == address_record->result_value_name) {
        return true;
      }
    }
    return false;
  };
  auto source_register_conflicts_with_materialized_address =
      [&](const CallBoundaryMoveInstructionRecord& move_record,
          const std::vector<module::MachineInstruction>& materialized_addresses) {
    if (!move_record.source_register.has_value() ||
        source_value_is_materialized_address(move_record, materialized_addresses)) {
      return false;
    }
    for (const auto& materialized : materialized_addresses) {
      const auto* address_record =
          std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
      if (address_record == nullptr || !address_record->result_register.has_value()) {
        continue;
      }
      if (registers_alias(*move_record.source_register,
                          *address_record->result_register)) {
        return true;
      }
    }
    return false;
  };
  for (auto& entry_formal : lower_entry_formal_publications(context, scalar_state)) {
    block.instructions.push_back(std::move(entry_formal));
  }
  for (auto& block_entry_move : lower_value_moves(
           context,
           prepare::PreparedMovePhase::BlockEntry,
           0,
           diagnostics)) {
    if (block_entry_move_clobbers_current_join_publication(context,
                                                           block_entry_move)) {
      continue;
    }
    if (const auto* move =
            std::get_if<CallBoundaryMoveInstructionRecord>(
                &block_entry_move.target.payload);
        move != nullptr &&
        move->authority_kind ==
            prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
        move->source_parallel_copy_predecessor_label ==
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} &&
        move->source_parallel_copy_successor_label.has_value() &&
        move->source_register.has_value() &&
        move->destination_register.has_value() &&
        registers_alias(*move->source_register, *move->destination_register)) {
      continue;
    }
    if (const auto* move =
            std::get_if<CallBoundaryMoveInstructionRecord>(
                &block_entry_move.target.payload);
        move != nullptr &&
        move->authority_kind ==
            prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
        move->source_parallel_copy_predecessor_label ==
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label} &&
        move->source_parallel_copy_successor_label.has_value() &&
        move->source_memory.has_value()) {
      continue;
    }
    record_call_boundary_destination(block_entry_move);
    block.instructions.push_back(std::move(block_entry_move));
  }
  if (context.bir_block != nullptr) {
    std::size_t prepared_memory_instruction_index = 0;
    for (std::size_t instruction_index = 0;
         instruction_index < context.bir_block->insts.size();
         ++instruction_index) {
      const auto& inst = context.bir_block->insts[instruction_index];
      const bool is_memory_inst =
          std::get_if<bir::LoadLocalInst>(&inst) != nullptr ||
          std::get_if<bir::LoadGlobalInst>(&inst) != nullptr ||
          std::get_if<bir::StoreLocalInst>(&inst) != nullptr ||
          std::get_if<bir::StoreGlobalInst>(&inst) != nullptr;
      const std::size_t memory_instruction_index =
          is_memory_inst ? ++prepared_memory_instruction_index : instruction_index;
      const auto* retained_index_access =
          is_memory_inst ? prepared_memory_access(context, instruction_index) : nullptr;
      const auto* prepared_index_access =
          is_memory_inst ? prepared_memory_access(context, memory_instruction_index) : nullptr;
      const bool use_prepared_memory_index =
          is_memory_inst &&
          memory_instruction_index != instruction_index &&
          !prepared_memory_access_matches_instruction(
              context, retained_index_access, inst) &&
          prepared_memory_access_matches_instruction(
              context, prepared_index_access, inst);
      const std::size_t memory_lowering_index =
          use_prepared_memory_index ? memory_instruction_index : instruction_index;
      const bool can_retry_prepared_memory_index =
          is_memory_inst &&
          memory_instruction_index != memory_lowering_index &&
          prepared_memory_access_matches_instruction(
              context, prepared_index_access, inst);
      if (std::get_if<bir::BinaryInst>(&inst) != nullptr) {
        const bool stack_home_fused_compare_branch =
            is_fused_compare_branch_support_instruction(context, inst, scalar_state) &&
            lower_stack_home_fused_compare_branch(context).has_value();
        for (auto& before_instruction_move : lower_value_moves(
                 context,
                 prepare::PreparedMovePhase::BeforeInstruction,
                 instruction_index,
                 diagnostics)) {
          if (block_entry_move_clobbers_current_join_publication(
                  context, before_instruction_move)) {
            continue;
          }
          if (!call_boundary_move_reloads_prepared_stack_source(
                  before_instruction_move)) {
            continue;
          }
          if (stack_home_fused_compare_branch) {
            continue;
          }
          record_call_boundary_destination(before_instruction_move);
          record_call_boundary_source_in_destination(before_instruction_move);
          block.instructions.push_back(std::move(before_instruction_move));
        }
      }
      if (const auto* call = std::get_if<bir::CallInst>(&inst)) {
        if (auto dynamic_stack = lower_dynamic_stack_helper_call(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*dynamic_stack));
          ++result.visited_operations;
          continue;
        }
        if (call->inline_asm.has_value() ||
            has_prepared_inline_asm_carrier(context, instruction_index)) {
          if (auto lowered = lower_inline_asm_instruction(
                  context, *call, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        if (has_prepared_intrinsic_carrier(context, instruction_index)) {
          if (auto lowered = lower_intrinsic_instruction(
                  context, instruction_index, diagnostics)) {
            block.instructions.push_back(std::move(*lowered));
          }
          ++result.visited_operations;
          continue;
        }
        const auto* call_plan = find_prepared_call_plan(context, instruction_index);
        if (call_plan != nullptr) {
          const bool inline_variadic_entry_helper =
              variadic_entry_helper_kind(call->callee).has_value();
          auto argument_producers =
              lower_scalar_call_argument_producers(context,
                                                   *call,
                                                   instruction_index,
                                                   scalar_state,
                                                   diagnostics);
          for (auto& argument_producer : argument_producers) {
            block.instructions.push_back(std::move(argument_producer));
          }
          for (auto& preserved_stack_publication :
               publish_stack_preserved_call_values(
                   context, *call_plan, instruction_index, scalar_state)) {
            block.instructions.push_back(std::move(preserved_stack_publication));
          }
          auto materialized_addresses =
              lower_address_materializations(context, instruction_index, diagnostics);
          auto before_call_moves =
              inline_variadic_entry_helper
                  ? std::vector<module::MachineInstruction>{}
                  : lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          for (auto& before_call_move : before_call_moves) {
            retarget_call_boundary_source_to_emitted_scalar(before_call_move);
          }
          std::vector<module::MachineInstruction> deferred_before_call_moves;
          for (auto& before_call_move : before_call_moves) {
            const auto* move_record =
                std::get_if<CallBoundaryMoveInstructionRecord>(
                    &before_call_move.target.payload);
            if (move_record != nullptr &&
                source_register_conflicts_with_materialized_address(
                    *move_record, materialized_addresses)) {
              record_call_boundary_destination(before_call_move);
              block.instructions.push_back(std::move(before_call_move));
            } else {
              deferred_before_call_moves.push_back(std::move(before_call_move));
            }
          }
          for (auto& materialized : materialized_addresses) {
            if (const auto* address_record =
                    std::get_if<AddressMaterializationRecord>(&materialized.target.payload);
                address_record != nullptr && address_record->result_register.has_value()) {
              record_emitted_scalar_register(scalar_state,
                                             address_record->result_value_name,
                                             *address_record->result_register);
            }
            block.instructions.push_back(std::move(materialized));
          }
          for (auto& before_call_move :
               order_before_call_moves_for_source_preservation(
                   std::move(deferred_before_call_moves))) {
            if (call_boundary_move_reloads_materialized_address(
                    before_call_move, materialized_addresses)) {
              continue;
            }
            retarget_call_boundary_source_to_emitted_scalar(before_call_move);
            if (auto materialized =
                    materialize_call_boundary_source_to_destination(
                        context, before_call_move, instruction_index, scalar_state)) {
              block.instructions.push_back(std::move(*materialized));
            } else {
              record_call_boundary_destination(before_call_move);
              block.instructions.push_back(std::move(before_call_move));
            }
          }
          if (!inline_variadic_entry_helper) {
            for (auto& materialized_argument :
                 materialize_missing_frame_slot_call_arguments(
                     context, *call_plan, instruction_index, scalar_state)) {
              block.instructions.push_back(std::move(materialized_argument));
            }
          }
        }
        if (auto lowered = lower_call_instruction(
                context, *call, instruction_index, diagnostics)) {
          block.instructions.push_back(std::move(*lowered));
          clear_call_clobbered_emitted_scalar_registers(scalar_state);
          if (call_plan != nullptr) {
            record_call_result_source_register(context, *call_plan, scalar_state);
            auto after_call_moves =
                lower_after_call_moves(context, *call_plan, instruction_index, diagnostics);
            for (auto& after_call_move : after_call_moves) {
              record_call_boundary_destination(after_call_move);
              block.instructions.push_back(std::move(after_call_move));
            }
          }
        }
      } else if (lower_store_local_with_address_materialization(
                     context,
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (is_current_block_join_parallel_copy_incoming_expression(
                     context, inst)) {
        continue;
      } else if (lower_scalar_with_address_materialization(
                     context,
                     inst,
                     instruction_index,
                     scalar_state,
                     block,
                     diagnostics)) {
        ++result.visited_operations;
        continue;
      } else if (auto lowered = lower_address_materialization(
                     context, instruction_index, diagnostics)) {
        record_address_materialization_result(scalar_state, *lowered);
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_mul =
                     lower_scalar_mul_with_distinct_rhs_scratch(
                         context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered_mul));
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_pair =
                     lower_i128_pair_operation_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_i128_pair.handled) {
        if (lowered_i128_pair.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_pair.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_i128_copy =
                     lower_i128_copy_instruction(context, inst, instruction_index, diagnostics);
                 lowered_i128_copy.handled) {
        if (lowered_i128_copy.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_i128_copy.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (auto lowered_f128_helper =
                     lower_f128_runtime_helper_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_helper.handled) {
        if (lowered_f128_helper.instruction.has_value()) {
          block.instructions.push_back(std::move(*lowered_f128_helper.instruction));
        }
        ++result.visited_operations;
        continue;
      } else if (const auto* load_global = std::get_if<bir::LoadGlobalInst>(&inst);
                 load_global != nullptr) {
        if (auto got_load =
                make_load_global_got_materialization_instruction(
                    context, instruction_index, *load_global, scalar_state)) {
          block.instructions.push_back(std::move(*got_load));
        } else if (auto lowered_ordinary_memory =
                       lower_memory_instruction(context, inst, memory_lowering_index, diagnostics);
                   lowered_ordinary_memory.handled) {
          if (!lowered_ordinary_memory.instruction.has_value() &&
              can_retry_prepared_memory_index &&
              memory_instruction_index != memory_lowering_index) {
            lowered_ordinary_memory =
                lower_memory_instruction(context, inst, memory_instruction_index, diagnostics);
          }
          if (lowered_ordinary_memory.instruction.has_value()) {
            retarget_memory_result_to_prepared_home(
                context, *lowered_ordinary_memory.instruction);
            record_memory_result(scalar_state, *lowered_ordinary_memory.instruction);
            block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
          }
        } else {
          append_unsupported_instruction_diagnostic(
              diagnostics, context, inst, instruction_index);
        }
      } else if (auto lowered = lower_prepared_scalar_float_alu_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_fused_compare_branch_support_instruction(context, inst, scalar_state)) {
        continue;
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_register(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_stack(
              context, inst, instruction_index)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_current_block_join_parallel_copy_source(context, inst)) {
        continue;
      } else if (auto lowered =
                     lower_stack_homed_pointer_value_load_publication(
                         context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered_f128_transport =
                     lower_f128_transport_instruction(
                         context, inst, instruction_index, diagnostics);
                 lowered_f128_transport.handled) {
        if (lowered_f128_transport.instruction.has_value()) {
          retarget_memory_result_to_prepared_home(
              context, *lowered_f128_transport.instruction);
          record_memory_result(scalar_state, *lowered_f128_transport.instruction);
          block.instructions.push_back(std::move(*lowered_f128_transport.instruction));
        }
      } else if (auto lowered = lower_scalar_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_control_value_instruction(
              context, inst, instruction_index, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*lowered));
      } else {
        auto lowered_i128_transport =
            lower_i128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_i128_transport.handled) {
          if (lowered_i128_transport.instruction.has_value()) {
            block.instructions.push_back(std::move(*lowered_i128_transport.instruction));
          }
          ++result.visited_operations;
          continue;
        }
        auto lowered_memory =
            lower_f128_transport_instruction(context, inst, instruction_index, diagnostics);
        if (lowered_memory.handled) {
          if (lowered_memory.instruction.has_value()) {
            retarget_memory_result_to_prepared_home(context, *lowered_memory.instruction);
            record_memory_result(scalar_state, *lowered_memory.instruction);
            block.instructions.push_back(std::move(*lowered_memory.instruction));
          }
        } else {
          if (std::get_if<bir::StoreGlobalInst>(&inst) != nullptr) {
            lower_pending_store_global_stack_value_publications(
                context,
                instruction_index,
                published_store_global_stack_values,
                block);
          }
          const auto diagnostic_count = diagnostics.entries.size();
          auto lowered_ordinary_memory =
              lower_memory_instruction(context, inst, memory_lowering_index, diagnostics);
          if (lowered_ordinary_memory.handled) {
            if (!lowered_ordinary_memory.instruction.has_value() &&
                can_retry_prepared_memory_index &&
                memory_instruction_index != memory_lowering_index) {
              lowered_ordinary_memory =
                  lower_memory_instruction(context, inst, memory_instruction_index, diagnostics);
            }
            if (lowered_ordinary_memory.instruction.has_value()) {
              retarget_pointer_store_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              retarget_store_local_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              retarget_fpr_call_result_store_value_to_emitted_scalar(
                  context, inst, scalar_state, *lowered_ordinary_memory.instruction);
              if (auto value_publication =
                      lower_store_local_value_publication(
                          context,
                          inst,
                          instruction_index,
                          *lowered_ordinary_memory.instruction,
                          scalar_state,
                          block)) {
                block.instructions.push_back(std::move(*value_publication));
              }
              if (auto value_publication =
                      lower_store_global_value_publication(
                          context,
                          inst,
                          instruction_index,
                          *lowered_ordinary_memory.instruction,
                          &published_store_global_stack_values)) {
                block.instructions.push_back(std::move(*value_publication));
              }
              retarget_memory_result_to_prepared_home(
                  context, *lowered_ordinary_memory.instruction);
              record_memory_result(scalar_state, *lowered_ordinary_memory.instruction);
              block.instructions.push_back(std::move(*lowered_ordinary_memory.instruction));
            } else if (auto pointer_store =
                           lower_stack_homed_pointer_store_writeback(
                               context, inst, memory_lowering_index)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*pointer_store));
            } else if (auto formal_store =
                           lower_fixed_formal_store_local_publication(
                               context, inst, memory_lowering_index, scalar_state)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*formal_store));
            }
          } else {
            append_unsupported_instruction_diagnostic(
                diagnostics, context, inst, instruction_index);
          }
        }
      }
      ++result.visited_operations;
    }
  }

  auto lowered_atomic_operations =
      lower_atomic_memory_operations_for_block(context, diagnostics);
  result.visited_operations += lowered_atomic_operations.size();
  for (auto& atomic_instruction : lowered_atomic_operations) {
    block.instructions.push_back(std::move(atomic_instruction));
  }

  result.visited_terminator = true;
  if (context.control_flow_block->terminator_kind ==
      c4c::backend::bir::TerminatorKind::Return) {
    const std::size_t return_instruction_index =
        context.bir_block != nullptr ? context.bir_block->insts.size() : 0;
    for (auto& before_return_move :
         lower_before_return_moves(context, return_instruction_index, diagnostics)) {
      if (before_return_publication_already_emitted(scalar_state,
                                                    before_return_move)) {
        continue;
      }
      record_before_return_publication(scalar_state, before_return_move);
      block.instructions.push_back(std::move(before_return_move));
    }
    if (auto lowered =
            lower_prepared_return_terminator(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::Branch) {
    for (auto& edge_source :
         lower_predecessor_select_parallel_copy_sources(context, scalar_state, diagnostics)) {
      block.instructions.push_back(std::move(edge_source));
    }
    if (auto lowered = lower_prepared_branch_terminator(context, diagnostics)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors.push_back(make_unconditional_branch_successor(context));
    }
  } else if (context.control_flow_block->terminator_kind ==
             c4c::backend::bir::TerminatorKind::CondBranch) {
    if (fused_compare_uses_selected_operand(context)) {
      for (auto& publication :
           lower_missing_fused_compare_operand_publications(
               context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(publication));
      }
    }
    if (auto lowered =
            lower_current_block_entry_fused_compare_branch(context)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_stack_home_fused_compare_branch(context)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
            lower_fused_compare_branch_from_emitted_cast(context, scalar_state)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_materialized_compare_condition_branch(context, scalar_state)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else {
      for (auto& publication :
           lower_missing_fused_compare_operand_publications(
               context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(publication));
      }
      if (auto publication =
              lower_missing_conditional_branch_condition_publication(
                  context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(*publication));
      }
      if (auto lowered = lower_conditional_branch_from_emitted_condition(
              context, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
        block.successors = make_conditional_branch_successors(context);
      } else if (auto lowered =
              lower_prepared_conditional_branch_terminator(
                  context, diagnostics, &scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
        block.successors = make_conditional_branch_successors(context);
      }
    }
  } else {
    append_block_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
        context,
        unsupported_terminator_message(context.control_flow_block->terminator_kind));
  }

  result.emitted_instructions = block.instructions.size();
  return result;
}

}  // namespace c4c::backend::aarch64::codegen

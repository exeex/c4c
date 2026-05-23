#include "dispatch.hpp"
#include "dispatch_branch_fusion.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch_dynamic_stack.hpp"
#include "dispatch_entry_formals.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_publication.hpp"

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
#include <unordered_map>
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











struct SameBlockSelectProducer {
  const bir::SelectInst* select = nullptr;
  std::size_t instruction_index = 0;
};

























struct LocalAggregateAddressPublication {
  module::MachineInstruction instruction;
  RegisterOperand result_register;
};

[[nodiscard]] std::optional<LocalAggregateAddressPublication>
materialize_local_aggregate_address_call_argument(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }
  auto result_register = make_named_prepared_result_register(context, value);
  if (!result_register.has_value() ||
      !abi::is_gp_register(result_register->reg)) {
    return std::nullopt;
  }
  const auto offset =
      local_aggregate_address_frame_offset(context, result_register->value_name);
  if (!offset.has_value()) {
    return std::nullopt;
  }
  const auto address_register = abi::x_register(result_register->reg.index);
  result_register->reg = address_register;
  result_register->expected_view = abi::RegisterView::X;
  std::vector<std::string> lines;
  const std::string_view base =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "x29"
          : "sp";
  std::string line = "add " + std::string{abi::register_name(address_register)} +
                     ", " + std::string{base} + ", #" + std::to_string(*offset);
  lines.push_back(std::move(line));
  auto instruction =
      make_select_chain_materialization_instruction(
          context, before_instruction_index, std::move(lines));
  if (!instruction.has_value()) {
    return std::nullopt;
  }
  return LocalAggregateAddressPublication{.instruction = std::move(*instruction),
                                          .result_register = *result_register};
}

[[nodiscard]] bool call_argument_allows_local_aggregate_address_publication(
    const bir::CallInst& call,
    std::size_t argument_index) {
  if (argument_index >= call.args.size()) {
    return false;
  }
  if (call.callee.rfind("llvm.", 0) == 0) {
    return false;
  }
  if (argument_index < call.arg_abi.size() &&
      call.arg_abi[argument_index].byval_copy) {
    return false;
  }
  return (argument_index < call.arg_types.size() &&
          call.arg_types[argument_index] == bir::TypeKind::Ptr) ||
         call.args[argument_index].type == bir::TypeKind::Ptr;
}

[[nodiscard]] bool materialize_scalar_call_argument_value(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    bool allow_local_aggregate_address_publication,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    std::vector<module::MachineInstruction>& lowered,
    std::vector<std::string_view>& active_values) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return true;
  }
  for (const auto active : active_values) {
    if (active == value.name) {
      return false;
    }
  }
  if (allow_local_aggregate_address_publication) {
    if (auto local_address =
            materialize_local_aggregate_address_call_argument(
                context, value, before_instruction_index)) {
      record_emitted_scalar_register(scalar_state,
                                     local_address->result_register.value_name,
                                     local_address->result_register);
      lowered.push_back(std::move(local_address->instruction));
      return true;
    }
  }
  if (emitted_scalar_value_available(context, value, scalar_state)) {
    return true;
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
	                                             allow_local_aggregate_address_publication,
	                                             scalar_state,
	                                             diagnostics,
	                                             lowered,
                                             active_values);
  const bool rhs_ready =
      materialize_scalar_call_argument_value(context,
	                                             binary->rhs,
	                                             *producer_index,
	                                             allow_local_aggregate_address_publication,
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
  for (std::size_t argument_index = 0; argument_index < call.args.size(); ++argument_index) {
    const auto& argument = call.args[argument_index];
    if (auto select_chain =
            materialize_direct_global_select_chain_call_argument(context,
                                                                 argument,
                                                                 instruction_index,
                                                                 scalar_state)) {
      lowered.push_back(std::move(*select_chain));
      continue;
    }
    std::vector<std::string_view> active_values;
    const bool allow_local_aggregate_address_publication =
        call_argument_allows_local_aggregate_address_publication(call, argument_index);
    if (!materialize_scalar_call_argument_value(context,
                                                argument,
                                                instruction_index,
                                                allow_local_aggregate_address_publication,
                                                scalar_state,
                                                diagnostics,
                                                lowered,
                                                active_values)) {
      return {};
    }
  }
  return lowered;
}







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
      find_value_home(context,
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
          find_value_home(context,
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
    const auto* result = instruction_result_value_ref(context.bir_block->insts[index - 1]);
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
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
  auto operand_depends = [&](const bir::Value& operand) {
    return operand.kind == bir::Value::Kind::Named &&
           !operand.name.empty() &&
           (operand.name == dependency_name ||
            same_block_result_depends_on_value(context,
                                               operand.name,
                                               dependency_name,
                                               *producer_index,
                                               depth + 1U));
  };
  return std::visit(
      [&](const auto& typed_inst) {
        using T = std::decay_t<decltype(typed_inst)>;
        if constexpr (std::is_same_v<T, bir::BinaryInst>) {
          return operand_depends(typed_inst.lhs) ||
                 operand_depends(typed_inst.rhs);
        } else if constexpr (std::is_same_v<T, bir::CastInst>) {
          return operand_depends(typed_inst.operand);
        } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
          return operand_depends(typed_inst.lhs) ||
                 operand_depends(typed_inst.rhs) ||
                 operand_depends(typed_inst.true_value) ||
                 operand_depends(typed_inst.false_value);
        }
        return false;
      },
      producer);
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
      find_value_home(context,
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
          find_value_home(context,
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

struct CurrentBlockJoinParallelCopyCache {
  const module::BlockLoweringContext* context = nullptr;
  std::vector<bool> incoming_expressions;
  std::vector<bool> sources;
};

[[nodiscard]] CurrentBlockJoinParallelCopyCache
build_current_block_join_parallel_copy_cache(
    const module::BlockLoweringContext& context) {
  CurrentBlockJoinParallelCopyCache cache{.context = &context};
  if (context.bir_block == nullptr) {
    return cache;
  }
  const bool has_relevant_bundle =
      context.function.value_locations != nullptr &&
      context.control_flow_block != nullptr &&
      std::any_of(context.function.value_locations->move_bundles.begin(),
                  context.function.value_locations->move_bundles.end(),
                  [&](const prepare::PreparedMoveBundle& bundle) {
                    return bundle.phase == prepare::PreparedMovePhase::BlockEntry &&
                           bundle.authority_kind ==
                               prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy &&
                           bundle.source_parallel_copy_successor_label ==
                               std::optional<c4c::BlockLabelId>{
                                   context.control_flow_block->block_label};
                  });
  cache.incoming_expressions.reserve(context.bir_block->insts.size());
  cache.sources.reserve(context.bir_block->insts.size());
  if (!has_relevant_bundle) {
    cache.incoming_expressions.assign(context.bir_block->insts.size(), false);
    cache.sources.assign(context.bir_block->insts.size(), false);
    return cache;
  }
  std::unordered_map<std::string_view, std::size_t> result_indices;
  result_indices.reserve(context.bir_block->insts.size());
  for (std::size_t index = 0; index < context.bir_block->insts.size(); ++index) {
    const auto* result = instruction_result_value_ref(context.bir_block->insts[index]);
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
        !result->name.empty()) {
      result_indices.emplace(result->name, index);
    }
  }

  std::unordered_set<prepare::PreparedValueId> incoming_value_ids;
  std::unordered_set<prepare::PreparedValueId> source_value_ids;
  std::unordered_set<std::string_view> incoming_expression_names;
  std::vector<std::string_view> pending_expression_names;
  auto add_expression_dependency = [&](std::string_view name) {
    if (!name.empty()) {
      pending_expression_names.push_back(name);
    }
  };

  for (const auto& bundle : context.function.value_locations->move_bundles) {
    if (bundle.phase != prepare::PreparedMovePhase::BlockEntry ||
        bundle.authority_kind != prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        bundle.source_parallel_copy_successor_label !=
            std::optional<c4c::BlockLabelId>{context.control_flow_block->block_label}) {
      continue;
    }
    for (const auto& move : bundle.moves) {
      if (move.op_kind != prepare::PreparedMoveResolutionOpKind::Move ||
          move.destination_kind != prepare::PreparedMoveDestinationKind::Value) {
        continue;
      }
      if (!move.source_immediate_i32.has_value()) {
        const auto* source_home =
            find_value_home(context,
move.from_value_id);
        if (source_home != nullptr &&
            source_home->value_name != c4c::kInvalidValueName) {
          incoming_value_ids.insert(source_home->value_id);
          add_expression_dependency(prepare::prepared_value_name(
              context.function.prepared->names, source_home->value_name));
        }
      }
      if (move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register) {
        continue;
      }
      source_value_ids.insert(move.to_value_id);
      if (move.source_immediate_i32.has_value() ||
          move.from_value_id == move.to_value_id) {
        continue;
      }
      const auto* result_home =
          find_value_home(context,
move.from_value_id);
      const auto* destination_home =
          find_value_home(context,
move.to_value_id);
      if (result_home != nullptr && destination_home != nullptr &&
          (prepared_edge_select_source_is_destination_register(*result_home,
                                                              *destination_home) ||
           result_home->kind == prepare::PreparedValueHomeKind::StackSlot)) {
        source_value_ids.insert(move.from_value_id);
      }
    }
  }

  while (!pending_expression_names.empty()) {
    const auto name = pending_expression_names.back();
    pending_expression_names.pop_back();
    if (name.empty() || !incoming_expression_names.insert(name).second) {
      continue;
    }
    const auto producer_it = result_indices.find(name);
    if (producer_it == result_indices.end()) {
      continue;
    }
    const auto& producer = context.bir_block->insts[producer_it->second];
    if (!is_join_parallel_copy_expression_instruction(producer)) {
      continue;
    }
    for (const auto operand_name : named_operands_of_instruction(producer)) {
      add_expression_dependency(operand_name);
    }
  }

  for (const auto& inst : context.bir_block->insts) {
    const auto* result = instruction_result_value_ref(inst);
    bool incoming_expression = false;
    bool source = false;
    if (result != nullptr && result->kind == bir::Value::Kind::Named &&
        !result->name.empty()) {
      const auto result_value_name = prepared_named_value_id(context, *result);
      const auto* result_home =
          result_value_name.has_value()
              ? find_value_home(context,
*result_value_name)
              : nullptr;
      if (result_home != nullptr && result_home->value_name != c4c::kInvalidValueName) {
        incoming_expression =
            incoming_value_ids.find(result_home->value_id) != incoming_value_ids.end() ||
            incoming_expression_names.find(result->name) != incoming_expression_names.end();
        source = source_value_ids.find(result_home->value_id) != source_value_ids.end();
      }
    }
    cache.incoming_expressions.push_back(incoming_expression);
    cache.sources.push_back(source);
  }
  return cache;
}

[[nodiscard]] bool cached_current_block_join_parallel_copy_incoming_expression(
    const CurrentBlockJoinParallelCopyCache& cache,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (cache.context == &context &&
      instruction_index < cache.incoming_expressions.size()) {
    return cache.incoming_expressions[instruction_index];
  }
  return is_current_block_join_parallel_copy_incoming_expression(context, inst);
}

[[nodiscard]] bool cached_current_block_join_parallel_copy_source(
    const CurrentBlockJoinParallelCopyCache& cache,
    const module::BlockLoweringContext& context,
    std::size_t instruction_index,
    const bir::Inst& inst) {
  if (cache.context == &context && instruction_index < cache.sources.size()) {
    return cache.sources[instruction_index];
  }
  return is_current_block_join_parallel_copy_source(context, inst);
}





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
        find_value_home(context,
move.from_value_id);
    const auto* destination_home =
        find_value_home(context,
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




[[nodiscard]] std::optional<module::MachineInstruction>
lower_scalar_mul_with_distinct_rhs_scratch(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state);









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
      find_value_home(context,
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







































































struct NarrowLocalStorePublication {
  bir::Value stored_value;
  std::size_t instruction_index = 0;
};



























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
            ? find_value_home(context,
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
                      frame_slot_address(context.function, *result_stack_offset_bytes));
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
                    frame_slot_address(context.function, *result_stack_offset_bytes));
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



struct EdgeProducerContext {
  module::BlockLoweringContext context;
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
};























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
    lines.push_back("str " + target + ", " + frame_slot_address(context.function, *home->offset_bytes));
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
  if (auto pointer_address_store =
          lower_pointer_base_plus_offset_store_local_publication(
              context, inst, instruction_index)) {
    block.instructions.push_back(std::move(*pointer_address_store));
    return true;
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
    if (auto pointer_address_store =
            lower_pointer_base_plus_offset_store_local_publication(
                context, inst, instruction_index)) {
      diagnostics.entries.resize(diagnostic_count);
      block.instructions.push_back(std::move(*pointer_address_store));
      return lowered_memory.handled;
    }
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
    const BlockAddressMaterializationIndex& address_materializations,
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
      lower_address_materializations(
          context, address_materializations, instruction_index, diagnostics);
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

[[nodiscard]] c4c::SlotNameId local_load_effective_slot_id(
    const bir::LoadLocalInst& load) {
  if (load.address.has_value() &&
      load.address->base_slot_id != c4c::kInvalidSlotName) {
    return load.address->base_slot_id;
  }
  return load.slot_id;
}

[[nodiscard]] c4c::SlotNameId local_store_effective_slot_id(
    const bir::StoreLocalInst& store) {
  if (store.address.has_value() &&
      store.address->base_slot_id != c4c::kInvalidSlotName) {
    return store.address->base_slot_id;
  }
  return store.slot_id;
}

[[nodiscard]] std::int64_t local_load_effective_byte_offset(
    const bir::LoadLocalInst& load) {
  return load.address.has_value()
             ? load.address->byte_offset
             : static_cast<std::int64_t>(load.byte_offset);
}

[[nodiscard]] std::int64_t local_store_effective_byte_offset(
    const bir::StoreLocalInst& store) {
  return store.address.has_value()
             ? store.address->byte_offset
             : static_cast<std::int64_t>(store.byte_offset);
}

[[nodiscard]] bool same_local_scalar_slot_access(
    const bir::LoadLocalInst& load,
    const bir::StoreLocalInst& store) {
  const auto load_slot_id = local_load_effective_slot_id(load);
  const auto store_slot_id = local_store_effective_slot_id(store);
  if (load_slot_id != c4c::kInvalidSlotName ||
      store_slot_id != c4c::kInvalidSlotName) {
    if (load_slot_id == c4c::kInvalidSlotName ||
        store_slot_id == c4c::kInvalidSlotName ||
        load_slot_id != store_slot_id) {
      return false;
    }
  } else if (!load.slot_name.empty() || !store.slot_name.empty()) {
    if (load.slot_name.empty() || store.slot_name.empty() ||
        load.slot_name != store.slot_name) {
      return false;
    }
  }

  const auto load_size = dispatch_publication_scalar_type_size_bytes(load.result.type);
  const auto store_size = dispatch_publication_scalar_type_size_bytes(store.value.type);
  return load_size.has_value() &&
         store_size.has_value() &&
         *load_size == *store_size &&
         local_load_effective_byte_offset(load) ==
             local_store_effective_byte_offset(store);
}

struct LocalLoadStoredValue {
  bir::Value value;
  std::size_t store_instruction_index = 0;
};

[[nodiscard]] std::optional<LocalLoadStoredValue>
find_latest_same_block_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::LoadLocalInst& load,
    std::size_t load_instruction_index) {
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const std::size_t limit =
      std::min(load_instruction_index, context.bir_block->insts.size());
  for (std::size_t index = limit; index > 0; --index) {
    const auto store_index = index - 1;
    const auto* store =
        std::get_if<bir::StoreLocalInst>(&context.bir_block->insts[store_index]);
    if (store != nullptr && same_local_scalar_slot_access(load, *store)) {
      return LocalLoadStoredValue{
          .value = store->value,
          .store_instruction_index = store_index,
      };
    }
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<LocalLoadStoredValue>
resolve_indirect_callee_local_load_store(
    const module::BlockLoweringContext& context,
    const bir::Value& callee_value,
    std::size_t call_instruction_index) {
  if (callee_value.kind != bir::Value::Kind::Named || callee_value.name.empty()) {
    return std::nullopt;
  }
  const auto* producer =
      find_same_block_named_producer(context, callee_value.name, call_instruction_index);
  const auto* load = producer != nullptr ? std::get_if<bir::LoadLocalInst>(producer)
                                         : nullptr;
  const auto load_index = producer_instruction_index(context, producer);
  if (load == nullptr || !load_index.has_value()) {
    return std::nullopt;
  }
  auto stored =
      find_latest_same_block_local_load_store(context, *load, *load_index);
  if (!stored.has_value()) {
    return std::nullopt;
  }
  return stored;
}

[[nodiscard]] std::vector<std::uint8_t> indirect_callee_materialization_scratch_indices() {
  std::vector<std::uint8_t> indices;
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    indices.push_back(scratch.index);
  }
  for (const auto scratch : abi::indirect_call_scratch_registers()) {
    indices.push_back(scratch.index);
  }
  return indices;
}

[[nodiscard]] bool scratch_index_is_available(
    std::uint8_t candidate,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  return candidate != target_index &&
         std::find(occupied_indices.begin(), occupied_indices.end(), candidate) ==
             occupied_indices.end();
}

[[nodiscard]] std::optional<std::uint8_t> choose_scratch_index(
    const std::vector<std::uint8_t>& scratch_indices,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& occupied_indices) {
  for (const auto candidate : scratch_indices) {
    if (scratch_index_is_available(candidate, target_index, occupied_indices)) {
      return candidate;
    }
  }
  return std::nullopt;
}

[[nodiscard]] bool emit_indirect_callee_value_to_register_with_csel(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index,
    std::uint8_t target_index,
    const std::vector<std::uint8_t>& scratch_indices,
    std::vector<std::uint8_t> occupied_indices,
    std::vector<std::string>& lines,
    unsigned depth = 0) {
  if (depth > 16U) {
    return false;
  }
  const auto* producer =
      value.kind == bir::Value::Kind::Named
          ? find_same_block_named_producer(context, value.name, before_instruction_index)
          : nullptr;
  const auto* select =
      producer != nullptr ? std::get_if<bir::SelectInst>(producer) : nullptr;
  if (select == nullptr) {
    const auto scratch_index =
        choose_scratch_index(scratch_indices, target_index, occupied_indices);
    return scratch_index.has_value() &&
           emit_value_publication_to_register(context,
                                              value,
                                              before_instruction_index,
                                              target_index,
                                              *scratch_index,
                                              lines,
                                              true);
  }

  const auto condition = branch_condition_suffix(select->predicate);
  const auto compare_view = scalar_view_for_type(select->compare_type);
  const auto result_view = scalar_view_for_type(value.type);
  if (!condition.has_value() || !compare_view.has_value() ||
      !result_view.has_value()) {
    return false;
  }

  const auto producer_index =
      producer_instruction_index(context, producer).value_or(before_instruction_index);
  const auto true_index =
      choose_scratch_index(scratch_indices, target_index, occupied_indices);
  if (!true_index.has_value()) {
    return false;
  }

  auto false_value = select->false_value;
  false_value.type = value.type;
  auto true_value = select->true_value;
  true_value.type = value.type;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        false_value,
                                                        producer_index,
                                                        target_index,
                                                        scratch_indices,
                                                        occupied_indices,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto true_occupied = occupied_indices;
  true_occupied.push_back(target_index);
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        true_value,
                                                        producer_index,
                                                        *true_index,
                                                        scratch_indices,
                                                        true_occupied,
                                                        lines,
                                                        depth + 1)) {
    return false;
  }

  auto compare_occupied = occupied_indices;
  compare_occupied.push_back(target_index);
  compare_occupied.push_back(*true_index);
  const auto lhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!lhs_index.has_value()) {
    return false;
  }
  compare_occupied.push_back(*lhs_index);
  const auto rhs_index =
      choose_scratch_index(scratch_indices, target_index, compare_occupied);
  if (!rhs_index.has_value()) {
    return false;
  }

  auto lhs = select->lhs;
  lhs.type = select->compare_type;
  if (!emit_value_publication_to_register(context,
                                          lhs,
                                          producer_index,
                                          *lhs_index,
                                          *rhs_index,
                                          lines,
                                          true)) {
    return false;
  }
  std::string rhs_name;
  if (select->rhs.kind == bir::Value::Kind::Immediate &&
      is_cmp_immediate_encodable(select->rhs.immediate)) {
    rhs_name = "#" + std::to_string(select->rhs.immediate);
  } else {
    auto rhs = select->rhs;
    rhs.type = select->compare_type;
    if (!emit_value_publication_to_register(context,
                                            rhs,
                                            producer_index,
                                            *rhs_index,
                                            *lhs_index,
                                            lines,
                                            true)) {
      return false;
    }
    const auto rhs_register = abi::gp_register(*rhs_index, *compare_view);
    if (!rhs_register.has_value()) {
      return false;
    }
    rhs_name = abi::register_name(*rhs_register);
  }

  const auto lhs_register = abi::gp_register(*lhs_index, *compare_view);
  const auto target_register = abi::gp_register(target_index, *result_view);
  const auto true_register = abi::gp_register(*true_index, *result_view);
  if (!lhs_register.has_value() || !target_register.has_value() ||
      !true_register.has_value()) {
    return false;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_register)} + ", " +
                  rhs_name);
  lines.push_back("csel " + std::string{abi::register_name(*target_register)} + ", " +
                  std::string{abi::register_name(*true_register)} + ", " +
                  std::string{abi::register_name(*target_register)} + ", " +
                  std::string{*condition});
  return true;
}

[[nodiscard]] std::optional<module::MachineInstruction>
materialize_indirect_call_callee_to_prepared_register(
    const module::BlockLoweringContext& context,
    const bir::CallInst& call,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    BlockScalarLoweringState& scalar_state) {
  if (!call.is_indirect || !call.callee_value.has_value() ||
      !call_plan.is_indirect || !call_plan.indirect_callee.has_value()) {
    return std::nullopt;
  }
  const auto& callee = *call_plan.indirect_callee;
  if (callee.value_name == c4c::kInvalidValueName ||
      callee.encoding != prepare::PreparedStorageEncodingKind::Register ||
      callee.bank != prepare::PreparedRegisterBank::Gpr ||
      !callee.register_name.has_value()) {
    return std::nullopt;
  }

  bir::Value source_value = *call.callee_value;
  std::size_t source_before_index = instruction_index;
  if (const auto stored = resolve_indirect_callee_local_load_store(
          context, *call.callee_value, instruction_index);
      stored.has_value() &&
      select_chain_contains_direct_global_load(
          context, stored->value, stored->store_instruction_index)) {
    source_value = stored->value;
    source_before_index = stored->store_instruction_index;
  }

  const auto target_register = abi::parse_aarch64_register_name(*callee.register_name);
  if (!target_register.has_value() ||
      target_register->bank != abi::RegisterBank::GeneralPurpose) {
    return std::nullopt;
  }
  const auto result_register = abi::gp_register(target_register->index, abi::RegisterView::X);
  if (!result_register.has_value()) {
    return std::nullopt;
  }
  const auto scratches = indirect_callee_materialization_scratch_indices();
  std::vector<std::string> lines;
  if (!emit_indirect_callee_value_to_register_with_csel(context,
                                                        source_value,
                                                        source_before_index,
                                                        result_register->index,
                                                        scratches,
                                                        {},
                                                        lines) ||
      lines.empty()) {
    return std::nullopt;
  }

  RegisterOperand emitted{
      .reg = *result_register,
      .role = RegisterOperandRole::CallAbi,
      .value_id = callee.value_id,
      .value_name = callee.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = prepare::PreparedRegisterBank::Gpr,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = 1,
      .occupied_register_references = {*result_register},
      .occupied_registers = {abi::register_name(*result_register)},
  };
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
            find_value_home(context,
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
      find_value_home(context,
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
        find_value_home(context,
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

[[nodiscard]] const std::vector<const prepare::PreparedCallPreservedValue*>*
first_stack_preserved_values_for_call(
    const module::PreparedCallPlanIndexes& call_plan_indexes,
    const prepare::PreparedCallPlansFunction& call_plans,
    const prepare::PreparedCallPlan& current_call_plan) {
  if (call_plans.calls.empty()) {
    return nullptr;
  }
  const auto* begin = call_plans.calls.data();
  const auto* end = begin + call_plans.calls.size();
  const auto* current = &current_call_plan;
  if (current >= begin && current < end) {
    return &call_plan_indexes.first_stack_preserved_by_call_index
                [static_cast<std::size_t>(current - begin)];
  }
  for (std::size_t call_index = 0; call_index < call_plans.calls.size(); ++call_index) {
    const auto& call = call_plans.calls[call_index];
    if (call.block_index == current_call_plan.block_index &&
        call.instruction_index == current_call_plan.instruction_index) {
      return &call_plan_indexes.first_stack_preserved_by_call_index[call_index];
    }
  }
  return nullptr;
}

[[nodiscard]] std::vector<const prepare::PreparedCallPreservedValue*>
first_stack_preserved_values_for_call_fallback(
    const prepare::PreparedCallPlansFunction& call_plans,
    const prepare::PreparedCallPlan& current_call_plan) {
  std::vector<const prepare::PreparedCallPreservedValue*> values;
  std::vector<unsigned char> seen_stack_values;
  for (const auto& call : call_plans.calls) {
    const bool is_current = call.block_index == current_call_plan.block_index &&
                            call.instruction_index == current_call_plan.instruction_index;
    for (const auto& preserved : call.preserved_values) {
      if (preserved.route != prepare::PreparedCallPreservationRoute::StackSlot ||
          preserved.value_name == c4c::kInvalidValueName ||
          !preserved.slot_id.has_value() ||
          !preserved.stack_offset_bytes.has_value() ||
          !preserved.stack_size_bytes.has_value() ||
          *preserved.stack_size_bytes == 0) {
        continue;
      }
      if (preserved.value_id >= seen_stack_values.size()) {
        seen_stack_values.resize(preserved.value_id + 1U, 0U);
      }
      if (seen_stack_values[preserved.value_id] != 0U) {
        continue;
      }
      seen_stack_values[preserved.value_id] = 1U;
      if (is_current) {
        values.push_back(&preserved);
      }
    }
    if (is_current) {
      break;
    }
  }
  return values;
}

[[nodiscard]] std::vector<module::MachineInstruction>
publish_stack_preserved_call_values(
    const module::BlockLoweringContext& context,
    const prepare::PreparedCallPlan& call_plan,
    std::size_t instruction_index,
    const BlockScalarLoweringState& scalar_state) {
  std::vector<module::MachineInstruction> lowered;
  const auto* call_plans =
      context.function.call_plans != nullptr
          ? context.function.call_plans
          : (context.function.prepared != nullptr &&
                     context.function.control_flow != nullptr
                 ? prepare::find_prepared_call_plans(
                       *context.function.prepared,
                       context.function.control_flow->function_name)
                 : nullptr);
  const auto* first_stack_values =
      call_plans == nullptr || context.function.call_plan_indexes == nullptr
          ? nullptr
          : first_stack_preserved_values_for_call(
                *context.function.call_plan_indexes, *call_plans, call_plan);
  std::vector<const prepare::PreparedCallPreservedValue*> fallback_values;
  if (call_plans != nullptr && context.function.call_plan_indexes == nullptr) {
    fallback_values =
        first_stack_preserved_values_for_call_fallback(*call_plans, call_plan);
  } else if (call_plans == nullptr) {
    fallback_values.reserve(call_plan.preserved_values.size());
    for (const auto& preserved : call_plan.preserved_values) {
      fallback_values.push_back(&preserved);
    }
  }
  const auto& values =
      first_stack_values != nullptr ? *first_stack_values : fallback_values;
  for (const auto* preserved_ptr : values) {
    if (preserved_ptr == nullptr) {
      continue;
    }
    const auto& preserved = *preserved_ptr;
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
    std::optional<abi::RegisterReference> source_register;
    if (emitted.has_value() && abi::is_gp_register(emitted->reg)) {
      source_register = emitted->reg;
    } else if (context.function.value_locations != nullptr) {
      const auto* home =
          find_value_home(context,
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
                    frame_slot_address(context.function, *preserved.stack_offset_bytes));
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
          ? find_value_home(context,
*value_name)
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

[[nodiscard]] DispatchBranchFusionHooks make_dispatch_branch_fusion_hooks() {
  return DispatchBranchFusionHooks{
      .scalar_view_for_type = scalar_view_for_type,
      .emit_value_publication_to_register = emit_value_publication_to_register,
      .current_block_entry_publication_register =
          current_block_entry_publication_register,
      .find_same_block_named_producer = find_same_block_named_producer,
      .producer_instruction_index = producer_instruction_index,
      .prepared_value_home_for_value = prepared_value_home_for_value,
      .value_has_current_block_entry_publication =
          value_has_current_block_entry_publication,
      .emit_prepared_value_home_to_register =
          emit_prepared_value_home_to_register,
      .fixed_slots_use_frame_pointer = fixed_slots_use_frame_pointer,
  };
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
  const auto branch_fusion_hooks = make_dispatch_branch_fusion_hooks();
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
    const auto join_parallel_copy_cache =
        build_current_block_join_parallel_copy_cache(context);
    std::optional<BlockAddressMaterializationIndex> address_materialization_index;
    auto current_address_materialization_index = [&]() -> const BlockAddressMaterializationIndex& {
      if (!address_materialization_index.has_value()) {
        address_materialization_index = make_block_address_materialization_index(context);
      }
      return *address_materialization_index;
    };
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
            is_fused_compare_branch_support_instruction(
                context, inst, scalar_state, branch_fusion_hooks) &&
            (lower_stack_home_fused_compare_branch(context, branch_fusion_hooks)
                 .has_value() ||
             lower_constant_rhs_fused_compare_branch(context, branch_fusion_hooks)
                 .has_value());
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
              lower_address_materializations(
                  context,
                  current_address_materialization_index(),
                  instruction_index,
                  diagnostics);
          auto before_call_moves =
              inline_variadic_entry_helper
                  ? std::vector<module::MachineInstruction>{}
                  : lower_before_call_moves(context, *call_plan, instruction_index, diagnostics);
          if (auto materialized_callee =
                  materialize_indirect_call_callee_to_prepared_register(
                      context, *call, *call_plan, instruction_index, scalar_state)) {
            block.instructions.push_back(std::move(*materialized_callee));
          }
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
      } else if (cached_current_block_join_parallel_copy_incoming_expression(
                     join_parallel_copy_cache, context, instruction_index, inst)) {
        continue;
      } else if (lower_scalar_with_address_materialization(
                     context,
                     current_address_materialization_index(),
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
      } else if (auto lowered = lower_scalar_fp_binary_publication_to_prepared_register(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (is_fused_compare_branch_support_instruction(
                     context, inst, scalar_state, branch_fusion_hooks)) {
        continue;
      } else if (auto lowered = lower_scalar_cast_instruction(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_register(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (auto lowered = lower_scalar_cast_publication_to_prepared_stack(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
      } else if (cached_current_block_join_parallel_copy_source(
                     join_parallel_copy_cache, context, instruction_index, inst)) {
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
      } else if (auto lowered = lower_local_slot_address_publication(
              context, inst, instruction_index, scalar_state)) {
        block.instructions.push_back(std::move(*lowered));
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
            } else if (auto pointer_address_store =
                           lower_pointer_base_plus_offset_store_local_publication(
                               context, inst, memory_lowering_index)) {
              diagnostics.entries.resize(diagnostic_count);
              block.instructions.push_back(std::move(*pointer_address_store));
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
    if (fused_compare_uses_selected_operand(context, branch_fusion_hooks)) {
      for (auto& publication :
           lower_missing_fused_compare_operand_publications(
               context, scalar_state, diagnostics)) {
        block.instructions.push_back(std::move(publication));
      }
    }
    if (auto lowered =
            lower_current_block_entry_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_stack_home_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
            lower_fused_compare_branch_from_emitted_cast(context, scalar_state, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_constant_rhs_fused_compare_branch(context, branch_fusion_hooks)) {
      block.instructions.push_back(std::move(*lowered));
      block.successors = make_conditional_branch_successors(context);
    } else if (auto lowered =
                   lower_materialized_compare_condition_branch(context, scalar_state, branch_fusion_hooks)) {
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
              context, scalar_state, branch_fusion_hooks)) {
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

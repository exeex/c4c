#include "dispatch.hpp"

#include "../abi/abi.hpp"
#include "alu.hpp"
#include "atomics.hpp"
#include "calls_dispatch_bridge.hpp"
#include "calls.hpp"
#include "cast_ops.hpp"
#include "comparison.hpp"
#include "comparison_branch_fusion.hpp"
#include "dispatch_diagnostics.hpp"
#include "dispatch_edge_copies.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_producers.hpp"
#include "dispatch_publication.hpp"
#include "dispatch_publication_common.hpp"
#include "dispatch_value_materialization.hpp"
#include "memory_store_sources.hpp"
#include "f128.hpp"
#include "float_ops.hpp"
#include "globals.hpp"
#include "i128_ops.hpp"
#include "inline_asm.hpp"
#include "intrinsics.hpp"
#include "machine_printer.hpp"
#include "memory.hpp"
#include "memory_dynamic_stack.hpp"
#include "operands.hpp"
#include "prologue.hpp"
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

[[nodiscard]] bool instruction_result_has_stack_home(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst) {
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
  const auto* result_home = find_value_home(context, *result_value_name);
  return result_home != nullptr &&
         result_home->kind == prepare::PreparedValueHomeKind::StackSlot;
}











struct SameBlockSelectProducer {
  const bir::SelectInst* select = nullptr;
  std::size_t instruction_index = 0;
};
































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




[[nodiscard]] bool is_store_local_instruction(const bir::Inst& inst) {
  return std::get_if<bir::StoreLocalInst>(&inst) != nullptr;
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
                                                   *call_plan,
                                                   call->args,
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
                     join_parallel_copy_cache, context, instruction_index, inst) &&
                 !instruction_result_has_stack_home(context, inst)) {
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
                     join_parallel_copy_cache, context, instruction_index, inst) &&
                 !instruction_result_has_stack_home(context, inst)) {
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

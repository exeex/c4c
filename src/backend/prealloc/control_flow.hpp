#pragma once

#include "names.hpp"
#include "value_locations.hpp"

#include "../bir/bir.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::prepare {

enum class PrepareRoute {
  SemanticBirShared,
};

[[nodiscard]] constexpr std::string_view prepare_route_name(PrepareRoute route) {
  switch (route) {
    case PrepareRoute::SemanticBirShared:
      return "semantic_bir_shared";
  }
  return "unknown";
}

enum class PreparedJoinTransferKind {
  PhiEdge,
  SelectMaterialization = PhiEdge,
  EdgeStoreSlot = PhiEdge,
  LoopCarry,
};

[[nodiscard]] constexpr std::string_view prepared_join_transfer_kind_name(
    PreparedJoinTransferKind kind) {
  switch (kind) {
    case PreparedJoinTransferKind::PhiEdge:
      return "phi_edge";
    case PreparedJoinTransferKind::LoopCarry:
      return "loop_carry";
  }
  return "unknown";
}

enum class PreparedJoinTransferCarrierKind {
  None,
  SelectMaterialization,
  EdgeStoreSlot,
};

[[nodiscard]] constexpr std::string_view prepared_join_transfer_carrier_kind_name(
    PreparedJoinTransferCarrierKind kind) {
  switch (kind) {
    case PreparedJoinTransferCarrierKind::None:
      return "none";
    case PreparedJoinTransferCarrierKind::SelectMaterialization:
      return "select_materialization";
    case PreparedJoinTransferCarrierKind::EdgeStoreSlot:
      return "edge_store_slot";
  }
  return "unknown";
}

enum class PreparedBranchConditionKind {
  MaterializedBool,
  FusedCompare,
};

[[nodiscard]] constexpr std::string_view prepared_branch_condition_kind_name(
    PreparedBranchConditionKind kind) {
  switch (kind) {
    case PreparedBranchConditionKind::MaterializedBool:
      return "materialized_bool";
    case PreparedBranchConditionKind::FusedCompare:
      return "fused_compare";
  }
  return "unknown";
}

struct PreparedEdgeValueTransfer {
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  bir::Value incoming_value;
  bir::Value destination_value;
  std::optional<SlotNameId> storage_name;
};

enum class PreparedParallelCopyStepKind {
  SaveDestinationToTemp,
  Move,
};

[[nodiscard]] constexpr std::string_view prepared_parallel_copy_step_kind_name(
    PreparedParallelCopyStepKind kind) {
  switch (kind) {
    case PreparedParallelCopyStepKind::SaveDestinationToTemp:
      return "save_destination_to_temp";
    case PreparedParallelCopyStepKind::Move:
      return "move";
  }
  return "unknown";
}

struct PreparedParallelCopyMove {
  std::size_t join_transfer_index = 0;
  std::size_t edge_transfer_index = 0;
  bir::Value source_value;
  bir::Value destination_value;
  PreparedJoinTransferCarrierKind carrier_kind = PreparedJoinTransferCarrierKind::None;
  std::optional<SlotNameId> storage_name;
};

struct PreparedParallelCopyStep {
  PreparedParallelCopyStepKind kind = PreparedParallelCopyStepKind::Move;
  std::size_t move_index = 0;
  bool uses_cycle_temp_source = false;
};

enum class PreparedParallelCopyExecutionSite {
  PredecessorTerminator,
  SuccessorEntry,
  CriticalEdge,
};

[[nodiscard]] constexpr std::string_view prepared_parallel_copy_execution_site_name(
    PreparedParallelCopyExecutionSite site) {
  switch (site) {
    case PreparedParallelCopyExecutionSite::PredecessorTerminator:
      return "predecessor_terminator";
    case PreparedParallelCopyExecutionSite::SuccessorEntry:
      return "successor_entry";
    case PreparedParallelCopyExecutionSite::CriticalEdge:
      return "critical_edge";
  }
  return "unknown";
}

struct PreparedParallelCopyBundle {
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedParallelCopyExecutionSite execution_site =
      PreparedParallelCopyExecutionSite::PredecessorTerminator;
  std::optional<BlockLabelId> execution_block_label;
  std::vector<PreparedParallelCopyMove> moves;
  std::vector<PreparedParallelCopyStep> steps;
  bool has_cycle = false;
};

struct PreparedBranchCondition {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  PreparedBranchConditionKind kind = PreparedBranchConditionKind::MaterializedBool;
  bir::Value condition_value;
  std::optional<bir::BinaryOpcode> predicate;
  std::optional<bir::TypeKind> compare_type;
  std::optional<bir::Value> lhs;
  std::optional<bir::Value> rhs;
  bool can_fuse_with_branch = false;
  BlockLabelId true_label = kInvalidBlockLabel;
  BlockLabelId false_label = kInvalidBlockLabel;
};

struct PreparedControlFlowBlock {
  BlockLabelId block_label = kInvalidBlockLabel;
  bir::TerminatorKind terminator_kind = bir::TerminatorKind::Return;
  BlockLabelId branch_target_label = kInvalidBlockLabel;
  BlockLabelId true_label = kInvalidBlockLabel;
  BlockLabelId false_label = kInvalidBlockLabel;
};

struct PreparedJoinTransfer {
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId join_block_label = kInvalidBlockLabel;
  bir::Value result;
  PreparedJoinTransferKind kind = PreparedJoinTransferKind::PhiEdge;
  PreparedJoinTransferCarrierKind carrier_kind = PreparedJoinTransferCarrierKind::None;
  std::optional<SlotNameId> storage_name;
  std::vector<bir::PhiIncoming> incomings;
  std::vector<PreparedEdgeValueTransfer> edge_transfers;
  std::optional<BlockLabelId> source_branch_block_label;
  std::optional<std::size_t> source_true_transfer_index;
  std::optional<std::size_t> source_false_transfer_index;
  std::optional<BlockLabelId> source_true_incoming_label;
  std::optional<BlockLabelId> source_false_incoming_label;
  std::optional<BlockLabelId> continuation_true_label;
  std::optional<BlockLabelId> continuation_false_label;
};

[[nodiscard]] constexpr PreparedJoinTransferCarrierKind effective_prepared_join_transfer_carrier_kind(
    const PreparedJoinTransfer& transfer) {
  if (transfer.storage_name.has_value()) {
    return PreparedJoinTransferCarrierKind::EdgeStoreSlot;
  }
  if (transfer.carrier_kind != PreparedJoinTransferCarrierKind::None) {
    return transfer.carrier_kind;
  }
  if (transfer.source_true_transfer_index.has_value() &&
      transfer.source_false_transfer_index.has_value()) {
    return PreparedJoinTransferCarrierKind::SelectMaterialization;
  }
  return PreparedJoinTransferCarrierKind::None;
}

struct PreparedControlFlowFunction {
  FunctionNameId function_name = kInvalidFunctionName;
  std::vector<PreparedControlFlowBlock> blocks;
  std::vector<PreparedBranchCondition> branch_conditions;
  std::vector<PreparedJoinTransfer> join_transfers;
  std::vector<PreparedParallelCopyBundle> parallel_copy_bundles;
};

struct PreparedAuthoritativeBranchJoinTransfer {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
};

struct PreparedAuthoritativeBranchParallelCopyBundles {
  PreparedAuthoritativeBranchJoinTransfer join_transfer;
  const PreparedParallelCopyBundle* true_bundle = nullptr;
  const PreparedParallelCopyBundle* false_bundle = nullptr;
};

struct PreparedAuthoritativeJoinBranchSources {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
  const bir::Block* true_predecessor = nullptr;
  const bir::Block* false_predecessor = nullptr;
};

struct PreparedClassifiedShortCircuitIncoming {
  bool short_circuit_on_compare_true = false;
  bool short_circuit_value = false;
};

struct PreparedAuthoritativeShortCircuitJoinSources {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
  PreparedClassifiedShortCircuitIncoming classified_incoming;
};

struct PreparedShortCircuitJoinContext {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
  const bir::Block* join_block = nullptr;
  PreparedClassifiedShortCircuitIncoming classified_incoming;
  BlockLabelId continuation_true_label = kInvalidBlockLabel;
  BlockLabelId continuation_false_label = kInvalidBlockLabel;
};

struct PreparedShortCircuitContinuationLabels {
  BlockLabelId incoming_label = kInvalidBlockLabel;
  BlockLabelId true_label = kInvalidBlockLabel;
  BlockLabelId false_label = kInvalidBlockLabel;
};

struct PreparedShortCircuitTargetLabels {
  BlockLabelId block_label = kInvalidBlockLabel;
  std::optional<PreparedShortCircuitContinuationLabels> continuation;
};

struct PreparedShortCircuitBranchPlan {
  PreparedShortCircuitTargetLabels on_compare_true;
  PreparedShortCircuitTargetLabels on_compare_false;
};

struct PreparedJoinCarrier {
  std::size_t carrier_index = 0;
  std::string_view result_name;
  ValueNameId result_name_id = kInvalidValueName;
};

struct PreparedSupportedImmediateBinary {
  bir::BinaryOpcode opcode = bir::BinaryOpcode::Add;
  std::int64_t immediate = 0;
};

enum class PreparedComputedBaseKind {
  ImmediateI32,
  ParamValue,
  GlobalI32Load,
  PointerBackedGlobalI32Load,
};

struct PreparedComputedBase {
  PreparedComputedBaseKind kind = PreparedComputedBaseKind::ImmediateI32;
  std::int64_t immediate = 0;
  ValueNameId param_name_id = kInvalidValueName;
  std::string_view global_name;
  LinkNameId global_name_id = kInvalidLinkName;
  std::size_t global_byte_offset = 0;
  std::string_view pointer_root_global_name;
  LinkNameId pointer_root_global_name_id = kInvalidLinkName;
};

struct PreparedComputedValue {
  PreparedComputedBase base;
  std::vector<PreparedSupportedImmediateBinary> operations;
};

struct PreparedMaterializedCompareJoinContext {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
  bir::NameTables bir_names;
  const bir::Function* function = nullptr;
  const bir::Block* true_predecessor = nullptr;
  const bir::Block* false_predecessor = nullptr;
  const bir::Block* join_block = nullptr;
  const bir::BinaryInst* trailing_binary = nullptr;
  std::size_t carrier_index = 0;
  std::string_view carrier_result_name;
  ValueNameId carrier_result_name_id = kInvalidValueName;
};

struct PreparedMaterializedCompareJoinReturnContext {
  PreparedComputedValue selected_value;
  std::optional<PreparedSupportedImmediateBinary> trailing_binary;
  FunctionNameId function_name = kInvalidFunctionName;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

enum class PreparedMaterializedCompareJoinReturnShape {
  ImmediateI32,
  ImmediateI32WithTrailingImmediateBinary,
  ParamValue,
  ParamValueWithTrailingImmediateBinary,
  GlobalI32Load,
  GlobalI32LoadWithTrailingImmediateBinary,
  PointerBackedGlobalI32Load,
  PointerBackedGlobalI32LoadWithTrailingImmediateBinary,
};

struct PreparedMaterializedCompareJoinBranches {
  PreparedMaterializedCompareJoinContext compare_join_context;
  PreparedMaterializedCompareJoinReturnContext true_return_context;
  PreparedMaterializedCompareJoinReturnContext false_return_context;
  BlockLabelId false_predecessor_label = kInvalidBlockLabel;
};

struct PreparedCompareJoinContinuationTargets {
  BlockLabelId true_label = kInvalidBlockLabel;
  BlockLabelId false_label = kInvalidBlockLabel;
};

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
published_prepared_compare_join_continuation_targets(
    const PreparedJoinTransfer& join_transfer) {
  if (!join_transfer.continuation_true_label.has_value() ||
      !join_transfer.continuation_false_label.has_value()) {
    return std::nullopt;
  }
  return PreparedCompareJoinContinuationTargets{
      .true_label = *join_transfer.continuation_true_label,
      .false_label = *join_transfer.continuation_false_label,
  };
}

struct PreparedBranchTargetLabels {
  BlockLabelId true_label = kInvalidBlockLabel;
  BlockLabelId false_label = kInvalidBlockLabel;
};

[[nodiscard]] inline std::optional<BlockLabelId> resolve_prepared_block_label_id(
    const PreparedNameTables& names,
    std::string_view block_label);

[[nodiscard]] inline std::optional<FunctionNameId> resolve_prepared_function_name_id(
    const PreparedNameTables& names,
    std::string_view function_name);

[[nodiscard]] inline std::optional<ValueNameId> resolve_prepared_value_name_id(
    const PreparedNameTables& names,
    std::string_view value_name);

[[nodiscard]] constexpr PreparedBranchTargetLabels prepared_branch_target_labels(
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  return PreparedBranchTargetLabels{
      .true_label = continuation_labels.true_label,
      .false_label = continuation_labels.false_label,
  };
}

// Shared consumers must take branch semantics from `branch_conditions` and former
// phi/join obligations from `join_transfers` instead of reconstructing them from CFG shape.
struct PreparedControlFlow {
  std::vector<PreparedControlFlowFunction> functions;
};

[[nodiscard]] inline const PreparedControlFlowFunction* find_prepared_control_flow_function(
    const PreparedControlFlow& control_flow,
    FunctionNameId function_name) {
  for (const auto& function : control_flow.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedBranchCondition* find_prepared_branch_condition(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId block_label) {
  for (const auto& condition : function_cf.branch_conditions) {
    if (condition.block_label == block_label) {
      return &condition;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedControlFlowBlock* find_prepared_control_flow_block(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId block_label) {
  for (const auto& block : function_cf.blocks) {
    if (block.block_label == block_label) {
      return &block;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<BlockLabelId> find_prepared_linear_join_predecessor(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId start_label,
    BlockLabelId join_label) {
  if (start_label == kInvalidBlockLabel || join_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  std::unordered_set<BlockLabelId> visited;
  BlockLabelId current_label = start_label;
  while (current_label != kInvalidBlockLabel && visited.insert(current_label).second) {
    const auto* current = find_prepared_control_flow_block(function_cf, current_label);
    if (current == nullptr || current->terminator_kind != bir::TerminatorKind::Branch ||
        current->branch_target_label == kInvalidBlockLabel) {
      return std::nullopt;
    }
    if (current->branch_target_label == join_label) {
      return current->block_label;
    }
    current_label = current->branch_target_label;
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<BlockLabelId> find_prepared_linear_join_predecessor(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    std::string_view start_label,
    std::string_view join_label) {
  const auto start_label_id = resolve_prepared_block_label_id(names, start_label);
  const auto join_label_id = resolve_prepared_block_label_id(names, join_label);
  if (!start_label_id.has_value() || !join_label_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_linear_join_predecessor(function_cf, *start_label_id, *join_label_id);
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_control_flow_branch_target_labels(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId block_label) {
  if (block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto* block = find_prepared_control_flow_block(function_cf, block_label);
  if (block == nullptr || block->terminator_kind != bir::TerminatorKind::CondBranch ||
      block->true_label == kInvalidBlockLabel || block->false_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  return PreparedBranchTargetLabels{
      .true_label = block->true_label,
      .false_label = block->false_label,
  };
}

namespace detail {

struct BranchTargetIdentityPassContext {
  const bir::Block* source_block = nullptr;
};

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
read_agreeing_bir_branch_target_labels(
    const BranchTargetIdentityPassContext& context,
    const PreparedBranchTargetLabels& prepared_labels) {
  if (context.source_block == nullptr ||
      context.source_block->terminator.kind != bir::TerminatorKind::CondBranch ||
      context.source_block->terminator.true_label_id == kInvalidBlockLabel ||
      context.source_block->terminator.false_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }

  if (context.source_block->terminator.true_label_id != prepared_labels.true_label ||
      context.source_block->terminator.false_label_id != prepared_labels.false_label) {
    return std::nullopt;
  }

  return PreparedBranchTargetLabels{
      .true_label = context.source_block->terminator.true_label_id,
      .false_label = context.source_block->terminator.false_label_id,
  };
}

}  // namespace detail

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_control_flow_branch_target_labels(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId block_label,
    const bir::Block& source_block) {
  const auto prepared_labels =
      find_prepared_control_flow_branch_target_labels(function_cf, block_label);
  if (!prepared_labels.has_value()) {
    return prepared_labels;
  }

  const auto bir_labels = detail::read_agreeing_bir_branch_target_labels(
      detail::BranchTargetIdentityPassContext{.source_block = &source_block},
      *prepared_labels);
  if (!bir_labels.has_value()) {
    return prepared_labels;
  }

  return bir_labels;
}

[[nodiscard]] inline const PreparedBranchCondition*
find_prepared_i32_immediate_branch_condition(const PreparedNameTables& names,
                                             const PreparedControlFlowFunction& function_cf,
                                             BlockLabelId block_label,
                                             ValueNameId value_name) {
  if (block_label == kInvalidBlockLabel || value_name == kInvalidValueName) {
    return nullptr;
  }

  const auto* branch_condition =
      find_prepared_branch_condition(function_cf, block_label);
  if (branch_condition == nullptr || !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() || !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value() ||
      *branch_condition->compare_type != bir::TypeKind::I32) {
    return nullptr;
  }

  const std::string_view value_name_spelling = prepared_value_name(names, value_name);
  const bool lhs_is_value_rhs_is_i32_immediate =
      branch_condition->lhs->kind == bir::Value::Kind::Named &&
      branch_condition->lhs->name == value_name_spelling &&
      branch_condition->rhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->rhs->type == bir::TypeKind::I32;
  const bool rhs_is_value_lhs_is_i32_immediate =
      branch_condition->rhs->kind == bir::Value::Kind::Named &&
      branch_condition->rhs->name == value_name_spelling &&
      branch_condition->lhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->lhs->type == bir::TypeKind::I32;
  if (!lhs_is_value_rhs_is_i32_immediate && !rhs_is_value_lhs_is_i32_immediate) {
    return nullptr;
  }

  return branch_condition;
}

[[nodiscard]] inline const PreparedBranchCondition*
find_prepared_i32_immediate_branch_condition(const PreparedNameTables& names,
                                             const PreparedControlFlowFunction& function_cf,
                                             std::string_view block_label,
                                             std::string_view value_name) {
  const auto block_label_id = resolve_prepared_block_label_id(names, block_label);
  const auto value_name_id = resolve_prepared_value_name_id(names, value_name);
  if (!block_label_id.has_value() || !value_name_id.has_value()) {
    return nullptr;
  }
  return find_prepared_i32_immediate_branch_condition(
      names, function_cf, *block_label_id, *value_name_id);
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_compare_branch_target_labels(const PreparedNameTables& names,
                                           const PreparedBranchCondition& branch_condition,
                                           const bir::Block& source_block) {
  if (source_block.terminator.kind != bir::TerminatorKind::CondBranch ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() || !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  return PreparedBranchTargetLabels{
      .true_label = branch_condition.true_label,
      .false_label = branch_condition.false_label,
  };
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
resolve_prepared_compare_branch_target_labels(const PreparedNameTables& names,
                                              const PreparedControlFlowFunction* function_cf,
                                              BlockLabelId source_block_label_id,
                                              const bir::Block& source_block,
                                              const PreparedBranchCondition& branch_condition) {
  const auto branch_target_labels =
      find_prepared_compare_branch_target_labels(names, branch_condition, source_block);
  if (!branch_target_labels.has_value()) {
    return std::nullopt;
  }
  if (function_cf == nullptr || source_block_label_id == kInvalidBlockLabel) {
    return branch_target_labels;
  }

  const auto control_flow_target_labels =
      find_prepared_control_flow_branch_target_labels(
          *function_cf, source_block_label_id, source_block);
  if (!control_flow_target_labels.has_value()) {
    return branch_target_labels;
  }
  if (control_flow_target_labels->true_label != branch_target_labels->true_label ||
      control_flow_target_labels->false_label != branch_target_labels->false_label) {
    return std::nullopt;
  }
  return control_flow_target_labels;
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
resolve_prepared_compare_branch_target_labels(const PreparedNameTables& names,
                                              const PreparedControlFlowFunction* function_cf,
                                              const bir::Block& source_block,
                                              const PreparedBranchCondition& branch_condition) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block.label);
  return resolve_prepared_compare_branch_target_labels(
      names,
      function_cf,
      source_block_label_id.value_or(kInvalidBlockLabel),
      source_block,
      branch_condition);
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_compare_branch_target_labels(const PreparedNameTables& names,
                                           const PreparedControlFlowFunction& function_cf,
                                           BlockLabelId source_block_label_id,
                                           const bir::Block& source_block) {
  if (source_block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto* branch_condition =
      find_prepared_branch_condition(function_cf, source_block_label_id);
  if (branch_condition == nullptr) {
    return std::nullopt;
  }

  return find_prepared_compare_branch_target_labels(names, *branch_condition, source_block);
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_compare_branch_target_labels(const PreparedNameTables& names,
                                           const PreparedControlFlowFunction* function_cf,
                                           const bir::Block& source_block) {
  if (function_cf == nullptr) {
    return std::nullopt;
  }

  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block.label);
  if (!source_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_compare_branch_target_labels(
      names, *function_cf, *source_block_label_id, source_block);
}

[[nodiscard]] inline std::optional<PreparedShortCircuitTargetLabels>
build_prepared_short_circuit_target_labels(
    const PreparedNameTables& names,
    const PreparedEdgeValueTransfer& transfer,
    bool is_short_circuit_lane,
    bool short_circuit_value,
    BlockLabelId rhs_entry_label,
    BlockLabelId continuation_true_label,
    BlockLabelId continuation_false_label) {
  if (is_short_circuit_lane) {
    return PreparedShortCircuitTargetLabels{
        .block_label = short_circuit_value ? continuation_true_label
                                           : continuation_false_label,
        .continuation = std::nullopt,
    };
  }

  return PreparedShortCircuitTargetLabels{
      .block_label = rhs_entry_label,
      .continuation =
          PreparedShortCircuitContinuationLabels{
              .incoming_label = transfer.predecessor_label,
              .true_label = continuation_true_label,
              .false_label = continuation_false_label,
          },
  };
}

struct PreparedParamZeroBranchCondition {
  const PreparedBranchCondition* branch_condition = nullptr;
  std::string_view false_label;
  const char* false_branch_opcode = nullptr;
  enum class CompareShape {
    SelfTest,
  } compare_shape = CompareShape::SelfTest;
};

struct PreparedParamZeroBranchReturnContext {
  PreparedParamZeroBranchCondition prepared_branch;
  const bir::Block* true_block = nullptr;
  const bir::Block* false_block = nullptr;
};

struct PreparedParamZeroMaterializedCompareJoinContext {
  PreparedParamZeroBranchCondition prepared_branch;
  PreparedMaterializedCompareJoinContext compare_join_context;
};

struct PreparedParamZeroMaterializedCompareJoinBranches {
  PreparedParamZeroBranchCondition prepared_branch;
  PreparedMaterializedCompareJoinBranches prepared_join_branches;
};

struct PreparedMaterializedCompareJoinBranchPlan {
  PreparedBranchTargetLabels target_labels;
  const char* false_branch_opcode = nullptr;
  PreparedParamZeroBranchCondition::CompareShape compare_shape =
      PreparedParamZeroBranchCondition::CompareShape::SelfTest;
};

struct PreparedMaterializedCompareJoinReturnArm {
  PreparedMaterializedCompareJoinReturnContext context;
  PreparedMaterializedCompareJoinReturnShape shape =
      PreparedMaterializedCompareJoinReturnShape::ImmediateI32;
};

struct PreparedMaterializedCompareJoinReturnBinaryPlan {
  std::optional<PreparedSupportedImmediateBinary> trailing_binary;
};

struct PreparedSameModuleGlobalRef {
  std::string_view name;
  LinkNameId name_id = kInvalidLinkName;
};

struct PreparedMaterializedCompareJoinRenderContract {
  PreparedMaterializedCompareJoinBranchPlan branch_plan;
  PreparedMaterializedCompareJoinReturnArm true_return;
  PreparedMaterializedCompareJoinReturnArm false_return;
  std::vector<std::string_view> same_module_global_names;
  std::vector<PreparedSameModuleGlobalRef> same_module_global_refs;
};

struct PreparedResolvedMaterializedCompareJoinReturnArm {
  PreparedMaterializedCompareJoinReturnArm arm;
  const bir::Global* global = nullptr;
  const bir::Global* pointer_root_global = nullptr;
};

struct PreparedResolvedMaterializedCompareJoinRenderContract {
  PreparedMaterializedCompareJoinBranchPlan branch_plan;
  PreparedResolvedMaterializedCompareJoinReturnArm true_return;
  PreparedResolvedMaterializedCompareJoinReturnArm false_return;
  std::vector<const bir::Global*> same_module_globals;
};

[[nodiscard]] inline std::optional<PreparedSameModuleGlobalRef>
resolve_prepared_bir_link_name_ref(PreparedNameTables& names,
                                   const bir::NameTables& bir_names,
                                   std::string_view display_name,
                                   LinkNameId name_id) {
  if (name_id != kInvalidLinkName) {
    const std::string_view authoritative_name = bir_names.link_names.spelling(name_id);
    if (authoritative_name.empty()) {
      return std::nullopt;
    }
    if (!display_name.empty() && display_name != authoritative_name) {
      return std::nullopt;
    }
    const LinkNameId prepared_name_id = names.link_names.intern(authoritative_name);
    return PreparedSameModuleGlobalRef{
        .name = prepared_link_name(names, prepared_name_id),
        .name_id = prepared_name_id,
    };
  }
  if (display_name.empty()) {
    return std::nullopt;
  }
  const LinkNameId prepared_name_id = names.link_names.intern(display_name);
  if (prepared_name_id == kInvalidLinkName) {
    return std::nullopt;
  }
  return PreparedSameModuleGlobalRef{
      .name = prepared_link_name(names, prepared_name_id),
      .name_id = prepared_name_id,
  };
}

[[nodiscard]] inline PreparedSameModuleGlobalRef prepared_computed_base_global_ref(
    const PreparedNameTables& names,
    const PreparedComputedBase& base) {
  if (base.global_name_id != kInvalidLinkName) {
    return PreparedSameModuleGlobalRef{
        .name = prepared_link_name(names, base.global_name_id),
        .name_id = base.global_name_id,
    };
  }
  return PreparedSameModuleGlobalRef{
      .name = base.global_name,
      .name_id = kInvalidLinkName,
  };
}

[[nodiscard]] inline PreparedSameModuleGlobalRef prepared_computed_base_pointer_root_ref(
    const PreparedNameTables& names,
    const PreparedComputedBase& base) {
  if (base.pointer_root_global_name_id != kInvalidLinkName) {
    return PreparedSameModuleGlobalRef{
        .name = prepared_link_name(names, base.pointer_root_global_name_id),
        .name_id = base.pointer_root_global_name_id,
    };
  }
  return PreparedSameModuleGlobalRef{
      .name = base.pointer_root_global_name,
      .name_id = kInvalidLinkName,
  };
}

[[nodiscard]] inline std::vector<PreparedSameModuleGlobalRef>
collect_prepared_computed_value_same_module_global_refs(
    const PreparedNameTables& names,
    const PreparedComputedValue& computed_value) {
  std::vector<PreparedSameModuleGlobalRef> global_refs;
  switch (computed_value.base.kind) {
    case PreparedComputedBaseKind::ImmediateI32:
    case PreparedComputedBaseKind::ParamValue:
      break;
    case PreparedComputedBaseKind::GlobalI32Load:
      global_refs.push_back(
          prepared_computed_base_global_ref(names, computed_value.base));
      break;
    case PreparedComputedBaseKind::PointerBackedGlobalI32Load:
      global_refs.push_back(
          prepared_computed_base_pointer_root_ref(names, computed_value.base));
      global_refs.push_back(
          prepared_computed_base_global_ref(names, computed_value.base));
      break;
  }
  return global_refs;
}

[[nodiscard]] inline std::vector<std::string_view> collect_prepared_computed_value_same_module_globals(
    const PreparedNameTables& names,
    const PreparedComputedValue& computed_value) {
  std::vector<std::string_view> global_names;
  for (const auto ref :
       collect_prepared_computed_value_same_module_global_refs(names, computed_value)) {
    global_names.push_back(ref.name);
  }
  return global_names;
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinReturnContext>
find_prepared_materialized_compare_join_return_context(
    PreparedNameTables& names,
    const PreparedMaterializedCompareJoinContext& compare_join_context,
    const bir::Value& selected_value);

[[nodiscard]] inline PreparedMaterializedCompareJoinReturnShape
classify_prepared_materialized_compare_join_return_shape(
    const PreparedMaterializedCompareJoinReturnContext& return_context) {
  switch (return_context.selected_value.base.kind) {
    case PreparedComputedBaseKind::ImmediateI32:
      return return_context.trailing_binary.has_value()
                 ? PreparedMaterializedCompareJoinReturnShape::
                       ImmediateI32WithTrailingImmediateBinary
                 : PreparedMaterializedCompareJoinReturnShape::ImmediateI32;
    case PreparedComputedBaseKind::ParamValue:
      return return_context.trailing_binary.has_value()
                 ? PreparedMaterializedCompareJoinReturnShape::
                       ParamValueWithTrailingImmediateBinary
                 : PreparedMaterializedCompareJoinReturnShape::ParamValue;
    case PreparedComputedBaseKind::GlobalI32Load:
      return return_context.trailing_binary.has_value()
                 ? PreparedMaterializedCompareJoinReturnShape::
                       GlobalI32LoadWithTrailingImmediateBinary
                 : PreparedMaterializedCompareJoinReturnShape::GlobalI32Load;
    case PreparedComputedBaseKind::PointerBackedGlobalI32Load:
      return return_context.trailing_binary.has_value()
                 ? PreparedMaterializedCompareJoinReturnShape::
                       PointerBackedGlobalI32LoadWithTrailingImmediateBinary
                 : PreparedMaterializedCompareJoinReturnShape::PointerBackedGlobalI32Load;
  }

  std::abort();
}

[[nodiscard]] inline PreparedMaterializedCompareJoinReturnArm
build_prepared_materialized_compare_join_return_arm(
    const PreparedMaterializedCompareJoinReturnContext& return_context) {
  return PreparedMaterializedCompareJoinReturnArm{
      .context = return_context,
      .shape = classify_prepared_materialized_compare_join_return_shape(return_context),
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinReturnBinaryPlan>
find_prepared_materialized_compare_join_return_binary_plan(
    const PreparedMaterializedCompareJoinReturnArm& return_arm) {
  switch (return_arm.shape) {
    case PreparedMaterializedCompareJoinReturnShape::ImmediateI32:
    case PreparedMaterializedCompareJoinReturnShape::ParamValue:
    case PreparedMaterializedCompareJoinReturnShape::GlobalI32Load:
    case PreparedMaterializedCompareJoinReturnShape::PointerBackedGlobalI32Load:
      if (return_arm.context.trailing_binary.has_value()) {
        return std::nullopt;
      }
      return PreparedMaterializedCompareJoinReturnBinaryPlan{
          .trailing_binary = std::nullopt,
      };
    case PreparedMaterializedCompareJoinReturnShape::ImmediateI32WithTrailingImmediateBinary:
    case PreparedMaterializedCompareJoinReturnShape::ParamValueWithTrailingImmediateBinary:
    case PreparedMaterializedCompareJoinReturnShape::GlobalI32LoadWithTrailingImmediateBinary:
    case PreparedMaterializedCompareJoinReturnShape::
        PointerBackedGlobalI32LoadWithTrailingImmediateBinary:
      if (!return_arm.context.trailing_binary.has_value()) {
        return std::nullopt;
      }
      return PreparedMaterializedCompareJoinReturnBinaryPlan{
          .trailing_binary = return_arm.context.trailing_binary,
      };
  }

  std::abort();
}

[[nodiscard]] inline std::vector<std::string_view>
collect_prepared_materialized_compare_join_same_module_globals(
    const PreparedNameTables& names,
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches);

[[nodiscard]] inline std::vector<PreparedSameModuleGlobalRef>
collect_prepared_materialized_compare_join_same_module_global_refs(
    const PreparedNameTables& names,
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches);

[[nodiscard]] inline std::optional<std::vector<const bir::Global*>>
resolve_prepared_materialized_compare_join_same_module_globals(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract);

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinReturnArm>
resolve_prepared_materialized_compare_join_return_arm(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinReturnArm& return_arm);

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
resolve_prepared_materialized_compare_join_render_contract(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract);

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
find_prepared_resolved_materialized_compare_join_render_contract(
    PreparedNameTables& names,
    const bir::Module& module,
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches);

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
find_prepared_param_zero_resolved_materialized_compare_join_render_contract(
    PreparedNameTables& names,
    const bir::Module& module,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match);

[[nodiscard]] inline std::optional<PreparedParamZeroBranchCondition>
find_prepared_param_zero_branch_condition(const PreparedNameTables& names,
                                          const PreparedControlFlowFunction& function_cf,
                                          BlockLabelId source_block_label_id,
                                          const bir::Block& source_block,
                                          ValueNameId param_name_id,
                                          bool require_label_match) {
  if (source_block.terminator.kind != bir::TerminatorKind::CondBranch ||
      source_block.terminator.condition.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  if (source_block_label_id == kInvalidBlockLabel || param_name_id == kInvalidValueName) {
    return std::nullopt;
  }

  const auto* branch_condition =
      find_prepared_branch_condition(function_cf, source_block_label_id);
  if (branch_condition == nullptr || !branch_condition->predicate.has_value() ||
      !branch_condition->compare_type.has_value() || !branch_condition->lhs.has_value() ||
      !branch_condition->rhs.has_value() ||
      (*branch_condition->predicate != bir::BinaryOpcode::Eq &&
       *branch_condition->predicate != bir::BinaryOpcode::Ne) ||
      *branch_condition->compare_type != bir::TypeKind::I32 ||
      branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      branch_condition->condition_value.name != source_block.terminator.condition.name) {
    return std::nullopt;
  }
  if (require_label_match &&
      (prepared_block_label(names, branch_condition->true_label) !=
           source_block.terminator.true_label ||
       prepared_block_label(names, branch_condition->false_label) !=
           source_block.terminator.false_label)) {
    return std::nullopt;
  }

  const std::string_view param_name = prepared_value_name(names, param_name_id);
  const bool lhs_is_param_rhs_is_zero =
      branch_condition->lhs->kind == bir::Value::Kind::Named &&
      branch_condition->lhs->name == param_name &&
      branch_condition->rhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->rhs->type == bir::TypeKind::I32 &&
      branch_condition->rhs->immediate == 0;
  const bool rhs_is_param_lhs_is_zero =
      branch_condition->rhs->kind == bir::Value::Kind::Named &&
      branch_condition->rhs->name == param_name &&
      branch_condition->lhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->lhs->type == bir::TypeKind::I32 &&
      branch_condition->lhs->immediate == 0;
  if (!lhs_is_param_rhs_is_zero && !rhs_is_param_lhs_is_zero) {
    return std::nullopt;
  }

  if (*branch_condition->predicate == bir::BinaryOpcode::Eq) {
    return PreparedParamZeroBranchCondition{
        .branch_condition = branch_condition,
        .false_label = prepared_block_label(names, branch_condition->false_label),
        .false_branch_opcode = "jne",
        .compare_shape = PreparedParamZeroBranchCondition::CompareShape::SelfTest,
    };
  }
  if (*branch_condition->predicate == bir::BinaryOpcode::Ne) {
    return PreparedParamZeroBranchCondition{
        .branch_condition = branch_condition,
        .false_label = prepared_block_label(names, branch_condition->false_label),
        .false_branch_opcode = "je",
        .compare_shape = PreparedParamZeroBranchCondition::CompareShape::SelfTest,
    };
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedParamZeroBranchCondition>
find_prepared_param_zero_branch_condition(const PreparedNameTables& names,
                                          const PreparedControlFlowFunction& function_cf,
                                          const bir::Block& source_block,
                                          const bir::Param& param,
                                          bool require_label_match) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block.label);
  const auto param_name_id = resolve_prepared_value_name_id(names, param.name);
  if (!source_block_label_id.has_value() || !param_name_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_param_zero_branch_condition(names,
                                                   function_cf,
                                                   *source_block_label_id,
                                                   source_block,
                                                   *param_name_id,
                                                   require_label_match);
}

[[nodiscard]] inline const bir::Block* find_block_in_function(const bir::Function& function,
                                                              std::string_view block_label) {
  for (const auto& block : function.blocks) {
    if (block.label == block_label) {
      return &block;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<PreparedParamZeroBranchReturnContext>
find_prepared_param_zero_branch_return_context(const PreparedNameTables& names,
                                               const PreparedControlFlowFunction& function_cf,
                                               const bir::Function& function,
                                               const bir::Block& source_block,
                                               const bir::Param& param,
                                               bool require_label_match) {
  const auto prepared_branch = find_prepared_param_zero_branch_condition(
      names, function_cf, source_block, param, require_label_match);
  if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto* branch_condition = prepared_branch->branch_condition;
  const auto* true_block =
      find_block_in_function(function, prepared_block_label(names, branch_condition->true_label));
  const auto* false_block =
      find_block_in_function(function, prepared_block_label(names, branch_condition->false_label));
  if (true_block == nullptr || false_block == nullptr || true_block == &source_block ||
      false_block == &source_block ||
      true_block->terminator.kind != bir::TerminatorKind::Return ||
      false_block->terminator.kind != bir::TerminatorKind::Return ||
      !true_block->terminator.value.has_value() || !false_block->terminator.value.has_value() ||
      !true_block->insts.empty() || !false_block->insts.empty()) {
    return std::nullopt;
  }

  return PreparedParamZeroBranchReturnContext{
      .prepared_branch = *prepared_branch,
      .true_block = true_block,
      .false_block = false_block,
  };
}

[[nodiscard]] inline const PreparedJoinTransfer* find_prepared_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId join_block_label,
    ValueNameId destination_value_name) {
  if (join_block_label == kInvalidBlockLabel || destination_value_name == kInvalidValueName) {
    return nullptr;
  }

  const std::string_view destination_value_name_spelling =
      prepared_value_name(names, destination_value_name);
  for (const auto& transfer : function_cf.join_transfers) {
    if (transfer.join_block_label == join_block_label &&
        transfer.result.kind == bir::Value::Kind::Named &&
        transfer.result.name == destination_value_name_spelling) {
      return &transfer;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedJoinTransfer* find_prepared_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId join_block_label,
    std::string_view destination_value_name) {
  const auto destination_value_name_id =
      resolve_prepared_value_name_id(names, destination_value_name);
  if (!destination_value_name_id.has_value()) {
    return nullptr;
  }
  return find_prepared_join_transfer(
      names, function_cf, join_block_label, *destination_value_name_id);
}

[[nodiscard]] inline const PreparedJoinTransfer* find_prepared_join_transfer(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId join_block_label,
    std::string_view destination_value_name) = delete;

[[nodiscard]] inline std::optional<BlockLabelId> resolve_prepared_block_label_id(
    const PreparedNameTables& names,
    std::string_view block_label) {
  const BlockLabelId block_label_id = names.block_labels.find(block_label);
  if (block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }
  return block_label_id;
}

[[nodiscard]] inline std::optional<FunctionNameId> resolve_prepared_function_name_id(
    const PreparedNameTables& names,
    std::string_view function_name) {
  const FunctionNameId function_name_id = names.function_names.find(function_name);
  if (function_name_id == kInvalidFunctionName) {
    return std::nullopt;
  }
  return function_name_id;
}

[[nodiscard]] inline std::optional<ValueNameId> resolve_prepared_value_name_id(
    const PreparedNameTables& names,
    std::string_view value_name) {
  const ValueNameId value_name_id = names.value_names.find(value_name);
  if (value_name_id == kInvalidValueName) {
    return std::nullopt;
  }
  return value_name_id;
}

[[nodiscard]] inline const PreparedJoinTransfer* find_branch_owned_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<BlockLabelId> true_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> false_predecessor_label = std::nullopt) {
  if (source_branch_block_label == kInvalidBlockLabel) {
    return nullptr;
  }

  const PreparedJoinTransfer* match = nullptr;
  for (const auto& transfer : function_cf.join_transfers) {
    if (!transfer.source_branch_block_label.has_value() ||
        *transfer.source_branch_block_label != source_branch_block_label) {
      continue;
    }
    if (required_kind.has_value() && transfer.kind != *required_kind) {
      continue;
    }
    if (true_predecessor_label.has_value() && false_predecessor_label.has_value()) {
      bool saw_true = false;
      bool saw_false = false;
      for (const auto& edge_transfer : transfer.edge_transfers) {
        saw_true = saw_true || edge_transfer.predecessor_label == *true_predecessor_label;
        saw_false = saw_false || edge_transfer.predecessor_label == *false_predecessor_label;
      }
      if (!saw_true || !saw_false) {
        continue;
      }
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &transfer;
  }
  return match;
}

[[nodiscard]] inline const PreparedJoinTransfer* find_select_materialization_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId source_branch_block_label,
    std::optional<BlockLabelId> true_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> false_predecessor_label = std::nullopt) {
  static_cast<void>(names);
  if (source_branch_block_label == kInvalidBlockLabel) {
    return nullptr;
  }

  const PreparedJoinTransfer* match = nullptr;
  for (const auto& transfer : function_cf.join_transfers) {
    if (!transfer.source_branch_block_label.has_value() ||
        *transfer.source_branch_block_label != source_branch_block_label ||
        transfer.kind != PreparedJoinTransferKind::PhiEdge ||
        effective_prepared_join_transfer_carrier_kind(transfer) !=
            PreparedJoinTransferCarrierKind::SelectMaterialization) {
      continue;
    }
    if (true_predecessor_label.has_value() && false_predecessor_label.has_value()) {
      bool saw_true = false;
      bool saw_false = false;
      for (const auto& edge_transfer : transfer.edge_transfers) {
        saw_true = saw_true || edge_transfer.predecessor_label == *true_predecessor_label;
        saw_false = saw_false || edge_transfer.predecessor_label == *false_predecessor_label;
      }
      if (!saw_true || !saw_false) {
        continue;
      }
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &transfer;
  }
  return match;
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeBranchJoinTransfer>
find_authoritative_branch_owned_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<BlockLabelId> true_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> false_predecessor_label = std::nullopt) {
  const auto* join_transfer = find_branch_owned_join_transfer(names,
                                                              function_cf,
                                                              source_branch_block_label,
                                                              required_kind,
                                                              true_predecessor_label,
                                                              false_predecessor_label);
  if (join_transfer == nullptr ||
      join_transfer->kind != PreparedJoinTransferKind::PhiEdge ||
      join_transfer->result.type != bir::TypeKind::I32 ||
      !join_transfer->source_true_transfer_index.has_value() ||
      !join_transfer->source_false_transfer_index.has_value()) {
    return std::nullopt;
  }

  const auto true_index = *join_transfer->source_true_transfer_index;
  const auto false_index = *join_transfer->source_false_transfer_index;
  if (true_index >= join_transfer->edge_transfers.size() ||
      false_index >= join_transfer->edge_transfers.size() || true_index == false_index) {
    return std::nullopt;
  }

  const auto* true_transfer = &join_transfer->edge_transfers[true_index];
  const auto* false_transfer = &join_transfer->edge_transfers[false_index];
  if (true_transfer->successor_label != join_transfer->join_block_label ||
      false_transfer->successor_label != join_transfer->join_block_label) {
    return std::nullopt;
  }

  if (effective_prepared_join_transfer_carrier_kind(*join_transfer) ==
      PreparedJoinTransferCarrierKind::EdgeStoreSlot) {
    if (!join_transfer->storage_name.has_value() ||
        !true_transfer->storage_name.has_value() ||
        !false_transfer->storage_name.has_value() ||
        *true_transfer->storage_name != *join_transfer->storage_name ||
        *false_transfer->storage_name != *join_transfer->storage_name) {
      return std::nullopt;
    }
  }

  return PreparedAuthoritativeBranchJoinTransfer{
      .join_transfer = join_transfer,
      .true_transfer = true_transfer,
      .false_transfer = false_transfer,
  };
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeBranchJoinTransfer>
find_authoritative_branch_owned_join_transfer(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    std::string_view source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<std::string_view> true_predecessor_label = std::nullopt,
    std::optional<std::string_view> false_predecessor_label = std::nullopt) {
  const auto source_branch_block_label_id =
      resolve_prepared_block_label_id(names, source_branch_block_label);
  if (!source_branch_block_label_id.has_value()) {
    return std::nullopt;
  }

  std::optional<BlockLabelId> true_predecessor_label_id;
  if (true_predecessor_label.has_value()) {
    true_predecessor_label_id = resolve_prepared_block_label_id(names, *true_predecessor_label);
    if (!true_predecessor_label_id.has_value()) {
      return std::nullopt;
    }
  }

  std::optional<BlockLabelId> false_predecessor_label_id;
  if (false_predecessor_label.has_value()) {
    false_predecessor_label_id =
        resolve_prepared_block_label_id(names, *false_predecessor_label);
    if (!false_predecessor_label_id.has_value()) {
      return std::nullopt;
    }
  }

  return find_authoritative_branch_owned_join_transfer(names,
                                                       function_cf,
                                                       *source_branch_block_label_id,
                                                       required_kind,
                                                       true_predecessor_label_id,
                                                       false_predecessor_label_id);
}

[[nodiscard]] inline const PreparedParallelCopyBundle*
find_published_parallel_copy_bundle_for_edge_transfer(
    const PreparedControlFlowFunction& function_cf,
    const PreparedEdgeValueTransfer& edge_transfer) {
  if (edge_transfer.predecessor_label == kInvalidBlockLabel ||
      edge_transfer.successor_label == kInvalidBlockLabel) {
    return nullptr;
  }
  for (const auto& bundle : function_cf.parallel_copy_bundles) {
    if (bundle.predecessor_label == edge_transfer.predecessor_label &&
        bundle.successor_label == edge_transfer.successor_label) {
      return &bundle;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeBranchParallelCopyBundles>
find_authoritative_branch_owned_parallel_copy_bundles(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<BlockLabelId> true_predecessor_label = std::nullopt,
    std::optional<BlockLabelId> false_predecessor_label = std::nullopt) {
  const auto authoritative_join_transfer = find_authoritative_branch_owned_join_transfer(
      names,
      function_cf,
      source_branch_block_label,
      required_kind,
      true_predecessor_label,
      false_predecessor_label);
  if (!authoritative_join_transfer.has_value() ||
      authoritative_join_transfer->join_transfer == nullptr ||
      authoritative_join_transfer->true_transfer == nullptr ||
      authoritative_join_transfer->false_transfer == nullptr) {
    return std::nullopt;
  }

  const auto* true_bundle = find_published_parallel_copy_bundle_for_edge_transfer(
      function_cf, *authoritative_join_transfer->true_transfer);
  const auto* false_bundle = find_published_parallel_copy_bundle_for_edge_transfer(
      function_cf, *authoritative_join_transfer->false_transfer);
  if (true_bundle == nullptr || false_bundle == nullptr || true_bundle == false_bundle) {
    return std::nullopt;
  }

  return PreparedAuthoritativeBranchParallelCopyBundles{
      .join_transfer = *authoritative_join_transfer,
      .true_bundle = true_bundle,
      .false_bundle = false_bundle,
  };
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeBranchParallelCopyBundles>
find_authoritative_branch_owned_parallel_copy_bundles(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    std::string_view source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<std::string_view> true_predecessor_label = std::nullopt,
    std::optional<std::string_view> false_predecessor_label = std::nullopt) {
  const auto source_branch_block_label_id =
      resolve_prepared_block_label_id(names, source_branch_block_label);
  if (!source_branch_block_label_id.has_value()) {
    return std::nullopt;
  }

  std::optional<BlockLabelId> true_predecessor_label_id;
  if (true_predecessor_label.has_value()) {
    true_predecessor_label_id = resolve_prepared_block_label_id(names, *true_predecessor_label);
    if (!true_predecessor_label_id.has_value()) {
      return std::nullopt;
    }
  }

  std::optional<BlockLabelId> false_predecessor_label_id;
  if (false_predecessor_label.has_value()) {
    false_predecessor_label_id =
        resolve_prepared_block_label_id(names, *false_predecessor_label);
    if (!false_predecessor_label_id.has_value()) {
      return std::nullopt;
    }
  }

  return find_authoritative_branch_owned_parallel_copy_bundles(names,
                                                               function_cf,
                                                               *source_branch_block_label_id,
                                                               required_kind,
                                                               true_predecessor_label_id,
                                                               false_predecessor_label_id);
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeJoinBranchSources>
find_authoritative_join_branch_sources(
    const PreparedNameTables& names,
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    BlockLabelId true_block_label_id,
    BlockLabelId false_block_label_id) {
  const auto* join_transfer = authoritative_join_transfer.join_transfer;
  const auto* true_transfer = authoritative_join_transfer.true_transfer;
  const auto* false_transfer = authoritative_join_transfer.false_transfer;
  if (join_transfer == nullptr || true_transfer == nullptr || false_transfer == nullptr ||
      true_block_label_id == kInvalidBlockLabel || false_block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }
  if (join_transfer->source_true_incoming_label.has_value() &&
      true_transfer->predecessor_label != *join_transfer->source_true_incoming_label) {
    return std::nullopt;
  }
  if (join_transfer->source_false_incoming_label.has_value() &&
      false_transfer->predecessor_label != *join_transfer->source_false_incoming_label) {
    return std::nullopt;
  }

  const auto* true_predecessor =
      find_block_in_function(function, prepared_block_label(names, true_block_label_id));
  const auto* false_predecessor =
      find_block_in_function(function, prepared_block_label(names, false_block_label_id));
  if (true_predecessor == nullptr || false_predecessor == nullptr ||
      true_predecessor == &entry_block || false_predecessor == &entry_block ||
      true_predecessor == false_predecessor || !true_predecessor->insts.empty() ||
      !false_predecessor->insts.empty()) {
    return std::nullopt;
  }

  const auto branches_to_join_with_optional_empty_passthrough =
      [&](const bir::Block& predecessor) -> bool {
    if (predecessor.terminator.kind != bir::TerminatorKind::Branch) {
      return false;
    }
    if (predecessor.terminator.target_label ==
        prepared_block_label(names, join_transfer->join_block_label)) {
      return true;
    }

    const auto* passthrough =
        find_block_in_function(function, predecessor.terminator.target_label);
    if (passthrough == nullptr || passthrough == &entry_block || passthrough == &predecessor ||
        !passthrough->insts.empty() ||
        passthrough->terminator.kind != bir::TerminatorKind::Branch ||
        passthrough->terminator.target_label !=
            prepared_block_label(names, join_transfer->join_block_label)) {
      return false;
    }
    return true;
  };

  if (!branches_to_join_with_optional_empty_passthrough(*true_predecessor) ||
      !branches_to_join_with_optional_empty_passthrough(*false_predecessor)) {
    return std::nullopt;
  }

  return PreparedAuthoritativeJoinBranchSources{
      .join_transfer = join_transfer,
      .true_transfer = true_transfer,
      .false_transfer = false_transfer,
      .true_predecessor = true_predecessor,
      .false_predecessor = false_predecessor,
  };
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeJoinBranchSources>
find_authoritative_join_branch_sources(
    const PreparedNameTables& names,
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    std::string_view true_block_label,
    std::string_view false_block_label) {
  const auto true_block_label_id = resolve_prepared_block_label_id(names, true_block_label);
  const auto false_block_label_id = resolve_prepared_block_label_id(names, false_block_label);
  if (!true_block_label_id.has_value() || !false_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_authoritative_join_branch_sources(
      names,
      authoritative_join_transfer,
      function,
      entry_block,
      *true_block_label_id,
      *false_block_label_id);
}

[[nodiscard]] inline std::optional<PreparedClassifiedShortCircuitIncoming>
classify_short_circuit_join_incoming(const PreparedEdgeValueTransfer& true_transfer,
                                     const PreparedEdgeValueTransfer& false_transfer) {
  const auto classify_join_incoming =
      [](const PreparedEdgeValueTransfer& incoming) -> std::optional<bool> {
    if (incoming.incoming_value.kind == bir::Value::Kind::Immediate &&
        incoming.incoming_value.type == bir::TypeKind::I32 &&
        (incoming.incoming_value.immediate == 0 || incoming.incoming_value.immediate == 1)) {
      return incoming.incoming_value.immediate != 0;
    }
    return std::nullopt;
  };

  const auto true_lane_short_circuit_value = classify_join_incoming(true_transfer);
  const auto false_lane_short_circuit_value = classify_join_incoming(false_transfer);
  if (true_lane_short_circuit_value.has_value() == false_lane_short_circuit_value.has_value()) {
    return std::nullopt;
  }

  return PreparedClassifiedShortCircuitIncoming{
      .short_circuit_on_compare_true = true_lane_short_circuit_value.has_value(),
      .short_circuit_value = true_lane_short_circuit_value.value_or(
          *false_lane_short_circuit_value),
  };
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeShortCircuitJoinSources>
find_authoritative_short_circuit_join_sources(
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer) {
  const auto* join_transfer = authoritative_join_transfer.join_transfer;
  const auto* true_transfer = authoritative_join_transfer.true_transfer;
  const auto* false_transfer = authoritative_join_transfer.false_transfer;
  if (join_transfer == nullptr || true_transfer == nullptr || false_transfer == nullptr ||
      join_transfer->edge_transfers.size() != 2) {
    return std::nullopt;
  }

  const auto classified_incoming =
      classify_short_circuit_join_incoming(*true_transfer, *false_transfer);
  if (!classified_incoming.has_value()) {
    return std::nullopt;
  }

  return PreparedAuthoritativeShortCircuitJoinSources{
      .join_transfer = join_transfer,
      .true_transfer = true_transfer,
      .false_transfer = false_transfer,
      .classified_incoming = *classified_incoming,
  };
}

[[nodiscard]] inline std::optional<PreparedJoinCarrier> find_supported_join_carrier(
    const PreparedNameTables& names,
    const PreparedJoinTransfer& join_transfer,
    const bir::Block& join_block,
    std::size_t carrier_index) {
  if (carrier_index >= join_block.insts.size()) {
    return std::nullopt;
  }

  const auto& join_carrier = join_block.insts[carrier_index];
  if (const auto* select = std::get_if<bir::SelectInst>(&join_carrier); select != nullptr) {
    const ValueNameId result_name_id =
        const_cast<ValueNameTable&>(names.value_names).intern(select->result.name);
    if (effective_prepared_join_transfer_carrier_kind(join_transfer) !=
            PreparedJoinTransferCarrierKind::SelectMaterialization ||
        select->result.kind != bir::Value::Kind::Named ||
        select->result.type != bir::TypeKind::I32 || result_name_id == kInvalidValueName) {
      return std::nullopt;
    }
    return PreparedJoinCarrier{
        .carrier_index = carrier_index,
        .result_name = select->result.name,
        .result_name_id = result_name_id,
    };
  }

  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&join_carrier);
      load_local != nullptr) {
    const ValueNameId result_name_id =
        const_cast<ValueNameTable&>(names.value_names).intern(load_local->result.name);
    if (effective_prepared_join_transfer_carrier_kind(join_transfer) !=
            PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
        !join_transfer.storage_name.has_value() ||
        load_local->slot_name != prepared_slot_name(names, *join_transfer.storage_name) ||
        load_local->result.kind != bir::Value::Kind::Named ||
        load_local->result.type != bir::TypeKind::I32 ||
        result_name_id == kInvalidValueName) {
      return std::nullopt;
    }
    return PreparedJoinCarrier{
        .carrier_index = carrier_index,
        .result_name = load_local->result.name,
        .result_name_id = result_name_id,
    };
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedSupportedImmediateBinary>
classify_supported_immediate_binary(PreparedNameTables& names,
                                    const bir::BinaryInst& binary,
                                    ValueNameId source_name) {
  if (binary.operand_type != bir::TypeKind::I32 || binary.result.type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  const auto lhs_name = prepared_named_value_id(names, binary.lhs);
  const auto rhs_name = prepared_named_value_id(names, binary.rhs);
  const bool lhs_is_source_rhs_is_imm =
      lhs_name.has_value() && *lhs_name == source_name &&
      binary.rhs.kind == bir::Value::Kind::Immediate && binary.rhs.type == bir::TypeKind::I32;
  const bool rhs_is_source_lhs_is_imm =
      rhs_name.has_value() && *rhs_name == source_name &&
      binary.lhs.kind == bir::Value::Kind::Immediate && binary.lhs.type == bir::TypeKind::I32;
  if (!lhs_is_source_rhs_is_imm && !rhs_is_source_lhs_is_imm) {
    return std::nullopt;
  }

  const auto immediate = lhs_is_source_rhs_is_imm ? binary.rhs.immediate : binary.lhs.immediate;
  switch (binary.opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return PreparedSupportedImmediateBinary{
          .opcode = binary.opcode,
          .immediate = immediate,
      };
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      if (!lhs_is_source_rhs_is_imm) {
        return std::nullopt;
      }
      return PreparedSupportedImmediateBinary{
          .opcode = binary.opcode,
          .immediate = binary.rhs.immediate,
      };
    default:
      break;
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedComputedValue> classify_immediate_root_binary(
    const bir::BinaryInst& binary) {
  if (binary.operand_type != bir::TypeKind::I32 || binary.result.type != bir::TypeKind::I32 ||
      binary.lhs.kind != bir::Value::Kind::Immediate ||
      binary.rhs.kind != bir::Value::Kind::Immediate ||
      binary.lhs.type != bir::TypeKind::I32 || binary.rhs.type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  std::optional<PreparedSupportedImmediateBinary> operation;
  switch (binary.opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
      operation = PreparedSupportedImmediateBinary{
          .opcode = binary.opcode,
          .immediate = binary.rhs.immediate,
      };
      break;
    default:
      return std::nullopt;
  }

  return PreparedComputedValue{
      .base = PreparedComputedBase{
          .kind = PreparedComputedBaseKind::ImmediateI32,
          .immediate = binary.lhs.immediate,
      },
      .operations = {*operation},
  };
}

[[nodiscard]] inline std::optional<PreparedComputedValue> classify_computed_value(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Value& value,
    const bir::Function& function,
    const std::unordered_map<ValueNameId, const bir::BinaryInst*>& binaries_by_value_name,
    const std::unordered_map<ValueNameId, const bir::LoadGlobalInst*>& global_loads_by_value_name,
    std::vector<ValueNameId>* active_names = nullptr) {
  if (value.type != bir::TypeKind::I32) {
    return std::nullopt;
  }
  if (value.kind != bir::Value::Kind::Named) {
    return PreparedComputedValue{
        .base = PreparedComputedBase{
            .kind = PreparedComputedBaseKind::ImmediateI32,
            .immediate = value.immediate,
        },
    };
  }

  const auto value_name = prepared_named_value_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }

  std::vector<ValueNameId> local_active_names;
  auto* recursion_stack = active_names == nullptr ? &local_active_names : active_names;
  for (const auto active_name : *recursion_stack) {
    if (active_name == *value_name) {
      return std::nullopt;
    }
  }

  const auto binary_it = binaries_by_value_name.find(*value_name);
  if (binary_it == binaries_by_value_name.end()) {
    for (const auto& param : function.params) {
      const ValueNameId param_name = names.value_names.intern(param.name);
      if (param.type == bir::TypeKind::I32 && param_name == *value_name) {
        return PreparedComputedValue{
            .base = PreparedComputedBase{
                .kind = PreparedComputedBaseKind::ParamValue,
                .param_name_id = param_name,
            },
        };
      }
    }
    const auto load_global_it = global_loads_by_value_name.find(*value_name);
    if (load_global_it != global_loads_by_value_name.end() && load_global_it->second != nullptr) {
      const auto* load_global = load_global_it->second;
      if (load_global->result.type != bir::TypeKind::I32 ||
          load_global->result.kind != bir::Value::Kind::Named ||
          prepared_named_value_id(names, load_global->result) != value_name) {
        return std::nullopt;
      }
      if (load_global->address.has_value()) {
        if (load_global->address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol ||
            (load_global->address->base_name.empty() &&
             load_global->address->base_link_name_id == kInvalidLinkName) ||
            load_global->address->byte_offset != 0) {
          return std::nullopt;
        }
        const auto global_ref = resolve_prepared_bir_link_name_ref(
            names, bir_names, load_global->global_name, load_global->global_name_id);
        const auto pointer_root_ref = resolve_prepared_bir_link_name_ref(
            names,
            bir_names,
            load_global->address->base_name,
            load_global->address->base_link_name_id);
        if (!global_ref.has_value() || !pointer_root_ref.has_value()) {
          return std::nullopt;
        }
        return PreparedComputedValue{
            .base = PreparedComputedBase{
                .kind = PreparedComputedBaseKind::PointerBackedGlobalI32Load,
                .global_name = global_ref->name,
                .global_name_id = global_ref->name_id,
                .global_byte_offset = load_global->byte_offset,
                .pointer_root_global_name = pointer_root_ref->name,
                .pointer_root_global_name_id = pointer_root_ref->name_id,
            },
        };
      }
      const auto global_ref = resolve_prepared_bir_link_name_ref(
          names, bir_names, load_global->global_name, load_global->global_name_id);
      if (!global_ref.has_value()) {
        return std::nullopt;
      }
      return PreparedComputedValue{
          .base = PreparedComputedBase{
              .kind = PreparedComputedBaseKind::GlobalI32Load,
              .global_name = global_ref->name,
              .global_name_id = global_ref->name_id,
              .global_byte_offset = load_global->byte_offset,
          },
      };
    }
    return std::nullopt;
  }

  const auto* binary = binary_it->second;
  if (binary == nullptr) {
    return std::nullopt;
  }

  recursion_stack->push_back(*value_name);
  auto pop_active_name = [&]() { recursion_stack->pop_back(); };

  if (const auto immediate_root = classify_immediate_root_binary(*binary);
      immediate_root.has_value()) {
    pop_active_name();
    return immediate_root;
  }

  const auto try_extend =
      [&](const bir::Value& source_value,
          std::optional<PreparedSupportedImmediateBinary> operation)
      -> std::optional<PreparedComputedValue> {
    if (!operation.has_value()) {
      return std::nullopt;
    }
    auto computed_value =
        classify_computed_value(names,
                                bir_names,
                                source_value,
                                function,
                                binaries_by_value_name,
                                global_loads_by_value_name,
                                recursion_stack);
    if (!computed_value.has_value()) {
      return std::nullopt;
    }
    computed_value->operations.push_back(*operation);
    return computed_value;
  };

  if (const auto lhs_name = prepared_named_value_id(names, binary->lhs);
      lhs_name.has_value()) {
    if (const auto computed_value =
            try_extend(binary->lhs, classify_supported_immediate_binary(names, *binary, *lhs_name));
      computed_value.has_value()) {
      pop_active_name();
      return computed_value;
    }
  }
  if (const auto rhs_name = prepared_named_value_id(names, binary->rhs);
      rhs_name.has_value()) {
    if (const auto computed_value =
            try_extend(binary->rhs, classify_supported_immediate_binary(names, *binary, *rhs_name));
      computed_value.has_value()) {
      pop_active_name();
      return computed_value;
    }
  }

  pop_active_name();
  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedComputedValue> classify_computed_value(
    PreparedNameTables& names,
    const bir::Value& value,
    const bir::Function& function,
    const std::unordered_map<ValueNameId, const bir::BinaryInst*>& binaries_by_value_name,
    const std::unordered_map<ValueNameId, const bir::LoadGlobalInst*>& global_loads_by_value_name,
    std::vector<ValueNameId>* active_names = nullptr) {
  bir::NameTables bir_names;
  bir_names.import_link_names(names.texts, names.link_names);
  return classify_computed_value(
      names,
      bir_names,
      value,
      function,
      binaries_by_value_name,
      global_loads_by_value_name,
      active_names);
}

[[nodiscard]] inline std::optional<PreparedComputedValue> classify_computed_value(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const bir::Value& value,
    const bir::Function& function,
    const std::unordered_map<std::string_view, const bir::BinaryInst*>& binaries_by_spelling,
    const std::unordered_map<std::string_view, const bir::LoadGlobalInst*>& global_loads_by_spelling) {
  std::unordered_map<ValueNameId, const bir::BinaryInst*> binaries_by_value_name;
  std::unordered_map<ValueNameId, const bir::LoadGlobalInst*> global_loads_by_value_name;
  for (const auto& [result_spelling, binary] : binaries_by_spelling) {
    const ValueNameId result_name = names.value_names.intern(result_spelling);
    if (result_name == kInvalidValueName) {
      return std::nullopt;
    }
    binaries_by_value_name.emplace(result_name, binary);
  }
  for (const auto& [result_spelling, global_load] : global_loads_by_spelling) {
    const ValueNameId result_name = names.value_names.intern(result_spelling);
    if (result_name == kInvalidValueName) {
      return std::nullopt;
    }
    global_loads_by_value_name.emplace(result_name, global_load);
  }
  return classify_computed_value(
      names,
      bir_names,
      value,
      function,
      binaries_by_value_name,
      global_loads_by_value_name);
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinContext>
find_materialized_compare_join_context(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    BlockLabelId true_block_label_id,
    BlockLabelId false_block_label_id) {
  const auto join_sources = find_authoritative_join_branch_sources(names,
                                                                   authoritative_join_transfer,
                                                                   function,
                                                                   entry_block,
                                                                   true_block_label_id,
                                                                   false_block_label_id);
  if (!join_sources.has_value() || join_sources->join_transfer == nullptr ||
      join_sources->true_transfer == nullptr || join_sources->false_transfer == nullptr ||
      join_sources->true_predecessor == nullptr || join_sources->false_predecessor == nullptr ||
      join_sources->true_predecessor == join_sources->false_predecessor) {
    return std::nullopt;
  }

  const auto* join_transfer = join_sources->join_transfer;
  const auto* join_block =
      find_block_in_function(function, prepared_block_label(names, join_transfer->join_block_label));
  if (join_block == nullptr || join_block == &entry_block ||
      join_block->terminator.kind != bir::TerminatorKind::Return ||
      !join_block->terminator.value.has_value() || join_block->insts.empty() ||
      join_sources->true_predecessor == join_block ||
      join_sources->false_predecessor == join_block ||
      prepared_block_label(names, join_transfer->join_block_label) != join_block->label) {
    return std::nullopt;
  }

  std::size_t carrier_index = join_block->insts.size() - 1;
  const bir::BinaryInst* trailing_binary = nullptr;
  if (const auto* trailing = std::get_if<bir::BinaryInst>(&join_block->insts.back());
      trailing != nullptr &&
      join_block->terminator.value->kind == bir::Value::Kind::Named &&
      join_block->terminator.value->name == trailing->result.name) {
    if (join_block->insts.size() < 2) {
      return std::nullopt;
    }
    trailing_binary = trailing;
    carrier_index = join_block->insts.size() - 2;
  }

  const auto prepared_carrier =
      find_supported_join_carrier(names, *join_transfer, *join_block, carrier_index);
  if (!prepared_carrier.has_value()) {
    return std::nullopt;
  }
  if (join_block->terminator.value->kind != bir::Value::Kind::Named ||
      (trailing_binary == nullptr &&
       join_block->terminator.value->name !=
           prepared_value_name(names, prepared_carrier->result_name_id))) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinContext{
      .join_transfer = join_transfer,
      .true_transfer = join_sources->true_transfer,
      .false_transfer = join_sources->false_transfer,
      .bir_names = bir_names,
      .function = &function,
      .true_predecessor = join_sources->true_predecessor,
      .false_predecessor = join_sources->false_predecessor,
      .join_block = join_block,
      .trailing_binary = trailing_binary,
      .carrier_index = prepared_carrier->carrier_index,
      .carrier_result_name = prepared_carrier->result_name,
      .carrier_result_name_id = prepared_carrier->result_name_id,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinContext>
find_materialized_compare_join_context(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    std::string_view true_block_label,
    std::string_view false_block_label) {
  const auto true_block_label_id = resolve_prepared_block_label_id(names, true_block_label);
  const auto false_block_label_id = resolve_prepared_block_label_id(names, false_block_label);
  if (!true_block_label_id.has_value() || !false_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_materialized_compare_join_context(
      names,
      bir_names,
      authoritative_join_transfer,
      function,
      entry_block,
      *true_block_label_id,
      *false_block_label_id);
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_compare_join_continuation_targets(
    const PreparedNameTables& names,
    const PreparedJoinTransfer& join_transfer,
    const bir::Block& join_block,
    const PreparedBranchCondition& join_branch_condition) {
  if (const auto published_targets =
          published_prepared_compare_join_continuation_targets(join_transfer);
      published_targets.has_value()) {
    return published_targets;
  }

  if (!join_branch_condition.predicate.has_value() ||
      !join_branch_condition.compare_type.has_value() ||
      !join_branch_condition.lhs.has_value() || !join_branch_condition.rhs.has_value() ||
      *join_branch_condition.compare_type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  if (join_block.insts.size() < 2) {
    return std::nullopt;
  }

  const auto prepared_carrier =
      find_supported_join_carrier(names, join_transfer, join_block, join_block.insts.size() - 2);
  if (!prepared_carrier.has_value()) {
    return std::nullopt;
  }

  const bool lhs_is_join_result_rhs_is_zero =
      join_branch_condition.lhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.lhs->name ==
          prepared_value_name(names, prepared_carrier->result_name_id) &&
      join_branch_condition.rhs->kind == bir::Value::Kind::Immediate &&
      join_branch_condition.rhs->type == bir::TypeKind::I32 &&
      join_branch_condition.rhs->immediate == 0;
  const bool rhs_is_join_result_lhs_is_zero =
      join_branch_condition.rhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.rhs->name ==
          prepared_value_name(names, prepared_carrier->result_name_id) &&
      join_branch_condition.lhs->kind == bir::Value::Kind::Immediate &&
      join_branch_condition.lhs->type == bir::TypeKind::I32 &&
      join_branch_condition.lhs->immediate == 0;
  if (!lhs_is_join_result_rhs_is_zero && !rhs_is_join_result_lhs_is_zero) {
    return std::nullopt;
  }

  if (*join_branch_condition.predicate == bir::BinaryOpcode::Ne) {
    return PreparedCompareJoinContinuationTargets{
        .true_label = join_branch_condition.true_label,
        .false_label = join_branch_condition.false_label,
    };
  }
  if (*join_branch_condition.predicate == bir::BinaryOpcode::Eq) {
    return PreparedCompareJoinContinuationTargets{
        .true_label = join_branch_condition.false_label,
        .false_label = join_branch_condition.true_label,
    };
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_short_circuit_continuation_targets(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    BlockLabelId source_block_label_id) {
  if (source_block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer =
      find_authoritative_branch_owned_join_transfer(names, function_cf, source_block_label_id);
  if (!authoritative_join_transfer.has_value()) {
    return std::nullopt;
  }

  const auto join_sources =
      find_authoritative_short_circuit_join_sources(*authoritative_join_transfer);
  if (!join_sources.has_value()) {
    return std::nullopt;
  }

  const auto* source_block =
      find_block_in_function(function, prepared_block_label(names, source_block_label_id));
  if (source_block == nullptr) {
    return std::nullopt;
  }

  const auto* source_branch_condition =
      find_prepared_branch_condition(function_cf, source_block_label_id);
  if (source_branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto direct_targets = resolve_prepared_compare_branch_target_labels(
      names,
      &function_cf,
      source_block_label_id,
      *source_block,
      *source_branch_condition);
  if (!direct_targets.has_value()) {
    return std::nullopt;
  }

  BlockLabelId rhs_entry_label = kInvalidBlockLabel;
  const BlockLabelId join_block_label = join_sources->join_transfer->join_block_label;
  if (direct_targets->true_label == join_block_label &&
      direct_targets->false_label != join_block_label) {
    rhs_entry_label = direct_targets->false_label;
  } else if (direct_targets->false_label == join_block_label &&
             direct_targets->true_label != join_block_label) {
    rhs_entry_label = direct_targets->true_label;
  } else {
    rhs_entry_label = join_sources->classified_incoming.short_circuit_on_compare_true
                          ? direct_targets->false_label
                          : direct_targets->true_label;
  }
  if (rhs_entry_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto* rhs_entry_block =
      find_block_in_function(function, prepared_block_label(names, rhs_entry_label));
  if (rhs_entry_block == nullptr) {
    return std::nullopt;
  }

  const auto* rhs_branch_condition =
      find_prepared_branch_condition(function_cf, rhs_entry_label);
  if (rhs_branch_condition == nullptr) {
    return std::nullopt;
  }

  if (!rhs_branch_condition->predicate.has_value() ||
      !rhs_branch_condition->compare_type.has_value() ||
      !rhs_branch_condition->lhs.has_value() || !rhs_branch_condition->rhs.has_value()) {
    return std::nullopt;
  }

  return PreparedCompareJoinContinuationTargets{
      .true_label = rhs_branch_condition->true_label,
      .false_label = rhs_branch_condition->false_label,
  };
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_short_circuit_continuation_targets(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    std::string_view source_block_label) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block_label);
  if (!source_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_short_circuit_continuation_targets(
      names, function_cf, function, *source_block_label_id);
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_compare_join_continuation_targets(const PreparedNameTables& names,
                                                const PreparedControlFlowFunction& function_cf,
                                                const bir::Function& function,
                                                BlockLabelId source_block_label_id) {
  if (source_block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer =
      find_authoritative_branch_owned_join_transfer(names, function_cf, source_block_label_id);
  if (authoritative_join_transfer.has_value() &&
      authoritative_join_transfer->join_transfer != nullptr) {
    return published_prepared_compare_join_continuation_targets(
        *authoritative_join_transfer->join_transfer);
  }

  if (const auto short_circuit_targets =
          find_prepared_short_circuit_continuation_targets(names,
                                                           function_cf,
                                                           function,
                                                           source_block_label_id);
      short_circuit_targets.has_value()) {
    return short_circuit_targets;
  }
  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_compare_join_continuation_targets(const PreparedNameTables& names,
                                                const PreparedControlFlowFunction& function_cf,
                                                const bir::Function& function,
                                                std::string_view source_block_label) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block_label);
  if (!source_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_compare_join_continuation_targets(
      names, function_cf, function, *source_block_label_id);
}

[[nodiscard]] constexpr PreparedBranchTargetLabels prepared_compare_join_entry_target_labels(
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  return prepared_branch_target_labels(continuation_labels);
}

[[nodiscard]] inline PreparedBranchTargetLabels find_prepared_compare_join_entry_target_labels(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    BlockLabelId source_block_label_id,
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  if (const auto prepared_targets =
          find_prepared_compare_join_continuation_targets(names,
                                                          function_cf,
                                                          function,
                                                          source_block_label_id);
      prepared_targets.has_value()) {
    return PreparedBranchTargetLabels{
        .true_label = prepared_targets->true_label,
        .false_label = prepared_targets->false_label,
    };
  }
  return prepared_compare_join_entry_target_labels(continuation_labels);
}

[[nodiscard]] inline PreparedBranchTargetLabels find_prepared_compare_join_entry_target_labels(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    std::string_view source_block_label,
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block_label);
  if (!source_block_label_id.has_value()) {
    return prepared_compare_join_entry_target_labels(continuation_labels);
  }
  return find_prepared_compare_join_entry_target_labels(
      names, function_cf, function, *source_block_label_id, continuation_labels);
}

[[nodiscard]] inline PreparedBranchTargetLabels resolve_prepared_compare_join_entry_target_labels(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction* function_cf,
    const bir::Function& function,
    BlockLabelId source_block_label_id,
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  if (function_cf == nullptr) {
    return prepared_compare_join_entry_target_labels(continuation_labels);
  }

  return find_prepared_compare_join_entry_target_labels(
      names, *function_cf, function, source_block_label_id, continuation_labels);
}

[[nodiscard]] inline PreparedBranchTargetLabels resolve_prepared_compare_join_entry_target_labels(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction* function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block.label);
  if (!source_block_label_id.has_value()) {
    return prepared_compare_join_entry_target_labels(continuation_labels);
  }
  return resolve_prepared_compare_join_entry_target_labels(
      names, function_cf, function, *source_block_label_id, continuation_labels);
}

[[nodiscard]] inline std::optional<PreparedShortCircuitBranchPlan>
find_prepared_compare_join_entry_branch_plan(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction* function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const PreparedShortCircuitContinuationLabels& continuation_labels) {
  const auto target_labels = resolve_prepared_compare_join_entry_target_labels(
      names, function_cf, function, source_block, continuation_labels);
  if (target_labels.true_label == kInvalidBlockLabel ||
      target_labels.false_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  return PreparedShortCircuitBranchPlan{
      .on_compare_true =
          PreparedShortCircuitTargetLabels{
              .block_label = target_labels.true_label,
              .continuation = std::nullopt,
          },
      .on_compare_false =
          PreparedShortCircuitTargetLabels{
              .block_label = target_labels.false_label,
              .continuation = std::nullopt,
          },
  };
}

[[nodiscard]] inline std::optional<PreparedShortCircuitJoinContext>
find_prepared_short_circuit_join_context(const PreparedNameTables& names,
                                         const PreparedControlFlowFunction& function_cf,
                                         const bir::Function& function,
                                         BlockLabelId source_block_label_id) {
  if (source_block_label_id == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer =
      find_authoritative_branch_owned_join_transfer(names, function_cf, source_block_label_id);
  if (!authoritative_join_transfer.has_value()) {
    return std::nullopt;
  }

  const auto join_sources =
      find_authoritative_short_circuit_join_sources(*authoritative_join_transfer);
  if (!join_sources.has_value() || join_sources->join_transfer == nullptr ||
      join_sources->true_transfer == nullptr || join_sources->false_transfer == nullptr) {
    return std::nullopt;
  }

  const auto* join_block =
      find_block_in_function(
          function, prepared_block_label(names, join_sources->join_transfer->join_block_label));
  if (join_block == nullptr) {
    return std::nullopt;
  }

  const auto continuation_targets = find_prepared_compare_join_continuation_targets(
      names, function_cf, function, source_block_label_id);
  if (!continuation_targets.has_value()) {
    return std::nullopt;
  }

  return PreparedShortCircuitJoinContext{
      .join_transfer = join_sources->join_transfer,
      .true_transfer = join_sources->true_transfer,
      .false_transfer = join_sources->false_transfer,
      .join_block = join_block,
      .classified_incoming = join_sources->classified_incoming,
      .continuation_true_label = continuation_targets->true_label,
      .continuation_false_label = continuation_targets->false_label,
  };
}

[[nodiscard]] inline std::optional<PreparedShortCircuitJoinContext>
find_prepared_short_circuit_join_context(const PreparedNameTables& names,
                                         const PreparedControlFlowFunction& function_cf,
                                         const bir::Function& function,
                                         std::string_view source_block_label) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block_label);
  if (!source_block_label_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_short_circuit_join_context(
      names, function_cf, function, *source_block_label_id);
}

[[nodiscard]] inline std::optional<PreparedShortCircuitBranchPlan>
find_prepared_short_circuit_branch_plan(
    const PreparedNameTables& names,
    const PreparedShortCircuitJoinContext& join_context,
    const PreparedBranchTargetLabels& direct_branch_targets) {
  if (join_context.true_transfer == nullptr || join_context.false_transfer == nullptr ||
      direct_branch_targets.true_label == kInvalidBlockLabel ||
      direct_branch_targets.false_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const bool short_circuit_on_compare_true =
      join_context.classified_incoming.short_circuit_on_compare_true;
  const bool short_circuit_on_compare_false = !short_circuit_on_compare_true;

  const auto rhs_entry_label = short_circuit_on_compare_true
                                   ? direct_branch_targets.false_label
                                   : direct_branch_targets.true_label;
  if (rhs_entry_label == kInvalidBlockLabel ||
      join_context.continuation_true_label == kInvalidBlockLabel ||
      join_context.continuation_false_label == kInvalidBlockLabel ||
      join_context.true_transfer->predecessor_label == kInvalidBlockLabel ||
      join_context.false_transfer->predecessor_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto on_compare_true = build_prepared_short_circuit_target_labels(
      names,
      *join_context.true_transfer,
      short_circuit_on_compare_true,
      join_context.classified_incoming.short_circuit_value,
      rhs_entry_label,
      join_context.continuation_true_label,
      join_context.continuation_false_label);
  const auto on_compare_false = build_prepared_short_circuit_target_labels(
      names,
      *join_context.false_transfer,
      short_circuit_on_compare_false,
      join_context.classified_incoming.short_circuit_value,
      rhs_entry_label,
      join_context.continuation_true_label,
      join_context.continuation_false_label);
  if (!on_compare_true.has_value() || !on_compare_false.has_value() ||
      on_compare_true->block_label == kInvalidBlockLabel ||
      on_compare_false->block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  return PreparedShortCircuitBranchPlan{
      .on_compare_true = *on_compare_true,
      .on_compare_false = *on_compare_false,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinBranches>
find_prepared_materialized_compare_join_branches(
    PreparedNameTables& names,
    const PreparedMaterializedCompareJoinContext& compare_join_context) {
  if (compare_join_context.join_transfer == nullptr || compare_join_context.true_transfer == nullptr ||
      compare_join_context.false_transfer == nullptr ||
      compare_join_context.true_predecessor == nullptr ||
      compare_join_context.false_predecessor == nullptr ||
      compare_join_context.true_predecessor == compare_join_context.false_predecessor) {
    return std::nullopt;
  }

  const auto true_return_context = find_prepared_materialized_compare_join_return_context(
      names, compare_join_context, compare_join_context.true_transfer->incoming_value);
  const auto false_return_context = find_prepared_materialized_compare_join_return_context(
      names, compare_join_context, compare_join_context.false_transfer->incoming_value);
  if (!true_return_context.has_value() || !false_return_context.has_value()) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinBranches{
      .compare_join_context = compare_join_context,
      .true_return_context = std::move(*true_return_context),
      .false_return_context = std::move(*false_return_context),
      .false_predecessor_label =
          compare_join_context.join_transfer->edge_transfers[*compare_join_context
                                                                  .join_transfer
                                                                  ->source_false_transfer_index]
              .predecessor_label,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinReturnContext>
find_prepared_materialized_compare_join_return_context(
    PreparedNameTables& names,
    const PreparedMaterializedCompareJoinContext& compare_join_context,
    const bir::Value& selected_value) {
  const auto* join_block = compare_join_context.join_block;
  if (join_block == nullptr || compare_join_context.function == nullptr ||
      compare_join_context.carrier_index > join_block->insts.size()) {
    return std::nullopt;
  }
  const auto function_name =
      resolve_prepared_function_name_id(names, compare_join_context.function->name);
  if (!function_name.has_value()) {
    return std::nullopt;
  }
  const auto block_it =
      std::find_if(compare_join_context.function->blocks.begin(),
                   compare_join_context.function->blocks.end(),
                   [&](const bir::Block& block) { return &block == join_block; });
  if (block_it == compare_join_context.function->blocks.end()) {
    return std::nullopt;
  }

  std::unordered_map<ValueNameId, const bir::BinaryInst*> binaries_by_value_name;
  std::unordered_map<ValueNameId, const bir::LoadGlobalInst*> global_loads_by_value_name;
  for (std::size_t inst_index = 0; inst_index < compare_join_context.carrier_index; ++inst_index) {
    if (const auto* binary = std::get_if<bir::BinaryInst>(&join_block->insts[inst_index]);
        binary != nullptr) {
      const auto result_name = prepared_named_value_id(names, binary->result);
      if (!result_name.has_value()) {
        return std::nullopt;
      }
      binaries_by_value_name.emplace(*result_name, binary);
      continue;
    }
    if (const auto* load_global =
            std::get_if<bir::LoadGlobalInst>(&join_block->insts[inst_index]);
        load_global != nullptr) {
      const auto result_name = prepared_named_value_id(names, load_global->result);
      if (!result_name.has_value()) {
        return std::nullopt;
      }
      global_loads_by_value_name.emplace(*result_name, load_global);
      continue;
    }
    return std::nullopt;
  }

  const auto computed_selected_value = classify_computed_value(
      names,
      compare_join_context.bir_names,
      selected_value,
      *compare_join_context.function,
      binaries_by_value_name,
      global_loads_by_value_name);
  if (!computed_selected_value.has_value()) {
    return std::nullopt;
  }

  std::optional<PreparedSupportedImmediateBinary> trailing_binary;
  if (compare_join_context.trailing_binary != nullptr) {
    trailing_binary = classify_supported_immediate_binary(
        names,
        *compare_join_context.trailing_binary,
        compare_join_context.carrier_result_name_id);
    if (!trailing_binary.has_value()) {
      return std::nullopt;
    }
  }

  return PreparedMaterializedCompareJoinReturnContext{
      .selected_value = std::move(*computed_selected_value),
      .trailing_binary = std::move(trailing_binary),
      .function_name = *function_name,
      .block_index =
          static_cast<std::size_t>(std::distance(compare_join_context.function->blocks.begin(),
                                                 block_it)),
      .instruction_index = compare_join_context.carrier_index +
                           (compare_join_context.trailing_binary != nullptr ? 1U : 0U),
  };
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinContext>
find_prepared_param_zero_materialized_compare_join_context(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    BlockLabelId source_block_label_id,
    const bir::Block& source_block,
    ValueNameId param_name_id,
    bool require_label_match) {
  const auto prepared_branch = find_prepared_param_zero_branch_condition(
      names, function_cf, source_block_label_id, source_block, param_name_id, require_label_match);
  if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer = find_authoritative_branch_owned_join_transfer(
      names, function_cf, source_block_label_id);
  if (!authoritative_join_transfer.has_value()) {
    return std::nullopt;
  }

  const auto compare_join_context = find_materialized_compare_join_context(
      names,
      bir_names,
      *authoritative_join_transfer,
      function,
      source_block,
      prepared_branch->branch_condition->true_label,
      prepared_branch->branch_condition->false_label);
  if (!compare_join_context.has_value()) {
    return std::nullopt;
  }

  return PreparedParamZeroMaterializedCompareJoinContext{
      .prepared_branch = *prepared_branch,
      .compare_join_context = *compare_join_context,
  };
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinContext>
find_prepared_param_zero_materialized_compare_join_context(
    const PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  const auto source_block_label_id =
      resolve_prepared_block_label_id(names, source_block.label);
  const auto param_name_id = resolve_prepared_value_name_id(names, param.name);
  if (!source_block_label_id.has_value() || !param_name_id.has_value()) {
    return std::nullopt;
  }
  return find_prepared_param_zero_materialized_compare_join_context(
      names,
      bir_names,
      function_cf,
      function,
      *source_block_label_id,
      source_block,
      *param_name_id,
      require_label_match);
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinContext>
find_prepared_param_zero_materialized_compare_join_context(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    BlockLabelId source_block_label_id,
    const bir::Block& source_block,
    ValueNameId param_name_id,
    bool require_label_match) {
  bir::NameTables bir_names;
  bir_names.import_link_names(names.texts, names.link_names);
  return find_prepared_param_zero_materialized_compare_join_context(
      names,
      bir_names,
      function_cf,
      function,
      source_block_label_id,
      source_block,
      param_name_id,
      require_label_match);
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinBranches>
find_prepared_param_zero_materialized_compare_join_branches(
    PreparedNameTables& names,
    const bir::NameTables& bir_names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  const auto prepared_compare_join_context =
      find_prepared_param_zero_materialized_compare_join_context(
          names, bir_names, function_cf, function, source_block, param, require_label_match);
  if (!prepared_compare_join_context.has_value()) {
    return std::nullopt;
  }

  const auto prepared_join_branches = find_prepared_materialized_compare_join_branches(
      names, prepared_compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return std::nullopt;
  }

  return PreparedParamZeroMaterializedCompareJoinBranches{
      .prepared_branch = prepared_compare_join_context->prepared_branch,
      .prepared_join_branches = std::move(*prepared_join_branches),
  };
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinBranches>
find_prepared_param_zero_materialized_compare_join_branches(
    PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  bir::NameTables bir_names;
  bir_names.import_link_names(names.texts, names.link_names);
  return find_prepared_param_zero_materialized_compare_join_branches(
      names, bir_names, function_cf, function, source_block, param, require_label_match);
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinBranchPlan>
find_prepared_materialized_compare_join_branch_plan(
    const PreparedNameTables& names,
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches) {
  if (prepared_compare_join_branches.prepared_branch.branch_condition == nullptr ||
      prepared_compare_join_branches.prepared_branch.false_branch_opcode == nullptr) {
    return std::nullopt;
  }
  const auto& branch_condition = *prepared_compare_join_branches.prepared_branch.branch_condition;
  if (branch_condition.true_label == kInvalidBlockLabel ||
      branch_condition.false_label == kInvalidBlockLabel) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinBranchPlan{
      .target_labels =
          PreparedBranchTargetLabels{
              .true_label = branch_condition.true_label,
              .false_label = branch_condition.false_label,
          },
      .false_branch_opcode =
          prepared_compare_join_branches.prepared_branch.false_branch_opcode,
      .compare_shape = prepared_compare_join_branches.prepared_branch.compare_shape,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinRenderContract>
find_prepared_materialized_compare_join_render_contract(
    PreparedNameTables& names,
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches) {
  const auto branch_plan =
      find_prepared_materialized_compare_join_branch_plan(names, prepared_compare_join_branches);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinRenderContract{
      .branch_plan = std::move(*branch_plan),
      .true_return = build_prepared_materialized_compare_join_return_arm(
          prepared_compare_join_branches.prepared_join_branches.true_return_context),
      .false_return = build_prepared_materialized_compare_join_return_arm(
          prepared_compare_join_branches.prepared_join_branches.false_return_context),
      .same_module_global_names =
          collect_prepared_materialized_compare_join_same_module_globals(
              names,
              prepared_compare_join_branches.prepared_join_branches),
      .same_module_global_refs =
          collect_prepared_materialized_compare_join_same_module_global_refs(
              names,
              prepared_compare_join_branches.prepared_join_branches),
  };
}

[[nodiscard]] inline std::vector<std::string_view>
collect_prepared_materialized_compare_join_same_module_globals(
    const PreparedNameTables& names,
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches) {
  std::vector<std::string_view> global_names;
  for (const auto ref : collect_prepared_materialized_compare_join_same_module_global_refs(
           names, prepared_join_branches)) {
    global_names.push_back(ref.name);
  }
  return global_names;
}

[[nodiscard]] inline std::vector<PreparedSameModuleGlobalRef>
collect_prepared_materialized_compare_join_same_module_global_refs(
    const PreparedNameTables& names,
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches) {
  std::vector<PreparedSameModuleGlobalRef> global_refs;
  std::unordered_set<std::string_view> seen_names;
  const auto append_names =
      [&](const PreparedMaterializedCompareJoinReturnContext& return_context) {
        for (const auto ref :
             collect_prepared_computed_value_same_module_global_refs(
                 names, return_context.selected_value)) {
          if (seen_names.insert(ref.name).second) {
            global_refs.push_back(ref);
          }
        }
      };
  append_names(prepared_join_branches.true_return_context);
  append_names(prepared_join_branches.false_return_context);
  return global_refs;
}

[[nodiscard]] inline std::string_view bir_link_name_or_raw(
    const bir::NameTables& bir_names,
    const bir::Global& global) {
  if (global.link_name_id == kInvalidLinkName) {
    return global.name;
  }
  const std::string_view semantic_name = bir_names.link_names.spelling(global.link_name_id);
  if (semantic_name.empty() || (!global.name.empty() && global.name != semantic_name)) {
    return {};
  }
  return semantic_name;
}

[[nodiscard]] inline std::optional<std::vector<const bir::Global*>>
resolve_prepared_materialized_compare_join_same_module_globals(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract) {
  const auto find_same_module_global =
      [&](PreparedSameModuleGlobalRef global_ref) -> const bir::Global* {
    if (global_ref.name_id != kInvalidLinkName) {
      if (global_ref.name.empty() ||
          prepared_link_name(names, global_ref.name_id) != global_ref.name) {
        return nullptr;
      }
      for (const auto& global : module.globals) {
        if (bir_link_name_or_raw(module.names, global) != global_ref.name) {
          continue;
        }
        if (global.is_extern || global.is_thread_local) {
          return nullptr;
        }
        return &global;
      }
      return nullptr;
    }
    if (global_ref.name.empty()) {
      return nullptr;
    }
    for (const auto& global : module.globals) {
      if (bir_link_name_or_raw(module.names, global) != global_ref.name) {
        continue;
      }
      if (global.is_extern || global.is_thread_local) {
        return nullptr;
      }
      return &global;
    }
    return nullptr;
  };

  const auto global_refs = render_contract.same_module_global_refs.empty()
                               ? [&]() {
                                   std::vector<PreparedSameModuleGlobalRef> refs;
                                   refs.reserve(render_contract.same_module_global_names.size());
                                   for (const auto name :
                                        render_contract.same_module_global_names) {
                                     refs.push_back(PreparedSameModuleGlobalRef{
                                         .name = name,
                                         .name_id = kInvalidLinkName,
                                     });
                                   }
                                   return refs;
                                 }()
                               : render_contract.same_module_global_refs;
  std::vector<const bir::Global*> globals;
  globals.reserve(global_refs.size());
  for (const auto global_ref : global_refs) {
    const auto* resolved_global = find_same_module_global(global_ref);
    if (resolved_global == nullptr) {
      return std::nullopt;
    }
    globals.push_back(resolved_global);
  }
  return globals;
}

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinReturnArm>
resolve_prepared_materialized_compare_join_return_arm(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinReturnArm& return_arm) {
  const auto find_same_module_global =
      [&](PreparedSameModuleGlobalRef global_ref) -> const bir::Global* {
    if (global_ref.name_id != kInvalidLinkName) {
      if (global_ref.name.empty() ||
          prepared_link_name(names, global_ref.name_id) != global_ref.name) {
        return nullptr;
      }
      for (const auto& global : module.globals) {
        if (bir_link_name_or_raw(module.names, global) != global_ref.name) {
          continue;
        }
        if (global.is_extern || global.is_thread_local) {
          return nullptr;
        }
        return &global;
      }
      return nullptr;
    }
    if (global_ref.name.empty()) {
      return nullptr;
    }
    for (const auto& global : module.globals) {
      if (bir_link_name_or_raw(module.names, global) != global_ref.name) {
        continue;
      }
      if (global.is_extern || global.is_thread_local) {
        return nullptr;
      }
      return &global;
    }
    return nullptr;
  };

  PreparedResolvedMaterializedCompareJoinReturnArm resolved_return{
      .arm = return_arm,
  };
  const auto& selected_value = return_arm.context.selected_value;
  switch (selected_value.base.kind) {
    case PreparedComputedBaseKind::ImmediateI32:
    case PreparedComputedBaseKind::ParamValue:
      break;
    case PreparedComputedBaseKind::GlobalI32Load:
      resolved_return.global =
          find_same_module_global(
              prepared_computed_base_global_ref(names, selected_value.base));
      if (resolved_return.global == nullptr) {
        return std::nullopt;
      }
      break;
    case PreparedComputedBaseKind::PointerBackedGlobalI32Load:
      resolved_return.pointer_root_global =
          find_same_module_global(
              prepared_computed_base_pointer_root_ref(names, selected_value.base));
      resolved_return.global =
          find_same_module_global(
              prepared_computed_base_global_ref(names, selected_value.base));
      if (resolved_return.pointer_root_global == nullptr ||
          resolved_return.pointer_root_global->type != bir::TypeKind::Ptr ||
          resolved_return.global == nullptr) {
        return std::nullopt;
      }
      break;
  }

  return resolved_return;
}

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
resolve_prepared_materialized_compare_join_render_contract(
    const PreparedNameTables& names,
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract) {
  const auto same_module_globals =
      resolve_prepared_materialized_compare_join_same_module_globals(names, module, render_contract);
  if (!same_module_globals.has_value()) {
    return std::nullopt;
  }
  const auto true_return =
      resolve_prepared_materialized_compare_join_return_arm(names, module, render_contract.true_return);
  const auto false_return =
      resolve_prepared_materialized_compare_join_return_arm(names, module, render_contract.false_return);
  if (!true_return.has_value() || !false_return.has_value()) {
    return std::nullopt;
  }

  return PreparedResolvedMaterializedCompareJoinRenderContract{
      .branch_plan = render_contract.branch_plan,
      .true_return = std::move(*true_return),
      .false_return = std::move(*false_return),
      .same_module_globals = std::move(*same_module_globals),
  };
}

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
find_prepared_resolved_materialized_compare_join_render_contract(
    PreparedNameTables& names,
    const bir::Module& module,
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches) {
  const auto render_contract =
      find_prepared_materialized_compare_join_render_contract(names,
                                                              prepared_compare_join_branches);
  if (!render_contract.has_value()) {
    return std::nullopt;
  }
  return resolve_prepared_materialized_compare_join_render_contract(names, module, *render_contract);
}

[[nodiscard]] inline std::optional<PreparedResolvedMaterializedCompareJoinRenderContract>
find_prepared_param_zero_resolved_materialized_compare_join_render_contract(
    PreparedNameTables& names,
    const bir::Module& module,
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  const auto prepared_compare_join_branches =
      find_prepared_param_zero_materialized_compare_join_branches(names,
                                                                  module.names,
                                                                  function_cf,
                                                                  function,
                                                                  source_block,
                                                                  param,
                                                                  require_label_match);
  if (!prepared_compare_join_branches.has_value()) {
    return std::nullopt;
  }
  return find_prepared_resolved_materialized_compare_join_render_contract(
      names, module, *prepared_compare_join_branches);
}

[[nodiscard]] inline const std::vector<PreparedEdgeValueTransfer>* incoming_transfers_for_join(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId join_block_label,
    ValueNameId destination_value_name) {
  const auto* transfer =
      find_prepared_join_transfer(names, function_cf, join_block_label, destination_value_name);
  if (transfer == nullptr) {
    return nullptr;
  }
  return &transfer->edge_transfers;
}

[[nodiscard]] inline const PreparedParallelCopyBundle* find_prepared_parallel_copy_bundle(
    const PreparedControlFlowFunction& function_cf,
    BlockLabelId predecessor_label,
    BlockLabelId successor_label) {
  for (const auto& bundle : function_cf.parallel_copy_bundles) {
    if (bundle.predecessor_label == predecessor_label &&
        bundle.successor_label == successor_label) {
      return &bundle;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedParallelCopyMove* find_prepared_parallel_copy_move_for_step(
    const PreparedParallelCopyBundle& bundle,
    const PreparedParallelCopyStep& step) {
  if (step.move_index >= bundle.moves.size()) {
    return nullptr;
  }
  return &bundle.moves[step.move_index];
}

[[nodiscard]] inline const PreparedParallelCopyMove* find_prepared_parallel_copy_move_for_step(
    const PreparedParallelCopyBundle& bundle,
    std::size_t step_index) {
  if (step_index >= bundle.steps.size()) {
    return nullptr;
  }
  return find_prepared_parallel_copy_move_for_step(bundle, bundle.steps[step_index]);
}

[[nodiscard]] inline PreparedParallelCopyMove* find_prepared_parallel_copy_move_for_step(
    PreparedParallelCopyBundle& bundle,
    std::size_t step_index) {
  return const_cast<PreparedParallelCopyMove*>(find_prepared_parallel_copy_move_for_step(
      static_cast<const PreparedParallelCopyBundle&>(bundle), step_index));
}

[[nodiscard]] inline std::optional<BlockLabelId> published_prepared_parallel_copy_execution_block_label(
    const PreparedParallelCopyBundle& bundle) {
  return bundle.execution_block_label;
}

[[nodiscard]] inline const PreparedParallelCopyBundle* find_prepared_parallel_copy_bundle(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function_cf,
    std::string_view predecessor_label,
    std::string_view successor_label) {
  const BlockLabelId predecessor_id = names.block_labels.find(predecessor_label);
  const BlockLabelId successor_id = names.block_labels.find(successor_label);
  if (predecessor_id == kInvalidBlockLabel || successor_id == kInvalidBlockLabel) {
    return nullptr;
  }
  return find_prepared_parallel_copy_bundle(function_cf, predecessor_id, successor_id);
}

enum class PreparedBirInvariant {
  NoTargetFacingI1,
  NoPhiNodes,
};

[[nodiscard]] constexpr std::string_view prepared_bir_invariant_name(
    PreparedBirInvariant invariant) {
  switch (invariant) {
    case PreparedBirInvariant::NoTargetFacingI1:
      return "no_target_facing_i1";
    case PreparedBirInvariant::NoPhiNodes:
      return "no_phi_nodes";
  }
  return "unknown";
}

// Step 5 fence: block-index lookup compares a prepared block-label ID against
// the route-local BIR block label text because prepared move bundles are still
// keyed by block index. Remove this bridge when prepared control-flow consumers
// carry a structured BIR block handle/index from the producer.
[[nodiscard]] inline std::optional<std::size_t> find_prepared_block_index_in_function(
    const PreparedNameTables& names,
    const bir::Function& function,
    BlockLabelId block_label) {
  if (block_label == kInvalidBlockLabel) {
    return std::nullopt;
  }
  const auto block_name = prepared_block_label(names, block_label);
  for (std::size_t block_index = 0; block_index < function.blocks.size(); ++block_index) {
    if (function.blocks[block_index].label == block_name) {
      return block_index;
    }
  }
  return std::nullopt;
}

// Step 5 fence: parallel-copy execution indexes are route-local scheduling
// coordinates. The authoritative ownership remains the prepared parallel-copy
// bundle labels plus OutOfSsaParallelCopy authority, not a semantic name.
[[nodiscard]] inline std::optional<std::size_t>
published_prepared_parallel_copy_execution_block_index(
    const PreparedNameTables& names,
    const bir::Function& function,
    const PreparedParallelCopyBundle& parallel_copy_bundle) {
  const auto execution_block_label =
      published_prepared_parallel_copy_execution_block_label(parallel_copy_bundle);
  if (!execution_block_label.has_value()) {
    return std::nullopt;
  }
  return find_prepared_block_index_in_function(names, function, *execution_block_label);
}

// Step 5 fence: this helper intentionally matches the out-of-SSA move bundle
// by prepared execution site, source edge labels, and authority kind. It is
// route-local parallel-copy state and must not be reused as semantic identity.
[[nodiscard]] inline const PreparedMoveBundle*
find_prepared_out_of_ssa_parallel_copy_move_bundle(
    const PreparedNameTables& names,
    const bir::Function& function,
    const PreparedValueLocationFunction& function_locations,
    const PreparedParallelCopyBundle& parallel_copy_bundle) {
  const auto block_index = published_prepared_parallel_copy_execution_block_index(
      names, function, parallel_copy_bundle);
  if (!block_index.has_value()) {
    return nullptr;
  }
  const PreparedMoveBundle* match = nullptr;
  for (const auto& move_bundle : function_locations.move_bundles) {
    if (move_bundle.phase != PreparedMovePhase::BlockEntry ||
        move_bundle.authority_kind != PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
        move_bundle.block_index != *block_index) {
      continue;
    }
    if (move_bundle.source_parallel_copy_predecessor_label != parallel_copy_bundle.predecessor_label ||
        move_bundle.source_parallel_copy_successor_label != parallel_copy_bundle.successor_label) {
      continue;
    }
    if (match != nullptr) {
      return nullptr;
    }
    match = &move_bundle;
  }
  return match;
}

[[nodiscard]] inline PreparedMoveBundle* find_prepared_out_of_ssa_parallel_copy_move_bundle(
    const PreparedNameTables& names,
    const bir::Function& function,
    PreparedValueLocationFunction& function_locations,
    const PreparedParallelCopyBundle& parallel_copy_bundle) {
  return const_cast<PreparedMoveBundle*>(find_prepared_out_of_ssa_parallel_copy_move_bundle(
      names,
      function,
      static_cast<const PreparedValueLocationFunction&>(function_locations),
      parallel_copy_bundle));
}

[[nodiscard]] inline const PreparedMoveResolution*
find_prepared_out_of_ssa_parallel_copy_move_for_step(
    const PreparedNameTables& names,
    const bir::Function& function,
    const PreparedValueLocationFunction& function_locations,
    const PreparedParallelCopyBundle& parallel_copy_bundle,
    std::size_t parallel_copy_step_index) {
  const auto* move_bundle = find_prepared_out_of_ssa_parallel_copy_move_bundle(
      names, function, function_locations, parallel_copy_bundle);
  if (move_bundle == nullptr) {
    return nullptr;
  }
  return find_prepared_out_of_ssa_parallel_copy_move_for_step(*move_bundle,
                                                              parallel_copy_step_index);
}

[[nodiscard]] inline PreparedMoveResolution*
find_prepared_out_of_ssa_parallel_copy_move_for_step(
    const PreparedNameTables& names,
    const bir::Function& function,
    PreparedValueLocationFunction& function_locations,
    const PreparedParallelCopyBundle& parallel_copy_bundle,
    std::size_t parallel_copy_step_index) {
  return const_cast<PreparedMoveResolution*>(find_prepared_out_of_ssa_parallel_copy_move_for_step(
      names,
      function,
      static_cast<const PreparedValueLocationFunction&>(function_locations),
      parallel_copy_bundle,
      parallel_copy_step_index));
}


}  // namespace c4c::backend::prepare

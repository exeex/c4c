#pragma once

#include "../bir/bir.hpp"
#include "../../target_profile.hpp"

#include <cstdlib>
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

using PreparedObjectId = std::size_t;
using PreparedValueId = std::size_t;
using PreparedFrameSlotId = std::size_t;

struct PrepareOptions {
  bool run_legalize = true;
  bool run_stack_layout = true;
  bool run_liveness = true;
  bool run_regalloc = true;
};

struct PrepareNote {
  std::string phase;
  std::string message;
};

struct PreparedStackObject {
  PreparedObjectId object_id = 0;
  std::string function_name;
  std::string source_name;
  std::string source_kind;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool address_exposed = false;
  bool requires_home_slot = false;
  bool permanent_home_slot = false;
};

struct PreparedFrameSlot {
  PreparedFrameSlotId slot_id = 0;
  PreparedObjectId object_id = 0;
  std::string function_name;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

struct PreparedStackLayout {
  std::vector<PreparedStackObject> objects;
  std::vector<PreparedFrameSlot> frame_slots;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
};

namespace stack_layout {

struct FunctionInlineAsmSummary {
  std::size_t instruction_count = 0;
  bool has_side_effects = false;
};

std::vector<PreparedStackObject> collect_function_stack_objects(const bir::Function& function,
                                                                PreparedObjectId& next_object_id);

void apply_alloca_coalescing_hints(const bir::Function& function,
                                   std::vector<PreparedStackObject>& objects);

void apply_copy_coalescing_hints(const bir::Function& function,
                                 std::vector<PreparedStackObject>& objects);

FunctionInlineAsmSummary summarize_inline_asm(const bir::Function& function);

void apply_regalloc_hints(const bir::Function& function,
                          const FunctionInlineAsmSummary& inline_asm_summary,
                          std::vector<PreparedStackObject>& objects);

std::vector<PreparedFrameSlot> assign_frame_slots(const std::vector<PreparedStackObject>& objects,
                                                  PreparedFrameSlotId& next_slot_id,
                                                  std::size_t& frame_size_bytes,
                                                  std::size_t& frame_alignment_bytes);

}  // namespace stack_layout

enum class PreparedValueKind {
  StackObject,
  Parameter,
  CallResult,
  Phi,
  Temporary,
};

[[nodiscard]] constexpr std::string_view prepared_value_kind_name(PreparedValueKind kind) {
  switch (kind) {
    case PreparedValueKind::StackObject:
      return "stack_object";
    case PreparedValueKind::Parameter:
      return "parameter";
    case PreparedValueKind::CallResult:
      return "call_result";
    case PreparedValueKind::Phi:
      return "phi";
    case PreparedValueKind::Temporary:
      return "temporary";
  }
  return "unknown";
}

struct PreparedLiveInterval {
  PreparedValueId value_id = 0;
  std::size_t start_point = 0;
  std::size_t end_point = 0;
};

struct PreparedLivenessValue {
  PreparedValueId value_id = 0;
  std::optional<PreparedObjectId> stack_object_id;
  std::string function_name;
  std::string value_name;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  bool address_taken = false;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::optional<std::size_t> definition_point;
  std::vector<std::size_t> use_points;
  std::optional<PreparedLiveInterval> live_interval;
};

struct PreparedLivenessBlock {
  std::string block_name;
  std::size_t block_index = 0;
  std::size_t start_point = 0;
  std::size_t end_point = 0;
  std::vector<std::size_t> predecessor_block_indices;
  std::vector<std::size_t> successor_block_indices;
  std::vector<PreparedValueId> live_in;
  std::vector<PreparedValueId> live_out;
};

struct PreparedLivenessFunction {
  std::string function_name;
  std::size_t instruction_count = 0;
  std::vector<PreparedLiveInterval> intervals;
  std::vector<std::size_t> call_points;
  std::vector<std::size_t> block_loop_depth;
  std::vector<PreparedLivenessBlock> blocks;
  std::vector<PreparedLivenessValue> values;
};

struct PreparedLiveness {
  std::vector<PreparedLivenessFunction> functions;
};

enum class PreparedRegisterClass {
  None,
  General,
  Float,
  Vector,
  AggregateAddress,
};

[[nodiscard]] constexpr std::string_view prepared_register_class_name(
    PreparedRegisterClass reg_class) {
  switch (reg_class) {
    case PreparedRegisterClass::None:
      return "none";
    case PreparedRegisterClass::General:
      return "general";
    case PreparedRegisterClass::Float:
      return "float";
    case PreparedRegisterClass::Vector:
      return "vector";
    case PreparedRegisterClass::AggregateAddress:
      return "aggregate_address";
  }
  return "unknown";
}

enum class PreparedAllocationStatus {
  Unallocated,
  AssignedRegister,
  AssignedStackSlot,
  Split,
  Spilled,
};

[[nodiscard]] constexpr std::string_view prepared_allocation_status_name(
    PreparedAllocationStatus status) {
  switch (status) {
    case PreparedAllocationStatus::Unallocated:
      return "unallocated";
    case PreparedAllocationStatus::AssignedRegister:
      return "assigned_register";
    case PreparedAllocationStatus::AssignedStackSlot:
      return "assigned_stack_slot";
    case PreparedAllocationStatus::Split:
      return "split";
    case PreparedAllocationStatus::Spilled:
      return "spilled";
  }
  return "unknown";
}

enum class PreparedSpillReloadOpKind {
  Spill,
  Reload,
  Rematerialize,
};

[[nodiscard]] constexpr std::string_view prepared_spill_reload_op_kind_name(
    PreparedSpillReloadOpKind kind) {
  switch (kind) {
    case PreparedSpillReloadOpKind::Spill:
      return "spill";
    case PreparedSpillReloadOpKind::Reload:
      return "reload";
    case PreparedSpillReloadOpKind::Rematerialize:
      return "rematerialize";
  }
  return "unknown";
}

struct PreparedPhysicalRegisterAssignment {
  PreparedRegisterClass reg_class = PreparedRegisterClass::None;
  std::string register_name;
};

struct PreparedStackSlotAssignment {
  PreparedFrameSlotId slot_id = 0;
  std::size_t offset_bytes = 0;
};

struct PreparedAllocationConstraint {
  PreparedValueId value_id = 0;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  bool requires_register = false;
  bool requires_home_slot = false;
  bool cannot_cross_call = false;
  std::optional<std::string> fixed_register_name;
  std::vector<std::string> preferred_register_names;
  std::vector<std::string> forbidden_register_names;
};

struct PreparedInterferenceEdge {
  PreparedValueId lhs_value_id = 0;
  PreparedValueId rhs_value_id = 0;
  std::string reason;
};

enum class PreparedMoveStorageKind {
  None,
  Register,
  StackSlot,
};

enum class PreparedMoveDestinationKind {
  Value,
  CallArgumentAbi,
  CallResultAbi,
  FunctionReturnAbi,
};

struct PreparedMoveResolution {
  PreparedValueId from_value_id = 0;
  PreparedValueId to_value_id = 0;
  PreparedMoveDestinationKind destination_kind = PreparedMoveDestinationKind::Value;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::optional<std::string> destination_register_name;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::string reason;
};

struct PreparedSpillReloadOp {
  PreparedValueId value_id = 0;
  PreparedSpillReloadOpKind op_kind = PreparedSpillReloadOpKind::Spill;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
};

struct PreparedRegallocValue {
  PreparedValueId value_id = 0;
  std::optional<PreparedObjectId> stack_object_id;
  std::string function_name;
  std::string value_name;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  PreparedValueKind value_kind = PreparedValueKind::Temporary;
  PreparedRegisterClass register_class = PreparedRegisterClass::None;
  PreparedAllocationStatus allocation_status = PreparedAllocationStatus::Unallocated;
  bool spillable = true;
  bool requires_home_slot = false;
  bool crosses_call = false;
  std::size_t priority = 0;
  double spill_weight = 0.0;
  std::optional<PreparedLiveInterval> live_interval;
  std::optional<PreparedPhysicalRegisterAssignment> assigned_register;
  std::optional<PreparedStackSlotAssignment> assigned_stack_slot;
};

struct PreparedRegallocFunction {
  std::string function_name;
  std::vector<PreparedRegallocValue> values;
  std::vector<PreparedAllocationConstraint> constraints;
  std::vector<PreparedInterferenceEdge> interference;
  std::vector<PreparedMoveResolution> move_resolution;
  std::vector<PreparedSpillReloadOp> spill_reload_ops;
};

struct PreparedRegalloc {
  std::vector<PreparedRegallocFunction> functions;
};

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
  SelectMaterialization,
  EdgeStoreSlot,
  LoopCarry,
};

[[nodiscard]] constexpr std::string_view prepared_join_transfer_kind_name(
    PreparedJoinTransferKind kind) {
  switch (kind) {
    case PreparedJoinTransferKind::SelectMaterialization:
      return "select_materialization";
    case PreparedJoinTransferKind::EdgeStoreSlot:
      return "edge_store_slot";
    case PreparedJoinTransferKind::LoopCarry:
      return "loop_carry";
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
  std::string predecessor_label;
  std::string successor_label;
  bir::Value incoming_value;
  bir::Value destination_value;
  std::optional<std::string> storage_name;
};

struct PreparedBranchCondition {
  std::string function_name;
  std::string block_label;
  PreparedBranchConditionKind kind = PreparedBranchConditionKind::MaterializedBool;
  bir::Value condition_value;
  std::optional<bir::BinaryOpcode> predicate;
  std::optional<bir::TypeKind> compare_type;
  std::optional<bir::Value> lhs;
  std::optional<bir::Value> rhs;
  bool can_fuse_with_branch = false;
  std::string true_label;
  std::string false_label;
};

struct PreparedJoinTransfer {
  std::string function_name;
  std::string join_block_label;
  bir::Value result;
  PreparedJoinTransferKind kind = PreparedJoinTransferKind::SelectMaterialization;
  std::optional<std::string> storage_name;
  std::vector<bir::PhiIncoming> incomings;
  std::vector<PreparedEdgeValueTransfer> edge_transfers;
  std::optional<std::string> source_branch_block_label;
  std::optional<std::size_t> source_true_transfer_index;
  std::optional<std::size_t> source_false_transfer_index;
  std::optional<std::string> source_true_incoming_label;
  std::optional<std::string> source_false_incoming_label;
};

struct PreparedControlFlowFunction {
  std::string function_name;
  std::vector<PreparedBranchCondition> branch_conditions;
  std::vector<PreparedJoinTransfer> join_transfers;
};

struct PreparedAuthoritativeBranchJoinTransfer {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
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
  std::string_view continuation_true_label;
  std::string_view continuation_false_label;
};

struct PreparedShortCircuitContinuationLabels {
  std::string_view incoming_label;
  std::string_view true_label;
  std::string_view false_label;
};

struct PreparedShortCircuitTargetLabels {
  std::string_view block_label;
  std::optional<PreparedShortCircuitContinuationLabels> continuation;
};

struct PreparedShortCircuitBranchPlan {
  PreparedShortCircuitTargetLabels on_compare_true;
  PreparedShortCircuitTargetLabels on_compare_false;
};

struct PreparedJoinCarrier {
  std::size_t carrier_index = 0;
  std::string_view result_name;
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
  std::string param_name;
  std::string global_name;
  std::size_t global_byte_offset = 0;
  std::string pointer_root_global_name;
};

struct PreparedComputedValue {
  PreparedComputedBase base;
  std::vector<PreparedSupportedImmediateBinary> operations;
};

struct PreparedMaterializedCompareJoinContext {
  const PreparedJoinTransfer* join_transfer = nullptr;
  const PreparedEdgeValueTransfer* true_transfer = nullptr;
  const PreparedEdgeValueTransfer* false_transfer = nullptr;
  const bir::Function* function = nullptr;
  const bir::Block* true_predecessor = nullptr;
  const bir::Block* false_predecessor = nullptr;
  const bir::Block* join_block = nullptr;
  const bir::BinaryInst* trailing_binary = nullptr;
  std::size_t carrier_index = 0;
  std::string_view carrier_result_name;
};

struct PreparedMaterializedCompareJoinReturnContext {
  PreparedComputedValue selected_value;
  std::optional<PreparedSupportedImmediateBinary> trailing_binary;
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
  std::string_view false_predecessor_label;
};

struct PreparedCompareJoinContinuationTargets {
  std::string_view true_label;
  std::string_view false_label;
};

struct PreparedBranchTargetLabels {
  std::string_view true_label;
  std::string_view false_label;
};

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
    std::string_view function_name) {
  for (const auto& function : control_flow.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedBranchCondition* find_prepared_branch_condition(
    const PreparedControlFlowFunction& function_cf,
    std::string_view block_label) {
  for (const auto& condition : function_cf.branch_conditions) {
    if (condition.block_label == block_label) {
      return &condition;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::optional<PreparedBranchTargetLabels>
find_prepared_compare_branch_target_labels(const PreparedBranchCondition& branch_condition,
                                           const bir::Block& source_block) {
  if (source_block.terminator.kind != bir::TerminatorKind::CondBranch ||
      source_block.terminator.condition.kind != bir::Value::Kind::Named ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() || !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value() ||
      *branch_condition.compare_type != bir::TypeKind::I32 ||
      branch_condition.condition_value.kind != bir::Value::Kind::Named ||
      branch_condition.condition_value.name != source_block.terminator.condition.name) {
    return std::nullopt;
  }

  return PreparedBranchTargetLabels{
      .true_label = branch_condition.true_label,
      .false_label = branch_condition.false_label,
  };
}

[[nodiscard]] inline std::optional<PreparedShortCircuitTargetLabels>
build_prepared_short_circuit_target_labels(
    const PreparedEdgeValueTransfer& transfer,
    bool is_short_circuit_lane,
    bool short_circuit_value,
    std::string_view rhs_entry_label,
    std::string_view continuation_true_label,
    std::string_view continuation_false_label) {
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
};

struct PreparedMaterializedCompareJoinRenderContract {
  PreparedMaterializedCompareJoinBranchPlan branch_plan;
  PreparedMaterializedCompareJoinReturnContext true_return_context;
  PreparedMaterializedCompareJoinReturnShape true_return_shape;
  PreparedMaterializedCompareJoinReturnContext false_return_context;
  PreparedMaterializedCompareJoinReturnShape false_return_shape;
  std::vector<std::string_view> same_module_global_names;
};

[[nodiscard]] inline std::vector<std::string_view>
collect_prepared_computed_value_same_module_globals(
    const PreparedComputedValue& computed_value) {
  std::vector<std::string_view> names;
  switch (computed_value.base.kind) {
    case PreparedComputedBaseKind::ImmediateI32:
    case PreparedComputedBaseKind::ParamValue:
      break;
    case PreparedComputedBaseKind::GlobalI32Load:
      names.push_back(computed_value.base.global_name);
      break;
    case PreparedComputedBaseKind::PointerBackedGlobalI32Load:
      names.push_back(computed_value.base.pointer_root_global_name);
      names.push_back(computed_value.base.global_name);
      break;
  }
  return names;
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinReturnContext>
find_prepared_materialized_compare_join_return_context(
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

[[nodiscard]] inline std::vector<std::string_view>
collect_prepared_materialized_compare_join_same_module_globals(
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches);

[[nodiscard]] inline std::optional<std::vector<const bir::Global*>>
resolve_prepared_materialized_compare_join_same_module_globals(
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract);

[[nodiscard]] inline std::optional<PreparedParamZeroBranchCondition>
find_prepared_param_zero_branch_condition(const PreparedControlFlowFunction& function_cf,
                                          const bir::Block& source_block,
                                          const bir::Param& param,
                                          bool require_label_match) {
  if (source_block.terminator.kind != bir::TerminatorKind::CondBranch ||
      source_block.terminator.condition.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto* branch_condition = find_prepared_branch_condition(function_cf, source_block.label);
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
      (branch_condition->true_label != source_block.terminator.true_label ||
       branch_condition->false_label != source_block.terminator.false_label)) {
    return std::nullopt;
  }

  const bool lhs_is_param_rhs_is_zero =
      branch_condition->lhs->kind == bir::Value::Kind::Named &&
      branch_condition->lhs->name == param.name &&
      branch_condition->rhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->rhs->type == bir::TypeKind::I32 &&
      branch_condition->rhs->immediate == 0;
  const bool rhs_is_param_lhs_is_zero =
      branch_condition->rhs->kind == bir::Value::Kind::Named &&
      branch_condition->rhs->name == param.name &&
      branch_condition->lhs->kind == bir::Value::Kind::Immediate &&
      branch_condition->lhs->type == bir::TypeKind::I32 &&
      branch_condition->lhs->immediate == 0;
  if (!lhs_is_param_rhs_is_zero && !rhs_is_param_lhs_is_zero) {
    return std::nullopt;
  }

  if (*branch_condition->predicate == bir::BinaryOpcode::Eq) {
    return PreparedParamZeroBranchCondition{
        .branch_condition = branch_condition,
        .false_label = branch_condition->false_label,
        .false_branch_opcode = "jne",
    };
  }
  if (*branch_condition->predicate == bir::BinaryOpcode::Ne) {
    return PreparedParamZeroBranchCondition{
        .branch_condition = branch_condition,
        .false_label = branch_condition->false_label,
        .false_branch_opcode = "je",
    };
  }

  return std::nullopt;
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
find_prepared_param_zero_branch_return_context(const PreparedControlFlowFunction& function_cf,
                                               const bir::Function& function,
                                               const bir::Block& source_block,
                                               const bir::Param& param,
                                               bool require_label_match) {
  const auto prepared_branch = find_prepared_param_zero_branch_condition(function_cf,
                                                                         source_block,
                                                                         param,
                                                                         require_label_match);
  if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto* branch_condition = prepared_branch->branch_condition;
  const auto* true_block = find_block_in_function(function, branch_condition->true_label);
  const auto* false_block = find_block_in_function(function, branch_condition->false_label);
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
    const PreparedControlFlowFunction& function_cf,
    std::string_view join_block_label,
    std::string_view destination_value_name) {
  for (const auto& transfer : function_cf.join_transfers) {
    if (transfer.join_block_label == join_block_label &&
        transfer.result.kind == bir::Value::Kind::Named &&
        transfer.result.name == destination_value_name) {
      return &transfer;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const PreparedJoinTransfer* find_branch_owned_join_transfer(
    const PreparedControlFlowFunction& function_cf,
    std::string_view source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<std::string_view> true_predecessor_label = std::nullopt,
    std::optional<std::string_view> false_predecessor_label = std::nullopt) {
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
    const PreparedControlFlowFunction& function_cf,
    std::string_view source_branch_block_label,
    std::optional<std::string_view> true_predecessor_label = std::nullopt,
    std::optional<std::string_view> false_predecessor_label = std::nullopt) {
  return find_branch_owned_join_transfer(function_cf,
                                         source_branch_block_label,
                                         PreparedJoinTransferKind::SelectMaterialization,
                                         true_predecessor_label,
                                         false_predecessor_label);
}

[[nodiscard]] inline std::optional<PreparedAuthoritativeBranchJoinTransfer>
find_authoritative_branch_owned_join_transfer(
    const PreparedControlFlowFunction& function_cf,
    std::string_view source_branch_block_label,
    std::optional<PreparedJoinTransferKind> required_kind = std::nullopt,
    std::optional<std::string_view> true_predecessor_label = std::nullopt,
    std::optional<std::string_view> false_predecessor_label = std::nullopt) {
  const auto* join_transfer = find_branch_owned_join_transfer(function_cf,
                                                              source_branch_block_label,
                                                              required_kind,
                                                              true_predecessor_label,
                                                              false_predecessor_label);
  if (join_transfer == nullptr ||
      (join_transfer->kind != PreparedJoinTransferKind::SelectMaterialization &&
       join_transfer->kind != PreparedJoinTransferKind::EdgeStoreSlot) ||
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

  if (join_transfer->kind == PreparedJoinTransferKind::EdgeStoreSlot) {
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

[[nodiscard]] inline std::optional<PreparedAuthoritativeJoinBranchSources>
find_authoritative_join_branch_sources(
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    std::string_view true_block_label,
    std::string_view false_block_label) {
  const auto* join_transfer = authoritative_join_transfer.join_transfer;
  const auto* true_transfer = authoritative_join_transfer.true_transfer;
  const auto* false_transfer = authoritative_join_transfer.false_transfer;
  if (join_transfer == nullptr || true_transfer == nullptr || false_transfer == nullptr) {
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

  const auto* true_predecessor = find_block_in_function(function, true_block_label);
  const auto* false_predecessor = find_block_in_function(function, false_block_label);
  if (true_predecessor == nullptr || false_predecessor == nullptr ||
      true_predecessor == &entry_block || false_predecessor == &entry_block ||
      true_predecessor == false_predecessor || !true_predecessor->insts.empty() ||
      !false_predecessor->insts.empty()) {
    return std::nullopt;
  }
  if (true_predecessor->terminator.kind != bir::TerminatorKind::Branch ||
      false_predecessor->terminator.kind != bir::TerminatorKind::Branch ||
      true_predecessor->terminator.target_label != join_transfer->join_block_label ||
      false_predecessor->terminator.target_label != join_transfer->join_block_label) {
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
    const PreparedJoinTransfer& join_transfer,
    const bir::Block& join_block,
    std::size_t carrier_index) {
  if (carrier_index >= join_block.insts.size()) {
    return std::nullopt;
  }

  const auto& join_carrier = join_block.insts[carrier_index];
  if (const auto* select = std::get_if<bir::SelectInst>(&join_carrier); select != nullptr) {
    if (join_transfer.kind != PreparedJoinTransferKind::SelectMaterialization ||
        select->result.kind != bir::Value::Kind::Named ||
        select->result.type != bir::TypeKind::I32) {
      return std::nullopt;
    }
    return PreparedJoinCarrier{
        .carrier_index = carrier_index,
        .result_name = select->result.name,
    };
  }

  if (const auto* load_local = std::get_if<bir::LoadLocalInst>(&join_carrier);
      load_local != nullptr) {
    if (join_transfer.kind != PreparedJoinTransferKind::EdgeStoreSlot ||
        !join_transfer.storage_name.has_value() ||
        load_local->slot_name != *join_transfer.storage_name ||
        load_local->result.kind != bir::Value::Kind::Named ||
        load_local->result.type != bir::TypeKind::I32) {
      return std::nullopt;
    }
    return PreparedJoinCarrier{
        .carrier_index = carrier_index,
        .result_name = load_local->result.name,
    };
  }

  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedSupportedImmediateBinary>
classify_supported_immediate_binary(const bir::BinaryInst& binary, std::string_view source_name) {
  if (binary.operand_type != bir::TypeKind::I32 || binary.result.type != bir::TypeKind::I32) {
    return std::nullopt;
  }

  const bool lhs_is_source_rhs_is_imm =
      binary.lhs.kind == bir::Value::Kind::Named && binary.lhs.name == source_name &&
      binary.rhs.kind == bir::Value::Kind::Immediate && binary.rhs.type == bir::TypeKind::I32;
  const bool rhs_is_source_lhs_is_imm =
      binary.rhs.kind == bir::Value::Kind::Named && binary.rhs.name == source_name &&
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
    const bir::Value& value,
    const bir::Function& function,
    const std::unordered_map<std::string_view, const bir::BinaryInst*>& named_binaries,
    const std::unordered_map<std::string_view, const bir::LoadGlobalInst*>& named_global_loads,
    std::vector<std::string_view>* active_names = nullptr) {
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

  std::vector<std::string_view> local_active_names;
  auto* recursion_stack = active_names == nullptr ? &local_active_names : active_names;
  for (const auto active_name : *recursion_stack) {
    if (active_name == value.name) {
      return std::nullopt;
    }
  }

  const auto binary_it = named_binaries.find(value.name);
  if (binary_it == named_binaries.end()) {
    for (const auto& param : function.params) {
      if (param.type == bir::TypeKind::I32 && param.name == value.name) {
        return PreparedComputedValue{
            .base = PreparedComputedBase{
                .kind = PreparedComputedBaseKind::ParamValue,
                .param_name = param.name,
            },
        };
      }
    }
    const auto load_global_it = named_global_loads.find(value.name);
    if (load_global_it != named_global_loads.end() && load_global_it->second != nullptr) {
      const auto* load_global = load_global_it->second;
      if (load_global->result.type != bir::TypeKind::I32 ||
          load_global->result.kind != bir::Value::Kind::Named ||
          load_global->result.name != value.name) {
        return std::nullopt;
      }
      if (load_global->address.has_value()) {
        if (load_global->address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol ||
            load_global->address->base_name.empty() || load_global->address->byte_offset != 0) {
          return std::nullopt;
        }
        return PreparedComputedValue{
            .base = PreparedComputedBase{
                .kind = PreparedComputedBaseKind::PointerBackedGlobalI32Load,
                .global_name = load_global->global_name,
                .global_byte_offset = load_global->byte_offset,
                .pointer_root_global_name = load_global->address->base_name,
            },
        };
      }
      return PreparedComputedValue{
          .base = PreparedComputedBase{
              .kind = PreparedComputedBaseKind::GlobalI32Load,
              .global_name = load_global->global_name,
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

  recursion_stack->push_back(value.name);
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
        classify_computed_value(
            source_value, function, named_binaries, named_global_loads, recursion_stack);
    if (!computed_value.has_value()) {
      return std::nullopt;
    }
    computed_value->operations.push_back(*operation);
    return computed_value;
  };

  if (const auto computed_value = try_extend(
          binary->lhs, classify_supported_immediate_binary(*binary, binary->lhs.name));
      computed_value.has_value()) {
    pop_active_name();
    return computed_value;
  }
  if (const auto computed_value = try_extend(
          binary->rhs, classify_supported_immediate_binary(*binary, binary->rhs.name));
      computed_value.has_value()) {
    pop_active_name();
    return computed_value;
  }

  pop_active_name();
  return std::nullopt;
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinContext>
find_materialized_compare_join_context(
    const PreparedAuthoritativeBranchJoinTransfer& authoritative_join_transfer,
    const bir::Function& function,
    const bir::Block& entry_block,
    std::string_view true_block_label,
    std::string_view false_block_label) {
  const auto join_sources = find_authoritative_join_branch_sources(authoritative_join_transfer,
                                                                   function,
                                                                   entry_block,
                                                                   true_block_label,
                                                                   false_block_label);
  if (!join_sources.has_value() || join_sources->join_transfer == nullptr ||
      join_sources->true_transfer == nullptr || join_sources->false_transfer == nullptr ||
      join_sources->true_predecessor == nullptr || join_sources->false_predecessor == nullptr ||
      join_sources->true_predecessor == join_sources->false_predecessor) {
    return std::nullopt;
  }

  const auto* join_transfer = join_sources->join_transfer;
  const auto* join_block = find_block_in_function(function, join_transfer->join_block_label);
  if (join_block == nullptr || join_block == &entry_block ||
      join_block->terminator.kind != bir::TerminatorKind::Return ||
      !join_block->terminator.value.has_value() || join_block->insts.empty() ||
      join_sources->true_predecessor == join_block ||
      join_sources->false_predecessor == join_block ||
      join_transfer->join_block_label != join_block->label) {
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
      find_supported_join_carrier(*join_transfer, *join_block, carrier_index);
  if (!prepared_carrier.has_value()) {
    return std::nullopt;
  }
  if (join_block->terminator.value->kind != bir::Value::Kind::Named ||
      (trailing_binary == nullptr &&
       join_block->terminator.value->name != prepared_carrier->result_name)) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinContext{
      .join_transfer = join_transfer,
      .true_transfer = join_sources->true_transfer,
      .false_transfer = join_sources->false_transfer,
      .function = &function,
      .true_predecessor = join_sources->true_predecessor,
      .false_predecessor = join_sources->false_predecessor,
      .join_block = join_block,
      .trailing_binary = trailing_binary,
      .carrier_index = prepared_carrier->carrier_index,
      .carrier_result_name = prepared_carrier->result_name,
  };
}

[[nodiscard]] inline std::optional<PreparedCompareJoinContinuationTargets>
find_prepared_compare_join_continuation_targets(
    const PreparedJoinTransfer& join_transfer,
    const bir::Block& join_block,
    const PreparedBranchCondition& join_branch_condition) {
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
      find_supported_join_carrier(join_transfer, join_block, join_block.insts.size() - 2);
  if (!prepared_carrier.has_value()) {
    return std::nullopt;
  }

  const bool lhs_is_join_result_rhs_is_zero =
      join_branch_condition.lhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.lhs->name == prepared_carrier->result_name &&
      join_branch_condition.rhs->kind == bir::Value::Kind::Immediate &&
      join_branch_condition.rhs->type == bir::TypeKind::I32 &&
      join_branch_condition.rhs->immediate == 0;
  const bool rhs_is_join_result_lhs_is_zero =
      join_branch_condition.rhs->kind == bir::Value::Kind::Named &&
      join_branch_condition.rhs->name == prepared_carrier->result_name &&
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

[[nodiscard]] inline std::optional<PreparedShortCircuitJoinContext>
find_prepared_short_circuit_join_context(const PreparedControlFlowFunction& function_cf,
                                         const bir::Function& function,
                                         std::string_view source_block_label) {
  const auto authoritative_join_transfer =
      find_authoritative_branch_owned_join_transfer(function_cf, source_block_label);
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
      find_block_in_function(function, join_sources->join_transfer->join_block_label);
  if (join_block == nullptr) {
    return std::nullopt;
  }

  const auto* join_branch_condition =
      find_prepared_branch_condition(function_cf, join_block->label);
  if (join_branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto continuation_targets = find_prepared_compare_join_continuation_targets(
      *join_sources->join_transfer, *join_block, *join_branch_condition);
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

[[nodiscard]] inline std::optional<PreparedShortCircuitBranchPlan>
find_prepared_short_circuit_branch_plan(
    const PreparedShortCircuitJoinContext& join_context,
    const PreparedBranchTargetLabels& direct_branch_targets) {
  if (join_context.true_transfer == nullptr || join_context.false_transfer == nullptr ||
      direct_branch_targets.true_label.empty() || direct_branch_targets.false_label.empty()) {
    return std::nullopt;
  }

  const bool short_circuit_on_compare_true =
      join_context.classified_incoming.short_circuit_on_compare_true;
  const bool short_circuit_on_compare_false = !short_circuit_on_compare_true;

  const auto rhs_entry_label = short_circuit_on_compare_true
                                   ? direct_branch_targets.false_label
                                   : direct_branch_targets.true_label;
  if (rhs_entry_label.empty()) {
    return std::nullopt;
  }

  const auto on_compare_true = build_prepared_short_circuit_target_labels(
      *join_context.true_transfer,
      short_circuit_on_compare_true,
      join_context.classified_incoming.short_circuit_value,
      rhs_entry_label,
      join_context.continuation_true_label,
      join_context.continuation_false_label);
  const auto on_compare_false = build_prepared_short_circuit_target_labels(
      *join_context.false_transfer,
      short_circuit_on_compare_false,
      join_context.classified_incoming.short_circuit_value,
      rhs_entry_label,
      join_context.continuation_true_label,
      join_context.continuation_false_label);
  if (!on_compare_true.has_value() || !on_compare_false.has_value() ||
      on_compare_true->block_label.empty() || on_compare_false->block_label.empty()) {
    return std::nullopt;
  }

  return PreparedShortCircuitBranchPlan{
      .on_compare_true = *on_compare_true,
      .on_compare_false = *on_compare_false,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinBranches>
find_prepared_materialized_compare_join_branches(
    const PreparedMaterializedCompareJoinContext& compare_join_context) {
  if (compare_join_context.join_transfer == nullptr || compare_join_context.true_transfer == nullptr ||
      compare_join_context.false_transfer == nullptr ||
      compare_join_context.true_predecessor == nullptr ||
      compare_join_context.false_predecessor == nullptr ||
      compare_join_context.true_predecessor == compare_join_context.false_predecessor) {
    return std::nullopt;
  }

  const auto true_return_context = find_prepared_materialized_compare_join_return_context(
      compare_join_context, compare_join_context.true_transfer->incoming_value);
  const auto false_return_context = find_prepared_materialized_compare_join_return_context(
      compare_join_context, compare_join_context.false_transfer->incoming_value);
  if (!true_return_context.has_value() || !false_return_context.has_value()) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinBranches{
      .compare_join_context = compare_join_context,
      .true_return_context = std::move(*true_return_context),
      .false_return_context = std::move(*false_return_context),
      .false_predecessor_label = compare_join_context.false_predecessor->label,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinReturnContext>
find_prepared_materialized_compare_join_return_context(
    const PreparedMaterializedCompareJoinContext& compare_join_context,
    const bir::Value& selected_value) {
  const auto* join_block = compare_join_context.join_block;
  if (join_block == nullptr || compare_join_context.function == nullptr ||
      compare_join_context.carrier_index > join_block->insts.size()) {
    return std::nullopt;
  }

  std::unordered_map<std::string_view, const bir::BinaryInst*> named_binaries;
  std::unordered_map<std::string_view, const bir::LoadGlobalInst*> named_global_loads;
  for (std::size_t inst_index = 0; inst_index < compare_join_context.carrier_index; ++inst_index) {
    if (const auto* binary = std::get_if<bir::BinaryInst>(&join_block->insts[inst_index]);
        binary != nullptr) {
      named_binaries.emplace(binary->result.name, binary);
      continue;
    }
    if (const auto* load_global =
            std::get_if<bir::LoadGlobalInst>(&join_block->insts[inst_index]);
        load_global != nullptr) {
      named_global_loads.emplace(load_global->result.name, load_global);
      continue;
    }
    return std::nullopt;
  }

  const auto computed_selected_value = classify_computed_value(
      selected_value, *compare_join_context.function, named_binaries, named_global_loads);
  if (!computed_selected_value.has_value()) {
    return std::nullopt;
  }

  std::optional<PreparedSupportedImmediateBinary> trailing_binary;
  if (compare_join_context.trailing_binary != nullptr) {
    trailing_binary = classify_supported_immediate_binary(*compare_join_context.trailing_binary,
                                                          compare_join_context.carrier_result_name);
    if (!trailing_binary.has_value()) {
      return std::nullopt;
    }
  }

  return PreparedMaterializedCompareJoinReturnContext{
      .selected_value = std::move(*computed_selected_value),
      .trailing_binary = std::move(trailing_binary),
  };
}

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinContext>
find_prepared_param_zero_materialized_compare_join_context(
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  const auto prepared_branch = find_prepared_param_zero_branch_condition(function_cf,
                                                                         source_block,
                                                                         param,
                                                                         require_label_match);
  if (!prepared_branch.has_value() || prepared_branch->branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer = find_authoritative_branch_owned_join_transfer(
      function_cf, source_block.label);
  if (!authoritative_join_transfer.has_value()) {
    return std::nullopt;
  }

  const auto compare_join_context = find_materialized_compare_join_context(
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

[[nodiscard]] inline std::optional<PreparedParamZeroMaterializedCompareJoinBranches>
find_prepared_param_zero_materialized_compare_join_branches(
    const PreparedControlFlowFunction& function_cf,
    const bir::Function& function,
    const bir::Block& source_block,
    const bir::Param& param,
    bool require_label_match) {
  const auto prepared_compare_join_context =
      find_prepared_param_zero_materialized_compare_join_context(function_cf,
                                                                 function,
                                                                 source_block,
                                                                 param,
                                                                 require_label_match);
  if (!prepared_compare_join_context.has_value()) {
    return std::nullopt;
  }

  const auto prepared_join_branches = find_prepared_materialized_compare_join_branches(
      prepared_compare_join_context->compare_join_context);
  if (!prepared_join_branches.has_value()) {
    return std::nullopt;
  }

  return PreparedParamZeroMaterializedCompareJoinBranches{
      .prepared_branch = prepared_compare_join_context->prepared_branch,
      .prepared_join_branches = std::move(*prepared_join_branches),
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinBranchPlan>
find_prepared_materialized_compare_join_branch_plan(
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches) {
  const auto& compare_join_context =
      prepared_compare_join_branches.prepared_join_branches.compare_join_context;
  if (prepared_compare_join_branches.prepared_branch.branch_condition == nullptr ||
      prepared_compare_join_branches.prepared_branch.false_branch_opcode == nullptr ||
      compare_join_context.true_predecessor == nullptr ||
      compare_join_context.false_predecessor == nullptr ||
      compare_join_context.true_predecessor == compare_join_context.false_predecessor) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinBranchPlan{
      .target_labels =
          PreparedBranchTargetLabels{
              .true_label = compare_join_context.true_predecessor->label,
              .false_label = compare_join_context.false_predecessor->label,
          },
      .false_branch_opcode =
      prepared_compare_join_branches.prepared_branch.false_branch_opcode,
  };
}

[[nodiscard]] inline std::optional<PreparedMaterializedCompareJoinRenderContract>
find_prepared_materialized_compare_join_render_contract(
    const PreparedParamZeroMaterializedCompareJoinBranches& prepared_compare_join_branches) {
  const auto branch_plan =
      find_prepared_materialized_compare_join_branch_plan(prepared_compare_join_branches);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }

  return PreparedMaterializedCompareJoinRenderContract{
      .branch_plan = std::move(*branch_plan),
      .true_return_context =
          prepared_compare_join_branches.prepared_join_branches.true_return_context,
      .true_return_shape = classify_prepared_materialized_compare_join_return_shape(
          prepared_compare_join_branches.prepared_join_branches.true_return_context),
      .false_return_context =
          prepared_compare_join_branches.prepared_join_branches.false_return_context,
      .false_return_shape = classify_prepared_materialized_compare_join_return_shape(
          prepared_compare_join_branches.prepared_join_branches.false_return_context),
      .same_module_global_names =
          collect_prepared_materialized_compare_join_same_module_globals(
              prepared_compare_join_branches.prepared_join_branches),
  };
}

[[nodiscard]] inline std::vector<std::string_view>
collect_prepared_materialized_compare_join_same_module_globals(
    const PreparedMaterializedCompareJoinBranches& prepared_join_branches) {
  std::vector<std::string_view> names;
  std::unordered_set<std::string_view> seen_names;
  const auto append_names =
      [&](const PreparedMaterializedCompareJoinReturnContext& return_context) {
        for (const auto name :
             collect_prepared_computed_value_same_module_globals(return_context.selected_value)) {
          if (seen_names.insert(name).second) {
            names.push_back(name);
          }
        }
      };
  append_names(prepared_join_branches.true_return_context);
  append_names(prepared_join_branches.false_return_context);
  return names;
}

[[nodiscard]] inline std::optional<std::vector<const bir::Global*>>
resolve_prepared_materialized_compare_join_same_module_globals(
    const bir::Module& module,
    const PreparedMaterializedCompareJoinRenderContract& render_contract) {
  std::vector<const bir::Global*> globals;
  globals.reserve(render_contract.same_module_global_names.size());
  for (const auto global_name : render_contract.same_module_global_names) {
    const bir::Global* resolved_global = nullptr;
    for (const auto& global : module.globals) {
      if (global.name != global_name) {
        continue;
      }
      if (global.is_extern || global.is_thread_local) {
        return std::nullopt;
      }
      resolved_global = &global;
      break;
    }
    if (resolved_global == nullptr) {
      return std::nullopt;
    }
    globals.push_back(resolved_global);
  }
  return globals;
}

[[nodiscard]] inline const std::vector<PreparedEdgeValueTransfer>* incoming_transfers_for_join(
    const PreparedControlFlowFunction& function_cf,
    std::string_view join_block_label,
    std::string_view destination_value_name) {
  const auto* transfer =
      find_prepared_join_transfer(function_cf, join_block_label, destination_value_name);
  if (transfer == nullptr) {
    return nullptr;
  }
  return &transfer->edge_transfers;
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

struct PreparedBirModule {
  c4c::backend::bir::Module module;
  c4c::TargetProfile target_profile{};
  PrepareRoute route = PrepareRoute::SemanticBirShared;
  std::vector<PreparedBirInvariant> invariants;
  PreparedControlFlow control_flow;
  PreparedStackLayout stack_layout;
  PreparedLiveness liveness;
  PreparedRegalloc regalloc;
  std::vector<std::string> completed_phases;
  std::vector<PrepareNote> notes;
};

class BirPreAlloc {
 public:
  BirPreAlloc(const c4c::backend::bir::Module& module,
              const c4c::TargetProfile& target_profile,
              const PrepareOptions& options = {})
      : options_(options),
        prepared_{
            .module = module,
            .target_profile = target_profile,
            .route = PrepareRoute::SemanticBirShared,
            .completed_phases = {},
            .notes = {},
        } {}
  explicit BirPreAlloc(PreparedBirModule prepared,
                       const PrepareOptions& options = {})
      : options_(options), prepared_(std::move(prepared)) {
    if (prepared_.target_profile.arch == c4c::TargetArch::Unknown) {
      const auto resolved_triple =
          prepared_.module.target_triple.empty() ? c4c::default_host_target_triple()
                                                 : prepared_.module.target_triple;
      prepared_.target_profile = c4c::target_profile_from_triple(resolved_triple);
    }
  }

  void run_legalize();
  void run_stack_layout();
  void run_liveness();
  void run_regalloc();

  PreparedBirModule& prepared() { return prepared_; }
  const PreparedBirModule& prepared() const { return prepared_; }
  PreparedBirModule run();

 private:
  void note(std::string_view message);

  PrepareOptions options_;
  PreparedBirModule prepared_;
};

PreparedBirModule prepare_semantic_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options = {});

PreparedBirModule prepare_bir_module_with_options(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile,
    const PrepareOptions& options = {});

}  // namespace c4c::backend::prepare

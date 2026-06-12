#include "comparison.hpp"
#include "dispatch_lookup.hpp"
#include "dispatch_publication.hpp"
#include "instruction.hpp"
#include "../../query.hpp"
#include "alu.hpp"
#include "operands.hpp"
#include "select_materialization.hpp"
#include "../../../prealloc/addressing.hpp"
#include "../../../prealloc/comparison.hpp"
#include "../../../prealloc/control_flow.hpp"
#include "../../../prealloc/publication_plans.hpp"
#include "../../../prealloc/select_chain_lookups.hpp"
#include "../../../prealloc/stack_layout/stack_layout.hpp"
#include "../../../prealloc/value_locations.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <string>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::codegen {
namespace {

namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

[[nodiscard]] std::string machine_block_label(c4c::FunctionNameId function_name,
                                              c4c::BlockLabelId block_label);

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

struct PreparedConditionalBranchFacts {
  const prepare::PreparedBranchCondition* branch_condition = nullptr;
  prepare::PreparedBranchTargetLabels targets;
  std::optional<prepare::PreparedShortCircuitBranchPlan> short_circuit_plan;
};

struct PreparedFusedCompareBranchFacts {
  const prepare::PreparedBranchCondition* branch_condition = nullptr;
  prepare::PreparedBranchTargetLabels targets;
  std::optional<prepare::PreparedShortCircuitBranchPlan> short_circuit_plan;
  bir::BinaryOpcode predicate = bir::BinaryOpcode::Eq;
  bir::TypeKind compare_type = bir::TypeKind::Void;
  bir::Value lhs;
  bir::Value rhs;
  std::string_view condition_suffix;
  abi::RegisterView operand_view = abi::RegisterView::W;
};

[[nodiscard]] std::optional<prepare::PreparedBranchTargetLabels>
find_prepared_materialized_compare_join_targets(
    const module::BlockLoweringContext& context,
    const prepare::PreparedBranchTargetLabels& direct_targets) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.function.bir_function == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      direct_targets.true_label == c4c::kInvalidBlockLabel ||
      direct_targets.false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }

  const auto authoritative_join_transfer =
      prepare::find_authoritative_branch_owned_join_transfer(
          context.function.prepared->names,
          *context.function.control_flow,
          context.control_flow_block->block_label);
  if (!authoritative_join_transfer.has_value()) {
    return std::nullopt;
  }

  bir::NameTables bir_names;
  bir_names.import_link_names(context.function.prepared->names.texts,
                              context.function.prepared->names.link_names);
  const auto compare_join_context = prepare::find_materialized_compare_join_context(
      context.function.prepared->names,
      bir_names,
      *authoritative_join_transfer,
      *context.function.bir_function,
      *context.bir_block,
      direct_targets.true_label,
      direct_targets.false_label);
  if (!compare_join_context.has_value() ||
      compare_join_context->join_transfer == nullptr) {
    return std::nullopt;
  }

  const auto continuation_targets =
      prepare::published_prepared_compare_join_continuation_targets(
          *compare_join_context->join_transfer);
  if (!continuation_targets.has_value()) {
    return std::nullopt;
  }

  return prepare::PreparedBranchTargetLabels{
      .true_label = continuation_targets->true_label,
      .false_label = continuation_targets->false_label,
  };
}

[[nodiscard]] std::optional<PreparedConditionalBranchFacts>
find_prepared_conditional_branch_facts(
    const module::BlockLoweringContext& context) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }

  const auto* branch_condition = prepare::find_prepared_branch_condition(
      *context.function.control_flow, context.control_flow_block->block_label);
  if (branch_condition == nullptr ||
      branch_condition->true_label == c4c::kInvalidBlockLabel ||
      branch_condition->false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }

  PreparedConditionalBranchFacts facts{
      .branch_condition = branch_condition,
      .targets =
          prepare::PreparedBranchTargetLabels{
              .true_label = branch_condition->true_label,
              .false_label = branch_condition->false_label,
          },
  };

  if (context.function.prepared == nullptr || context.function.bir_function == nullptr ||
      context.bir_block == nullptr) {
    return facts;
  }

  const bool has_compare_branch_facts =
      branch_condition->predicate.has_value() &&
      branch_condition->compare_type.has_value() &&
      branch_condition->lhs.has_value() &&
      branch_condition->rhs.has_value();
  if (!has_compare_branch_facts) {
    if (const auto prepared_control_flow_targets =
            prepare::find_prepared_control_flow_branch_target_labels(
                *context.function.control_flow,
                context.control_flow_block->block_label);
        prepared_control_flow_targets.has_value()) {
      facts.targets = *prepared_control_flow_targets;
      if (const auto bir_control_flow_targets =
              prepare::detail::read_agreeing_bir_branch_target_labels(
                  prepare::detail::BranchTargetIdentityPassContext{
                      .source_block = context.bir_block,
                  },
                  *prepared_control_flow_targets);
          bir_control_flow_targets.has_value()) {
        facts.targets = *bir_control_flow_targets;
      }
    }
    return facts;
  }

  const auto direct_targets = prepare::resolve_prepared_compare_branch_target_labels(
      context.function.prepared->names,
      context.function.control_flow,
      *context.bir_block,
      *branch_condition);
  if (!direct_targets.has_value()) {
    return std::nullopt;
  }
  facts.targets = *direct_targets;
  if (const auto compare_join_targets =
          find_prepared_materialized_compare_join_targets(context, *direct_targets);
      compare_join_targets.has_value()) {
    facts.targets = *compare_join_targets;
  }

  const auto join_context = prepare::find_prepared_short_circuit_join_context(
      context.function.prepared->names,
      *context.function.control_flow,
      *context.function.bir_function,
      context.control_flow_block->block_label);
  if (!join_context.has_value()) {
    return facts;
  }

  const auto branch_plan = prepare::find_prepared_short_circuit_branch_plan(
      context.function.prepared->names, *join_context, *direct_targets);
  if (!branch_plan.has_value()) {
    return std::nullopt;
  }
  facts.short_circuit_plan = *branch_plan;
  facts.targets = prepare::PreparedBranchTargetLabels{
      .true_label = branch_plan->on_compare_true.block_label,
      .false_label = branch_plan->on_compare_false.block_label,
  };
  return facts;
}

[[nodiscard]] std::optional<PreparedFusedCompareBranchFacts>
find_prepared_fused_compare_branch_facts(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks) {
  const auto branch_facts = find_prepared_conditional_branch_facts(context);
  if (!branch_facts.has_value() || branch_facts->branch_condition == nullptr) {
    return std::nullopt;
  }

  const auto& branch_condition = *branch_facts->branch_condition;
  if (branch_condition.kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.can_fuse_with_branch ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  const auto condition = branch_condition_suffix(*branch_condition.predicate);
  const auto operand_view = hooks.scalar_view_for_type(*branch_condition.compare_type);
  if (!condition.has_value() || !operand_view.has_value() ||
      branch_facts->targets.true_label == c4c::kInvalidBlockLabel ||
      branch_facts->targets.false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }

  return PreparedFusedCompareBranchFacts{
      .branch_condition = branch_facts->branch_condition,
      .targets = branch_facts->targets,
      .short_circuit_plan = branch_facts->short_circuit_plan,
      .predicate = *branch_condition.predicate,
      .compare_type = *branch_condition.compare_type,
      .lhs = *branch_condition.lhs,
      .rhs = *branch_condition.rhs,
      .condition_suffix = *condition,
      .operand_view = *operand_view,
  };
}

[[nodiscard]] std::optional<prepare::PreparedEdgePublicationSourceProducerKind>
prepared_producer_kind_for_route7(bir::ComparisonProducerKind kind) {
  switch (kind) {
    case bir::ComparisonProducerKind::Immediate:
      return prepare::PreparedEdgePublicationSourceProducerKind::Immediate;
    case bir::ComparisonProducerKind::LoadLocal:
      return prepare::PreparedEdgePublicationSourceProducerKind::LoadLocal;
    case bir::ComparisonProducerKind::LoadGlobal:
      return prepare::PreparedEdgePublicationSourceProducerKind::LoadGlobal;
    case bir::ComparisonProducerKind::Cast:
      return prepare::PreparedEdgePublicationSourceProducerKind::Cast;
    case bir::ComparisonProducerKind::Binary:
      return prepare::PreparedEdgePublicationSourceProducerKind::Binary;
    case bir::ComparisonProducerKind::Select:
      return prepare::PreparedEdgePublicationSourceProducerKind::SelectMaterialization;
    case bir::ComparisonProducerKind::Unknown:
      return std::nullopt;
  }
  return std::nullopt;
}

[[nodiscard]] std::optional<prepare::PreparedFusedCompareOperandProducer>
route7_comparison_operand_producer_to_prepared(
    const module::BlockLoweringContext& context,
    const bir::ComparisonOperandProducer& route7) {
  if (!route7.available) {
    return std::nullopt;
  }
  const auto prepared_kind =
      prepared_producer_kind_for_route7(route7.producer_kind);
  if (!prepared_kind.has_value()) {
    return std::nullopt;
  }
  prepare::PreparedFusedCompareOperandProducer prepared{
      .kind = *prepared_kind,
      .instruction = route7.producer_instruction,
      .instruction_index = route7.producer_instruction_index,
      .integer_constant = route7.integer_constant,
  };
  if (route7.producer_kind == bir::ComparisonProducerKind::Immediate) {
    return prepared;
  }
  if (context.function.prepared == nullptr ||
      route7.producer_instruction == nullptr ||
      route7.produced_value == nullptr ||
      route7.produced_value->kind != bir::Value::Kind::Named ||
      route7.produced_value->name.empty()) {
    return std::nullopt;
  }
  const auto value_name = prepare::resolve_prepared_value_name_id(
      context.function.prepared->names, route7.produced_value->name);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  prepared.value_name = *value_name;
  switch (route7.producer_kind) {
    case bir::ComparisonProducerKind::LoadLocal:
      prepared.load_local =
          std::get_if<bir::LoadLocalInst>(route7.producer_instruction);
      break;
    case bir::ComparisonProducerKind::LoadGlobal:
      prepared.load_global =
          std::get_if<bir::LoadGlobalInst>(route7.producer_instruction);
      break;
    case bir::ComparisonProducerKind::Cast:
      prepared.cast = std::get_if<bir::CastInst>(route7.producer_instruction);
      break;
    case bir::ComparisonProducerKind::Binary:
      prepared.binary =
          std::get_if<bir::BinaryInst>(route7.producer_instruction);
      break;
    case bir::ComparisonProducerKind::Select:
      prepared.select =
          std::get_if<bir::SelectInst>(route7.producer_instruction);
      break;
    case bir::ComparisonProducerKind::Immediate:
    case bir::ComparisonProducerKind::Unknown:
      return std::nullopt;
  }
  if ((route7.producer_kind == bir::ComparisonProducerKind::LoadLocal &&
       prepared.load_local == nullptr) ||
      (route7.producer_kind == bir::ComparisonProducerKind::LoadGlobal &&
       prepared.load_global == nullptr) ||
      (route7.producer_kind == bir::ComparisonProducerKind::Cast &&
       prepared.cast == nullptr) ||
      (route7.producer_kind == bir::ComparisonProducerKind::Binary &&
       prepared.binary == nullptr) ||
      (route7.producer_kind == bir::ComparisonProducerKind::Select &&
       prepared.select == nullptr)) {
    return std::nullopt;
  }
  return prepared;
}

[[nodiscard]] bool prepared_fused_compare_operand_producer_matches(
    const std::optional<prepare::PreparedFusedCompareOperandProducer>& actual,
    const std::optional<prepare::PreparedFusedCompareOperandProducer>& expected) {
  if (actual.has_value() != expected.has_value()) {
    return false;
  }
  if (!actual.has_value()) {
    return true;
  }
  return actual->kind == expected->kind &&
         actual->instruction == expected->instruction &&
         actual->instruction_index == expected->instruction_index &&
         actual->value_name == expected->value_name &&
         actual->cast == expected->cast &&
         actual->load_local == expected->load_local &&
         actual->load_global == expected->load_global &&
         actual->binary == expected->binary &&
         actual->select == expected->select &&
         actual->integer_constant == expected->integer_constant;
}

[[nodiscard]] std::optional<prepare::PreparedFusedCompareOperandProducerFacts>
route7_fused_compare_operand_producer_facts_if_agree_with_prepared(
    const module::BlockLoweringContext& context,
    const prepare::PreparedFusedCompareOperandProducerFacts& prepared,
    const bir::FusedCompareOperandProducerFacts& route7) {
  if (!route7.available) {
    return std::nullopt;
  }
  prepare::PreparedFusedCompareOperandProducerFacts converted;
  if (route7.lhs.has_value()) {
    converted.lhs =
        route7_comparison_operand_producer_to_prepared(context, *route7.lhs);
    if (!converted.lhs.has_value()) {
      return std::nullopt;
    }
  }
  if (route7.rhs.has_value()) {
    converted.rhs =
        route7_comparison_operand_producer_to_prepared(context, *route7.rhs);
    if (!converted.rhs.has_value()) {
      return std::nullopt;
    }
  }
  if (!prepared_fused_compare_operand_producer_matches(converted.lhs,
                                                       prepared.lhs) ||
      !prepared_fused_compare_operand_producer_matches(converted.rhs,
                                                       prepared.rhs)) {
    return std::nullopt;
  }
  return converted;
}

[[nodiscard]] std::optional<prepare::PreparedFusedCompareOperandProducerFacts>
find_prepared_fused_compare_operand_producer_facts(
    const module::BlockLoweringContext& context,
    const prepare::PreparedBranchCondition& branch_condition) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      branch_condition.kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  const auto before_instruction_index = context.bir_block->insts.size();
  std::optional<prepare::PreparedFusedCompareOperandProducerFacts> prepared;
  if (context.function.prepared_lookups != nullptr) {
    prepared = prepare::find_prepared_fused_compare_operand_producer_facts(
        context.function.prepared->names,
        &context.function.prepared_lookups->edge_publication_source_producers,
        context.control_flow_block->block_label,
        context.bir_block,
        branch_condition,
        before_instruction_index);
  } else {
    const auto source_producers =
        prepare::make_prepared_edge_publication_source_producer_lookups(
            *context.function.prepared,
            *context.function.control_flow);
    prepared = prepare::find_prepared_fused_compare_operand_producer_facts(
        context.function.prepared->names,
        &source_producers,
        context.control_flow_block->block_label,
        context.bir_block,
        branch_condition,
        before_instruction_index);
  }
  if (!prepared.has_value()) {
    return prepared;
  }

  const auto route7_index =
      bir::route7_build_comparison_condition_index(*context.bir_block);
  const auto route7 =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          *context.bir_block,
          *branch_condition.lhs,
          *branch_condition.rhs,
          before_instruction_index);
  if (const auto agreed =
          route7_fused_compare_operand_producer_facts_if_agree_with_prepared(
              context, *prepared, route7);
      agreed.has_value()) {
    return agreed;
  }
  return prepared;
}

[[nodiscard]] bir::MaterializedConditionProducerIdentity
find_bir_materialized_condition_producer_identity(
    const bir::Block& block,
    const bir::Value& condition_value,
    std::size_t before_instruction_index) {
  return bir::find_materialized_condition_producer_identity(
      block, condition_value, before_instruction_index);
}

[[nodiscard]] std::optional<prepare::PreparedMaterializedConditionProducer>
find_prepared_materialized_condition_producer_identity(
    const module::BlockLoweringContext& context,
    const bir::Value& condition_value,
    std::size_t before_instruction_index) {
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr) {
    return std::nullopt;
  }
  if (context.function.prepared_lookups != nullptr) {
    return prepare::find_prepared_materialized_condition_producer(
        context.function.prepared->names,
        &context.function.prepared_lookups->edge_publication_source_producers,
        context.control_flow_block->block_label,
        context.bir_block,
        condition_value,
        before_instruction_index);
  }
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          *context.function.prepared,
          *context.function.control_flow);
  return prepare::find_prepared_materialized_condition_producer(
      context.function.prepared->names,
      &source_producers,
      context.control_flow_block->block_label,
      context.bir_block,
      condition_value,
      before_instruction_index);
}

[[nodiscard]] bool route7_source_identity_matches_value(
    const bir::Route1SourceValueIdentity& identity,
    const bir::Value& value) {
  if (identity.value_kind != value.kind ||
      identity.type != value.type ||
      identity.pointer_symbol_link_name_id != value.pointer_symbol_link_name_id) {
    return false;
  }
  switch (value.kind) {
    case bir::Value::Kind::Named:
      return identity.name == value.name;
    case bir::Value::Kind::Immediate:
      return identity.integer_constant.has_value() &&
             *identity.integer_constant == value.immediate;
  }
  return false;
}

[[nodiscard]] bool route7_operand_record_matches_reference(
    const bir::Route7ComparisonOperandRecord& record,
    const bir::Route7IndexReferenceValidation& reference,
    const bir::Block& block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    bir::Route7ComparisonOperandRole role) {
  if (!record ||
      !reference ||
      reference.operand_record == nullptr ||
      record.status != bir::Route7ComparisonStatus::Available ||
      record.role != role ||
      record.before_instruction_index != before_instruction_index ||
      !route7_source_identity_matches_value(record.value, value) ||
      !route7_source_identity_matches_value(reference.operand_record->value, value) ||
      record.producer_kind != reference.operand_record->producer_kind ||
      record.integer_constant != reference.operand_record->integer_constant ||
      record.producer_kind == bir::ComparisonProducerKind::Unknown) {
    return false;
  }
  if (value.kind == bir::Value::Kind::Immediate) {
    return record.producer_kind == bir::ComparisonProducerKind::Immediate &&
           record.integer_constant.has_value() &&
           *record.integer_constant == value.immediate;
  }
  if (record.producer_instruction == nullptr ||
      reference.operand_record->producer_instruction == nullptr ||
      record.producer_instruction_index !=
          reference.operand_record->producer_instruction_index ||
      record.producer_instruction_index >= before_instruction_index ||
      record.producer_instruction_index >= block.insts.size() ||
      record.producer_instruction !=
          &block.insts[record.producer_instruction_index] ||
      reference.operand_record->producer_instruction !=
          record.producer_instruction ||
      record.produced_value.value == nullptr ||
      reference.operand_record->produced_value.value == nullptr ||
      !route7_source_identity_matches_value(record.produced_value, value) ||
      !route7_source_identity_matches_value(
          reference.operand_record->produced_value, value)) {
    return false;
  }
  return true;
}

[[nodiscard]] std::optional<bir::MaterializedConditionProducerIdentity>
find_valid_route7_materialized_condition_producer_identity(
    const bir::Block& block,
    const bir::Value& condition_value,
    std::size_t before_instruction_index) {
  const auto index = bir::route7_build_comparison_condition_index(block);
  const auto record = bir::route7_find_materialized_condition(
      index, block, condition_value, before_instruction_index);
  if (!record) {
    return std::nullopt;
  }
  const auto reference =
      bir::route_index_validate_materialized_condition_reference(
          bir::route_index_reference_facade(index),
          block,
          condition_value,
          before_instruction_index);
  const auto* validated_record = reference.comparison_record;
  if (!record ||
      !reference ||
      validated_record == nullptr ||
      record.status != bir::Route7ComparisonStatus::Available ||
      validated_record->binary == nullptr ||
      validated_record->instruction == nullptr ||
      validated_record->instruction_index >= before_instruction_index ||
      validated_record->instruction_index >= block.insts.size() ||
      validated_record->instruction !=
          &block.insts[validated_record->instruction_index] ||
      std::get_if<bir::BinaryInst>(validated_record->instruction) !=
          validated_record->binary ||
      !route7_source_identity_matches_value(validated_record->condition_value,
                                            condition_value) ||
      validated_record->binary->result != condition_value ||
      validated_record->instruction_index != record.instruction_index ||
      validated_record->binary != record.binary ||
      validated_record->predicate != validated_record->binary->opcode ||
      validated_record->compare_type != validated_record->binary->operand_type ||
      !route7_source_identity_matches_value(validated_record->lhs_value,
                                            validated_record->binary->lhs) ||
      !route7_source_identity_matches_value(validated_record->rhs_value,
                                            validated_record->binary->rhs)) {
    return std::nullopt;
  }
  const auto lhs_reference =
      bir::route_index_validate_comparison_operand_reference(
          bir::route_index_reference_facade(index),
          block,
          validated_record->binary->lhs,
          validated_record->instruction_index,
          bir::Route7ComparisonOperandRole::Lhs);
  const auto rhs_reference =
      bir::route_index_validate_comparison_operand_reference(
          bir::route_index_reference_facade(index),
          block,
          validated_record->binary->rhs,
          validated_record->instruction_index,
          bir::Route7ComparisonOperandRole::Rhs);
  if (!route7_operand_record_matches_reference(
          validated_record->lhs,
          lhs_reference,
          block,
          validated_record->binary->lhs,
          validated_record->instruction_index,
          bir::Route7ComparisonOperandRole::Lhs) ||
      !route7_operand_record_matches_reference(
          validated_record->rhs,
          rhs_reference,
          block,
          validated_record->binary->rhs,
          validated_record->instruction_index,
          bir::Route7ComparisonOperandRole::Rhs)) {
    return std::nullopt;
  }
  return bir::MaterializedConditionProducerIdentity{
      .available = true,
      .binary = validated_record->binary,
      .instruction_index = validated_record->instruction_index,
      .condition_value_name = std::string(validated_record->condition_value.name),
      .lhs = bir::find_comparison_operand_producer(
          block, validated_record->binary->lhs, validated_record->instruction_index),
      .rhs = bir::find_comparison_operand_producer(
          block, validated_record->binary->rhs, validated_record->instruction_index),
  };
}

[[nodiscard]] bool comparison_operand_producer_matches(
    const std::optional<bir::ComparisonOperandProducer>& actual,
    const std::optional<bir::ComparisonOperandProducer>& expected) {
  if (actual.has_value() != expected.has_value()) {
    return false;
  }
  if (!actual.has_value()) {
    return true;
  }
  return actual->available == expected->available &&
         actual->producer_kind == expected->producer_kind &&
         actual->producer_instruction == expected->producer_instruction &&
         actual->producer_instruction_index ==
             expected->producer_instruction_index &&
         actual->produced_value == expected->produced_value &&
         actual->integer_constant == expected->integer_constant;
}

[[nodiscard]] bool materialized_condition_identity_matches_bir(
    const bir::MaterializedConditionProducerIdentity& actual,
    const bir::MaterializedConditionProducerIdentity& expected) {
  return actual.available == expected.available &&
         actual.binary == expected.binary &&
         actual.instruction_index == expected.instruction_index &&
         actual.condition_value_name == expected.condition_value_name &&
         comparison_operand_producer_matches(actual.lhs, expected.lhs) &&
         comparison_operand_producer_matches(actual.rhs, expected.rhs);
}

[[nodiscard]] bool materialized_condition_identity_agrees_with_prepared(
    const module::BlockLoweringContext& context,
    const prepare::PreparedMaterializedConditionProducer& prepared,
    const bir::MaterializedConditionProducerIdentity& route7) {
  if (context.function.prepared == nullptr ||
      !route7.available ||
      route7.binary == nullptr ||
      route7.binary != prepared.binary ||
      route7.instruction_index != prepared.instruction_index ||
      route7.binary->opcode != prepared.binary->opcode ||
      route7.binary->operand_type != prepared.binary->operand_type ||
      route7.binary->lhs != prepared.binary->lhs ||
      route7.binary->rhs != prepared.binary->rhs) {
    return false;
  }
  const auto route7_condition_name =
      prepare::resolve_prepared_value_name_id(
          context.function.prepared->names, route7.condition_value_name);
  return route7_condition_name.has_value() &&
         *route7_condition_name == prepared.condition_value_name;
}

void append_prepared_compare_branch_lines(
    std::vector<std::string>& lines,
    const PreparedFusedCompareBranchFacts& facts) {
  lines.push_back("b." + std::string{facts.condition_suffix} + " " +
                  machine_block_label(facts.branch_condition->function_name,
                                      facts.targets.true_label));
  lines.push_back("b " + machine_block_label(facts.branch_condition->function_name,
                                             facts.targets.false_label));
}

void apply_prepared_conditional_branch_targets(
    BranchInstructionRecord& record,
    const PreparedConditionalBranchFacts& facts) {
  if (!record.target_pair.has_value()) {
    return;
  }
  record.target_pair->true_target.block_label = facts.targets.true_label;
  record.target_pair->false_target.block_label = facts.targets.false_label;
  record.target = record.target_pair->true_target;
  if (record.condition_record.has_value() &&
      record.condition_record->compare_branch_candidate.has_value() &&
      record.condition_record->compare_branch_candidate->target_pair.has_value()) {
    record.condition_record->compare_branch_candidate->target_pair = record.target_pair;
  }
}

void append_branch_diagnostic(module::ModuleLoweringDiagnostics& diagnostics,
                              const module::BlockLoweringContext& context,
                              std::string message) {
  diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
      .kind = module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
      .instruction_family = module::InstructionLoweringFamily::BranchControl,
      .message = std::move(message),
  });
}

void append_i128_compare_diagnostic(
    module::ModuleLoweringDiagnostics& diagnostics,
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
      .instruction_family = module::InstructionLoweringFamily::Scalar,
      .message = std::move(message),
  });
}

[[nodiscard]] std::string i128_compare_error_message(
    PreparedI128PairRecordError error) {
  std::string message =
      "AArch64 i128 comparison lowering requires prepared i128 carrier facts";
  message += "; error=";
  message += prepared_i128_pair_record_error_name(error);
  return message;
}

[[nodiscard]] RegisterOperandRole register_role_from_authority(
    OperandAuthority authority) {
  switch (authority) {
    case OperandAuthority::RegallocAssignment:
      return RegisterOperandRole::AllocationResult;
    case OperandAuthority::StoragePlan:
      return RegisterOperandRole::StoragePlan;
    case OperandAuthority::PreparedValueHome:
      return RegisterOperandRole::ValueHome;
    default:
      return RegisterOperandRole::Physical;
  }
}

[[nodiscard]] std::optional<OperandRecord> make_condition_register_operand(
    const module::BlockLoweringContext& context,
    prepare::PreparedValueId condition_value_id,
    c4c::ValueNameId condition_value_name,
    bir::TypeKind condition_type,
    module::ModuleLoweringDiagnostics& diagnostics,
    const BlockScalarLoweringState* scalar_state) {
  if (scalar_state != nullptr) {
    if (const auto emitted =
            find_emitted_scalar_register(*scalar_state, condition_value_name);
        emitted.has_value()) {
      auto reg = *emitted;
      if (const auto expected_view = scalar_register_view(condition_type);
          expected_view.has_value()) {
        reg.reg.view = *expected_view;
        reg.expected_view = expected_view;
      }
      reg.value_id = condition_value_id;
      reg.value_name = condition_value_name;
      return make_register_operand(reg);
    }
  }
  auto resolved = resolve_value_operand(condition_value_id, context.function, diagnostics);
  if (!resolved.has_value() || !resolved->register_reference.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_id = condition_value_id,
        .value_name = condition_value_name,
        .message =
            "AArch64 materialized-bool branch condition requires typed register authority",
    });
    return std::nullopt;
  }

  return make_register_operand(RegisterOperand{
      .reg = *resolved->register_reference,
      .role = register_role_from_authority(resolved->authority),
      .value_id = condition_value_id,
      .value_name = condition_value_name,
      .expected_view = scalar_register_view(condition_type),
  });
}

[[nodiscard]] std::optional<OperandRecord> make_fused_compare_print_operand(
    const module::BlockLoweringContext& context,
    const CompareValueRecord& value,
    module::ModuleLoweringDiagnostics& diagnostics,
    const BlockScalarLoweringState* scalar_state) {
  if (value.source_value.kind == bir::Value::Kind::Immediate) {
    const auto immediate =
        make_scalar_immediate_operand(value.source_value, value.value_id, value.value_name);
    if (!immediate.has_value()) {
      diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
          .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
          .function_name = context.function.control_flow != nullptr
                               ? context.function.control_flow->function_name
                               : c4c::kInvalidFunctionName,
          .block_label = context.control_flow_block != nullptr
                             ? context.control_flow_block->block_label
                             : c4c::kInvalidBlockLabel,
          .instruction_family = module::InstructionLoweringFamily::BranchControl,
          .message =
              "AArch64 fused compare branch requires scalar immediate compare operands",
      });
      return std::nullopt;
    }
    return make_immediate_operand(*immediate);
  }

  if (!value.value_id.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch requires prepared compare operand identity",
    });
    return std::nullopt;
  }

  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_id = *value.value_id,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch only supports scalar integer or pointer compare operands",
    });
    return std::nullopt;
  }

  if (scalar_state != nullptr) {
    if (const auto emitted =
            find_emitted_scalar_register(*scalar_state, value.value_name);
        emitted.has_value()) {
      auto reg = *emitted;
      reg.reg.view = *expected_view;
      reg.expected_view = expected_view;
      reg.value_id = *value.value_id;
      reg.value_name = value.value_name;
      return make_register_operand(reg);
    }
  }

  auto resolved = resolve_value_operand(*value.value_id, context.function, diagnostics);
  if (!resolved.has_value() || !resolved->register_reference.has_value()) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingTypedRegisterAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .value_id = *value.value_id,
        .value_name = value.value_name,
        .message =
            "AArch64 fused compare branch requires register compare operands",
    });
    return std::nullopt;
  }

  auto reg = *resolved->register_reference;
  reg.view = *expected_view;

  return make_register_operand(RegisterOperand{
      .reg = reg,
      .role = register_role_from_authority(resolved->authority),
      .value_id = *value.value_id,
      .value_name = value.value_name,
      .expected_view = expected_view,
  });
}

[[nodiscard]] bool install_fused_compare_print_operands(
    const module::BlockLoweringContext& context,
    const BranchConditionRecord& condition,
    InstructionRecord& instruction,
    module::ModuleLoweringDiagnostics& diagnostics,
    const BlockScalarLoweringState* scalar_state) {
  if (condition.form != BranchConditionForm::FusedCompare) {
    return true;
  }
  if (!condition.compare_operands.has_value() || instruction.operands.size() < 5U) {
    diagnostics.entries.push_back(module::ModuleLoweringDiagnostic{
        .kind = module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        .function_name = context.function.control_flow != nullptr
                             ? context.function.control_flow->function_name
                             : c4c::kInvalidFunctionName,
        .block_label = context.control_flow_block != nullptr
                           ? context.control_flow_block->block_label
                           : c4c::kInvalidBlockLabel,
        .instruction_family = module::InstructionLoweringFamily::BranchControl,
        .message =
            "AArch64 fused compare branch requires printable compare operands",
    });
    return false;
  }

  auto lhs = make_fused_compare_print_operand(
      context, condition.compare_operands->lhs, diagnostics, scalar_state);
  auto rhs = make_fused_compare_print_operand(
      context, condition.compare_operands->rhs, diagnostics, scalar_state);
  if (!lhs.has_value() || !rhs.has_value()) {
    return false;
  }

  instruction.operands[3] = std::move(*lhs);
  instruction.operands[4] = std::move(*rhs);
  return true;
}

[[nodiscard]] module::MachineInstruction make_branch_instruction(
    const module::BlockLoweringContext& context,
    BranchInstructionRecord record) {
  InstructionRecord target = make_branch_instruction(std::move(record));
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;

  return module::MachineInstruction{
      .opcode = static_cast<c4c::backend::mir::TargetOpcode>(target.opcode),
      .operands = {},
      .target = std::move(target),
      .origin =
          c4c::backend::mir::MachineOrigin{
              .reason = c4c::backend::mir::MachineOriginReason::BirTerminator,
              .function_name = context.function.control_flow->function_name,
              .block_label = context.control_flow_block->block_label,
          },
  };
}

[[nodiscard]] module::MachineInstruction make_bir_compare_machine_instruction(
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

[[nodiscard]] PreparedBranchInstructionRecordResult branch_record_error(
    PreparedBranchRecordError error) {
  return PreparedBranchInstructionRecordResult{.record = std::nullopt, .error = error};
}

[[nodiscard]] bool bir_terminator_target_matches_prepared_label(
    const prepare::PreparedNameTables& names,
    c4c::BlockLabelId prepared_label,
    c4c::BlockLabelId bir_label_id,
    std::string_view bir_label) {
  if (prepared_label == c4c::kInvalidBlockLabel ||
      (bir_label_id == c4c::kInvalidBlockLabel && bir_label.empty())) {
    return false;
  }
  if (prepared_label == bir_label_id) {
    return true;
  }
  return names.block_labels.find(bir_label) == prepared_label;
}

[[nodiscard]] bir::Terminator canonicalize_branch_terminator_target(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator) {
  bir::Terminator canonical = terminator;
  if (block.terminator_kind != bir::TerminatorKind::Branch ||
      canonical.kind != bir::TerminatorKind::Branch) {
    return canonical;
  }
  if (bir_terminator_target_matches_prepared_label(names,
                                                   block.branch_target_label,
                                                   canonical.target_label_id,
                                                   canonical.target_label)) {
    canonical.target_label_id = block.branch_target_label;
  }
  return canonical;
}

[[nodiscard]] PreparedBranchRecordError validate_prepared_conditional_branch_terminator(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const bir::Terminator& terminator) {
  if (terminator.kind != bir::TerminatorKind::CondBranch) {
    return PreparedBranchRecordError::TerminatorKindMismatch;
  }
  if (terminator.true_label_id == c4c::kInvalidBlockLabel &&
      names.block_labels.find(terminator.true_label) == c4c::kInvalidBlockLabel) {
    return PreparedBranchRecordError::MissingBranchTarget;
  }
  if (terminator.false_label_id == c4c::kInvalidBlockLabel &&
      names.block_labels.find(terminator.false_label) == c4c::kInvalidBlockLabel) {
    return PreparedBranchRecordError::MissingBranchTarget;
  }
  if (!bir_terminator_target_matches_prepared_label(names,
                                                    branch_condition.true_label,
                                                    terminator.true_label_id,
                                                    terminator.true_label) ||
      !bir_terminator_target_matches_prepared_label(names,
                                                    branch_condition.false_label,
                                                    terminator.false_label_id,
                                                    terminator.false_label) ||
      !bir_terminator_target_matches_prepared_label(names,
                                                    block.true_label,
                                                    terminator.true_label_id,
                                                    terminator.true_label) ||
      !bir_terminator_target_matches_prepared_label(names,
                                                    block.false_label,
                                                    terminator.false_label_id,
                                                    terminator.false_label)) {
    return PreparedBranchRecordError::TerminatorTargetMismatch;
  }
  const bool is_fused_compare =
      branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare;
  if ((!is_fused_compare && terminator.condition.type != bir::TypeKind::I1) ||
      (is_fused_compare && terminator.condition.type == bir::TypeKind::Void)) {
    return PreparedBranchRecordError::MissingBranchCondition;
  }
  if (branch_condition.condition_value != terminator.condition) {
    return PreparedBranchRecordError::ConditionValueMismatch;
  }
  return PreparedBranchRecordError::None;
}

}  // namespace

std::string_view comparison_unconditional_branch_mnemonic(
    const InstructionRecord& instruction) {
  return machine_instruction_primary_printer_mnemonic(instruction);
}

std::optional<ComparisonBranchPrintSpelling> comparison_materialized_bool_branch_spelling(
    const InstructionRecord& instruction) {
  const auto condition_mnemonic = machine_instruction_primary_printer_mnemonic(instruction);
  const auto branch_mnemonic =
      machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind::Branch);
  if (condition_mnemonic.empty() || branch_mnemonic.empty()) {
    return std::nullopt;
  }
  return ComparisonBranchPrintSpelling{
      .condition_mnemonic = condition_mnemonic,
      .branch_mnemonic = branch_mnemonic,
  };
}

std::optional<std::string_view> f128_compare_result_condition(
    prepare::PreparedF128CmpResultZeroTest zero_test) {
  switch (zero_test) {
    case prepare::PreparedF128CmpResultZeroTest::EqualZero:
      return std::string_view{"eq"};
    case prepare::PreparedF128CmpResultZeroTest::NotEqualZero:
      return std::string_view{"ne"};
    case prepare::PreparedF128CmpResultZeroTest::LessThanZero:
      return std::string_view{"lt"};
    case prepare::PreparedF128CmpResultZeroTest::LessOrEqualZero:
      return std::string_view{"le"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterThanZero:
      return std::string_view{"gt"};
    case prepare::PreparedF128CmpResultZeroTest::GreaterOrEqualZero:
      return std::string_view{"ge"};
    case prepare::PreparedF128CmpResultZeroTest::Missing:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<std::string_view> i128_equality_compare_condition(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Eq:
      return std::string_view{"eq"};
    case bir::BinaryOpcode::Ne:
      return std::string_view{"ne"};
    default:
      return std::nullopt;
  }
}

bool is_i128_relational_compare_predicate(bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return true;
    default:
      return false;
  }
}

std::optional<I128RelationalComparePrintSpelling> i128_relational_compare_spelling(
    bir::BinaryOpcode predicate) {
  if (!is_i128_relational_compare_predicate(predicate)) {
    return std::nullopt;
  }
  const auto signed_high =
      predicate == bir::BinaryOpcode::Slt ||
      predicate == bir::BinaryOpcode::Sle ||
      predicate == bir::BinaryOpcode::Sgt ||
      predicate == bir::BinaryOpcode::Sge;
  const auto greater_predicate =
      predicate == bir::BinaryOpcode::Sgt ||
      predicate == bir::BinaryOpcode::Sge ||
      predicate == bir::BinaryOpcode::Ugt ||
      predicate == bir::BinaryOpcode::Uge;
  const auto inclusive_predicate =
      predicate == bir::BinaryOpcode::Sle ||
      predicate == bir::BinaryOpcode::Sge ||
      predicate == bir::BinaryOpcode::Ule ||
      predicate == bir::BinaryOpcode::Uge;
  return I128RelationalComparePrintSpelling{
      .high_true_condition =
          signed_high ? (greater_predicate ? std::string_view{"gt"} : std::string_view{"lt"})
                      : (greater_predicate ? std::string_view{"hi"} : std::string_view{"lo"}),
      .high_false_condition =
          signed_high ? (greater_predicate ? std::string_view{"lt"} : std::string_view{"gt"})
                      : (greater_predicate ? std::string_view{"lo"} : std::string_view{"hi"}),
      .low_true_condition =
          greater_predicate
              ? (inclusive_predicate ? std::string_view{"hs"} : std::string_view{"hi"})
              : (inclusive_predicate ? std::string_view{"ls"} : std::string_view{"lo"}),
  };
}

PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator) {
  if (function_name == c4c::kInvalidFunctionName) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != bir::TerminatorKind::Branch ||
      terminator.kind != bir::TerminatorKind::Branch) {
    return branch_record_error(PreparedBranchRecordError::TerminatorKindMismatch);
  }
  if (block.branch_target_label == c4c::kInvalidBlockLabel ||
      terminator.target_label_id == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchTarget);
  }
  if (block.branch_target_label != terminator.target_label_id) {
    return branch_record_error(PreparedBranchRecordError::TerminatorTargetMismatch);
  }

  return PreparedBranchInstructionRecordResult{
      .record =
          BranchInstructionRecord{
              .target =
                  BranchTargetOperand{
                      .surface = RecordSurfaceKind::RecordOnly,
                      .block_label = block.branch_target_label,
                      .function_name = function_name,
                  },
              .condition_record =
                  BranchConditionRecord{
                      .surface = RecordSurfaceKind::RecordOnly,
                      .form = BranchConditionForm::Unconditional,
                  },
              .conditional = false,
          },
      .error = PreparedBranchRecordError::None,
  };
}

[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record_impl(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const prepare::PreparedFusedCompareOperandProducerFacts* operand_producers) {
  if (branch_condition.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != branch_condition.function_name) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel ||
      branch_condition.block_label != block.block_label) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != bir::TerminatorKind::CondBranch) {
    return branch_record_error(PreparedBranchRecordError::TerminatorKindMismatch);
  }
  if (block.true_label == c4c::kInvalidBlockLabel ||
      block.false_label == c4c::kInvalidBlockLabel ||
      branch_condition.true_label == c4c::kInvalidBlockLabel ||
      branch_condition.false_label == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchTarget);
  }
  if (block.true_label != branch_condition.true_label ||
      block.false_label != branch_condition.false_label) {
    return branch_record_error(PreparedBranchRecordError::TerminatorTargetMismatch);
  }
  const bool is_fused_compare =
      branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare;
  if (!is_fused_compare &&
      branch_condition.condition_value.type != bir::TypeKind::I1) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchCondition);
  }
  if (is_fused_compare &&
      branch_condition.condition_value.type == bir::TypeKind::Void) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchCondition);
  }

  const auto* condition_home =
      branch_condition.condition_value.kind == bir::Value::Kind::Named
          ? prepare::find_prepared_value_home(
                names, value_locations, branch_condition.condition_value.name)
          : nullptr;
  if (condition_home == nullptr || condition_home->value_name == c4c::kInvalidValueName) {
    return branch_record_error(PreparedBranchRecordError::MissingConditionValueHome);
  }

  BranchConditionRecord condition{
      .surface = RecordSurfaceKind::RecordOnly,
      .form = branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare
                  ? BranchConditionForm::FusedCompare
                  : BranchConditionForm::MaterializedBool,
      .condition_value_id = condition_home->value_id,
      .condition_value_name = condition_home->value_name,
      .condition_type = branch_condition.condition_value.type,
      .can_fuse_with_branch = branch_condition.can_fuse_with_branch,
  };

  if (branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare) {
    if (!branch_condition.predicate.has_value() || !branch_condition.compare_type.has_value() ||
        !branch_condition.lhs.has_value() || !branch_condition.rhs.has_value()) {
      return branch_record_error(PreparedBranchRecordError::MissingCompareFacts);
    }
    if (!is_compare_predicate(*branch_condition.predicate)) {
      return branch_record_error(PreparedBranchRecordError::UnsupportedComparePredicate);
    }

    const auto make_record = [&names, &value_locations](
                                 const bir::Value& value,
                                 const std::optional<
                                     prepare::PreparedFusedCompareOperandProducer>* producer,
                                 bool allow_immediate_operand)
        -> std::optional<CompareValueRecord> {
      CompareValueRecord record{
          .surface = RecordSurfaceKind::RecordOnly,
          .value_id = std::nullopt,
          .value_name = c4c::kInvalidValueName,
          .type = value.type,
          .source_value = value,
      };
      if (value.kind == bir::Value::Kind::Immediate) {
        return record;
      }
      if (value.kind != bir::Value::Kind::Named) {
        return std::nullopt;
      }

      if (allow_immediate_operand &&
          producer != nullptr && producer->has_value() &&
          (*producer)->integer_constant.has_value() &&
          is_cmp_immediate_encodable(*(*producer)->integer_constant)) {
        record.source_value = bir::Value{
            .kind = bir::Value::Kind::Immediate,
            .type = value.type,
            .immediate = *(*producer)->integer_constant,
        };
        return record;
      }

      const auto* home =
          producer != nullptr && producer->has_value() &&
                  (*producer)->value_name != c4c::kInvalidValueName
              ? prepare::find_prepared_value_home(value_locations,
                                                  (*producer)->value_name)
              : prepare::find_prepared_value_home(names, value_locations, value.name);
      if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
        return std::nullopt;
      }
      record.value_id = home->value_id;
      record.value_name = home->value_name;
      return record;
    };

    const auto lhs = make_record(
        *branch_condition.lhs,
        operand_producers != nullptr ? &operand_producers->lhs : nullptr,
        false);
    const auto rhs = make_record(
        *branch_condition.rhs,
        operand_producers != nullptr ? &operand_producers->rhs : nullptr,
        true);
    if (!lhs.has_value() || !rhs.has_value()) {
      return branch_record_error(PreparedBranchRecordError::MissingCompareValueHome);
    }

    condition.predicate = ComparePredicateRecord{
        .surface = RecordSurfaceKind::RecordOnly,
        .source_predicate = *branch_condition.predicate,
        .compare_type = *branch_condition.compare_type,
    };
    condition.compare_operands = CompareOperandPairRecord{
        .surface = RecordSurfaceKind::RecordOnly,
        .lhs = *lhs,
        .rhs = *rhs,
        .compare_type = *branch_condition.compare_type,
    };
  }

  const auto condition_value_id = condition.condition_value_id;
  const BranchTargetPairRecord target_pair{
      .surface = RecordSurfaceKind::RecordOnly,
      .true_target =
          BranchTargetOperand{
              .surface = RecordSurfaceKind::RecordOnly,
              .block_label = branch_condition.true_label,
              .function_name = branch_condition.function_name,
              .condition_value_id = condition_value_id,
          },
      .false_target =
          BranchTargetOperand{
              .surface = RecordSurfaceKind::RecordOnly,
              .block_label = branch_condition.false_label,
              .function_name = branch_condition.function_name,
              .condition_value_id = condition_value_id,
          },
  };
  condition.compare_branch_candidate = BranchCompareCandidateRecord{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind =
          branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare
              ? (branch_condition.can_fuse_with_branch
                     ? BranchCompareCandidateKind::FusedCompareAndBranch
                     : BranchCompareCandidateKind::NonFusableCompare)
              : BranchCompareCandidateKind::MaterializedBoolCondition,
      .condition_value_id = condition.condition_value_id,
      .condition_value_name = condition.condition_value_name,
      .condition_type = condition.condition_type,
      .predicate = condition.predicate,
      .compare_operands = condition.compare_operands,
      .target_pair = target_pair,
      .can_fuse_with_branch = condition.can_fuse_with_branch,
  };

  return PreparedBranchInstructionRecordResult{
      .record =
          BranchInstructionRecord{
              .target = target_pair.true_target,
              .target_pair = target_pair,
              .condition_record = condition,
              .conditional = true,
          },
      .error = PreparedBranchRecordError::None,
  };
}

PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition) {
  return make_prepared_conditional_branch_record_impl(
      names, value_locations, block, branch_condition, nullptr);
}

std::optional<module::MachineInstruction> lower_prepared_branch_terminator(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (context.function.control_flow == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 unconditional branch lowering requires prepared and retained BIR block context");
    return std::nullopt;
  }
  if (context.control_flow_block->terminator_kind != bir::TerminatorKind::Branch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::Branch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 branch-control lowering only supports prepared unconditional branches");
    return std::nullopt;
  }

  const auto canonical_terminator =
      context.function.prepared != nullptr
          ? canonicalize_branch_terminator_target(context.function.prepared->names,
                                                  *context.control_flow_block,
                                                  context.bir_block->terminator)
          : context.bir_block->terminator;
  const auto prepared_record = make_prepared_unconditional_branch_record(
      context.function.control_flow->function_name,
      *context.control_flow_block,
      canonical_terminator);
  if (!prepared_record.record.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 unconditional branch lowering rejected prepared branch facts: "} +
            std::string{prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }

  auto instruction = make_branch_instruction(context, std::move(*prepared_record.record));
  if (instruction.target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 unconditional branch lowering did not produce a selected machine node");
    return std::nullopt;
  }
  return instruction;
}

std::optional<module::MachineInstruction> lower_prepared_conditional_branch_terminator(
    const module::BlockLoweringContext& context,
    module::ModuleLoweringDiagnostics& diagnostics,
    const BlockScalarLoweringState* scalar_state) {
  if (context.function.prepared == nullptr || context.function.control_flow == nullptr ||
      context.function.value_locations == nullptr || context.control_flow_block == nullptr ||
      context.bir_block == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering requires prepared and retained BIR block context");
    return std::nullopt;
  }
  if (context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 branch-control lowering only supports prepared conditional branches");
    return std::nullopt;
  }

  const auto branch_facts = find_prepared_conditional_branch_facts(context);
  if (!branch_facts.has_value() || branch_facts->branch_condition == nullptr) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering requires prepared branch condition authority");
    return std::nullopt;
  }
  const auto& branch_condition = *branch_facts->branch_condition;
  if (branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare &&
      !branch_condition.can_fuse_with_branch) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering keeps non-fusable compare branches fail-closed");
    return std::nullopt;
  }

  const auto operand_producers =
      find_prepared_fused_compare_operand_producer_facts(context, branch_condition);
  auto prepared_record = make_prepared_conditional_branch_record_impl(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.control_flow_block,
      branch_condition,
      operand_producers.has_value() ? &*operand_producers : nullptr);
  if (!prepared_record.record.has_value()) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 conditional branch lowering rejected prepared branch facts: "} +
            std::string{prepared_branch_record_error_name(prepared_record.error)});
    return std::nullopt;
  }
  const auto terminator_validation = validate_prepared_conditional_branch_terminator(
      context.function.prepared->names,
      *context.control_flow_block,
      branch_condition,
      context.bir_block->terminator);
  if (terminator_validation != PreparedBranchRecordError::None) {
    append_branch_diagnostic(
        diagnostics,
        context,
        std::string{"AArch64 conditional branch lowering rejected raw terminator validation: "} +
            std::string{prepared_branch_record_error_name(terminator_validation)});
    return std::nullopt;
  }

  auto& record = *prepared_record.record;
  apply_prepared_conditional_branch_targets(record, *branch_facts);
  if (branch_condition.kind == prepare::PreparedBranchConditionKind::MaterializedBool) {
    if (!record.condition_record.has_value() ||
        !record.condition_record->condition_value_id.has_value()) {
      append_branch_diagnostic(
          diagnostics,
          context,
          "AArch64 conditional branch lowering requires prepared condition value identity");
      return std::nullopt;
    }
    auto condition_operand = make_condition_register_operand(
        context,
        *record.condition_record->condition_value_id,
        record.condition_record->condition_value_name,
        record.condition_record->condition_type,
        diagnostics,
        scalar_state);
    if (!condition_operand.has_value()) {
      return std::nullopt;
    }
    record.condition = std::move(*condition_operand);
  }

  auto instruction = make_branch_instruction(context, std::move(record));
  const auto* branch =
      std::get_if<BranchInstructionRecord>(&instruction.target.payload);
  if (branch != nullptr && branch->condition_record.has_value() &&
      !install_fused_compare_print_operands(
          context, *branch->condition_record, instruction.target, diagnostics, scalar_state)) {
    return std::nullopt;
  }
  const auto expected_opcode =
      branch_condition.kind == prepare::PreparedBranchConditionKind::FusedCompare
          ? MachineOpcode::CompareBranch
          : MachineOpcode::ConditionalBranch;
  if (instruction.target.selection.status != MachineNodeSelectionStatus::Selected ||
      instruction.target.opcode != expected_opcode) {
    append_branch_diagnostic(
        diagnostics,
        context,
        "AArch64 conditional branch lowering did not produce a selected branch-control node");
    return std::nullopt;
  }
  return instruction;
}

std::optional<module::MachineInstruction> lower_prepared_i128_compare_instruction(
    const module::BlockLoweringContext& context,
    const bir::BinaryInst& binary,
    std::size_t instruction_index,
    module::ModuleLoweringDiagnostics& diagnostics) {
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I1 ||
      !is_compare_predicate(binary.opcode)) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::UnsupportedInstructionFamily,
        context,
        instruction_index,
        "AArch64 i128 comparison lowering only supports prepared i128 compare instructions");
    return std::nullopt;
  }
  if (context.function.prepared == nullptr ||
      context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return std::nullopt;
  }

  const auto* i128_carriers =
      prepare::find_prepared_i128_carriers(*context.function.prepared,
                                           context.function.control_flow->function_name);
  if (i128_carriers == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingPreparedI128Carrier));
    return std::nullopt;
  }

  if (context.function.value_locations == nullptr ||
      context.function.storage_plan == nullptr) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(
            PreparedI128PairRecordError::MissingScalarResultStorage));
    return std::nullopt;
  }

  auto prepared = make_prepared_i128_compare_record(
      context.function.prepared->names,
      *context.function.value_locations,
      *context.function.storage_plan,
      *i128_carriers,
      binary);
  if (!prepared.record.has_value()) {
    append_i128_compare_diagnostic(
        diagnostics,
        module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
        context,
        instruction_index,
        i128_compare_error_message(prepared.error));
    return std::nullopt;
  }

  auto target = make_i128_compare_instruction(*prepared.record);
  target.function_name = context.function.control_flow->function_name;
  target.block_label = context.control_flow_block->block_label;
  target.block_index = context.block_index;
  target.instruction_index = instruction_index;
  if (auto* record = std::get_if<I128CompareRecord>(&target.payload)) {
    record->block_label = context.control_flow_block->block_label;
    record->instruction_index = instruction_index;
  }
  if (target.selection.status != MachineNodeSelectionStatus::Selected) {
    append_i128_compare_diagnostic(diagnostics,
                                   module::ModuleLoweringDiagnosticKind::MissingValueAuthority,
                                   context,
                                   instruction_index,
                                   std::string{target.selection.diagnostic});
    return std::nullopt;
  }

  return make_bir_compare_machine_instruction(
      context, instruction_index, std::move(target));
}

mir::MachineBlockSuccessor make_unconditional_branch_successor(
    const module::BlockLoweringContext& context) {
  return mir::MachineBlockSuccessor{
      .target_label = context.control_flow_block != nullptr
                          ? context.control_flow_block->branch_target_label
                          : c4c::kInvalidBlockLabel,
      .kind = mir::MachineBlockSuccessorKind::Unconditional,
      .origin =
          mir::MachineOrigin{
              .reason = mir::MachineOriginReason::BirTerminator,
              .function_name = context.function.control_flow != nullptr
                                   ? context.function.control_flow->function_name
                                   : c4c::kInvalidFunctionName,
              .block_label = context.control_flow_block != nullptr
                                 ? context.control_flow_block->block_label
                                 : c4c::kInvalidBlockLabel,
          },
  };
}

std::vector<mir::MachineBlockSuccessor> make_conditional_branch_successors(
    const module::BlockLoweringContext& context) {
  const mir::MachineOrigin origin{
      .reason = mir::MachineOriginReason::BirTerminator,
      .function_name = context.function.control_flow != nullptr
                           ? context.function.control_flow->function_name
                           : c4c::kInvalidFunctionName,
      .block_label = context.control_flow_block != nullptr
                         ? context.control_flow_block->block_label
                         : c4c::kInvalidBlockLabel,
  };
  return {
      mir::MachineBlockSuccessor{
          .target_label = context.control_flow_block != nullptr
                              ? context.control_flow_block->true_label
                              : c4c::kInvalidBlockLabel,
          .kind = mir::MachineBlockSuccessorKind::ConditionalTrue,
          .origin = origin,
      },
      mir::MachineBlockSuccessor{
          .target_label = context.control_flow_block != nullptr
                              ? context.control_flow_block->false_label
                              : c4c::kInvalidBlockLabel,
          .kind = mir::MachineBlockSuccessorKind::ConditionalFalse,
          .origin = origin,
      },
  };
}

namespace mir = c4c::backend::mir;
namespace {

[[nodiscard]] std::optional<unsigned> branch_fusion_integer_bit_width(bir::TypeKind type) {
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

[[nodiscard]] std::string machine_block_label(c4c::FunctionNameId function_name,
                                              c4c::BlockLabelId block_label) {
  return ".LBB" + std::to_string(function_name) + "_" + std::to_string(block_label);
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

[[nodiscard]] const prepare::PreparedFrameSlot* find_frame_slot(
    const prepare::PreparedStackLayout& stack_layout,
    prepare::PreparedFrameSlotId slot_id) {
  return prepare::find_frame_slot_by_id(stack_layout, slot_id);
}

[[nodiscard]] std::optional<std::string> branch_fusion_prepared_frame_slot_load_address(
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
  std::string address =
      context.function.frame_plan != nullptr &&
              context.function.frame_plan->uses_frame_pointer_for_fixed_slots
          ? "[x29"
          : "[sp";
  if (offset != 0) {
    address += ", #";
    address += std::to_string(offset);
  }
  address += "]";
  return address;
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

[[nodiscard]] bool fused_compare_operand_has_select_producer(
    const std::optional<bir::ComparisonOperandProducer>& producer) {
  return producer.has_value() &&
         producer->producer_kind == bir::ComparisonProducerKind::Select &&
         producer->producer_instruction != nullptr;
}

[[nodiscard]] std::optional<std::uint8_t>
preferred_fused_compare_operand_publication_target(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() <= 1U) {
    return std::nullopt;
  }
  if (context.bir_block == nullptr) {
    return std::nullopt;
  }
  const auto producer = bir::find_comparison_operand_producer(
      *context.bir_block, value, before_instruction_index);
  if (!fused_compare_operand_has_select_producer(producer)) {
    return std::nullopt;
  }
  return scratches[1].index;
}

}  // namespace

std::optional<std::string_view> branch_condition_suffix(
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

bool is_cmp_immediate_encodable(std::int64_t value) {
  return value >= 0 && value <= 4095;
}

std::optional<module::MachineInstruction>
lower_missing_conditional_branch_condition_publication(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    const DispatchBranchFusionHooks& hooks) {
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
  const auto* branch_condition =
      context.function.control_flow != nullptr
          ? prepare::find_prepared_branch_condition(
                *context.function.control_flow,
                context.control_flow_block->block_label)
          : nullptr;
  if (branch_condition == nullptr ||
      branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      branch_condition->condition_value.name != condition.name) {
    return std::nullopt;
  }
  const auto producer = hooks.prepared_publication_source_producer_for_value(
      context, branch_condition->condition_value);
  const auto* producer_inst =
      producer.has_value()
          ? hooks.prepared_source_producer_instruction(context, *producer)
          : nullptr;
  if (!producer.has_value() || producer_inst == nullptr) {
    return std::nullopt;
  }
  return lower_scalar_control_value_instruction(
      context,
      *producer_inst,
      producer->instruction_index,
      scalar_state,
      diagnostics,
      true);
}

std::optional<module::MachineInstruction>
lower_missing_fused_compare_operand_publication(
    const module::BlockLoweringContext& context,
    const bir::Value& value,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    const DispatchBranchFusionHooks& hooks,
    std::optional<std::uint8_t> preferred_target_index) {
  if (context.bir_block == nullptr || value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto value_name = prepared_named_value_id(context, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto* home = hooks.prepared_value_home_for_value(context, value);
  if (home == nullptr) {
    return std::nullopt;
  }
  if (find_emitted_scalar_register(scalar_state, *value_name).has_value() &&
      home->kind != prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  auto resolved =
      resolve_value_operand(home->value_id, context.function, diagnostics);
  const auto expected_view = hooks.scalar_view_for_type(value.type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  if (auto published =
          hooks.current_block_entry_publication_register(context,
                                                         value,
                                                         *expected_view)) {
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
    auto scratch_is_occupied = [&](const abi::RegisterReference& scratch) {
      for (const auto& [_, emitted] : scalar_state.emitted_registers) {
        if (emitted.reg.bank == scratch.bank && emitted.reg.index == scratch.index) {
          return true;
        }
      }
      return false;
    };
    if (preferred_target_index.has_value()) {
      for (const auto scratch : scratches) {
        if (scratch.index == *preferred_target_index &&
            !scratch_is_occupied(scratch)) {
          target_index = scratch.index;
          has_target = true;
          break;
        }
      }
    }
    for (const auto scratch : scratches) {
      if (!has_target && !scratch_is_occupied(scratch)) {
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
  const auto producer =
      bir::find_comparison_operand_producer(*context.bir_block,
                                            value,
                                            context.bir_block->insts.size());
  if (producer.has_value() && producer->producer_instruction != nullptr) {
    if (!hooks.emit_value_publication_to_register(context,
                                                  value,
                                                  context.bir_block->insts.size(),
                                                  target_index,
                                                  scratch_index,
                                                  lines,
                                                  true) ||
        lines.empty()) {
      return std::nullopt;
    }
  } else {
    const auto plan = prepare::plan_prepared_scalar_publication(
        prepare::PreparedScalarPublicationInputs{
            .source_value = &value,
            .destination_home = home,
        });
    if (!prepare::prepared_scalar_publication_available(plan) ||
        (plan.hook_kind != prepare::PreparedScalarPublicationHookKind::RegisterHome &&
         plan.hook_kind != prepare::PreparedScalarPublicationHookKind::StackSlotHome) ||
        !hooks.emit_prepared_value_home_publication_to_register(context,
                                                                value,
                                                                *home,
                                                                target_index,
                                                                lines) ||
        lines.empty()) {
      return std::nullopt;
    }
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

std::vector<module::MachineInstruction>
lower_missing_fused_compare_operand_publications(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    module::ModuleLoweringDiagnostics& diagnostics,
    const DispatchBranchFusionHooks& hooks) {
  std::vector<module::MachineInstruction> lowered;
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
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
    const auto preferred_target_index =
        preferred_fused_compare_operand_publication_target(
            context, *branch_condition->lhs, context.bir_block->insts.size());
    if (auto lhs = lower_missing_fused_compare_operand_publication(
            context,
            *branch_condition->lhs,
            scalar_state,
            diagnostics,
            hooks,
            preferred_target_index)) {
      lowered.push_back(std::move(*lhs));
    }
  }
  if (branch_condition->rhs.has_value()) {
    const auto preferred_target_index =
        preferred_fused_compare_operand_publication_target(
            context, *branch_condition->rhs, context.bir_block->insts.size());
    if (auto rhs = lower_missing_fused_compare_operand_publication(
            context,
            *branch_condition->rhs,
            scalar_state,
            diagnostics,
            hooks,
            preferred_target_index)) {
      lowered.push_back(std::move(*rhs));
    }
  }
  return lowered;
}

std::optional<module::MachineInstruction>
lower_fused_compare_branch_from_emitted_cast(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value()) {
    return std::nullopt;
  }

  const auto before_instruction_index =
      context.bir_block->insts.size();
  auto producer_cast =
      [](const std::optional<bir::ComparisonOperandProducer>& producer)
          -> const bir::CastInst* {
    if (!producer.has_value() ||
        producer->producer_kind != bir::ComparisonProducerKind::Cast ||
        producer->producer_instruction == nullptr) {
      return nullptr;
    }
    return std::get_if<bir::CastInst>(producer->producer_instruction);
  };
  const bir::Value* cast_value = &branch_facts->lhs;
  const bir::Value* other_value = &branch_facts->rhs;
  auto cast_producer = bir::find_comparison_operand_producer(
      *context.bir_block, *cast_value, before_instruction_index);
  if (producer_cast(cast_producer) == nullptr) {
    cast_value = &branch_facts->rhs;
    other_value = &branch_facts->lhs;
    cast_producer = bir::find_comparison_operand_producer(
        *context.bir_block, *cast_value, before_instruction_index);
  }
  const bir::CastInst* cast = producer_cast(cast_producer);
  if (cast == nullptr ||
      cast->opcode != bir::CastOpcode::SExt ||
      cast->operand.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }

  const auto source_bits = branch_fusion_integer_bit_width(cast->operand.type);
  const auto result_bits = branch_fusion_integer_bit_width(cast->result.type);
  const auto result_view = scalar_register_view(cast->result.type);
  if (!source_bits.has_value() || !result_bits.has_value() ||
      !result_view.has_value() || *source_bits >= *result_bits) {
    return std::nullopt;
  }
  auto source_name =
      emitted_register_name(context, cast->operand, scalar_state, abi::RegisterView::W);
  auto rhs_name = compare_operand_name(context, *other_value, scalar_state, *result_view);
  if (!rhs_name.has_value()) {
    const auto rhs_producer = bir::find_comparison_operand_producer(
        *context.bir_block, *other_value, before_instruction_index);
    if (rhs_producer.has_value() &&
        rhs_producer->integer_constant.has_value() &&
        is_cmp_immediate_encodable(*rhs_producer->integer_constant)) {
      rhs_name = "#" + std::to_string(*rhs_producer->integer_constant);
    }
  }
  std::vector<std::string> lines;
  if (!source_name.has_value()) {
    const auto load_producer = bir::find_comparison_operand_producer(
        *context.bir_block, cast->operand, cast_producer->producer_instruction_index);
    const auto* load_local =
        load_producer.has_value() && load_producer->producer_instruction != nullptr
            ? std::get_if<bir::LoadLocalInst>(load_producer->producer_instruction)
            : nullptr;
    const auto load_address =
        load_local != nullptr
            ? branch_fusion_prepared_frame_slot_load_address(
                  context, load_producer->producer_instruction_index)
            : std::optional<std::string>{};
    if (!load_producer.has_value() ||
        load_producer->producer_kind != bir::ComparisonProducerKind::LoadLocal ||
        load_local == nullptr ||
        load_local->result.type != bir::TypeKind::I8 ||
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
  append_prepared_compare_branch_lines(lines, *branch_facts);
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

std::optional<module::MachineInstruction>
lower_materialized_compare_condition_branch(
    const module::BlockLoweringContext& context,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch ||
      context.bir_block->terminator.kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts = find_prepared_conditional_branch_facts(context);
  if (!branch_facts.has_value() || branch_facts->branch_condition == nullptr ||
      branch_facts->branch_condition->condition_value.kind != bir::Value::Kind::Named ||
      branch_facts->branch_condition->condition_value != context.bir_block->terminator.condition) {
    return std::nullopt;
  }
  const auto& branch_condition = *branch_facts->branch_condition;
  const auto* condition_home =
      prepare::find_prepared_value_home_for_bir_value(
          context.function.prepared->names,
          context.function.value_home_lookups,
          context.function.regalloc,
          context.function.value_locations,
          branch_condition.condition_value);
  if (condition_home == nullptr) {
    return std::nullopt;
  }
  const auto before_instruction_index = context.bir_block->insts.size();
  const auto bir_producer =
      find_bir_materialized_condition_producer_identity(
          *context.bir_block,
          branch_condition.condition_value,
          before_instruction_index);
  auto selected_producer = bir_producer;
  const auto route7_producer =
      find_valid_route7_materialized_condition_producer_identity(
          *context.bir_block,
          branch_condition.condition_value,
          before_instruction_index);
  const auto prepared_producer =
      find_prepared_materialized_condition_producer_identity(
          context, branch_condition.condition_value, before_instruction_index);
  if (route7_producer.has_value() &&
      prepared_producer.has_value() &&
      materialized_condition_identity_matches_bir(*route7_producer, bir_producer) &&
      materialized_condition_identity_agrees_with_prepared(
          context, *prepared_producer, *route7_producer)) {
    selected_producer = *route7_producer;
  }
  const auto* binary =
      selected_producer.available ? selected_producer.binary : nullptr;
  if (binary == nullptr) {
    return std::nullopt;
  }
  const auto condition = branch_condition_suffix(binary->opcode);
  const auto operand_view = hooks.scalar_view_for_type(binary->operand_type);
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
      branch_facts->targets.true_label == c4c::kInvalidBlockLabel ||
      branch_facts->targets.false_label == c4c::kInvalidBlockLabel) {
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
            hooks.current_block_entry_publication_register(context, lhs, *operand_view)) {
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
    if (!hooks.emit_value_publication_to_register(context,
                                                  lhs,
                                                  selected_producer.instruction_index,
                                                  scratches[0].index,
                                                  scratches[1].index,
                                                  lines,
                                                  false)) {
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
      if (!hooks.emit_value_publication_to_register(context,
                                                    rhs_value,
                                                    selected_producer.instruction_index,
                                                    rhs_target_index,
                                                    rhs_scratch_index,
                                                    lines,
                                                    false)) {
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
                  machine_block_label(branch_condition.function_name,
                                      branch_facts->targets.true_label));
  lines.push_back("b " + machine_block_label(branch_condition.function_name,
                                             branch_facts->targets.false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

std::optional<module::MachineInstruction>
lower_current_block_entry_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value()) {
    return std::nullopt;
  }
  auto lhs = branch_facts->lhs;
  lhs.type = branch_facts->compare_type;
  const auto published_lhs =
      current_block_entry_publication_register(context, lhs, branch_facts->operand_view);
  if (!published_lhs.has_value() ||
      published_lhs->reg.bank != abi::RegisterBank::GeneralPurpose ||
      branch_facts->rhs.kind != bir::Value::Kind::Immediate ||
      !is_cmp_immediate_encodable(branch_facts->rhs.immediate)) {
    return std::nullopt;
  }
  const auto lhs_reg = abi::gp_register(published_lhs->reg.index, branch_facts->operand_view);
  if (!lhs_reg.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_reg)} + ", #" +
                  std::to_string(branch_facts->rhs.immediate));
  append_prepared_compare_branch_lines(lines, *branch_facts);
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

std::optional<module::MachineInstruction>
lower_constant_rhs_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.bir_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value()) {
    return std::nullopt;
  }

  if (branch_facts->rhs.kind != bir::Value::Kind::Named ||
      branch_facts->rhs.name.empty()) {
    return std::nullopt;
  }
  auto rhs = branch_facts->rhs;
  rhs.type = branch_facts->compare_type;
  const auto rhs_producer = bir::find_comparison_operand_producer(
      *context.bir_block, rhs, context.bir_block->insts.size());
  const auto* rhs_binary =
      rhs_producer.has_value() && rhs_producer->producer_instruction != nullptr
          ? std::get_if<bir::BinaryInst>(rhs_producer->producer_instruction)
          : nullptr;
  const auto* rhs_home =
      prepare::find_prepared_value_home_for_bir_value(
          context.function.prepared->names,
          context.function.value_home_lookups,
          context.function.regalloc,
          context.function.value_locations,
          rhs);
  if (!rhs_producer.has_value() ||
      rhs_producer->producer_kind != bir::ComparisonProducerKind::Binary ||
      rhs_binary == nullptr ||
      rhs_home == nullptr ||
      rhs_home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      value_has_current_block_entry_publication(context, *rhs_home)) {
    return std::nullopt;
  }
  if (!rhs_producer->integer_constant.has_value() ||
      !is_cmp_immediate_encodable(*rhs_producer->integer_constant)) {
    return std::nullopt;
  }

  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.size() < 2U) {
    return std::nullopt;
  }
  const auto lhs_reg = abi::gp_register(scratches[0].index, branch_facts->operand_view);
  if (!lhs_reg.has_value()) {
    return std::nullopt;
  }
  auto lhs = branch_facts->lhs;
  lhs.type = branch_facts->compare_type;
  std::vector<std::string> lines;
  if (!hooks.emit_value_publication_to_register(context,
                                                lhs,
                                                context.bir_block->insts.size(),
                                                scratches[0].index,
                                                scratches[1].index,
                                                lines,
                                                true)) {
    return std::nullopt;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*lhs_reg)} + ", #" +
                  std::to_string(*rhs_producer->integer_constant));
  append_prepared_compare_branch_lines(lines, *branch_facts);
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

std::optional<module::MachineInstruction>
lower_conditional_branch_from_emitted_condition(
    const module::BlockLoweringContext& context,
    BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts = find_prepared_conditional_branch_facts(context);
  if (!branch_facts.has_value() || branch_facts->branch_condition == nullptr ||
      branch_facts->branch_condition->condition_value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto& branch_condition = *branch_facts->branch_condition;
  const auto condition_name =
      prepared_named_value_id(context, branch_condition.condition_value);
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
        hooks.scalar_view_for_type(branch_condition.condition_value.type)
            .value_or(abi::RegisterView::W);
    if (!hooks.emit_value_publication_to_register(context,
                                                  branch_condition.condition_value,
                                                  context.bir_block != nullptr
                                                      ? context.bir_block->insts.size()
                                                      : 0U,
                                                  scratches[0].index,
                                                  scratches[1].index,
                                                  lines,
                                                  false)) {
      return std::nullopt;
    }
    auto reg = abi::gp_register(scratches[0].index, expected_view);
    if (!reg.has_value()) {
      return std::nullopt;
    }
    const auto* home =
        prepare::find_prepared_value_home_for_bir_value(
            context.function.prepared->names,
            context.function.value_home_lookups,
            context.function.regalloc,
            context.function.value_locations,
            branch_condition.condition_value);
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
      branch_facts->targets.true_label == c4c::kInvalidBlockLabel ||
      branch_facts->targets.false_label == c4c::kInvalidBlockLabel) {
    return std::nullopt;
  }
  lines.push_back("cbnz " + condition + ", " +
                  machine_block_label(branch_condition.function_name,
                                      branch_facts->targets.true_label));
  lines.push_back("b " + machine_block_label(branch_condition.function_name,
                                             branch_facts->targets.false_label));
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

bool is_fused_compare_branch_support_instruction(
    const module::BlockLoweringContext& context,
    const bir::Inst& inst,
    const BlockScalarLoweringState& scalar_state,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr) {
    return false;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value()) {
    return false;
  }
  if (const auto* cast = std::get_if<bir::CastInst>(&inst);
      cast != nullptr &&
      cast->result.kind == bir::Value::Kind::Named) {
    if (!lower_fused_compare_branch_from_emitted_cast(context, scalar_state, hooks)
             .has_value()) {
      return false;
    }
    const auto matches = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named &&
             value.name == cast->result.name;
    };
    return matches(branch_facts->lhs) || matches(branch_facts->rhs);
  }
  if (const auto* binary = std::get_if<bir::BinaryInst>(&inst);
      binary != nullptr &&
      binary->result.kind == bir::Value::Kind::Named &&
      branch_facts->branch_condition->condition_value.kind == bir::Value::Kind::Named) {
    if (binary->result.name == branch_facts->branch_condition->condition_value.name) {
      return true;
    }
    const auto matches_binary_result = [&](const bir::Value& value) {
      return value.kind == bir::Value::Kind::Named &&
             value.name == binary->result.name;
    };
    if ((matches_binary_result(branch_facts->lhs) ||
         matches_binary_result(branch_facts->rhs)) &&
        mir::evaluate_same_block_integer_constant(context.bir_block, binary->result).has_value()) {
      return true;
    }
  }
  return false;
}

std::optional<module::MachineInstruction>
lower_stack_home_fused_compare_branch(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.function.prepared == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return std::nullopt;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value() ||
      branch_facts->rhs.kind != bir::Value::Kind::Immediate ||
      !is_cmp_immediate_encodable(branch_facts->rhs.immediate)) {
    return std::nullopt;
  }
  auto lhs = branch_facts->lhs;
  lhs.type = branch_facts->compare_type;
  if (lhs.kind == bir::Value::Kind::Named && context.bir_block != nullptr) {
    const auto producer = bir::find_comparison_operand_producer(
        *context.bir_block, lhs, context.bir_block->insts.size());
    const auto has_allowed_load_producer =
        producer.has_value() &&
        (producer->producer_kind == bir::ComparisonProducerKind::LoadLocal ||
         producer->producer_kind == bir::ComparisonProducerKind::LoadGlobal);
    if (producer.has_value() && !has_allowed_load_producer) {
      return std::nullopt;
    }
  }
  const auto* home = hooks.prepared_value_home_for_value(context, lhs);
  if (home == nullptr ||
      home->kind != prepare::PreparedValueHomeKind::StackSlot ||
      !home->offset_bytes.has_value()) {
    return std::nullopt;
  }
  const auto scratches = abi::reserved_mir_scratch_gp_registers();
  if (scratches.empty()) {
    return std::nullopt;
  }
  const auto compare_reg = abi::gp_register(scratches.front().index, branch_facts->operand_view);
  if (!compare_reg.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string> lines;
  if (!hooks.emit_prepared_value_home_to_register(
          &context.function.prepared->stack_layout,
          *home,
          branch_facts->compare_type,
          scratches.front().index,
          lines,
          hooks.fixed_slots_use_frame_pointer(context.function))) {
    return std::nullopt;
  }
  lines.push_back("cmp " + std::string{abi::register_name(*compare_reg)} + ", #" +
                  std::to_string(branch_facts->rhs.immediate));
  append_prepared_compare_branch_lines(lines, *branch_facts);
  return make_branch_compare_assembler_instruction(context, std::move(lines));
}

bool fused_compare_uses_selected_operand(
    const module::BlockLoweringContext& context,
    const DispatchBranchFusionHooks& hooks) {
  if (context.function.control_flow == nullptr ||
      context.control_flow_block == nullptr ||
      context.control_flow_block->terminator_kind != bir::TerminatorKind::CondBranch) {
    return false;
  }
  const auto branch_facts =
      find_prepared_fused_compare_branch_facts(context, hooks);
  if (!branch_facts.has_value()) {
    return false;
  }
  if (context.bir_block == nullptr) {
    return false;
  }
  const auto route7_index =
      bir::route7_build_comparison_condition_index(*context.bir_block);
  const auto route7_producer_facts =
      bir::route7_find_fused_compare_operand_producer_facts(
          route7_index,
          *context.bir_block,
          branch_facts->lhs,
          branch_facts->rhs,
          context.bir_block->insts.size());
  const auto bir_producer_facts = bir::find_fused_compare_operand_producer_facts(
      *context.bir_block,
      branch_facts->lhs,
      branch_facts->rhs,
      context.bir_block->insts.size());
  const auto& producer_facts =
      route7_producer_facts.available ? route7_producer_facts
                                      : bir_producer_facts;
  return producer_facts.available &&
         (fused_compare_operand_has_select_producer(producer_facts.lhs) ||
          fused_compare_operand_has_select_producer(producer_facts.rhs));
}


}  // namespace c4c::backend::aarch64::codegen

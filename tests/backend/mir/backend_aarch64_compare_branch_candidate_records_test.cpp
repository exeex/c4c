#include "src/backend/mir/aarch64/codegen/records.hpp"

#include <iostream>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named_value(bir::TypeKind type, const char* name) {
  return bir::Value::named(type, name);
}

aarch64_codegen::BranchTargetOperand target(c4c::FunctionNameId function_name,
                                            c4c::BlockLabelId block_label,
                                            prepare::PreparedValueId condition_value_id) {
  return aarch64_codegen::BranchTargetOperand{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .block_label = block_label,
      .function_name = function_name,
      .condition_value_id = condition_value_id,
  };
}

prepare::PreparedValueLocationFunction value_locations(c4c::FunctionNameId function_name,
                                                       c4c::ValueNameId condition_name,
                                                       c4c::ValueNameId lhs_name,
                                                       c4c::ValueNameId rhs_name) {
  return prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{40},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{41},
                  .function_name = function_name,
                  .value_name = lhs_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{42},
                  .function_name = function_name,
                  .value_name = rhs_name,
              },
          },
  };
}

bir::Terminator conditional_terminator(const bir::Value& condition,
                                       c4c::BlockLabelId true_label,
                                       c4c::BlockLabelId false_label) {
  return bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "then",
      .false_label = "else",
      .true_label_id = true_label,
      .false_label_id = false_label,
  };
}

int materialized_bool_candidate_keeps_condition_and_targets_only() {
  const auto pair = aarch64_codegen::BranchTargetPairRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .true_target = target(c4c::FunctionNameId{3},
                            c4c::BlockLabelId{20},
                            prepare::PreparedValueId{40}),
      .false_target = target(c4c::FunctionNameId{3},
                             c4c::BlockLabelId{21},
                             prepare::PreparedValueId{40}),
  };
  const auto candidate = aarch64_codegen::BranchCompareCandidateRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .kind = aarch64_codegen::BranchCompareCandidateKind::MaterializedBoolCondition,
      .condition_value_id = prepare::PreparedValueId{40},
      .condition_value_name = c4c::ValueNameId{9},
      .condition_type = bir::TypeKind::I1,
      .target_pair = pair,
      .can_fuse_with_branch = false,
  };

  if (aarch64_codegen::branch_compare_candidate_kind_name(candidate.kind) !=
      "materialized_bool_condition") {
    return fail("expected materialized bool candidate kind name");
  }
  if (candidate.predicate.has_value() || candidate.compare_operands.has_value() ||
      !candidate.target_pair.has_value() ||
      candidate.target_pair->true_target.block_label != c4c::BlockLabelId{20} ||
      candidate.target_pair->false_target.block_label != c4c::BlockLabelId{21} ||
      candidate.condition_value_id != prepare::PreparedValueId{40} ||
      candidate.condition_value_name != c4c::ValueNameId{9} ||
      candidate.condition_type != bir::TypeKind::I1 || candidate.can_fuse_with_branch) {
    return fail("expected materialized bool candidate to carry no compare facts");
  }
  return 0;
}

int prepared_fused_and_non_fusable_compare_candidates_are_distinct() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("f");
  const auto source_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("then");
  const auto false_label = names.block_labels.intern("else");
  const auto condition_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto locations = value_locations(function_name, condition_name, lhs_name, rhs_name);
  const auto condition_value = named_value(bir::TypeKind::I1, "%cmp");
  const auto lhs_value = named_value(bir::TypeKind::I64, "%lhs");
  const auto rhs_value = bir::Value::immediate_i64(7);
  const auto block = prepare::PreparedControlFlowBlock{
      .block_label = source_label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
  const auto fused_condition = prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = source_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = condition_value,
      .predicate = bir::BinaryOpcode::Uge,
      .compare_type = bir::TypeKind::I64,
      .lhs = lhs_value,
      .rhs = rhs_value,
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };

  const auto fused = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations, block, fused_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (!fused.record.has_value() || !fused.record->condition_record.has_value() ||
      !fused.record->condition_record->compare_branch_candidate.has_value()) {
    return fail("expected fused compare candidate conversion to succeed");
  }
  const auto& fused_candidate = *fused.record->condition_record->compare_branch_candidate;
  if (fused_candidate.kind != aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch ||
      aarch64_codegen::branch_compare_candidate_kind_name(fused_candidate.kind) !=
          "fused_compare_and_branch" ||
      !fused_candidate.can_fuse_with_branch || !fused_candidate.predicate.has_value() ||
      !fused_candidate.compare_operands.has_value() || !fused_candidate.target_pair.has_value() ||
      fused_candidate.predicate->source_predicate != bir::BinaryOpcode::Uge ||
      fused_candidate.predicate->compare_type != bir::TypeKind::I64 ||
      fused_candidate.compare_operands->lhs.value_id != prepare::PreparedValueId{41} ||
      fused_candidate.compare_operands->rhs.value_id.has_value() ||
      fused_candidate.compare_operands->rhs.source_value != rhs_value) {
    return fail("expected fused candidate to preserve compare metadata and immediates");
  }

  auto non_fusable_condition = fused_condition;
  non_fusable_condition.can_fuse_with_branch = false;
  const auto non_fusable = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations, block, non_fusable_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (!non_fusable.record.has_value() ||
      !non_fusable.record->condition_record->compare_branch_candidate.has_value()) {
    return fail("expected non-fusable compare facts to remain explicit");
  }
  const auto& non_fusable_candidate =
      *non_fusable.record->condition_record->compare_branch_candidate;
  if (non_fusable_candidate.kind !=
          aarch64_codegen::BranchCompareCandidateKind::NonFusableCompare ||
      aarch64_codegen::branch_compare_candidate_kind_name(non_fusable_candidate.kind) !=
          "non_fusable_compare" ||
      non_fusable_candidate.can_fuse_with_branch ||
      !non_fusable_candidate.predicate.has_value() ||
      !non_fusable_candidate.compare_operands.has_value() ||
      !non_fusable_candidate.target_pair.has_value() ||
      non_fusable_candidate.condition_value_id != prepare::PreparedValueId{40} ||
      non_fusable_candidate.condition_value_name != condition_name ||
      non_fusable_candidate.target_pair->true_target.block_label != true_label ||
      non_fusable_candidate.target_pair->false_target.block_label != false_label) {
    return fail("expected non-fusable compare candidate to preserve facts distinctly");
  }
  return 0;
}

int unsupported_predicates_fail_closed_before_candidate_record() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("f");
  const auto source_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("then");
  const auto false_label = names.block_labels.intern("else");
  const auto condition_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto locations = value_locations(function_name, condition_name, lhs_name, rhs_name);
  const auto condition_value = named_value(bir::TypeKind::I1, "%cmp");
  const auto block = prepare::PreparedControlFlowBlock{
      .block_label = source_label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
  const auto branch_condition = prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = source_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = condition_value,
      .predicate = bir::BinaryOpcode::Add,
      .compare_type = bir::TypeKind::I64,
      .lhs = named_value(bir::TypeKind::I64, "%lhs"),
      .rhs = named_value(bir::TypeKind::I64, "%rhs"),
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };

  const auto result = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations, block, branch_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (result.record.has_value() ||
      result.error != aarch64_codegen::PreparedBranchRecordError::UnsupportedComparePredicate) {
    return fail("expected non-compare predicate to fail closed");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = materialized_bool_candidate_keeps_condition_and_targets_only();
      status != 0) {
    return status;
  }
  if (const int status = prepared_fused_and_non_fusable_compare_candidates_are_distinct();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_predicates_fail_closed_before_candidate_record();
      status != 0) {
    return status;
  }
  return 0;
}

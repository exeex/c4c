#include "src/backend/mir/aarch64/codegen/instruction.hpp"

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

bir::Terminator unconditional_terminator(c4c::BlockLabelId target_label) {
  return bir::BranchTerminator{
      .target_label = "exit",
      .target_label_id = target_label,
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

prepare::PreparedValueLocationFunction value_locations(
    c4c::FunctionNameId function_name,
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

int unconditional_branch_record_preserves_prepared_target_ids() {
  const auto function_name = c4c::FunctionNameId{3};
  const auto source_label = c4c::BlockLabelId{10};
  const auto target_label = c4c::BlockLabelId{11};
  const auto block = prepare::PreparedControlFlowBlock{
      .block_label = source_label,
      .terminator_kind = bir::TerminatorKind::Branch,
      .branch_target_label = target_label,
  };

  const auto result = aarch64_codegen::make_prepared_unconditional_branch_record(
      function_name, block, unconditional_terminator(target_label));
  if (!result.record.has_value() ||
      result.error != aarch64_codegen::PreparedBranchRecordError::None) {
    return fail("expected unconditional branch conversion to succeed");
  }

  const auto& record = *result.record;
  if (record.conditional || !record.condition_record.has_value() ||
      record.condition_record->form != aarch64_codegen::BranchConditionForm::Unconditional ||
      record.target.function_name != function_name || record.target.block_label != target_label ||
      record.target.condition_value_id.has_value()) {
    return fail("expected unconditional record to preserve only prepared target ids");
  }
  if (aarch64_codegen::prepared_branch_record_error_name(result.error) != "none") {
    return fail("expected successful prepared branch error name");
  }

  const auto missing_target = aarch64_codegen::make_prepared_unconditional_branch_record(
      function_name,
      prepare::PreparedControlFlowBlock{
          .block_label = source_label,
          .terminator_kind = bir::TerminatorKind::Branch,
      },
      unconditional_terminator(target_label));
  if (missing_target.record.has_value() ||
      missing_target.error != aarch64_codegen::PreparedBranchRecordError::MissingBranchTarget) {
    return fail("expected missing unconditional target fact to fail closed");
  }
  return 0;
}

int conditional_branch_record_preserves_materialized_bool_facts() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("f");
  const auto source_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("then");
  const auto false_label = names.block_labels.intern("else");
  const auto condition_name = names.value_names.intern("%cond");
  const auto locations =
      value_locations(function_name, condition_name, c4c::kInvalidValueName, c4c::kInvalidValueName);
  const auto condition_value = named_value(bir::TypeKind::I1, "%cond");
  const auto block = prepare::PreparedControlFlowBlock{
      .block_label = source_label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
  const auto branch_condition = prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = source_label,
      .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
      .condition_value = condition_value,
      .true_label = true_label,
      .false_label = false_label,
  };

  const auto result = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations, block, branch_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (!result.record.has_value()) {
    return fail("expected materialized-bool branch conversion to succeed");
  }

  const auto& record = *result.record;
  if (!record.conditional || !record.target_pair.has_value() ||
      !record.condition_record.has_value() ||
      record.condition_record->form != aarch64_codegen::BranchConditionForm::MaterializedBool ||
      record.condition_record->condition_value_id != prepare::PreparedValueId{40} ||
      record.condition_record->condition_value_name != condition_name ||
      record.condition_record->condition_type != bir::TypeKind::I1 ||
      record.condition_record->predicate.has_value() ||
      record.condition_record->compare_operands.has_value() ||
      !record.condition_record->compare_branch_candidate.has_value() ||
      record.condition_record->compare_branch_candidate->kind !=
          aarch64_codegen::BranchCompareCandidateKind::MaterializedBoolCondition) {
    return fail("expected materialized bool condition ids to be preserved");
  }
  if (record.condition_record->compare_branch_candidate->predicate.has_value() ||
      record.condition_record->compare_branch_candidate->compare_operands.has_value() ||
      !record.condition_record->compare_branch_candidate->target_pair.has_value()) {
    return fail("expected materialized bool candidate to keep compare facts absent");
  }
  if (record.target_pair->true_target.block_label != true_label ||
      record.target_pair->false_target.block_label != false_label ||
      record.target_pair->true_target.condition_value_id != prepare::PreparedValueId{40} ||
      record.target_pair->false_target.condition_value_id != prepare::PreparedValueId{40}) {
    return fail("expected conditional branch target pair to preserve labels and condition id");
  }
  return 0;
}

int conditional_branch_record_preserves_fused_compare_facts() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("f");
  const auto source_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("lt");
  const auto false_label = names.block_labels.intern("ge");
  const auto condition_name = names.value_names.intern("%cmp");
  const auto lhs_name = names.value_names.intern("%lhs");
  const auto rhs_name = names.value_names.intern("%rhs");
  const auto locations = value_locations(function_name, condition_name, lhs_name, rhs_name);
  const auto condition_value = named_value(bir::TypeKind::I1, "%cmp");
  const auto lhs_value = named_value(bir::TypeKind::I64, "%lhs");
  const auto rhs_value = named_value(bir::TypeKind::I64, "%rhs");
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
      .predicate = bir::BinaryOpcode::Slt,
      .compare_type = bir::TypeKind::I64,
      .lhs = lhs_value,
      .rhs = rhs_value,
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };

  const auto result = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations, block, branch_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (!result.record.has_value()) {
    return fail("expected fused-compare branch conversion to succeed");
  }

  const auto& condition = *result.record->condition_record;
  if (condition.form != aarch64_codegen::BranchConditionForm::FusedCompare ||
      !condition.can_fuse_with_branch || !condition.predicate.has_value() ||
      !condition.compare_operands.has_value() ||
      !condition.compare_branch_candidate.has_value() ||
      condition.compare_branch_candidate->kind !=
          aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch) {
    return fail("expected fused compare condition record");
  }
  if (!condition.compare_branch_candidate->target_pair.has_value() ||
      !condition.compare_branch_candidate->predicate.has_value() ||
      !condition.compare_branch_candidate->compare_operands.has_value()) {
    return fail("expected fused compare candidate to preserve target and compare facts");
  }
  if (condition.predicate->source_predicate != bir::BinaryOpcode::Slt ||
      condition.predicate->compare_type != bir::TypeKind::I64 ||
      condition.compare_operands->lhs.value_id != prepare::PreparedValueId{41} ||
      condition.compare_operands->rhs.value_id != prepare::PreparedValueId{42} ||
      condition.compare_operands->lhs.value_name != lhs_name ||
      condition.compare_operands->rhs.value_name != rhs_name ||
      condition.compare_operands->lhs.source_value != lhs_value ||
      condition.compare_operands->rhs.source_value != rhs_value) {
    return fail("expected fused compare predicate and operand facts to be preserved");
  }
  return 0;
}

int missing_required_facts_fail_closed() {
  prepare::PreparedNameTables names;
  const auto function_name = names.function_names.intern("f");
  const auto source_label = names.block_labels.intern("entry");
  const auto true_label = names.block_labels.intern("then");
  const auto false_label = names.block_labels.intern("else");
  const auto condition_name = names.value_names.intern("%cond");
  const auto condition_value = named_value(bir::TypeKind::I1, "%cond");
  const auto block = prepare::PreparedControlFlowBlock{
      .block_label = source_label,
      .terminator_kind = bir::TerminatorKind::CondBranch,
      .true_label = true_label,
      .false_label = false_label,
  };
  const auto locations_without_condition = prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{41},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
          },
  };
  const auto branch_condition = prepare::PreparedBranchCondition{
      .function_name = function_name,
      .block_label = source_label,
      .kind = prepare::PreparedBranchConditionKind::FusedCompare,
      .condition_value = condition_value,
      .predicate = bir::BinaryOpcode::Add,
      .compare_type = bir::TypeKind::I64,
      .lhs = named_value(bir::TypeKind::I64, "%missing"),
      .rhs = bir::Value::immediate_i64(0),
      .can_fuse_with_branch = true,
      .true_label = true_label,
      .false_label = false_label,
  };

  const auto unsupported = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations_without_condition, block, branch_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (unsupported.record.has_value() ||
      unsupported.error != aarch64_codegen::PreparedBranchRecordError::UnsupportedComparePredicate) {
    return fail("expected unsupported predicate to fail closed");
  }

  const auto mismatched_terminator = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations_without_condition, block, branch_condition,
      conditional_terminator(condition_value, false_label, true_label));
  if (mismatched_terminator.record.has_value() ||
      mismatched_terminator.error !=
          aarch64_codegen::PreparedBranchRecordError::TerminatorTargetMismatch) {
    return fail("expected mismatched structured target ids to fail closed");
  }

  const auto locations_missing_condition = prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {},
  };
  const auto missing_condition_home = aarch64_codegen::make_prepared_conditional_branch_record(
      names, locations_missing_condition, block, branch_condition,
      conditional_terminator(condition_value, true_label, false_label));
  if (missing_condition_home.record.has_value() ||
      missing_condition_home.error !=
          aarch64_codegen::PreparedBranchRecordError::MissingConditionValueHome) {
    return fail("expected missing condition value home to fail closed");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = unconditional_branch_record_preserves_prepared_target_ids();
      status != 0) {
    return status;
  }
  if (const int status = conditional_branch_record_preserves_materialized_bool_facts();
      status != 0) {
    return status;
  }
  if (const int status = conditional_branch_record_preserves_fused_compare_facts(); status != 0) {
    return status;
  }
  if (const int status = missing_required_facts_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}

#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <variant>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

bir::Value named_value(bir::TypeKind type, const char* name) {
  bir::Value value;
  value.kind = bir::Value::Kind::Named;
  value.type = type;
  value.name = name;
  return value;
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

int true_false_target_pairs_preserve_block_and_function_ids() {
  const auto pair = aarch64_codegen::BranchTargetPairRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .true_target = target(c4c::FunctionNameId{2},
                            c4c::BlockLabelId{10},
                            prepare::PreparedValueId{30}),
      .false_target = target(c4c::FunctionNameId{2},
                             c4c::BlockLabelId{11},
                             prepare::PreparedValueId{30}),
  };

  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target = pair.true_target,
          .target_pair = pair,
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::MaterializedBool,
                  .condition_value_id = prepare::PreparedValueId{30},
                  .condition_value_name = c4c::ValueNameId{7},
                  .condition_type = bir::TypeKind::I1,
              },
          .conditional = true,
      });

  const auto* payload = std::get_if<aarch64_codegen::BranchInstructionRecord>(&branch.payload);
  if (payload == nullptr || !payload->conditional || !payload->target_pair.has_value()) {
    return fail("expected conditional branch record with true/false target pair");
  }
  if (payload->target_pair->true_target.block_label != c4c::BlockLabelId{10} ||
      payload->target_pair->false_target.block_label != c4c::BlockLabelId{11} ||
      payload->target_pair->true_target.function_name != c4c::FunctionNameId{2} ||
      payload->target_pair->false_target.function_name != c4c::FunctionNameId{2}) {
    return fail("expected branch target pair to retain authoritative labels");
  }
  if (!payload->condition_record.has_value() ||
      payload->condition_record->form != aarch64_codegen::BranchConditionForm::MaterializedBool ||
      payload->condition_record->condition_value_id != prepare::PreparedValueId{30} ||
      payload->condition_record->condition_value_name != c4c::ValueNameId{7} ||
      payload->condition_record->condition_type != bir::TypeKind::I1 ||
      payload->condition_record->predicate.has_value() ||
      payload->condition_record->compare_operands.has_value()) {
    return fail("expected materialized-bool branch condition to retain value identity only");
  }
  if (branch.opcode != aarch64_codegen::MachineOpcode::ConditionalBranch ||
      branch.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      branch.operands.size() != 3 || branch.uses.size() != 3 ||
      branch.operands[2].kind != aarch64_codegen::OperandKind::PreparedValue ||
      branch.uses[2].value_id != prepare::PreparedValueId{30} ||
      branch.uses[2].value_name != c4c::ValueNameId{7}) {
    return fail("expected materialized-bool branch node to carry condition value operand");
  }
  if (aarch64_codegen::branch_condition_form_name(payload->condition_record->form) !=
      "materialized_bool") {
    return fail("expected materialized-bool form name");
  }
  return 0;
}

int fused_compare_conditions_preserve_predicate_type_and_operands() {
  const auto lhs = aarch64_codegen::CompareValueRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .value_id = prepare::PreparedValueId{41},
      .value_name = c4c::ValueNameId{12},
      .type = bir::TypeKind::I64,
      .source_value = named_value(bir::TypeKind::I64, "%lhs"),
  };
  const auto rhs = aarch64_codegen::CompareValueRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .value_id = prepare::PreparedValueId{42},
      .value_name = c4c::ValueNameId{13},
      .type = bir::TypeKind::I64,
      .source_value = named_value(bir::TypeKind::I64, "%rhs"),
  };
  const auto operands = aarch64_codegen::CompareOperandPairRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .lhs = lhs,
      .rhs = rhs,
      .compare_type = bir::TypeKind::I64,
  };
  const auto predicate = aarch64_codegen::ComparePredicateRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .source_predicate = bir::BinaryOpcode::Slt,
      .compare_type = bir::TypeKind::I64,
  };

  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target = target(c4c::FunctionNameId{3},
                           c4c::BlockLabelId{20},
                           prepare::PreparedValueId{40}),
          .target_pair =
              aarch64_codegen::BranchTargetPairRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .true_target = target(c4c::FunctionNameId{3},
                                        c4c::BlockLabelId{20},
                                        prepare::PreparedValueId{40}),
                  .false_target = target(c4c::FunctionNameId{3},
                                         c4c::BlockLabelId{21},
                                         prepare::PreparedValueId{40}),
              },
          .condition_record =
              aarch64_codegen::BranchConditionRecord{
                  .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
                  .form = aarch64_codegen::BranchConditionForm::FusedCompare,
                  .condition_value_id = prepare::PreparedValueId{40},
                  .condition_value_name = c4c::ValueNameId{11},
                  .condition_type = bir::TypeKind::I1,
                  .predicate = predicate,
                  .compare_operands = operands,
                  .can_fuse_with_branch = true,
              },
          .conditional = true,
      });

  const auto* payload = std::get_if<aarch64_codegen::BranchInstructionRecord>(&branch.payload);
  if (payload == nullptr || !payload->condition_record.has_value()) {
    return fail("expected fused compare branch condition record");
  }
  const auto& condition = *payload->condition_record;
  if (condition.form != aarch64_codegen::BranchConditionForm::FusedCompare ||
      aarch64_codegen::branch_condition_form_name(condition.form) != "fused_compare" ||
      !condition.can_fuse_with_branch || !condition.predicate.has_value() ||
      !condition.compare_operands.has_value()) {
    return fail("expected fused compare condition shape");
  }
  if (condition.predicate->source_predicate != bir::BinaryOpcode::Slt ||
      condition.predicate->compare_type != bir::TypeKind::I64 ||
      !aarch64_codegen::is_compare_predicate(condition.predicate->source_predicate) ||
      aarch64_codegen::is_compare_predicate(bir::BinaryOpcode::Add)) {
    return fail("expected BIR compare predicate to be preserved without opcode selection");
  }
  if (condition.compare_operands->lhs.value_id != prepare::PreparedValueId{41} ||
      condition.compare_operands->rhs.value_id != prepare::PreparedValueId{42} ||
      condition.compare_operands->lhs.value_name != c4c::ValueNameId{12} ||
      condition.compare_operands->rhs.value_name != c4c::ValueNameId{13} ||
      condition.compare_operands->compare_type != bir::TypeKind::I64 ||
      condition.compare_operands->lhs.source_value.name != "%lhs" ||
      condition.compare_operands->rhs.source_value.name != "%rhs") {
    return fail("expected compare operand pair to retain prepared and BIR value facts");
  }
  if (branch.opcode != aarch64_codegen::MachineOpcode::CompareBranch ||
      branch.selection.status != aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      branch.operands.size() != 5 || branch.uses.size() != 5 ||
      branch.operands[2].kind != aarch64_codegen::OperandKind::PreparedValue ||
      branch.operands[3].kind != aarch64_codegen::OperandKind::PreparedValue ||
      branch.operands[4].kind != aarch64_codegen::OperandKind::PreparedValue ||
      branch.uses[3].value_id != prepare::PreparedValueId{41} ||
      branch.uses[4].value_id != prepare::PreparedValueId{42}) {
    return fail("expected fused compare branch node to carry condition and compare operands");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = true_false_target_pairs_preserve_block_and_function_ids();
      status != 0) {
    return status;
  }
  if (const int status = fused_compare_conditions_preserve_predicate_type_and_operands();
      status != 0) {
    return status;
  }
  return 0;
}

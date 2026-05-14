#include "src/backend/mir/aarch64/codegen/instruction.hpp"

#include <iostream>
#include <string_view>
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

int branch_compare_records_remain_target_mir_record() {
  const auto target_pair = aarch64_codegen::BranchTargetPairRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .true_target = target(c4c::FunctionNameId{3},
                            c4c::BlockLabelId{20},
                            prepare::PreparedValueId{40}),
      .false_target = target(c4c::FunctionNameId{3},
                             c4c::BlockLabelId{21},
                             prepare::PreparedValueId{40}),
  };
  const auto predicate = aarch64_codegen::ComparePredicateRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .source_predicate = bir::BinaryOpcode::Sge,
      .compare_type = bir::TypeKind::I64,
  };
  const auto operands = aarch64_codegen::CompareOperandPairRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .lhs =
          aarch64_codegen::CompareValueRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .value_id = prepare::PreparedValueId{41},
              .value_name = c4c::ValueNameId{12},
              .type = bir::TypeKind::I64,
              .source_value = named_value(bir::TypeKind::I64, "%lhs"),
          },
      .rhs =
          aarch64_codegen::CompareValueRecord{
              .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
              .value_id = prepare::PreparedValueId{42},
              .value_name = c4c::ValueNameId{13},
              .type = bir::TypeKind::I64,
              .source_value = named_value(bir::TypeKind::I64, "%rhs"),
          },
      .compare_type = bir::TypeKind::I64,
  };
  const auto candidate = aarch64_codegen::BranchCompareCandidateRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .kind = aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch,
      .condition_value_id = prepare::PreparedValueId{40},
      .condition_value_name = c4c::ValueNameId{11},
      .condition_type = bir::TypeKind::I1,
      .predicate = predicate,
      .compare_operands = operands,
      .target_pair = target_pair,
      .can_fuse_with_branch = true,
  };
  const auto condition = aarch64_codegen::BranchConditionRecord{
      .surface = aarch64_codegen::RecordSurfaceKind::RecordOnly,
      .form = aarch64_codegen::BranchConditionForm::FusedCompare,
      .condition_value_id = prepare::PreparedValueId{40},
      .condition_value_name = c4c::ValueNameId{11},
      .condition_type = bir::TypeKind::I1,
      .predicate = predicate,
      .compare_operands = operands,
      .compare_branch_candidate = candidate,
      .can_fuse_with_branch = true,
  };
  const auto branch = aarch64_codegen::make_branch_instruction(
      aarch64_codegen::BranchInstructionRecord{
          .target = target_pair.true_target,
          .target_pair = target_pair,
          .condition_record = condition,
          .conditional = true,
      });

  const auto* branch_payload =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&branch.payload);
  if (branch.family != aarch64_codegen::InstructionFamily::Branch ||
      branch.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      branch_payload == nullptr || !branch_payload->target_pair.has_value() ||
      !branch_payload->condition_record.has_value()) {
    return fail("expected branch compare instruction to be a machine-node branch payload");
  }

  const auto& recorded_condition = *branch_payload->condition_record;
  if (recorded_condition.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      recorded_condition.form != aarch64_codegen::BranchConditionForm::FusedCompare ||
      !recorded_condition.predicate.has_value() ||
      !recorded_condition.compare_operands.has_value() ||
      !recorded_condition.compare_branch_candidate.has_value()) {
    return fail("expected fused compare condition to keep structured target MIR metadata");
  }

  const auto& recorded_candidate = *recorded_condition.compare_branch_candidate;
  if (recorded_candidate.surface != aarch64_codegen::RecordSurfaceKind::RecordOnly ||
      recorded_candidate.kind !=
          aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch ||
      !recorded_candidate.can_fuse_with_branch ||
      recorded_candidate.condition_value_id != prepare::PreparedValueId{40} ||
      recorded_candidate.condition_value_name != c4c::ValueNameId{11} ||
      recorded_candidate.condition_type != bir::TypeKind::I1 ||
      !recorded_candidate.predicate.has_value() ||
      !recorded_candidate.compare_operands.has_value() ||
      !recorded_candidate.target_pair.has_value()) {
    return fail("expected candidate metadata to preserve condition and fusion facts");
  }

  if (recorded_candidate.predicate->source_predicate != bir::BinaryOpcode::Sge ||
      recorded_candidate.predicate->compare_type != bir::TypeKind::I64 ||
      recorded_candidate.compare_operands->lhs.value_id != prepare::PreparedValueId{41} ||
      recorded_candidate.compare_operands->rhs.value_id != prepare::PreparedValueId{42} ||
      recorded_candidate.compare_operands->lhs.value_name != c4c::ValueNameId{12} ||
      recorded_candidate.compare_operands->rhs.value_name != c4c::ValueNameId{13} ||
      recorded_candidate.compare_operands->lhs.source_value.name != "%lhs" ||
      recorded_candidate.compare_operands->rhs.source_value.name != "%rhs" ||
      recorded_candidate.target_pair->true_target.block_label != c4c::BlockLabelId{20} ||
      recorded_candidate.target_pair->false_target.block_label != c4c::BlockLabelId{21}) {
    return fail("expected branch compare records to retain structured ids and BIR facts");
  }

  return 0;
}

int diagnostic_names_do_not_claim_lowering_ownership() {
  if (aarch64_codegen::record_surface_kind_name(aarch64_codegen::RecordSurfaceKind::RecordOnly) !=
      std::string_view{"target_mir_record"}) {
    return fail("expected branch compare surfaces to advertise target MIR ownership");
  }
  if (aarch64_codegen::instruction_family_name(aarch64_codegen::InstructionFamily::Branch) !=
      std::string_view{"branch"}) {
    return fail("expected branch family name to remain generic");
  }
  if (aarch64_codegen::branch_condition_form_name(
          aarch64_codegen::BranchConditionForm::FusedCompare) !=
      std::string_view{"fused_compare"}) {
    return fail("expected condition form to describe source shape, not an opcode");
  }
  if (aarch64_codegen::branch_compare_candidate_kind_name(
          aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch) !=
      std::string_view{"fused_compare_and_branch"}) {
    return fail("expected candidate kind to describe metadata, not an opcode");
  }
  if (aarch64_codegen::branch_compare_candidate_kind_name(
          aarch64_codegen::BranchCompareCandidateKind::NonFusableCompare) !=
      std::string_view{"non_fusable_compare"}) {
    return fail("expected non-fusable compare facts to stay explicit");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = branch_compare_records_remain_target_mir_record(); status != 0) {
    return status;
  }
  if (const int status = diagnostic_names_do_not_claim_lowering_ownership(); status != 0) {
    return status;
  }
  return 0;
}

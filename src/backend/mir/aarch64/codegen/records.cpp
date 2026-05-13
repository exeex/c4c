#include "records.hpp"

namespace c4c::backend::aarch64::codegen {

std::string_view operand_kind_name(OperandKind kind) {
  switch (kind) {
    case OperandKind::Register:
      return "register";
    case OperandKind::Immediate:
      return "immediate";
    case OperandKind::PreparedValue:
      return "prepared_value";
    case OperandKind::FrameSlot:
      return "frame_slot";
    case OperandKind::Symbol:
      return "symbol";
    case OperandKind::BranchTarget:
      return "branch_target";
    case OperandKind::Memory:
      return "memory";
  }
  return "unknown";
}

std::string_view record_surface_kind_name(RecordSurfaceKind kind) {
  switch (kind) {
    case RecordSurfaceKind::RecordOnly:
      return "record_only";
  }
  return "unknown";
}

std::string_view register_operand_role_name(RegisterOperandRole role) {
  switch (role) {
    case RegisterOperandRole::Physical:
      return "physical";
    case RegisterOperandRole::PreparedAssignment:
      return "prepared_assignment";
    case RegisterOperandRole::ValueHome:
      return "value_home";
    case RegisterOperandRole::SpillAuthority:
      return "spill_authority";
    case RegisterOperandRole::StoragePlan:
      return "storage_plan";
    case RegisterOperandRole::CallAbi:
      return "call_abi";
  }
  return "unknown";
}

std::string_view immediate_kind_name(ImmediateKind kind) {
  switch (kind) {
    case ImmediateKind::SignedInteger:
      return "signed_integer";
    case ImmediateKind::UnsignedInteger:
      return "unsigned_integer";
    case ImmediateKind::Boolean:
      return "boolean";
    case ImmediateKind::NullPointer:
      return "null_pointer";
  }
  return "unknown";
}

std::string_view memory_base_kind_name(MemoryBaseKind kind) {
  switch (kind) {
    case MemoryBaseKind::None:
      return "none";
    case MemoryBaseKind::FrameSlot:
      return "frame_slot";
    case MemoryBaseKind::Symbol:
      return "symbol";
    case MemoryBaseKind::PointerValue:
      return "pointer_value";
    case MemoryBaseKind::StringConstant:
      return "string_constant";
    case MemoryBaseKind::Register:
      return "register";
  }
  return "unknown";
}

std::string_view instruction_family_name(InstructionFamily family) {
  switch (family) {
    case InstructionFamily::Branch:
      return "branch";
    case InstructionFamily::Scalar:
      return "scalar";
    case InstructionFamily::Memory:
      return "memory";
    case InstructionFamily::Call:
      return "call";
    case InstructionFamily::Return:
      return "return";
    case InstructionFamily::Assembler:
      return "assembler";
    case InstructionFamily::Object:
      return "object";
  }
  return "unknown";
}

std::string_view memory_instruction_kind_name(MemoryInstructionKind kind) {
  switch (kind) {
    case MemoryInstructionKind::Load:
      return "load";
    case MemoryInstructionKind::Store:
      return "store";
  }
  return "unknown";
}

std::string_view branch_condition_form_name(BranchConditionForm form) {
  switch (form) {
    case BranchConditionForm::Unconditional:
      return "unconditional";
    case BranchConditionForm::MaterializedBool:
      return "materialized_bool";
    case BranchConditionForm::FusedCompare:
      return "fused_compare";
  }
  return "unknown";
}

std::string_view branch_compare_candidate_kind_name(BranchCompareCandidateKind kind) {
  switch (kind) {
    case BranchCompareCandidateKind::None:
      return "none";
    case BranchCompareCandidateKind::MaterializedBoolCondition:
      return "materialized_bool_condition";
    case BranchCompareCandidateKind::FusedCompareAndBranch:
      return "fused_compare_and_branch";
    case BranchCompareCandidateKind::NonFusableCompare:
      return "non_fusable_compare";
  }
  return "unknown";
}

std::string_view prepared_branch_record_error_name(PreparedBranchRecordError error) {
  switch (error) {
    case PreparedBranchRecordError::None:
      return "none";
    case PreparedBranchRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedBranchRecordError::InvalidSourceBlock:
      return "invalid_source_block";
    case PreparedBranchRecordError::TerminatorKindMismatch:
      return "terminator_kind_mismatch";
    case PreparedBranchRecordError::MissingBranchTarget:
      return "missing_branch_target";
    case PreparedBranchRecordError::TerminatorTargetMismatch:
      return "terminator_target_mismatch";
    case PreparedBranchRecordError::MissingBranchCondition:
      return "missing_branch_condition";
    case PreparedBranchRecordError::ConditionValueMismatch:
      return "condition_value_mismatch";
    case PreparedBranchRecordError::MissingConditionValueHome:
      return "missing_condition_value_home";
    case PreparedBranchRecordError::MissingCompareFacts:
      return "missing_compare_facts";
    case PreparedBranchRecordError::UnsupportedComparePredicate:
      return "unsupported_compare_predicate";
    case PreparedBranchRecordError::MissingCompareValueHome:
      return "missing_compare_value_home";
  }
  return "unknown";
}

bool is_compare_predicate(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Eq:
    case c4c::backend::bir::BinaryOpcode::Ne:
    case c4c::backend::bir::BinaryOpcode::Slt:
    case c4c::backend::bir::BinaryOpcode::Sle:
    case c4c::backend::bir::BinaryOpcode::Sgt:
    case c4c::backend::bir::BinaryOpcode::Sge:
    case c4c::backend::bir::BinaryOpcode::Ult:
    case c4c::backend::bir::BinaryOpcode::Ule:
    case c4c::backend::bir::BinaryOpcode::Ugt:
    case c4c::backend::bir::BinaryOpcode::Uge:
      return true;
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
      return false;
  }
  return false;
}

namespace {

PreparedBranchInstructionRecordResult branch_record_error(PreparedBranchRecordError error) {
  return PreparedBranchInstructionRecordResult{.record = std::nullopt, .error = error};
}

BranchTargetOperand make_prepared_branch_target(c4c::FunctionNameId function_name,
                                                c4c::BlockLabelId block_label,
                                                std::optional<c4c::backend::prepare::PreparedValueId>
                                                    condition_value_id = std::nullopt) {
  return BranchTargetOperand{
      .surface = RecordSurfaceKind::RecordOnly,
      .block_label = block_label,
      .function_name = function_name,
      .condition_value_id = condition_value_id,
  };
}

bool same_bir_value(const c4c::backend::bir::Value& lhs,
                    const c4c::backend::bir::Value& rhs) {
  return lhs == rhs;
}

const c4c::backend::prepare::PreparedValueHome* find_named_value_home(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::bir::Value& value) {
  if (value.kind != c4c::backend::bir::Value::Kind::Named) {
    return nullptr;
  }
  return c4c::backend::prepare::find_prepared_value_home(names, value_locations, value.name);
}

std::optional<CompareValueRecord> make_compare_value_record(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::bir::Value& value) {
  CompareValueRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .value_id = std::nullopt,
      .value_name = c4c::kInvalidValueName,
      .type = value.type,
      .source_value = value,
  };
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    return record;
  }

  const auto* home = find_named_value_home(names, value_locations, value);
  if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  record.value_id = home->value_id;
  record.value_name = home->value_name;
  return record;
}

}  // namespace

OperandRecord make_register_operand(RegisterOperand operand) {
  return OperandRecord{.kind = OperandKind::Register, .payload = operand};
}

OperandRecord make_immediate_operand(ImmediateOperand operand) {
  return OperandRecord{.kind = OperandKind::Immediate, .payload = operand};
}

OperandRecord make_prepared_value_operand(PreparedValueOperand operand) {
  return OperandRecord{.kind = OperandKind::PreparedValue, .payload = operand};
}

OperandRecord make_frame_slot_operand(FrameSlotOperand operand) {
  return OperandRecord{.kind = OperandKind::FrameSlot, .payload = operand};
}

OperandRecord make_symbol_operand(SymbolOperand operand) {
  return OperandRecord{.kind = OperandKind::Symbol, .payload = operand};
}

OperandRecord make_branch_target_operand(BranchTargetOperand operand) {
  return OperandRecord{.kind = OperandKind::BranchTarget, .payload = operand};
}

OperandRecord make_memory_operand(MemoryOperand operand) {
  return OperandRecord{.kind = OperandKind::Memory, .payload = operand};
}

InstructionRecord make_branch_instruction(BranchInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Branch,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Scalar,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_return_instruction(ReturnInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Return,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

InstructionRecord make_object_instruction(ObjectInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Object,
      .surface = RecordSurfaceKind::RecordOnly,
      .payload = instruction,
  };
}

PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedControlFlowBlock& block,
    const c4c::backend::bir::Terminator& terminator) {
  if (function_name == c4c::kInvalidFunctionName) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != c4c::backend::bir::TerminatorKind::Branch ||
      terminator.kind != c4c::backend::bir::TerminatorKind::Branch) {
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
              .target = make_prepared_branch_target(function_name, block.branch_target_label),
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

PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::prepare::PreparedControlFlowBlock& block,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::bir::Terminator& terminator) {
  if (branch_condition.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != branch_condition.function_name) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel ||
      branch_condition.block_label != block.block_label) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != c4c::backend::bir::TerminatorKind::CondBranch ||
      terminator.kind != c4c::backend::bir::TerminatorKind::CondBranch) {
    return branch_record_error(PreparedBranchRecordError::TerminatorKindMismatch);
  }
  if (block.true_label == c4c::kInvalidBlockLabel ||
      block.false_label == c4c::kInvalidBlockLabel ||
      branch_condition.true_label == c4c::kInvalidBlockLabel ||
      branch_condition.false_label == c4c::kInvalidBlockLabel ||
      terminator.true_label_id == c4c::kInvalidBlockLabel ||
      terminator.false_label_id == c4c::kInvalidBlockLabel) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchTarget);
  }
  if (block.true_label != branch_condition.true_label ||
      block.false_label != branch_condition.false_label ||
      terminator.true_label_id != branch_condition.true_label ||
      terminator.false_label_id != branch_condition.false_label) {
    return branch_record_error(PreparedBranchRecordError::TerminatorTargetMismatch);
  }
  if (branch_condition.condition_value.type != c4c::backend::bir::TypeKind::I1 ||
      terminator.condition.type != c4c::backend::bir::TypeKind::I1) {
    return branch_record_error(PreparedBranchRecordError::MissingBranchCondition);
  }
  if (!same_bir_value(branch_condition.condition_value, terminator.condition)) {
    return branch_record_error(PreparedBranchRecordError::ConditionValueMismatch);
  }

  const auto* condition_home =
      find_named_value_home(names, value_locations, branch_condition.condition_value);
  if (condition_home == nullptr || condition_home->value_name == c4c::kInvalidValueName) {
    return branch_record_error(PreparedBranchRecordError::MissingConditionValueHome);
  }

  BranchConditionRecord condition{
      .surface = RecordSurfaceKind::RecordOnly,
      .form = branch_condition.kind ==
                      c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare
                  ? BranchConditionForm::FusedCompare
                  : BranchConditionForm::MaterializedBool,
      .condition_value_id = condition_home->value_id,
      .condition_value_name = condition_home->value_name,
      .condition_type = branch_condition.condition_value.type,
      .can_fuse_with_branch = branch_condition.can_fuse_with_branch,
  };

  if (branch_condition.kind == c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare) {
    if (!branch_condition.predicate.has_value() || !branch_condition.compare_type.has_value() ||
        !branch_condition.lhs.has_value() || !branch_condition.rhs.has_value()) {
      return branch_record_error(PreparedBranchRecordError::MissingCompareFacts);
    }
    if (!is_compare_predicate(*branch_condition.predicate)) {
      return branch_record_error(PreparedBranchRecordError::UnsupportedComparePredicate);
    }

    const auto lhs =
        make_compare_value_record(names, value_locations, *branch_condition.lhs);
    const auto rhs =
        make_compare_value_record(names, value_locations, *branch_condition.rhs);
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
      .true_target = make_prepared_branch_target(
          branch_condition.function_name, branch_condition.true_label, condition_value_id),
      .false_target = make_prepared_branch_target(
          branch_condition.function_name, branch_condition.false_label, condition_value_id),
  };
  condition.compare_branch_candidate = BranchCompareCandidateRecord{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind =
          branch_condition.kind == c4c::backend::prepare::PreparedBranchConditionKind::FusedCompare
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

}  // namespace c4c::backend::aarch64::codegen

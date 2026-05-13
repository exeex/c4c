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

std::string_view scalar_alu_operation_kind_name(ScalarAluOperationKind kind) {
  switch (kind) {
    case ScalarAluOperationKind::Add:
      return "add";
    case ScalarAluOperationKind::Sub:
      return "sub";
    case ScalarAluOperationKind::And:
      return "and";
    case ScalarAluOperationKind::Or:
      return "or";
    case ScalarAluOperationKind::Xor:
      return "xor";
    case ScalarAluOperationKind::Deferred:
      return "deferred";
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

std::string_view prepared_scalar_alu_record_error_name(PreparedScalarAluRecordError error) {
  switch (error) {
    case PreparedScalarAluRecordError::None:
      return "none";
    case PreparedScalarAluRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedScalarAluRecordError::UnsupportedOpcode:
      return "unsupported_opcode";
    case PreparedScalarAluRecordError::UnsupportedResultValue:
      return "unsupported_result_value";
    case PreparedScalarAluRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedScalarAluRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
      return "unsupported_operand_value";
    case PreparedScalarAluRecordError::MissingOperandValueHome:
      return "missing_operand_value_home";
    case PreparedScalarAluRecordError::MissingOperandStorage:
      return "missing_operand_storage";
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
      return "unsupported_operand_storage";
    case PreparedScalarAluRecordError::UnsupportedOperandType:
      return "unsupported_operand_type";
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
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

bool is_scalar_alu_integer_opcode(c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
    case c4c::backend::bir::BinaryOpcode::Sub:
    case c4c::backend::bir::BinaryOpcode::And:
    case c4c::backend::bir::BinaryOpcode::Or:
    case c4c::backend::bir::BinaryOpcode::Xor:
      return true;
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
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
      return false;
  }
  return false;
}

ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    c4c::backend::bir::BinaryOpcode opcode) {
  switch (opcode) {
    case c4c::backend::bir::BinaryOpcode::Add:
      return ScalarAluOperationKind::Add;
    case c4c::backend::bir::BinaryOpcode::Sub:
      return ScalarAluOperationKind::Sub;
    case c4c::backend::bir::BinaryOpcode::And:
      return ScalarAluOperationKind::And;
    case c4c::backend::bir::BinaryOpcode::Or:
      return ScalarAluOperationKind::Or;
    case c4c::backend::bir::BinaryOpcode::Xor:
      return ScalarAluOperationKind::Xor;
    case c4c::backend::bir::BinaryOpcode::Mul:
    case c4c::backend::bir::BinaryOpcode::Shl:
    case c4c::backend::bir::BinaryOpcode::LShr:
    case c4c::backend::bir::BinaryOpcode::AShr:
    case c4c::backend::bir::BinaryOpcode::SDiv:
    case c4c::backend::bir::BinaryOpcode::UDiv:
    case c4c::backend::bir::BinaryOpcode::SRem:
    case c4c::backend::bir::BinaryOpcode::URem:
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
      return ScalarAluOperationKind::Deferred;
  }
  return ScalarAluOperationKind::Deferred;
}

namespace {

PreparedBranchInstructionRecordResult branch_record_error(PreparedBranchRecordError error) {
  return PreparedBranchInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarAluRecordResult scalar_alu_record_error(PreparedScalarAluRecordError error) {
  return PreparedScalarAluRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarInstructionRecordResult scalar_instruction_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarInstructionRecordResult{.record = std::nullopt, .error = error};
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

const c4c::backend::prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const c4c::backend::prepare::PreparedStoragePlanFunction& storage_plan,
    c4c::backend::prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

std::optional<c4c::backend::aarch64::abi::RegisterView> scalar_register_view(
    c4c::backend::bir::TypeKind type) {
  switch (type) {
    case c4c::backend::bir::TypeKind::I1:
    case c4c::backend::bir::TypeKind::I8:
    case c4c::backend::bir::TypeKind::I16:
    case c4c::backend::bir::TypeKind::I32:
      return c4c::backend::aarch64::abi::RegisterView::W;
    case c4c::backend::bir::TypeKind::I64:
    case c4c::backend::bir::TypeKind::Ptr:
      return c4c::backend::aarch64::abi::RegisterView::X;
    case c4c::backend::bir::TypeKind::Void:
    case c4c::backend::bir::TypeKind::I128:
    case c4c::backend::bir::TypeKind::F32:
    case c4c::backend::bir::TypeKind::F64:
    case c4c::backend::bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

c4c::backend::prepare::PreparedRegisterClass register_class_from_bank(
    c4c::backend::prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case c4c::backend::prepare::PreparedRegisterBank::Gpr:
      return c4c::backend::prepare::PreparedRegisterClass::General;
    case c4c::backend::prepare::PreparedRegisterBank::Fpr:
      return c4c::backend::prepare::PreparedRegisterClass::Float;
    case c4c::backend::prepare::PreparedRegisterBank::Vreg:
      return c4c::backend::prepare::PreparedRegisterClass::Vector;
    case c4c::backend::prepare::PreparedRegisterBank::AggregateAddress:
      return c4c::backend::prepare::PreparedRegisterClass::AggregateAddress;
    case c4c::backend::prepare::PreparedRegisterBank::None:
      return c4c::backend::prepare::PreparedRegisterClass::None;
  }
  return c4c::backend::prepare::PreparedRegisterClass::None;
}

std::optional<ImmediateOperand> make_scalar_immediate_operand(
    const c4c::backend::bir::Value& value,
    std::optional<c4c::backend::prepare::PreparedValueId> source_value_id = std::nullopt,
    c4c::ValueNameId source_value_name = c4c::kInvalidValueName) {
  if (value.kind != c4c::backend::bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  ImmediateKind kind = ImmediateKind::SignedInteger;
  if (value.type == c4c::backend::bir::TypeKind::I1) {
    kind = ImmediateKind::Boolean;
  } else if (!scalar_register_view(value.type).has_value()) {
    return std::nullopt;
  }

  return ImmediateOperand{
      .kind = kind,
      .type = value.type,
      .signed_value = value.immediate,
      .unsigned_value = value.immediate_bits != 0U ? value.immediate_bits
                                                   : static_cast<std::uint64_t>(value.immediate),
      .source_value_id = source_value_id,
      .source_value_name = source_value_name,
  };
}

PreparedScalarAluRecordError make_prepared_scalar_operand(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::prepare::PreparedStoragePlanFunction& storage_plan,
    const c4c::backend::bir::Value& value,
    OperandRecord& out) {
  if (value.kind == c4c::backend::bir::Value::Kind::Immediate) {
    const auto immediate = make_scalar_immediate_operand(value);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }
  if (value.kind != c4c::backend::bir::Value::Kind::Named || value.name.empty()) {
    return PreparedScalarAluRecordError::UnsupportedOperandValue;
  }

  const auto* home = find_named_value_home(names, value_locations, value);
  if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
    return PreparedScalarAluRecordError::MissingOperandValueHome;
  }
  const auto* storage = find_storage_plan_value(storage_plan, home->value_id);
  if (storage == nullptr || storage->value_name != home->value_name) {
    return PreparedScalarAluRecordError::MissingOperandStorage;
  }

  if (storage->encoding == c4c::backend::prepare::PreparedStorageEncodingKind::Immediate) {
    if (home->kind != c4c::backend::prepare::PreparedValueHomeKind::RematerializableImmediate ||
        !home->immediate_i32.has_value() || !storage->immediate_i32.has_value() ||
        *home->immediate_i32 != *storage->immediate_i32) {
      return PreparedScalarAluRecordError::UnsupportedOperandStorage;
    }
    const auto immediate = make_scalar_immediate_operand(
        c4c::backend::bir::Value::immediate_i32(static_cast<std::int32_t>(*storage->immediate_i32)),
        home->value_id,
        home->value_name);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }

  if (home->kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      storage->encoding != c4c::backend::prepare::PreparedStorageEncodingKind::Register ||
      !home->register_name.has_value() || !storage->register_name.has_value() ||
      *home->register_name != *storage->register_name) {
    return PreparedScalarAluRecordError::UnsupportedOperandStorage;
  }

  const auto expected_view = scalar_register_view(value.type);
  if (!expected_view.has_value()) {
    return PreparedScalarAluRecordError::UnsupportedOperandType;
  }
  const auto prepared_class = register_class_from_bank(storage->bank);
  const auto converted = c4c::backend::aarch64::abi::convert_prepared_register(
      *storage->register_name, storage->bank, prepared_class, expected_view);
  if (!converted.has_value()) {
    return PreparedScalarAluRecordError::RegisterConversionFailed;
  }

  out = make_register_operand(RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = home->value_id,
      .value_name = home->value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage->bank,
      .expected_view = expected_view,
      .contiguous_width = storage->contiguous_width,
  });
  return PreparedScalarAluRecordError::None;
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

ScalarInstructionRecord make_scalar_alu_instruction_record(ScalarAluRecord alu) {
  return ScalarInstructionRecord{
      .result_value_id = alu.result_value_id,
      .result_value_name = alu.result_value_name,
      .result_type = alu.result_type,
      .inputs = {alu.lhs, alu.rhs},
      .source_binary_opcode = alu.source_binary_opcode,
      .scalar_alu = alu,
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

PreparedScalarAluRecordResult make_prepared_scalar_alu_record(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::prepare::PreparedStoragePlanFunction& storage_plan,
    const c4c::backend::bir::BinaryInst& binary) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::InvalidFunction);
  }
  if (!is_scalar_alu_integer_opcode(binary.opcode)) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != c4c::backend::bir::Value::Kind::Named || binary.result.name.empty()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedResultValue);
  }
  if (!scalar_register_view(binary.result.type).has_value() ||
      !scalar_register_view(binary.operand_type).has_value()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOperandType);
  }

  const auto* result_home = find_named_value_home(names, value_locations, binary.result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::MissingResultStorage);
  }
  if (result_home->kind != c4c::backend::prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != c4c::backend::prepare::PreparedStorageEncodingKind::Register ||
      !result_home->register_name.has_value() || !result_storage->register_name.has_value() ||
      *result_home->register_name != *result_storage->register_name) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedResultStorage);
  }

  OperandRecord lhs;
  OperandRecord rhs;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.lhs, lhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_alu_record_error(error);
  }
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.rhs, rhs);
      error != PreparedScalarAluRecordError::None) {
    return scalar_alu_record_error(error);
  }

  return PreparedScalarAluRecordResult{
      .record =
          ScalarAluRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = scalar_alu_operation_from_binary_opcode(binary.opcode),
              .source_binary_opcode = binary.opcode,
              .operand_type = binary.operand_type,
              .result_value_id = result_home->value_id,
              .result_value_name = result_home->value_name,
              .result_type = binary.result.type,
              .lhs = lhs,
              .rhs = rhs,
              .supported_integer_operation = true,
          },
      .error = PreparedScalarAluRecordError::None,
  };
}

PreparedScalarInstructionRecordResult make_prepared_scalar_alu_instruction_record(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::prepare::PreparedStoragePlanFunction& storage_plan,
    const c4c::backend::bir::BinaryInst& binary) {
  const auto result = make_prepared_scalar_alu_record(names, value_locations, storage_plan, binary);
  if (!result.record.has_value()) {
    return scalar_instruction_record_error(result.error);
  }
  return PreparedScalarInstructionRecordResult{
      .record = make_scalar_alu_instruction_record(*result.record),
      .error = PreparedScalarAluRecordError::None,
  };
}

}  // namespace c4c::backend::aarch64::codegen

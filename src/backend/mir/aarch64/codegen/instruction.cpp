#include "instruction.hpp"

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

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
    case RecordSurfaceKind::TargetMirRecord:
      return "target_mir_record";
    case RecordSurfaceKind::MachineInstructionNode:
      return "machine_instruction_node";
    case RecordSurfaceKind::PrinterOutput:
      return "printer_output";
    case RecordSurfaceKind::EncoderInput:
      return "encoder_input";
    case RecordSurfaceKind::ExternalAssemblerInput:
      return "external_assembler_input";
  }
  return "unknown";
}

bool is_target_mir_record_surface(RecordSurfaceKind kind) {
  return kind == RecordSurfaceKind::TargetMirRecord;
}

bool is_structured_downstream_surface(RecordSurfaceKind kind) {
  return kind == RecordSurfaceKind::MachineInstructionNode ||
         kind == RecordSurfaceKind::EncoderInput;
}

bool is_text_first_external_input_surface(RecordSurfaceKind kind) {
  return kind == RecordSurfaceKind::ExternalAssemblerInput;
}

std::string_view register_operand_role_name(RegisterOperandRole role) {
  switch (role) {
    case RegisterOperandRole::Physical:
      return "physical";
    case RegisterOperandRole::PreparedAssignment:
      return "prepared_assignment";
    case RegisterOperandRole::AllocationResult:
      return "allocation_result";
    case RegisterOperandRole::ReservedMirScratch:
      return "reserved_mir_scratch";
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

std::string_view memory_operand_support_kind_name(MemoryOperandSupportKind kind) {
  switch (kind) {
    case MemoryOperandSupportKind::Prepared:
      return "prepared";
    case MemoryOperandSupportKind::DeferredUnsupported:
      return "deferred_unsupported";
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
    case InstructionFamily::Frame:
      return "frame";
    case InstructionFamily::Call:
      return "call";
    case InstructionFamily::CallBoundary:
      return "call_boundary";
    case InstructionFamily::Return:
      return "return";
    case InstructionFamily::Assembler:
      return "assembler";
    case InstructionFamily::Object:
      return "object";
  }
  return "unknown";
}

std::string_view machine_opcode_name(MachineOpcode opcode) {
  switch (opcode) {
    case MachineOpcode::Unspecified:
      return "unspecified";
    case MachineOpcode::Branch:
      return "branch";
    case MachineOpcode::ConditionalBranch:
      return "conditional_branch";
    case MachineOpcode::CompareBranch:
      return "compare_branch";
    case MachineOpcode::DirectCall:
      return "direct_call";
    case MachineOpcode::IndirectCall:
      return "indirect_call";
    case MachineOpcode::FrameSetup:
      return "frame_setup";
    case MachineOpcode::FrameTeardown:
      return "frame_teardown";
    case MachineOpcode::CalleeSaveStore:
      return "callee_save_store";
    case MachineOpcode::CalleeSaveLoad:
      return "callee_save_load";
    case MachineOpcode::CallBoundaryMove:
      return "call_boundary_move";
    case MachineOpcode::CallBoundaryAbiBinding:
      return "call_boundary_abi_binding";
    case MachineOpcode::Add:
      return "add";
    case MachineOpcode::Sub:
      return "sub";
    case MachineOpcode::And:
      return "and";
    case MachineOpcode::Or:
      return "or";
    case MachineOpcode::Xor:
      return "xor";
    case MachineOpcode::SignExtend:
      return "sign_extend";
    case MachineOpcode::ZeroExtend:
      return "zero_extend";
    case MachineOpcode::Truncate:
      return "truncate";
    case MachineOpcode::Load:
      return "load";
    case MachineOpcode::Store:
      return "store";
    case MachineOpcode::SpillToSlot:
      return "spill_to_slot";
    case MachineOpcode::ReloadFromSlot:
      return "reload_from_slot";
  }
  return "unknown";
}

std::string_view machine_pseudo_kind_name(MachinePseudoKind pseudo) {
  switch (pseudo) {
    case MachinePseudoKind::None:
      return "none";
    case MachinePseudoKind::SpillToSlot:
      return "spill_to_slot";
    case MachinePseudoKind::ReloadFromSlot:
      return "reload_from_slot";
  }
  return "unknown";
}

std::string_view machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind kind) {
  switch (kind) {
    case MachinePrinterMnemonicKind::None:
      return "";
    case MachinePrinterMnemonicKind::Branch:
      return "b";
    case MachinePrinterMnemonicKind::ConditionalBranchNonZero:
      return "cbnz";
    case MachinePrinterMnemonicKind::DirectCall:
      return "bl";
    case MachinePrinterMnemonicKind::IndirectCall:
      return "blr";
    case MachinePrinterMnemonicKind::Add:
      return "add";
    case MachinePrinterMnemonicKind::Sub:
      return "sub";
    case MachinePrinterMnemonicKind::Load:
      return "ldr";
    case MachinePrinterMnemonicKind::Store:
      return "str";
    case MachinePrinterMnemonicKind::Move:
      return "mov";
    case MachinePrinterMnemonicKind::Return:
      return "ret";
  }
  return "";
}

MachinePrinterMnemonicKind machine_opcode_printer_mnemonic_kind(MachineOpcode opcode) {
  switch (opcode) {
    case MachineOpcode::Branch:
      return MachinePrinterMnemonicKind::Branch;
    case MachineOpcode::ConditionalBranch:
      return MachinePrinterMnemonicKind::ConditionalBranchNonZero;
    case MachineOpcode::DirectCall:
      return MachinePrinterMnemonicKind::DirectCall;
    case MachineOpcode::IndirectCall:
      return MachinePrinterMnemonicKind::IndirectCall;
    case MachineOpcode::FrameSetup:
      return MachinePrinterMnemonicKind::Sub;
    case MachineOpcode::FrameTeardown:
      return MachinePrinterMnemonicKind::Add;
    case MachineOpcode::Add:
      return MachinePrinterMnemonicKind::Add;
    case MachineOpcode::Sub:
      return MachinePrinterMnemonicKind::Sub;
    case MachineOpcode::Load:
    case MachineOpcode::ReloadFromSlot:
      return MachinePrinterMnemonicKind::Load;
    case MachineOpcode::Store:
    case MachineOpcode::SpillToSlot:
      return MachinePrinterMnemonicKind::Store;
    case MachineOpcode::CallBoundaryMove:
      return MachinePrinterMnemonicKind::Move;
    case MachineOpcode::Unspecified:
    case MachineOpcode::CompareBranch:
    case MachineOpcode::CalleeSaveStore:
    case MachineOpcode::CalleeSaveLoad:
    case MachineOpcode::CallBoundaryAbiBinding:
    case MachineOpcode::And:
    case MachineOpcode::Or:
    case MachineOpcode::Xor:
    case MachineOpcode::SignExtend:
    case MachineOpcode::ZeroExtend:
    case MachineOpcode::Truncate:
      return MachinePrinterMnemonicKind::None;
  }
  return MachinePrinterMnemonicKind::None;
}

MachinePrinterMnemonicKind machine_pseudo_printer_mnemonic_kind(MachinePseudoKind pseudo) {
  switch (pseudo) {
    case MachinePseudoKind::SpillToSlot:
      return MachinePrinterMnemonicKind::Store;
    case MachinePseudoKind::ReloadFromSlot:
      return MachinePrinterMnemonicKind::Load;
    case MachinePseudoKind::None:
      return MachinePrinterMnemonicKind::None;
  }
  return MachinePrinterMnemonicKind::None;
}

MachinePrinterMnemonicKind machine_instruction_primary_printer_mnemonic_kind(
    const InstructionRecord& instruction) {
  if (std::get_if<ReturnInstructionRecord>(&instruction.payload) != nullptr) {
    return MachinePrinterMnemonicKind::Return;
  }
  if (std::get_if<SpillReloadInstructionRecord>(&instruction.payload) != nullptr) {
    return machine_pseudo_printer_mnemonic_kind(instruction.pseudo);
  }
  return machine_opcode_printer_mnemonic_kind(instruction.opcode);
}

std::string_view machine_instruction_primary_printer_mnemonic(
    const InstructionRecord& instruction) {
  return machine_printer_mnemonic_kind_name(
      machine_instruction_primary_printer_mnemonic_kind(instruction));
}

MachinePrinterMnemonicKind machine_instruction_auxiliary_printer_mnemonic_kind(
    const InstructionRecord& instruction) {
  const auto* ret = std::get_if<ReturnInstructionRecord>(&instruction.payload);
  if (ret != nullptr && ret->value.has_value()) {
    return MachinePrinterMnemonicKind::Move;
  }
  return MachinePrinterMnemonicKind::None;
}

std::string_view machine_instruction_auxiliary_printer_mnemonic(
    const InstructionRecord& instruction) {
  return machine_printer_mnemonic_kind_name(
      machine_instruction_auxiliary_printer_mnemonic_kind(instruction));
}

std::string_view machine_node_selection_status_name(MachineNodeSelectionStatus status) {
  switch (status) {
    case MachineNodeSelectionStatus::Selected:
      return "selected";
    case MachineNodeSelectionStatus::DeferredUnsupported:
      return "deferred_unsupported";
    case MachineNodeSelectionStatus::MissingRequiredFacts:
      return "missing_required_facts";
  }
  return "unknown";
}

std::string_view machine_effect_resource_kind_name(MachineEffectResourceKind kind) {
  switch (kind) {
    case MachineEffectResourceKind::PreparedValue:
      return "prepared_value";
    case MachineEffectResourceKind::Register:
      return "register";
    case MachineEffectResourceKind::Memory:
      return "memory";
    case MachineEffectResourceKind::FrameSlot:
      return "frame_slot";
    case MachineEffectResourceKind::Symbol:
      return "symbol";
    case MachineEffectResourceKind::BranchTarget:
      return "branch_target";
    case MachineEffectResourceKind::Flags:
      return "flags";
  }
  return "unknown";
}

std::string_view machine_side_effect_kind_name(MachineSideEffectKind kind) {
  switch (kind) {
    case MachineSideEffectKind::ControlFlowTransfer:
      return "control_flow_transfer";
    case MachineSideEffectKind::MemoryRead:
      return "memory_read";
    case MachineSideEffectKind::MemoryWrite:
      return "memory_write";
    case MachineSideEffectKind::VolatileMemoryAccess:
      return "volatile_memory_access";
    case MachineSideEffectKind::Call:
      return "call";
    case MachineSideEffectKind::Return:
      return "return";
    case MachineSideEffectKind::FrameSetup:
      return "frame_setup";
    case MachineSideEffectKind::FrameTeardown:
      return "frame_teardown";
    case MachineSideEffectKind::InlineAssembly:
      return "inline_assembly";
    case MachineSideEffectKind::ObjectEmission:
      return "object_emission";
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

std::string_view frame_instruction_kind_name(FrameInstructionKind kind) {
  switch (kind) {
    case FrameInstructionKind::PrologueSetup:
      return "prologue_setup";
    case FrameInstructionKind::EpilogueTeardown:
      return "epilogue_teardown";
    case FrameInstructionKind::CalleeSaveStore:
      return "callee_save_store";
    case FrameInstructionKind::CalleeSaveLoad:
      return "callee_save_load";
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

std::string_view scalar_cast_operation_kind_name(ScalarCastOperationKind kind) {
  switch (kind) {
    case ScalarCastOperationKind::SignExtend:
      return "sign_extend";
    case ScalarCastOperationKind::ZeroExtend:
      return "zero_extend";
    case ScalarCastOperationKind::Truncate:
      return "truncate";
    case ScalarCastOperationKind::Deferred:
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

std::string_view prepared_scalar_cast_record_error_name(PreparedScalarCastRecordError error) {
  switch (error) {
    case PreparedScalarCastRecordError::None:
      return "none";
    case PreparedScalarCastRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedScalarCastRecordError::UnsupportedOpcode:
      return "unsupported_opcode";
    case PreparedScalarCastRecordError::UnsupportedResultValue:
      return "unsupported_result_value";
    case PreparedScalarCastRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedScalarCastRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedScalarCastRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedScalarCastRecordError::UnsupportedOperandValue:
      return "unsupported_operand_value";
    case PreparedScalarCastRecordError::MissingOperandValueHome:
      return "missing_operand_value_home";
    case PreparedScalarCastRecordError::MissingOperandStorage:
      return "missing_operand_storage";
    case PreparedScalarCastRecordError::UnsupportedOperandStorage:
      return "unsupported_operand_storage";
    case PreparedScalarCastRecordError::UnsupportedOperandType:
      return "unsupported_operand_type";
    case PreparedScalarCastRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
  }
  return "unknown";
}

std::string_view prepared_memory_operand_record_error_name(
    PreparedMemoryOperandRecordError error) {
  switch (error) {
    case PreparedMemoryOperandRecordError::None:
      return "none";
    case PreparedMemoryOperandRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess:
      return "missing_prepared_memory_access";
    case PreparedMemoryOperandRecordError::UnsupportedBase:
      return "unsupported_base";
    case PreparedMemoryOperandRecordError::MissingFrameSlotId:
      return "missing_frame_slot_id";
    case PreparedMemoryOperandRecordError::MissingSymbolName:
      return "missing_symbol_name";
    case PreparedMemoryOperandRecordError::SymbolMismatch:
      return "symbol_mismatch";
    case PreparedMemoryOperandRecordError::AddressFactMismatch:
      return "address_fact_mismatch";
    case PreparedMemoryOperandRecordError::MissingPointerValueName:
      return "missing_pointer_value_name";
    case PreparedMemoryOperandRecordError::MissingPointerValueHome:
      return "missing_pointer_value_home";
    case PreparedMemoryOperandRecordError::AmbiguousPointerValueHome:
      return "ambiguous_pointer_value_home";
    case PreparedMemoryOperandRecordError::PointerValueMismatch:
      return "pointer_value_mismatch";
    case PreparedMemoryOperandRecordError::StringIdentityMismatch:
      return "string_identity_mismatch";
    case PreparedMemoryOperandRecordError::ResultValueMismatch:
      return "result_value_mismatch";
    case PreparedMemoryOperandRecordError::StoredValueMismatch:
      return "stored_value_mismatch";
  }
  return "unknown";
}

bool is_compare_predicate(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return true;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
      return false;
  }
  return false;
}

bool is_scalar_alu_integer_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return true;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

bool is_simple_integer_cast_opcode(bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
      return true;
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return false;
  }
  return false;
}

ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return ScalarAluOperationKind::Add;
    case bir::BinaryOpcode::Sub:
      return ScalarAluOperationKind::Sub;
    case bir::BinaryOpcode::And:
      return ScalarAluOperationKind::And;
    case bir::BinaryOpcode::Or:
      return ScalarAluOperationKind::Or;
    case bir::BinaryOpcode::Xor:
      return ScalarAluOperationKind::Xor;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return ScalarAluOperationKind::Deferred;
  }
  return ScalarAluOperationKind::Deferred;
}

ScalarCastOperationKind scalar_cast_operation_from_cast_opcode(
    bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::SExt:
      return ScalarCastOperationKind::SignExtend;
    case bir::CastOpcode::ZExt:
      return ScalarCastOperationKind::ZeroExtend;
    case bir::CastOpcode::Trunc:
      return ScalarCastOperationKind::Truncate;
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
    case bir::CastOpcode::PtrToInt:
    case bir::CastOpcode::IntToPtr:
    case bir::CastOpcode::Bitcast:
      return ScalarCastOperationKind::Deferred;
  }
  return ScalarCastOperationKind::Deferred;
}

namespace {

PreparedBranchInstructionRecordResult branch_record_error(PreparedBranchRecordError error) {
  return PreparedBranchInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarAluRecordResult scalar_alu_record_error(PreparedScalarAluRecordError error) {
  return PreparedScalarAluRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarCastRecordResult scalar_cast_record_error(PreparedScalarCastRecordError error) {
  return PreparedScalarCastRecordResult{.record = std::nullopt, .error = error};
}

PreparedMemoryOperandRecordResult memory_operand_record_error(
    PreparedMemoryOperandRecordError error) {
  return PreparedMemoryOperandRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarInstructionRecordResult scalar_instruction_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarCastInstructionRecordResult scalar_cast_instruction_record_error(
    PreparedScalarCastRecordError error) {
  return PreparedScalarCastInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarCastRecordError scalar_cast_operand_error_from_alu_error(
    PreparedScalarAluRecordError error) {
  switch (error) {
    case PreparedScalarAluRecordError::None:
      return PreparedScalarCastRecordError::None;
    case PreparedScalarAluRecordError::UnsupportedOperandValue:
      return PreparedScalarCastRecordError::UnsupportedOperandValue;
    case PreparedScalarAluRecordError::MissingOperandValueHome:
      return PreparedScalarCastRecordError::MissingOperandValueHome;
    case PreparedScalarAluRecordError::MissingOperandStorage:
      return PreparedScalarCastRecordError::MissingOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandStorage:
      return PreparedScalarCastRecordError::UnsupportedOperandStorage;
    case PreparedScalarAluRecordError::UnsupportedOperandType:
      return PreparedScalarCastRecordError::UnsupportedOperandType;
    case PreparedScalarAluRecordError::RegisterConversionFailed:
      return PreparedScalarCastRecordError::RegisterConversionFailed;
    case PreparedScalarAluRecordError::InvalidFunction:
      return PreparedScalarCastRecordError::InvalidFunction;
    case PreparedScalarAluRecordError::UnsupportedOpcode:
      return PreparedScalarCastRecordError::UnsupportedOpcode;
    case PreparedScalarAluRecordError::UnsupportedResultValue:
      return PreparedScalarCastRecordError::UnsupportedResultValue;
    case PreparedScalarAluRecordError::MissingResultValueHome:
      return PreparedScalarCastRecordError::MissingResultValueHome;
    case PreparedScalarAluRecordError::MissingResultStorage:
      return PreparedScalarCastRecordError::MissingResultStorage;
    case PreparedScalarAluRecordError::UnsupportedResultStorage:
      return PreparedScalarCastRecordError::UnsupportedResultStorage;
  }
  return PreparedScalarCastRecordError::UnsupportedOperandType;
}

BranchTargetOperand make_prepared_branch_target(c4c::FunctionNameId function_name,
                                                c4c::BlockLabelId block_label,
                                                std::optional<prepare::PreparedValueId>
                                                    condition_value_id = std::nullopt) {
  return BranchTargetOperand{
      .surface = RecordSurfaceKind::RecordOnly,
      .block_label = block_label,
      .function_name = function_name,
      .condition_value_id = condition_value_id,
  };
}

bool same_bir_value(const bir::Value& lhs,
                    const bir::Value& rhs) {
  return lhs == rhs;
}

const prepare::PreparedValueHome* find_named_value_home(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named) {
    return nullptr;
  }
  return prepare::find_prepared_value_home(names, value_locations, value.name);
}

const prepare::PreparedStoragePlanValue* find_storage_plan_value(
    const prepare::PreparedStoragePlanFunction& storage_plan,
    prepare::PreparedValueId value_id) {
  for (const auto& value : storage_plan.values) {
    if (value.value_id == value_id) {
      return &value;
    }
  }
  return nullptr;
}

std::optional<prepare::PreparedValueId> find_value_home_id(
    const prepare::PreparedValueLocationFunction& value_locations,
    c4c::ValueNameId value_name) {
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  for (const auto& home : value_locations.value_homes) {
    if (home.value_name == value_name) {
      return home.value_id;
    }
  }
  return std::nullopt;
}

PreparedMemoryOperandRecordError find_unique_value_home_id(
    const prepare::PreparedValueLocationFunction& value_locations,
    c4c::ValueNameId value_name,
    prepare::PreparedValueId& out) {
  if (value_name == c4c::kInvalidValueName) {
    return PreparedMemoryOperandRecordError::MissingPointerValueName;
  }

  std::optional<prepare::PreparedValueId> found;
  for (const auto& home : value_locations.value_homes) {
    if (home.value_name != value_name) {
      continue;
    }
    if (found.has_value()) {
      return PreparedMemoryOperandRecordError::AmbiguousPointerValueHome;
    }
    found = home.value_id;
  }
  if (!found.has_value()) {
    return PreparedMemoryOperandRecordError::MissingPointerValueHome;
  }

  out = *found;
  return PreparedMemoryOperandRecordError::None;
}

std::optional<c4c::ValueNameId> named_value_id(
    const prepare::PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

PreparedMemoryOperandRecordError validate_structured_memory_address_facts(
    const bir::MemoryAddress& address,
    const MemoryOperand& memory) {
  if (address.byte_offset != memory.byte_offset ||
      (address.size_bytes != 0 && address.size_bytes != memory.size_bytes) ||
      (address.align_bytes != 0 && address.align_bytes != memory.align_bytes) ||
      address.address_space != memory.address_space ||
      address.is_volatile != memory.is_volatile) {
    return PreparedMemoryOperandRecordError::AddressFactMismatch;
  }
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError validate_unstructured_memory_instruction_facts(
    std::size_t instruction_byte_offset,
    std::size_t instruction_align_bytes,
    const MemoryOperand& memory) {
  if (static_cast<std::int64_t>(instruction_byte_offset) != memory.byte_offset ||
      (instruction_align_bytes != 0 && instruction_align_bytes != memory.align_bytes) ||
      memory.size_bytes != 0 || memory.address_space != bir::AddressSpace::Default ||
      memory.is_volatile) {
    return PreparedMemoryOperandRecordError::AddressFactMismatch;
  }
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError validate_memory_instruction_facts(
    const bir::MemoryAddress* address,
    std::size_t instruction_byte_offset,
    std::size_t instruction_align_bytes,
    const MemoryOperand& memory) {
  if (address != nullptr) {
    return validate_structured_memory_address_facts(*address, memory);
  }
  return validate_unstructured_memory_instruction_facts(
      instruction_byte_offset, instruction_align_bytes, memory);
}

std::optional<c4c::LinkNameId> global_symbol_id_from_address_or_inst(
    const bir::MemoryAddress* address,
    c4c::LinkNameId fallback_link_name) {
  if (address != nullptr) {
    if (address->base_kind != bir::MemoryAddress::BaseKind::GlobalSymbol) {
      return std::nullopt;
    }
    if (address->base_link_name_id != c4c::kInvalidLinkName) {
      return address->base_link_name_id;
    }
  }
  if (fallback_link_name != c4c::kInvalidLinkName) {
    return fallback_link_name;
  }
  return std::nullopt;
}

PreparedMemoryOperandRecordError validate_memory_base_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::MemoryAddress* address,
    c4c::LinkNameId fallback_link_name,
    MemoryOperand& memory) {
  switch (memory.base_kind) {
    case MemoryBaseKind::FrameSlot:
      if (address != nullptr &&
          address->base_kind != bir::MemoryAddress::BaseKind::LocalSlot) {
        return PreparedMemoryOperandRecordError::UnsupportedBase;
      }
      return PreparedMemoryOperandRecordError::None;
    case MemoryBaseKind::Symbol: {
      const auto symbol = global_symbol_id_from_address_or_inst(address, fallback_link_name);
      if (!symbol.has_value() || memory.symbol_name != *symbol) {
        return PreparedMemoryOperandRecordError::SymbolMismatch;
      }
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::PointerValue: {
      if (!memory.pointer_value_name.has_value()) {
        return PreparedMemoryOperandRecordError::MissingPointerValueName;
      }
      if (address == nullptr ||
          address->base_kind != bir::MemoryAddress::BaseKind::PointerValue) {
        return PreparedMemoryOperandRecordError::PointerValueMismatch;
      }
      const auto pointer_value_name = named_value_id(names, address->base_value);
      if (!pointer_value_name.has_value() || *pointer_value_name != *memory.pointer_value_name) {
        return PreparedMemoryOperandRecordError::PointerValueMismatch;
      }

      prepare::PreparedValueId pointer_value_id = 0;
      const auto error =
          find_unique_value_home_id(value_locations, *memory.pointer_value_name, pointer_value_id);
      if (error != PreparedMemoryOperandRecordError::None) {
        return error;
      }
      memory.pointer_value_id = pointer_value_id;
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::StringConstant: {
      if (!memory.string_symbol_name.has_value()) {
        return PreparedMemoryOperandRecordError::MissingSymbolName;
      }
      if (address == nullptr ||
          address->base_kind != bir::MemoryAddress::BaseKind::StringConstant) {
        return PreparedMemoryOperandRecordError::StringIdentityMismatch;
      }

      const std::string_view prepared_symbol =
          prepare::prepared_link_name(names, *memory.string_symbol_name);
      if (prepared_symbol.empty()) {
        return PreparedMemoryOperandRecordError::MissingSymbolName;
      }
      if (address->base_link_name_id != c4c::kInvalidLinkName) {
        if (address->base_link_name_id != *memory.string_symbol_name) {
          return PreparedMemoryOperandRecordError::StringIdentityMismatch;
        }
      } else if (address->base_name != prepared_symbol) {
        return PreparedMemoryOperandRecordError::StringIdentityMismatch;
      }

      const auto text_id = names.texts.find(prepared_symbol);
      if (text_id != c4c::kInvalidText) {
        memory.string_name = text_id;
      }
      return PreparedMemoryOperandRecordError::None;
    }
    case MemoryBaseKind::None:
    case MemoryBaseKind::Register:
      return PreparedMemoryOperandRecordError::UnsupportedBase;
  }
  return PreparedMemoryOperandRecordError::UnsupportedBase;
}

PreparedMemoryOperandRecordError apply_load_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedMemoryAccess& access,
    const bir::Value& result,
    MemoryOperand& memory) {
  if (!access.result_value_name.has_value() || access.stored_value_name.has_value()) {
    return PreparedMemoryOperandRecordError::ResultValueMismatch;
  }
  const auto result_name = named_value_id(names, result);
  if (!result_name.has_value() || *result_name != *access.result_value_name) {
    return PreparedMemoryOperandRecordError::ResultValueMismatch;
  }
  memory.result_value_name = *result_name;
  memory.result_value_id = find_value_home_id(value_locations, *result_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordError apply_store_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedMemoryAccess& access,
    const bir::Value& stored,
    MemoryOperand& memory) {
  if (access.result_value_name.has_value()) {
    return PreparedMemoryOperandRecordError::StoredValueMismatch;
  }
  const auto stored_name = named_value_id(names, stored);
  if (!stored_name.has_value()) {
    if (access.stored_value_name.has_value()) {
      return PreparedMemoryOperandRecordError::StoredValueMismatch;
    }
    return PreparedMemoryOperandRecordError::None;
  }
  if (!access.stored_value_name.has_value() || *stored_name != *access.stored_value_name) {
    return PreparedMemoryOperandRecordError::StoredValueMismatch;
  }
  memory.stored_value_name = *stored_name;
  memory.stored_value_id = find_value_home_id(value_locations, *stored_name);
  return PreparedMemoryOperandRecordError::None;
}

PreparedMemoryOperandRecordResult make_memory_record_from_prepared_access(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name) {
    return memory_operand_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  const auto* access =
      prepare::find_prepared_memory_access(addressing, block_label, instruction_index);
  if (access == nullptr || access->function_name != addressing.function_name) {
    return memory_operand_record_error(
        PreparedMemoryOperandRecordError::MissingPreparedMemoryAccess);
  }

  MemoryOperand memory{
      .surface = RecordSurfaceKind::RecordOnly,
      .support = MemoryOperandSupportKind::Prepared,
      .function_name = access->function_name,
      .block_label = access->block_label,
      .instruction_index = access->inst_index,
      .byte_offset = access->address.byte_offset,
      .size_bytes = access->address.size_bytes,
      .align_bytes = access->address.align_bytes,
      .address_space = access->address_space,
      .is_volatile = access->is_volatile,
      .can_use_base_plus_offset = access->address.can_use_base_plus_offset,
  };

  switch (access->address.base_kind) {
    case prepare::PreparedAddressBaseKind::FrameSlot:
      if (!access->address.frame_slot_id.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingFrameSlotId);
      }
      memory.base_kind = MemoryBaseKind::FrameSlot;
      memory.frame_slot_id = access->address.frame_slot_id;
      break;
    case prepare::PreparedAddressBaseKind::GlobalSymbol:
      if (!access->address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::Symbol;
      memory.symbol_name = access->address.symbol_name;
      break;
    case prepare::PreparedAddressBaseKind::PointerValue:
      if (!access->address.pointer_value_name.has_value()) {
        return memory_operand_record_error(
            PreparedMemoryOperandRecordError::MissingPointerValueName);
      }
      memory.base_kind = MemoryBaseKind::PointerValue;
      memory.pointer_value_name = access->address.pointer_value_name;
      break;
    case prepare::PreparedAddressBaseKind::StringConstant:
      if (!access->address.symbol_name.has_value()) {
        return memory_operand_record_error(PreparedMemoryOperandRecordError::MissingSymbolName);
      }
      memory.base_kind = MemoryBaseKind::StringConstant;
      memory.string_symbol_name = access->address.symbol_name;
      if (const auto text_id =
              names.texts.find(prepare::prepared_link_name(
                  names, *access->address.symbol_name));
          text_id != c4c::kInvalidText) {
        memory.string_name = text_id;
      }
      break;
    case prepare::PreparedAddressBaseKind::None:
      return memory_operand_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }

  return PreparedMemoryOperandRecordResult{
      .record = memory,
      .error = PreparedMemoryOperandRecordError::None,
  };
}

std::optional<abi::RegisterView> scalar_register_view(
    bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
      return abi::RegisterView::W;
    case bir::TypeKind::I64:
    case bir::TypeKind::Ptr:
      return abi::RegisterView::X;
    case bir::TypeKind::Void:
    case bir::TypeKind::I128:
    case bir::TypeKind::F32:
    case bir::TypeKind::F64:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

prepare::PreparedRegisterClass register_class_from_bank(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
      return prepare::PreparedRegisterClass::General;
    case prepare::PreparedRegisterBank::Fpr:
      return prepare::PreparedRegisterClass::Float;
    case prepare::PreparedRegisterBank::Vreg:
      return prepare::PreparedRegisterClass::Vector;
    case prepare::PreparedRegisterBank::AggregateAddress:
      return prepare::PreparedRegisterClass::AggregateAddress;
    case prepare::PreparedRegisterBank::None:
      return prepare::PreparedRegisterClass::None;
  }
  return prepare::PreparedRegisterClass::None;
}

std::string_view register_display_name(
    abi::RegisterReference reg) {
  static constexpr std::string_view x_names[] = {
      "x0",  "x1",  "x2",  "x3",  "x4",  "x5",  "x6",  "x7",
      "x8",  "x9",  "x10", "x11", "x12", "x13", "x14", "x15",
      "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",
      "x24", "x25", "x26", "x27", "x28", "x29", "x30", "x31"};
  static constexpr std::string_view w_names[] = {
      "w0",  "w1",  "w2",  "w3",  "w4",  "w5",  "w6",  "w7",
      "w8",  "w9",  "w10", "w11", "w12", "w13", "w14", "w15",
      "w16", "w17", "w18", "w19", "w20", "w21", "w22", "w23",
      "w24", "w25", "w26", "w27", "w28", "w29", "w30", "w31"};
  static constexpr std::string_view v_names[] = {
      "v0",  "v1",  "v2",  "v3",  "v4",  "v5",  "v6",  "v7",
      "v8",  "v9",  "v10", "v11", "v12", "v13", "v14", "v15",
      "v16", "v17", "v18", "v19", "v20", "v21", "v22", "v23",
      "v24", "v25", "v26", "v27", "v28", "v29", "v30", "v31"};
  static constexpr std::string_view s_names[] = {
      "s0",  "s1",  "s2",  "s3",  "s4",  "s5",  "s6",  "s7",
      "s8",  "s9",  "s10", "s11", "s12", "s13", "s14", "s15",
      "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
      "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"};
  static constexpr std::string_view d_names[] = {
      "d0",  "d1",  "d2",  "d3",  "d4",  "d5",  "d6",  "d7",
      "d8",  "d9",  "d10", "d11", "d12", "d13", "d14", "d15",
      "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31"};
  static constexpr std::string_view q_names[] = {
      "q0",  "q1",  "q2",  "q3",  "q4",  "q5",  "q6",  "q7",
      "q8",  "q9",  "q10", "q11", "q12", "q13", "q14", "q15",
      "q16", "q17", "q18", "q19", "q20", "q21", "q22", "q23",
      "q24", "q25", "q26", "q27", "q28", "q29", "q30", "q31"};
  if (reg.index >= 32U) {
    return {};
  }
  switch (reg.view) {
    case abi::RegisterView::X:
      return x_names[reg.index];
    case abi::RegisterView::W:
      return w_names[reg.index];
    case abi::RegisterView::V:
      return v_names[reg.index];
    case abi::RegisterView::S:
      return s_names[reg.index];
    case abi::RegisterView::D:
      return d_names[reg.index];
    case abi::RegisterView::Q:
      return q_names[reg.index];
    case abi::RegisterView::Sp:
      return "sp";
  }
  return {};
}

std::vector<std::string_view> occupied_register_views(
    abi::RegisterReference reg) {
  const auto display_name = register_display_name(reg);
  if (display_name.empty()) {
    return {};
  }
  return {display_name};
}

std::vector<std::string_view> occupied_register_views(
    const std::vector<abi::RegisterReference>& regs) {
  std::vector<std::string_view> views;
  views.reserve(regs.size());
  for (const auto reg : regs) {
    const auto display_name = register_display_name(reg);
    if (display_name.empty()) {
      return {};
    }
    views.push_back(display_name);
  }
  return views;
}

std::vector<abi::RegisterReference> occupied_register_references(
    abi::RegisterReference reg) {
  return {reg};
}

std::optional<abi::RegisterView> prepared_clobber_expected_view(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return abi::RegisterView::X;
    case prepare::PreparedRegisterBank::Fpr:
    case prepare::PreparedRegisterBank::Vreg:
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<MachineEffectResource> effect_from_prepared_call_clobber(
    const prepare::PreparedClobberedRegister& clobber) {
  if (clobber.register_name.empty() || clobber.contiguous_width == 0 ||
      clobber.bank == prepare::PreparedRegisterBank::None) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(clobber.bank);
  const auto expected_view = prepared_clobber_expected_view(clobber.bank);
  const auto converted_primary = abi::convert_prepared_register(
      clobber.register_name, clobber.bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = clobber.occupied_register_names;
  if (occupied_names.empty() && clobber.contiguous_width == 1) {
    occupied_names.push_back(clobber.register_name);
  }
  if (occupied_names.size() != clobber.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, clobber.bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .prepared_class = prepared_class,
      .prepared_bank = clobber.bank,
      .expected_view = expected_view,
      .contiguous_width = clobber.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_clobbers(
    const std::vector<prepare::PreparedClobberedRegister>& clobbers) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(clobbers.size());
  for (const auto& clobber : clobbers) {
    if (auto effect = effect_from_prepared_call_clobber(clobber)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

std::optional<MachineEffectResource> effect_from_prepared_call_preserved_value(
    const prepare::PreparedCallPreservedValue& preserved) {
  if (preserved.route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      !preserved.register_name.has_value() || !preserved.register_bank.has_value() ||
      preserved.register_name->empty() ||
      *preserved.register_bank == prepare::PreparedRegisterBank::None ||
      preserved.contiguous_width == 0) {
    return std::nullopt;
  }

  const auto prepared_class = register_class_from_bank(*preserved.register_bank);
  const auto expected_view = prepared_clobber_expected_view(*preserved.register_bank);
  const auto converted_primary = abi::convert_prepared_register(
      *preserved.register_name, *preserved.register_bank, prepared_class, expected_view);
  if (!converted_primary.has_value()) {
    return std::nullopt;
  }

  std::vector<std::string> occupied_names = preserved.occupied_register_names;
  if (occupied_names.empty() && preserved.contiguous_width == 1) {
    occupied_names.push_back(*preserved.register_name);
  }
  if (occupied_names.size() != preserved.contiguous_width) {
    return std::nullopt;
  }

  std::vector<abi::RegisterReference> occupied_refs;
  occupied_refs.reserve(occupied_names.size());
  for (const auto& occupied_name : occupied_names) {
    const auto converted_occupied = abi::convert_prepared_register(
        occupied_name, *preserved.register_bank, prepared_class, expected_view);
    if (!converted_occupied.has_value()) {
      return std::nullopt;
    }
    occupied_refs.push_back(*converted_occupied.reg);
  }
  if (occupied_refs.empty() || occupied_refs.front() != *converted_primary.reg) {
    return std::nullopt;
  }

  const auto occupied_views = occupied_register_views(occupied_refs);
  if (occupied_views.size() != occupied_refs.size()) {
    return std::nullopt;
  }

  const OperandRecord operand = make_register_operand(RegisterOperand{
      .reg = *converted_primary.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = *preserved.register_bank,
      .expected_view = expected_view,
      .contiguous_width = preserved.contiguous_width,
      .occupied_register_references = occupied_refs,
      .occupied_registers = occupied_views,
  });
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::Register,
      .operand = operand,
      .value_id = preserved.value_id,
      .value_name = preserved.value_name,
      .reg = *converted_primary.reg,
  };
}

std::vector<MachineEffectResource> effects_from_prepared_call_preserved_values(
    const std::vector<prepare::PreparedCallPreservedValue>& preserved_values) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(preserved_values.size());
  for (const auto& preserved : preserved_values) {
    if (auto effect = effect_from_prepared_call_preserved_value(preserved)) {
      effects.push_back(std::move(*effect));
    }
  }
  return effects;
}

std::optional<RegisterOperand> make_prepared_register_operand(
    const prepare::PreparedValueHome& home,
    const prepare::PreparedStoragePlanValue& storage,
    bir::TypeKind type,
    RegisterOperandRole role) {
  if (home.kind != prepare::PreparedValueHomeKind::Register ||
      storage.encoding != prepare::PreparedStorageEncodingKind::Register) {
    return std::nullopt;
  }

  const auto expected_view = scalar_register_view(type);
  if (!expected_view.has_value()) {
    return std::nullopt;
  }
  const auto prepared_class = register_class_from_bank(storage.bank);
  abi::PreparedRegisterConversionResult converted;
  if (storage.register_placement.has_value()) {
    converted = abi::convert_prepared_register(
        *storage.register_placement, prepared_class, expected_view);
  } else {
    if (!home.register_name.has_value() || !storage.register_name.has_value() ||
        *home.register_name != *storage.register_name) {
      return std::nullopt;
    }
    converted = abi::convert_prepared_register(
        *storage.register_name, storage.bank, prepared_class, expected_view);
  }
  if (!converted.has_value()) {
    return std::nullopt;
  }

  return RegisterOperand{
      .reg = *converted.reg,
      .role = role,
      .value_id = home.value_id,
      .value_name = home.value_name,
      .prepared_class = prepared_class,
      .prepared_bank = storage.bank,
      .expected_view = expected_view,
      .contiguous_width = storage.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

std::optional<ImmediateOperand> make_scalar_immediate_operand(
    const bir::Value& value,
    std::optional<prepare::PreparedValueId> source_value_id = std::nullopt,
    c4c::ValueNameId source_value_name = c4c::kInvalidValueName) {
  if (value.kind != bir::Value::Kind::Immediate) {
    return std::nullopt;
  }

  ImmediateKind kind = ImmediateKind::SignedInteger;
  if (value.type == bir::TypeKind::I1) {
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
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& value,
    OperandRecord& out) {
  if (value.kind == bir::Value::Kind::Immediate) {
    const auto immediate = make_scalar_immediate_operand(value);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
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

  if (storage->encoding == prepare::PreparedStorageEncodingKind::Immediate) {
    if (home->kind != prepare::PreparedValueHomeKind::RematerializableImmediate ||
        !home->immediate_i32.has_value() || !storage->immediate_i32.has_value() ||
        *home->immediate_i32 != *storage->immediate_i32) {
      return PreparedScalarAluRecordError::UnsupportedOperandStorage;
    }
    const auto immediate = make_scalar_immediate_operand(
        bir::Value::immediate_i32(static_cast<std::int32_t>(*storage->immediate_i32)),
        home->value_id,
        home->value_name);
    if (!immediate.has_value()) {
      return PreparedScalarAluRecordError::UnsupportedOperandType;
    }
    out = make_immediate_operand(*immediate);
    return PreparedScalarAluRecordError::None;
  }

  if (home->kind != prepare::PreparedValueHomeKind::Register ||
      storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!storage->register_placement.has_value() &&
       (!home->register_name.has_value() || !storage->register_name.has_value() ||
        *home->register_name != *storage->register_name))) {
    return PreparedScalarAluRecordError::UnsupportedOperandStorage;
  }

  const auto reg =
      make_prepared_register_operand(*home, *storage, value.type, RegisterOperandRole::StoragePlan);
  if (!reg.has_value()) {
    return PreparedScalarAluRecordError::RegisterConversionFailed;
  }
  out = make_register_operand(*reg);
  return PreparedScalarAluRecordError::None;
}

std::optional<CompareValueRecord> make_compare_value_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const bir::Value& value) {
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

  const auto* home = find_named_value_home(names, value_locations, value);
  if (home == nullptr || home->value_name == c4c::kInvalidValueName) {
    return std::nullopt;
  }
  record.value_id = home->value_id;
  record.value_name = home->value_name;
  return record;
}

MachineOpcode machine_opcode_from_scalar_alu(ScalarAluOperationKind operation) {
  switch (operation) {
    case ScalarAluOperationKind::Add:
      return MachineOpcode::Add;
    case ScalarAluOperationKind::Sub:
      return MachineOpcode::Sub;
    case ScalarAluOperationKind::And:
      return MachineOpcode::And;
    case ScalarAluOperationKind::Or:
      return MachineOpcode::Or;
    case ScalarAluOperationKind::Xor:
      return MachineOpcode::Xor;
    case ScalarAluOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_scalar_cast(ScalarCastOperationKind operation) {
  switch (operation) {
    case ScalarCastOperationKind::SignExtend:
      return MachineOpcode::SignExtend;
    case ScalarCastOperationKind::ZeroExtend:
      return MachineOpcode::ZeroExtend;
    case ScalarCastOperationKind::Truncate:
      return MachineOpcode::Truncate;
    case ScalarCastOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_scalar_instruction(const ScalarInstructionRecord& instruction) {
  if (instruction.scalar_alu.has_value()) {
    return machine_opcode_from_scalar_alu(instruction.scalar_alu->operation);
  }
  if (instruction.scalar_cast.has_value()) {
    return machine_opcode_from_scalar_cast(instruction.scalar_cast->operation);
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_branch_instruction(const BranchInstructionRecord& instruction) {
  if (!instruction.conditional) {
    return MachineOpcode::Branch;
  }
  if (instruction.condition_record.has_value() &&
      instruction.condition_record->form == BranchConditionForm::FusedCompare) {
    return MachineOpcode::CompareBranch;
  }
  return MachineOpcode::ConditionalBranch;
}

MachineOpcode machine_opcode_from_memory_instruction(const MemoryInstructionRecord& instruction) {
  switch (instruction.memory_kind) {
    case MemoryInstructionKind::Load:
      return MachineOpcode::Load;
    case MemoryInstructionKind::Store:
      return MachineOpcode::Store;
  }
  return MachineOpcode::Unspecified;
}

MachineEffectResource effect_from_operand(const OperandRecord& operand) {
  MachineEffectResource resource;
  resource.operand = operand;
  switch (operand.kind) {
    case OperandKind::Register: {
      const auto* reg = std::get_if<RegisterOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Register;
      if (reg != nullptr) {
        resource.value_id = reg->value_id;
        resource.value_name = reg->value_name;
        resource.reg = reg->reg;
      }
      break;
    }
    case OperandKind::Immediate: {
      const auto* immediate = std::get_if<ImmediateOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (immediate != nullptr) {
        resource.value_id = immediate->source_value_id;
        resource.value_name = immediate->source_value_name;
      }
      break;
    }
    case OperandKind::PreparedValue: {
      const auto* value = std::get_if<PreparedValueOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::PreparedValue;
      if (value != nullptr) {
        resource.value_id = value->value_id;
        resource.value_name = value->value_name;
      }
      break;
    }
    case OperandKind::FrameSlot: {
      const auto* slot = std::get_if<FrameSlotOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::FrameSlot;
      if (slot != nullptr) {
        resource.frame_slot_id = slot->slot_id;
        if (slot->value_name.has_value()) {
          resource.value_name = *slot->value_name;
        }
      }
      break;
    }
    case OperandKind::Symbol: {
      const auto* symbol = std::get_if<SymbolOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Symbol;
      if (symbol != nullptr) {
        resource.symbol_name = symbol->link_name;
      }
      break;
    }
    case OperandKind::BranchTarget: {
      const auto* target = std::get_if<BranchTargetOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::BranchTarget;
      if (target != nullptr) {
        resource.value_id = target->condition_value_id;
        resource.block_label = target->block_label;
      }
      break;
    }
    case OperandKind::Memory: {
      const auto* memory = std::get_if<MemoryOperand>(&operand.payload);
      resource.kind = MachineEffectResourceKind::Memory;
      if (memory != nullptr) {
        resource.value_id = memory->result_value_id.has_value() ? memory->result_value_id
                                                                : memory->stored_value_id;
        if (memory->result_value_name.has_value()) {
          resource.value_name = *memory->result_value_name;
        } else if (memory->stored_value_name.has_value()) {
          resource.value_name = *memory->stored_value_name;
        }
        resource.frame_slot_id = memory->frame_slot_id;
        resource.symbol_name = memory->symbol_name.has_value() ? memory->symbol_name
                                                               : memory->string_symbol_name;
      }
      break;
    }
  }
  return resource;
}

MachineEffectResource prepared_value_def(
    std::optional<prepare::PreparedValueId> value_id,
    c4c::ValueNameId value_name) {
  return MachineEffectResource{
      .kind = MachineEffectResourceKind::PreparedValue,
      .value_id = value_id,
      .value_name = value_name,
  };
}

OperandRecord make_condition_value_operand(const BranchConditionRecord& condition) {
  return make_prepared_value_operand(PreparedValueOperand{
      .value_id = condition.condition_value_id.value_or(0),
      .value_name = condition.condition_value_name,
      .type = condition.condition_type,
  });
}

std::optional<OperandRecord> make_compare_value_operand(const CompareValueRecord& value) {
  if (value.source_value.kind == bir::Value::Kind::Immediate) {
    const auto immediate =
        make_scalar_immediate_operand(value.source_value, value.value_id, value.value_name);
    if (immediate.has_value()) {
      return make_immediate_operand(*immediate);
    }
    return std::nullopt;
  }
  if (!value.value_id.has_value()) {
    return std::nullopt;
  }
  return make_prepared_value_operand(PreparedValueOperand{
      .value_id = *value.value_id,
      .value_name = value.value_name,
      .type = value.type,
  });
}

std::vector<OperandRecord> branch_instruction_operands(const BranchInstructionRecord& instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.target_pair.has_value()) {
    operands.push_back(OperandRecord{
        .kind = OperandKind::BranchTarget,
        .payload = instruction.target_pair->true_target,
    });
    operands.push_back(OperandRecord{
        .kind = OperandKind::BranchTarget,
        .payload = instruction.target_pair->false_target,
    });
  } else {
    operands.push_back(OperandRecord{.kind = OperandKind::BranchTarget,
                                     .payload = instruction.target});
  }
  if (instruction.condition.has_value()) {
    operands.push_back(*instruction.condition);
  } else if (instruction.condition_record.has_value() &&
             instruction.condition_record->condition_value_id.has_value()) {
    operands.push_back(make_condition_value_operand(*instruction.condition_record));
  }
  if (instruction.condition_record.has_value() &&
      instruction.condition_record->form == BranchConditionForm::FusedCompare &&
      instruction.condition_record->compare_operands.has_value()) {
    if (const auto lhs =
            make_compare_value_operand(instruction.condition_record->compare_operands->lhs);
        lhs.has_value()) {
      operands.push_back(*lhs);
    }
    if (const auto rhs =
            make_compare_value_operand(instruction.condition_record->compare_operands->rhs);
        rhs.has_value()) {
      operands.push_back(*rhs);
    }
  }
  return operands;
}

std::vector<MachineEffectResource> effects_from_operands(
    const std::vector<OperandRecord>& operands) {
  std::vector<MachineEffectResource> effects;
  effects.reserve(operands.size());
  for (const auto& operand : operands) {
    effects.push_back(effect_from_operand(operand));
  }
  return effects;
}

std::vector<MachineSideEffectKind> memory_side_effects(
    const MemoryInstructionRecord& instruction) {
  std::vector<MachineSideEffectKind> side_effects;
  side_effects.push_back(instruction.memory_kind == MemoryInstructionKind::Load
                             ? MachineSideEffectKind::MemoryRead
                             : MachineSideEffectKind::MemoryWrite);
  if (instruction.address.is_volatile) {
    side_effects.push_back(MachineSideEffectKind::VolatileMemoryAccess);
  }
  return side_effects;
}

std::vector<MachineSideEffectKind> spill_reload_side_effects(
    const SpillReloadInstructionRecord& instruction) {
  switch (instruction.pseudo_kind) {
    case MachinePseudoKind::SpillToSlot:
      return {MachineSideEffectKind::MemoryWrite};
    case MachinePseudoKind::ReloadFromSlot:
      return {MachineSideEffectKind::MemoryRead};
    case MachinePseudoKind::None:
      break;
  }
  return {};
}

MachineOpcode machine_opcode_from_frame_instruction(const FrameInstructionRecord& instruction) {
  switch (instruction.frame_kind) {
    case FrameInstructionKind::PrologueSetup:
      return MachineOpcode::FrameSetup;
    case FrameInstructionKind::EpilogueTeardown:
      return MachineOpcode::FrameTeardown;
    case FrameInstructionKind::CalleeSaveStore:
      return MachineOpcode::CalleeSaveStore;
    case FrameInstructionKind::CalleeSaveLoad:
      return MachineOpcode::CalleeSaveLoad;
  }
  return MachineOpcode::Unspecified;
}

std::vector<MachineSideEffectKind> frame_side_effects(
    const FrameInstructionRecord& instruction) {
  switch (instruction.frame_kind) {
    case FrameInstructionKind::PrologueSetup:
      return {MachineSideEffectKind::FrameSetup};
    case FrameInstructionKind::EpilogueTeardown:
      return {MachineSideEffectKind::FrameTeardown};
    case FrameInstructionKind::CalleeSaveStore:
      return {MachineSideEffectKind::MemoryWrite};
    case FrameInstructionKind::CalleeSaveLoad:
      return {MachineSideEffectKind::MemoryRead};
  }
  return {};
}

MachineNodeStatusRecord branch_selection_status(const BranchInstructionRecord& instruction,
                                                const std::vector<OperandRecord>& operands) {
  if (!instruction.conditional) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (!instruction.target_pair.has_value() || !instruction.condition_record.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "conditional branch is missing target pair or condition"};
  }
  const auto& condition = *instruction.condition_record;
  if (!condition.condition_value_id.has_value() ||
      condition.condition_value_name == c4c::kInvalidValueName ||
      condition.condition_type != bir::TypeKind::I1) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "conditional branch is missing condition value identity"};
  }
  if (condition.form == BranchConditionForm::FusedCompare) {
    const auto has_candidate = condition.compare_branch_candidate.has_value();
    const auto candidate_fusable =
        has_candidate ? condition.compare_branch_candidate->kind ==
                                BranchCompareCandidateKind::FusedCompareAndBranch &&
                            condition.compare_branch_candidate->can_fuse_with_branch
                      : condition.can_fuse_with_branch;
    if (!condition.predicate.has_value() || !condition.compare_operands.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                     .diagnostic =
                                         "fused compare branch is missing compare facts"};
    }
    if (!condition.can_fuse_with_branch || !candidate_fusable) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                     .diagnostic =
                                         "compare branch candidate is not fusable"};
    }
    if (operands.size() < 5U) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                     .diagnostic =
                                         "fused compare branch is missing compare operands"};
    }
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord scalar_selection_status(const ScalarInstructionRecord& instruction) {
  if (instruction.scalar_alu.has_value()) {
    if (instruction.scalar_alu->supported_integer_operation &&
        instruction.scalar_alu->operation != ScalarAluOperationKind::Deferred) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "scalar ALU operation is outside the selected subset"};
  }
  if (instruction.scalar_cast.has_value()) {
    if (instruction.scalar_cast->supported_simple_integer_cast &&
        instruction.scalar_cast->operation != ScalarCastOperationKind::Deferred) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "scalar cast operation is outside the selected subset"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                 .diagnostic = "scalar node is missing scalar ALU or cast record"};
}

bool is_supported_memory_base(MemoryBaseKind base_kind) {
  switch (base_kind) {
    case MemoryBaseKind::FrameSlot:
    case MemoryBaseKind::PointerValue:
      return true;
    case MemoryBaseKind::Symbol:
    case MemoryBaseKind::StringConstant:
    case MemoryBaseKind::None:
    case MemoryBaseKind::Register:
      return false;
  }
  return false;
}

MachineNodeStatusRecord memory_selection_status(const MemoryInstructionRecord& instruction) {
  if (instruction.address.support != MemoryOperandSupportKind::Prepared ||
      !is_supported_memory_base(instruction.address.base_kind)) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "memory operand is outside the selected subset"};
  }
  if (instruction.memory_kind == MemoryInstructionKind::Load) {
    if (!instruction.result_value_id.has_value() ||
        instruction.result_value_name == c4c::kInvalidValueName ||
        !instruction.address.result_value_id.has_value() ||
        !instruction.address.result_value_name.has_value()) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                     .diagnostic =
                                         "load node is missing prepared result identity"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (!instruction.value.has_value() || !instruction.address.stored_value_id.has_value() ||
      !instruction.address.stored_value_name.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "store node is missing prepared stored value identity"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord spill_reload_selection_status(
    const SpillReloadInstructionRecord& instruction) {
  if (instruction.pseudo_kind == MachinePseudoKind::None ||
      instruction.op_kind == prepare::PreparedSpillReloadOpKind::Rematerialize) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "spill/reload op is outside the selected subset"};
  }
  if (!instruction.source_spill_reload || !instruction.slot_id.has_value() ||
      !instruction.stack_offset_bytes.has_value() || !instruction.scratch.has_value() ||
      instruction.occupied_scratch_register_references.empty() ||
      !instruction.scratch_register_authority.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "spill/reload node is missing slot or scratch facts"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord frame_selection_status(const FrameInstructionRecord& instruction) {
  if (instruction.source_frame == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName ||
      instruction.frame_alignment_bytes == 0) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "frame node is missing prepared frame facts"};
  }
  if ((instruction.frame_kind == FrameInstructionKind::CalleeSaveStore ||
       instruction.frame_kind == FrameInstructionKind::CalleeSaveLoad) &&
      !instruction.callee_save.has_value()) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::MissingRequiredFacts,
                                   .diagnostic =
                                       "callee-save frame node is missing prepared save facts"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_boundary_move_selection_status(
    const CallBoundaryMoveInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_move == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary move node is missing prepared move provenance"};
  }
  const bool selected_register_argument_move =
      instruction.phase == prepare::PreparedMovePhase::BeforeCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  const bool selected_register_result_move =
      instruction.phase == prepare::PreparedMovePhase::AfterCall &&
      instruction.move.destination_kind ==
          prepare::PreparedMoveDestinationKind::CallResultAbi &&
      instruction.move.destination_storage_kind ==
          prepare::PreparedMoveStorageKind::Register &&
      instruction.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move;
  if (!selected_register_argument_move && !selected_register_result_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node is outside the selected register call-boundary move subset"};
  }
  if (!instruction.source_register.has_value() ||
      !instruction.destination_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_register->prepared_bank != prepare::PreparedRegisterBank::Gpr ||
      instruction.destination_register->prepared_bank != prepare::PreparedRegisterBank::Gpr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared GPR source and destination"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord call_boundary_abi_binding_selection_status(
    const CallBoundaryAbiBindingInstructionRecord& instruction) {
  if (instruction.source_bundle == nullptr || instruction.source_binding == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "call-boundary ABI binding node is missing prepared binding provenance"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
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
  const auto operands = branch_instruction_operands(instruction);
  const auto selection = branch_selection_status(instruction, operands);
  return InstructionRecord{
      .family = InstructionFamily::Branch,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_branch_instruction(instruction),
      .selection = selection,
      .function_name = instruction.target.function_name,
      .block_label = instruction.target.block_label,
      .operands = operands,
      .uses = effects_from_operands(operands),
      .side_effects = {MachineSideEffectKind::ControlFlowTransfer},
      .payload = instruction,
  };
}

InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction) {
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }
  const auto selection = scalar_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Scalar,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_scalar_instruction(instruction),
      .selection = selection,
      .operands = instruction.inputs,
      .defs = defs,
      .uses = effects_from_operands(instruction.inputs),
      .payload = instruction,
  };
}

ScalarInstructionRecord make_scalar_alu_instruction_record(ScalarAluRecord alu) {
  return ScalarInstructionRecord{
      .result_value_id = alu.result_value_id,
      .result_value_name = alu.result_value_name,
      .result_type = alu.result_type,
      .result_register = alu.result_register,
      .inputs = {alu.lhs, alu.rhs},
      .source_binary_opcode = alu.source_binary_opcode,
      .scalar_alu = alu,
  };
}

ScalarInstructionRecord make_scalar_cast_instruction_record(ScalarCastRecord cast) {
  return ScalarInstructionRecord{
      .result_value_id = cast.result_value_id,
      .result_value_name = cast.result_value_name,
      .result_type = cast.result_type,
      .inputs = {cast.source},
      .source_cast_opcode = cast.source_cast_opcode,
      .scalar_cast = cast,
  };
}

InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction) {
  std::vector<OperandRecord> operands = {make_memory_operand(instruction.address)};
  if (instruction.value.has_value()) {
    operands.push_back(*instruction.value);
  }
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses = effects_from_operands(operands);
  if (instruction.memory_kind == MemoryInstructionKind::Load) {
    defs.push_back(prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }
  const auto selection = memory_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_memory_instruction(instruction),
      .selection = selection,
      .function_name = instruction.address.function_name,
      .block_label = instruction.address.block_label,
      .instruction_index = instruction.address.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .side_effects = memory_side_effects(instruction),
      .payload = instruction,
  };
}

InstructionRecord make_spill_reload_instruction(SpillReloadInstructionRecord instruction) {
  if (instruction.scratch.has_value() &&
      instruction.scratch->occupied_register_references.empty()) {
    instruction.scratch->occupied_register_references.push_back(instruction.scratch->reg);
  }
  if (instruction.occupied_scratch_register_references.empty() &&
      instruction.scratch.has_value()) {
    instruction.occupied_scratch_register_references =
        instruction.scratch->occupied_register_references;
  }
  std::vector<OperandRecord> operands = {make_memory_operand(instruction.slot)};
  if (instruction.scratch.has_value()) {
    operands.push_back(make_register_operand(*instruction.scratch));
  }
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses = effects_from_operands(operands);
  if (instruction.pseudo_kind == MachinePseudoKind::ReloadFromSlot &&
      instruction.scratch.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.scratch)));
  }
  const auto selection = spill_reload_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.pseudo_kind == MachinePseudoKind::SpillToSlot
                    ? MachineOpcode::SpillToSlot
                    : instruction.pseudo_kind == MachinePseudoKind::ReloadFromSlot
                          ? MachineOpcode::ReloadFromSlot
                          : MachineOpcode::Unspecified,
      .pseudo = instruction.pseudo_kind,
      .selection = selection,
      .function_name = instruction.slot.function_name,
      .block_label = instruction.slot.block_label,
      .instruction_index = instruction.slot.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .side_effects = spill_reload_side_effects(instruction),
      .payload = instruction,
  };
}

InstructionRecord make_frame_instruction(FrameInstructionRecord instruction) {
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.callee_save.has_value() &&
      instruction.callee_save->register_operand.has_value()) {
    const auto reg_operand = make_register_operand(*instruction.callee_save->register_operand);
    if (instruction.frame_kind == FrameInstructionKind::CalleeSaveLoad) {
      defs.push_back(effect_from_operand(reg_operand));
    } else {
      uses.push_back(effect_from_operand(reg_operand));
    }
  }
  if (instruction.callee_save.has_value() &&
      instruction.callee_save->slot_id.has_value()) {
    uses.push_back(MachineEffectResource{
        .kind = MachineEffectResourceKind::FrameSlot,
        .frame_slot_id = instruction.callee_save->slot_id,
    });
  }
  return InstructionRecord{
      .family = InstructionFamily::Frame,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_frame_instruction(instruction),
      .selection = frame_selection_status(instruction),
      .function_name = instruction.function_name,
      .defs = defs,
      .uses = uses,
      .side_effects = frame_side_effects(instruction),
      .payload = instruction,
  };
}

InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.destination_register.has_value()) {
    const auto destination = make_register_operand(*instruction.destination_register);
    operands.push_back(destination);
    defs.push_back(effect_from_operand(destination));
  } else if (instruction.move.to_value_id != 0) {
    defs.push_back(prepared_value_def(instruction.move.to_value_id, c4c::kInvalidValueName));
  }
  if (instruction.source_register.has_value()) {
    const auto source = make_register_operand(*instruction.source_register);
    operands.push_back(source);
    uses.push_back(effect_from_operand(source));
  } else if (instruction.move.from_value_id != 0) {
    uses.push_back(prepared_value_def(instruction.move.from_value_id, c4c::kInvalidValueName));
  }
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryMove,
      .selection = call_boundary_move_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
      .payload = instruction,
  };
}

InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::CallBoundaryAbiBinding,
      .selection = call_boundary_abi_binding_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .payload = instruction,
  };
}

InstructionRecord make_call_instruction(CallInstructionRecord instruction) {
  std::vector<OperandRecord> operands = instruction.arguments;
  if (instruction.indirect_callee.has_value()) {
    operands.insert(operands.begin(), *instruction.indirect_callee);
  } else if (instruction.direct_callee.has_value()) {
    operands.insert(operands.begin(), make_symbol_operand(*instruction.direct_callee));
  }
  std::vector<MachineEffectResource> defs;
  if (instruction.result.has_value()) {
    defs.push_back(effect_from_operand(*instruction.result));
  }
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(effect_from_operand(make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.is_indirect ? MachineOpcode::IndirectCall : MachineOpcode::DirectCall,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .preserves = effects_from_prepared_call_preserved_values(instruction.preserved_values),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

InstructionRecord make_return_instruction(ReturnInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.value.has_value()) {
    operands.push_back(*instruction.value);
  }
  return InstructionRecord{
      .family = InstructionFamily::Return,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .selection = MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected},
      .operands = operands,
      .uses = effects_from_operands(operands),
      .side_effects = {MachineSideEffectKind::Return, MachineSideEffectKind::ControlFlowTransfer},
      .payload = instruction,
  };
}

InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction) {
  return InstructionRecord{
      .family = InstructionFamily::Assembler,
      .surface = RecordSurfaceKind::ExternalAssemblerInput,
      .selection =
          MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                  .diagnostic = "external assembler input is not a selected node"},
      .operands = instruction.operands,
      .uses = effects_from_operands(instruction.operands),
      .side_effects = instruction.side_effects
                          ? std::vector<MachineSideEffectKind>{
                                MachineSideEffectKind::InlineAssembly}
                          : std::vector<MachineSideEffectKind>{},
      .payload = instruction,
  };
}

InstructionRecord make_object_instruction(ObjectInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.symbol.has_value()) {
    operands.push_back(make_symbol_operand(*instruction.symbol));
  }
  if (instruction.frame_slot.has_value()) {
    operands.push_back(make_frame_slot_operand(*instruction.frame_slot));
  }
  if (instruction.value.has_value()) {
    operands.push_back(*instruction.value);
  }
  return InstructionRecord{
      .family = InstructionFamily::Object,
      .surface = RecordSurfaceKind::EncoderInput,
      .selection =
          MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                  .diagnostic = "object records are future encoder input"},
      .operands = operands,
      .uses = effects_from_operands(operands),
      .side_effects = {MachineSideEffectKind::ObjectEmission},
      .payload = instruction,
  };
}

InstructionRecord make_unsupported_machine_instruction(InstructionFamily family,
                                                       MachineNodeSelectionStatus status,
                                                       std::string_view diagnostic) {
  return InstructionRecord{
      .family = family,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .selection = MachineNodeStatusRecord{.status = status, .diagnostic = diagnostic},
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
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const bir::Terminator& terminator) {
  if (branch_condition.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != branch_condition.function_name) {
    return branch_record_error(PreparedBranchRecordError::InvalidFunction);
  }
  if (block.block_label == c4c::kInvalidBlockLabel ||
      branch_condition.block_label != block.block_label) {
    return branch_record_error(PreparedBranchRecordError::InvalidSourceBlock);
  }
  if (block.terminator_kind != bir::TerminatorKind::CondBranch ||
      terminator.kind != bir::TerminatorKind::CondBranch) {
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
  if (branch_condition.condition_value.type != bir::TypeKind::I1 ||
      terminator.condition.type != bir::TypeKind::I1) {
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
                      prepare::PreparedBranchConditionKind::FusedCompare
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

PreparedScalarAluRecordResult make_prepared_scalar_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::InvalidFunction);
  }
  if (!is_scalar_alu_integer_opcode(binary.opcode)) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
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
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() || !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedResultStorage);
  }
  const auto result_register = make_prepared_register_operand(
      *result_home, *result_storage, binary.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::RegisterConversionFailed);
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
              .result_register = result_register,
              .lhs = lhs,
              .rhs = rhs,
              .supported_integer_operation = true,
          },
      .error = PreparedScalarAluRecordError::None,
  };
}

PreparedScalarInstructionRecordResult make_prepared_scalar_alu_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary) {
  const auto result = make_prepared_scalar_alu_record(names, value_locations, storage_plan, binary);
  if (!result.record.has_value()) {
    return scalar_instruction_record_error(result.error);
  }
  return PreparedScalarInstructionRecordResult{
      .record = make_scalar_alu_instruction_record(*result.record),
      .error = PreparedScalarAluRecordError::None,
  };
}

PreparedScalarCastRecordResult make_prepared_scalar_cast_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  if (value_locations.function_name == c4c::kInvalidFunctionName ||
      storage_plan.function_name != value_locations.function_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::InvalidFunction);
  }
  if (!is_simple_integer_cast_opcode(cast.opcode)) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOpcode);
  }
  if (cast.result.kind != bir::Value::Kind::Named || cast.result.name.empty()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultValue);
  }
  if (!scalar_register_view(cast.result.type).has_value() ||
      !scalar_register_view(cast.operand.type).has_value()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandType);
  }

  const auto* result_home = find_named_value_home(names, value_locations, cast.result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register ||
      (!result_storage->register_placement.has_value() &&
       (!result_home->register_name.has_value() || !result_storage->register_name.has_value() ||
        *result_home->register_name != *result_storage->register_name))) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultStorage);
  }

  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, cast.operand, source);
      error != PreparedScalarAluRecordError::None) {
    return scalar_cast_record_error(scalar_cast_operand_error_from_alu_error(error));
  }

  return PreparedScalarCastRecordResult{
      .record =
          ScalarCastRecord{
              .surface = RecordSurfaceKind::RecordOnly,
              .operation = scalar_cast_operation_from_cast_opcode(cast.opcode),
              .source_cast_opcode = cast.opcode,
              .source_type = cast.operand.type,
              .result_value_id = result_home->value_id,
              .result_value_name = result_home->value_name,
              .result_type = cast.result.type,
              .source = source,
              .supported_simple_integer_cast = true,
          },
      .error = PreparedScalarCastRecordError::None,
  };
}

PreparedScalarCastInstructionRecordResult make_prepared_scalar_cast_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast) {
  const auto result = make_prepared_scalar_cast_record(names, value_locations, storage_plan, cast);
  if (!result.record.has_value()) {
    return scalar_cast_instruction_record_error(result.error);
  }
  return PreparedScalarCastInstructionRecordResult{
      .record = make_scalar_cast_instruction_record(*result.record),
      .error = PreparedScalarCastRecordError::None,
  };
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          load.address ? &*load.address : nullptr,
          c4c::kInvalidLinkName,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          load.address ? &*load.address : nullptr,
          load.byte_offset,
          load.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_load_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          load.result,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          store.address ? &*store.address : nullptr,
          c4c::kInvalidLinkName,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          store.address ? &*store.address : nullptr,
          store.byte_offset,
          store.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_store_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          store.value,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          load.address ? &*load.address : nullptr,
          load.global_name_id,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          load.address ? &*load.address : nullptr,
          load.byte_offset,
          load.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_load_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          load.result,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store) {
  auto result = make_memory_record_from_prepared_access(
      names, value_locations, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return result;
  }
  if (const auto error = validate_memory_base_identity(
          names,
          value_locations,
          store.address ? &*store.address : nullptr,
          store.global_name_id,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = validate_memory_instruction_facts(
          store.address ? &*store.address : nullptr,
          store.byte_offset,
          store.align_bytes,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  if (const auto error = apply_store_identity(
          names,
          value_locations,
          *prepare::find_prepared_memory_access(
              addressing, block_label, instruction_index),
          store.value,
          *result.record);
      error != PreparedMemoryOperandRecordError::None) {
    return memory_operand_record_error(error);
  }
  return result;
}

}  // namespace c4c::backend::aarch64::codegen

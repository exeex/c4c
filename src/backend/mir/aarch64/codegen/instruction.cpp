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
    case InstructionFamily::I128Transport:
      return "i128_transport";
    case InstructionFamily::F128Transport:
      return "f128_transport";
    case InstructionFamily::I128Pair:
      return "i128_pair";
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
    case MachineOpcode::AddressMaterialization:
      return "address_materialization";
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
    case MachineOpcode::I128Transport:
      return "i128_transport";
    case MachineOpcode::F128Transport:
      return "f128_transport";
    case MachineOpcode::F128RuntimeHelper:
      return "f128_runtime_helper";
    case MachineOpcode::I128Pair:
      return "i128_pair";
    case MachineOpcode::I128Shift:
      return "i128_shift";
    case MachineOpcode::I128Compare:
      return "i128_compare";
    case MachineOpcode::I128RuntimeHelper:
      return "i128_runtime_helper";
    case MachineOpcode::Add:
      return "add";
    case MachineOpcode::Sub:
      return "sub";
    case MachineOpcode::Mul:
      return "mul";
    case MachineOpcode::Div:
      return "div";
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
    case MachineOpcode::AtomicLoad:
      return "atomic_load";
    case MachineOpcode::AtomicStore:
      return "atomic_store";
    case MachineOpcode::AtomicFence:
      return "atomic_fence";
    case MachineOpcode::AtomicRmw:
      return "atomic_rmw";
    case MachineOpcode::AtomicCompareExchange:
      return "atomic_compare_exchange";
    case MachineOpcode::SpillToSlot:
      return "spill_to_slot";
    case MachineOpcode::ReloadFromSlot:
      return "reload_from_slot";
    case MachineOpcode::VariadicVaStart:
      return "variadic_va_start";
    case MachineOpcode::VariadicVaArgScalar:
      return "variadic_va_arg_scalar";
    case MachineOpcode::VariadicVaArgAggregate:
      return "variadic_va_arg_aggregate";
    case MachineOpcode::VariadicVaCopy:
      return "variadic_va_copy";
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
    case MachinePrinterMnemonicKind::VariadicVaStart:
      return "va.start";
    case MachinePrinterMnemonicKind::VariadicVaArgScalar:
      return "va.arg.scalar";
    case MachinePrinterMnemonicKind::VariadicVaArgAggregate:
      return "va.arg.aggregate";
    case MachinePrinterMnemonicKind::VariadicVaCopy:
      return "va.copy";
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
    case MachineOpcode::VariadicVaStart:
      return MachinePrinterMnemonicKind::VariadicVaStart;
    case MachineOpcode::VariadicVaArgScalar:
      return MachinePrinterMnemonicKind::VariadicVaArgScalar;
    case MachineOpcode::VariadicVaArgAggregate:
      return MachinePrinterMnemonicKind::VariadicVaArgAggregate;
    case MachineOpcode::VariadicVaCopy:
      return MachinePrinterMnemonicKind::VariadicVaCopy;
    case MachineOpcode::Unspecified:
    case MachineOpcode::CompareBranch:
    case MachineOpcode::AddressMaterialization:
    case MachineOpcode::CalleeSaveStore:
    case MachineOpcode::CalleeSaveLoad:
    case MachineOpcode::CallBoundaryAbiBinding:
    case MachineOpcode::I128Transport:
    case MachineOpcode::F128Transport:
    case MachineOpcode::F128RuntimeHelper:
    case MachineOpcode::I128Pair:
    case MachineOpcode::I128Shift:
    case MachineOpcode::I128Compare:
    case MachineOpcode::I128RuntimeHelper:
    case MachineOpcode::AtomicLoad:
    case MachineOpcode::AtomicStore:
    case MachineOpcode::AtomicFence:
    case MachineOpcode::AtomicRmw:
    case MachineOpcode::AtomicCompareExchange:
    case MachineOpcode::And:
    case MachineOpcode::Mul:
    case MachineOpcode::Div:
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
    case MachineSideEffectKind::AtomicMemoryAccess:
      return "atomic_memory_access";
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
    case ScalarAluOperationKind::Mul:
      return "mul";
    case ScalarAluOperationKind::Div:
      return "div";
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
    case ScalarCastOperationKind::FloatExtend:
      return "float_extend";
    case ScalarCastOperationKind::FloatTruncate:
      return "float_truncate";
    case ScalarCastOperationKind::SignedIntToFloat:
      return "signed_int_to_float";
    case ScalarCastOperationKind::UnsignedIntToFloat:
      return "unsigned_int_to_float";
    case ScalarCastOperationKind::FloatToSignedInt:
      return "float_to_signed_int";
    case ScalarCastOperationKind::FloatToUnsignedInt:
      return "float_to_unsigned_int";
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
    case PreparedMemoryOperandRecordError::MissingPointerValueStorage:
      return "missing_pointer_value_storage";
    case PreparedMemoryOperandRecordError::UnsupportedPointerValueStorage:
      return "unsupported_pointer_value_storage";
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
    case PreparedMemoryOperandRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedMemoryOperandRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedMemoryOperandRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedMemoryOperandRecordError::MissingStoredValueHome:
      return "missing_stored_value_home";
    case PreparedMemoryOperandRecordError::MissingStoredStorage:
      return "missing_stored_storage";
    case PreparedMemoryOperandRecordError::UnsupportedStoredStorage:
      return "unsupported_stored_storage";
    case PreparedMemoryOperandRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
  }
  return "unknown";
}

std::string_view prepared_atomic_operation_record_error_name(
    PreparedAtomicOperationRecordError error) {
  switch (error) {
    case PreparedAtomicOperationRecordError::None:
      return "none";
    case PreparedAtomicOperationRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedAtomicOperationRecordError::MissingPreparedAtomicOperation:
      return "missing_prepared_atomic_operation";
    case PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation:
      return "incomplete_prepared_atomic_operation";
    case PreparedAtomicOperationRecordError::UnsupportedOperationKind:
      return "unsupported_operation_kind";
    case PreparedAtomicOperationRecordError::UnsupportedOrdering:
      return "unsupported_ordering";
    case PreparedAtomicOperationRecordError::UnsupportedFailureOrdering:
      return "unsupported_failure_ordering";
    case PreparedAtomicOperationRecordError::UnsupportedWidth:
      return "unsupported_width";
    case PreparedAtomicOperationRecordError::UnsupportedRmwOpcode:
      return "unsupported_rmw_opcode";
    case PreparedAtomicOperationRecordError::UnsupportedResultMode:
      return "unsupported_result_mode";
    case PreparedAtomicOperationRecordError::MissingPointerValueName:
      return "missing_pointer_value_name";
    case PreparedAtomicOperationRecordError::MissingPointerValueHome:
      return "missing_pointer_value_home";
    case PreparedAtomicOperationRecordError::MissingPointerValueStorage:
      return "missing_pointer_value_storage";
    case PreparedAtomicOperationRecordError::MissingResultValueName:
      return "missing_result_value_name";
    case PreparedAtomicOperationRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedAtomicOperationRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedAtomicOperationRecordError::MissingStoredValueName:
      return "missing_stored_value_name";
    case PreparedAtomicOperationRecordError::MissingStoredValueHome:
      return "missing_stored_value_home";
    case PreparedAtomicOperationRecordError::MissingStoredStorage:
      return "missing_stored_storage";
    case PreparedAtomicOperationRecordError::MissingExpectedValueName:
      return "missing_expected_value_name";
    case PreparedAtomicOperationRecordError::MissingExpectedValueHome:
      return "missing_expected_value_home";
    case PreparedAtomicOperationRecordError::MissingExpectedStorage:
      return "missing_expected_storage";
    case PreparedAtomicOperationRecordError::MissingDesiredValueName:
      return "missing_desired_value_name";
    case PreparedAtomicOperationRecordError::MissingDesiredValueHome:
      return "missing_desired_value_home";
    case PreparedAtomicOperationRecordError::MissingDesiredStorage:
      return "missing_desired_storage";
    case PreparedAtomicOperationRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
  }
  return "unknown";
}

std::string_view atomic_memory_instruction_kind_name(
    AtomicMemoryInstructionKind kind) {
  switch (kind) {
    case AtomicMemoryInstructionKind::Load:
      return "load";
    case AtomicMemoryInstructionKind::Store:
      return "store";
    case AtomicMemoryInstructionKind::Fence:
      return "fence";
    case AtomicMemoryInstructionKind::RmwLoop:
      return "rmw_loop";
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return "compare_exchange_loop";
  }
  return "unknown";
}

std::string_view i128_transport_kind_name(I128TransportKind kind) {
  switch (kind) {
    case I128TransportKind::CarrierSnapshot:
      return "carrier_snapshot";
    case I128TransportKind::LoadFromMemory:
      return "load_from_memory";
    case I128TransportKind::StoreToMemory:
      return "store_to_memory";
  }
  return "unknown";
}

std::string_view f128_transport_kind_name(F128TransportKind kind) {
  switch (kind) {
    case F128TransportKind::CarrierSnapshot:
      return "carrier_snapshot";
    case F128TransportKind::LoadFromMemory:
      return "load_from_memory";
    case F128TransportKind::StoreToMemory:
      return "store_to_memory";
  }
  return "unknown";
}

std::string_view prepared_i128_transport_record_error_name(
    PreparedI128TransportRecordError error) {
  switch (error) {
    case PreparedI128TransportRecordError::None:
      return "none";
    case PreparedI128TransportRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128TransportRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128TransportRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128TransportRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128TransportRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128TransportRecordError::MissingMemoryOperand:
      return "missing_memory_operand";
    case PreparedI128TransportRecordError::MemoryAccessSizeMismatch:
      return "memory_access_size_mismatch";
  }
  return "unknown";
}

std::string_view prepared_f128_transport_record_error_name(
    PreparedF128TransportRecordError error) {
  switch (error) {
    case PreparedF128TransportRecordError::None:
      return "none";
    case PreparedF128TransportRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedF128TransportRecordError::MissingPreparedF128Carrier:
      return "missing_prepared_f128_carrier";
    case PreparedF128TransportRecordError::IncompletePreparedF128Carrier:
      return "incomplete_prepared_f128_carrier";
    case PreparedF128TransportRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedF128TransportRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedF128TransportRecordError::MissingMemoryOperand:
      return "missing_memory_operand";
    case PreparedF128TransportRecordError::MemoryAccessSizeMismatch:
      return "memory_access_size_mismatch";
  }
  return "unknown";
}

std::string_view i128_pair_operation_kind_name(I128PairOperationKind kind) {
  switch (kind) {
    case I128PairOperationKind::Add:
      return "add";
    case I128PairOperationKind::Sub:
      return "sub";
    case I128PairOperationKind::And:
      return "and";
    case I128PairOperationKind::Or:
      return "or";
    case I128PairOperationKind::Xor:
      return "xor";
  }
  return "unknown";
}

std::string_view i128_pair_lane_semantics_name(
    I128PairLaneSemantics semantics) {
  switch (semantics) {
    case I128PairLaneSemantics::CarryPropagating:
      return "carry_propagating";
    case I128PairLaneSemantics::BorrowPropagating:
      return "borrow_propagating";
    case I128PairLaneSemantics::IndependentBitwise:
      return "independent_bitwise";
  }
  return "unknown";
}

std::string_view i128_shift_kind_name(I128ShiftKind kind) {
  switch (kind) {
    case I128ShiftKind::Left:
      return "left";
    case I128ShiftKind::LogicalRight:
      return "logical_right";
    case I128ShiftKind::ArithmeticRight:
      return "arithmetic_right";
  }
  return "unknown";
}

std::string_view i128_shift_lane_semantics_name(
    I128ShiftLaneSemantics semantics) {
  switch (semantics) {
    case I128ShiftLaneSemantics::CrossLaneLeft:
      return "cross_lane_left";
    case I128ShiftLaneSemantics::CrossLaneLogicalRight:
      return "cross_lane_logical_right";
    case I128ShiftLaneSemantics::CrossLaneArithmeticRight:
      return "cross_lane_arithmetic_right";
  }
  return "unknown";
}

std::string_view i128_shift_count_kind_name(I128ShiftCountKind kind) {
  switch (kind) {
    case I128ShiftCountKind::Immediate:
      return "immediate";
    case I128ShiftCountKind::Register:
      return "register";
  }
  return "unknown";
}

std::string_view i128_compare_signedness_name(
    I128CompareSignedness signedness) {
  switch (signedness) {
    case I128CompareSignedness::Equality:
      return "equality";
    case I128CompareSignedness::Signed:
      return "signed";
    case I128CompareSignedness::Unsigned:
      return "unsigned";
  }
  return "unknown";
}

std::string_view i128_compare_high_word_semantics_name(
    I128CompareHighWordSemantics semantics) {
  switch (semantics) {
    case I128CompareHighWordSemantics::EqualityBothLanes:
      return "equality_both_lanes";
    case I128CompareHighWordSemantics::SignedHighWordFirst:
      return "signed_high_word_first";
    case I128CompareHighWordSemantics::UnsignedHighWordFirst:
      return "unsigned_high_word_first";
  }
  return "unknown";
}

std::string_view i128_runtime_helper_boundary_kind_name(
    I128RuntimeHelperBoundaryKind kind) {
  switch (kind) {
    case I128RuntimeHelperBoundaryKind::SignedDiv:
      return "signed_div";
    case I128RuntimeHelperBoundaryKind::UnsignedDiv:
      return "unsigned_div";
    case I128RuntimeHelperBoundaryKind::SignedRem:
      return "signed_rem";
    case I128RuntimeHelperBoundaryKind::UnsignedRem:
      return "unsigned_rem";
  }
  return "unknown";
}

std::string_view f128_runtime_helper_boundary_kind_name(
    F128RuntimeHelperBoundaryKind kind) {
  switch (kind) {
    case F128RuntimeHelperBoundaryKind::Add:
      return "add";
    case F128RuntimeHelperBoundaryKind::Sub:
      return "sub";
    case F128RuntimeHelperBoundaryKind::Mul:
      return "mul";
    case F128RuntimeHelperBoundaryKind::Div:
      return "div";
    case F128RuntimeHelperBoundaryKind::Eq:
      return "eq";
    case F128RuntimeHelperBoundaryKind::Ne:
      return "ne";
    case F128RuntimeHelperBoundaryKind::Lt:
      return "lt";
    case F128RuntimeHelperBoundaryKind::Le:
      return "le";
    case F128RuntimeHelperBoundaryKind::Gt:
      return "gt";
    case F128RuntimeHelperBoundaryKind::Ge:
      return "ge";
    case F128RuntimeHelperBoundaryKind::F32ToF128:
      return "f32_to_f128";
    case F128RuntimeHelperBoundaryKind::F64ToF128:
      return "f64_to_f128";
    case F128RuntimeHelperBoundaryKind::F128ToF32:
      return "f128_to_f32";
    case F128RuntimeHelperBoundaryKind::F128ToF64:
      return "f128_to_f64";
  }
  return "unknown";
}

std::string_view prepared_i128_pair_record_error_name(
    PreparedI128PairRecordError error) {
  switch (error) {
    case PreparedI128PairRecordError::None:
      return "none";
    case PreparedI128PairRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128PairRecordError::UnsupportedOpcode:
      return "unsupported_opcode";
    case PreparedI128PairRecordError::UnsupportedOperandType:
      return "unsupported_operand_type";
    case PreparedI128PairRecordError::UnsupportedResultValue:
      return "unsupported_result_value";
    case PreparedI128PairRecordError::UnsupportedOperandValue:
      return "unsupported_operand_value";
    case PreparedI128PairRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128PairRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128PairRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128PairRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128PairRecordError::MissingScalarResultValueHome:
      return "missing_scalar_result_value_home";
    case PreparedI128PairRecordError::MissingScalarResultStorage:
      return "missing_scalar_result_storage";
    case PreparedI128PairRecordError::UnsupportedScalarResultStorage:
      return "unsupported_scalar_result_storage";
    case PreparedI128PairRecordError::UnsupportedShiftCount:
      return "unsupported_shift_count";
    case PreparedI128PairRecordError::MissingShiftCountStorage:
      return "missing_shift_count_storage";
  }
  return "unknown";
}

std::string_view prepared_i128_runtime_helper_record_error_name(
    PreparedI128RuntimeHelperRecordError error) {
  switch (error) {
    case PreparedI128RuntimeHelperRecordError::None:
      return "none";
    case PreparedI128RuntimeHelperRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedI128RuntimeHelperRecordError::MissingPreparedI128RuntimeHelper:
      return "missing_prepared_i128_runtime_helper";
    case PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper:
      return "incomplete_prepared_i128_runtime_helper";
    case PreparedI128RuntimeHelperRecordError::UnsupportedHelperFamily:
      return "unsupported_helper_family";
    case PreparedI128RuntimeHelperRecordError::UnsupportedSourceOperation:
      return "unsupported_source_operation";
    case PreparedI128RuntimeHelperRecordError::UnsupportedResultOwnership:
      return "unsupported_result_ownership";
    case PreparedI128RuntimeHelperRecordError::MissingPreparedI128Carrier:
      return "missing_prepared_i128_carrier";
    case PreparedI128RuntimeHelperRecordError::IncompletePreparedI128Carrier:
      return "incomplete_prepared_i128_carrier";
    case PreparedI128RuntimeHelperRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedI128RuntimeHelperRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedI128RuntimeHelperRecordError::MissingBoundaryResourcePolicy:
      return "missing_boundary_resource_policy";
    case PreparedI128RuntimeHelperRecordError::MissingBoundaryAbiPolicy:
      return "missing_boundary_abi_policy";
    case PreparedI128RuntimeHelperRecordError::MissingClobberPolicy:
      return "missing_clobber_policy";
  }
  return "unknown";
}

std::string_view prepared_f128_runtime_helper_record_error_name(
    PreparedF128RuntimeHelperRecordError error) {
  switch (error) {
    case PreparedF128RuntimeHelperRecordError::None:
      return "none";
    case PreparedF128RuntimeHelperRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedF128RuntimeHelperRecordError::MissingPreparedF128RuntimeHelper:
      return "missing_prepared_f128_runtime_helper";
    case PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper:
      return "incomplete_prepared_f128_runtime_helper";
    case PreparedF128RuntimeHelperRecordError::UnsupportedHelperFamily:
      return "unsupported_helper_family";
    case PreparedF128RuntimeHelperRecordError::UnsupportedSourceOperation:
      return "unsupported_source_operation";
    case PreparedF128RuntimeHelperRecordError::UnsupportedResultOwnership:
      return "unsupported_result_ownership";
    case PreparedF128RuntimeHelperRecordError::MissingPreparedF128Carrier:
      return "missing_prepared_f128_carrier";
    case PreparedF128RuntimeHelperRecordError::IncompletePreparedF128Carrier:
      return "incomplete_prepared_f128_carrier";
    case PreparedF128RuntimeHelperRecordError::UnsupportedCarrierKind:
      return "unsupported_carrier_kind";
    case PreparedF128RuntimeHelperRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedF128RuntimeHelperRecordError::MissingBoundaryResourcePolicy:
      return "missing_boundary_resource_policy";
    case PreparedF128RuntimeHelperRecordError::MissingBoundaryAbiPolicy:
      return "missing_boundary_abi_policy";
    case PreparedF128RuntimeHelperRecordError::MissingClobberPolicy:
      return "missing_clobber_policy";
  }
  return "unknown";
}

std::string_view address_materialization_kind_name(AddressMaterializationKind kind) {
  switch (kind) {
    case AddressMaterializationKind::DirectPageLow12:
      return "direct_page_low12";
    case AddressMaterializationKind::GotPageLow12:
      return "got_page_low12";
    case AddressMaterializationKind::TlsRelative:
      return "tls_relative";
    case AddressMaterializationKind::StringConstant:
      return "string_constant";
    case AddressMaterializationKind::LabelPageLow12:
      return "label_page_low12";
    case AddressMaterializationKind::DeferredUnsupported:
      return "deferred_unsupported";
  }
  return "unknown";
}

std::string_view prepared_address_materialization_record_error_name(
    PreparedAddressMaterializationRecordError error) {
  switch (error) {
    case PreparedAddressMaterializationRecordError::None:
      return "none";
    case PreparedAddressMaterializationRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedAddressMaterializationRecordError::MissingPreparedAddressMaterialization:
      return "missing_prepared_address_materialization";
    case PreparedAddressMaterializationRecordError::UnsupportedAddressKind:
      return "unsupported_address_kind";
    case PreparedAddressMaterializationRecordError::MissingResultValueName:
      return "missing_result_value_name";
    case PreparedAddressMaterializationRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedAddressMaterializationRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedAddressMaterializationRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedAddressMaterializationRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
    case PreparedAddressMaterializationRecordError::MissingSymbolIdentity:
      return "missing_symbol_identity";
    case PreparedAddressMaterializationRecordError::MissingStringIdentity:
      return "missing_string_identity";
    case PreparedAddressMaterializationRecordError::MissingLabelIdentity:
      return "missing_label_identity";
    case PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy:
      return "missing_address_materialization_policy";
    case PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch:
      return "address_materialization_policy_mismatch";
    case PreparedAddressMaterializationRecordError::MissingTlsMaterializationFacts:
      return "missing_tls_materialization_facts";
    case PreparedAddressMaterializationRecordError::TlsFactMismatch:
      return "tls_fact_mismatch";
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

bool is_scalar_alu_floating_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
      return true;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
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

bool is_scalar_alu_floating_type(bir::TypeKind type) {
  return type == bir::TypeKind::F32 || type == bir::TypeKind::F64;
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

bool is_supported_scalar_conversion_cast_opcode(bir::CastOpcode opcode) {
  switch (opcode) {
    case bir::CastOpcode::FPTrunc:
    case bir::CastOpcode::FPExt:
    case bir::CastOpcode::FPToSI:
    case bir::CastOpcode::FPToUI:
    case bir::CastOpcode::SIToFP:
    case bir::CastOpcode::UIToFP:
      return true;
    case bir::CastOpcode::SExt:
    case bir::CastOpcode::ZExt:
    case bir::CastOpcode::Trunc:
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
    case bir::BinaryOpcode::Mul:
      return ScalarAluOperationKind::Mul;
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
      return ScalarAluOperationKind::Div;
    case bir::BinaryOpcode::And:
      return ScalarAluOperationKind::And;
    case bir::BinaryOpcode::Or:
      return ScalarAluOperationKind::Or;
    case bir::BinaryOpcode::Xor:
      return ScalarAluOperationKind::Xor;
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::AShr:
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
      return ScalarCastOperationKind::FloatTruncate;
    case bir::CastOpcode::FPExt:
      return ScalarCastOperationKind::FloatExtend;
    case bir::CastOpcode::FPToSI:
      return ScalarCastOperationKind::FloatToSignedInt;
    case bir::CastOpcode::FPToUI:
      return ScalarCastOperationKind::FloatToUnsignedInt;
    case bir::CastOpcode::SIToFP:
      return ScalarCastOperationKind::SignedIntToFloat;
    case bir::CastOpcode::UIToFP:
      return ScalarCastOperationKind::UnsignedIntToFloat;
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

PreparedMemoryInstructionRecordResult memory_instruction_record_error(
    PreparedMemoryOperandRecordError error) {
  return PreparedMemoryInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedAddressMaterializationRecordResult address_materialization_record_error(
    PreparedAddressMaterializationRecordError error) {
  return PreparedAddressMaterializationRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarInstructionRecordResult scalar_instruction_record_error(
    PreparedScalarAluRecordError error) {
  return PreparedScalarInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedScalarCastInstructionRecordResult scalar_cast_instruction_record_error(
    PreparedScalarCastRecordError error) {
  return PreparedScalarCastInstructionRecordResult{.record = std::nullopt, .error = error};
}

PreparedAddressMaterializationInstructionRecordResult
address_materialization_instruction_record_error(
    PreparedAddressMaterializationRecordError error) {
  return PreparedAddressMaterializationInstructionRecordResult{
      .record = std::nullopt,
      .error = error,
  };
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

std::optional<abi::RegisterView> scalar_fp_register_view(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::F32:
      return abi::RegisterView::S;
    case bir::TypeKind::F64:
      return abi::RegisterView::D;
    case bir::TypeKind::Void:
    case bir::TypeKind::I1:
    case bir::TypeKind::I8:
    case bir::TypeKind::I16:
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
    case bir::TypeKind::I128:
    case bir::TypeKind::Ptr:
    case bir::TypeKind::F128:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> scalar_storage_register_view(bir::TypeKind type) {
  if (const auto integer_view = scalar_register_view(type)) {
    return integer_view;
  }
  return scalar_fp_register_view(type);
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

bool same_gp_allocation_register(abi::RegisterReference lhs,
                                 abi::RegisterReference rhs) {
  return lhs.bank == abi::RegisterBank::GeneralPurpose &&
         rhs.bank == abi::RegisterBank::GeneralPurpose &&
         lhs.index == rhs.index;
}

bool record_uses_gp_register(const AtomicMemoryInstructionRecord& record,
                             abi::RegisterReference candidate) {
  const auto used_by = [&](const std::optional<RegisterOperand>& operand) {
    return operand.has_value() && same_gp_allocation_register(operand->reg, candidate);
  };
  return used_by(record.pointer_register) || used_by(record.result_register) ||
         used_by(record.stored_register) || used_by(record.expected_register) ||
         used_by(record.desired_register) ||
         used_by(record.exclusive_status_register) ||
         used_by(record.rmw_new_value_register) ||
         used_by(record.compare_loaded_register);
}

std::optional<RegisterOperand> next_reserved_gp_scratch_operand(
    const AtomicMemoryInstructionRecord& record,
    abi::RegisterView expected_view) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (record_uses_gp_register(record, scratch)) {
      continue;
    }
    const auto viewed = abi::gp_register(scratch.index, expected_view);
    if (!viewed.has_value()) {
      continue;
    }
    return RegisterOperand{
        .reg = *viewed,
        .role = RegisterOperandRole::ReservedMirScratch,
        .prepared_class = prepare::PreparedRegisterClass::General,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = expected_view,
        .contiguous_width = 1,
        .occupied_register_references = occupied_register_references(*viewed),
        .occupied_registers = occupied_register_views(*viewed),
    };
  }
  return std::nullopt;
}

std::optional<abi::RegisterView> atomic_value_register_view(std::size_t width_bytes) {
  if (width_bytes == 0 || width_bytes > 8) {
    return std::nullopt;
  }
  return width_bytes == 8 ? abi::RegisterView::X : abi::RegisterView::W;
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
  if (preserved.route == prepare::PreparedCallPreservationRoute::StackSlot) {
    if (!preserved.slot_id.has_value() ||
        !preserved.stack_offset_bytes.has_value() ||
        !preserved.stack_size_bytes.has_value() ||
        *preserved.stack_size_bytes == 0 ||
        !preserved.stack_align_bytes.has_value() ||
        *preserved.stack_align_bytes == 0) {
      return std::nullopt;
    }

    const OperandRecord operand = make_memory_operand(MemoryOperand{
        .support = MemoryOperandSupportKind::Prepared,
        .base_kind = MemoryBaseKind::FrameSlot,
        .frame_slot_id = preserved.slot_id,
        .byte_offset = static_cast<std::int64_t>(*preserved.stack_offset_bytes),
        .byte_offset_is_prepared_snapshot = true,
        .size_bytes = *preserved.stack_size_bytes,
        .align_bytes = *preserved.stack_align_bytes,
        .can_use_base_plus_offset = true,
    });
    return MachineEffectResource{
        .kind = MachineEffectResourceKind::Memory,
        .operand = operand,
        .value_id = preserved.value_id,
        .value_name = preserved.value_name,
        .frame_slot_id = preserved.slot_id,
    };
  }
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

  const auto expected_view = scalar_storage_register_view(type);
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

std::optional<AddressMaterializationKind> selected_address_materialization_kind(
    prepare::PreparedAddressMaterializationKind kind) {
  switch (kind) {
    case prepare::PreparedAddressMaterializationKind::DirectGlobal:
      return AddressMaterializationKind::DirectPageLow12;
    case prepare::PreparedAddressMaterializationKind::GotGlobal:
      return AddressMaterializationKind::GotPageLow12;
    case prepare::PreparedAddressMaterializationKind::TlsGlobal:
      return AddressMaterializationKind::TlsRelative;
    case prepare::PreparedAddressMaterializationKind::StringConstant:
      return AddressMaterializationKind::StringConstant;
    case prepare::PreparedAddressMaterializationKind::Label:
      return AddressMaterializationKind::LabelPageLow12;
    case prepare::PreparedAddressMaterializationKind::None:
      return std::nullopt;
  }
  return std::nullopt;
}

PreparedAddressMaterializationRecordError validate_address_materialization_identity(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedAddressMaterialization& materialization,
    AddressMaterializationRecord& record) {
  switch (materialization.kind) {
    case prepare::PreparedAddressMaterializationKind::DirectGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::TlsGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::Direct) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      if (!materialization.is_thread_local || !materialization.has_tls_address_space ||
          materialization.address_space != bir::AddressSpace::Tls) {
        return PreparedAddressMaterializationRecordError::TlsFactMismatch;
      }
      if (materialization.tls_model !=
              prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
          materialization.tls_thread_pointer_register !=
              prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
          materialization.tls_high_relocation !=
              prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
          materialization.tls_low_relocation !=
              prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc) {
        return PreparedAddressMaterializationRecordError::MissingTlsMaterializationFacts;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::StringConstant:
      if (!materialization.text_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingStringIdentity;
      }
      record.text_name = materialization.text_name;
      record.text_label = names.texts.lookup(*materialization.text_name);
      if (record.text_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingStringIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::Label:
      if (!materialization.target_label.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingLabelIdentity;
      }
      record.target_label = materialization.target_label;
      record.target_label_name = prepare::prepared_block_label(names, *materialization.target_label);
      if (record.target_label_name.empty()) {
        return PreparedAddressMaterializationRecordError::MissingLabelIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::GotGlobal:
      if (materialization.address_materialization_policy ==
          bir::GlobalAddressMaterializationPolicy::Unspecified) {
        return PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy;
      }
      if (materialization.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
        return PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch;
      }
      if (!materialization.symbol_name.has_value()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      if (materialization.is_thread_local || materialization.has_tls_address_space ||
          materialization.address_space == bir::AddressSpace::Tls) {
        return PreparedAddressMaterializationRecordError::TlsFactMismatch;
      }
      record.symbol_name = materialization.symbol_name;
      record.symbol_label = prepare::prepared_link_name(names, *materialization.symbol_name);
      if (record.symbol_label.empty()) {
        return PreparedAddressMaterializationRecordError::MissingSymbolIdentity;
      }
      return PreparedAddressMaterializationRecordError::None;
    case prepare::PreparedAddressMaterializationKind::None:
      return PreparedAddressMaterializationRecordError::UnsupportedAddressKind;
  }
  return PreparedAddressMaterializationRecordError::UnsupportedAddressKind;
}

PreparedAddressMaterializationRecordResult make_address_record_from_prepared_materialization(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  if (addressing.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != addressing.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::InvalidFunction);
  }
  const auto* materialization =
      prepare::find_prepared_address_materialization(addressing, block_label, instruction_index);
  if (materialization == nullptr || materialization->function_name != addressing.function_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingPreparedAddressMaterialization);
  }

  const auto selected_kind = selected_address_materialization_kind(materialization->kind);
  if (!selected_kind.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedAddressKind);
  }

  AddressMaterializationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .kind = *selected_kind,
      .prepared_kind = materialization->kind,
      .function_name = materialization->function_name,
      .block_label = materialization->block_label,
      .instruction_index = materialization->inst_index,
      .address_materialization_policy = materialization->address_materialization_policy,
      .byte_offset = materialization->byte_offset,
      .address_space = materialization->address_space,
      .is_thread_local = materialization->is_thread_local,
      .has_tls_address_space = materialization->has_tls_address_space,
      .tls_model = materialization->tls_model,
      .tls_thread_pointer_register = materialization->tls_thread_pointer_register,
      .tls_high_relocation = materialization->tls_high_relocation,
      .tls_low_relocation = materialization->tls_low_relocation,
      .source_materialization = materialization,
  };

  const auto identity_error =
      validate_address_materialization_identity(names, *materialization, record);
  if (identity_error != PreparedAddressMaterializationRecordError::None) {
    return address_materialization_record_error(identity_error);
  }
  if (record.kind == AddressMaterializationKind::DeferredUnsupported) {
    record.result_value_id = materialization->result_value_id;
    record.result_value_name =
        materialization->result_value_name.value_or(c4c::kInvalidValueName);
    record.result_home_kind = materialization->result_home_kind.value_or(
        prepare::PreparedValueHomeKind::None);
    return PreparedAddressMaterializationRecordResult{
        .record = record,
        .error = PreparedAddressMaterializationRecordError::None,
    };
  }
  if (!materialization->result_value_name.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueName);
  }
  const auto result_value_name = *materialization->result_value_name;
  const auto* result_home = prepare::find_prepared_value_home(value_locations, result_value_name);
  if (result_home == nullptr) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::MissingResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::UnsupportedResultStorage);
  }
  auto result_register =
      make_prepared_register_operand(*result_home,
                                     *result_storage,
                                     bir::TypeKind::Ptr,
                                     RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return address_materialization_record_error(
        PreparedAddressMaterializationRecordError::RegisterConversionFailed);
  }

  record.result_value_id = result_home->value_id;
  record.result_value_name = result_home->value_name;
  record.result_home_kind = result_home->kind;
  record.result_register = *result_register;
  return PreparedAddressMaterializationRecordResult{
      .record = record,
      .error = PreparedAddressMaterializationRecordError::None,
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
    case ScalarAluOperationKind::Mul:
      return MachineOpcode::Mul;
    case ScalarAluOperationKind::Div:
      return MachineOpcode::Div;
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
    case ScalarCastOperationKind::FloatExtend:
    case ScalarCastOperationKind::FloatTruncate:
    case ScalarCastOperationKind::SignedIntToFloat:
    case ScalarCastOperationKind::UnsignedIntToFloat:
    case ScalarCastOperationKind::FloatToSignedInt:
    case ScalarCastOperationKind::FloatToUnsignedInt:
      return MachineOpcode::Unspecified;
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

MachineOpcode machine_opcode_from_atomic_memory_instruction(
    const AtomicMemoryInstructionRecord& instruction) {
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      return MachineOpcode::AtomicLoad;
    case AtomicMemoryInstructionKind::Store:
      return MachineOpcode::AtomicStore;
    case AtomicMemoryInstructionKind::Fence:
      return MachineOpcode::AtomicFence;
    case AtomicMemoryInstructionKind::RmwLoop:
      return MachineOpcode::AtomicRmw;
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return MachineOpcode::AtomicCompareExchange;
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_address_materialization(
    const AddressMaterializationRecord& instruction) {
  switch (instruction.kind) {
    case AddressMaterializationKind::DirectPageLow12:
    case AddressMaterializationKind::GotPageLow12:
    case AddressMaterializationKind::TlsRelative:
    case AddressMaterializationKind::StringConstant:
    case AddressMaterializationKind::LabelPageLow12:
      return MachineOpcode::AddressMaterialization;
    case AddressMaterializationKind::DeferredUnsupported:
      return MachineOpcode::Unspecified;
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

std::vector<MachineSideEffectKind> atomic_memory_side_effects(
    const AtomicMemoryInstructionRecord& instruction) {
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::Store:
      return {MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::Fence:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
    case AtomicMemoryInstructionKind::RmwLoop:
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      return {MachineSideEffectKind::MemoryRead,
              MachineSideEffectKind::MemoryWrite,
              MachineSideEffectKind::AtomicMemoryAccess};
  }
  return {};
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
    if ((instruction.scalar_alu->supported_integer_operation ||
         instruction.scalar_alu->supported_floating_operation) &&
        instruction.scalar_alu->operation != ScalarAluOperationKind::Deferred) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::DeferredUnsupported,
                                   .diagnostic =
                                       "scalar ALU operation is outside the selected subset"};
  }
  if (instruction.scalar_cast.has_value()) {
    if ((instruction.scalar_cast->supported_simple_integer_cast ||
         instruction.scalar_cast->supported_float_integer_conversion ||
         instruction.scalar_cast->supported_float_width_conversion) &&
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

MachineNodeStatusRecord atomic_memory_selection_status(
    const AtomicMemoryInstructionRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "atomic memory node is missing prepared atomic carrier provenance"};
  }
  if (instruction.source_carrier->carrier_kind !=
      prepare::PreparedAtomicOperationCarrierKind::Complete) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "atomic memory node requires a complete prepared carrier"};
  }
  switch (instruction.atomic_kind) {
    case AtomicMemoryInstructionKind::Load:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic load is missing pointer or result register authority"};
      }
      break;
    case AtomicMemoryInstructionKind::Store:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.stored_value_id.has_value() ||
          !instruction.stored_value_name.has_value() ||
          !instruction.stored_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic store is missing pointer or stored register authority"};
      }
      break;
    case AtomicMemoryInstructionKind::Fence:
      if (!instruction.memory_barrier_required) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::DeferredUnsupported,
            .diagnostic = "relaxed atomic fence is outside the selected subset"};
      }
      break;
    case AtomicMemoryInstructionKind::RmwLoop:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.stored_value_id.has_value() ||
          !instruction.stored_value_name.has_value() ||
          !instruction.stored_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value() ||
          !instruction.rmw_new_value_register.has_value() ||
          !instruction.exclusive_status_register.has_value() ||
          instruction.rmw_opcode == bir::AtomicRmwOpcode::None ||
          instruction.result_mode != bir::AtomicResultMode::OldValue ||
          !instruction.exclusive_retry_loop) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic rmw loop is missing operand, result, scratch, status, opcode, or retry-loop authority"};
      }
      break;
    case AtomicMemoryInstructionKind::CompareExchangeLoop:
      if (!instruction.pointer_value_id.has_value() ||
          !instruction.pointer_value_name.has_value() ||
          !instruction.pointer_register.has_value() ||
          !instruction.expected_value_id.has_value() ||
          !instruction.expected_value_name.has_value() ||
          !instruction.expected_register.has_value() ||
          !instruction.desired_value_id.has_value() ||
          !instruction.desired_value_name.has_value() ||
          !instruction.desired_register.has_value() ||
          !instruction.result_value_id.has_value() ||
          !instruction.result_value_name.has_value() ||
          !instruction.result_register.has_value() ||
          !instruction.exclusive_status_register.has_value() ||
          !instruction.exclusive_retry_loop ||
          !instruction.compare_exchange_failure_clears_monitor ||
          instruction.failure_ordering == bir::AtomicOrdering::None) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing operand, result, status, ordering, or monitor-clear authority"};
      }
      if (!instruction.compare_exchange_result_is_boolean &&
          !instruction.compare_exchange_result_is_old_value) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing result-mode authority"};
      }
      if (instruction.compare_exchange_result_is_boolean &&
          !instruction.compare_loaded_register.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic = "atomic compare-exchange loop is missing loaded-value register authority"};
      }
      break;
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

MachineNodeStatusRecord address_materialization_selection_status(
    const AddressMaterializationRecord& instruction) {
  if (instruction.source_materialization == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName ||
      instruction.block_label == c4c::kInvalidBlockLabel) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "address materialization node is missing prepared provenance"};
  }
  if (instruction.kind == AddressMaterializationKind::DeferredUnsupported) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "address materialization kind is outside the selected subset"};
  }
  if (!instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      !instruction.result_register.has_value() ||
      instruction.result_home_kind != prepare::PreparedValueHomeKind::Register) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "address materialization node is missing prepared result register"};
  }
  if (instruction.kind == AddressMaterializationKind::StringConstant) {
    if (!instruction.text_name.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic = "string address materialization is missing text identity"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (instruction.kind == AddressMaterializationKind::LabelPageLow12) {
    if (!instruction.target_label.has_value()) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic = "label address materialization is missing target label identity"};
    }
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  if (!instruction.symbol_name.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "global address materialization is missing symbol identity"};
  }
  if (instruction.kind == AddressMaterializationKind::GotPageLow12 &&
      instruction.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "GOT address materialization is missing GOT-required policy"};
  }
  if (instruction.kind == AddressMaterializationKind::TlsRelative &&
      (!instruction.is_thread_local || !instruction.has_tls_address_space ||
       instruction.address_space != bir::AddressSpace::Tls)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "TLS address materialization is missing TLS facts"};
  }
  if (instruction.kind == AddressMaterializationKind::TlsRelative &&
      (instruction.tls_model !=
           prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
       instruction.tls_thread_pointer_register !=
           prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
       instruction.tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
       instruction.tls_low_relocation !=
           prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic =
            "TLS address materialization is missing thread-pointer-relative relocation facts"};
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
    const bool selected_f128_constant_argument_move =
        selected_register_argument_move &&
        !instruction.source_register.has_value() &&
        instruction.destination_register.has_value() &&
        instruction.destination_register->prepared_bank ==
            prepare::PreparedRegisterBank::Vreg &&
        instruction.destination_register->expected_view == abi::RegisterView::Q &&
        instruction.source_f128_carrier != nullptr &&
        instruction.source_f128_carrier->source_type == bir::TypeKind::F128 &&
        instruction.source_f128_carrier->kind ==
            prepare::PreparedF128CarrierKind::Missing &&
        instruction.source_f128_carrier->missing_required_facts.empty() &&
        instruction.source_f128_carrier->total_size_bytes == 16 &&
        instruction.source_f128_carrier->total_align_bytes == 16 &&
        instruction.source_f128_carrier->constant_payload.has_value() &&
        instruction.source_f128_constant_payload.has_value() &&
        instruction.source_f128_constant_payload->low_bits ==
            instruction.source_f128_carrier->constant_payload->low_bits &&
        instruction.source_f128_constant_payload->high_bits ==
            instruction.source_f128_carrier->constant_payload->high_bits;
    if (selected_f128_constant_argument_move) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared register source and destination"};
  }
  if (instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr) {
    return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
  }
  const auto* f128_carrier =
      instruction.source_f128_carrier != nullptr
          ? instruction.source_f128_carrier
          : instruction.destination_f128_carrier;
  const bool selected_f128_register_move =
      instruction.source_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.destination_register->prepared_bank == prepare::PreparedRegisterBank::Vreg &&
      instruction.source_register->expected_view == abi::RegisterView::Q &&
      instruction.destination_register->expected_view == abi::RegisterView::Q &&
      f128_carrier != nullptr &&
      f128_carrier->kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      f128_carrier->missing_required_facts.empty() &&
      f128_carrier->total_size_bytes == 16 && f128_carrier->total_align_bytes == 16;
  if (!selected_f128_register_move) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "call-boundary move node requires prepared GPR registers or structured f128 q-register authority"};
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

std::optional<VariadicVaStartRecord> make_variadic_va_start_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !entry.named_register_counts.gp.has_value() ||
      !entry.named_register_counts.fp.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.register_save_area.saved_gp_register_count.has_value() ||
      !entry.register_save_area.saved_fp_register_count.has_value() ||
      !entry.register_save_area.initial_gp_offset_bytes.has_value() ||
      !entry.register_save_area.initial_fp_offset_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicVaStartRecord{
      .destination_va_list = *homes.destination_va_list,
      .named_gp_register_count = *entry.named_register_counts.gp,
      .named_fp_register_count = *entry.named_register_counts.fp,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .va_list_fields = entry.va_list_layout.fields,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .saved_gp_register_count = *entry.register_save_area.saved_gp_register_count,
      .saved_fp_register_count = *entry.register_save_area.saved_fp_register_count,
      .initial_gp_offset_bytes = *entry.register_save_area.initial_gp_offset_bytes,
      .initial_fp_offset_bytes = *entry.register_save_area.initial_fp_offset_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicScalarVaArgRecord> make_variadic_scalar_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.source_va_list.has_value() || !homes.scalar_result.has_value() ||
      !homes.scalar_access_plan.has_value()) {
    return std::nullopt;
  }
  const auto& plan = *homes.scalar_access_plan;
  if (plan.source_class == prepare::PreparedVariadicScalarVaArgSourceClass::Unknown ||
      plan.value_type == bir::TypeKind::Void ||
      plan.value_size_bytes == 0 ||
      plan.value_align_bytes == 0 ||
      !plan.result_home.has_value() ||
      !plan.source_field.has_value() ||
      !plan.source_field_offset_bytes.has_value() ||
      !plan.source_slot_size_bytes.has_value() ||
      !plan.progression_field.has_value() ||
      !plan.progression_field_offset_bytes.has_value() ||
      !plan.progression_stride_bytes.has_value() ||
      !plan.overflow_source_field.has_value() ||
      !plan.overflow_source_field_offset_bytes.has_value() ||
      !plan.overflow_stride_bytes.has_value() ||
      !entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicScalarVaArgRecord{
      .source_class = plan.source_class,
      .value_type = plan.value_type,
      .value_size_bytes = plan.value_size_bytes,
      .value_align_bytes = plan.value_align_bytes,
      .source_va_list = *homes.source_va_list,
      .result_home = *plan.result_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .overflow_source_field = *plan.overflow_source_field,
      .overflow_source_field_offset_bytes = *plan.overflow_source_field_offset_bytes,
      .overflow_stride_bytes = *plan.overflow_stride_bytes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicAggregateVaArgRecord> make_variadic_aggregate_va_arg_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!prepare::has_complete_prepared_variadic_aggregate_va_arg_access_plan(homes)) {
    return std::nullopt;
  }
  const auto& plan = *homes.aggregate_access_plan;
  if (!entry.register_save_area.slot_id.has_value() ||
      !entry.register_save_area.stack_offset_bytes.has_value() ||
      !entry.register_save_area.size_bytes.has_value() ||
      !entry.register_save_area.align_bytes.has_value() ||
      !entry.register_save_area.gp_offset_bytes.has_value() ||
      !entry.register_save_area.fp_offset_bytes.has_value() ||
      !entry.register_save_area.gp_slot_size_bytes.has_value() ||
      !entry.register_save_area.fp_slot_size_bytes.has_value() ||
      !entry.overflow_area.base_slot_id.has_value() ||
      !entry.overflow_area.base_stack_offset_bytes.has_value() ||
      !entry.overflow_area.align_bytes.has_value() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  return VariadicAggregateVaArgRecord{
      .source_class = plan.source_class,
      .payload_size_bytes = plan.payload_size_bytes,
      .payload_align_bytes = plan.payload_align_bytes,
      .source_va_list = *homes.source_va_list,
      .destination_payload_home = *plan.destination_payload_home,
      .source_field = *plan.source_field,
      .source_field_offset_bytes = *plan.source_field_offset_bytes,
      .source_payload_offset_bytes = *plan.source_payload_offset_bytes,
      .source_slot_size_bytes = *plan.source_slot_size_bytes,
      .copy_size_bytes = *plan.copy_size_bytes,
      .copy_align_bytes = *plan.copy_align_bytes,
      .progression_field = *plan.progression_field,
      .progression_field_offset_bytes = *plan.progression_field_offset_bytes,
      .progression_stride_bytes = *plan.progression_stride_bytes,
      .register_save_area_slot_id = *entry.register_save_area.slot_id,
      .register_save_area_stack_offset_bytes =
          *entry.register_save_area.stack_offset_bytes,
      .register_save_area_size_bytes = *entry.register_save_area.size_bytes,
      .register_save_area_align_bytes = *entry.register_save_area.align_bytes,
      .register_save_area_gp_offset_bytes = *entry.register_save_area.gp_offset_bytes,
      .register_save_area_fp_offset_bytes = *entry.register_save_area.fp_offset_bytes,
      .register_save_area_gp_slot_size_bytes =
          *entry.register_save_area.gp_slot_size_bytes,
      .register_save_area_fp_slot_size_bytes =
          *entry.register_save_area.fp_slot_size_bytes,
      .overflow_area_base_slot_id = *entry.overflow_area.base_slot_id,
      .overflow_area_base_stack_offset_bytes =
          *entry.overflow_area.base_stack_offset_bytes,
      .overflow_area_align_bytes = *entry.overflow_area.align_bytes,
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

std::optional<VariadicVaCopyRecord> make_variadic_va_copy_record(
    const prepare::PreparedVariadicEntryPlanFunction& entry,
    const prepare::PreparedVariadicEntryHelperOperandHomes& homes) {
  if (!homes.destination_va_list.has_value() ||
      !homes.source_va_list.has_value() ||
      !entry.va_list_layout.size_bytes.has_value() ||
      !entry.va_list_layout.align_bytes.has_value() ||
      entry.va_list_layout.fields.empty() ||
      !entry.helper_resources.scratch_register_count.has_value() ||
      !entry.helper_resources.scratch_stack_bytes.has_value()) {
    return std::nullopt;
  }

  std::vector<VariadicVaCopyFieldRecord> field_copies;
  field_copies.reserve(entry.va_list_layout.fields.size());
  for (const auto& field : entry.va_list_layout.fields) {
    if (field.size_bytes == 0) {
      return std::nullopt;
    }
    field_copies.push_back(VariadicVaCopyFieldRecord{
        .kind = field.kind,
        .source_offset_bytes = field.offset_bytes,
        .destination_offset_bytes = field.offset_bytes,
        .size_bytes = field.size_bytes,
    });
  }

  return VariadicVaCopyRecord{
      .destination_va_list = *homes.destination_va_list,
      .source_va_list = *homes.source_va_list,
      .va_list_size_bytes = *entry.va_list_layout.size_bytes,
      .va_list_align_bytes = *entry.va_list_layout.align_bytes,
      .field_copies = std::move(field_copies),
      .scratch_register_count = *entry.helper_resources.scratch_register_count,
      .scratch_stack_bytes = *entry.helper_resources.scratch_stack_bytes,
  };
}

MachineNodeStatusRecord call_selection_status(const CallInstructionRecord& instruction) {
  if (instruction.variadic_entry_helper.has_value()) {
    if (instruction.source_variadic_entry == nullptr) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "variadic entry helper node is missing prepared entry provenance"};
    }
    if (instruction.source_variadic_helper_operand_homes == nullptr) {
      return MachineNodeStatusRecord{
          .status = MachineNodeSelectionStatus::MissingRequiredFacts,
          .diagnostic =
              "variadic entry helper node is missing prepared operand-home provenance"};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaStart) {
      if (!instruction.variadic_va_start.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "va_start helper node is missing structured prepared va_start record"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaArg) {
      if (!instruction.variadic_scalar_va_arg.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "scalar va_arg machine-node lowering requires complete prepared fact "
                "helper_operand_homes.va_arg.scalar_access_plan"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaArgAggregate) {
      if (!instruction.variadic_aggregate_va_arg.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "aggregate va_arg machine-node lowering requires complete prepared fact "
                "helper_operand_homes.va_arg_aggregate.aggregate_access_plan"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    if (*instruction.variadic_entry_helper ==
        prepare::PreparedVariadicEntryHelperKind::VaCopy) {
      if (!instruction.variadic_va_copy.has_value()) {
        return MachineNodeStatusRecord{
            .status = MachineNodeSelectionStatus::MissingRequiredFacts,
            .diagnostic =
                "va_copy machine-node lowering requires complete prepared source and "
                "destination va_list homes plus va_list_layout field facts"};
      }
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic =
            "variadic entry helper machine-node lowering is outside the selected va_start subset"};
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
      .result_register = cast.result_register,
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
    if (instruction.result_register.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
    } else {
      defs.push_back(
          prepared_value_def(instruction.result_value_id, instruction.result_value_name));
    }
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

InstructionRecord make_atomic_memory_instruction(
    AtomicMemoryInstructionRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.pointer_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.pointer_register));
  }
  if (instruction.stored_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.stored_register));
  }
  if (instruction.expected_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.expected_register));
  }
  if (instruction.desired_register.has_value()) {
    operands.push_back(make_register_operand(*instruction.desired_register));
  }

  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value() &&
             instruction.result_value_name.has_value()) {
    defs.push_back(prepared_value_def(instruction.result_value_id,
                                      *instruction.result_value_name));
  }
  if (instruction.rmw_new_value_register.has_value()) {
    defs.push_back(effect_from_operand(
        make_register_operand(*instruction.rmw_new_value_register)));
  }
  if (instruction.compare_loaded_register.has_value()) {
    defs.push_back(effect_from_operand(
        make_register_operand(*instruction.compare_loaded_register)));
  }
  if (instruction.exclusive_status_register.has_value()) {
    defs.push_back(effect_from_operand(
        make_register_operand(*instruction.exclusive_status_register)));
  }

  const auto selection = atomic_memory_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Memory,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_atomic_memory_instruction(instruction),
      .selection = selection,
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = effects_from_operands(operands),
      .side_effects = atomic_memory_side_effects(instruction),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_transport_selection_status(
    const I128TransportRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport is missing prepared i128 carrier provenance"};
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::Missing) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport carrier is missing complete low/high authority"};
  }
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 transport carrier has invalid size, lane width, or alignment"};
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
      (!instruction.low_lane.reg.has_value() || !instruction.high_lane.reg.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 register-pair transport is missing low/high registers"};
  }
  if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::MemoryBacked &&
      (!instruction.low_lane.slot_id.has_value() ||
       !instruction.low_lane.stack_offset_bytes.has_value() ||
       !instruction.high_lane.slot_id.has_value() ||
       !instruction.high_lane.stack_offset_bytes.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 memory-backed transport is missing low/high memory offsets"};
  }
  if ((instruction.transport_kind == I128TransportKind::LoadFromMemory ||
       instruction.transport_kind == I128TransportKind::StoreToMemory) &&
      !instruction.memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 memory transport is missing structured memory operand"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_transport_instruction(I128TransportRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.memory.has_value()) {
    operands.push_back(make_memory_operand(*instruction.memory));
  }
  if (instruction.low_lane.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.low_lane.reg));
  }
  if (instruction.high_lane.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.high_lane.reg));
  }

  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.transport_kind == I128TransportKind::LoadFromMemory ||
      instruction.transport_kind == I128TransportKind::CarrierSnapshot) {
    if (instruction.low_lane.reg.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.low_lane.reg)));
    }
    if (instruction.high_lane.reg.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.high_lane.reg)));
    }
    if (instruction.carrier_kind == prepare::PreparedI128CarrierKind::MemoryBacked) {
      defs.push_back(prepared_value_def(instruction.value_id, instruction.value_name));
    }
  }
  if (instruction.transport_kind == I128TransportKind::StoreToMemory ||
      instruction.transport_kind == I128TransportKind::CarrierSnapshot) {
    if (instruction.low_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.low_lane.reg)));
    }
    if (instruction.high_lane.reg.has_value()) {
      uses.push_back(effect_from_operand(make_register_operand(*instruction.high_lane.reg)));
    }
  }
  if (instruction.memory.has_value()) {
    auto memory_effect = effect_from_operand(make_memory_operand(*instruction.memory));
    if (instruction.transport_kind == I128TransportKind::LoadFromMemory) {
      uses.push_back(std::move(memory_effect));
    } else if (instruction.transport_kind == I128TransportKind::StoreToMemory) {
      defs.push_back(std::move(memory_effect));
    }
  }

  std::vector<MachineSideEffectKind> side_effects;
  if (instruction.transport_kind == I128TransportKind::LoadFromMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
  } else if (instruction.transport_kind == I128TransportKind::StoreToMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }

  return InstructionRecord{
      .family = InstructionFamily::I128Transport,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Transport,
      .selection = i128_transport_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

MachineNodeStatusRecord f128_transport_selection_status(
    const F128TransportRecord& instruction) {
  if (instruction.source_carrier == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport is missing prepared f128 carrier provenance"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::Missing) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport carrier is missing complete full-width authority"};
  }
  if (instruction.total_size_bytes != 16 || instruction.total_align_bytes != 16) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 transport carrier requires complete 16-byte size and alignment"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
      !instruction.reg.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 full-width register transport is missing q-register authority"};
  }
  if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::MemoryBacked &&
      (!instruction.slot_id.has_value() || !instruction.stack_offset_bytes.has_value())) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 memory-backed transport is missing frame-slot authority"};
  }
  if ((instruction.transport_kind == F128TransportKind::LoadFromMemory ||
       instruction.transport_kind == F128TransportKind::StoreToMemory) &&
      !instruction.memory.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 memory transport is missing structured memory operand"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_f128_transport_instruction(F128TransportRecord instruction) {
  std::vector<OperandRecord> operands;
  if (instruction.memory.has_value()) {
    operands.push_back(make_memory_operand(*instruction.memory));
  }
  if (instruction.reg.has_value()) {
    operands.push_back(make_register_operand(*instruction.reg));
  }

  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.transport_kind == F128TransportKind::LoadFromMemory ||
      instruction.transport_kind == F128TransportKind::CarrierSnapshot) {
    if (instruction.reg.has_value()) {
      defs.push_back(effect_from_operand(make_register_operand(*instruction.reg)));
    } else if (instruction.carrier_kind == prepare::PreparedF128CarrierKind::MemoryBacked) {
      defs.push_back(prepared_value_def(instruction.value_id, instruction.value_name));
    }
  }
  if ((instruction.transport_kind == F128TransportKind::StoreToMemory ||
       instruction.transport_kind == F128TransportKind::CarrierSnapshot) &&
      instruction.reg.has_value()) {
    uses.push_back(effect_from_operand(make_register_operand(*instruction.reg)));
  }
  if (instruction.memory.has_value()) {
    auto memory_effect = effect_from_operand(make_memory_operand(*instruction.memory));
    if (instruction.transport_kind == F128TransportKind::LoadFromMemory) {
      uses.push_back(std::move(memory_effect));
    } else if (instruction.transport_kind == F128TransportKind::StoreToMemory) {
      defs.push_back(std::move(memory_effect));
    }
  }

  std::vector<MachineSideEffectKind> side_effects;
  if (instruction.transport_kind == F128TransportKind::LoadFromMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
  } else if (instruction.transport_kind == F128TransportKind::StoreToMemory) {
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }

  return InstructionRecord{
      .family = InstructionFamily::F128Transport,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::F128Transport,
      .selection = f128_transport_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .side_effects = std::move(side_effects),
      .payload = instruction,
  };
}

MachineNodeStatusRecord f128_runtime_helper_boundary_selection_status(
    const F128RuntimeHelperBoundaryRecord& instruction) {
  const auto has_full_width_register = [](const F128RuntimeHelperOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedF128CarrierKind::FullWidthRegister &&
           operand.carrier_register.has_value() &&
           operand.abi_register.has_value() &&
           operand.width_bytes == 16 &&
           operand.align_bytes == 16;
  };
  if (instruction.source_helper == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing prepared helper provenance"};
  }
  const bool supported_helper =
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Add &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Add &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Add) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Sub &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sub &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Sub) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Mul &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Mul &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Mul) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Div &&
       instruction.source_binary_opcode == bir::BinaryOpcode::SDiv &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Div) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Eq &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Eq &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Eq) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ne &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Ne &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Ne) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Lt &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Slt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Lt) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Le &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sle &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Le) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Gt &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sgt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Gt) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ge &&
       instruction.source_binary_opcode == bir::BinaryOpcode::Sge &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::Ge) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F32ToF128 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPExt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F32ToF128) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F64ToF128 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPExt &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F64ToF128) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF32 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPTrunc &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F128ToF32) ||
      (instruction.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF64 &&
       instruction.source_cast_opcode == bir::CastOpcode::FPTrunc &&
       instruction.boundary_kind == F128RuntimeHelperBoundaryKind::F128ToF64);
  const bool comparison_helper =
      instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison;
  const bool cast_helper =
      instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast;
  if ((instruction.helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       !comparison_helper && !cast_helper) ||
      !supported_helper ||
      instruction.callee_name.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing helper family or callee identity"};
  }
  if ((!comparison_helper && !cast_helper &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (comparison_helper &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "f128 helper boundary requires modeled result ownership"};
  }
  if (instruction.width_bytes != 16 ||
      instruction.align_bytes != 16 ||
      (!cast_helper &&
       (instruction.source_type != bir::TypeKind::F128 ||
        (!comparison_helper ? instruction.result_type != bir::TypeKind::F128
                            : instruction.result_type != bir::TypeKind::I32))) ||
      (cast_helper && !((instruction.source_type == bir::TypeKind::F32 &&
                         instruction.result_type == bir::TypeKind::F128) ||
                        (instruction.source_type == bir::TypeKind::F64 &&
                         instruction.result_type == bir::TypeKind::F128) ||
                        (instruction.source_type == bir::TypeKind::F128 &&
                         instruction.result_type == bir::TypeKind::F32) ||
                        (instruction.source_type == bir::TypeKind::F128 &&
                         instruction.result_type == bir::TypeKind::F64)))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary has invalid operation, type, size, or alignment facts"};
  }
  const auto has_scalar_fp = [](const F128RuntimeHelperScalarResultRecord& scalar) {
    return (scalar.type == bir::TypeKind::F32 || scalar.type == bir::TypeKind::F64) &&
           (scalar.width_bytes == 4 || scalar.width_bytes == 8) &&
           scalar.register_bank == prepare::PreparedRegisterBank::Fpr &&
           scalar.materialized_i1_register.has_value() &&
           scalar.abi_register.has_value();
  };
  if ((!comparison_helper && !cast_helper && !has_full_width_register(instruction.result)) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       (!has_scalar_fp(instruction.scalar_operand) ||
        !has_full_width_register(instruction.result))) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       (!has_full_width_register(instruction.lhs) || !has_scalar_fp(instruction.scalar_result))) ||
      (comparison_helper &&
       (instruction.scalar_result.type != bir::TypeKind::I32 ||
        instruction.scalar_result.width_bytes != 4 ||
        instruction.scalar_result.register_bank != prepare::PreparedRegisterBank::Gpr ||
        !instruction.scalar_result.abi_register.has_value() ||
        !instruction.scalar_result.materialized_i1_register.has_value() ||
        !instruction.scalar_result.cmp_result_consumption.has_value() ||
        instruction.scalar_result.cmp_result_consumption->cmp_type != bir::TypeKind::I32 ||
        instruction.scalar_result.cmp_result_consumption->bir_result_type !=
            bir::TypeKind::I1 ||
        instruction.scalar_result.cmp_result_consumption->zero_test ==
            prepare::PreparedF128CmpResultZeroTest::Missing ||
        !instruction.scalar_result.cmp_result_consumption->consumes_helper_cmp_result ||
        !instruction.scalar_result.cmp_result_consumption->owns_bir_i1_result)) ||
      (!cast_helper && !has_full_width_register(instruction.lhs)) ||
      (!cast_helper && !has_full_width_register(instruction.rhs))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing full-width q-register carrier or ABI facts"};
  }
  if (!instruction.resource_policy.call_boundary ||
      !instruction.resource_policy.runtime_helper_callee ||
      !instruction.resource_policy.caller_saved_clobbers ||
      !instruction.resource_policy.preserves_source_operation_identity) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing resource policy facts"};
  }
  if ((!comparison_helper && !cast_helper &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult) ||
      (comparison_helper &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) ||
      (!cast_helper && instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (cast_helper && instruction.result_type == bir::TypeKind::F128 &&
       instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Fpr) ||
      (cast_helper && instruction.source_type == bir::TypeKind::F128 &&
       instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (!comparison_helper && instruction.result_type == bir::TypeKind::F128
           ? instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
           : (comparison_helper
                  ? instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr
                  : instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Fpr)) ||
      (!cast_helper ? instruction.abi_policy.argument_count != 2
                    : instruction.abi_policy.argument_count != 1) ||
      instruction.abi_policy.result_count != 1 ||
      (!comparison_helper && instruction.result_type == bir::TypeKind::F128
           ? instruction.abi_policy.width_bytes != 16
           : (comparison_helper ? instruction.abi_policy.width_bytes != 4
                                : (instruction.abi_policy.width_bytes != 4 &&
                                   instruction.abi_policy.width_bytes != 8)))) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing ABI/register-bank policy facts"};
  }
  if (instruction.clobbered_registers.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing caller-saved clobber policy"};
  }
  if (!instruction.live_preservation_policy.evaluated ||
      !instruction.live_preservation_policy.caller_saved_clobbers_modeled ||
      !instruction.live_preservation_policy.no_additional_live_preservation_required) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing live-preservation policy"};
  }
  if (!instruction.selected_call_ownership.owns_terminal_call ||
      !instruction.selected_call_ownership.has_callee_identity ||
      !instruction.selected_call_ownership.has_resource_policy ||
      !instruction.selected_call_ownership.has_clobber_policy ||
      !instruction.selected_call_ownership.has_abi_bindings ||
      !instruction.selected_call_ownership.has_marshaling ||
      !instruction.selected_call_ownership.has_live_preservation) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "f128 helper boundary is missing selected-call ownership policy"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_f128_runtime_helper_boundary_instruction(
    F128RuntimeHelperBoundaryRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.carrier_register);
  add_def(instruction.result.abi_register);
  add_def(instruction.scalar_result.abi_register);
  if (instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast) {
    add_def(instruction.scalar_result.materialized_i1_register);
    if (instruction.scalar_operand.materialized_i1_register.has_value()) {
      add_use(instruction.scalar_operand.materialized_i1_register);
    }
    add_use(instruction.scalar_operand.abi_register);
  }
  if (instruction.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison) {
    add_def(instruction.scalar_result.materialized_i1_register);
    if (instruction.scalar_result.abi_register.has_value()) {
      uses.push_back(effect_from_operand(
          make_register_operand(*instruction.scalar_result.abi_register)));
    }
    defs.push_back(prepared_value_def(instruction.result_value_id,
                                      instruction.result_value_name));
  }
  add_use(instruction.lhs.carrier_register);
  add_use(instruction.lhs.abi_register);
  add_use(instruction.rhs.carrier_register);
  add_use(instruction.rhs.abi_register);

  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::F128RuntimeHelper,
      .selection = f128_runtime_helper_boundary_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .side_effects = {MachineSideEffectKind::Call},
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_pair_selection_status(
    const I128PairOperationRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation is missing result register-pair carrier"};
  }
  if (!has_pair_registers(instruction.lhs) || !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 pair operation is missing source register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_pair_operation_instruction(
    I128PairOperationRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Pair,
      .selection = i128_pair_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_shift_selection_status(
    const I128ShiftRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 shift has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result) ||
      !has_pair_registers(instruction.source)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 shift is missing source/result register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_shift_instruction(I128ShiftRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.source.low_lane.reg);
  add_use(instruction.source.high_lane.reg);
  operands.push_back(instruction.shift_count);
  uses.push_back(effect_from_operand(instruction.shift_count));

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Shift,
      .selection = i128_shift_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_compare_selection_status(
    const I128CompareRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.operand_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I1) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare has invalid type, size, or alignment facts"};
  }
  if (!instruction.result_register.has_value()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare is missing scalar result register"};
  }
  if (!has_pair_registers(instruction.lhs) || !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 compare is missing source register-pair carriers"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_compare_instruction(I128CompareRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.result_register.has_value()) {
    const auto result = make_register_operand(*instruction.result_register);
    operands.push_back(result);
    defs.push_back(effect_from_operand(result));
  }
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::I128Pair,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128Compare,
      .selection = i128_compare_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .payload = instruction,
  };
}

MachineNodeStatusRecord i128_runtime_helper_boundary_selection_status(
    const I128RuntimeHelperBoundaryRecord& instruction) {
  const auto has_pair_registers = [](const I128PairOperandRecord& operand) {
    return operand.source_carrier != nullptr &&
           operand.carrier_kind == prepare::PreparedI128CarrierKind::RegisterPair &&
           operand.low_lane.reg.has_value() &&
           operand.high_lane.reg.has_value();
  };
  if (instruction.source_helper == nullptr) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing prepared helper provenance"};
  }
  if (instruction.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem ||
      instruction.callee_name.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing helper family or callee identity"};
  }
  if (instruction.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "i128 helper boundary requires direct low/high result ownership"};
  }
  if (instruction.total_size_bytes != 16 || instruction.lane_width_bytes != 8 ||
      instruction.total_align_bytes == 0 ||
      instruction.source_type != bir::TypeKind::I128 ||
      instruction.result_type != bir::TypeKind::I128) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary has invalid type, size, or alignment facts"};
  }
  if (!has_pair_registers(instruction.result) ||
      !has_pair_registers(instruction.lhs) ||
      !has_pair_registers(instruction.rhs)) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing low/high register-pair lanes"};
  }
  if (!instruction.resource_policy.call_boundary ||
      !instruction.resource_policy.runtime_helper_callee ||
      !instruction.resource_policy.caller_saved_clobbers ||
      !instruction.resource_policy.preserves_source_operation_identity) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing resource policy facts"};
  }
  if (instruction.abi_policy.transition !=
          prepare::PreparedI128RuntimeHelperAbiTransition::
              DirectRegisterPairArgumentsAndResult ||
      instruction.abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      instruction.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      instruction.abi_policy.argument_count != 2 ||
      instruction.abi_policy.lanes_per_argument != 2 ||
      instruction.abi_policy.result_lane_count != 2 ||
      instruction.abi_policy.lane_width_bytes != 8) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing ABI/register-bank policy facts"};
  }
  if (instruction.clobbered_registers.empty()) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing caller-saved clobber policy"};
  }
  if (!instruction.live_preservation_policy.evaluated ||
      !instruction.live_preservation_policy.caller_saved_clobbers_modeled ||
      !instruction.live_preservation_policy.no_additional_live_preservation_required) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing live-preservation policy"};
  }
  if (!instruction.selected_call_ownership.owns_terminal_call ||
      !instruction.selected_call_ownership.has_callee_identity ||
      !instruction.selected_call_ownership.has_resource_policy ||
      !instruction.selected_call_ownership.has_clobber_policy ||
      !instruction.selected_call_ownership.has_abi_bindings ||
      !instruction.selected_call_ownership.has_marshaling ||
      !instruction.selected_call_ownership.has_live_preservation) {
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::MissingRequiredFacts,
        .diagnostic = "i128 helper boundary is missing selected-call ownership policy"};
  }
  return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
}

InstructionRecord make_i128_runtime_helper_boundary_instruction(
    I128RuntimeHelperBoundaryRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  const auto add_def = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      defs.push_back(effect_from_operand(operand));
    }
  };
  const auto add_use = [&](const std::optional<RegisterOperand>& reg) {
    if (reg.has_value()) {
      const auto operand = make_register_operand(*reg);
      operands.push_back(operand);
      uses.push_back(effect_from_operand(operand));
    }
  };
  add_def(instruction.result.low_lane.reg);
  add_def(instruction.result.high_lane.reg);
  add_use(instruction.lhs.low_lane.reg);
  add_use(instruction.lhs.high_lane.reg);
  add_use(instruction.rhs.low_lane.reg);
  add_use(instruction.rhs.high_lane.reg);

  return InstructionRecord{
      .family = InstructionFamily::CallBoundary,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = MachineOpcode::I128RuntimeHelper,
      .selection = i128_runtime_helper_boundary_selection_status(instruction),
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .block_index = instruction.block_index,
      .instruction_index = instruction.instruction_index,
      .operands = std::move(operands),
      .defs = std::move(defs),
      .uses = std::move(uses),
      .clobbers = effects_from_prepared_call_clobbers(instruction.clobbered_registers),
      .side_effects = {MachineSideEffectKind::Call},
      .payload = instruction,
  };
}

InstructionRecord make_address_materialization_instruction(
    AddressMaterializationRecord instruction) {
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  if (instruction.result_register.has_value()) {
    const auto result = make_register_operand(*instruction.result_register);
    operands.push_back(result);
    defs.push_back(effect_from_operand(result));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(
        prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }
  if (instruction.symbol_name.has_value()) {
    const auto symbol = make_symbol_operand(SymbolOperand{
        .link_name = *instruction.symbol_name,
        .type = bir::TypeKind::Ptr,
        .byte_offset = instruction.byte_offset,
    });
    operands.push_back(symbol);
    uses.push_back(effect_from_operand(symbol));
  } else if (instruction.target_label.has_value()) {
    const auto target = make_branch_target_operand(BranchTargetOperand{
        .surface = RecordSurfaceKind::RecordOnly,
        .block_label = *instruction.target_label,
        .function_name = instruction.function_name,
    });
    operands.push_back(target);
    uses.push_back(effect_from_operand(target));
  }
  const auto selection = address_materialization_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Scalar,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_address_materialization(instruction),
      .selection = selection,
      .function_name = instruction.function_name,
      .block_label = instruction.block_label,
      .instruction_index = instruction.instruction_index,
      .operands = operands,
      .defs = defs,
      .uses = uses,
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
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaStart} &&
      !instruction.variadic_va_start.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_start =
        make_variadic_va_start_record(*instruction.source_variadic_entry,
                                      *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArg} &&
      !instruction.variadic_scalar_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_scalar_va_arg =
        make_variadic_scalar_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaArgAggregate} &&
      !instruction.variadic_aggregate_va_arg.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_aggregate_va_arg =
        make_variadic_aggregate_va_arg_record(
            *instruction.source_variadic_entry,
            *instruction.source_variadic_helper_operand_homes);
  }
  if (instruction.variadic_entry_helper ==
          std::optional<prepare::PreparedVariadicEntryHelperKind>{
              prepare::PreparedVariadicEntryHelperKind::VaCopy} &&
      !instruction.variadic_va_copy.has_value() &&
      instruction.source_variadic_entry != nullptr &&
      instruction.source_variadic_helper_operand_homes != nullptr) {
    instruction.variadic_va_copy =
        make_variadic_va_copy_record(*instruction.source_variadic_entry,
                                     *instruction.source_variadic_helper_operand_homes);
  }

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
  std::vector<MachineEffectResource> uses = effects_from_operands(operands);
  std::vector<MachineSideEffectKind> side_effects = {MachineSideEffectKind::Call};
  if (instruction.memory_return_storage.has_value()) {
    defs.push_back(effect_from_operand(make_memory_operand(*instruction.memory_return_storage)));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_start.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_start->destination_va_list.value_id,
        instruction.variadic_va_start->destination_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_scalar_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->result_home.value_id,
        instruction.variadic_scalar_va_arg->result_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_scalar_va_arg->source_va_list.value_id,
        instruction.variadic_scalar_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_aggregate_va_arg.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_id,
        instruction.variadic_aggregate_va_arg->destination_payload_home.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_aggregate_va_arg->source_va_list.value_id,
        instruction.variadic_aggregate_va_arg->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  if (instruction.variadic_va_copy.has_value()) {
    defs.push_back(prepared_value_def(
        instruction.variadic_va_copy->destination_va_list.value_id,
        instruction.variadic_va_copy->destination_va_list.value_name));
    uses.push_back(prepared_value_def(
        instruction.variadic_va_copy->source_va_list.value_id,
        instruction.variadic_va_copy->source_va_list.value_name));
    side_effects.push_back(MachineSideEffectKind::MemoryRead);
    side_effects.push_back(MachineSideEffectKind::MemoryWrite);
  }
  return InstructionRecord{
      .family = InstructionFamily::Call,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = instruction.variadic_va_start.has_value()
                    ? MachineOpcode::VariadicVaStart
                    : (instruction.variadic_scalar_va_arg.has_value()
                           ? MachineOpcode::VariadicVaArgScalar
                           : (instruction.variadic_aggregate_va_arg.has_value()
                                  ? MachineOpcode::VariadicVaArgAggregate
                                  : (instruction.variadic_va_copy.has_value()
                                         ? MachineOpcode::VariadicVaCopy
                                         : (instruction.is_indirect
                                                ? MachineOpcode::IndirectCall
                                                : MachineOpcode::DirectCall)))),
      .selection = call_selection_status(instruction),
      .operands = operands,
      .defs = defs,
      .uses = uses,
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
  const bool is_integer_operation =
      scalar_register_view(binary.operand_type).has_value() &&
      scalar_register_view(binary.result.type).has_value() &&
      is_scalar_alu_integer_opcode(binary.opcode);
  const bool is_floating_operation = is_scalar_alu_floating_type(binary.operand_type) &&
                                     is_scalar_alu_floating_type(binary.result.type) &&
                                     is_scalar_alu_floating_opcode(binary.opcode);
  if (!is_integer_operation && !is_floating_operation) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return scalar_alu_record_error(PreparedScalarAluRecordError::UnsupportedResultValue);
  }
  if (!scalar_storage_register_view(binary.result.type).has_value() ||
      !scalar_storage_register_view(binary.operand_type).has_value()) {
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
              .supported_integer_operation = is_integer_operation,
              .supported_floating_operation = is_floating_operation,
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
  const bool is_simple_integer_cast = is_simple_integer_cast_opcode(cast.opcode);
  const bool is_conversion_cast = is_supported_scalar_conversion_cast_opcode(cast.opcode);
  if (!is_simple_integer_cast && !is_conversion_cast) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOpcode);
  }
  if (cast.result.kind != bir::Value::Kind::Named || cast.result.name.empty()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedResultValue);
  }
  const bool source_is_integer = scalar_register_view(cast.operand.type).has_value();
  const bool result_is_integer = scalar_register_view(cast.result.type).has_value();
  const bool source_is_float = is_scalar_alu_floating_type(cast.operand.type);
  const bool result_is_float = is_scalar_alu_floating_type(cast.result.type);
  const bool supported_float_width_conversion =
      (cast.opcode == bir::CastOpcode::FPExt && cast.operand.type == bir::TypeKind::F32 &&
       cast.result.type == bir::TypeKind::F64) ||
      (cast.opcode == bir::CastOpcode::FPTrunc && cast.operand.type == bir::TypeKind::F64 &&
       cast.result.type == bir::TypeKind::F32);
  const bool supported_float_integer_conversion =
      ((cast.opcode == bir::CastOpcode::SIToFP || cast.opcode == bir::CastOpcode::UIToFP) &&
       source_is_integer && result_is_float) ||
      ((cast.opcode == bir::CastOpcode::FPToSI || cast.opcode == bir::CastOpcode::FPToUI) &&
       source_is_float && result_is_integer);
  if ((!is_simple_integer_cast || !source_is_integer || !result_is_integer) &&
      !supported_float_width_conversion && !supported_float_integer_conversion) {
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
  const auto result_register = make_prepared_register_operand(
      *result_home, *result_storage, cast.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::RegisterConversionFailed);
  }

  OperandRecord source;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, cast.operand, source);
      error != PreparedScalarAluRecordError::None) {
    return scalar_cast_record_error(scalar_cast_operand_error_from_alu_error(error));
  }
  const auto* source_register = std::get_if<RegisterOperand>(&source.payload);
  if ((is_conversion_cast || supported_float_width_conversion) &&
      (source.kind != OperandKind::Register || source_register == nullptr)) {
    return scalar_cast_record_error(PreparedScalarCastRecordError::UnsupportedOperandStorage);
  }
  const auto source_bank =
      source_register != nullptr ? source_register->prepared_bank : prepare::PreparedRegisterBank::None;
  const auto result_bank = result_register->prepared_bank;

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
              .result_register = result_register,
              .source = source,
              .source_register_bank = source_bank,
              .result_register_bank = result_bank,
              .crosses_register_bank = source_bank != prepare::PreparedRegisterBank::None &&
                                       result_bank != prepare::PreparedRegisterBank::None &&
                                       source_bank != result_bank,
              .supported_simple_integer_cast = is_simple_integer_cast,
              .supported_float_integer_conversion = supported_float_integer_conversion,
              .supported_float_width_conversion = supported_float_width_conversion,
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

PreparedMemoryInstructionRecordResult
make_prepared_frame_slot_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }

  const auto operand = make_prepared_memory_operand_record(
      names, value_locations, addressing, block_label, instruction_index, load);
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (!operand.record->result_value_id.has_value() ||
      !operand.record->result_value_name.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingResultValueHome);
  }

  const auto* result_home =
      prepare::find_prepared_value_home(value_locations, *operand.record->result_value_id);
  if (result_home == nullptr ||
      result_home->value_name != *operand.record->result_value_name ||
      result_home->kind != prepare::PreparedValueHomeKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingResultValueHome);
  }

  const auto* result_storage =
      find_storage_plan_value(storage_plan, *operand.record->result_value_id);
  if (result_storage == nullptr ||
      result_storage->value_name != *operand.record->result_value_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::MissingResultStorage);
  }
  if (result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedResultStorage);
  }

  auto result_register =
      make_prepared_register_operand(
          *result_home, *result_storage, load.result.type, RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::RegisterConversionFailed);
  }

  return PreparedMemoryInstructionRecordResult{
      .record =
          MemoryInstructionRecord{
              .memory_kind = MemoryInstructionKind::Load,
              .address = *operand.record,
              .result_value_id = operand.record->result_value_id,
              .result_value_name = *operand.record->result_value_name,
              .value_type = load.result.type,
              .result_register = *result_register,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
}

PreparedMemoryInstructionRecordResult make_store_memory_instruction_record(
    PreparedMemoryOperandRecordResult operand,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::Value& stored_value) {
  if (!operand.record.has_value()) {
    return memory_instruction_record_error(operand.error);
  }
  if (operand.record->base_kind != MemoryBaseKind::FrameSlot &&
      operand.record->base_kind != MemoryBaseKind::PointerValue) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::UnsupportedBase);
  }
  if (operand.record->base_kind == MemoryBaseKind::PointerValue) {
    if (!operand.record->pointer_value_id.has_value() ||
        !operand.record->pointer_value_name.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueHome);
    }
    const auto* pointer_home =
        prepare::find_prepared_value_home(value_locations, *operand.record->pointer_value_id);
    if (pointer_home == nullptr ||
        pointer_home->value_name != *operand.record->pointer_value_name ||
        pointer_home->kind != prepare::PreparedValueHomeKind::Register) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueHome);
    }
    const auto* pointer_storage =
        find_storage_plan_value(storage_plan, *operand.record->pointer_value_id);
    if (pointer_storage == nullptr ||
        pointer_storage->value_name != *operand.record->pointer_value_name) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::MissingPointerValueStorage);
    }
    if (pointer_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::UnsupportedPointerValueStorage);
    }
    auto base_register =
        make_prepared_register_operand(
            *pointer_home, *pointer_storage, bir::TypeKind::Ptr, RegisterOperandRole::StoragePlan);
    if (!base_register.has_value()) {
      return memory_instruction_record_error(
          PreparedMemoryOperandRecordError::RegisterConversionFailed);
    }
    operand.record->base_register = *base_register;
  }
  if (!operand.record->stored_value_id.has_value() ||
      !operand.record->stored_value_name.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingStoredValueHome);
  }

  const auto* stored_home =
      prepare::find_prepared_value_home(value_locations, *operand.record->stored_value_id);
  if (stored_home == nullptr ||
      stored_home->value_name != *operand.record->stored_value_name ||
      stored_home->kind != prepare::PreparedValueHomeKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::MissingStoredValueHome);
  }

  const auto* stored_storage =
      find_storage_plan_value(storage_plan, *operand.record->stored_value_id);
  if (stored_storage == nullptr ||
      stored_storage->value_name != *operand.record->stored_value_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::MissingStoredStorage);
  }
  if (stored_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::UnsupportedStoredStorage);
  }

  auto stored_register =
      make_prepared_register_operand(
          *stored_home, *stored_storage, stored_value.type, RegisterOperandRole::StoragePlan);
  if (!stored_register.has_value()) {
    return memory_instruction_record_error(
        PreparedMemoryOperandRecordError::RegisterConversionFailed);
  }

  return PreparedMemoryInstructionRecordResult{
      .record =
          MemoryInstructionRecord{
              .memory_kind = MemoryInstructionKind::Store,
              .address = *operand.record,
              .value = make_register_operand(*stored_register),
              .value_type = stored_value.type,
          },
      .error = PreparedMemoryOperandRecordError::None,
  };
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

PreparedMemoryInstructionRecordResult make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  return make_store_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, store),
      value_locations,
      storage_plan,
      store.value);
}

PreparedAtomicOperationInstructionRecordResult atomic_instruction_record_error(
    PreparedAtomicOperationRecordError error) {
  return PreparedAtomicOperationInstructionRecordResult{.record = std::nullopt,
                                                        .error = error};
}

bool supported_atomic_width(std::size_t width_bytes) {
  return width_bytes == 1 || width_bytes == 2 || width_bytes == 4 || width_bytes == 8;
}

bool atomic_ordering_has_acquire(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool atomic_ordering_has_release(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_load_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_store_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_fence_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_rmw_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::Release ||
         ordering == bir::AtomicOrdering::AcqRel ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_compare_exchange_success_ordering(bir::AtomicOrdering ordering) {
  return supported_atomic_rmw_ordering(ordering);
}

bool supported_atomic_compare_exchange_failure_ordering(bir::AtomicOrdering ordering) {
  return ordering == bir::AtomicOrdering::Relaxed ||
         ordering == bir::AtomicOrdering::Acquire ||
         ordering == bir::AtomicOrdering::SeqCst;
}

bool supported_atomic_rmw_opcode(bir::AtomicRmwOpcode opcode) {
  return opcode == bir::AtomicRmwOpcode::Exchange ||
         opcode == bir::AtomicRmwOpcode::Add ||
         opcode == bir::AtomicRmwOpcode::Sub ||
         opcode == bir::AtomicRmwOpcode::And ||
         opcode == bir::AtomicRmwOpcode::Or ||
         opcode == bir::AtomicRmwOpcode::Xor;
}

PreparedAtomicOperationRecordError register_for_named_atomic_value(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    std::optional<c4c::ValueNameId> value_name,
    bir::TypeKind type,
    PreparedAtomicOperationRecordError missing_name_error,
    PreparedAtomicOperationRecordError missing_home_error,
    PreparedAtomicOperationRecordError missing_storage_error,
    std::optional<prepare::PreparedValueId>& value_id,
    std::optional<RegisterOperand>& reg) {
  if (!value_name.has_value() || *value_name == c4c::kInvalidValueName) {
    return missing_name_error;
  }
  const auto* home = prepare::find_prepared_value_home(value_locations, *value_name);
  if (home == nullptr || home->kind != prepare::PreparedValueHomeKind::Register) {
    return missing_home_error;
  }
  const auto* storage = find_storage_plan_value(storage_plan, home->value_id);
  if (storage == nullptr || storage->value_name != *value_name) {
    return missing_storage_error;
  }
  if (storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return missing_storage_error;
  }
  auto register_operand =
      make_prepared_register_operand(*home, *storage, type, RegisterOperandRole::StoragePlan);
  if (!register_operand.has_value()) {
    return PreparedAtomicOperationRecordError::RegisterConversionFailed;
  }
  value_id = home->value_id;
  reg = *register_operand;
  return PreparedAtomicOperationRecordError::None;
}

PreparedAtomicOperationInstructionRecordResult
make_prepared_atomic_operation_instruction_record(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAtomicOperationCarrier& operation) {
  if (value_locations.function_name != storage_plan.function_name ||
      operation.function_name != value_locations.function_name) {
    return atomic_instruction_record_error(
        PreparedAtomicOperationRecordError::InvalidFunction);
  }
  if (operation.carrier_kind !=
      prepare::PreparedAtomicOperationCarrierKind::Complete) {
    return atomic_instruction_record_error(
        PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation);
  }

  AtomicMemoryInstructionRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .function_name = operation.function_name,
      .block_label = operation.block_label,
      .instruction_index = operation.inst_index,
      .value_type = operation.value_type,
      .width_bytes = operation.width_bytes,
      .ordering = operation.ordering,
      .result_mode = operation.result_mode,
      .address_space = operation.address_space,
      .acquire_semantics = atomic_ordering_has_acquire(operation.ordering),
      .release_semantics = atomic_ordering_has_release(operation.ordering),
      .sequentially_consistent = operation.ordering == bir::AtomicOrdering::SeqCst,
      .memory_barrier_required = operation.ordering != bir::AtomicOrdering::Relaxed,
      .source_carrier = &operation,
  };

  switch (operation.operation_kind) {
    case bir::AtomicOperationKind::Load: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_load_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Load;
      record.result_value_name = operation.result_value_name;
      record.pointer_value_name = operation.pointer_value_name;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (operation.result_mode != bir::AtomicResultMode::LoadedValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::IncompletePreparedAtomicOperation);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      break;
    }
    case bir::AtomicOperationKind::Store: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_store_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Store;
      record.pointer_value_name = operation.pointer_value_name;
      record.stored_value_name = operation.value_name;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingStoredValueName,
              PreparedAtomicOperationRecordError::MissingStoredValueHome,
              PreparedAtomicOperationRecordError::MissingStoredStorage,
              record.stored_value_id,
              record.stored_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      break;
    }
    case bir::AtomicOperationKind::Fence:
      if (!supported_atomic_fence_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::Fence;
      record.value_type = bir::TypeKind::Void;
      record.width_bytes = 0;
      break;
    case bir::AtomicOperationKind::Rmw: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_rmw_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      if (!supported_atomic_rmw_opcode(operation.rmw_opcode)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedRmwOpcode);
      }
      if (operation.result_mode != bir::AtomicResultMode::OldValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedResultMode);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::RmwLoop;
      record.pointer_value_name = operation.pointer_value_name;
      record.stored_value_name = operation.value_name;
      record.result_value_name = operation.result_value_name;
      record.rmw_opcode = operation.rmw_opcode;
      record.exclusive_retry_loop = true;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingStoredValueName,
              PreparedAtomicOperationRecordError::MissingStoredValueHome,
              PreparedAtomicOperationRecordError::MissingStoredStorage,
              record.stored_value_id,
              record.stored_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      const auto value_view = atomic_value_register_view(operation.width_bytes);
      if (!value_view.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      record.rmw_new_value_register =
          next_reserved_gp_scratch_operand(record, *value_view);
      record.exclusive_status_register =
          next_reserved_gp_scratch_operand(record, abi::RegisterView::W);
      if (!record.rmw_new_value_register.has_value() ||
          !record.exclusive_status_register.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::RegisterConversionFailed);
      }
      break;
    }
    case bir::AtomicOperationKind::CompareExchange: {
      if (!supported_atomic_width(operation.width_bytes)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (!supported_atomic_compare_exchange_success_ordering(operation.ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedOrdering);
      }
      if (!supported_atomic_compare_exchange_failure_ordering(
              operation.failure_ordering)) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedFailureOrdering);
      }
      if (operation.result_mode != bir::AtomicResultMode::BooleanSuccess &&
          operation.result_mode != bir::AtomicResultMode::OldValue) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedResultMode);
      }
      record.atomic_kind = AtomicMemoryInstructionKind::CompareExchangeLoop;
      record.pointer_value_name = operation.pointer_value_name;
      record.expected_value_name = operation.expected_value_name;
      record.desired_value_name = operation.desired_value_name;
      record.result_value_name = operation.result_value_name;
      record.failure_ordering = operation.failure_ordering;
      record.exclusive_retry_loop = true;
      record.compare_exchange_failure_clears_monitor = true;
      record.compare_exchange_result_is_boolean =
          operation.result_mode == bir::AtomicResultMode::BooleanSuccess;
      record.compare_exchange_result_is_old_value =
          operation.result_mode == bir::AtomicResultMode::OldValue;
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.pointer_value_name,
              bir::TypeKind::Ptr,
              PreparedAtomicOperationRecordError::MissingPointerValueName,
              PreparedAtomicOperationRecordError::MissingPointerValueHome,
              PreparedAtomicOperationRecordError::MissingPointerValueStorage,
              record.pointer_value_id,
              record.pointer_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.expected_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingExpectedValueName,
              PreparedAtomicOperationRecordError::MissingExpectedValueHome,
              PreparedAtomicOperationRecordError::MissingExpectedStorage,
              record.expected_value_id,
              record.expected_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.desired_value_name,
              operation.value_type,
              PreparedAtomicOperationRecordError::MissingDesiredValueName,
              PreparedAtomicOperationRecordError::MissingDesiredValueHome,
              PreparedAtomicOperationRecordError::MissingDesiredStorage,
              record.desired_value_id,
              record.desired_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      if (const auto error = register_for_named_atomic_value(
              value_locations,
              storage_plan,
              operation.result_value_name,
              operation.result_mode == bir::AtomicResultMode::BooleanSuccess
                  ? bir::TypeKind::I1
                  : operation.value_type,
              PreparedAtomicOperationRecordError::MissingResultValueName,
              PreparedAtomicOperationRecordError::MissingResultValueHome,
              PreparedAtomicOperationRecordError::MissingResultStorage,
              record.result_value_id,
              record.result_register);
          error != PreparedAtomicOperationRecordError::None) {
        return atomic_instruction_record_error(error);
      }
      const auto value_view = atomic_value_register_view(operation.width_bytes);
      if (!value_view.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::UnsupportedWidth);
      }
      if (record.compare_exchange_result_is_boolean) {
        record.compare_loaded_register =
            next_reserved_gp_scratch_operand(record, *value_view);
      }
      record.exclusive_status_register =
          next_reserved_gp_scratch_operand(record, abi::RegisterView::W);
      if ((record.compare_exchange_result_is_boolean &&
           !record.compare_loaded_register.has_value()) ||
          !record.exclusive_status_register.has_value()) {
        return atomic_instruction_record_error(
            PreparedAtomicOperationRecordError::RegisterConversionFailed);
      }
      break;
    }
    case bir::AtomicOperationKind::None:
      return atomic_instruction_record_error(
          PreparedAtomicOperationRecordError::UnsupportedOperationKind);
  }

  return PreparedAtomicOperationInstructionRecordResult{
      .record = std::move(record),
      .error = PreparedAtomicOperationRecordError::None,
  };
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

PreparedMemoryInstructionRecordResult make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store) {
  if (storage_plan.function_name != value_locations.function_name ||
      storage_plan.function_name != addressing.function_name) {
    return memory_instruction_record_error(PreparedMemoryOperandRecordError::InvalidFunction);
  }
  return make_store_memory_instruction_record(
      make_prepared_memory_operand_record(
          names, value_locations, addressing, block_label, instruction_index, store),
      value_locations,
      storage_plan,
      store.value);
}

PreparedI128TransportRecordResult i128_transport_record_error(
    PreparedI128TransportRecordError error) {
  return PreparedI128TransportRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedF128TransportRecordResult f128_transport_record_error(
    PreparedF128TransportRecordError error) {
  return PreparedF128TransportRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedF128RuntimeHelperRecordResult f128_runtime_helper_record_error(
    PreparedF128RuntimeHelperRecordError error) {
  return PreparedF128RuntimeHelperRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

std::optional<RegisterOperand> make_i128_lane_register_operand(
    const prepare::PreparedI128Carrier& carrier,
    const prepare::PreparedI128LaneCarrier& lane) {
  if (!lane.register_name.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *lane.register_name, carrier.register_bank, carrier.register_class, abi::RegisterView::X);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::X,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_register_operand(
    const prepare::PreparedF128Carrier& carrier) {
  if (!carrier.register_name.has_value()) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *carrier.register_name, carrier.register_bank, carrier.register_class, abi::RegisterView::Q);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = carrier.value_id,
      .value_name = carrier.value_name,
      .prepared_class = carrier.register_class,
      .prepared_bank = carrier.register_bank,
      .expected_view = abi::RegisterView::Q,
      .contiguous_width = carrier.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_abi_register_operand(
    const prepare::PreparedF128RuntimeHelper::AbiRegisterBinding& binding) {
  abi::RegisterView view = abi::RegisterView::Q;
  if (binding.register_bank == prepare::PreparedRegisterBank::Gpr) {
    view = abi::RegisterView::W;
  } else if (binding.register_bank == prepare::PreparedRegisterBank::Fpr) {
    view = binding.width_bytes == 4 ? abi::RegisterView::S : abi::RegisterView::D;
  }
  const auto converted = abi::convert_prepared_register(
      binding.register_name, binding.register_bank, binding.register_class, view);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::CallAbi,
      .value_id = binding.value_id,
      .value_name = binding.value_name,
      .prepared_class = binding.register_class,
      .prepared_bank = binding.register_bank,
      .expected_view = view,
      .contiguous_width = binding.contiguous_width,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_scalar_register_operand(
    const prepare::PreparedF128RuntimeHelper::ScalarResultOwnership& scalar) {
  if (!scalar.register_name.has_value()) {
    return std::nullopt;
  }
  const abi::RegisterView view =
      scalar.register_bank == prepare::PreparedRegisterBank::Gpr
          ? abi::RegisterView::W
          : (scalar.width_bytes == 4 ? abi::RegisterView::S : abi::RegisterView::D);
  const auto converted = abi::convert_prepared_register(
      *scalar.register_name,
      scalar.register_bank,
      register_class_from_bank(scalar.register_bank),
      view);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .prepared_class = register_class_from_bank(scalar.register_bank),
      .prepared_bank = scalar.register_bank,
      .expected_view = view,
      .contiguous_width = 1,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

std::optional<RegisterOperand> make_f128_cmp_materialized_i1_register_operand(
    const prepare::PreparedF128RuntimeHelper::ScalarResultOwnership& scalar) {
  if (!scalar.register_name.has_value() ||
      scalar.register_bank != prepare::PreparedRegisterBank::Gpr) {
    return std::nullopt;
  }
  const auto converted = abi::convert_prepared_register(
      *scalar.register_name,
      scalar.register_bank,
      prepare::PreparedRegisterClass::General,
      abi::RegisterView::W);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::StoragePlan,
      .value_id = scalar.value_id,
      .value_name = scalar.value_name,
      .prepared_class = prepare::PreparedRegisterClass::General,
      .prepared_bank = scalar.register_bank,
      .expected_view = abi::RegisterView::W,
      .contiguous_width = 1,
      .occupied_register_references = occupied_register_references(*converted.reg),
      .occupied_registers = occupied_register_views(*converted.reg),
  };
}

PreparedI128TransportRecordResult make_prepared_i128_carrier_transport_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    c4c::ValueNameId value_name,
    I128TransportKind transport_kind,
    std::optional<MemoryOperand> memory) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName) {
    return i128_transport_record_error(PreparedI128TransportRecordError::InvalidFunction);
  }
  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_name);
  if (carrier == nullptr) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::MissingPreparedI128Carrier);
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }
  if (carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return i128_transport_record_error(
        PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }
  if (transport_kind != I128TransportKind::CarrierSnapshot) {
    if (!memory.has_value()) {
      return i128_transport_record_error(PreparedI128TransportRecordError::MissingMemoryOperand);
    }
    if (memory->size_bytes != carrier->total_size_bytes) {
      return i128_transport_record_error(
          PreparedI128TransportRecordError::MemoryAccessSizeMismatch);
    }
  }

  I128TransportRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .transport_kind = transport_kind,
      .function_name = i128_carriers.function_name,
      .block_label = memory.has_value() ? memory->block_label : c4c::kInvalidBlockLabel,
      .instruction_index = memory.has_value() ? memory->instruction_index : 0,
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .value_type = bir::TypeKind::I128,
      .carrier_kind = carrier->kind,
      .lane_width_bytes = carrier->lane_width_bytes,
      .total_size_bytes = carrier->total_size_bytes,
      .total_align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .contiguous_width = carrier->contiguous_width,
      .occupied_register_names = carrier->occupied_register_names,
      .register_placement = carrier->register_placement,
      .slot_id = carrier->slot_id,
      .stack_offset_bytes = carrier->stack_offset_bytes,
      .low_lane =
          I128LaneTransportRecord{
              .role = carrier->low_lane.role,
              .lane_index = carrier->low_lane.lane_index,
              .width_bytes = carrier->low_lane.width_bytes,
              .slot_id = carrier->low_lane.slot_id,
              .stack_offset_bytes = carrier->low_lane.stack_offset_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = carrier->high_lane.role,
              .lane_index = carrier->high_lane.lane_index,
              .width_bytes = carrier->high_lane.width_bytes,
              .slot_id = carrier->high_lane.slot_id,
              .stack_offset_bytes = carrier->high_lane.stack_offset_bytes,
          },
      .memory = std::move(memory),
      .source_carrier = carrier,
  };

  switch (carrier->kind) {
    case prepare::PreparedI128CarrierKind::RegisterPair: {
      record.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
      record.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
      if (!record.low_lane.reg.has_value() || !record.high_lane.reg.has_value()) {
        return i128_transport_record_error(
            PreparedI128TransportRecordError::RegisterConversionFailed);
      }
      break;
    }
    case prepare::PreparedI128CarrierKind::MemoryBacked:
      if (!record.low_lane.slot_id.has_value() ||
          !record.high_lane.slot_id.has_value() ||
          !record.low_lane.stack_offset_bytes.has_value() ||
          !record.high_lane.stack_offset_bytes.has_value()) {
        return i128_transport_record_error(
            PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
      }
      break;
    case prepare::PreparedI128CarrierKind::Missing:
      return i128_transport_record_error(
          PreparedI128TransportRecordError::IncompletePreparedI128Carrier);
  }

  return PreparedI128TransportRecordResult{
      .record = std::move(record),
      .error = PreparedI128TransportRecordError::None,
  };
}

PreparedF128TransportRecordResult make_prepared_f128_carrier_transport_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    c4c::ValueNameId value_name,
    F128TransportKind transport_kind,
    std::optional<MemoryOperand> memory) {
  if (f128_carriers.function_name == c4c::kInvalidFunctionName) {
    return f128_transport_record_error(PreparedF128TransportRecordError::InvalidFunction);
  }
  const auto* carrier = prepare::find_prepared_f128_carrier(f128_carriers, value_name);
  if (carrier == nullptr) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::MissingPreparedF128Carrier);
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedF128CarrierKind::Missing) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }
  if (carrier->total_size_bytes != 16 || carrier->total_align_bytes != 16) {
    return f128_transport_record_error(
        PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }
  if (transport_kind != F128TransportKind::CarrierSnapshot) {
    if (!memory.has_value()) {
      return f128_transport_record_error(PreparedF128TransportRecordError::MissingMemoryOperand);
    }
    if (memory->size_bytes != carrier->total_size_bytes) {
      return f128_transport_record_error(
          PreparedF128TransportRecordError::MemoryAccessSizeMismatch);
    }
  }

  F128TransportRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .transport_kind = transport_kind,
      .function_name = f128_carriers.function_name,
      .block_label = memory.has_value() ? memory->block_label : c4c::kInvalidBlockLabel,
      .instruction_index = memory.has_value() ? memory->instruction_index : 0,
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .value_type = bir::TypeKind::F128,
      .carrier_kind = carrier->kind,
      .total_size_bytes = carrier->total_size_bytes,
      .total_align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .contiguous_width = carrier->contiguous_width,
      .occupied_register_names = carrier->occupied_register_names,
      .register_placement = carrier->register_placement,
      .slot_id = carrier->slot_id,
      .stack_offset_bytes = carrier->stack_offset_bytes,
      .memory = std::move(memory),
      .source_carrier = carrier,
  };

  switch (carrier->kind) {
    case prepare::PreparedF128CarrierKind::FullWidthRegister:
      record.reg = make_f128_register_operand(*carrier);
      if (!record.reg.has_value()) {
        return f128_transport_record_error(
            PreparedF128TransportRecordError::RegisterConversionFailed);
      }
      break;
    case prepare::PreparedF128CarrierKind::MemoryBacked:
      if (!record.slot_id.has_value() || !record.stack_offset_bytes.has_value()) {
        return f128_transport_record_error(
            PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
      }
      break;
    case prepare::PreparedF128CarrierKind::Missing:
      return f128_transport_record_error(
          PreparedF128TransportRecordError::IncompletePreparedF128Carrier);
  }

  return PreparedF128TransportRecordResult{
      .record = std::move(record),
      .error = PreparedF128TransportRecordError::None,
  };
}

PreparedI128PairRecordResult i128_pair_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128PairRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

bool is_supported_i128_pair_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return true;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      return false;
  }
  return false;
}

I128PairOperationKind i128_pair_operation_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return I128PairOperationKind::Add;
    case bir::BinaryOpcode::Sub:
      return I128PairOperationKind::Sub;
    case bir::BinaryOpcode::And:
      return I128PairOperationKind::And;
    case bir::BinaryOpcode::Or:
      return I128PairOperationKind::Or;
    case bir::BinaryOpcode::Xor:
      return I128PairOperationKind::Xor;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128PairOperationKind::Add;
}

I128PairLaneSemantics i128_pair_lane_semantics_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Add:
      return I128PairLaneSemantics::CarryPropagating;
    case bir::BinaryOpcode::Sub:
      return I128PairLaneSemantics::BorrowPropagating;
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
      return I128PairLaneSemantics::IndependentBitwise;
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::SDiv:
    case bir::BinaryOpcode::UDiv:
    case bir::BinaryOpcode::SRem:
    case bir::BinaryOpcode::URem:
    case bir::BinaryOpcode::Shl:
    case bir::BinaryOpcode::AShr:
    case bir::BinaryOpcode::LShr:
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Sge:
    case bir::BinaryOpcode::Uge:
      break;
  }
  return I128PairLaneSemantics::IndependentBitwise;
}

PreparedI128PairRecordError make_i128_pair_operand_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    c4c::ValueNameId value_name,
    I128PairOperandRecord& operand) {
  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_name);
  if (carrier == nullptr) {
    return PreparedI128PairRecordError::MissingPreparedI128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return PreparedI128PairRecordError::IncompletePreparedI128Carrier;
  }
  if (carrier->kind != prepare::PreparedI128CarrierKind::RegisterPair) {
    return PreparedI128PairRecordError::UnsupportedCarrierKind;
  }

  operand = I128PairOperandRecord{
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .carrier_kind = carrier->kind,
      .low_lane =
          I128LaneTransportRecord{
              .role = carrier->low_lane.role,
              .lane_index = carrier->low_lane.lane_index,
              .width_bytes = carrier->low_lane.width_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = carrier->high_lane.role,
              .lane_index = carrier->high_lane.lane_index,
              .width_bytes = carrier->high_lane.width_bytes,
          },
      .source_carrier = carrier,
  };
  operand.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
  operand.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
  if (!operand.low_lane.reg.has_value() || !operand.high_lane.reg.has_value()) {
    return PreparedI128PairRecordError::RegisterConversionFailed;
  }
  return PreparedI128PairRecordError::None;
}

PreparedI128PairRecordResult make_prepared_i128_pair_operation_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName) {
    return i128_pair_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I128) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_supported_i128_pair_opcode(binary.opcode)) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (binary.lhs.kind != bir::Value::Kind::Named || binary.rhs.kind != bir::Value::Kind::Named ||
      binary.lhs.name.empty() || binary.rhs.name.empty()) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }

  I128PairOperationRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .operation = i128_pair_operation_from_binary_opcode(binary.opcode),
      .lane_semantics = i128_pair_lane_semantics_from_binary_opcode(binary.opcode),
      .source_binary_opcode = binary.opcode,
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };

  const auto result_name = names.value_names.find(binary.result.name);
  const auto lhs_name = names.value_names.find(binary.lhs.name);
  const auto rhs_name = names.value_names.find(binary.rhs.name);
  if (result_name == c4c::kInvalidValueName) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (lhs_name == c4c::kInvalidValueName || rhs_name == c4c::kInvalidValueName) {
    return i128_pair_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }

  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, result_name, record.result);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, lhs_name, record.lhs);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, rhs_name, record.rhs);
      error != PreparedI128PairRecordError::None) {
    return i128_pair_record_error(error);
  }

  return PreparedI128PairRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

PreparedI128ShiftRecordResult i128_shift_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128ShiftRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedI128CompareRecordResult i128_compare_record_error(
    PreparedI128PairRecordError error) {
  return PreparedI128CompareRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

PreparedI128RuntimeHelperRecordResult i128_runtime_helper_record_error(
    PreparedI128RuntimeHelperRecordError error) {
  return PreparedI128RuntimeHelperRecordResult{
      .record = std::nullopt,
      .error = error,
  };
}

bool is_i128_shift_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::Shl || opcode == bir::BinaryOpcode::LShr ||
         opcode == bir::BinaryOpcode::AShr;
}

I128ShiftKind i128_shift_kind_from_binary_opcode(bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Shl:
      return I128ShiftKind::Left;
    case bir::BinaryOpcode::LShr:
      return I128ShiftKind::LogicalRight;
    case bir::BinaryOpcode::AShr:
      return I128ShiftKind::ArithmeticRight;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
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
      break;
  }
  return I128ShiftKind::Left;
}

I128ShiftLaneSemantics i128_shift_lane_semantics_from_binary_opcode(
    bir::BinaryOpcode opcode) {
  switch (opcode) {
    case bir::BinaryOpcode::Shl:
      return I128ShiftLaneSemantics::CrossLaneLeft;
    case bir::BinaryOpcode::LShr:
      return I128ShiftLaneSemantics::CrossLaneLogicalRight;
    case bir::BinaryOpcode::AShr:
      return I128ShiftLaneSemantics::CrossLaneArithmeticRight;
    case bir::BinaryOpcode::Add:
    case bir::BinaryOpcode::Sub:
    case bir::BinaryOpcode::Mul:
    case bir::BinaryOpcode::And:
    case bir::BinaryOpcode::Or:
    case bir::BinaryOpcode::Xor:
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
      break;
  }
  return I128ShiftLaneSemantics::CrossLaneLeft;
}

bool is_supported_i128_shift_count(const bir::Value& count) {
  if (count.kind != bir::Value::Kind::Immediate) {
    return scalar_register_view(count.type).has_value();
  }
  return count.immediate >= 0 && count.immediate < 128;
}

PreparedI128ShiftRecordResult make_prepared_i128_shift_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != i128_carriers.function_name ||
      storage_plan.function_name != i128_carriers.function_name) {
    return i128_shift_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I128) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_i128_shift_opcode(binary.opcode)) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.lhs.kind != bir::Value::Kind::Named ||
      binary.result.name.empty() || binary.lhs.name.empty()) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (!is_supported_i128_shift_count(binary.rhs)) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedShiftCount);
  }

  I128ShiftRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .shift_kind = i128_shift_kind_from_binary_opcode(binary.opcode),
      .lane_semantics = i128_shift_lane_semantics_from_binary_opcode(binary.opcode),
      .count_kind = binary.rhs.kind == bir::Value::Kind::Immediate
                        ? I128ShiftCountKind::Immediate
                        : I128ShiftCountKind::Register,
      .source_binary_opcode = binary.opcode,
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };
  const auto result_name = names.value_names.find(binary.result.name);
  const auto source_name = names.value_names.find(binary.lhs.name);
  if (result_name == c4c::kInvalidValueName || source_name == c4c::kInvalidValueName) {
    return i128_shift_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, result_name, record.result);
      error != PreparedI128PairRecordError::None) {
    return i128_shift_record_error(error);
  }
  if (const auto error =
          make_i128_pair_operand_record(i128_carriers, source_name, record.source);
      error != PreparedI128PairRecordError::None) {
    return i128_shift_record_error(error);
  }
  OperandRecord count;
  if (const auto error =
          make_prepared_scalar_operand(names, value_locations, storage_plan, binary.rhs, count);
      error != PreparedScalarAluRecordError::None) {
    return i128_shift_record_error(PreparedI128PairRecordError::MissingShiftCountStorage);
  }
  record.shift_count = count;
  return PreparedI128ShiftRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

I128CompareSignedness i128_compare_signedness_from_predicate(
    bir::BinaryOpcode predicate) {
  switch (predicate) {
    case bir::BinaryOpcode::Slt:
    case bir::BinaryOpcode::Sle:
    case bir::BinaryOpcode::Sgt:
    case bir::BinaryOpcode::Sge:
      return I128CompareSignedness::Signed;
    case bir::BinaryOpcode::Ult:
    case bir::BinaryOpcode::Ule:
    case bir::BinaryOpcode::Ugt:
    case bir::BinaryOpcode::Uge:
      return I128CompareSignedness::Unsigned;
    case bir::BinaryOpcode::Eq:
    case bir::BinaryOpcode::Ne:
      return I128CompareSignedness::Equality;
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
      break;
  }
  return I128CompareSignedness::Equality;
}

I128CompareHighWordSemantics i128_compare_high_word_semantics_from_predicate(
    bir::BinaryOpcode predicate) {
  switch (i128_compare_signedness_from_predicate(predicate)) {
    case I128CompareSignedness::Equality:
      return I128CompareHighWordSemantics::EqualityBothLanes;
    case I128CompareSignedness::Signed:
      return I128CompareHighWordSemantics::SignedHighWordFirst;
    case I128CompareSignedness::Unsigned:
      return I128CompareHighWordSemantics::UnsignedHighWordFirst;
  }
  return I128CompareHighWordSemantics::EqualityBothLanes;
}

PreparedI128CompareRecordResult make_prepared_i128_compare_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const bir::BinaryInst& binary) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      value_locations.function_name != i128_carriers.function_name ||
      storage_plan.function_name != i128_carriers.function_name) {
    return i128_compare_record_error(PreparedI128PairRecordError::InvalidFunction);
  }
  if (binary.operand_type != bir::TypeKind::I128 ||
      binary.result.type != bir::TypeKind::I1) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandType);
  }
  if (!is_compare_predicate(binary.opcode)) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOpcode);
  }
  if (binary.result.kind != bir::Value::Kind::Named || binary.result.name.empty()) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedResultValue);
  }
  if (binary.lhs.kind != bir::Value::Kind::Named || binary.rhs.kind != bir::Value::Kind::Named ||
      binary.lhs.name.empty() || binary.rhs.name.empty()) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  const auto* result_home = find_named_value_home(names, value_locations, binary.result);
  if (result_home == nullptr || result_home->value_name == c4c::kInvalidValueName) {
    return i128_compare_record_error(PreparedI128PairRecordError::MissingScalarResultValueHome);
  }
  const auto* result_storage = find_storage_plan_value(storage_plan, result_home->value_id);
  if (result_storage == nullptr || result_storage->value_name != result_home->value_name) {
    return i128_compare_record_error(PreparedI128PairRecordError::MissingScalarResultStorage);
  }
  if (result_home->kind != prepare::PreparedValueHomeKind::Register ||
      result_storage->encoding != prepare::PreparedStorageEncodingKind::Register) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedScalarResultStorage);
  }
  const auto result_register =
      make_prepared_register_operand(*result_home,
                                     *result_storage,
                                     binary.result.type,
                                     RegisterOperandRole::StoragePlan);
  if (!result_register.has_value()) {
    return i128_compare_record_error(PreparedI128PairRecordError::RegisterConversionFailed);
  }

  I128CompareRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .predicate = binary.opcode,
      .signedness = i128_compare_signedness_from_predicate(binary.opcode),
      .high_word_semantics =
          i128_compare_high_word_semantics_from_predicate(binary.opcode),
      .function_name = i128_carriers.function_name,
      .operand_type = binary.operand_type,
      .result_type = binary.result.type,
      .result_value_id = result_home->value_id,
      .result_value_name = result_home->value_name,
      .result_register = result_register,
      .lane_width_bytes = 8,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
  };
  const auto lhs_name = names.value_names.find(binary.lhs.name);
  const auto rhs_name = names.value_names.find(binary.rhs.name);
  if (lhs_name == c4c::kInvalidValueName || rhs_name == c4c::kInvalidValueName) {
    return i128_compare_record_error(PreparedI128PairRecordError::UnsupportedOperandValue);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, lhs_name, record.lhs);
      error != PreparedI128PairRecordError::None) {
    return i128_compare_record_error(error);
  }
  if (const auto error = make_i128_pair_operand_record(i128_carriers, rhs_name, record.rhs);
      error != PreparedI128PairRecordError::None) {
    return i128_compare_record_error(error);
  }
  return PreparedI128CompareRecordResult{
      .record = std::move(record),
      .error = PreparedI128PairRecordError::None,
  };
}

bool is_i128_div_rem_opcode(bir::BinaryOpcode opcode) {
  return opcode == bir::BinaryOpcode::SDiv || opcode == bir::BinaryOpcode::UDiv ||
         opcode == bir::BinaryOpcode::SRem || opcode == bir::BinaryOpcode::URem;
}

I128RuntimeHelperBoundaryKind i128_runtime_helper_boundary_kind_from_prepared(
    prepare::PreparedI128RuntimeHelperKind kind) {
  switch (kind) {
    case prepare::PreparedI128RuntimeHelperKind::SignedDiv:
      return I128RuntimeHelperBoundaryKind::SignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedDiv:
      return I128RuntimeHelperBoundaryKind::UnsignedDiv;
    case prepare::PreparedI128RuntimeHelperKind::SignedRem:
      return I128RuntimeHelperBoundaryKind::SignedRem;
    case prepare::PreparedI128RuntimeHelperKind::UnsignedRem:
      return I128RuntimeHelperBoundaryKind::UnsignedRem;
  }
  return I128RuntimeHelperBoundaryKind::SignedDiv;
}

PreparedI128RuntimeHelperRecordError make_i128_helper_operand_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const std::optional<prepare::PreparedI128RuntimeHelper::LaneBinding>& low,
    const std::optional<prepare::PreparedI128RuntimeHelper::LaneBinding>& high,
    I128PairOperandRecord& operand) {
  if (!low.has_value() || !high.has_value()) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }
  if (low->value_id != value_id || high->value_id != value_id ||
      low->value_name != value_name || high->value_name != value_name ||
      low->role != prepare::PreparedI128LaneRole::Low ||
      high->role != prepare::PreparedI128LaneRole::High ||
      low->lane_index != 0 || high->lane_index != 1 ||
      low->width_bytes != 8 || high->width_bytes != 8) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }

  const auto* carrier = prepare::find_prepared_i128_carrier(i128_carriers, value_id);
  if (carrier == nullptr || carrier->value_name != value_name) {
    return PreparedI128RuntimeHelperRecordError::MissingPreparedI128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedI128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 || carrier->lane_width_bytes != 8) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128Carrier;
  }
  if (carrier->kind != prepare::PreparedI128CarrierKind::RegisterPair) {
    return PreparedI128RuntimeHelperRecordError::UnsupportedCarrierKind;
  }
  if (carrier->low_lane.register_name != low->register_name ||
      carrier->high_lane.register_name != high->register_name) {
    return PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper;
  }

  operand = I128PairOperandRecord{
      .value_id = carrier->value_id,
      .value_name = carrier->value_name,
      .carrier_kind = carrier->kind,
      .low_lane =
          I128LaneTransportRecord{
              .role = low->role,
              .lane_index = low->lane_index,
              .width_bytes = low->width_bytes,
          },
      .high_lane =
          I128LaneTransportRecord{
              .role = high->role,
              .lane_index = high->lane_index,
              .width_bytes = high->width_bytes,
          },
      .source_carrier = carrier,
  };
  operand.low_lane.reg = make_i128_lane_register_operand(*carrier, carrier->low_lane);
  operand.high_lane.reg = make_i128_lane_register_operand(*carrier, carrier->high_lane);
  if (!operand.low_lane.reg.has_value() || !operand.high_lane.reg.has_value()) {
    return PreparedI128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedI128RuntimeHelperRecordError::None;
}

PreparedI128RuntimeHelperRecordResult make_prepared_i128_runtime_helper_boundary_record(
    const prepare::PreparedI128CarrierFunction& i128_carriers,
    const prepare::PreparedI128RuntimeHelper& helper) {
  if (i128_carriers.function_name == c4c::kInvalidFunctionName ||
      helper.function_name != i128_carriers.function_name) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::InvalidFunction);
  }
  if (helper.helper_family != prepare::PreparedI128RuntimeHelperFamily::DivRem) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedHelperFamily);
  }
  if (!is_i128_div_rem_opcode(helper.source_binary_opcode) ||
      helper.source_type != bir::TypeKind::I128 ||
      helper.result_type != bir::TypeKind::I128 ||
      helper.callee_name.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  if (!helper.missing_required_facts.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper);
  }
  if (helper.result_ownership !=
      prepare::PreparedI128RuntimeHelperResultOwnership::DirectLowHighLanes) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::UnsupportedResultOwnership);
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingBoundaryResourcePolicy);
  }
  if (helper.abi_policy.transition !=
          prepare::PreparedI128RuntimeHelperAbiTransition::
              DirectRegisterPairArgumentsAndResult ||
      helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr ||
      helper.abi_policy.argument_count != 2 ||
      helper.abi_policy.lanes_per_argument != 2 ||
      helper.abi_policy.result_lane_count != 2 ||
      helper.abi_policy.lane_width_bytes != 8) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingBoundaryAbiPolicy);
  }
  if (helper.clobbered_registers.empty()) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::MissingClobberPolicy);
  }
  if (!helper.live_preservation_policy.evaluated ||
      !helper.live_preservation_policy.caller_saved_clobbers_modeled ||
      !helper.live_preservation_policy.no_additional_live_preservation_required ||
      !helper.selected_call_ownership.owns_terminal_call ||
      !helper.selected_call_ownership.has_callee_identity ||
      !helper.selected_call_ownership.has_resource_policy ||
      !helper.selected_call_ownership.has_clobber_policy ||
      !helper.selected_call_ownership.has_abi_bindings ||
      !helper.selected_call_ownership.has_marshaling ||
      !helper.selected_call_ownership.has_live_preservation) {
    return i128_runtime_helper_record_error(
        PreparedI128RuntimeHelperRecordError::IncompletePreparedI128RuntimeHelper);
  }

  I128RuntimeHelperBoundaryRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .boundary_kind = i128_runtime_helper_boundary_kind_from_prepared(helper.helper_kind),
      .helper_family = helper.helper_family,
      .helper_kind = helper.helper_kind,
      .callee_name = helper.callee_name,
      .source_binary_opcode = helper.source_binary_opcode,
      .function_name = helper.function_name,
      .block_index = helper.block_index,
      .instruction_index = helper.instruction_index,
      .source_type = helper.source_type,
      .result_type = helper.result_type,
      .result_value_id = helper.result_value_id,
      .result_value_name = helper.result_value_name,
      .lhs_value_id = helper.lhs_value_id,
      .lhs_value_name = helper.lhs_value_name,
      .rhs_value_id = helper.rhs_value_id,
      .rhs_value_name = helper.rhs_value_name,
      .result_ownership = helper.result_ownership,
      .lane_width_bytes = helper.abi_policy.lane_width_bytes,
      .total_size_bytes = 16,
      .total_align_bytes = 16,
      .resource_policy = helper.resource_policy,
      .abi_policy = helper.abi_policy,
      .live_preservation_policy = helper.live_preservation_policy,
      .selected_call_ownership = helper.selected_call_ownership,
      .clobbered_registers = helper.clobbered_registers,
      .source_helper = &helper,
  };
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.result_value_id,
                                          helper.result_value_name,
                                          helper.result_low_lane,
                                          helper.result_high_lane,
                                          record.result);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.lhs_value_id,
                                          helper.lhs_value_name,
                                          helper.lhs_low_lane,
                                          helper.lhs_high_lane,
                                          record.lhs);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  if (const auto error =
          make_i128_helper_operand_record(i128_carriers,
                                          helper.rhs_value_id,
                                          helper.rhs_value_name,
                                          helper.rhs_low_lane,
                                          helper.rhs_high_lane,
                                          record.rhs);
      error != PreparedI128RuntimeHelperRecordError::None) {
    return i128_runtime_helper_record_error(error);
  }
  return PreparedI128RuntimeHelperRecordResult{
      .record = std::move(record),
      .error = PreparedI128RuntimeHelperRecordError::None,
  };
}

PreparedF128RuntimeHelperRecordError make_f128_helper_operand_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name,
    const std::optional<prepare::PreparedF128RuntimeHelper::CarrierBinding>& carrier_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::MarshalingMove>& marshaling_move,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    F128RuntimeHelperOperandRecord& operand) {
  if (!carrier_binding.has_value() ||
      !abi_binding.has_value() ||
      !marshaling_move.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (carrier_binding->value_id != value_id ||
      carrier_binding->value_name != value_name ||
      abi_binding->value_id != value_id ||
      abi_binding->value_name != value_name ||
      marshaling_move->carrier.value_id != value_id ||
      marshaling_move->carrier.value_name != value_name ||
      marshaling_move->abi_register.value_id != value_id ||
      marshaling_move->abi_register.value_name != value_name ||
      marshaling_move->direction != direction) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (carrier_binding->carrier_kind != prepare::PreparedF128CarrierKind::FullWidthRegister ||
      carrier_binding->width_bytes != 16 ||
      carrier_binding->align_bytes != 16 ||
      carrier_binding->register_bank != prepare::PreparedRegisterBank::Vreg ||
      carrier_binding->register_class != prepare::PreparedRegisterClass::Vector ||
      !carrier_binding->register_name.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (abi_binding->width_bytes != 16 ||
      abi_binding->register_bank != prepare::PreparedRegisterBank::Vreg ||
      abi_binding->register_class != prepare::PreparedRegisterClass::Vector ||
      abi_binding->register_name.empty() ||
      abi_binding->contiguous_width != 1 ||
      abi_binding->occupied_register_names.empty() ||
      !abi_binding->register_placement.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }

  const auto* carrier = prepare::find_prepared_f128_carrier(f128_carriers, value_id);
  if (carrier == nullptr || carrier->value_name != value_name) {
    return PreparedF128RuntimeHelperRecordError::MissingPreparedF128Carrier;
  }
  if (!carrier->missing_required_facts.empty() ||
      carrier->kind == prepare::PreparedF128CarrierKind::Missing ||
      carrier->total_size_bytes != 16 ||
      carrier->total_align_bytes != 16) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128Carrier;
  }
  if (carrier->kind != prepare::PreparedF128CarrierKind::FullWidthRegister) {
    return PreparedF128RuntimeHelperRecordError::UnsupportedCarrierKind;
  }
  if (!carrier->register_name.has_value() ||
      carrier->register_name != carrier_binding->register_name ||
      carrier->register_bank != carrier_binding->register_bank ||
      carrier->register_class != carrier_binding->register_class) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }

  operand = F128RuntimeHelperOperandRecord{
      .value_id = value_id,
      .value_name = value_name,
      .carrier_kind = carrier->kind,
      .width_bytes = carrier->total_size_bytes,
      .align_bytes = carrier->total_align_bytes,
      .register_bank = carrier->register_bank,
      .register_class = carrier->register_class,
      .carrier_binding = *carrier_binding,
      .abi_binding = *abi_binding,
      .marshaling_move = *marshaling_move,
      .source_carrier = carrier,
  };
  operand.carrier_register = make_f128_register_operand(*carrier);
  operand.abi_register = make_f128_abi_register_operand(*abi_binding);
  if (!operand.carrier_register.has_value() || !operand.abi_register.has_value()) {
    return PreparedF128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedF128RuntimeHelperRecordError::None;
}

PreparedF128RuntimeHelperRecordError make_f128_helper_scalar_record(
    const std::optional<prepare::PreparedF128RuntimeHelper::ScalarResultOwnership>& scalar,
    const std::optional<prepare::PreparedF128RuntimeHelper::AbiRegisterBinding>& abi_binding,
    const std::optional<prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove>& marshaling_move,
    prepare::PreparedF128RuntimeHelperMarshalDirection direction,
    F128RuntimeHelperScalarResultRecord& record) {
  if (!scalar.has_value() || !abi_binding.has_value() || !marshaling_move.has_value()) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (marshaling_move->direction != direction ||
      marshaling_move->scalar_result.value_id != scalar->value_id ||
      marshaling_move->scalar_result.value_name != scalar->value_name ||
      marshaling_move->abi_register.value_id != abi_binding->value_id ||
      marshaling_move->abi_register.value_name != abi_binding->value_name) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  if (scalar->register_bank != prepare::PreparedRegisterBank::Fpr ||
      (scalar->width_bytes != 4 && scalar->width_bytes != 8) ||
      abi_binding->register_bank != prepare::PreparedRegisterBank::Fpr ||
      abi_binding->width_bytes != scalar->width_bytes) {
    return PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper;
  }
  record = F128RuntimeHelperScalarResultRecord{
      .value_id = scalar->value_id,
      .value_name = scalar->value_name,
      .type = scalar->type,
      .width_bytes = scalar->width_bytes,
      .register_bank = scalar->register_bank,
      .home_kind = scalar->home_kind,
      .materialized_i1_register = make_f128_scalar_register_operand(*scalar),
      .abi_register = make_f128_abi_register_operand(*abi_binding),
      .scalar_ownership = *scalar,
      .marshaling_move = marshaling_move,
  };
  if (!record.materialized_i1_register.has_value() || !record.abi_register.has_value()) {
    return PreparedF128RuntimeHelperRecordError::RegisterConversionFailed;
  }
  return PreparedF128RuntimeHelperRecordError::None;
}

PreparedF128RuntimeHelperRecordResult make_prepared_f128_runtime_helper_boundary_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    const prepare::PreparedF128RuntimeHelper& helper) {
  if (f128_carriers.function_name == c4c::kInvalidFunctionName ||
      helper.function_name != f128_carriers.function_name) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::InvalidFunction);
  }
  if (helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Arithmetic) {
    if (helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Comparison &&
        helper.helper_family != prepare::PreparedF128RuntimeHelperFamily::Cast) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::UnsupportedHelperFamily);
    }
  }
  auto boundary_kind = F128RuntimeHelperBoundaryKind::Add;
  if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Add &&
      helper.source_binary_opcode == bir::BinaryOpcode::Add) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Add;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Sub &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sub) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Sub;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Mul &&
             helper.source_binary_opcode == bir::BinaryOpcode::Mul) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Mul;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Div &&
             helper.source_binary_opcode == bir::BinaryOpcode::SDiv) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Div;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Eq &&
             helper.source_binary_opcode == bir::BinaryOpcode::Eq) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Eq;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ne &&
             helper.source_binary_opcode == bir::BinaryOpcode::Ne) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Ne;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Lt &&
             helper.source_binary_opcode == bir::BinaryOpcode::Slt) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Lt;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Le &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sle) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Le;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Gt &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sgt) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Gt;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::Ge &&
             helper.source_binary_opcode == bir::BinaryOpcode::Sge) {
    boundary_kind = F128RuntimeHelperBoundaryKind::Ge;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F32ToF128 &&
             helper.source_cast_opcode == bir::CastOpcode::FPExt &&
             helper.source_type == bir::TypeKind::F32 &&
             helper.result_type == bir::TypeKind::F128) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F32ToF128;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F64ToF128 &&
             helper.source_cast_opcode == bir::CastOpcode::FPExt &&
             helper.source_type == bir::TypeKind::F64 &&
             helper.result_type == bir::TypeKind::F128) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F64ToF128;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF32 &&
             helper.source_cast_opcode == bir::CastOpcode::FPTrunc &&
             helper.source_type == bir::TypeKind::F128 &&
             helper.result_type == bir::TypeKind::F32) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F128ToF32;
  } else if (helper.helper_kind == prepare::PreparedF128RuntimeHelperKind::F128ToF64 &&
             helper.source_cast_opcode == bir::CastOpcode::FPTrunc &&
             helper.source_type == bir::TypeKind::F128 &&
             helper.result_type == bir::TypeKind::F64) {
    boundary_kind = F128RuntimeHelperBoundaryKind::F128ToF64;
  } else {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  const bool cast_helper =
      helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Cast;
  if ((!cast_helper &&
       (helper.source_type != bir::TypeKind::F128 ||
        (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
             ? helper.result_type != bir::TypeKind::I32
             : helper.result_type != bir::TypeKind::F128))) ||
      (cast_helper && !((helper.source_type == bir::TypeKind::F32 &&
                         helper.result_type == bir::TypeKind::F128) ||
                        (helper.source_type == bir::TypeKind::F64 &&
                         helper.result_type == bir::TypeKind::F128) ||
                        (helper.source_type == bir::TypeKind::F128 &&
                         helper.result_type == bir::TypeKind::F32) ||
                        (helper.source_type == bir::TypeKind::F128 &&
                         helper.result_type == bir::TypeKind::F64))) ||
      helper.callee_name.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedSourceOperation);
  }
  if (!helper.missing_required_facts.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
  }
  if ((helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarCmpResult) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::FullWidthCarrier) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.result_ownership !=
           prepare::PreparedF128RuntimeHelperResultOwnership::ScalarValue)) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::UnsupportedResultOwnership);
  }
  if (!helper.resource_policy.call_boundary ||
      !helper.resource_policy.runtime_helper_callee ||
      !helper.resource_policy.caller_saved_clobbers ||
      !helper.resource_policy.preserves_source_operation_identity) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingBoundaryResourcePolicy);
  }
  if ((helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndResult) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentsAndCmpResult) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectScalarArgumentAndF128Result) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.abi_policy.transition !=
           prepare::PreparedF128RuntimeHelperAbiTransition::DirectF128ArgumentAndScalarResult) ||
      (!cast_helper && helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (cast_helper && helper.result_type == bir::TypeKind::F128 &&
       helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Fpr) ||
      (cast_helper && helper.source_type == bir::TypeKind::F128 &&
       helper.abi_policy.argument_bank != prepare::PreparedRegisterBank::Vreg) ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic
           ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
           : (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
                  ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Gpr
                  : (helper.result_type == bir::TypeKind::F128
                         ? helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Vreg
                         : helper.abi_policy.result_bank != prepare::PreparedRegisterBank::Fpr))) ||
      (!cast_helper ? helper.abi_policy.argument_count != 2
                    : helper.abi_policy.argument_count != 1) ||
      helper.abi_policy.result_count != 1 ||
      (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Arithmetic
           ? helper.abi_policy.width_bytes != 16
           : (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison
                  ? helper.abi_policy.width_bytes != 4
                  : (helper.result_type == bir::TypeKind::F128
                         ? helper.abi_policy.width_bytes != 16
                         : (helper.abi_policy.width_bytes != 4 &&
                            helper.abi_policy.width_bytes != 8))))) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingBoundaryAbiPolicy);
  }
  if (helper.clobbered_registers.empty()) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::MissingClobberPolicy);
  }
  if (!helper.live_preservation_policy.evaluated ||
      !helper.live_preservation_policy.caller_saved_clobbers_modeled ||
      !helper.live_preservation_policy.no_additional_live_preservation_required ||
      !helper.selected_call_ownership.owns_terminal_call ||
      !helper.selected_call_ownership.has_callee_identity ||
      !helper.selected_call_ownership.has_resource_policy ||
      !helper.selected_call_ownership.has_clobber_policy ||
      !helper.selected_call_ownership.has_abi_bindings ||
      !helper.selected_call_ownership.has_marshaling ||
      !helper.selected_call_ownership.has_live_preservation) {
    return f128_runtime_helper_record_error(
        PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
  }

  F128RuntimeHelperBoundaryRecord record{
      .surface = RecordSurfaceKind::RecordOnly,
      .boundary_kind = boundary_kind,
      .helper_family = helper.helper_family,
      .helper_kind = helper.helper_kind,
      .callee_name = helper.callee_name,
      .source_binary_opcode = helper.source_binary_opcode,
      .source_cast_opcode = helper.source_cast_opcode,
      .function_name = helper.function_name,
      .block_index = helper.block_index,
      .instruction_index = helper.instruction_index,
      .source_type = helper.source_type,
      .result_type = helper.result_type,
      .result_value_id = helper.result_value_id,
      .result_value_name = helper.result_value_name,
      .operand_value_id = helper.operand_value_id,
      .operand_value_name = helper.operand_value_name,
      .lhs_value_id = helper.lhs_value_id,
      .lhs_value_name = helper.lhs_value_name,
      .rhs_value_id = helper.rhs_value_id,
      .rhs_value_name = helper.rhs_value_name,
      .result_ownership = helper.result_ownership,
      .width_bytes = 16,
      .align_bytes = 16,
      .resource_policy = helper.resource_policy,
      .abi_policy = helper.abi_policy,
      .live_preservation_policy = helper.live_preservation_policy,
      .selected_call_ownership = helper.selected_call_ownership,
      .clobbered_registers = helper.clobbered_registers,
      .source_helper = &helper,
  };
  if (helper.helper_family == prepare::PreparedF128RuntimeHelperFamily::Comparison) {
    if (!helper.scalar_result.has_value() || !helper.result_abi_result.has_value() ||
        !helper.scalar_result_unmarshal_move.has_value() ||
        !helper.scalar_cmp_result_consumption.has_value() ||
        helper.scalar_result->type != bir::TypeKind::I32 ||
        helper.scalar_result->width_bytes != 4 ||
        helper.result_abi_result->register_bank != prepare::PreparedRegisterBank::Gpr ||
        helper.scalar_cmp_result_consumption->cmp_type != bir::TypeKind::I32 ||
        helper.scalar_cmp_result_consumption->bir_result_type != bir::TypeKind::I1 ||
        helper.scalar_cmp_result_consumption->zero_test ==
            prepare::PreparedF128CmpResultZeroTest::Missing ||
        !helper.scalar_cmp_result_consumption->consumes_helper_cmp_result ||
        !helper.scalar_cmp_result_consumption->owns_bir_i1_result) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::IncompletePreparedF128RuntimeHelper);
    }
    record.scalar_result = F128RuntimeHelperScalarResultRecord{
        .value_id = helper.scalar_result->value_id,
        .value_name = helper.scalar_result->value_name,
        .type = helper.scalar_result->type,
        .width_bytes = helper.scalar_result->width_bytes,
        .register_bank = helper.scalar_result->register_bank,
        .home_kind = helper.scalar_result->home_kind,
        .materialized_i1_register =
            make_f128_cmp_materialized_i1_register_operand(*helper.scalar_result),
        .abi_register = make_f128_abi_register_operand(*helper.result_abi_result),
        .scalar_ownership = *helper.scalar_result,
        .marshaling_move = helper.scalar_result_unmarshal_move,
        .cmp_result_consumption = helper.scalar_cmp_result_consumption,
    };
    if (!record.scalar_result.abi_register.has_value() ||
        !record.scalar_result.materialized_i1_register.has_value()) {
      return f128_runtime_helper_record_error(
          PreparedF128RuntimeHelperRecordError::RegisterConversionFailed);
    }
  } else if (cast_helper && helper.result_type == bir::TypeKind::F128) {
    if (const auto error =
            make_f128_helper_scalar_record(
                helper.scalar_operand,
                helper.scalar_operand_abi_argument,
                helper.scalar_operand_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::ScalarToAbiArgument,
                record.scalar_operand);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.result_value_id,
                helper.result_value_name,
                helper.result_carrier,
                helper.result_abi_result,
                helper.result_unmarshal_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
                record.result);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  } else if (cast_helper && helper.source_type == bir::TypeKind::F128) {
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.operand_value_id,
                helper.operand_value_name,
                helper.lhs_carrier,
                helper.lhs_abi_argument,
                helper.lhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.lhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_scalar_record(
                helper.scalar_result,
                helper.result_abi_result,
                helper.scalar_result_unmarshal_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToScalar,
                record.scalar_result);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  } else if (const auto error =
          make_f128_helper_operand_record(
              f128_carriers,
              helper.result_value_id,
              helper.result_value_name,
              helper.result_carrier,
              helper.result_abi_result,
              helper.result_unmarshal_move,
              prepare::PreparedF128RuntimeHelperMarshalDirection::AbiResultToCarrier,
              record.result);
      error != PreparedF128RuntimeHelperRecordError::None) {
    return f128_runtime_helper_record_error(error);
  }
  if (!cast_helper) {
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.lhs_value_id,
                helper.lhs_value_name,
                helper.lhs_carrier,
                helper.lhs_abi_argument,
                helper.lhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.lhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
    if (const auto error =
            make_f128_helper_operand_record(
                f128_carriers,
                helper.rhs_value_id,
                helper.rhs_value_name,
                helper.rhs_carrier,
                helper.rhs_abi_argument,
                helper.rhs_argument_move,
                prepare::PreparedF128RuntimeHelperMarshalDirection::CarrierToAbiArgument,
                record.rhs);
        error != PreparedF128RuntimeHelperRecordError::None) {
      return f128_runtime_helper_record_error(error);
    }
  }
  return PreparedF128RuntimeHelperRecordResult{
      .record = std::move(record),
      .error = PreparedF128RuntimeHelperRecordError::None,
  };
}

PreparedAddressMaterializationRecordResult make_prepared_address_materialization_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  return make_address_record_from_prepared_materialization(
      names, value_locations, storage_plan, addressing, block_label, instruction_index);
}

PreparedAddressMaterializationInstructionRecordResult
make_prepared_address_materialization_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index) {
  const auto result = make_prepared_address_materialization_record(
      names, value_locations, storage_plan, addressing, block_label, instruction_index);
  if (!result.record.has_value()) {
    return address_materialization_instruction_record_error(result.error);
  }
  return PreparedAddressMaterializationInstructionRecordResult{
      .record = *result.record,
      .error = PreparedAddressMaterializationRecordError::None,
  };
}

}  // namespace c4c::backend::aarch64::codegen

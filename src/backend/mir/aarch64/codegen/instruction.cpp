#include "instruction.hpp"
#include "alu.hpp"
#include "cast_ops.hpp"
#include "calls.hpp"
#include "f128.hpp"
#include "memory.hpp"

#include <algorithm>

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
    case InstructionFamily::Intrinsic:
      return "intrinsic";
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
    case MachineOpcode::LogicalShiftRight:
      return "logical_shift_right";
    case MachineOpcode::Neg:
      return "neg";
    case MachineOpcode::BitNot:
      return "bit_not";
    case MachineOpcode::CountLeadingZeros:
      return "count_leading_zeros";
    case MachineOpcode::CountTrailingZeros:
      return "count_trailing_zeros";
    case MachineOpcode::ByteSwap:
      return "byte_swap";
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
    case MachineOpcode::ScalarFpUnaryIntrinsic:
      return "scalar_fp_unary_intrinsic";
    case MachineOpcode::Crc32WIntrinsic:
      return "crc32w_intrinsic";
    case MachineOpcode::VectorLoadIntrinsic:
      return "vector_load_intrinsic";
    case MachineOpcode::VectorAddIntrinsic:
      return "vector_add_intrinsic";
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
    case MachineOpcode::LogicalShiftRight:
    case MachineOpcode::Mul:
    case MachineOpcode::Div:
    case MachineOpcode::Or:
    case MachineOpcode::Xor:
    case MachineOpcode::Neg:
    case MachineOpcode::BitNot:
    case MachineOpcode::CountLeadingZeros:
    case MachineOpcode::CountTrailingZeros:
    case MachineOpcode::ByteSwap:
    case MachineOpcode::SignExtend:
    case MachineOpcode::ZeroExtend:
    case MachineOpcode::Truncate:
    case MachineOpcode::ScalarFpUnaryIntrinsic:
    case MachineOpcode::Crc32WIntrinsic:
    case MachineOpcode::VectorLoadIntrinsic:
    case MachineOpcode::VectorAddIntrinsic:
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
    case ScalarAluOperationKind::LogicalShiftRight:
      return "logical_shift_right";
    case ScalarAluOperationKind::Deferred:
      return "deferred";
  }
  return "unknown";
}

std::string_view scalar_unary_operation_kind_name(ScalarUnaryOperationKind kind) {
  switch (kind) {
    case ScalarUnaryOperationKind::Neg:
      return "neg";
    case ScalarUnaryOperationKind::BitNot:
      return "bit_not";
    case ScalarUnaryOperationKind::CountLeadingZeros:
      return "count_leading_zeros";
    case ScalarUnaryOperationKind::CountTrailingZeros:
      return "count_trailing_zeros";
    case ScalarUnaryOperationKind::ByteSwap:
      return "byte_swap";
    case ScalarUnaryOperationKind::Deferred:
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

std::string_view prepared_scalar_fp_unary_intrinsic_record_error_name(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  switch (error) {
    case PreparedScalarFpUnaryIntrinsicRecordError::None:
      return "none";
    case PreparedScalarFpUnaryIntrinsicRecordError::InvalidFunction:
      return "invalid_function";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedIntrinsicCarrier:
      return "missing_prepared_intrinsic_carrier";
    case PreparedScalarFpUnaryIntrinsicRecordError::IncompletePreparedIntrinsicCarrier:
      return "incomplete_prepared_intrinsic_carrier";
    case PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicFamily:
      return "unsupported_intrinsic_family";
    case PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicOperation:
      return "unsupported_intrinsic_operation";
    case PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandType:
      return "unsupported_operand_type";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedCallPlan:
      return "missing_prepared_call_plan";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandValueHome:
      return "missing_operand_value_home";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandStorage:
      return "missing_operand_storage";
    case PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandStorage:
      return "unsupported_operand_storage";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingResultValueHome:
      return "missing_result_value_home";
    case PreparedScalarFpUnaryIntrinsicRecordError::MissingResultStorage:
      return "missing_result_storage";
    case PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedResultStorage:
      return "unsupported_result_storage";
    case PreparedScalarFpUnaryIntrinsicRecordError::RegisterConversionFailed:
      return "register_conversion_failed";
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

namespace {

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
    case ScalarAluOperationKind::LogicalShiftRight:
      return MachineOpcode::LogicalShiftRight;
    case ScalarAluOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_scalar_unary(ScalarUnaryOperationKind operation) {
  switch (operation) {
    case ScalarUnaryOperationKind::Neg:
      return MachineOpcode::Neg;
    case ScalarUnaryOperationKind::BitNot:
      return MachineOpcode::BitNot;
    case ScalarUnaryOperationKind::CountLeadingZeros:
      return MachineOpcode::CountLeadingZeros;
    case ScalarUnaryOperationKind::CountTrailingZeros:
      return MachineOpcode::CountTrailingZeros;
    case ScalarUnaryOperationKind::ByteSwap:
      return MachineOpcode::ByteSwap;
    case ScalarUnaryOperationKind::Deferred:
      return MachineOpcode::Unspecified;
  }
  return MachineOpcode::Unspecified;
}

MachineOpcode machine_opcode_from_scalar_instruction(const ScalarInstructionRecord& instruction) {
  if (instruction.scalar_alu.has_value()) {
    return machine_opcode_from_scalar_alu(instruction.scalar_alu->operation);
  }
  if (instruction.scalar_unary.has_value()) {
    return machine_opcode_from_scalar_unary(instruction.scalar_unary->operation);
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
  const bool valid_condition_type =
      condition.form == BranchConditionForm::FusedCompare
          ? condition.condition_type != bir::TypeKind::Void
          : condition.condition_type == bir::TypeKind::I1;
  if (!condition.condition_value_id.has_value() ||
      condition.condition_value_name == c4c::kInvalidValueName ||
      !valid_condition_type) {
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
  if (instruction.scalar_unary.has_value()) {
    if (instruction.scalar_unary->supported_integer_operation &&
        instruction.scalar_unary->operation != ScalarUnaryOperationKind::Deferred) {
      return MachineNodeStatusRecord{.status = MachineNodeSelectionStatus::Selected};
    }
    return MachineNodeStatusRecord{
        .status = MachineNodeSelectionStatus::DeferredUnsupported,
        .diagnostic = "scalar unary operation is outside the selected subset"};
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

}  // namespace c4c::backend::aarch64::codegen

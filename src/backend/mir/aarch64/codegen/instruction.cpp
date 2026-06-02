#include "instruction.hpp"
#include "alu.hpp"
#include "cast_ops.hpp"
#include "effects.hpp"
#include "f128.hpp"
#include "memory.hpp"

#include <algorithm>
#include <array>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

namespace {

struct MachinePrinterMnemonicSpelling {
  MachinePrinterMnemonicKind kind;
  std::string_view spelling;
};

struct MachineOpcodePrinterMnemonic {
  MachineOpcode opcode;
  MachinePrinterMnemonicKind kind;
};

struct MachineOpcodeSpelling {
  MachineOpcode opcode;
  std::string_view spelling;
};

struct MachineNodeSelectionStatusSpelling {
  MachineNodeSelectionStatus status;
  std::string_view spelling;
};

struct OperandKindSpelling {
  OperandKind kind;
  std::string_view spelling;
};

struct RecordSurfaceKindSpelling {
  RecordSurfaceKind surface;
  std::string_view spelling;
};

struct RegisterOperandRoleSpelling {
  RegisterOperandRole role;
  std::string_view spelling;
};

struct ImmediateKindSpelling {
  ImmediateKind kind;
  std::string_view spelling;
};

struct InstructionFamilySpelling {
  InstructionFamily family;
  std::string_view spelling;
};

struct FrameInstructionKindSpelling {
  FrameInstructionKind kind;
  std::string_view spelling;
};

struct ScalarAluOperationKindSpelling {
  ScalarAluOperationKind kind;
  std::string_view spelling;
};

struct ScalarUnaryOperationKindSpelling {
  ScalarUnaryOperationKind kind;
  std::string_view spelling;
};

struct ScalarCastOperationKindSpelling {
  ScalarCastOperationKind kind;
  std::string_view spelling;
};

struct BranchConditionFormSpelling {
  BranchConditionForm form;
  std::string_view spelling;
};

struct BranchCompareCandidateKindSpelling {
  BranchCompareCandidateKind kind;
  std::string_view spelling;
};

struct PreparedBranchRecordErrorSpelling {
  PreparedBranchRecordError error;
  std::string_view spelling;
};

struct PreparedScalarAluRecordErrorSpelling {
  PreparedScalarAluRecordError error;
  std::string_view spelling;
};

struct PreparedScalarCastRecordErrorSpelling {
  PreparedScalarCastRecordError error;
  std::string_view spelling;
};

struct PreparedScalarFpUnaryIntrinsicRecordErrorSpelling {
  PreparedScalarFpUnaryIntrinsicRecordError error;
  std::string_view spelling;
};

struct AddressMaterializationKindSpelling {
  AddressMaterializationKind kind;
  std::string_view spelling;
};

struct PreparedAddressMaterializationRecordErrorSpelling {
  PreparedAddressMaterializationRecordError error;
  std::string_view spelling;
};

struct MachinePseudoKindSpelling {
  MachinePseudoKind pseudo;
  std::string_view spelling;
};

struct MachinePseudoPrinterMnemonic {
  MachinePseudoKind pseudo;
  MachinePrinterMnemonicKind kind;
};

struct AggregateWidthMnemonic {
  std::size_t width_bytes;
  std::string_view mnemonic;
};

constexpr std::array<MachinePrinterMnemonicSpelling, 17> kMachinePrinterMnemonicSpellings{{
    {MachinePrinterMnemonicKind::None, ""},
    {MachinePrinterMnemonicKind::Branch, "b"},
    {MachinePrinterMnemonicKind::ConditionalBranchNonZero, "cbnz"},
    {MachinePrinterMnemonicKind::DirectCall, "bl"},
    {MachinePrinterMnemonicKind::IndirectCall, "blr"},
    {MachinePrinterMnemonicKind::Add, "add"},
    {MachinePrinterMnemonicKind::Sub, "sub"},
    {MachinePrinterMnemonicKind::Load, "ldr"},
    {MachinePrinterMnemonicKind::LoadByte, "ldrb"},
    {MachinePrinterMnemonicKind::Store, "str"},
    {MachinePrinterMnemonicKind::StoreByte, "strb"},
    {MachinePrinterMnemonicKind::Move, "mov"},
    {MachinePrinterMnemonicKind::Return, "ret"},
    {MachinePrinterMnemonicKind::VariadicVaStart, "va.start"},
    {MachinePrinterMnemonicKind::VariadicVaArgScalar, "va.arg.scalar"},
    {MachinePrinterMnemonicKind::VariadicVaArgAggregate, "va.arg.aggregate"},
    {MachinePrinterMnemonicKind::VariadicVaCopy, "va.copy"},
}};

constexpr std::array<MachineOpcodePrinterMnemonic, 17> kMachineOpcodePrinterMnemonics{{
    {MachineOpcode::Branch, MachinePrinterMnemonicKind::Branch},
    {MachineOpcode::ConditionalBranch,
     MachinePrinterMnemonicKind::ConditionalBranchNonZero},
    {MachineOpcode::DirectCall, MachinePrinterMnemonicKind::DirectCall},
    {MachineOpcode::IndirectCall, MachinePrinterMnemonicKind::IndirectCall},
    {MachineOpcode::FrameSetup, MachinePrinterMnemonicKind::Sub},
    {MachineOpcode::FrameTeardown, MachinePrinterMnemonicKind::Add},
    {MachineOpcode::Add, MachinePrinterMnemonicKind::Add},
    {MachineOpcode::Sub, MachinePrinterMnemonicKind::Sub},
    {MachineOpcode::Load, MachinePrinterMnemonicKind::Load},
    {MachineOpcode::ReloadFromSlot, MachinePrinterMnemonicKind::Load},
    {MachineOpcode::Store, MachinePrinterMnemonicKind::Store},
    {MachineOpcode::SpillToSlot, MachinePrinterMnemonicKind::Store},
    {MachineOpcode::CallBoundaryMove, MachinePrinterMnemonicKind::Move},
    {MachineOpcode::VariadicVaStart, MachinePrinterMnemonicKind::VariadicVaStart},
    {MachineOpcode::VariadicVaArgScalar,
     MachinePrinterMnemonicKind::VariadicVaArgScalar},
    {MachineOpcode::VariadicVaArgAggregate,
     MachinePrinterMnemonicKind::VariadicVaArgAggregate},
    {MachineOpcode::VariadicVaCopy, MachinePrinterMnemonicKind::VariadicVaCopy},
}};

constexpr std::array<MachineOpcodeSpelling, 53> kMachineOpcodeSpellings{{
    {MachineOpcode::Unspecified, "unspecified"},
    {MachineOpcode::Branch, "branch"},
    {MachineOpcode::ConditionalBranch, "conditional_branch"},
    {MachineOpcode::CompareBranch, "compare_branch"},
    {MachineOpcode::AddressMaterialization, "address_materialization"},
    {MachineOpcode::DirectCall, "direct_call"},
    {MachineOpcode::IndirectCall, "indirect_call"},
    {MachineOpcode::FrameSetup, "frame_setup"},
    {MachineOpcode::FrameTeardown, "frame_teardown"},
    {MachineOpcode::CalleeSaveStore, "callee_save_store"},
    {MachineOpcode::CalleeSaveLoad, "callee_save_load"},
    {MachineOpcode::CallBoundaryMove, "call_boundary_move"},
    {MachineOpcode::CallBoundaryAbiBinding, "call_boundary_abi_binding"},
    {MachineOpcode::I128Transport, "i128_transport"},
    {MachineOpcode::F128Transport, "f128_transport"},
    {MachineOpcode::F128RuntimeHelper, "f128_runtime_helper"},
    {MachineOpcode::I128Pair, "i128_pair"},
    {MachineOpcode::I128Shift, "i128_shift"},
    {MachineOpcode::I128Compare, "i128_compare"},
    {MachineOpcode::I128RuntimeHelper, "i128_runtime_helper"},
    {MachineOpcode::Add, "add"},
    {MachineOpcode::Sub, "sub"},
    {MachineOpcode::Mul, "mul"},
    {MachineOpcode::Div, "div"},
    {MachineOpcode::And, "and"},
    {MachineOpcode::Or, "or"},
    {MachineOpcode::Xor, "xor"},
    {MachineOpcode::LogicalShiftRight, "logical_shift_right"},
    {MachineOpcode::Neg, "neg"},
    {MachineOpcode::BitNot, "bit_not"},
    {MachineOpcode::CountLeadingZeros, "count_leading_zeros"},
    {MachineOpcode::CountTrailingZeros, "count_trailing_zeros"},
    {MachineOpcode::ByteSwap, "byte_swap"},
    {MachineOpcode::SignExtend, "sign_extend"},
    {MachineOpcode::ZeroExtend, "zero_extend"},
    {MachineOpcode::Truncate, "truncate"},
    {MachineOpcode::Load, "load"},
    {MachineOpcode::Store, "store"},
    {MachineOpcode::AtomicLoad, "atomic_load"},
    {MachineOpcode::AtomicStore, "atomic_store"},
    {MachineOpcode::AtomicFence, "atomic_fence"},
    {MachineOpcode::AtomicRmw, "atomic_rmw"},
    {MachineOpcode::AtomicCompareExchange, "atomic_compare_exchange"},
    {MachineOpcode::SpillToSlot, "spill_to_slot"},
    {MachineOpcode::ReloadFromSlot, "reload_from_slot"},
    {MachineOpcode::VariadicVaStart, "variadic_va_start"},
    {MachineOpcode::VariadicVaArgScalar, "variadic_va_arg_scalar"},
    {MachineOpcode::VariadicVaArgAggregate, "variadic_va_arg_aggregate"},
    {MachineOpcode::VariadicVaCopy, "variadic_va_copy"},
    {MachineOpcode::ScalarFpUnaryIntrinsic, "scalar_fp_unary_intrinsic"},
    {MachineOpcode::Crc32WIntrinsic, "crc32w_intrinsic"},
    {MachineOpcode::VectorLoadIntrinsic, "vector_load_intrinsic"},
    {MachineOpcode::VectorAddIntrinsic, "vector_add_intrinsic"},
}};

constexpr std::array<MachineNodeSelectionStatusSpelling, 3>
    kMachineNodeSelectionStatusSpellings{{
        {MachineNodeSelectionStatus::Selected, "selected"},
        {MachineNodeSelectionStatus::DeferredUnsupported, "deferred_unsupported"},
        {MachineNodeSelectionStatus::MissingRequiredFacts, "missing_required_facts"},
    }};

constexpr std::array<OperandKindSpelling, 7> kOperandKindSpellings{{
    {OperandKind::Register, "register"},
    {OperandKind::Immediate, "immediate"},
    {OperandKind::PreparedValue, "prepared_value"},
    {OperandKind::FrameSlot, "frame_slot"},
    {OperandKind::Symbol, "symbol"},
    {OperandKind::BranchTarget, "branch_target"},
    {OperandKind::Memory, "memory"},
}};

constexpr std::array<RecordSurfaceKindSpelling, 5> kRecordSurfaceKindSpellings{{
    {RecordSurfaceKind::TargetMirRecord, "target_mir_record"},
    {RecordSurfaceKind::MachineInstructionNode, "machine_instruction_node"},
    {RecordSurfaceKind::PrinterOutput, "printer_output"},
    {RecordSurfaceKind::EncoderInput, "encoder_input"},
    {RecordSurfaceKind::ExternalAssemblerInput, "external_assembler_input"},
}};

constexpr std::array<RegisterOperandRoleSpelling, 8>
    kRegisterOperandRoleSpellings{{
        {RegisterOperandRole::Physical, "physical"},
        {RegisterOperandRole::PreparedAssignment, "prepared_assignment"},
        {RegisterOperandRole::AllocationResult, "allocation_result"},
        {RegisterOperandRole::ReservedMirScratch, "reserved_mir_scratch"},
        {RegisterOperandRole::ValueHome, "value_home"},
        {RegisterOperandRole::SpillAuthority, "spill_authority"},
        {RegisterOperandRole::StoragePlan, "storage_plan"},
        {RegisterOperandRole::CallAbi, "call_abi"},
    }};

constexpr std::array<ImmediateKindSpelling, 4> kImmediateKindSpellings{{
    {ImmediateKind::SignedInteger, "signed_integer"},
    {ImmediateKind::UnsignedInteger, "unsigned_integer"},
    {ImmediateKind::Boolean, "boolean"},
    {ImmediateKind::NullPointer, "null_pointer"},
}};

constexpr std::array<InstructionFamilySpelling, 13> kInstructionFamilySpellings{{
    {InstructionFamily::Branch, "branch"},
    {InstructionFamily::Scalar, "scalar"},
    {InstructionFamily::I128Transport, "i128_transport"},
    {InstructionFamily::F128Transport, "f128_transport"},
    {InstructionFamily::I128Pair, "i128_pair"},
    {InstructionFamily::Memory, "memory"},
    {InstructionFamily::Frame, "frame"},
    {InstructionFamily::Call, "call"},
    {InstructionFamily::CallBoundary, "call_boundary"},
    {InstructionFamily::Intrinsic, "intrinsic"},
    {InstructionFamily::Return, "return"},
    {InstructionFamily::Assembler, "assembler"},
    {InstructionFamily::Object, "object"},
}};

constexpr std::array<FrameInstructionKindSpelling, 4> kFrameInstructionKindSpellings{{
    {FrameInstructionKind::PrologueSetup, "prologue_setup"},
    {FrameInstructionKind::EpilogueTeardown, "epilogue_teardown"},
    {FrameInstructionKind::CalleeSaveStore, "callee_save_store"},
    {FrameInstructionKind::CalleeSaveLoad, "callee_save_load"},
}};

constexpr std::array<ScalarAluOperationKindSpelling, 9>
    kScalarAluOperationKindSpellings{{
        {ScalarAluOperationKind::Add, "add"},
        {ScalarAluOperationKind::Sub, "sub"},
        {ScalarAluOperationKind::Mul, "mul"},
        {ScalarAluOperationKind::Div, "div"},
        {ScalarAluOperationKind::And, "and"},
        {ScalarAluOperationKind::Or, "or"},
        {ScalarAluOperationKind::Xor, "xor"},
        {ScalarAluOperationKind::LogicalShiftRight, "logical_shift_right"},
        {ScalarAluOperationKind::Deferred, "deferred"},
    }};

constexpr std::array<ScalarUnaryOperationKindSpelling, 6>
    kScalarUnaryOperationKindSpellings{{
        {ScalarUnaryOperationKind::Neg, "neg"},
        {ScalarUnaryOperationKind::BitNot, "bit_not"},
        {ScalarUnaryOperationKind::CountLeadingZeros, "count_leading_zeros"},
        {ScalarUnaryOperationKind::CountTrailingZeros, "count_trailing_zeros"},
        {ScalarUnaryOperationKind::ByteSwap, "byte_swap"},
        {ScalarUnaryOperationKind::Deferred, "deferred"},
    }};

constexpr std::array<ScalarCastOperationKindSpelling, 10>
    kScalarCastOperationKindSpellings{{
        {ScalarCastOperationKind::SignExtend, "sign_extend"},
        {ScalarCastOperationKind::ZeroExtend, "zero_extend"},
        {ScalarCastOperationKind::Truncate, "truncate"},
        {ScalarCastOperationKind::FloatExtend, "float_extend"},
        {ScalarCastOperationKind::FloatTruncate, "float_truncate"},
        {ScalarCastOperationKind::SignedIntToFloat, "signed_int_to_float"},
        {ScalarCastOperationKind::UnsignedIntToFloat, "unsigned_int_to_float"},
        {ScalarCastOperationKind::FloatToSignedInt, "float_to_signed_int"},
        {ScalarCastOperationKind::FloatToUnsignedInt, "float_to_unsigned_int"},
        {ScalarCastOperationKind::Deferred, "deferred"},
    }};

constexpr std::array<BranchConditionFormSpelling, 3> kBranchConditionFormSpellings{{
    {BranchConditionForm::Unconditional, "unconditional"},
    {BranchConditionForm::MaterializedBool, "materialized_bool"},
    {BranchConditionForm::FusedCompare, "fused_compare"},
}};

constexpr std::array<BranchCompareCandidateKindSpelling, 4>
    kBranchCompareCandidateKindSpellings{{
        {BranchCompareCandidateKind::None, "none"},
        {BranchCompareCandidateKind::MaterializedBoolCondition,
         "materialized_bool_condition"},
        {BranchCompareCandidateKind::FusedCompareAndBranch,
         "fused_compare_and_branch"},
        {BranchCompareCandidateKind::NonFusableCompare, "non_fusable_compare"},
    }};

constexpr std::array<PreparedBranchRecordErrorSpelling, 12>
    kPreparedBranchRecordErrorSpellings{{
        {PreparedBranchRecordError::None, "none"},
        {PreparedBranchRecordError::InvalidFunction, "invalid_function"},
        {PreparedBranchRecordError::InvalidSourceBlock, "invalid_source_block"},
        {PreparedBranchRecordError::TerminatorKindMismatch,
         "terminator_kind_mismatch"},
        {PreparedBranchRecordError::MissingBranchTarget,
         "missing_branch_target"},
        {PreparedBranchRecordError::TerminatorTargetMismatch,
         "terminator_target_mismatch"},
        {PreparedBranchRecordError::MissingBranchCondition,
         "missing_branch_condition"},
        {PreparedBranchRecordError::ConditionValueMismatch,
         "condition_value_mismatch"},
        {PreparedBranchRecordError::MissingConditionValueHome,
         "missing_condition_value_home"},
        {PreparedBranchRecordError::MissingCompareFacts, "missing_compare_facts"},
        {PreparedBranchRecordError::UnsupportedComparePredicate,
         "unsupported_compare_predicate"},
        {PreparedBranchRecordError::MissingCompareValueHome,
         "missing_compare_value_home"},
    }};

constexpr std::array<PreparedScalarAluRecordErrorSpelling, 13>
    kPreparedScalarAluRecordErrorSpellings{{
        {PreparedScalarAluRecordError::None, "none"},
        {PreparedScalarAluRecordError::InvalidFunction, "invalid_function"},
        {PreparedScalarAluRecordError::UnsupportedOpcode, "unsupported_opcode"},
        {PreparedScalarAluRecordError::UnsupportedResultValue,
         "unsupported_result_value"},
        {PreparedScalarAluRecordError::MissingResultValueHome,
         "missing_result_value_home"},
        {PreparedScalarAluRecordError::MissingResultStorage,
         "missing_result_storage"},
        {PreparedScalarAluRecordError::UnsupportedResultStorage,
         "unsupported_result_storage"},
        {PreparedScalarAluRecordError::UnsupportedOperandValue,
         "unsupported_operand_value"},
        {PreparedScalarAluRecordError::MissingOperandValueHome,
         "missing_operand_value_home"},
        {PreparedScalarAluRecordError::MissingOperandStorage,
         "missing_operand_storage"},
        {PreparedScalarAluRecordError::UnsupportedOperandStorage,
         "unsupported_operand_storage"},
        {PreparedScalarAluRecordError::UnsupportedOperandType,
         "unsupported_operand_type"},
        {PreparedScalarAluRecordError::RegisterConversionFailed,
         "register_conversion_failed"},
    }};

constexpr std::array<PreparedScalarCastRecordErrorSpelling, 13>
    kPreparedScalarCastRecordErrorSpellings{{
        {PreparedScalarCastRecordError::None, "none"},
        {PreparedScalarCastRecordError::InvalidFunction, "invalid_function"},
        {PreparedScalarCastRecordError::UnsupportedOpcode, "unsupported_opcode"},
        {PreparedScalarCastRecordError::UnsupportedResultValue,
         "unsupported_result_value"},
        {PreparedScalarCastRecordError::MissingResultValueHome,
         "missing_result_value_home"},
        {PreparedScalarCastRecordError::MissingResultStorage,
         "missing_result_storage"},
        {PreparedScalarCastRecordError::UnsupportedResultStorage,
         "unsupported_result_storage"},
        {PreparedScalarCastRecordError::UnsupportedOperandValue,
         "unsupported_operand_value"},
        {PreparedScalarCastRecordError::MissingOperandValueHome,
         "missing_operand_value_home"},
        {PreparedScalarCastRecordError::MissingOperandStorage,
         "missing_operand_storage"},
        {PreparedScalarCastRecordError::UnsupportedOperandStorage,
         "unsupported_operand_storage"},
        {PreparedScalarCastRecordError::UnsupportedOperandType,
         "unsupported_operand_type"},
        {PreparedScalarCastRecordError::RegisterConversionFailed,
         "register_conversion_failed"},
    }};

constexpr std::array<PreparedScalarFpUnaryIntrinsicRecordErrorSpelling, 15>
    kPreparedScalarFpUnaryIntrinsicRecordErrorSpellings{{
        {PreparedScalarFpUnaryIntrinsicRecordError::None, "none"},
        {PreparedScalarFpUnaryIntrinsicRecordError::InvalidFunction,
         "invalid_function"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedIntrinsicCarrier,
         "missing_prepared_intrinsic_carrier"},
        {PreparedScalarFpUnaryIntrinsicRecordError::IncompletePreparedIntrinsicCarrier,
         "incomplete_prepared_intrinsic_carrier"},
        {PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicFamily,
         "unsupported_intrinsic_family"},
        {PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedIntrinsicOperation,
         "unsupported_intrinsic_operation"},
        {PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandType,
         "unsupported_operand_type"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingPreparedCallPlan,
         "missing_prepared_call_plan"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandValueHome,
         "missing_operand_value_home"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingOperandStorage,
         "missing_operand_storage"},
        {PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedOperandStorage,
         "unsupported_operand_storage"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingResultValueHome,
         "missing_result_value_home"},
        {PreparedScalarFpUnaryIntrinsicRecordError::MissingResultStorage,
         "missing_result_storage"},
        {PreparedScalarFpUnaryIntrinsicRecordError::UnsupportedResultStorage,
         "unsupported_result_storage"},
        {PreparedScalarFpUnaryIntrinsicRecordError::RegisterConversionFailed,
         "register_conversion_failed"},
    }};

constexpr std::array<AddressMaterializationKindSpelling, 7>
    kAddressMaterializationKindSpellings{{
        {AddressMaterializationKind::FrameSlot, "frame_slot"},
        {AddressMaterializationKind::DirectPageLow12, "direct_page_low12"},
        {AddressMaterializationKind::GotPageLow12, "got_page_low12"},
        {AddressMaterializationKind::TlsRelative, "tls_relative"},
        {AddressMaterializationKind::StringConstant, "string_constant"},
        {AddressMaterializationKind::LabelPageLow12, "label_page_low12"},
        {AddressMaterializationKind::DeferredUnsupported, "deferred_unsupported"},
    }};

constexpr std::array<PreparedAddressMaterializationRecordErrorSpelling, 17>
    kPreparedAddressMaterializationRecordErrorSpellings{{
        {PreparedAddressMaterializationRecordError::None, "none"},
        {PreparedAddressMaterializationRecordError::InvalidFunction,
         "invalid_function"},
        {PreparedAddressMaterializationRecordError::MissingPreparedAddressMaterialization,
         "missing_prepared_address_materialization"},
        {PreparedAddressMaterializationRecordError::UnsupportedAddressKind,
         "unsupported_address_kind"},
        {PreparedAddressMaterializationRecordError::MissingResultValueName,
         "missing_result_value_name"},
        {PreparedAddressMaterializationRecordError::MissingResultValueHome,
         "missing_result_value_home"},
        {PreparedAddressMaterializationRecordError::MissingResultStorage,
         "missing_result_storage"},
        {PreparedAddressMaterializationRecordError::UnsupportedResultStorage,
         "unsupported_result_storage"},
        {PreparedAddressMaterializationRecordError::RegisterConversionFailed,
         "register_conversion_failed"},
        {PreparedAddressMaterializationRecordError::MissingFrameSlotId,
         "missing_frame_slot_id"},
        {PreparedAddressMaterializationRecordError::MissingSymbolIdentity,
         "missing_symbol_identity"},
        {PreparedAddressMaterializationRecordError::MissingStringIdentity,
         "missing_string_identity"},
        {PreparedAddressMaterializationRecordError::MissingLabelIdentity,
         "missing_label_identity"},
        {PreparedAddressMaterializationRecordError::MissingAddressMaterializationPolicy,
         "missing_address_materialization_policy"},
        {PreparedAddressMaterializationRecordError::AddressMaterializationPolicyMismatch,
         "address_materialization_policy_mismatch"},
        {PreparedAddressMaterializationRecordError::MissingTlsMaterializationFacts,
         "missing_tls_materialization_facts"},
        {PreparedAddressMaterializationRecordError::TlsFactMismatch,
         "tls_fact_mismatch"},
    }};

constexpr std::array<MachinePseudoKindSpelling, 3> kMachinePseudoKindSpellings{{
    {MachinePseudoKind::None, "none"},
    {MachinePseudoKind::SpillToSlot, "spill_to_slot"},
    {MachinePseudoKind::ReloadFromSlot, "reload_from_slot"},
}};

constexpr std::array<MachinePseudoPrinterMnemonic, 2> kMachinePseudoPrinterMnemonics{{
    {MachinePseudoKind::SpillToSlot, MachinePrinterMnemonicKind::Store},
    {MachinePseudoKind::ReloadFromSlot, MachinePrinterMnemonicKind::Load},
}};

constexpr std::array<AggregateWidthMnemonic, 3> kAggregateStackCopyLoadMnemonics{{
    {1, "ldrb"},
    {4, "ldr"},
    {8, "ldr"},
}};

constexpr std::array<AggregateWidthMnemonic, 3> kAggregateStackCopyStoreMnemonics{{
    {1, "strb"},
    {4, "str"},
    {8, "str"},
}};

constexpr std::array<AggregateWidthMnemonic, 4> kAggregateRegisterLaneLoadMnemonics{{
    {1, "ldrb"},
    {2, "ldrh"},
    {4, "ldr"},
    {8, "ldr"},
}};

template <std::size_t N>
[[nodiscard]] std::string_view aggregate_width_mnemonic(
    const std::array<AggregateWidthMnemonic, N>& mnemonics,
    std::size_t width_bytes) {
  const auto found = std::find_if(
      mnemonics.begin(), mnemonics.end(),
      [width_bytes](const AggregateWidthMnemonic& mnemonic) {
        return mnemonic.width_bytes == width_bytes;
      });
  if (found == mnemonics.end()) {
    return {};
  }
  return found->mnemonic;
}

[[nodiscard]] std::string_view machine_printer_mnemonic_spelling(
    MachinePrinterMnemonicKind kind) {
  const auto found = std::find_if(
      kMachinePrinterMnemonicSpellings.begin(), kMachinePrinterMnemonicSpellings.end(),
      [kind](const MachinePrinterMnemonicSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kMachinePrinterMnemonicSpellings.end()) {
    return "";
  }
  return found->spelling;
}

[[nodiscard]] MachinePrinterMnemonicKind machine_opcode_printer_mnemonic(
    MachineOpcode opcode) {
  const auto found = std::find_if(
      kMachineOpcodePrinterMnemonics.begin(), kMachineOpcodePrinterMnemonics.end(),
      [opcode](const MachineOpcodePrinterMnemonic& mnemonic) {
        return mnemonic.opcode == opcode;
      });
  if (found == kMachineOpcodePrinterMnemonics.end()) {
    return MachinePrinterMnemonicKind::None;
  }
  return found->kind;
}

[[nodiscard]] std::string_view machine_opcode_spelling(MachineOpcode opcode) {
  const auto found = std::find_if(
      kMachineOpcodeSpellings.begin(), kMachineOpcodeSpellings.end(),
      [opcode](const MachineOpcodeSpelling& spelling) {
        return spelling.opcode == opcode;
      });
  if (found == kMachineOpcodeSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view operand_kind_spelling(OperandKind kind) {
  const auto found = std::find_if(
      kOperandKindSpellings.begin(), kOperandKindSpellings.end(),
      [kind](const OperandKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kOperandKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] MachinePrinterMnemonicKind machine_pseudo_printer_mnemonic(
    MachinePseudoKind pseudo) {
  const auto found = std::find_if(
      kMachinePseudoPrinterMnemonics.begin(), kMachinePseudoPrinterMnemonics.end(),
      [pseudo](const MachinePseudoPrinterMnemonic& mnemonic) {
        return mnemonic.pseudo == pseudo;
      });
  if (found == kMachinePseudoPrinterMnemonics.end()) {
    return MachinePrinterMnemonicKind::None;
  }
  return found->kind;
}

[[nodiscard]] std::string_view machine_pseudo_kind_spelling(MachinePseudoKind pseudo) {
  const auto found = std::find_if(
      kMachinePseudoKindSpellings.begin(), kMachinePseudoKindSpellings.end(),
      [pseudo](const MachinePseudoKindSpelling& spelling) {
        return spelling.pseudo == pseudo;
      });
  if (found == kMachinePseudoKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view record_surface_kind_spelling(RecordSurfaceKind surface) {
  const auto found = std::find_if(
      kRecordSurfaceKindSpellings.begin(), kRecordSurfaceKindSpellings.end(),
      [surface](const RecordSurfaceKindSpelling& spelling) {
        return spelling.surface == surface;
      });
  if (found == kRecordSurfaceKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view register_operand_role_spelling(
    RegisterOperandRole role) {
  const auto found = std::find_if(
      kRegisterOperandRoleSpellings.begin(), kRegisterOperandRoleSpellings.end(),
      [role](const RegisterOperandRoleSpelling& spelling) {
        return spelling.role == role;
      });
  if (found == kRegisterOperandRoleSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view immediate_kind_spelling(ImmediateKind kind) {
  const auto found = std::find_if(
      kImmediateKindSpellings.begin(), kImmediateKindSpellings.end(),
      [kind](const ImmediateKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kImmediateKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view instruction_family_spelling(
    InstructionFamily family) {
  const auto found = std::find_if(
      kInstructionFamilySpellings.begin(), kInstructionFamilySpellings.end(),
      [family](const InstructionFamilySpelling& spelling) {
        return spelling.family == family;
      });
  if (found == kInstructionFamilySpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view frame_instruction_kind_spelling(
    FrameInstructionKind kind) {
  const auto found = std::find_if(
      kFrameInstructionKindSpellings.begin(), kFrameInstructionKindSpellings.end(),
      [kind](const FrameInstructionKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kFrameInstructionKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view scalar_alu_operation_kind_spelling(
    ScalarAluOperationKind kind) {
  const auto found = std::find_if(
      kScalarAluOperationKindSpellings.begin(),
      kScalarAluOperationKindSpellings.end(),
      [kind](const ScalarAluOperationKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kScalarAluOperationKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view scalar_unary_operation_kind_spelling(
    ScalarUnaryOperationKind kind) {
  const auto found = std::find_if(
      kScalarUnaryOperationKindSpellings.begin(),
      kScalarUnaryOperationKindSpellings.end(),
      [kind](const ScalarUnaryOperationKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kScalarUnaryOperationKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view scalar_cast_operation_kind_spelling(
    ScalarCastOperationKind kind) {
  const auto found = std::find_if(
      kScalarCastOperationKindSpellings.begin(),
      kScalarCastOperationKindSpellings.end(),
      [kind](const ScalarCastOperationKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kScalarCastOperationKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view branch_condition_form_spelling(
    BranchConditionForm form) {
  const auto found = std::find_if(
      kBranchConditionFormSpellings.begin(), kBranchConditionFormSpellings.end(),
      [form](const BranchConditionFormSpelling& spelling) {
        return spelling.form == form;
      });
  if (found == kBranchConditionFormSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view branch_compare_candidate_kind_spelling(
    BranchCompareCandidateKind kind) {
  const auto found = std::find_if(
      kBranchCompareCandidateKindSpellings.begin(),
      kBranchCompareCandidateKindSpellings.end(),
      [kind](const BranchCompareCandidateKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kBranchCompareCandidateKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view prepared_branch_record_error_spelling(
    PreparedBranchRecordError error) {
  const auto found = std::find_if(
      kPreparedBranchRecordErrorSpellings.begin(),
      kPreparedBranchRecordErrorSpellings.end(),
      [error](const PreparedBranchRecordErrorSpelling& spelling) {
        return spelling.error == error;
      });
  if (found == kPreparedBranchRecordErrorSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view prepared_scalar_alu_record_error_spelling(
    PreparedScalarAluRecordError error) {
  const auto found = std::find_if(
      kPreparedScalarAluRecordErrorSpellings.begin(),
      kPreparedScalarAluRecordErrorSpellings.end(),
      [error](const PreparedScalarAluRecordErrorSpelling& spelling) {
        return spelling.error == error;
      });
  if (found == kPreparedScalarAluRecordErrorSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view prepared_scalar_cast_record_error_spelling(
    PreparedScalarCastRecordError error) {
  const auto found = std::find_if(
      kPreparedScalarCastRecordErrorSpellings.begin(),
      kPreparedScalarCastRecordErrorSpellings.end(),
      [error](const PreparedScalarCastRecordErrorSpelling& spelling) {
        return spelling.error == error;
      });
  if (found == kPreparedScalarCastRecordErrorSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view prepared_scalar_fp_unary_intrinsic_record_error_spelling(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  const auto found = std::find_if(
      kPreparedScalarFpUnaryIntrinsicRecordErrorSpellings.begin(),
      kPreparedScalarFpUnaryIntrinsicRecordErrorSpellings.end(),
      [error](const PreparedScalarFpUnaryIntrinsicRecordErrorSpelling& spelling) {
        return spelling.error == error;
      });
  if (found == kPreparedScalarFpUnaryIntrinsicRecordErrorSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view address_materialization_kind_spelling(
    AddressMaterializationKind kind) {
  const auto found = std::find_if(
      kAddressMaterializationKindSpellings.begin(),
      kAddressMaterializationKindSpellings.end(),
      [kind](const AddressMaterializationKindSpelling& spelling) {
        return spelling.kind == kind;
      });
  if (found == kAddressMaterializationKindSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view prepared_address_materialization_record_error_spelling(
    PreparedAddressMaterializationRecordError error) {
  const auto found = std::find_if(
      kPreparedAddressMaterializationRecordErrorSpellings.begin(),
      kPreparedAddressMaterializationRecordErrorSpellings.end(),
      [error](const PreparedAddressMaterializationRecordErrorSpelling& spelling) {
        return spelling.error == error;
      });
  if (found == kPreparedAddressMaterializationRecordErrorSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] std::string_view machine_node_selection_status_spelling(
    MachineNodeSelectionStatus status) {
  const auto found = std::find_if(
      kMachineNodeSelectionStatusSpellings.begin(),
      kMachineNodeSelectionStatusSpellings.end(),
      [status](const MachineNodeSelectionStatusSpelling& spelling) {
        return spelling.status == status;
      });
  if (found == kMachineNodeSelectionStatusSpellings.end()) {
    return "unknown";
  }
  return found->spelling;
}

[[nodiscard]] bool same_aggregate_gp_register_index(abi::RegisterReference lhs,
                                                    abi::RegisterReference rhs) {
  return lhs.index == rhs.index && abi::is_gp_register(lhs) && abi::is_gp_register(rhs);
}

[[nodiscard]] bool aggregate_frame_slot_direct_offset_is_encodable(
    const MemoryOperand& memory,
    std::size_t load_width_bytes) {
  if (memory.base_kind != MemoryBaseKind::FrameSlot || memory.byte_offset < 0 ||
      load_width_bytes == 0) {
    return false;
  }
  if (load_width_bytes != 1 && load_width_bytes != 2 && load_width_bytes != 4 &&
      load_width_bytes != 8 && load_width_bytes != 16) {
    return false;
  }
  const auto offset = static_cast<std::uint64_t>(memory.byte_offset);
  return offset % load_width_bytes == 0 && offset / load_width_bytes <= 4095U;
}

}  // namespace

std::string_view operand_kind_name(OperandKind kind) {
  return operand_kind_spelling(kind);
}

std::string_view record_surface_kind_name(RecordSurfaceKind kind) {
  return record_surface_kind_spelling(kind);
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
  return register_operand_role_spelling(role);
}

std::string_view immediate_kind_name(ImmediateKind kind) {
  return immediate_kind_spelling(kind);
}

std::string_view instruction_family_name(InstructionFamily family) {
  return instruction_family_spelling(family);
}

std::string_view machine_opcode_name(MachineOpcode opcode) {
  return machine_opcode_spelling(opcode);
}

std::string_view machine_pseudo_kind_name(MachinePseudoKind pseudo) {
  return machine_pseudo_kind_spelling(pseudo);
}

std::string_view machine_printer_mnemonic_kind_name(MachinePrinterMnemonicKind kind) {
  return machine_printer_mnemonic_spelling(kind);
}

MachinePrinterMnemonicKind machine_opcode_printer_mnemonic_kind(MachineOpcode opcode) {
  return machine_opcode_printer_mnemonic(opcode);
}

MachinePrinterMnemonicKind machine_pseudo_printer_mnemonic_kind(MachinePseudoKind pseudo) {
  return machine_pseudo_printer_mnemonic(pseudo);
}

MachinePrinterMnemonicKind machine_instruction_primary_printer_mnemonic_kind(
    const InstructionRecord& instruction) {
  if (std::get_if<ReturnInstructionRecord>(&instruction.payload) != nullptr) {
    return MachinePrinterMnemonicKind::Return;
  }
  if (std::get_if<SpillReloadInstructionRecord>(&instruction.payload) != nullptr) {
    return machine_pseudo_printer_mnemonic_kind(instruction.pseudo);
  }
  if (const auto* memory = std::get_if<MemoryInstructionRecord>(&instruction.payload);
      memory != nullptr && memory->address.size_bytes == 1) {
    if (memory->memory_kind == MemoryInstructionKind::Load) {
      return MachinePrinterMnemonicKind::LoadByte;
    }
    if (memory->memory_kind == MemoryInstructionKind::Store) {
      return MachinePrinterMnemonicKind::StoreByte;
    }
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
  return machine_node_selection_status_spelling(status);
}

[[nodiscard]] static MachineNodeStatusRecord machine_node_status_record(
    MachineNodeSelectionStatus status, std::string_view diagnostic = {}) {
  return MachineNodeStatusRecord{.status = status, .diagnostic = diagnostic};
}

[[nodiscard]] static MachineNodeStatusRecord selected_node_status() {
  return machine_node_status_record(MachineNodeSelectionStatus::Selected);
}

[[nodiscard]] static MachineNodeStatusRecord deferred_unsupported_node_status(
    std::string_view diagnostic) {
  return machine_node_status_record(MachineNodeSelectionStatus::DeferredUnsupported,
                                    diagnostic);
}

[[nodiscard]] static MachineNodeStatusRecord missing_required_facts_node_status(
    std::string_view diagnostic) {
  return machine_node_status_record(MachineNodeSelectionStatus::MissingRequiredFacts,
                                    diagnostic);
}

std::string_view aggregate_stack_copy_load_mnemonic(std::size_t width_bytes) {
  return aggregate_width_mnemonic(kAggregateStackCopyLoadMnemonics, width_bytes);
}

std::string_view aggregate_stack_copy_store_mnemonic(std::size_t width_bytes) {
  return aggregate_width_mnemonic(kAggregateStackCopyStoreMnemonics, width_bytes);
}

std::optional<abi::RegisterReference> aggregate_stack_copy_scratch(
    std::size_t width_bytes) {
  const auto scratch = abi::reserved_mir_scratch_gp_registers().front();
  if (width_bytes == 1 || width_bytes == 4) {
    return abi::w_register(scratch.index);
  }
  if (width_bytes == 8) {
    return abi::x_register(scratch.index);
  }
  return std::nullopt;
}

std::vector<std::size_t> aggregate_stack_copy_chunks(std::size_t size_bytes) {
  std::vector<std::size_t> chunks;
  std::size_t remaining = size_bytes;
  while (remaining >= 8) {
    chunks.push_back(8);
    remaining -= 8;
  }
  if (remaining >= 4) {
    chunks.push_back(4);
    remaining -= 4;
  }
  while (remaining > 0) {
    chunks.push_back(1);
    --remaining;
  }
  return chunks;
}

[[nodiscard]] static std::string_view aggregate_register_lane_load_mnemonic(
    std::size_t width_bytes) {
  return aggregate_width_mnemonic(kAggregateRegisterLaneLoadMnemonics, width_bytes);
}

[[nodiscard]] static abi::RegisterReference aggregate_register_lane_load_register(
    abi::RegisterReference reg,
    std::size_t width_bytes) {
  return width_bytes == 8 ? abi::x_register(reg.index) : abi::w_register(reg.index);
}

std::optional<abi::RegisterReference> aggregate_register_lane_scratch(
    const RegisterOperand& destination) {
  for (const auto scratch : abi::reserved_mir_scratch_gp_registers()) {
    if (same_aggregate_gp_register_index(scratch, destination.reg)) {
      continue;
    }
    bool aliases_occupied = false;
    for (const auto occupied : destination.occupied_register_references) {
      if (same_aggregate_gp_register_index(scratch, occupied)) {
        aliases_occupied = true;
        break;
      }
    }
    if (!aliases_occupied) {
      return abi::x_register(scratch.index);
    }
  }
  return std::nullopt;
}

MemoryOperand aggregate_register_lane_memory(MemoryOperand memory,
                                             std::size_t byte_offset,
                                             std::size_t width_bytes) {
  memory.byte_offset += static_cast<std::int64_t>(byte_offset);
  memory.size_bytes = width_bytes;
  memory.align_bytes = std::min<std::size_t>(memory.align_bytes, width_bytes);
  return memory;
}

bool aggregate_register_lane_memory_is_printable(const MemoryOperand& memory,
                                                 std::size_t width_bytes) {
  if (memory.base_kind == MemoryBaseKind::PointerValue && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  if (memory.base_kind == MemoryBaseKind::Register && memory.base_register.has_value()) {
    return !memory_address(memory).empty();
  }
  return memory.base_kind == MemoryBaseKind::FrameSlot &&
         aggregate_frame_slot_direct_offset_is_encodable(memory, width_bytes) &&
         !memory_address(memory).empty();
}

[[nodiscard]] static std::optional<std::size_t> aggregate_register_lane_printable_chunk(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining) {
  static constexpr std::size_t kCandidateChunks[] = {8, 4, 2, 1};
  for (const std::size_t chunk_width : kCandidateChunks) {
    if (chunk_width > remaining) {
      continue;
    }
    const auto chunk_memory =
        aggregate_register_lane_memory(memory, source_offset, chunk_width);
    if (aggregate_register_lane_memory_is_printable(chunk_memory, chunk_width) &&
        !aggregate_register_lane_load_mnemonic(chunk_width).empty()) {
      return chunk_width;
    }
  }
  return std::nullopt;
}

std::optional<AggregateRegisterLanePrintableChunk>
aggregate_register_lane_printable_chunk_descriptor(
    const MemoryOperand& memory,
    std::size_t source_offset,
    std::size_t remaining,
    abi::RegisterReference load_base_register) {
  const auto width =
      aggregate_register_lane_printable_chunk(memory, source_offset, remaining);
  if (!width.has_value()) {
    return std::nullopt;
  }
  return AggregateRegisterLanePrintableChunk{
      .memory = aggregate_register_lane_memory(memory, source_offset, *width),
      .width_bytes = *width,
      .load_mnemonic = aggregate_register_lane_load_mnemonic(*width),
      .load_register = aggregate_register_lane_load_register(load_base_register, *width),
  };
}

std::optional<abi::RegisterReference> aggregate_register_lane_destination(
    const RegisterOperand& destination,
    std::size_t lane_index) {
  if (lane_index < destination.occupied_register_references.size()) {
    return abi::x_register(destination.occupied_register_references[lane_index].index);
  }
  if (destination.reg.index + lane_index > 30) {
    return std::nullopt;
  }
  return abi::x_register(static_cast<std::uint8_t>(destination.reg.index + lane_index));
}

bool is_aggregate_register_lane_publication(
    const CallBoundaryMoveInstructionRecord& move) {
  return move.phase == prepare::PreparedMovePhase::BeforeCall &&
         move.move.destination_kind ==
             prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
         move.move.destination_storage_kind ==
             prepare::PreparedMoveStorageKind::Register &&
         move.move.op_kind == prepare::PreparedMoveResolutionOpKind::Move &&
         move.move.reason == "call_arg_byval_aggregate_register_lanes" &&
         move.source_memory.has_value() && !move.source_memory_materializes_address &&
         move.source_memory->support == MemoryOperandSupportKind::Prepared &&
         move.source_memory->size_bytes > 0 && move.source_memory->size_bytes <= 16 &&
         move.destination_register.has_value() &&
         move.destination_register->prepared_bank == prepare::PreparedRegisterBank::Gpr &&
         move.destination_register->expected_view == abi::RegisterView::X &&
         abi::is_gp_register(move.destination_register->reg);
}

std::string_view frame_instruction_kind_name(FrameInstructionKind kind) {
  return frame_instruction_kind_spelling(kind);
}

std::string_view scalar_alu_operation_kind_name(ScalarAluOperationKind kind) {
  return scalar_alu_operation_kind_spelling(kind);
}

std::string_view scalar_unary_operation_kind_name(ScalarUnaryOperationKind kind) {
  return scalar_unary_operation_kind_spelling(kind);
}

std::string_view scalar_cast_operation_kind_name(ScalarCastOperationKind kind) {
  return scalar_cast_operation_kind_spelling(kind);
}

std::string_view branch_condition_form_name(BranchConditionForm form) {
  return branch_condition_form_spelling(form);
}

std::string_view branch_compare_candidate_kind_name(BranchCompareCandidateKind kind) {
  return branch_compare_candidate_kind_spelling(kind);
}

std::string_view prepared_branch_record_error_name(PreparedBranchRecordError error) {
  return prepared_branch_record_error_spelling(error);
}

std::string_view prepared_scalar_alu_record_error_name(PreparedScalarAluRecordError error) {
  return prepared_scalar_alu_record_error_spelling(error);
}

std::string_view prepared_scalar_cast_record_error_name(PreparedScalarCastRecordError error) {
  return prepared_scalar_cast_record_error_spelling(error);
}

std::string_view prepared_scalar_fp_unary_intrinsic_record_error_name(
    PreparedScalarFpUnaryIntrinsicRecordError error) {
  return prepared_scalar_fp_unary_intrinsic_record_error_spelling(error);
}

std::string_view address_materialization_kind_name(AddressMaterializationKind kind) {
  return address_materialization_kind_spelling(kind);
}

std::string_view prepared_address_materialization_record_error_name(
    PreparedAddressMaterializationRecordError error) {
  return prepared_address_materialization_record_error_spelling(error);
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

std::optional<abi::RegisterView> saved_register_expected_view(
    prepare::PreparedRegisterBank bank) {
  switch (bank) {
    case prepare::PreparedRegisterBank::Gpr:
    case prepare::PreparedRegisterBank::AggregateAddress:
      return abi::RegisterView::X;
    case prepare::PreparedRegisterBank::Fpr:
      return abi::RegisterView::D;
    case prepare::PreparedRegisterBank::Vreg:
      return abi::RegisterView::Q;
    case prepare::PreparedRegisterBank::None:
      return std::nullopt;
  }
  return std::nullopt;
}

std::optional<RegisterOperand> register_operand_from_saved_register(
    const prepare::PreparedSavedRegister& saved) {
  const auto prepared_class = register_class_from_bank(saved.bank);
  const auto expected_view = saved_register_expected_view(saved.bank);
  const auto converted =
      abi::convert_prepared_register(saved, prepared_class, expected_view);
  if (!converted.has_value()) {
    return std::nullopt;
  }
  std::vector<std::string_view> occupied_registers;
  occupied_registers.reserve(saved.occupied_register_names.size());
  for (const auto& occupied : saved.occupied_register_names) {
    occupied_registers.push_back(occupied);
  }
  return RegisterOperand{
      .reg = *converted.reg,
      .role = RegisterOperandRole::SpillAuthority,
      .prepared_class = prepared_class,
      .prepared_bank = saved.bank,
      .expected_view = expected_view,
      .contiguous_width = saved.contiguous_width,
      .occupied_register_references = {*converted.reg},
      .occupied_registers = std::move(occupied_registers),
  };
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
    case AddressMaterializationKind::FrameSlot:
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
    return selected_node_status();
  }
  if (!instruction.target_pair.has_value() || !instruction.condition_record.has_value()) {
    return missing_required_facts_node_status(
        "conditional branch is missing target pair or condition");
  }
  const auto& condition = *instruction.condition_record;
  const bool valid_condition_type =
      condition.form == BranchConditionForm::FusedCompare
          ? condition.condition_type != bir::TypeKind::Void
          : condition.condition_type == bir::TypeKind::I1;
  if (!condition.condition_value_id.has_value() ||
      condition.condition_value_name == c4c::kInvalidValueName ||
      !valid_condition_type) {
    return missing_required_facts_node_status(
        "conditional branch is missing condition value identity");
  }
  if (condition.form == BranchConditionForm::FusedCompare) {
    const auto has_candidate = condition.compare_branch_candidate.has_value();
    const auto candidate_fusable =
        has_candidate ? condition.compare_branch_candidate->kind ==
                                BranchCompareCandidateKind::FusedCompareAndBranch &&
                            condition.compare_branch_candidate->can_fuse_with_branch
                      : condition.can_fuse_with_branch;
    if (!condition.predicate.has_value() || !condition.compare_operands.has_value()) {
      return missing_required_facts_node_status(
          "fused compare branch is missing compare facts");
    }
    if (!condition.can_fuse_with_branch || !candidate_fusable) {
      return deferred_unsupported_node_status("compare branch candidate is not fusable");
    }
    if (operands.size() < 5U) {
      return missing_required_facts_node_status(
          "fused compare branch is missing compare operands");
    }
  }
  return selected_node_status();
}

MachineNodeStatusRecord scalar_selection_status(const ScalarInstructionRecord& instruction) {
  if (instruction.scalar_alu.has_value()) {
    if ((instruction.scalar_alu->supported_integer_operation ||
         instruction.scalar_alu->supported_floating_operation) &&
        instruction.scalar_alu->operation != ScalarAluOperationKind::Deferred) {
      return selected_node_status();
    }
    return deferred_unsupported_node_status(
        "scalar ALU operation is outside the selected subset");
  }
  if (instruction.scalar_unary.has_value()) {
    if (instruction.scalar_unary->supported_integer_operation &&
        instruction.scalar_unary->operation != ScalarUnaryOperationKind::Deferred) {
      return selected_node_status();
    }
    return deferred_unsupported_node_status(
        "scalar unary operation is outside the selected subset");
  }
  if (instruction.scalar_cast.has_value()) {
    if ((instruction.scalar_cast->supported_simple_integer_cast ||
         instruction.scalar_cast->supported_float_integer_conversion ||
         instruction.scalar_cast->supported_float_width_conversion) &&
        instruction.scalar_cast->operation != ScalarCastOperationKind::Deferred) {
      return selected_node_status();
    }
    return deferred_unsupported_node_status(
        "scalar cast operation is outside the selected subset");
  }
  return missing_required_facts_node_status("scalar node is missing scalar ALU or cast record");
}

MachineNodeStatusRecord address_materialization_selection_status(
    const AddressMaterializationRecord& instruction) {
  if (instruction.source_materialization == nullptr ||
      instruction.function_name == c4c::kInvalidFunctionName ||
      instruction.block_label == c4c::kInvalidBlockLabel) {
    return missing_required_facts_node_status(
        "address materialization node is missing prepared provenance");
  }
  if (instruction.kind == AddressMaterializationKind::DeferredUnsupported) {
    return deferred_unsupported_node_status(
        "address materialization kind is outside the selected subset");
  }
  if (!instruction.result_value_id.has_value() ||
      instruction.result_value_name == c4c::kInvalidValueName ||
      !instruction.result_register.has_value() ||
      instruction.result_home_kind != prepare::PreparedValueHomeKind::Register) {
    return missing_required_facts_node_status(
        "address materialization node is missing prepared result register");
  }
  if (instruction.kind == AddressMaterializationKind::StringConstant) {
    if (!instruction.text_name.has_value()) {
      return missing_required_facts_node_status(
          "string address materialization is missing text identity");
    }
    return selected_node_status();
  }
  if (instruction.kind == AddressMaterializationKind::LabelPageLow12) {
    if (!instruction.target_label.has_value()) {
      return missing_required_facts_node_status(
          "label address materialization is missing target label identity");
    }
    return selected_node_status();
  }
  if (instruction.kind == AddressMaterializationKind::FrameSlot) {
    if (!instruction.frame_slot_id.has_value()) {
      return missing_required_facts_node_status(
          "frame-slot address materialization is missing frame slot identity");
    }
    return selected_node_status();
  }
  if (!instruction.symbol_name.has_value()) {
    return missing_required_facts_node_status(
        "global address materialization is missing symbol identity");
  }
  if (instruction.kind == AddressMaterializationKind::GotPageLow12 &&
      instruction.address_materialization_policy !=
          bir::GlobalAddressMaterializationPolicy::GotRequired) {
    return missing_required_facts_node_status(
        "GOT address materialization is missing GOT-required policy");
  }
  if (instruction.kind == AddressMaterializationKind::TlsRelative &&
      (!instruction.is_thread_local || !instruction.has_tls_address_space ||
       instruction.address_space != bir::AddressSpace::Tls)) {
    return missing_required_facts_node_status(
        "TLS address materialization is missing TLS facts");
  }
  if (instruction.kind == AddressMaterializationKind::TlsRelative &&
      (instruction.tls_model !=
           prepare::PreparedTlsMaterializationModel::LocalExecThreadPointerRelative ||
       instruction.tls_thread_pointer_register !=
           prepare::PreparedTlsThreadPointerRegister::Aarch64TpidrEl0 ||
       instruction.tls_high_relocation != prepare::PreparedTlsRelocationKind::Aarch64TprelHi12 ||
       instruction.tls_low_relocation !=
           prepare::PreparedTlsRelocationKind::Aarch64TprelLo12Nc)) {
    return missing_required_facts_node_status(
        "TLS address materialization is missing thread-pointer-relative relocation facts");
  }
  return selected_node_status();
}

MachineNodeStatusRecord spill_reload_selection_status(
    const SpillReloadInstructionRecord& instruction) {
  if (instruction.pseudo_kind == MachinePseudoKind::None ||
      instruction.op_kind == prepare::PreparedSpillReloadOpKind::Rematerialize) {
    return deferred_unsupported_node_status(
        "spill/reload op is outside the selected subset");
  }
  if (!instruction.source_spill_reload || !instruction.slot_id.has_value() ||
      !instruction.stack_offset_bytes.has_value() || !instruction.scratch.has_value() ||
      instruction.occupied_scratch_register_references.empty() ||
      !instruction.scratch_register_authority.has_value()) {
    return missing_required_facts_node_status(
        "spill/reload node is missing slot or scratch facts");
  }
  return selected_node_status();
}

MachineNodeStatusRecord frame_selection_status(const FrameInstructionRecord& instruction) {
  if (instruction.function_name == c4c::kInvalidFunctionName ||
      instruction.frame_alignment_bytes == 0) {
    return missing_required_facts_node_status(
        "frame node is missing prepared frame facts");
  }
  if (instruction.source_frame == nullptr && !instruction.preserves_link_register) {
    return missing_required_facts_node_status(
        "frame node is missing prepared frame facts");
  }
  if (instruction.preserves_link_register &&
      (!instruction.link_register_save_offset_bytes.has_value() ||
       instruction.frame_size_bytes == 0)) {
    return missing_required_facts_node_status(
        "link-register frame node is missing save slot facts");
  }
  if ((instruction.frame_kind == FrameInstructionKind::CalleeSaveStore ||
       instruction.frame_kind == FrameInstructionKind::CalleeSaveLoad) &&
      !instruction.callee_save.has_value()) {
    return missing_required_facts_node_status(
        "callee-save frame node is missing prepared save facts");
  }
  return selected_node_status();
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
      .uses = machine_effects_from_operands(operands),
      .side_effects = {MachineSideEffectKind::ControlFlowTransfer},
      .payload = instruction,
  };
}

InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction) {
  std::vector<MachineEffectResource> defs;
  if (instruction.result_register.has_value()) {
    defs.push_back(
        machine_effect_from_operand(make_register_operand(*instruction.result_register)));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(machine_prepared_value_def(instruction.result_value_id,
                                              instruction.result_value_name));
  }
  const auto selection = scalar_selection_status(instruction);
  return InstructionRecord{
      .family = InstructionFamily::Scalar,
      .surface = RecordSurfaceKind::MachineInstructionNode,
      .opcode = machine_opcode_from_scalar_instruction(instruction),
      .selection = selection,
      .operands = instruction.inputs,
      .defs = defs,
      .uses = machine_effects_from_operands(instruction.inputs),
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
    defs.push_back(machine_effect_from_operand(result));
  } else if (instruction.result_value_id.has_value()) {
    defs.push_back(
        machine_prepared_value_def(instruction.result_value_id, instruction.result_value_name));
  }
  if (instruction.symbol_name.has_value()) {
    const auto symbol = make_symbol_operand(SymbolOperand{
        .link_name = *instruction.symbol_name,
        .type = bir::TypeKind::Ptr,
        .byte_offset = instruction.byte_offset,
    });
    operands.push_back(symbol);
    uses.push_back(machine_effect_from_operand(symbol));
  } else if (instruction.target_label.has_value()) {
    const auto target = make_branch_target_operand(BranchTargetOperand{
        .surface = RecordSurfaceKind::RecordOnly,
        .block_label = *instruction.target_label,
        .function_name = instruction.function_name,
    });
    operands.push_back(target);
    uses.push_back(machine_effect_from_operand(target));
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
  std::vector<MachineEffectResource> uses = machine_effects_from_operands(operands);
  if (instruction.pseudo_kind == MachinePseudoKind::ReloadFromSlot &&
      instruction.scratch.has_value()) {
    defs.push_back(
        machine_effect_from_operand(make_register_operand(*instruction.scratch)));
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
  if (instruction.preserves_link_register) {
    const auto link_register = make_register_operand(RegisterOperand{
        .reg = abi::link_register(),
        .role = RegisterOperandRole::CallAbi,
        .prepared_class = prepare::PreparedRegisterClass::General,
        .prepared_bank = prepare::PreparedRegisterBank::Gpr,
        .expected_view = abi::RegisterView::X,
        .contiguous_width = 1,
        .occupied_register_references = {abi::link_register()},
        .occupied_registers = {"x30"},
    });
    if (instruction.frame_kind == FrameInstructionKind::EpilogueTeardown) {
      defs.push_back(machine_effect_from_operand(link_register));
    } else if (instruction.frame_kind == FrameInstructionKind::PrologueSetup) {
      uses.push_back(machine_effect_from_operand(link_register));
    }
  }
  if (instruction.callee_save.has_value() &&
      instruction.callee_save->register_operand.has_value()) {
    const auto reg_operand = make_register_operand(*instruction.callee_save->register_operand);
    if (instruction.frame_kind == FrameInstructionKind::CalleeSaveLoad) {
      defs.push_back(machine_effect_from_operand(reg_operand));
    } else {
      uses.push_back(machine_effect_from_operand(reg_operand));
    }
  }
  for (const auto& saved : instruction.saved_callee_registers) {
    auto saved_register = register_operand_from_saved_register(saved);
    if (!saved_register.has_value()) {
      continue;
    }
    const auto reg_operand = make_register_operand(*saved_register);
    if (instruction.frame_kind == FrameInstructionKind::EpilogueTeardown) {
      defs.push_back(machine_effect_from_operand(reg_operand));
    } else if (instruction.frame_kind == FrameInstructionKind::PrologueSetup) {
      uses.push_back(machine_effect_from_operand(reg_operand));
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
          deferred_unsupported_node_status("external assembler input is not a selected node"),
      .operands = instruction.operands,
      .uses = machine_effects_from_operands(instruction.operands),
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
      .selection = deferred_unsupported_node_status(
          "object records are future encoder input"),
      .operands = operands,
      .uses = machine_effects_from_operands(operands),
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
      .selection = machine_node_status_record(status, diagnostic),
  };
}

}  // namespace c4c::backend::aarch64::codegen

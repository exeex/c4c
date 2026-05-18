#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;

enum class OperandKind {
  Register,
  Immediate,
  PreparedValue,
  FrameSlot,
  Symbol,
  BranchTarget,
  Memory,
};

enum class RecordSurfaceKind {
  // Target MIR records snapshot prepared facts before instruction selection.
  TargetMirRecord,
  // Structured downstream instruction records; not assembly text.
  MachineInstructionNode,
  // Human/debug printer output records.
  PrinterOutput,
  // Structured records suitable for a future encoder/object consumer.
  EncoderInput,
  // User/tool-provided assembly input; not an internal codegen handoff.
  ExternalAssemblerInput,
  // Compatibility spelling for older record-only target MIR initializers.
  RecordOnly = TargetMirRecord,
};

enum class RegisterOperandRole {
  Physical,
  PreparedAssignment,
  AllocationResult,
  ReservedMirScratch,
  ValueHome,
  SpillAuthority,
  StoragePlan,
  CallAbi,
};

enum class ImmediateKind {
  SignedInteger,
  UnsignedInteger,
  Boolean,
  NullPointer,
};

enum class MemoryBaseKind {
  None,
  FrameSlot,
  Symbol,
  PointerValue,
  StringConstant,
  Register,
};

enum class InstructionFamily {
  // Families are intentionally broad record owners for later branch, scalar,
  // memory, call, return, assembler, and object slices. They do not name
  // concrete AArch64 opcodes.
  Branch,
  Scalar,
  I128Transport,
  F128Transport,
  I128Pair,
  Memory,
  Frame,
  Call,
  CallBoundary,
  Intrinsic,
  Return,
  Assembler,
  Object,
};

enum class FrameInstructionKind {
  PrologueSetup,
  EpilogueTeardown,
  CalleeSaveStore,
  CalleeSaveLoad,
};

enum class MemoryInstructionKind {
  Load,
  Store,
};

enum class MemoryOperandSupportKind {
  Prepared,
  DeferredUnsupported,
};

enum class MachineOpcode {
  Unspecified,
  Branch,
  ConditionalBranch,
  CompareBranch,
  AddressMaterialization,
  DirectCall,
  IndirectCall,
  FrameSetup,
  FrameTeardown,
  CalleeSaveStore,
  CalleeSaveLoad,
  CallBoundaryMove,
  CallBoundaryAbiBinding,
  I128Transport,
  F128Transport,
  F128RuntimeHelper,
  I128Pair,
  I128Shift,
  I128Compare,
  I128RuntimeHelper,
  Add,
  Sub,
  Mul,
  Div,
  And,
  Or,
  Xor,
  LogicalShiftRight,
  Neg,
  BitNot,
  CountLeadingZeros,
  CountTrailingZeros,
  ByteSwap,
  SignExtend,
  ZeroExtend,
  Truncate,
  Load,
  Store,
  AtomicLoad,
  AtomicStore,
  AtomicFence,
  AtomicRmw,
  AtomicCompareExchange,
  SpillToSlot,
  ReloadFromSlot,
  VariadicVaStart,
  VariadicVaArgScalar,
  VariadicVaArgAggregate,
  VariadicVaCopy,
  ScalarFpUnaryIntrinsic,
  Crc32WIntrinsic,
  VectorLoadIntrinsic,
  VectorAddIntrinsic,
};

enum class MachinePseudoKind {
  None,
  SpillToSlot,
  ReloadFromSlot,
};

enum class MachinePrinterMnemonicKind {
  None,
  Branch,
  ConditionalBranchNonZero,
  DirectCall,
  IndirectCall,
  Add,
  Sub,
  Load,
  Store,
  Move,
  Return,
  VariadicVaStart,
  VariadicVaArgScalar,
  VariadicVaArgAggregate,
  VariadicVaCopy,
};

enum class MachineNodeSelectionStatus {
  Selected,
  DeferredUnsupported,
  MissingRequiredFacts,
};

enum class MachineEffectResourceKind {
  PreparedValue,
  Register,
  Memory,
  FrameSlot,
  Symbol,
  BranchTarget,
  Flags,
};

enum class MachineSideEffectKind {
  ControlFlowTransfer,
  MemoryRead,
  MemoryWrite,
  VolatileMemoryAccess,
  AtomicMemoryAccess,
  Call,
  Return,
  FrameSetup,
  FrameTeardown,
  InlineAssembly,
  ObjectEmission,
};

enum class ScalarAluOperationKind {
  Add,
  Sub,
  Mul,
  Div,
  And,
  Or,
  Xor,
  LogicalShiftRight,
  Deferred,
};

enum class ScalarUnaryOperationKind {
  Neg,
  BitNot,
  CountLeadingZeros,
  CountTrailingZeros,
  ByteSwap,
  Deferred,
};

enum class ScalarCastOperationKind {
  SignExtend,
  ZeroExtend,
  Truncate,
  FloatExtend,
  FloatTruncate,
  SignedIntToFloat,
  UnsignedIntToFloat,
  FloatToSignedInt,
  FloatToUnsignedInt,
  Deferred,
};

enum class BranchConditionForm {
  Unconditional,
  MaterializedBool,
  FusedCompare,
};

enum class BranchCompareCandidateKind {
  None,
  MaterializedBoolCondition,
  FusedCompareAndBranch,
  NonFusableCompare,
};

enum class PreparedBranchRecordError {
  None,
  InvalidFunction,
  InvalidSourceBlock,
  TerminatorKindMismatch,
  MissingBranchTarget,
  TerminatorTargetMismatch,
  MissingBranchCondition,
  ConditionValueMismatch,
  MissingConditionValueHome,
  MissingCompareFacts,
  UnsupportedComparePredicate,
  MissingCompareValueHome,
};

enum class PreparedScalarAluRecordError {
  None,
  InvalidFunction,
  UnsupportedOpcode,
  UnsupportedResultValue,
  MissingResultValueHome,
  MissingResultStorage,
  UnsupportedResultStorage,
  UnsupportedOperandValue,
  MissingOperandValueHome,
  MissingOperandStorage,
  UnsupportedOperandStorage,
  UnsupportedOperandType,
  RegisterConversionFailed,
};

enum class PreparedScalarCastRecordError {
  None,
  InvalidFunction,
  UnsupportedOpcode,
  UnsupportedResultValue,
  MissingResultValueHome,
  MissingResultStorage,
  UnsupportedResultStorage,
  UnsupportedOperandValue,
  MissingOperandValueHome,
  MissingOperandStorage,
  UnsupportedOperandStorage,
  UnsupportedOperandType,
  RegisterConversionFailed,
};

enum class PreparedMemoryOperandRecordError {
  None,
  InvalidFunction,
  MissingPreparedMemoryAccess,
  UnsupportedBase,
  MissingFrameSlotId,
  MissingSymbolName,
  SymbolMismatch,
  AddressFactMismatch,
  MissingPointerValueName,
  MissingPointerValueHome,
  MissingPointerValueStorage,
  UnsupportedPointerValueStorage,
  AmbiguousPointerValueHome,
  PointerValueMismatch,
  StringIdentityMismatch,
  ResultValueMismatch,
  StoredValueMismatch,
  MissingResultValueHome,
  MissingResultStorage,
  UnsupportedResultStorage,
  MissingStoredValueHome,
  MissingStoredStorage,
  UnsupportedStoredStorage,
  RegisterConversionFailed,
};

enum class I128TransportKind {
  CarrierSnapshot,
  LoadFromMemory,
  StoreToMemory,
  CopyRegisterPair,
};

enum class F128TransportKind {
  CarrierSnapshot,
  LoadFromMemory,
  StoreToMemory,
};

enum class I128PairOperationKind {
  Add,
  Sub,
  And,
  Or,
  Xor,
};

enum class I128PairLaneSemantics {
  CarryPropagating,
  BorrowPropagating,
  IndependentBitwise,
};

enum class I128ShiftKind {
  Left,
  LogicalRight,
  ArithmeticRight,
};

enum class I128ShiftLaneSemantics {
  CrossLaneLeft,
  CrossLaneLogicalRight,
  CrossLaneArithmeticRight,
};

enum class I128ShiftCountKind {
  Immediate,
  Register,
};

enum class I128CompareSignedness {
  Equality,
  Signed,
  Unsigned,
};

enum class I128CompareHighWordSemantics {
  EqualityBothLanes,
  SignedHighWordFirst,
  UnsignedHighWordFirst,
};

enum class I128RuntimeHelperBoundaryKind {
  SignedDiv,
  UnsignedDiv,
  SignedRem,
  UnsignedRem,
};

enum class F128RuntimeHelperBoundaryKind {
  Add,
  Sub,
  Mul,
  Div,
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  F32ToF128,
  F64ToF128,
  F128ToF32,
  F128ToF64,
};

enum class PreparedI128TransportRecordError {
  None,
  InvalidFunction,
  MissingPreparedI128Carrier,
  IncompletePreparedI128Carrier,
  UnsupportedCarrierKind,
  RegisterConversionFailed,
  MissingMemoryOperand,
  MemoryAccessSizeMismatch,
};

enum class PreparedF128TransportRecordError {
  None,
  InvalidFunction,
  MissingPreparedF128Carrier,
  IncompletePreparedF128Carrier,
  UnsupportedCarrierKind,
  RegisterConversionFailed,
  MissingMemoryOperand,
  MemoryAccessSizeMismatch,
};

enum class PreparedI128PairRecordError {
  None,
  InvalidFunction,
  UnsupportedOpcode,
  UnsupportedOperandType,
  UnsupportedResultValue,
  UnsupportedOperandValue,
  MissingPreparedI128Carrier,
  IncompletePreparedI128Carrier,
  UnsupportedCarrierKind,
  RegisterConversionFailed,
  MissingScalarResultValueHome,
  MissingScalarResultStorage,
  UnsupportedScalarResultStorage,
  UnsupportedShiftCount,
  MissingShiftCountStorage,
};

enum class PreparedI128RuntimeHelperRecordError {
  None,
  InvalidFunction,
  MissingPreparedI128RuntimeHelper,
  IncompletePreparedI128RuntimeHelper,
  UnsupportedHelperFamily,
  UnsupportedSourceOperation,
  UnsupportedResultOwnership,
  MissingPreparedI128Carrier,
  IncompletePreparedI128Carrier,
  UnsupportedCarrierKind,
  RegisterConversionFailed,
  MissingBoundaryResourcePolicy,
  MissingBoundaryAbiPolicy,
  MissingClobberPolicy,
};

enum class PreparedF128RuntimeHelperRecordError {
  None,
  InvalidFunction,
  MissingPreparedF128RuntimeHelper,
  IncompletePreparedF128RuntimeHelper,
  UnsupportedHelperFamily,
  UnsupportedSourceOperation,
  UnsupportedResultOwnership,
  MissingPreparedF128Carrier,
  IncompletePreparedF128Carrier,
  UnsupportedCarrierKind,
  RegisterConversionFailed,
  MissingBoundaryResourcePolicy,
  MissingBoundaryAbiPolicy,
  MissingClobberPolicy,
};

enum class PreparedAtomicOperationRecordError {
  None,
  InvalidFunction,
  MissingPreparedAtomicOperation,
  IncompletePreparedAtomicOperation,
  UnsupportedOperationKind,
  UnsupportedOrdering,
  UnsupportedFailureOrdering,
  UnsupportedWidth,
  UnsupportedRmwOpcode,
  UnsupportedResultMode,
  MissingPointerValueName,
  MissingPointerValueHome,
  MissingPointerValueStorage,
  MissingResultValueName,
  MissingResultValueHome,
  MissingResultStorage,
  MissingStoredValueName,
  MissingStoredValueHome,
  MissingStoredStorage,
  MissingExpectedValueName,
  MissingExpectedValueHome,
  MissingExpectedStorage,
  MissingDesiredValueName,
  MissingDesiredValueHome,
  MissingDesiredStorage,
  RegisterConversionFailed,
};

enum class AddressMaterializationKind {
  DirectPageLow12,
  GotPageLow12,
  TlsRelative,
  StringConstant,
  LabelPageLow12,
  DeferredUnsupported,
};

enum class PreparedAddressMaterializationRecordError {
  None,
  InvalidFunction,
  MissingPreparedAddressMaterialization,
  UnsupportedAddressKind,
  MissingResultValueName,
  MissingResultValueHome,
  MissingResultStorage,
  UnsupportedResultStorage,
  RegisterConversionFailed,
  MissingSymbolIdentity,
  MissingStringIdentity,
  MissingLabelIdentity,
  MissingAddressMaterializationPolicy,
  AddressMaterializationPolicyMismatch,
  MissingTlsMaterializationFacts,
  TlsFactMismatch,
};

struct RegisterOperand {
  abi::RegisterReference reg{};
  RegisterOperandRole role = RegisterOperandRole::Physical;
  std::optional<prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  prepare::PreparedRegisterClass prepared_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank prepared_bank =
      prepare::PreparedRegisterBank::None;
  std::optional<abi::RegisterView> expected_view;
  std::size_t contiguous_width = 1;
  std::vector<abi::RegisterReference> occupied_register_references;
  std::vector<std::string_view> occupied_registers;
};

struct ImmediateOperand {
  ImmediateKind kind = ImmediateKind::SignedInteger;
  bir::TypeKind type = bir::TypeKind::Void;
  std::int64_t signed_value = 0;
  std::uint64_t unsigned_value = 0;
  std::optional<prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
};

struct PreparedValueOperand {
  prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind type = bir::TypeKind::Void;
  prepare::PreparedValueKind value_kind =
      prepare::PreparedValueKind::Temporary;
  prepare::PreparedValueHomeKind home_kind =
      prepare::PreparedValueHomeKind::None;
  prepare::PreparedStorageEncodingKind storage_encoding =
      prepare::PreparedStorageEncodingKind::None;
  prepare::PreparedRegisterClass register_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank storage_bank =
      prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
};

struct FrameSlotOperand {
  prepare::PreparedFrameSlotId slot_id = 0;
  std::optional<prepare::PreparedObjectId> object_id;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::SlotNameId> slot_name;
  std::optional<c4c::ValueNameId> value_name;
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t offset_bytes = 0;
  bool offset_is_prepared_snapshot = true;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

struct SymbolOperand {
  c4c::LinkNameId link_name = c4c::kInvalidLinkName;
  bir::TypeKind type = bir::TypeKind::Void;
  std::int64_t byte_offset = 0;
  bool is_extern = false;
};

struct BranchTargetOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<prepare::PreparedValueId> condition_value_id;
};

struct BranchTargetPairRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchTargetOperand true_target;
  BranchTargetOperand false_target;
};

struct CompareValueRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  std::optional<prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind type = bir::TypeKind::Void;
  bir::Value source_value;
};

struct ComparePredicateRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  bir::BinaryOpcode source_predicate = bir::BinaryOpcode::Eq;
  bir::TypeKind compare_type = bir::TypeKind::Void;
};

struct CompareOperandPairRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  CompareValueRecord lhs;
  CompareValueRecord rhs;
  bir::TypeKind compare_type = bir::TypeKind::Void;
};

struct BranchCompareCandidateRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchCompareCandidateKind kind = BranchCompareCandidateKind::None;
  std::optional<prepare::PreparedValueId> condition_value_id;
  c4c::ValueNameId condition_value_name = c4c::kInvalidValueName;
  bir::TypeKind condition_type = bir::TypeKind::Void;
  std::optional<ComparePredicateRecord> predicate;
  std::optional<CompareOperandPairRecord> compare_operands;
  std::optional<BranchTargetPairRecord> target_pair;
  bool can_fuse_with_branch = false;
};

struct BranchConditionRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchConditionForm form = BranchConditionForm::Unconditional;
  std::optional<prepare::PreparedValueId> condition_value_id;
  c4c::ValueNameId condition_value_name = c4c::kInvalidValueName;
  bir::TypeKind condition_type = bir::TypeKind::Void;
  std::optional<ComparePredicateRecord> predicate;
  std::optional<CompareOperandPairRecord> compare_operands;
  std::optional<BranchCompareCandidateRecord> compare_branch_candidate;
  bool can_fuse_with_branch = false;
};

struct MemoryOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  MemoryOperandSupportKind support = MemoryOperandSupportKind::Prepared;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  std::optional<prepare::PreparedValueId> result_value_id;
  std::optional<c4c::ValueNameId> result_value_name;
  std::optional<prepare::PreparedValueId> stored_value_id;
  std::optional<c4c::ValueNameId> stored_value_name;
  MemoryBaseKind base_kind = MemoryBaseKind::None;
  std::optional<RegisterOperand> base_register;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<c4c::LinkNameId> symbol_name;
  std::string symbol_label;
  std::optional<c4c::ValueNameId> pointer_value_name;
  std::optional<prepare::PreparedValueId> pointer_value_id;
  std::optional<c4c::TextId> string_name;
  std::optional<c4c::LinkNameId> string_symbol_name;
  std::int64_t byte_offset = 0;
  bool byte_offset_is_prepared_snapshot = true;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_volatile = false;
  bool can_use_base_plus_offset = false;
};

using OperandPayload = std::variant<RegisterOperand,
                                    ImmediateOperand,
                                    PreparedValueOperand,
                                    FrameSlotOperand,
                                    SymbolOperand,
                                    BranchTargetOperand,
                                    MemoryOperand>;

struct OperandRecord {
  OperandKind kind = OperandKind::Register;
  OperandPayload payload = RegisterOperand{};
};

struct MachineEffectResource {
  MachineEffectResourceKind kind = MachineEffectResourceKind::PreparedValue;
  std::optional<OperandRecord> operand;
  std::optional<prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::optional<abi::RegisterReference> reg;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<c4c::LinkNameId> symbol_name;
  std::optional<c4c::BlockLabelId> block_label;
};

struct MachineNodeStatusRecord {
  MachineNodeSelectionStatus status = MachineNodeSelectionStatus::Selected;
  std::string_view diagnostic;
};

struct BranchInstructionRecord {
  BranchTargetOperand target;
  std::optional<BranchTargetPairRecord> target_pair;
  std::optional<OperandRecord> condition;
  std::optional<BranchConditionRecord> condition_record;
  bool conditional = false;
};

struct PreparedBranchInstructionRecordResult {
  std::optional<BranchInstructionRecord> record;
  PreparedBranchRecordError error = PreparedBranchRecordError::None;
};

struct ScalarAluRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  ScalarAluOperationKind operation = ScalarAluOperationKind::Deferred;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Add;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::optional<RegisterOperand> result_register;
  OperandRecord lhs;
  OperandRecord rhs;
  std::optional<unsigned> post_zero_extend_result_bits;
  std::optional<unsigned> post_sign_extend_result_bits;
  bool supported_integer_operation = false;
  bool supported_floating_operation = false;
};

struct ScalarCastRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  ScalarCastOperationKind operation = ScalarCastOperationKind::Deferred;
  bir::CastOpcode source_cast_opcode = bir::CastOpcode::SExt;
  bir::TypeKind source_type = bir::TypeKind::Void;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::optional<RegisterOperand> result_register;
  OperandRecord source;
  prepare::PreparedRegisterBank source_register_bank =
      prepare::PreparedRegisterBank::None;
  prepare::PreparedRegisterBank result_register_bank =
      prepare::PreparedRegisterBank::None;
  bool crosses_register_bank = false;
  bool supported_simple_integer_cast = false;
  bool supported_float_integer_conversion = false;
  bool supported_float_width_conversion = false;
};

struct ScalarUnaryRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  ScalarUnaryOperationKind operation = ScalarUnaryOperationKind::Deferred;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::optional<RegisterOperand> result_register;
  OperandRecord operand;
  bool supported_integer_operation = false;
};

struct ScalarInstructionRecord {
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::optional<RegisterOperand> result_register;
  std::vector<OperandRecord> inputs;
  std::optional<bir::BinaryOpcode> source_binary_opcode;
  std::optional<bir::CastOpcode> source_cast_opcode;
  std::optional<ScalarAluRecord> scalar_alu;
  std::optional<ScalarUnaryRecord> scalar_unary;
  std::optional<ScalarCastRecord> scalar_cast;
};

struct ScalarFpUnaryIntrinsicRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  const prepare::PreparedIntrinsicCarrier* source_carrier = nullptr;
  bir::IntrinsicFamilyKind family = bir::IntrinsicFamilyKind::None;
  bir::IntrinsicOperationKind operation = bir::IntrinsicOperationKind::None;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::optional<prepare::PreparedValueId> operand_value_id;
  c4c::ValueNameId operand_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  OperandRecord operand;
  std::optional<RegisterOperand> result_register;
  bool has_side_effects = false;
  bool requires_feature = false;
  std::optional<std::string> source_callee_name;
  bool has_prepared_call_plan = false;
};

struct Crc32WIntrinsicRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  const prepare::PreparedIntrinsicCarrier* source_carrier = nullptr;
  bir::IntrinsicFamilyKind family = bir::IntrinsicFamilyKind::None;
  bir::IntrinsicOperationKind operation = bir::IntrinsicOperationKind::None;
  bir::IntrinsicFeatureKind required_feature = bir::IntrinsicFeatureKind::None;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::vector<bir::IntrinsicOperandRole> operand_roles;
  bir::IntrinsicSignedness signedness = bir::IntrinsicSignedness::None;
  std::optional<prepare::PreparedValueId> accumulator_value_id;
  c4c::ValueNameId accumulator_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> data_value_id;
  c4c::ValueNameId data_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  OperandRecord accumulator;
  OperandRecord data;
  std::optional<RegisterOperand> result_register;
  bool requires_feature = false;
  std::optional<std::string> source_callee_name;
  bool has_prepared_call_plan = false;
};

struct VectorLoadIntrinsicRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  const prepare::PreparedIntrinsicCarrier* source_carrier = nullptr;
  bir::IntrinsicFamilyKind family = bir::IntrinsicFamilyKind::None;
  bir::IntrinsicOperationKind operation = bir::IntrinsicOperationKind::None;
  bir::IntrinsicFeatureKind required_feature = bir::IntrinsicFeatureKind::None;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::vector<bir::IntrinsicOperandRole> operand_roles;
  bir::TypeKind vector_element_type = bir::TypeKind::Void;
  std::size_t vector_element_width_bytes = 0;
  std::size_t vector_lane_count = 0;
  std::size_t vector_total_width_bytes = 0;
  bir::IntrinsicSignedness signedness = bir::IntrinsicSignedness::None;
  bir::IntrinsicMemoryAccessKind memory_access =
      bir::IntrinsicMemoryAccessKind::None;
  std::optional<prepare::PreparedValueId> pointer_value_id;
  c4c::ValueNameId pointer_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  OperandRecord pointer;
  MemoryOperand memory;
  std::optional<RegisterOperand> result_register;
  bool requires_feature = false;
  std::optional<std::string> source_callee_name;
  bool has_prepared_call_plan = false;
};

struct VectorAddIntrinsicRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  const prepare::PreparedIntrinsicCarrier* source_carrier = nullptr;
  bir::IntrinsicFamilyKind family = bir::IntrinsicFamilyKind::None;
  bir::IntrinsicOperationKind operation = bir::IntrinsicOperationKind::None;
  bir::IntrinsicFeatureKind required_feature = bir::IntrinsicFeatureKind::None;
  bir::TypeKind operand_type = bir::TypeKind::Void;
  bir::TypeKind result_type = bir::TypeKind::Void;
  std::vector<bir::IntrinsicOperandRole> operand_roles;
  bir::TypeKind vector_element_type = bir::TypeKind::Void;
  std::size_t vector_element_width_bytes = 0;
  std::size_t vector_lane_count = 0;
  std::size_t vector_total_width_bytes = 0;
  bir::IntrinsicSignedness signedness = bir::IntrinsicSignedness::None;
  bir::IntrinsicMemoryAccessKind memory_access =
      bir::IntrinsicMemoryAccessKind::None;
  std::optional<prepare::PreparedValueId> lhs_value_id;
  c4c::ValueNameId lhs_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> rhs_value_id;
  c4c::ValueNameId rhs_value_name = c4c::kInvalidValueName;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  OperandRecord lhs;
  OperandRecord rhs;
  std::optional<RegisterOperand> result_register;
  bool requires_feature = false;
  std::optional<std::string> source_callee_name;
  bool has_prepared_call_plan = false;
};

struct PreparedScalarAluRecordResult {
  std::optional<ScalarAluRecord> record;
  PreparedScalarAluRecordError error = PreparedScalarAluRecordError::None;
};

struct PreparedScalarUnaryRecordResult {
  std::optional<ScalarUnaryRecord> record;
  PreparedScalarAluRecordError error = PreparedScalarAluRecordError::None;
};

enum class PreparedScalarFpUnaryIntrinsicRecordError {
  None,
  InvalidFunction,
  MissingPreparedIntrinsicCarrier,
  IncompletePreparedIntrinsicCarrier,
  UnsupportedIntrinsicFamily,
  UnsupportedIntrinsicOperation,
  UnsupportedOperandType,
  MissingPreparedCallPlan,
  MissingOperandValueHome,
  MissingOperandStorage,
  UnsupportedOperandStorage,
  MissingResultValueHome,
  MissingResultStorage,
  UnsupportedResultStorage,
  RegisterConversionFailed,
};

struct PreparedScalarFpUnaryIntrinsicInstructionRecordResult {
  std::optional<ScalarFpUnaryIntrinsicRecord> record;
  PreparedScalarFpUnaryIntrinsicRecordError error =
      PreparedScalarFpUnaryIntrinsicRecordError::None;
};

struct PreparedScalarCastRecordResult {
  std::optional<ScalarCastRecord> record;
  PreparedScalarCastRecordError error = PreparedScalarCastRecordError::None;
};

struct PreparedScalarInstructionRecordResult {
  std::optional<ScalarInstructionRecord> record;
  PreparedScalarAluRecordError error = PreparedScalarAluRecordError::None;
};

struct PreparedScalarCastInstructionRecordResult {
  std::optional<ScalarInstructionRecord> record;
  PreparedScalarCastRecordError error = PreparedScalarCastRecordError::None;
};

struct PreparedMemoryOperandRecordResult {
  std::optional<MemoryOperand> record;
  PreparedMemoryOperandRecordError error = PreparedMemoryOperandRecordError::None;
};

struct MemoryInstructionRecord {
  MemoryInstructionKind memory_kind = MemoryInstructionKind::Load;
  MemoryOperand address;
  std::optional<OperandRecord> value;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::optional<RegisterOperand> result_register;
};

struct PreparedMemoryInstructionRecordResult {
  std::optional<MemoryInstructionRecord> record;
  PreparedMemoryOperandRecordError error = PreparedMemoryOperandRecordError::None;
};

enum class AtomicMemoryInstructionKind {
  Load,
  Store,
  Fence,
  RmwLoop,
  CompareExchangeLoop,
};

struct AtomicMemoryInstructionRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  AtomicMemoryInstructionKind atomic_kind = AtomicMemoryInstructionKind::Load;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t width_bytes = 0;
  bir::AtomicOrdering ordering = bir::AtomicOrdering::None;
  bir::AtomicResultMode result_mode = bir::AtomicResultMode::None;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  std::optional<prepare::PreparedValueId> pointer_value_id;
  std::optional<c4c::ValueNameId> pointer_value_name;
  std::optional<RegisterOperand> pointer_register;
  std::optional<prepare::PreparedValueId> result_value_id;
  std::optional<c4c::ValueNameId> result_value_name;
  std::optional<RegisterOperand> result_register;
  std::optional<prepare::PreparedValueId> stored_value_id;
  std::optional<c4c::ValueNameId> stored_value_name;
  std::optional<RegisterOperand> stored_register;
  std::optional<prepare::PreparedValueId> expected_value_id;
  std::optional<c4c::ValueNameId> expected_value_name;
  std::optional<RegisterOperand> expected_register;
  std::optional<prepare::PreparedValueId> desired_value_id;
  std::optional<c4c::ValueNameId> desired_value_name;
  std::optional<RegisterOperand> desired_register;
  std::optional<RegisterOperand> exclusive_status_register;
  std::optional<RegisterOperand> rmw_new_value_register;
  std::optional<RegisterOperand> compare_loaded_register;
  bir::AtomicRmwOpcode rmw_opcode = bir::AtomicRmwOpcode::None;
  bir::AtomicOrdering failure_ordering = bir::AtomicOrdering::None;
  bool exclusive_retry_loop = false;
  bool compare_exchange_failure_clears_monitor = false;
  bool compare_exchange_result_is_boolean = false;
  bool compare_exchange_result_is_old_value = false;
  bool acquire_semantics = false;
  bool release_semantics = false;
  bool sequentially_consistent = false;
  bool memory_barrier_required = false;
  const prepare::PreparedAtomicOperationCarrier* source_carrier = nullptr;
};

struct PreparedAtomicOperationInstructionRecordResult {
  std::optional<AtomicMemoryInstructionRecord> record;
  PreparedAtomicOperationRecordError error =
      PreparedAtomicOperationRecordError::None;
};

struct I128LaneTransportRecord {
  prepare::PreparedI128LaneRole role = prepare::PreparedI128LaneRole::Low;
  std::size_t lane_index = 0;
  std::size_t width_bytes = 8;
  std::optional<RegisterOperand> reg;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
};

struct I128TransportRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  I128TransportKind transport_kind = I128TransportKind::CarrierSnapshot;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind value_type = bir::TypeKind::I128;
  prepare::PreparedI128CarrierKind carrier_kind = prepare::PreparedI128CarrierKind::Missing;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  prepare::PreparedRegisterBank register_bank = prepare::PreparedRegisterBank::None;
  prepare::PreparedRegisterClass register_class = prepare::PreparedRegisterClass::None;
  std::size_t contiguous_width = 1;
  std::vector<std::string> occupied_register_names;
  std::optional<prepare::PreparedRegisterPlacement> register_placement;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  I128LaneTransportRecord low_lane;
  I128LaneTransportRecord high_lane{.role = prepare::PreparedI128LaneRole::High, .lane_index = 1};
  std::optional<prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
  I128LaneTransportRecord source_low_lane;
  I128LaneTransportRecord source_high_lane{.role = prepare::PreparedI128LaneRole::High,
                                           .lane_index = 1};
  std::optional<MemoryOperand> memory;
  const prepare::PreparedI128Carrier* source_carrier = nullptr;
  const prepare::PreparedI128Carrier* copy_source_carrier = nullptr;
};

struct PreparedI128TransportRecordResult {
  std::optional<I128TransportRecord> record;
  PreparedI128TransportRecordError error = PreparedI128TransportRecordError::None;
};

struct F128TransportRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  F128TransportKind transport_kind = F128TransportKind::CarrierSnapshot;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind value_type = bir::TypeKind::F128;
  prepare::PreparedF128CarrierKind carrier_kind =
      prepare::PreparedF128CarrierKind::Missing;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  prepare::PreparedRegisterBank register_bank = prepare::PreparedRegisterBank::None;
  prepare::PreparedRegisterClass register_class = prepare::PreparedRegisterClass::None;
  std::size_t contiguous_width = 1;
  std::optional<RegisterOperand> reg;
  std::vector<std::string> occupied_register_names;
  std::optional<prepare::PreparedRegisterPlacement> register_placement;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<MemoryOperand> memory;
  const prepare::PreparedF128Carrier* source_carrier = nullptr;
};

struct PreparedF128TransportRecordResult {
  std::optional<F128TransportRecord> record;
  PreparedF128TransportRecordError error = PreparedF128TransportRecordError::None;
};

struct F128RuntimeHelperOperandRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  prepare::PreparedF128CarrierKind carrier_kind =
      prepare::PreparedF128CarrierKind::Missing;
  std::size_t width_bytes = 16;
  std::size_t align_bytes = 16;
  prepare::PreparedRegisterBank register_bank = prepare::PreparedRegisterBank::None;
  prepare::PreparedRegisterClass register_class = prepare::PreparedRegisterClass::None;
  std::optional<RegisterOperand> carrier_register;
  std::optional<RegisterOperand> abi_register;
  prepare::PreparedF128RuntimeHelper::CarrierBinding carrier_binding;
  prepare::PreparedF128RuntimeHelper::AbiRegisterBinding abi_binding;
  prepare::PreparedF128RuntimeHelper::MarshalingMove marshaling_move;
  const prepare::PreparedF128Carrier* source_carrier = nullptr;
};

struct F128RuntimeHelperScalarResultRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind type = bir::TypeKind::I32;
  std::size_t width_bytes = 4;
  prepare::PreparedRegisterBank register_bank = prepare::PreparedRegisterBank::None;
  prepare::PreparedValueHomeKind home_kind = prepare::PreparedValueHomeKind::None;
  std::optional<RegisterOperand> materialized_i1_register;
  std::optional<RegisterOperand> abi_register;
  prepare::PreparedF128RuntimeHelper::ScalarResultOwnership scalar_ownership;
  std::optional<prepare::PreparedF128RuntimeHelper::ScalarMarshalingMove> marshaling_move;
  std::optional<prepare::PreparedF128RuntimeHelper::ScalarCmpResultConsumption>
      cmp_result_consumption;
};

struct I128PairOperandRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  prepare::PreparedI128CarrierKind carrier_kind = prepare::PreparedI128CarrierKind::Missing;
  I128LaneTransportRecord low_lane;
  I128LaneTransportRecord high_lane{.role = prepare::PreparedI128LaneRole::High, .lane_index = 1};
  const prepare::PreparedI128Carrier* source_carrier = nullptr;
};

struct I128PairOperationRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  I128PairOperationKind operation = I128PairOperationKind::Add;
  I128PairLaneSemantics lane_semantics = I128PairLaneSemantics::CarryPropagating;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Add;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  bir::TypeKind operand_type = bir::TypeKind::I128;
  bir::TypeKind result_type = bir::TypeKind::I128;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  I128PairOperandRecord result;
  I128PairOperandRecord lhs;
  I128PairOperandRecord rhs;
};

struct PreparedI128PairRecordResult {
  std::optional<I128PairOperationRecord> record;
  PreparedI128PairRecordError error = PreparedI128PairRecordError::None;
};

struct I128ShiftRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  I128ShiftKind shift_kind = I128ShiftKind::Left;
  I128ShiftLaneSemantics lane_semantics = I128ShiftLaneSemantics::CrossLaneLeft;
  I128ShiftCountKind count_kind = I128ShiftCountKind::Immediate;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Shl;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  bir::TypeKind operand_type = bir::TypeKind::I128;
  bir::TypeKind result_type = bir::TypeKind::I128;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  I128PairOperandRecord result;
  I128PairOperandRecord source;
  OperandRecord shift_count;
};

struct I128CompareRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  bir::BinaryOpcode predicate = bir::BinaryOpcode::Eq;
  I128CompareSignedness signedness = I128CompareSignedness::Equality;
  I128CompareHighWordSemantics high_word_semantics =
      I128CompareHighWordSemantics::EqualityBothLanes;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  bir::TypeKind operand_type = bir::TypeKind::I128;
  bir::TypeKind result_type = bir::TypeKind::I1;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  std::optional<RegisterOperand> result_register;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  I128PairOperandRecord lhs;
  I128PairOperandRecord rhs;
};

struct I128RuntimeHelperBoundaryRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  I128RuntimeHelperBoundaryKind boundary_kind = I128RuntimeHelperBoundaryKind::SignedDiv;
  prepare::PreparedI128RuntimeHelperFamily helper_family =
      prepare::PreparedI128RuntimeHelperFamily::DivRem;
  prepare::PreparedI128RuntimeHelperKind helper_kind =
      prepare::PreparedI128RuntimeHelperKind::SignedDiv;
  std::string callee_name;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::SDiv;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bir::TypeKind source_type = bir::TypeKind::I128;
  bir::TypeKind result_type = bir::TypeKind::I128;
  prepare::PreparedValueId result_value_id = 0;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueId lhs_value_id = 0;
  c4c::ValueNameId lhs_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueId rhs_value_id = 0;
  c4c::ValueNameId rhs_value_name = c4c::kInvalidValueName;
  prepare::PreparedI128RuntimeHelperResultOwnership result_ownership =
      prepare::PreparedI128RuntimeHelperResultOwnership::Missing;
  std::size_t lane_width_bytes = 8;
  std::size_t total_size_bytes = 16;
  std::size_t total_align_bytes = 16;
  I128PairOperandRecord result;
  I128PairOperandRecord lhs;
  I128PairOperandRecord rhs;
  prepare::PreparedI128RuntimeHelper::ResourcePolicy resource_policy;
  prepare::PreparedI128RuntimeHelper::AbiPolicy abi_policy;
  prepare::PreparedI128RuntimeHelper::LivePreservationPolicy live_preservation_policy;
  prepare::PreparedI128RuntimeHelper::SelectedCallOwnershipPolicy selected_call_ownership;
  std::vector<prepare::PreparedClobberedRegister> clobbered_registers;
  const prepare::PreparedI128RuntimeHelper* source_helper = nullptr;
};

struct F128RuntimeHelperBoundaryRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  F128RuntimeHelperBoundaryKind boundary_kind = F128RuntimeHelperBoundaryKind::Add;
  prepare::PreparedF128RuntimeHelperFamily helper_family =
      prepare::PreparedF128RuntimeHelperFamily::Arithmetic;
  prepare::PreparedF128RuntimeHelperKind helper_kind =
      prepare::PreparedF128RuntimeHelperKind::Add;
  std::string callee_name;
  bir::BinaryOpcode source_binary_opcode = bir::BinaryOpcode::Add;
  std::optional<bir::CastOpcode> source_cast_opcode;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bir::TypeKind source_type = bir::TypeKind::F128;
  bir::TypeKind result_type = bir::TypeKind::F128;
  prepare::PreparedValueId result_value_id = 0;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueId operand_value_id = 0;
  c4c::ValueNameId operand_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueId lhs_value_id = 0;
  c4c::ValueNameId lhs_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueId rhs_value_id = 0;
  c4c::ValueNameId rhs_value_name = c4c::kInvalidValueName;
  prepare::PreparedF128RuntimeHelperResultOwnership result_ownership =
      prepare::PreparedF128RuntimeHelperResultOwnership::Missing;
  std::size_t width_bytes = 16;
  std::size_t align_bytes = 16;
  F128RuntimeHelperOperandRecord result;
  F128RuntimeHelperScalarResultRecord scalar_result;
  F128RuntimeHelperScalarResultRecord scalar_operand;
  F128RuntimeHelperOperandRecord lhs;
  F128RuntimeHelperOperandRecord rhs;
  prepare::PreparedF128RuntimeHelper::ResourcePolicy resource_policy;
  prepare::PreparedF128RuntimeHelper::AbiPolicy abi_policy;
  prepare::PreparedF128RuntimeHelper::LivePreservationPolicy live_preservation_policy;
  prepare::PreparedF128RuntimeHelper::SelectedCallOwnershipPolicy selected_call_ownership;
  std::vector<prepare::PreparedClobberedRegister> clobbered_registers;
  const prepare::PreparedF128RuntimeHelper* source_helper = nullptr;
};

struct PreparedI128ShiftRecordResult {
  std::optional<I128ShiftRecord> record;
  PreparedI128PairRecordError error = PreparedI128PairRecordError::None;
};

struct PreparedI128CompareRecordResult {
  std::optional<I128CompareRecord> record;
  PreparedI128PairRecordError error = PreparedI128PairRecordError::None;
};

struct PreparedI128RuntimeHelperRecordResult {
  std::optional<I128RuntimeHelperBoundaryRecord> record;
  PreparedI128RuntimeHelperRecordError error =
      PreparedI128RuntimeHelperRecordError::None;
};

struct PreparedF128RuntimeHelperRecordResult {
  std::optional<F128RuntimeHelperBoundaryRecord> record;
  PreparedF128RuntimeHelperRecordError error =
      PreparedF128RuntimeHelperRecordError::None;
};

struct AddressMaterializationRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  AddressMaterializationKind kind = AddressMaterializationKind::DeferredUnsupported;
  prepare::PreparedAddressMaterializationKind prepared_kind =
      prepare::PreparedAddressMaterializationKind::None;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t instruction_index = 0;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  prepare::PreparedValueHomeKind result_home_kind =
      prepare::PreparedValueHomeKind::None;
  std::optional<RegisterOperand> result_register;
  std::optional<c4c::LinkNameId> symbol_name;
  std::string_view symbol_label;
  std::optional<c4c::TextId> text_name;
  std::string_view text_label;
  std::optional<c4c::BlockLabelId> target_label;
  std::string_view target_label_name;
  bir::GlobalAddressMaterializationPolicy address_materialization_policy =
      bir::GlobalAddressMaterializationPolicy::Unspecified;
  std::int64_t byte_offset = 0;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_thread_local = false;
  bool has_tls_address_space = false;
  prepare::PreparedTlsMaterializationModel tls_model =
      prepare::PreparedTlsMaterializationModel::None;
  prepare::PreparedTlsThreadPointerRegister tls_thread_pointer_register =
      prepare::PreparedTlsThreadPointerRegister::None;
  prepare::PreparedTlsRelocationKind tls_high_relocation =
      prepare::PreparedTlsRelocationKind::None;
  prepare::PreparedTlsRelocationKind tls_low_relocation =
      prepare::PreparedTlsRelocationKind::None;
  const prepare::PreparedAddressMaterialization* source_materialization = nullptr;
};

struct PreparedAddressMaterializationRecordResult {
  std::optional<AddressMaterializationRecord> record;
  PreparedAddressMaterializationRecordError error =
      PreparedAddressMaterializationRecordError::None;
};

struct PreparedAddressMaterializationInstructionRecordResult {
  std::optional<AddressMaterializationRecord> record;
  PreparedAddressMaterializationRecordError error =
      PreparedAddressMaterializationRecordError::None;
};

struct SpillReloadInstructionRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  bir::TypeKind value_type = bir::TypeKind::Void;
  prepare::PreparedSpillReloadOpKind op_kind =
      prepare::PreparedSpillReloadOpKind::Spill;
  MachinePseudoKind pseudo_kind = MachinePseudoKind::SpillToSlot;
  MemoryOperand slot;
  std::optional<RegisterOperand> scratch;
  std::vector<abi::RegisterReference>
      occupied_scratch_register_references;
  std::vector<std::string_view> occupied_scratch_registers;
  std::optional<std::size_t> scratch_register_authority;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  bool stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedSpillReloadOp* source_spill_reload = nullptr;
};

struct CalleeSaveInstructionRecord {
  prepare::PreparedSavedRegister saved_register;
  std::optional<RegisterOperand> register_operand;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  bool stack_offset_is_prepared_snapshot = false;
};

struct FrameInstructionRecord {
  FrameInstructionKind frame_kind = FrameInstructionKind::PrologueSetup;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  bool preserves_link_register = false;
  std::optional<std::size_t> link_register_save_offset_bytes;
  std::vector<prepare::PreparedFrameSlotId> frame_slot_order;
  std::vector<prepare::PreparedSavedRegister> saved_callee_registers;
  bool has_dynamic_stack = false;
  bool uses_frame_pointer_for_fixed_slots = false;
  std::optional<CalleeSaveInstructionRecord> callee_save;
  const prepare::PreparedFramePlanFunction* source_frame = nullptr;
};

struct CallBoundaryMoveInstructionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  prepare::PreparedMovePhase phase = prepare::PreparedMovePhase::BeforeCall;
  prepare::PreparedMoveAuthorityKind authority_kind =
      prepare::PreparedMoveAuthorityKind::None;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::optional<c4c::BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<c4c::BlockLabelId> source_parallel_copy_successor_label;
  prepare::PreparedMoveResolution move;
  std::optional<ImmediateOperand> source_immediate;
  std::optional<RegisterOperand> source_register;
  std::optional<RegisterOperand> destination_register;
  std::optional<c4c::backend::bir::Value::F128Payload> source_f128_constant_payload;
  const prepare::PreparedF128Carrier* source_f128_carrier = nullptr;
  const prepare::PreparedF128Carrier* destination_f128_carrier = nullptr;
  const prepare::PreparedMoveBundle* source_bundle = nullptr;
  const prepare::PreparedMoveResolution* source_move = nullptr;
};

struct CallBoundaryAbiBindingInstructionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  prepare::PreparedMovePhase phase = prepare::PreparedMovePhase::BeforeCall;
  prepare::PreparedMoveAuthorityKind authority_kind =
      prepare::PreparedMoveAuthorityKind::None;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t binding_index = 0;
  std::optional<c4c::BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<c4c::BlockLabelId> source_parallel_copy_successor_label;
  prepare::PreparedAbiBinding binding;
  const prepare::PreparedMoveBundle* source_bundle = nullptr;
  const prepare::PreparedAbiBinding* source_binding = nullptr;
};

struct VariadicVaStartRecord {
  prepare::PreparedValueHome destination_va_list;
  std::size_t named_gp_register_count = 0;
  std::size_t named_fp_register_count = 0;
  std::size_t va_list_size_bytes = 0;
  std::size_t va_list_align_bytes = 0;
  std::vector<prepare::PreparedVariadicVaListField> va_list_fields;
  prepare::PreparedFrameSlotId register_save_area_slot_id = 0;
  std::size_t register_save_area_stack_offset_bytes = 0;
  std::size_t register_save_area_size_bytes = 0;
  std::size_t register_save_area_align_bytes = 0;
  std::size_t register_save_area_gp_offset_bytes = 0;
  std::size_t register_save_area_fp_offset_bytes = 0;
  std::size_t register_save_area_gp_slot_size_bytes = 0;
  std::size_t register_save_area_fp_slot_size_bytes = 0;
  std::size_t saved_gp_register_count = 0;
  std::size_t saved_fp_register_count = 0;
  std::ptrdiff_t initial_gp_offset_bytes = 0;
  std::ptrdiff_t initial_fp_offset_bytes = 0;
  prepare::PreparedFrameSlotId overflow_area_base_slot_id = 0;
  std::size_t overflow_area_base_stack_offset_bytes = 0;
  std::size_t overflow_area_align_bytes = 0;
  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
};

struct VariadicScalarVaArgRecord {
  prepare::PreparedVariadicScalarVaArgSourceClass source_class =
      prepare::PreparedVariadicScalarVaArgSourceClass::Unknown;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t value_size_bytes = 0;
  std::size_t value_align_bytes = 0;
  prepare::PreparedValueHome source_va_list;
  prepare::PreparedValueHome result_home;
  prepare::PreparedVariadicVaListFieldKind source_field =
      prepare::PreparedVariadicVaListFieldKind::GpOffset;
  std::size_t source_field_offset_bytes = 0;
  std::size_t source_slot_size_bytes = 0;
  prepare::PreparedVariadicVaListFieldKind progression_field =
      prepare::PreparedVariadicVaListFieldKind::GpOffset;
  std::size_t progression_field_offset_bytes = 0;
  std::size_t progression_stride_bytes = 0;
  prepare::PreparedVariadicVaListFieldKind overflow_source_field =
      prepare::PreparedVariadicVaListFieldKind::OverflowArgArea;
  std::size_t overflow_source_field_offset_bytes = 0;
  std::size_t overflow_stride_bytes = 0;
  prepare::PreparedFrameSlotId register_save_area_slot_id = 0;
  std::size_t register_save_area_stack_offset_bytes = 0;
  std::size_t register_save_area_size_bytes = 0;
  std::size_t register_save_area_align_bytes = 0;
  std::size_t register_save_area_gp_offset_bytes = 0;
  std::size_t register_save_area_fp_offset_bytes = 0;
  std::size_t register_save_area_gp_slot_size_bytes = 0;
  std::size_t register_save_area_fp_slot_size_bytes = 0;
  prepare::PreparedFrameSlotId overflow_area_base_slot_id = 0;
  std::size_t overflow_area_base_stack_offset_bytes = 0;
  std::size_t overflow_area_align_bytes = 0;
  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
};

struct VariadicAggregateVaArgRecord {
  prepare::PreparedVariadicAggregateVaArgSourceClass source_class =
      prepare::PreparedVariadicAggregateVaArgSourceClass::Unknown;
  std::size_t payload_size_bytes = 0;
  std::size_t payload_align_bytes = 0;
  prepare::PreparedValueHome source_va_list;
  prepare::PreparedValueHome destination_payload_home;
  prepare::PreparedVariadicVaListFieldKind source_field =
      prepare::PreparedVariadicVaListFieldKind::OverflowArgArea;
  std::size_t source_field_offset_bytes = 0;
  std::size_t source_payload_offset_bytes = 0;
  std::size_t source_slot_size_bytes = 0;
  std::size_t copy_size_bytes = 0;
  std::size_t copy_align_bytes = 0;
  prepare::PreparedVariadicVaListFieldKind progression_field =
      prepare::PreparedVariadicVaListFieldKind::OverflowArgArea;
  std::size_t progression_field_offset_bytes = 0;
  std::size_t progression_stride_bytes = 0;
  prepare::PreparedFrameSlotId register_save_area_slot_id = 0;
  std::size_t register_save_area_stack_offset_bytes = 0;
  std::size_t register_save_area_size_bytes = 0;
  std::size_t register_save_area_align_bytes = 0;
  std::size_t register_save_area_gp_offset_bytes = 0;
  std::size_t register_save_area_fp_offset_bytes = 0;
  std::size_t register_save_area_gp_slot_size_bytes = 0;
  std::size_t register_save_area_fp_slot_size_bytes = 0;
  prepare::PreparedFrameSlotId overflow_area_base_slot_id = 0;
  std::size_t overflow_area_base_stack_offset_bytes = 0;
  std::size_t overflow_area_align_bytes = 0;
  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
};

struct VariadicVaCopyFieldRecord {
  prepare::PreparedVariadicVaListFieldKind kind =
      prepare::PreparedVariadicVaListFieldKind::GpOffset;
  std::size_t source_offset_bytes = 0;
  std::size_t destination_offset_bytes = 0;
  std::size_t size_bytes = 0;
};

struct VariadicVaCopyRecord {
  prepare::PreparedValueHome destination_va_list;
  prepare::PreparedValueHome source_va_list;
  std::size_t va_list_size_bytes = 0;
  std::size_t va_list_align_bytes = 0;
  std::vector<VariadicVaCopyFieldRecord> field_copies;
  std::size_t scratch_register_count = 0;
  std::size_t scratch_stack_bytes = 0;
};

struct CallInstructionRecord {
  std::optional<SymbolOperand> direct_callee;
  std::string_view direct_callee_label;
  std::optional<OperandRecord> indirect_callee;
  std::vector<OperandRecord> arguments;
  std::optional<OperandRecord> result;
  std::optional<prepare::PreparedCallWrapperKind> wrapper_kind;
  std::size_t variadic_fpr_arg_register_count = 0;
  std::optional<prepare::PreparedMemoryReturnPlan> memory_return;
  std::optional<MemoryOperand> memory_return_storage;
  std::optional<prepare::PreparedIndirectCalleePlan> prepared_indirect_callee;
  std::vector<prepare::PreparedCallArgumentPlan> prepared_arguments;
  std::optional<prepare::PreparedCallResultPlan> prepared_result;
  std::vector<prepare::PreparedCallPreservedValue> preserved_values;
  std::vector<prepare::PreparedClobberedRegister> clobbered_registers;
  const prepare::PreparedCallPlan* source_call = nullptr;
  const prepare::PreparedVariadicEntryPlanFunction* source_variadic_entry = nullptr;
  const prepare::PreparedVariadicEntryHelperOperandHomes*
      source_variadic_helper_operand_homes = nullptr;
  std::optional<prepare::PreparedVariadicEntryHelperKind> variadic_entry_helper;
  std::optional<VariadicVaStartRecord> variadic_va_start;
  std::optional<VariadicScalarVaArgRecord> variadic_scalar_va_arg;
  std::optional<VariadicAggregateVaArgRecord> variadic_aggregate_va_arg;
  std::optional<VariadicVaCopyRecord> variadic_va_copy;
  bir::CallingConv calling_convention = bir::CallingConv::C;
  bool is_indirect = false;
  bool is_variadic = false;
  bool is_noreturn = false;
};

struct ReturnInstructionRecord {
  std::optional<OperandRecord> value;
  bir::TypeKind value_type = bir::TypeKind::Void;
};

struct InlineAsmMachineOperandRecord {
  bir::InlineAsmOperandKind kind = bir::InlineAsmOperandKind::Unsupported;
  std::size_t constraint_index = 0;
  std::string constraint;
  std::optional<std::size_t> arg_index;
  std::optional<std::size_t> output_index;
  std::optional<std::size_t> tied_output_index;
  std::optional<std::string> name;
  std::optional<bir::Value> value;
  std::optional<c4c::ValueNameId> value_name;
  std::optional<prepare::PreparedValueHome> home;
  std::optional<std::int64_t> immediate_value;
  std::optional<OperandRecord> selected_operand;
};

struct AssemblerInstructionRecord {
  std::vector<OperandRecord> operands;
  bool has_inline_asm_payload = false;
  bool side_effects = false;
  std::string inline_asm_template;
  std::string inline_asm_constraints;
  bool inline_asm_has_named_operand_references = false;
  bool inline_asm_has_template_modifiers = false;
  std::vector<InlineAsmMachineOperandRecord> inline_asm_operands;
  std::vector<std::string> inline_asm_clobbers;
  std::optional<bir::Value> inline_asm_result;
  std::optional<c4c::ValueNameId> inline_asm_result_value_name;
  std::optional<prepare::PreparedValueHome> inline_asm_result_home;
};

struct ObjectInstructionRecord {
  std::optional<SymbolOperand> symbol;
  std::optional<FrameSlotOperand> frame_slot;
  std::optional<OperandRecord> value;
  bir::TypeKind object_type = bir::TypeKind::Void;
};

using InstructionPayload = std::variant<BranchInstructionRecord,
                                        ScalarInstructionRecord,
                                        I128TransportRecord,
                                        F128TransportRecord,
                                        F128RuntimeHelperBoundaryRecord,
                                        I128PairOperationRecord,
                                        I128ShiftRecord,
                                        I128CompareRecord,
                                        I128RuntimeHelperBoundaryRecord,
                                        MemoryInstructionRecord,
                                        AtomicMemoryInstructionRecord,
                                        ScalarFpUnaryIntrinsicRecord,
                                        Crc32WIntrinsicRecord,
                                        VectorLoadIntrinsicRecord,
                                        VectorAddIntrinsicRecord,
                                        AddressMaterializationRecord,
                                        SpillReloadInstructionRecord,
                                        FrameInstructionRecord,
                                        CallBoundaryMoveInstructionRecord,
                                        CallBoundaryAbiBindingInstructionRecord,
                                        CallInstructionRecord,
                                        ReturnInstructionRecord,
                                        AssemblerInstructionRecord,
                                        ObjectInstructionRecord>;

struct InstructionRecord {
  InstructionFamily family = InstructionFamily::Scalar;
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  MachineOpcode opcode = MachineOpcode::Unspecified;
  MachinePseudoKind pseudo = MachinePseudoKind::None;
  MachineNodeStatusRecord selection;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::vector<OperandRecord> operands;
  std::vector<MachineEffectResource> defs;
  std::vector<MachineEffectResource> uses;
  std::vector<MachineEffectResource> clobbers;
  std::vector<MachineEffectResource> preserves;
  std::vector<MachineSideEffectKind> side_effects;
  InstructionPayload payload = ScalarInstructionRecord{};
};

[[nodiscard]] std::string_view operand_kind_name(OperandKind kind);
[[nodiscard]] std::string_view record_surface_kind_name(RecordSurfaceKind kind);
[[nodiscard]] bool is_target_mir_record_surface(RecordSurfaceKind kind);
[[nodiscard]] bool is_structured_downstream_surface(RecordSurfaceKind kind);
[[nodiscard]] bool is_text_first_external_input_surface(RecordSurfaceKind kind);
[[nodiscard]] std::string_view register_operand_role_name(RegisterOperandRole role);
[[nodiscard]] std::string_view immediate_kind_name(ImmediateKind kind);
[[nodiscard]] std::string_view memory_base_kind_name(MemoryBaseKind kind);
[[nodiscard]] std::string_view memory_operand_support_kind_name(MemoryOperandSupportKind kind);
[[nodiscard]] std::string_view instruction_family_name(InstructionFamily family);
[[nodiscard]] std::string_view machine_opcode_name(MachineOpcode opcode);
[[nodiscard]] std::string_view machine_pseudo_kind_name(MachinePseudoKind pseudo);
[[nodiscard]] std::string_view machine_printer_mnemonic_kind_name(
    MachinePrinterMnemonicKind kind);
[[nodiscard]] MachinePrinterMnemonicKind machine_opcode_printer_mnemonic_kind(
    MachineOpcode opcode);
[[nodiscard]] MachinePrinterMnemonicKind machine_pseudo_printer_mnemonic_kind(
    MachinePseudoKind pseudo);
[[nodiscard]] MachinePrinterMnemonicKind machine_instruction_primary_printer_mnemonic_kind(
    const InstructionRecord& instruction);
[[nodiscard]] std::string_view machine_instruction_primary_printer_mnemonic(
    const InstructionRecord& instruction);
[[nodiscard]] MachinePrinterMnemonicKind machine_instruction_auxiliary_printer_mnemonic_kind(
    const InstructionRecord& instruction);
[[nodiscard]] std::string_view machine_instruction_auxiliary_printer_mnemonic(
    const InstructionRecord& instruction);
[[nodiscard]] std::string_view machine_node_selection_status_name(
    MachineNodeSelectionStatus status);
[[nodiscard]] std::string_view machine_effect_resource_kind_name(
    MachineEffectResourceKind kind);
[[nodiscard]] std::string_view machine_side_effect_kind_name(MachineSideEffectKind kind);
[[nodiscard]] std::string_view memory_instruction_kind_name(MemoryInstructionKind kind);
[[nodiscard]] std::string_view frame_instruction_kind_name(FrameInstructionKind kind);
[[nodiscard]] std::string_view scalar_alu_operation_kind_name(ScalarAluOperationKind kind);
[[nodiscard]] std::string_view scalar_unary_operation_kind_name(
    ScalarUnaryOperationKind kind);
[[nodiscard]] std::string_view scalar_cast_operation_kind_name(ScalarCastOperationKind kind);
[[nodiscard]] std::string_view branch_condition_form_name(BranchConditionForm form);
[[nodiscard]] std::string_view branch_compare_candidate_kind_name(
    BranchCompareCandidateKind kind);
[[nodiscard]] std::string_view prepared_branch_record_error_name(PreparedBranchRecordError error);
[[nodiscard]] std::string_view prepared_scalar_alu_record_error_name(
    PreparedScalarAluRecordError error);
[[nodiscard]] std::string_view prepared_scalar_cast_record_error_name(
    PreparedScalarCastRecordError error);
[[nodiscard]] std::string_view prepared_memory_operand_record_error_name(
    PreparedMemoryOperandRecordError error);
[[nodiscard]] std::string_view prepared_atomic_operation_record_error_name(
    PreparedAtomicOperationRecordError error);
[[nodiscard]] std::string_view prepared_scalar_fp_unary_intrinsic_record_error_name(
    PreparedScalarFpUnaryIntrinsicRecordError error);
[[nodiscard]] std::string_view atomic_memory_instruction_kind_name(
    AtomicMemoryInstructionKind kind);
[[nodiscard]] std::string_view f128_transport_kind_name(F128TransportKind kind);
[[nodiscard]] std::string_view prepared_f128_transport_record_error_name(
    PreparedF128TransportRecordError error);
[[nodiscard]] std::string_view f128_runtime_helper_boundary_kind_name(
    F128RuntimeHelperBoundaryKind kind);
[[nodiscard]] std::string_view prepared_f128_runtime_helper_record_error_name(
    PreparedF128RuntimeHelperRecordError error);
[[nodiscard]] std::string_view address_materialization_kind_name(
    AddressMaterializationKind kind);
[[nodiscard]] std::string_view prepared_address_materialization_record_error_name(
    PreparedAddressMaterializationRecordError error);
[[nodiscard]] bool is_compare_predicate(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_integer_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_floating_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_floating_type(bir::TypeKind type);
[[nodiscard]] bool is_scalar_unary_integer_operation(ScalarUnaryOperationKind operation,
                                                     bir::TypeKind type);
[[nodiscard]] ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    bir::BinaryOpcode opcode);
[[nodiscard]] OperandRecord make_register_operand(RegisterOperand operand);
[[nodiscard]] OperandRecord make_immediate_operand(ImmediateOperand operand);
[[nodiscard]] OperandRecord make_prepared_value_operand(PreparedValueOperand operand);
[[nodiscard]] OperandRecord make_frame_slot_operand(FrameSlotOperand operand);
[[nodiscard]] OperandRecord make_symbol_operand(SymbolOperand operand);
[[nodiscard]] OperandRecord make_branch_target_operand(BranchTargetOperand operand);
[[nodiscard]] OperandRecord make_memory_operand(MemoryOperand operand);
[[nodiscard]] InstructionRecord make_branch_instruction(BranchInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_scalar_fp_unary_intrinsic_instruction(
    ScalarFpUnaryIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_crc32w_intrinsic_instruction(
    Crc32WIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_vector_load_intrinsic_instruction(
    VectorLoadIntrinsicRecord instruction);
[[nodiscard]] InstructionRecord make_vector_add_intrinsic_instruction(
    VectorAddIntrinsicRecord instruction);
[[nodiscard]] ScalarInstructionRecord make_scalar_alu_instruction_record(ScalarAluRecord alu);
[[nodiscard]] ScalarInstructionRecord make_scalar_unary_instruction_record(
    ScalarUnaryRecord unary);
[[nodiscard]] InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_atomic_memory_instruction(
    AtomicMemoryInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_f128_transport_instruction(
    F128TransportRecord instruction);
[[nodiscard]] InstructionRecord make_f128_runtime_helper_boundary_instruction(
    F128RuntimeHelperBoundaryRecord instruction);
[[nodiscard]] InstructionRecord make_address_materialization_instruction(
    AddressMaterializationRecord instruction);
[[nodiscard]] InstructionRecord make_spill_reload_instruction(
    SpillReloadInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_frame_instruction(FrameInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_object_instruction(ObjectInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_unsupported_machine_instruction(
    InstructionFamily family,
    MachineNodeSelectionStatus status,
    std::string_view diagnostic);
[[nodiscard]] PreparedScalarAluRecordResult make_prepared_scalar_alu_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedScalarInstructionRecordResult make_prepared_scalar_alu_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::BinaryInst& binary);
[[nodiscard]] PreparedScalarUnaryRecordResult make_prepared_scalar_unary_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    ScalarUnaryOperationKind operation,
    const bir::Value& result,
    const bir::Value& operand);
[[nodiscard]] PreparedScalarInstructionRecordResult
make_prepared_scalar_unary_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    ScalarUnaryOperationKind operation,
    const bir::Value& result,
    const bir::Value& operand);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_frame_slot_load_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadLocalInst& load);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreLocalInst& store);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::LoadGlobalInst& load);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store);
[[nodiscard]] PreparedAtomicOperationInstructionRecordResult
make_prepared_atomic_operation_instruction_record(
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAtomicOperationCarrier& operation);
[[nodiscard]] PreparedScalarFpUnaryIntrinsicInstructionRecordResult
make_prepared_scalar_fp_unary_intrinsic_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedIntrinsicCarrier& carrier);
[[nodiscard]] PreparedMemoryInstructionRecordResult
make_prepared_store_memory_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index,
    const bir::StoreGlobalInst& store);
[[nodiscard]] PreparedF128TransportRecordResult make_prepared_f128_carrier_transport_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    c4c::ValueNameId value_name,
    F128TransportKind transport_kind,
    std::optional<MemoryOperand> memory = std::nullopt);
[[nodiscard]] PreparedF128RuntimeHelperRecordResult
make_prepared_f128_runtime_helper_boundary_record(
    const prepare::PreparedF128CarrierFunction& f128_carriers,
    const prepare::PreparedF128RuntimeHelper& helper);
[[nodiscard]] PreparedAddressMaterializationRecordResult
make_prepared_address_materialization_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index);
[[nodiscard]] PreparedAddressMaterializationInstructionRecordResult
make_prepared_address_materialization_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const prepare::PreparedAddressingFunction& addressing,
    c4c::BlockLabelId block_label,
    std::size_t instruction_index);

}  // namespace c4c::backend::aarch64::codegen

#include "i128_ops.hpp"

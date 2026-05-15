#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
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
  Memory,
  Frame,
  Call,
  CallBoundary,
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
  DirectCall,
  IndirectCall,
  FrameSetup,
  FrameTeardown,
  CalleeSaveStore,
  CalleeSaveLoad,
  CallBoundaryMove,
  CallBoundaryAbiBinding,
  Add,
  Sub,
  And,
  Or,
  Xor,
  SignExtend,
  ZeroExtend,
  Truncate,
  Load,
  Store,
  SpillToSlot,
  ReloadFromSlot,
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
  And,
  Or,
  Xor,
  Deferred,
};

enum class ScalarCastOperationKind {
  SignExtend,
  ZeroExtend,
  Truncate,
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
  AmbiguousPointerValueHome,
  PointerValueMismatch,
  StringIdentityMismatch,
  ResultValueMismatch,
  StoredValueMismatch,
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
  bool supported_integer_operation = false;
};

struct ScalarCastRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  ScalarCastOperationKind operation = ScalarCastOperationKind::Deferred;
  bir::CastOpcode source_cast_opcode = bir::CastOpcode::SExt;
  bir::TypeKind source_type = bir::TypeKind::Void;
  std::optional<prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  bir::TypeKind result_type = bir::TypeKind::Void;
  OperandRecord source;
  bool supported_simple_integer_cast = false;
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
  std::optional<ScalarCastRecord> scalar_cast;
};

struct PreparedScalarAluRecordResult {
  std::optional<ScalarAluRecord> record;
  PreparedScalarAluRecordError error = PreparedScalarAluRecordError::None;
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
  std::optional<RegisterOperand> source_register;
  std::optional<RegisterOperand> destination_register;
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
  bir::CallingConv calling_convention = bir::CallingConv::C;
  bool is_indirect = false;
  bool is_variadic = false;
  bool is_noreturn = false;
};

struct ReturnInstructionRecord {
  std::optional<OperandRecord> value;
  bir::TypeKind value_type = bir::TypeKind::Void;
};

struct AssemblerInstructionRecord {
  std::vector<OperandRecord> operands;
  bool has_inline_asm_payload = false;
  bool side_effects = false;
};

struct ObjectInstructionRecord {
  std::optional<SymbolOperand> symbol;
  std::optional<FrameSlotOperand> frame_slot;
  std::optional<OperandRecord> value;
  bir::TypeKind object_type = bir::TypeKind::Void;
};

using InstructionPayload = std::variant<BranchInstructionRecord,
                                        ScalarInstructionRecord,
                                        MemoryInstructionRecord,
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
[[nodiscard]] bool is_compare_predicate(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_scalar_alu_integer_opcode(bir::BinaryOpcode opcode);
[[nodiscard]] bool is_simple_integer_cast_opcode(bir::CastOpcode opcode);
[[nodiscard]] ScalarAluOperationKind scalar_alu_operation_from_binary_opcode(
    bir::BinaryOpcode opcode);
[[nodiscard]] ScalarCastOperationKind scalar_cast_operation_from_cast_opcode(
    bir::CastOpcode opcode);
[[nodiscard]] OperandRecord make_register_operand(RegisterOperand operand);
[[nodiscard]] OperandRecord make_immediate_operand(ImmediateOperand operand);
[[nodiscard]] OperandRecord make_prepared_value_operand(PreparedValueOperand operand);
[[nodiscard]] OperandRecord make_frame_slot_operand(FrameSlotOperand operand);
[[nodiscard]] OperandRecord make_symbol_operand(SymbolOperand operand);
[[nodiscard]] OperandRecord make_branch_target_operand(BranchTargetOperand operand);
[[nodiscard]] OperandRecord make_memory_operand(MemoryOperand operand);
[[nodiscard]] InstructionRecord make_branch_instruction(BranchInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction);
[[nodiscard]] ScalarInstructionRecord make_scalar_alu_instruction_record(ScalarAluRecord alu);
[[nodiscard]] ScalarInstructionRecord make_scalar_cast_instruction_record(ScalarCastRecord cast);
[[nodiscard]] InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_spill_reload_instruction(
    SpillReloadInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_frame_instruction(FrameInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_move_instruction(
    CallBoundaryMoveInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_boundary_abi_binding_instruction(
    CallBoundaryAbiBindingInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_return_instruction(ReturnInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_object_instruction(ObjectInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_unsupported_machine_instruction(
    InstructionFamily family,
    MachineNodeSelectionStatus status,
    std::string_view diagnostic);
[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const prepare::PreparedControlFlowBlock& block,
    const bir::Terminator& terminator);
[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedControlFlowBlock& block,
    const prepare::PreparedBranchCondition& branch_condition,
    const bir::Terminator& terminator);
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
[[nodiscard]] PreparedScalarCastRecordResult make_prepared_scalar_cast_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast);
[[nodiscard]] PreparedScalarCastInstructionRecordResult
make_prepared_scalar_cast_instruction_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
    const prepare::PreparedStoragePlanFunction& storage_plan,
    const bir::CastInst& cast);
[[nodiscard]] PreparedMemoryOperandRecordResult make_prepared_memory_operand_record(
    const prepare::PreparedNameTables& names,
    const prepare::PreparedValueLocationFunction& value_locations,
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

}  // namespace c4c::backend::aarch64::codegen

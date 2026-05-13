#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace c4c::backend::aarch64::codegen {

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
  // The target record core is data-only. Later codegen ideas may consume these
  // records, but this layer must not imply selection, emission, or encoding.
  RecordOnly,
};

enum class RegisterOperandRole {
  Physical,
  PreparedAssignment,
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
  Call,
  Return,
  Assembler,
  Object,
};

enum class MemoryInstructionKind {
  Load,
  Store,
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

struct RegisterOperand {
  c4c::backend::aarch64::abi::RegisterReference reg{};
  RegisterOperandRole role = RegisterOperandRole::Physical;
  std::optional<c4c::backend::prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::prepare::PreparedRegisterClass prepared_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank prepared_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::optional<c4c::backend::aarch64::abi::RegisterView> expected_view;
  std::size_t contiguous_width = 1;
};

struct ImmediateOperand {
  ImmediateKind kind = ImmediateKind::SignedInteger;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::int64_t signed_value = 0;
  std::uint64_t unsigned_value = 0;
  std::optional<c4c::backend::prepare::PreparedValueId> source_value_id;
  c4c::ValueNameId source_value_name = c4c::kInvalidValueName;
};

struct PreparedValueOperand {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::prepare::PreparedValueKind value_kind =
      c4c::backend::prepare::PreparedValueKind::Temporary;
  c4c::backend::prepare::PreparedValueHomeKind home_kind =
      c4c::backend::prepare::PreparedValueHomeKind::None;
  c4c::backend::prepare::PreparedStorageEncodingKind storage_encoding =
      c4c::backend::prepare::PreparedStorageEncodingKind::None;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank storage_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
};

struct FrameSlotOperand {
  c4c::backend::prepare::PreparedFrameSlotId slot_id = 0;
  std::optional<c4c::backend::prepare::PreparedObjectId> object_id;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::SlotNameId> slot_name;
  std::optional<c4c::ValueNameId> value_name;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
};

struct SymbolOperand {
  c4c::LinkNameId link_name = c4c::kInvalidLinkName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::int64_t byte_offset = 0;
  bool is_extern = false;
};

struct BranchTargetOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::backend::prepare::PreparedValueId> condition_value_id;
};

struct BranchTargetPairRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchTargetOperand true_target;
  BranchTargetOperand false_target;
};

struct CompareValueRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  std::optional<c4c::backend::prepare::PreparedValueId> value_id;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::bir::Value source_value;
};

struct ComparePredicateRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  c4c::backend::bir::BinaryOpcode source_predicate = c4c::backend::bir::BinaryOpcode::Eq;
  c4c::backend::bir::TypeKind compare_type = c4c::backend::bir::TypeKind::Void;
};

struct CompareOperandPairRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  CompareValueRecord lhs;
  CompareValueRecord rhs;
  c4c::backend::bir::TypeKind compare_type = c4c::backend::bir::TypeKind::Void;
};

struct BranchCompareCandidateRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchCompareCandidateKind kind = BranchCompareCandidateKind::None;
  std::optional<c4c::backend::prepare::PreparedValueId> condition_value_id;
  c4c::ValueNameId condition_value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind condition_type = c4c::backend::bir::TypeKind::Void;
  std::optional<ComparePredicateRecord> predicate;
  std::optional<CompareOperandPairRecord> compare_operands;
  std::optional<BranchTargetPairRecord> target_pair;
  bool can_fuse_with_branch = false;
};

struct BranchConditionRecord {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  BranchConditionForm form = BranchConditionForm::Unconditional;
  std::optional<c4c::backend::prepare::PreparedValueId> condition_value_id;
  c4c::ValueNameId condition_value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind condition_type = c4c::backend::bir::TypeKind::Void;
  std::optional<ComparePredicateRecord> predicate;
  std::optional<CompareOperandPairRecord> compare_operands;
  std::optional<BranchCompareCandidateRecord> compare_branch_candidate;
  bool can_fuse_with_branch = false;
};

struct MemoryOperand {
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  MemoryBaseKind base_kind = MemoryBaseKind::None;
  std::optional<RegisterOperand> base_register;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<c4c::LinkNameId> symbol_name;
  std::optional<c4c::ValueNameId> pointer_value_name;
  std::optional<c4c::backend::prepare::PreparedValueId> pointer_value_id;
  std::optional<c4c::TextId> string_name;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  c4c::backend::bir::AddressSpace address_space = c4c::backend::bir::AddressSpace::Default;
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

struct ScalarInstructionRecord {
  std::optional<c4c::backend::prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind result_type = c4c::backend::bir::TypeKind::Void;
  std::vector<OperandRecord> inputs;
  std::optional<c4c::backend::bir::BinaryOpcode> source_binary_opcode;
  std::optional<c4c::backend::bir::CastOpcode> source_cast_opcode;
};

struct MemoryInstructionRecord {
  MemoryInstructionKind memory_kind = MemoryInstructionKind::Load;
  MemoryOperand address;
  std::optional<OperandRecord> value;
  std::optional<c4c::backend::prepare::PreparedValueId> result_value_id;
  c4c::ValueNameId result_value_name = c4c::kInvalidValueName;
  c4c::backend::bir::TypeKind value_type = c4c::backend::bir::TypeKind::Void;
};

struct CallInstructionRecord {
  std::optional<SymbolOperand> direct_callee;
  std::optional<OperandRecord> indirect_callee;
  std::vector<OperandRecord> arguments;
  std::optional<OperandRecord> result;
  c4c::backend::bir::CallingConv calling_convention = c4c::backend::bir::CallingConv::C;
  bool is_indirect = false;
  bool is_variadic = false;
  bool is_noreturn = false;
};

struct ReturnInstructionRecord {
  std::optional<OperandRecord> value;
  c4c::backend::bir::TypeKind value_type = c4c::backend::bir::TypeKind::Void;
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
  c4c::backend::bir::TypeKind object_type = c4c::backend::bir::TypeKind::Void;
};

using InstructionPayload = std::variant<BranchInstructionRecord,
                                        ScalarInstructionRecord,
                                        MemoryInstructionRecord,
                                        CallInstructionRecord,
                                        ReturnInstructionRecord,
                                        AssemblerInstructionRecord,
                                        ObjectInstructionRecord>;

struct InstructionRecord {
  InstructionFamily family = InstructionFamily::Scalar;
  RecordSurfaceKind surface = RecordSurfaceKind::RecordOnly;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  InstructionPayload payload = ScalarInstructionRecord{};
};

[[nodiscard]] std::string_view operand_kind_name(OperandKind kind);
[[nodiscard]] std::string_view record_surface_kind_name(RecordSurfaceKind kind);
[[nodiscard]] std::string_view register_operand_role_name(RegisterOperandRole role);
[[nodiscard]] std::string_view immediate_kind_name(ImmediateKind kind);
[[nodiscard]] std::string_view memory_base_kind_name(MemoryBaseKind kind);
[[nodiscard]] std::string_view instruction_family_name(InstructionFamily family);
[[nodiscard]] std::string_view memory_instruction_kind_name(MemoryInstructionKind kind);
[[nodiscard]] std::string_view branch_condition_form_name(BranchConditionForm form);
[[nodiscard]] std::string_view branch_compare_candidate_kind_name(
    BranchCompareCandidateKind kind);
[[nodiscard]] std::string_view prepared_branch_record_error_name(PreparedBranchRecordError error);
[[nodiscard]] bool is_compare_predicate(c4c::backend::bir::BinaryOpcode opcode);
[[nodiscard]] OperandRecord make_register_operand(RegisterOperand operand);
[[nodiscard]] OperandRecord make_immediate_operand(ImmediateOperand operand);
[[nodiscard]] OperandRecord make_prepared_value_operand(PreparedValueOperand operand);
[[nodiscard]] OperandRecord make_frame_slot_operand(FrameSlotOperand operand);
[[nodiscard]] OperandRecord make_symbol_operand(SymbolOperand operand);
[[nodiscard]] OperandRecord make_branch_target_operand(BranchTargetOperand operand);
[[nodiscard]] OperandRecord make_memory_operand(MemoryOperand operand);
[[nodiscard]] InstructionRecord make_branch_instruction(BranchInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_scalar_instruction(ScalarInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_memory_instruction(MemoryInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_call_instruction(CallInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_return_instruction(ReturnInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_assembler_instruction(AssemblerInstructionRecord instruction);
[[nodiscard]] InstructionRecord make_object_instruction(ObjectInstructionRecord instruction);
[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_unconditional_branch_record(
    c4c::FunctionNameId function_name,
    const c4c::backend::prepare::PreparedControlFlowBlock& block,
    const c4c::backend::bir::Terminator& terminator);
[[nodiscard]] PreparedBranchInstructionRecordResult make_prepared_conditional_branch_record(
    const c4c::backend::prepare::PreparedNameTables& names,
    const c4c::backend::prepare::PreparedValueLocationFunction& value_locations,
    const c4c::backend::prepare::PreparedControlFlowBlock& block,
    const c4c::backend::prepare::PreparedBranchCondition& branch_condition,
    const c4c::backend::bir::Terminator& terminator);

}  // namespace c4c::backend::aarch64::codegen

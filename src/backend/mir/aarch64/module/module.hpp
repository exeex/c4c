#pragma once

#include "../abi/abi.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::module {

enum class TargetRegisterReferenceKind {
  ValueHome,
  RegallocAssignment,
  SpillAuthority,
  StoragePlan,
};

struct TargetRegisterRecord {
  TargetRegisterReferenceKind reference_kind = TargetRegisterReferenceKind::ValueHome;
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank register_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::string_view physical_register;
  std::size_t contiguous_width = 1;
  std::vector<std::string_view> occupied_registers;
};

struct FrameSlotRecord {
  c4c::backend::prepare::PreparedFrameSlotId slot_id = 0;
  c4c::backend::prepare::PreparedObjectId object_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::SlotNameId> slot_name;
  std::string_view slot_label;
  std::optional<c4c::ValueNameId> value_name;
  std::string_view value_label;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  std::size_t offset_bytes = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
  bool address_exposed = false;
  bool requires_home_slot = false;
  bool permanent_home_slot = false;
  const c4c::backend::prepare::PreparedFrameSlot* source_slot = nullptr;
  const c4c::backend::prepare::PreparedStackObject* source_object = nullptr;
};

struct CalleeSaveRecord {
  c4c::backend::prepare::PreparedRegisterBank bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::string_view register_name;
  std::size_t contiguous_width = 1;
  std::vector<std::string_view> occupied_registers;
  std::size_t save_index = 0;
  const c4c::backend::prepare::PreparedSavedRegister* source_saved_register = nullptr;
};

struct DynamicStackRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view block_label_text;
  std::size_t instruction_index = 0;
  c4c::backend::prepare::PreparedDynamicStackOpKind kind =
      c4c::backend::prepare::PreparedDynamicStackOpKind::StackSave;
  std::optional<c4c::ValueNameId> result_value_name;
  std::string_view result_label;
  std::optional<c4c::ValueNameId> operand_value_name;
  std::string_view operand_label;
  std::string_view allocation_type_text;
  std::size_t element_size_bytes = 0;
  std::size_t element_align_bytes = 0;
  const c4c::backend::prepare::PreparedDynamicStackOp* source_op = nullptr;
};

struct FrameRecord {
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  bool has_dynamic_stack = false;
  bool requires_stack_save_restore = false;
  bool uses_frame_pointer_for_fixed_slots = false;
  std::vector<c4c::backend::prepare::PreparedFrameSlotId> frame_slot_order;
  std::vector<FrameSlotRecord> slots;
  std::vector<CalleeSaveRecord> callee_saves;
  std::vector<DynamicStackRecord> dynamic_stack;
  const c4c::backend::prepare::PreparedFramePlanFunction* source_frame_plan = nullptr;
  const c4c::backend::prepare::PreparedDynamicStackPlanFunction* source_dynamic_stack = nullptr;
};

struct OperandRecord {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string_view label;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  c4c::backend::prepare::PreparedValueKind value_kind =
      c4c::backend::prepare::PreparedValueKind::Temporary;
  std::optional<c4c::backend::prepare::PreparedObjectId> stack_object_id;
  c4c::backend::prepare::PreparedValueHomeKind home_kind =
      c4c::backend::prepare::PreparedValueHomeKind::None;
  c4c::backend::prepare::PreparedStorageEncodingKind storage_encoding =
      c4c::backend::prepare::PreparedStorageEncodingKind::None;
  c4c::backend::prepare::PreparedRegisterClass register_class =
      c4c::backend::prepare::PreparedRegisterClass::None;
  c4c::backend::prepare::PreparedRegisterBank storage_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  std::optional<std::int64_t> immediate_i32;
  std::optional<c4c::LinkNameId> symbol_name;
  std::string_view symbol_label;
  std::optional<c4c::ValueNameId> pointer_base_value_name;
  std::string_view pointer_base_label;
  std::optional<std::int64_t> pointer_byte_delta;
  std::optional<std::size_t> value_home_register;
  std::optional<std::size_t> assigned_register;
  std::optional<std::size_t> spill_register_authority;
  std::optional<std::size_t> storage_register;
  const c4c::backend::prepare::PreparedValueHome* source_value_home = nullptr;
  const c4c::backend::prepare::PreparedRegallocValue* source_regalloc = nullptr;
  const c4c::backend::prepare::PreparedStoragePlanValue* source_storage = nullptr;
};

struct BranchRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view block_label_text;
  c4c::backend::prepare::PreparedBranchConditionKind condition_kind =
      c4c::backend::prepare::PreparedBranchConditionKind::MaterializedBool;
  c4c::backend::bir::Value condition_value;
  std::optional<c4c::backend::bir::BinaryOpcode> predicate;
  std::optional<c4c::backend::bir::TypeKind> compare_type;
  std::optional<c4c::backend::bir::Value> lhs;
  std::optional<c4c::backend::bir::Value> rhs;
  bool can_fuse_with_branch = false;
  c4c::BlockLabelId true_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId false_label = c4c::kInvalidBlockLabel;
  const c4c::backend::prepare::PreparedBranchCondition* source_condition = nullptr;
};

struct CallArgumentRecord {
  std::size_t arg_index = 0;
  c4c::backend::prepare::PreparedRegisterBank value_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  c4c::backend::prepare::PreparedStorageEncodingKind source_encoding =
      c4c::backend::prepare::PreparedStorageEncodingKind::None;
  std::optional<c4c::backend::prepare::PreparedValueId> source_value_id;
  std::optional<c4c::backend::prepare::PreparedValueId> source_base_value_id;
  std::optional<c4c::backend::bir::Value> source_literal;
  std::optional<c4c::LinkNameId> source_symbol_name_id;
  std::string_view source_symbol_label;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::optional<c4c::ValueNameId> source_base_value_name;
  std::string_view source_base_label;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::string_view destination_register;
  std::optional<c4c::backend::prepare::PreparedRegisterBank> destination_register_bank;
  std::optional<std::size_t> destination_stack_offset_bytes;
  const c4c::backend::prepare::PreparedCallArgumentPlan* source_argument = nullptr;
};

struct CallResultRecord {
  c4c::backend::prepare::PreparedRegisterBank value_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  c4c::backend::prepare::PreparedMoveStorageKind source_storage_kind =
      c4c::backend::prepare::PreparedMoveStorageKind::None;
  c4c::backend::prepare::PreparedMoveStorageKind destination_storage_kind =
      c4c::backend::prepare::PreparedMoveStorageKind::None;
  std::optional<c4c::backend::prepare::PreparedValueId> destination_value_id;
  std::string_view source_register;
  std::optional<c4c::backend::prepare::PreparedRegisterBank> source_register_bank;
  std::optional<std::size_t> source_stack_offset_bytes;
  std::string_view destination_register;
  std::optional<c4c::backend::prepare::PreparedRegisterBank> destination_register_bank;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  const c4c::backend::prepare::PreparedCallResultPlan* source_result = nullptr;
};

struct CallPreservedValueRecord {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string_view value_label;
  c4c::backend::prepare::PreparedCallPreservationRoute route =
      c4c::backend::prepare::PreparedCallPreservationRoute::Unknown;
  std::optional<std::size_t> callee_saved_save_index;
  std::string_view register_name;
  std::optional<c4c::backend::prepare::PreparedRegisterBank> register_bank;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  const c4c::backend::prepare::PreparedCallPreservedValue* source_preserved_value = nullptr;
};

struct CallRecord {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  c4c::backend::prepare::PreparedCallWrapperKind wrapper_kind =
      c4c::backend::prepare::PreparedCallWrapperKind::Indirect;
  bool is_indirect = false;
  std::string_view direct_callee_name;
  std::optional<c4c::ValueNameId> indirect_callee_value_name;
  std::optional<c4c::backend::prepare::PreparedValueId> indirect_callee_value_id;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> memory_return_slot_id;
  std::optional<std::size_t> memory_return_stack_offset_bytes;
  std::vector<CallArgumentRecord> arguments;
  std::optional<CallResultRecord> result;
  std::vector<CallPreservedValueRecord> preserved_values;
  std::vector<CalleeSaveRecord> clobbered_registers;
  const c4c::backend::prepare::PreparedCallPlan* source_call = nullptr;
};

struct MoveRecord {
  c4c::backend::prepare::PreparedMovePhase phase =
      c4c::backend::prepare::PreparedMovePhase::BeforeInstruction;
  c4c::backend::prepare::PreparedMoveAuthorityKind authority_kind =
      c4c::backend::prepare::PreparedMoveAuthorityKind::None;
  c4c::backend::prepare::PreparedValueId from_value_id = 0;
  c4c::backend::prepare::PreparedValueId to_value_id = 0;
  c4c::backend::prepare::PreparedMoveDestinationKind destination_kind =
      c4c::backend::prepare::PreparedMoveDestinationKind::Value;
  c4c::backend::prepare::PreparedMoveStorageKind destination_storage_kind =
      c4c::backend::prepare::PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::string_view destination_register;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bool uses_cycle_temp_source = false;
  bool coalesced_by_assigned_storage = false;
  std::optional<std::size_t> source_parallel_copy_step_index;
  std::optional<std::int64_t> source_immediate_i32;
  c4c::backend::prepare::PreparedMoveResolutionOpKind op_kind =
      c4c::backend::prepare::PreparedMoveResolutionOpKind::Move;
  std::optional<c4c::BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<c4c::BlockLabelId> source_parallel_copy_successor_label;
  std::string_view reason;
  const c4c::backend::prepare::PreparedMoveBundle* source_bundle = nullptr;
  const c4c::backend::prepare::PreparedMoveResolution* source_move = nullptr;
};

struct AbiBindingRecord {
  c4c::backend::prepare::PreparedMovePhase phase =
      c4c::backend::prepare::PreparedMovePhase::BeforeInstruction;
  c4c::backend::prepare::PreparedMoveDestinationKind destination_kind =
      c4c::backend::prepare::PreparedMoveDestinationKind::Value;
  c4c::backend::prepare::PreparedMoveStorageKind destination_storage_kind =
      c4c::backend::prepare::PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::string_view destination_register;
  std::optional<std::size_t> destination_stack_offset_bytes;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const c4c::backend::prepare::PreparedMoveBundle* source_bundle = nullptr;
  const c4c::backend::prepare::PreparedAbiBinding* source_binding = nullptr;
};

struct SpillReloadRecord {
  c4c::backend::prepare::PreparedValueId value_id = 0;
  c4c::backend::prepare::PreparedSpillReloadOpKind op_kind =
      c4c::backend::prepare::PreparedSpillReloadOpKind::Spill;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  c4c::backend::prepare::PreparedRegisterBank register_bank =
      c4c::backend::prepare::PreparedRegisterBank::None;
  std::string_view register_name;
  std::optional<c4c::backend::prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  const c4c::backend::prepare::PreparedSpillReloadOp* source_spill_reload = nullptr;
};

struct ParallelCopyRecord {
  c4c::BlockLabelId predecessor_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId successor_label = c4c::kInvalidBlockLabel;
  c4c::backend::prepare::PreparedParallelCopyExecutionSite execution_site =
      c4c::backend::prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator;
  std::optional<c4c::BlockLabelId> execution_block_label;
  bool has_cycle = false;
  std::size_t move_count = 0;
  std::size_t step_count = 0;
  const c4c::backend::prepare::PreparedParallelCopyBundle* source_bundle = nullptr;
};

enum class SymbolVisibilityRecordKind {
  ExternalDeclaration,
  LinkVisibleDefinition,
};

enum class DataRelocationNeedKind {
  InitializerSymbol,
  InitializerElementSymbol,
};

struct DataRelocationNeedRecord {
  DataRelocationNeedKind kind = DataRelocationNeedKind::InitializerSymbol;
  std::size_t global_index = 0;
  c4c::LinkNameId owner_link_name = c4c::kInvalidLinkName;
  std::string_view owner_label;
  c4c::LinkNameId target_link_name = c4c::kInvalidLinkName;
  std::string_view target_label;
  std::optional<std::size_t> initializer_element_index;
  std::optional<c4c::backend::bir::Value> initializer_element;
  const c4c::backend::bir::Global* source_global = nullptr;
};

struct GlobalDataRecord {
  std::size_t global_index = 0;
  std::string_view label;
  c4c::LinkNameId link_name = c4c::kInvalidLinkName;
  c4c::backend::bir::TypeKind type = c4c::backend::bir::TypeKind::Void;
  SymbolVisibilityRecordKind visibility = SymbolVisibilityRecordKind::LinkVisibleDefinition;
  bool is_extern = false;
  bool is_thread_local = false;
  bool is_constant = false;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<c4c::backend::bir::Value> initializer;
  std::optional<std::string_view> initializer_symbol_label;
  c4c::LinkNameId initializer_symbol_name_id = c4c::kInvalidLinkName;
  std::vector<c4c::backend::bir::Value> initializer_elements;
  std::vector<DataRelocationNeedRecord> relocation_needs;
  const c4c::backend::bir::Global* source_global = nullptr;
};

struct StringDataRecord {
  std::size_t string_index = 0;
  std::string_view label;
  c4c::TextId name_id = c4c::kInvalidText;
  std::string_view bytes;
  std::size_t align_bytes = 1;
  const c4c::backend::bir::StringConstant* source_string = nullptr;
};

struct BlockRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view label;
  c4c::backend::bir::TerminatorKind terminator_kind = c4c::backend::bir::TerminatorKind::Return;
  c4c::BlockLabelId branch_target_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId true_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId false_label = c4c::kInvalidBlockLabel;
  const c4c::backend::bir::Block* source_block = nullptr;
  const c4c::backend::prepare::PreparedControlFlowBlock* control_flow = nullptr;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::string_view label;
  const c4c::backend::bir::Function* source_function = nullptr;
  const c4c::backend::prepare::PreparedControlFlowFunction* control_flow = nullptr;
  FrameRecord frame;
  std::vector<BranchRecord> branches;
  std::vector<CallRecord> calls;
  std::vector<MoveRecord> moves;
  std::vector<AbiBindingRecord> abi_bindings;
  std::vector<SpillReloadRecord> spill_reloads;
  std::vector<ParallelCopyRecord> parallel_copies;
  std::vector<OperandRecord> operands;
  std::vector<TargetRegisterRecord> target_registers;
  std::vector<BlockRecord> blocks;
};

struct Module {
  const c4c::backend::prepare::PreparedBirModule* prepared = nullptr;
  c4c::TargetProfile target_profile{};
  std::vector<GlobalDataRecord> globals;
  std::vector<StringDataRecord> strings;
  std::vector<DataRelocationNeedRecord> relocation_needs;
  std::vector<FunctionRecord> functions;
};

struct BuildResult {
  std::optional<Module> module;
  std::optional<c4c::backend::aarch64::abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::module

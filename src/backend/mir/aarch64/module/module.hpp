#pragma once

#include "../abi/abi.hpp"
#include "../codegen/records.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace c4c::backend::aarch64::module {

namespace prepare = c4c::backend::prepare;
namespace bir = c4c::backend::bir;
namespace abi = c4c::backend::aarch64::abi;
namespace codegen = c4c::backend::aarch64::codegen;

enum class TargetRegisterReferenceKind {
  ValueHome,
  RegallocAssignment,
  SpillAuthority,
  StoragePlan,
};

enum class AllocationSnapshotKind {
  PreparedSnapshot,
  AllocationResult,
  SpillReloadScratch,
  DisplayOnly,
};

enum class AllocationLocationKind {
  None,
  PhysicalRegister,
  SpillSlot,
  NonRegister,
  FutureVirtualRegister,
};

enum class AllocationAuthorityKind {
  None,
  ValueHome,
  RegallocAssignment,
  SpillAuthority,
  StoragePlan,
  DeferredPlaceholder,
};

enum class SpillReloadPseudoKind {
  StoreFromRegisterToSlot,
  ReloadFromSlotToScratch,
  RematerializeToScratch,
};

struct TargetRegisterRecord {
  TargetRegisterReferenceKind reference_kind = TargetRegisterReferenceKind::ValueHome;
  AllocationSnapshotKind allocation_snapshot = AllocationSnapshotKind::PreparedSnapshot;
  AllocationAuthorityKind allocation_authority = AllocationAuthorityKind::None;
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  prepare::PreparedRegisterClass register_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank register_bank =
      prepare::PreparedRegisterBank::None;
  std::optional<abi::RegisterReference> register_reference;
  std::optional<abi::AllocationRegisterPool> allocation_pool;
  std::string_view physical_register;
  std::size_t contiguous_width = 1;
  std::vector<abi::RegisterReference> occupied_register_references;
  std::vector<std::string_view> occupied_registers;
  bool is_reserved_mir_scratch = false;
  bool may_be_long_lived_home = false;
};

struct FrameSlotRecord {
  prepare::PreparedFrameSlotId slot_id = 0;
  prepare::PreparedObjectId object_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::optional<c4c::SlotNameId> slot_name;
  std::string_view slot_label;
  std::optional<c4c::ValueNameId> value_name;
  std::string_view value_label;
  bir::TypeKind type = bir::TypeKind::Void;
  std::size_t offset_bytes = 0;
  bool offset_is_prepared_snapshot = true;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool fixed_location = false;
  bool address_exposed = false;
  bool requires_home_slot = false;
  bool permanent_home_slot = false;
  const prepare::PreparedFrameSlot* source_slot = nullptr;
  const prepare::PreparedStackObject* source_object = nullptr;
};

struct CalleeSaveRecord {
  prepare::PreparedRegisterBank bank =
      prepare::PreparedRegisterBank::None;
  std::optional<abi::RegisterReference> register_reference;
  std::string_view register_name;
  std::size_t contiguous_width = 1;
  std::vector<abi::RegisterReference> occupied_register_references;
  std::vector<std::string_view> occupied_registers;
  std::size_t save_index = 0;
  const prepare::PreparedSavedRegister* source_saved_register = nullptr;
};

struct DynamicStackRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view block_label_text;
  std::size_t instruction_index = 0;
  prepare::PreparedDynamicStackOpKind kind =
      prepare::PreparedDynamicStackOpKind::StackSave;
  std::optional<c4c::ValueNameId> result_value_name;
  std::string_view result_label;
  std::optional<c4c::ValueNameId> operand_value_name;
  std::string_view operand_label;
  std::string_view allocation_type_text;
  std::size_t element_size_bytes = 0;
  std::size_t element_align_bytes = 0;
  const prepare::PreparedDynamicStackOp* source_op = nullptr;
};

struct FrameRecord {
  std::size_t frame_size_bytes = 0;
  std::size_t frame_alignment_bytes = 0;
  bool has_dynamic_stack = false;
  bool requires_stack_save_restore = false;
  bool uses_frame_pointer_for_fixed_slots = false;
  std::vector<prepare::PreparedFrameSlotId> frame_slot_order;
  std::vector<FrameSlotRecord> slots;
  std::vector<CalleeSaveRecord> callee_saves;
  std::vector<DynamicStackRecord> dynamic_stack;
  const prepare::PreparedFramePlanFunction* source_frame_plan = nullptr;
  const prepare::PreparedDynamicStackPlanFunction* source_dynamic_stack = nullptr;
};

struct OperandRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string_view label;
  bir::TypeKind type = bir::TypeKind::Void;
  prepare::PreparedValueKind value_kind =
      prepare::PreparedValueKind::Temporary;
  std::optional<prepare::PreparedObjectId> stack_object_id;
  prepare::PreparedValueHomeKind home_kind =
      prepare::PreparedValueHomeKind::None;
  prepare::PreparedStorageEncodingKind storage_encoding =
      prepare::PreparedStorageEncodingKind::None;
  prepare::PreparedAllocationStatus allocation_status =
      prepare::PreparedAllocationStatus::Unallocated;
  AllocationLocationKind allocation_location = AllocationLocationKind::None;
  AllocationAuthorityKind allocation_authority = AllocationAuthorityKind::None;
  prepare::PreparedRegisterClass register_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank storage_bank =
      prepare::PreparedRegisterBank::None;
  std::size_t register_group_width = 1;
  std::optional<prepare::PreparedFrameSlotId> frame_slot_id;
  std::optional<prepare::PreparedFrameSlotId> spill_slot_id;
  std::optional<std::size_t> spill_slot_size_bytes;
  std::optional<std::size_t> spill_slot_align_bytes;
  bool spill_slot_fixed_location = false;
  std::optional<std::size_t> stack_offset_bytes;
  bool stack_offset_is_prepared_snapshot = false;
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
  const prepare::PreparedValueHome* source_value_home = nullptr;
  const prepare::PreparedRegallocValue* source_regalloc = nullptr;
  const prepare::PreparedStoragePlanValue* source_storage = nullptr;
};

struct BranchRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view block_label_text;
  prepare::PreparedBranchConditionKind condition_kind =
      prepare::PreparedBranchConditionKind::MaterializedBool;
  bir::Value condition_value;
  std::optional<bir::BinaryOpcode> predicate;
  std::optional<bir::TypeKind> compare_type;
  std::optional<bir::Value> lhs;
  std::optional<bir::Value> rhs;
  bool can_fuse_with_branch = false;
  c4c::BlockLabelId true_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId false_label = c4c::kInvalidBlockLabel;
  const prepare::PreparedBranchCondition* source_condition = nullptr;
};

struct CallArgumentRecord {
  std::size_t arg_index = 0;
  prepare::PreparedRegisterBank value_bank =
      prepare::PreparedRegisterBank::None;
  prepare::PreparedStorageEncodingKind source_encoding =
      prepare::PreparedStorageEncodingKind::None;
  std::optional<prepare::PreparedValueId> source_value_id;
  std::optional<prepare::PreparedValueId> source_base_value_id;
  std::optional<bir::Value> source_literal;
  std::optional<c4c::LinkNameId> source_symbol_name_id;
  std::string_view source_symbol_label;
  std::optional<prepare::PreparedFrameSlotId> source_slot_id;
  std::optional<std::size_t> source_stack_offset_bytes;
  bool source_stack_offset_is_prepared_snapshot = false;
  std::optional<c4c::ValueNameId> source_base_value_name;
  std::string_view source_base_label;
  std::optional<std::int64_t> source_pointer_byte_delta;
  std::optional<abi::RegisterReference> destination_register_reference;
  std::string_view destination_register;
  std::size_t destination_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      destination_occupied_register_references;
  std::vector<std::string_view> destination_occupied_registers;
  std::optional<prepare::PreparedRegisterBank> destination_register_bank;
  std::optional<std::size_t> destination_stack_offset_bytes;
  bool destination_stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedCallArgumentPlan* source_argument = nullptr;
};

struct CallResultRecord {
  prepare::PreparedRegisterBank value_bank =
      prepare::PreparedRegisterBank::None;
  prepare::PreparedMoveStorageKind source_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  prepare::PreparedMoveStorageKind destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  std::optional<prepare::PreparedValueId> destination_value_id;
  std::optional<abi::RegisterReference> source_register_reference;
  std::string_view source_register;
  std::size_t source_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      source_occupied_register_references;
  std::vector<std::string_view> source_occupied_registers;
  std::optional<prepare::PreparedRegisterBank> source_register_bank;
  std::optional<std::size_t> source_stack_offset_bytes;
  bool source_stack_offset_is_prepared_snapshot = false;
  std::optional<abi::RegisterReference> destination_register_reference;
  std::string_view destination_register;
  std::size_t destination_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      destination_occupied_register_references;
  std::vector<std::string_view> destination_occupied_registers;
  std::optional<prepare::PreparedRegisterBank> destination_register_bank;
  std::optional<prepare::PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  bool destination_stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedCallResultPlan* source_result = nullptr;
};

struct CallPreservedValueRecord {
  prepare::PreparedValueId value_id = 0;
  c4c::ValueNameId value_name = c4c::kInvalidValueName;
  std::string_view value_label;
  prepare::PreparedCallPreservationRoute route =
      prepare::PreparedCallPreservationRoute::Unknown;
  std::optional<std::size_t> callee_saved_save_index;
  std::optional<abi::RegisterReference> register_reference;
  std::string_view register_name;
  std::optional<prepare::PreparedRegisterBank> register_bank;
  std::size_t contiguous_width = 1;
  std::vector<abi::RegisterReference> occupied_register_references;
  std::vector<std::string_view> occupied_registers;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  bool stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedCallPreservedValue* source_preserved_value = nullptr;
};

struct CallRecord {
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  prepare::PreparedCallWrapperKind wrapper_kind =
      prepare::PreparedCallWrapperKind::Indirect;
  bool is_indirect = false;
  std::string_view direct_callee_name;
  std::optional<c4c::ValueNameId> indirect_callee_value_name;
  std::optional<prepare::PreparedValueId> indirect_callee_value_id;
  std::optional<prepare::PreparedFrameSlotId> memory_return_slot_id;
  std::optional<std::size_t> memory_return_stack_offset_bytes;
  std::vector<CallArgumentRecord> arguments;
  std::optional<CallResultRecord> result;
  std::vector<CallPreservedValueRecord> preserved_values;
  std::vector<CalleeSaveRecord> clobbered_registers;
  const prepare::PreparedCallPlan* source_call = nullptr;
};

struct MoveRecord {
  prepare::PreparedMovePhase phase =
      prepare::PreparedMovePhase::BeforeInstruction;
  prepare::PreparedMoveAuthorityKind authority_kind =
      prepare::PreparedMoveAuthorityKind::None;
  prepare::PreparedValueId from_value_id = 0;
  prepare::PreparedValueId to_value_id = 0;
  prepare::PreparedMoveDestinationKind destination_kind =
      prepare::PreparedMoveDestinationKind::Value;
  prepare::PreparedMoveStorageKind destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::optional<abi::RegisterReference> destination_register_reference;
  std::string_view destination_register;
  std::size_t destination_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      destination_occupied_register_references;
  std::vector<std::string_view> destination_occupied_registers;
  std::optional<prepare::PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  bool destination_stack_offset_is_prepared_snapshot = false;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  bool uses_cycle_temp_source = false;
  bool coalesced_by_assigned_storage = false;
  std::optional<std::size_t> source_parallel_copy_step_index;
  std::optional<std::int64_t> source_immediate_i32;
  prepare::PreparedMoveResolutionOpKind op_kind =
      prepare::PreparedMoveResolutionOpKind::Move;
  std::optional<c4c::BlockLabelId> source_parallel_copy_predecessor_label;
  std::optional<c4c::BlockLabelId> source_parallel_copy_successor_label;
  std::string_view reason;
  const prepare::PreparedMoveBundle* source_bundle = nullptr;
  const prepare::PreparedMoveResolution* source_move = nullptr;
};

struct AbiBindingRecord {
  prepare::PreparedMovePhase phase =
      prepare::PreparedMovePhase::BeforeInstruction;
  prepare::PreparedMoveAuthorityKind authority_kind =
      prepare::PreparedMoveAuthorityKind::None;
  prepare::PreparedMoveDestinationKind destination_kind =
      prepare::PreparedMoveDestinationKind::Value;
  prepare::PreparedMoveStorageKind destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  std::optional<std::size_t> destination_abi_index;
  std::optional<abi::RegisterReference> destination_register_reference;
  std::string_view destination_register;
  std::size_t destination_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      destination_occupied_register_references;
  std::vector<std::string_view> destination_occupied_registers;
  std::optional<prepare::PreparedFrameSlotId> destination_slot_id;
  std::optional<std::size_t> destination_stack_offset_bytes;
  bool destination_stack_offset_is_prepared_snapshot = false;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  const prepare::PreparedMoveBundle* source_bundle = nullptr;
  const prepare::PreparedAbiBinding* source_binding = nullptr;
};

struct SpillReloadRecord {
  prepare::PreparedValueId value_id = 0;
  prepare::PreparedSpillReloadOpKind op_kind =
      prepare::PreparedSpillReloadOpKind::Spill;
  SpillReloadPseudoKind pseudo_kind = SpillReloadPseudoKind::StoreFromRegisterToSlot;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  prepare::PreparedRegisterClass register_class =
      prepare::PreparedRegisterClass::None;
  prepare::PreparedRegisterBank register_bank =
      prepare::PreparedRegisterBank::None;
  std::optional<abi::RegisterReference> register_reference;
  std::string_view register_name;
  std::size_t contiguous_width = 1;
  std::vector<abi::RegisterReference> occupied_register_references;
  std::vector<std::string_view> occupied_registers;
  std::optional<std::size_t> scratch_register_authority;
  std::optional<prepare::PreparedFrameSlotId> slot_id;
  std::optional<std::size_t> stack_offset_bytes;
  bool stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedSpillReloadOp* source_spill_reload = nullptr;
};

struct ParallelCopyMoveRecord {
  std::size_t join_transfer_index = 0;
  std::size_t edge_transfer_index = 0;
  bir::Value source_value;
  bir::Value destination_value;
  prepare::PreparedJoinTransferCarrierKind carrier_kind =
      prepare::PreparedJoinTransferCarrierKind::None;
  std::optional<c4c::SlotNameId> storage_name;
  const prepare::PreparedParallelCopyMove* source_move = nullptr;
};

struct ParallelCopyStepRecord {
  prepare::PreparedParallelCopyStepKind kind =
      prepare::PreparedParallelCopyStepKind::Move;
  std::size_t move_index = 0;
  bool uses_cycle_temp_source = false;
  bir::Value source_value;
  bir::Value destination_value;
  prepare::PreparedJoinTransferCarrierKind carrier_kind =
      prepare::PreparedJoinTransferCarrierKind::None;
  std::optional<c4c::SlotNameId> storage_name;
  bool has_target_move_record = false;
  prepare::PreparedMoveAuthorityKind target_move_authority_kind =
      prepare::PreparedMoveAuthorityKind::None;
  prepare::PreparedMoveDestinationKind target_destination_kind =
      prepare::PreparedMoveDestinationKind::Value;
  prepare::PreparedMoveStorageKind target_destination_storage_kind =
      prepare::PreparedMoveStorageKind::None;
  std::optional<abi::RegisterReference>
      target_destination_register_reference;
  std::string_view target_destination_register;
  std::size_t target_destination_contiguous_width = 1;
  std::vector<abi::RegisterReference>
      target_destination_occupied_register_references;
  std::vector<std::string_view> target_destination_occupied_registers;
  std::optional<prepare::PreparedFrameSlotId> target_destination_slot_id;
  std::optional<std::size_t> target_destination_stack_offset_bytes;
  bool target_destination_stack_offset_is_prepared_snapshot = false;
  const prepare::PreparedParallelCopyStep* source_step = nullptr;
  const prepare::PreparedParallelCopyMove* source_move = nullptr;
  const prepare::PreparedMoveResolution* source_target_move = nullptr;
};

struct ParallelCopyRecord {
  c4c::BlockLabelId predecessor_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId successor_label = c4c::kInvalidBlockLabel;
  prepare::PreparedParallelCopyExecutionSite execution_site =
      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator;
  std::optional<c4c::BlockLabelId> execution_block_label;
  bool has_cycle = false;
  std::size_t move_count = 0;
  std::size_t step_count = 0;
  std::vector<ParallelCopyMoveRecord> moves;
  std::vector<ParallelCopyStepRecord> steps;
  const prepare::PreparedParallelCopyBundle* source_bundle = nullptr;
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
  std::optional<bir::Value> initializer_element;
  const bir::Global* source_global = nullptr;
};

struct GlobalDataRecord {
  std::size_t global_index = 0;
  std::string_view label;
  c4c::LinkNameId link_name = c4c::kInvalidLinkName;
  bir::TypeKind type = bir::TypeKind::Void;
  SymbolVisibilityRecordKind visibility = SymbolVisibilityRecordKind::LinkVisibleDefinition;
  bool is_extern = false;
  bool is_thread_local = false;
  bool is_constant = false;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  std::optional<bir::Value> initializer;
  std::optional<std::string_view> initializer_symbol_label;
  c4c::LinkNameId initializer_symbol_name_id = c4c::kInvalidLinkName;
  std::vector<bir::Value> initializer_elements;
  std::vector<DataRelocationNeedRecord> relocation_needs;
  const bir::Global* source_global = nullptr;
};

struct StringDataRecord {
  std::size_t string_index = 0;
  std::string_view label;
  c4c::TextId name_id = c4c::kInvalidText;
  std::string_view bytes;
  std::size_t align_bytes = 1;
  const bir::StringConstant* source_string = nullptr;
};

struct BlockRecord {
  c4c::BlockLabelId block_label = c4c::kInvalidBlockLabel;
  std::string_view label;
  bir::TerminatorKind terminator_kind = bir::TerminatorKind::Return;
  c4c::BlockLabelId branch_target_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId true_label = c4c::kInvalidBlockLabel;
  c4c::BlockLabelId false_label = c4c::kInvalidBlockLabel;
  const bir::Block* source_block = nullptr;
  const prepare::PreparedControlFlowBlock* control_flow = nullptr;
};

struct FunctionRecord {
  c4c::FunctionNameId function_name = c4c::kInvalidFunctionName;
  std::string_view label;
  const bir::Function* source_function = nullptr;
  const prepare::PreparedControlFlowFunction* control_flow = nullptr;
  FrameRecord frame;
  std::vector<BranchRecord> branches;
  std::vector<CallRecord> calls;
  std::vector<MoveRecord> moves;
  std::vector<AbiBindingRecord> abi_bindings;
  std::vector<SpillReloadRecord> spill_reloads;
  std::vector<ParallelCopyRecord> parallel_copies;
  std::vector<OperandRecord> operands;
  std::vector<TargetRegisterRecord> target_registers;
  std::vector<codegen::InstructionRecord> machine_nodes;
  std::vector<BlockRecord> blocks;
};

struct Module {
  const prepare::PreparedBirModule* prepared = nullptr;
  c4c::TargetProfile target_profile{};
  std::vector<GlobalDataRecord> globals;
  std::vector<StringDataRecord> strings;
  std::vector<DataRelocationNeedRecord> relocation_needs;
  std::vector<FunctionRecord> functions;
};

struct BuildResult {
  std::optional<Module> module;
  std::optional<abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::module

#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "publication_plans.hpp"
#include "stack_layout/stack_layout.hpp"
#include "value_locations.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace c4c::backend::prepare {

struct PreparedBirModule;
[[nodiscard]] const PreparedFrameSlot* find_frame_slot_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedFrameSlotId slot_id);

[[nodiscard]] const PreparedStackObject* find_stack_object_by_id(
    const PreparedStackLayout& stack_layout,
    PreparedObjectId object_id);

struct PreparedAfterCallResultLaneBinding {
  const PreparedMoveBundle* move_bundle = nullptr;
  const PreparedAbiBinding* abi_binding = nullptr;
  PreparedValueId value_id = 0;
  ValueNameId value_name = kInvalidValueName;
  std::size_t block_index = 0;
  std::size_t instruction_index = 0;
  std::size_t lane_index = 0;
};

struct PreparedMoveBundleLookups {
  std::unordered_map<std::size_t, const PreparedMoveBundle*> bundles_by_position;
  std::unordered_map<std::size_t, const PreparedMoveResolution*>
      before_call_argument_moves_by_position_and_abi;
  std::unordered_map<std::size_t, const PreparedMoveResolution*>
      before_return_abi_moves_by_source_and_bank;
  std::vector<PreparedAfterCallResultLaneBinding> after_call_result_lane_bindings;
  std::unordered_map<std::size_t, const PreparedAfterCallResultLaneBinding*>
      after_call_result_lane_bindings_by_position_and_value;
};

struct PreparedReturnChainLookups {
  std::unordered_map<std::size_t, ValueNameId> terminal_return_values_by_chain_value;
  std::unordered_map<std::size_t, ValueNameId> next_operand_values_by_chain_value;
};

struct PreparedValueHomeLookups {
  std::unordered_map<PreparedValueId, const PreparedValueHome*> homes_by_id;
  std::unordered_map<ValueNameId, PreparedValueId> value_ids;
};

enum class PreparedCurrentBlockJoinParallelCopySourceStatus {
  Available,
  MissingNames,
  MissingValueLocations,
  MissingEdgePublicationLookups,
  MissingBlock,
  MissingSuccessorLabel,
};

[[nodiscard]] constexpr std::string_view
prepared_current_block_join_parallel_copy_source_status_name(
    PreparedCurrentBlockJoinParallelCopySourceStatus status) {
  switch (status) {
    case PreparedCurrentBlockJoinParallelCopySourceStatus::Available:
      return "available";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingNames:
      return "missing_names";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations:
      return "missing_value_locations";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingEdgePublicationLookups:
      return "missing_edge_publication_lookups";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingBlock:
      return "missing_block";
    case PreparedCurrentBlockJoinParallelCopySourceStatus::MissingSuccessorLabel:
      return "missing_successor_label";
  }
  return "unknown";
}

enum class PreparedCurrentBlockEntryPublicationStatus {
  Available,
  MissingNames,
  MissingValueLocations,
  MissingSuccessorLabel,
  MissingDestinationValue,
  MissingDestinationHome,
  MissingPublication,
  PublicationUnavailable,
};

[[nodiscard]] constexpr std::string_view
prepared_current_block_entry_publication_status_name(
    PreparedCurrentBlockEntryPublicationStatus status) {
  switch (status) {
    case PreparedCurrentBlockEntryPublicationStatus::Available:
      return "available";
    case PreparedCurrentBlockEntryPublicationStatus::MissingNames:
      return "missing_names";
    case PreparedCurrentBlockEntryPublicationStatus::MissingValueLocations:
      return "missing_value_locations";
    case PreparedCurrentBlockEntryPublicationStatus::MissingSuccessorLabel:
      return "missing_successor_label";
    case PreparedCurrentBlockEntryPublicationStatus::MissingDestinationValue:
      return "missing_destination_value";
    case PreparedCurrentBlockEntryPublicationStatus::MissingDestinationHome:
      return "missing_destination_home";
    case PreparedCurrentBlockEntryPublicationStatus::MissingPublication:
      return "missing_publication";
    case PreparedCurrentBlockEntryPublicationStatus::PublicationUnavailable:
      return "publication_unavailable";
  }
  return "unknown";
}

struct PreparedSameBlockScalarProducer {
  PreparedEdgePublicationSourceProducer producer;
  const bir::Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  ValueNameId value_name = kInvalidValueName;
};

struct PreparedSameBlockValueMaterializationQuery {
  const PreparedNameTables* names = nullptr;
  const PreparedEdgePublicationSourceProducerLookups* source_producers = nullptr;
  BlockLabelId block_label = kInvalidBlockLabel;
  const bir::Block* block = nullptr;
  std::size_t before_instruction_index = 0;
};

struct PreparedCurrentBlockPublicationConsumption {
  bool available = false;
  const PreparedEdgePublicationSourceProducer* source_producer = nullptr;
  const bir::Inst* instruction = nullptr;
  const bir::Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  ValueNameId value_name = kInvalidValueName;
  PreparedEdgePublicationSourceProducerKind source_producer_kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
};

struct PreparedCallArgumentSourceProducerMaterialization {
  PreparedSameBlockScalarProducer producer;
  bool materializable = false;
};

struct PreparedFusedCompareOperandProducer {
  PreparedEdgePublicationSourceProducerKind kind =
      PreparedEdgePublicationSourceProducerKind::Unknown;
  const bir::Inst* instruction = nullptr;
  std::size_t instruction_index = 0;
  ValueNameId value_name = kInvalidValueName;
  const bir::CastInst* cast = nullptr;
  const bir::LoadLocalInst* load_local = nullptr;
  const bir::LoadGlobalInst* load_global = nullptr;
  const bir::BinaryInst* binary = nullptr;
  const bir::SelectInst* select = nullptr;
  std::optional<std::int64_t> integer_constant;
};

struct PreparedFusedCompareOperandProducerFacts {
  std::optional<PreparedFusedCompareOperandProducer> lhs;
  std::optional<PreparedFusedCompareOperandProducer> rhs;
};

struct PreparedMaterializedConditionProducer {
  const bir::BinaryInst* binary = nullptr;
  std::size_t instruction_index = 0;
  ValueNameId condition_value_name = kInvalidValueName;
};

struct PreparedSameBlockLoadLocalStoredValueSource {
  bir::Value stored_value;
  std::size_t store_instruction_index = 0;
  const PreparedEdgePublicationSourceProducer* load_producer = nullptr;
  const PreparedMemoryAccess* load_access = nullptr;
  const PreparedMemoryAccess* store_access = nullptr;
};

// AArch64 current-block join routing convenience. These declarations stay in
// this shared header while their definitions remain in prepared_lookups.cpp.
struct PreparedCurrentBlockJoinParallelCopySourceFact {
  PreparedEdgeCopySourceFactsStatus status =
      PreparedEdgeCopySourceFactsStatus::MissingPublication;
  const PreparedMoveBundle* bundle = nullptr;
  const PreparedMoveResolution* move = nullptr;
  const PreparedEdgePublication* publication = nullptr;
  BlockLabelId predecessor_label = kInvalidBlockLabel;
  BlockLabelId successor_label = kInvalidBlockLabel;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
  std::optional<PreparedValueId> source_value_id;
  ValueNameId source_value_name = kInvalidValueName;
  const PreparedValueHome* source_home = nullptr;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueHomeKind source_home_kind = PreparedValueHomeKind::None;
  PreparedValueHomeKind destination_home_kind = PreparedValueHomeKind::None;
  PreparedMoveStorageKind destination_storage_kind = PreparedMoveStorageKind::None;
  std::optional<std::string> destination_register_name;
  bool source_is_incoming_expression = false;
  bool destination_is_source_value = false;
  bool source_is_source_value = false;
  bool source_shares_destination_register = false;
  bool source_home_is_stack = false;
  bool immediate_source = false;
};

struct PreparedCurrentBlockJoinParallelCopySourceFacts {
  PreparedCurrentBlockJoinParallelCopySourceStatus status =
      PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations;
  std::vector<PreparedCurrentBlockJoinParallelCopySourceFact> facts;
  std::vector<PreparedValueId> incoming_expression_value_ids;
  std::vector<ValueNameId> incoming_expression_value_names;
  std::vector<PreparedValueId> source_value_ids;
  std::vector<ValueNameId> source_value_names;
};

struct PreparedCurrentBlockJoinParallelCopySourceQueryInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedRegallocFunction* regalloc = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
  const PreparedEdgePublicationLookups* edge_publications = nullptr;
  const bir::Block* block = nullptr;
  BlockLabelId successor_label = kInvalidBlockLabel;
};

struct PreparedCurrentBlockEntryPublicationQueryInputs {
  const PreparedNameTables* names = nullptr;
  const PreparedRegallocFunction* regalloc = nullptr;
  const PreparedValueLocationFunction* value_locations = nullptr;
  const PreparedValueHomeLookups* value_home_lookups = nullptr;
  BlockLabelId successor_label = kInvalidBlockLabel;
};

struct PreparedCurrentBlockEntryPublication {
  PreparedCurrentBlockEntryPublicationStatus status =
      PreparedCurrentBlockEntryPublicationStatus::MissingPublication;
  PreparedBlockEntryPublication publication;
  const PreparedValueHome* destination_home = nullptr;
  PreparedValueId destination_value_id = 0;
  ValueNameId destination_value_name = kInvalidValueName;
};

struct PreparedCurrentBlockJoinParallelCopyInstructionRouting {
  PreparedCurrentBlockJoinParallelCopySourceStatus status =
      PreparedCurrentBlockJoinParallelCopySourceStatus::MissingValueLocations;
  std::vector<bool> incoming_expression_instruction_results;
  std::vector<bool> source_instruction_results;
};

struct PreparedFunctionLookups {
  PreparedCallPlanLookups call_plans;
  PreparedAddressMaterializationLookups address_materializations;
  PreparedMemoryAccessLookups memory_accesses;
  PreparedMoveBundleLookups move_bundles;
  PreparedReturnChainLookups return_chains;
  PreparedValueHomeLookups value_homes;
  PreparedEdgePublicationLookups edge_publications;
  PreparedEdgePublicationSourceProducerLookups edge_publication_source_producers;
};

[[nodiscard]] std::size_t prepared_move_bundle_position_key(PreparedMovePhase phase,
                                                            std::size_t block_index,
                                                            std::size_t instruction_index);
[[nodiscard]] std::size_t prepared_after_call_result_lane_position_key(
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id);
[[nodiscard]] std::size_t prepared_before_return_abi_move_source_bank_key(
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank);
[[nodiscard]] std::size_t prepared_return_chain_value_key(std::size_t block_index,
                                                          std::size_t instruction_index,
                                                          ValueNameId value_name);

[[nodiscard]] PreparedMoveBundleLookups make_prepared_move_bundle_lookups(
    const PreparedValueLocationFunction* value_locations);

[[nodiscard]] PreparedReturnChainLookups make_prepared_return_chain_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] PreparedValueHomeLookups make_prepared_value_home_lookups(
    const PreparedValueLocationFunction* value_locations);

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedNameTables& names,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedEdgePublicationLookups make_prepared_edge_publication_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function,
    const PreparedValueLocationFunction* value_locations,
    const PreparedValueHomeLookups* value_home_lookups = nullptr);

[[nodiscard]] PreparedEdgePublicationSourceProducerLookups
make_prepared_edge_publication_source_producer_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] PreparedFunctionLookups make_prepared_function_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

[[nodiscard]] bool prepared_value_homes_share_register_name(
    const PreparedValueHome& lhs,
    const PreparedValueHome& rhs);

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_register_destination_matches_value(
    const PreparedMoveResolution& move,
    PreparedValueId value_id);

[[nodiscard]] bool
prepared_out_of_ssa_parallel_copy_source_shares_destination_register(
    const PreparedMoveResolution& move,
    const PreparedValueHome& source_home,
    const PreparedValueHome& destination_home);

[[nodiscard]] PreparedCurrentBlockEntryPublication
find_prepared_current_block_entry_publication(
    const PreparedCurrentBlockEntryPublicationQueryInputs& query,
    PreparedValueId destination_value_id);

[[nodiscard]] PreparedCurrentBlockEntryPublication
find_prepared_current_block_entry_publication(
    const PreparedCurrentBlockEntryPublicationQueryInputs& query,
    const bir::Value& destination_value);

[[nodiscard]] const PreparedMoveBundle* find_indexed_prepared_move_bundle(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    PreparedMovePhase phase,
    std::size_t block_index,
    std::size_t instruction_index);

[[nodiscard]] const PreparedMoveResolution*
find_indexed_prepared_before_call_argument_move(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t abi_index);

[[nodiscard]] const PreparedAfterCallResultLaneBinding*
find_indexed_prepared_after_call_result_lane_binding(
    const PreparedMoveBundleLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    PreparedValueId value_id);

[[nodiscard]] const PreparedMoveResolution*
find_prepared_before_return_abi_move_by_source_and_destination_bank(
    const PreparedMoveBundleLookups* lookups,
    const PreparedValueLocationFunction* value_locations,
    std::size_t block_index,
    PreparedValueId source_value_id,
    PreparedRegisterBank destination_bank);

[[nodiscard]] ValueNameId find_prepared_return_chain_terminal_value(
    const PreparedReturnChainLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name);

[[nodiscard]] ValueNameId find_prepared_return_chain_next_operand_value(
    const PreparedReturnChainLookups* lookups,
    std::size_t block_index,
    std::size_t instruction_index,
    ValueNameId value_name);

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
find_indexed_prepared_edge_publication_source_producer(
    const PreparedEdgePublicationSourceProducerLookups* lookups,
    ValueNameId value_name);

[[nodiscard]] PreparedCurrentBlockPublicationConsumption
find_prepared_current_block_publication_consumption(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    ValueNameId value_name,
    bir::TypeKind value_type,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedSameBlockScalarProducer>
find_prepared_same_block_scalar_producer(
    const PreparedSameBlockValueMaterializationQuery& query,
    const bir::Value& value);

[[nodiscard]] bool prepared_call_argument_binary_producer_opcode_is_materializable(
    bir::BinaryOpcode opcode);

[[nodiscard]] std::optional<PreparedCallArgumentSourceProducerMaterialization>
find_prepared_call_argument_source_producer_materialization(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<std::int64_t>
evaluate_prepared_same_block_integer_constant(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<std::int64_t>
evaluate_prepared_same_block_integer_constant(
    const PreparedSameBlockValueMaterializationQuery& query,
    const bir::Value& value);

[[nodiscard]] std::optional<PreparedFusedCompareOperandProducer>
find_prepared_fused_compare_operand_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedFusedCompareOperandProducerFacts>
find_prepared_fused_compare_operand_producer_facts(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const PreparedBranchCondition& branch_condition,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedMaterializedConditionProducer>
find_prepared_materialized_condition_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& condition_value,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<PreparedSameBlockLoadLocalStoredValueSource>
find_prepared_same_block_load_local_stored_value_source(
    const PreparedNameTables& names,
    const PreparedStackLayout& stack_layout,
    const PreparedAddressingFunction* addressing,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] PreparedCurrentBlockJoinParallelCopySourceFacts
prepare_current_block_join_parallel_copy_source_facts(
    const PreparedCurrentBlockJoinParallelCopySourceQueryInputs& inputs);

[[nodiscard]] PreparedCurrentBlockJoinParallelCopyInstructionRouting
prepare_current_block_join_parallel_copy_instruction_routing(
    const PreparedCurrentBlockJoinParallelCopySourceQueryInputs& inputs);

}  // namespace c4c::backend::prepare

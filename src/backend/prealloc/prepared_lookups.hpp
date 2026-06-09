#pragma once

#include "addressing.hpp"
#include "calls.hpp"
#include "control_flow.hpp"
#include "publication_plans.hpp"
#include "select_chain_lookups.hpp"
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

struct PreparedReturnChainLookups {
  std::unordered_map<std::size_t, ValueNameId> terminal_return_values_by_chain_value;
  std::unordered_map<std::size_t, ValueNameId> next_operand_values_by_chain_value;
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

[[nodiscard]] std::size_t prepared_return_chain_value_key(std::size_t block_index,
                                                          std::size_t instruction_index,
                                                          ValueNameId value_name);

[[nodiscard]] PreparedReturnChainLookups make_prepared_return_chain_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

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

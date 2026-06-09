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

struct PreparedSameBlockLoadLocalStoredValueSource {
  bir::Value stored_value;
  std::size_t store_instruction_index = 0;
  const PreparedEdgePublicationSourceProducer* load_producer = nullptr;
  const PreparedMemoryAccess* load_access = nullptr;
  const PreparedMemoryAccess* store_access = nullptr;
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

}  // namespace c4c::backend::prepare

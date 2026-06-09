#pragma once

#include "calls.hpp"
#include "names.hpp"

#include <cstddef>
#include <optional>

namespace c4c::backend::bir {
struct Block;
struct Value;
}  // namespace c4c::backend::bir

namespace c4c::backend::prepare {

struct PreparedEdgePublicationSourceProducer;
struct PreparedEdgePublicationSourceProducerLookups;

struct PreparedSelectChainDependencyQuery {
  const PreparedNameTables* names = nullptr;
  const PreparedEdgePublicationSourceProducerLookups* source_producers = nullptr;
  BlockLabelId block_label = kInvalidBlockLabel;
  const bir::Block* block = nullptr;
  std::size_t before_instruction_index = 0;
};

using PreparedStoreSourceDirectGlobalSelectChainDependency =
    PreparedDirectGlobalSelectChainDependency;

struct PreparedScalarSelectChainMaterialization {
  bool available = false;
  ValueNameId root_value_name = kInvalidValueName;
  bool root_is_select = false;
  std::optional<std::size_t> root_instruction_index;
  PreparedDirectGlobalSelectChainDependency direct_global_dependency;
};

[[nodiscard]] PreparedDirectGlobalSelectChainDependency
find_prepared_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] PreparedDirectGlobalSelectChainDependency
find_prepared_direct_global_select_chain_dependency(
    const PreparedSelectChainDependencyQuery& query,
    const bir::Value& value);

[[nodiscard]] const PreparedEdgePublicationSourceProducer*
find_prepared_select_chain_source_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] PreparedStoreSourceDirectGlobalSelectChainDependency
find_prepared_store_source_direct_global_select_chain_dependency(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] PreparedScalarSelectChainMaterialization
find_prepared_scalar_select_chain_materialization(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] PreparedScalarSelectChainMaterialization
find_prepared_scalar_select_chain_materialization(
    const PreparedSelectChainDependencyQuery& query,
    const bir::Value& value);

}  // namespace c4c::backend::prepare

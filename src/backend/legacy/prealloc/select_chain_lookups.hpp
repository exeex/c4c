#pragma once

#include "calls.hpp"
#include "names.hpp"
#include "publication_plans.hpp"

#include <cstdint>
#include <cstddef>
#include <optional>

namespace c4c::backend::bir {
struct Block;
struct Value;
}  // namespace c4c::backend::bir

namespace c4c::backend::prepare {

struct PreparedBirModule;
struct PreparedControlFlowFunction;

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

[[nodiscard]] PreparedEdgePublicationSourceProducerLookups
make_prepared_edge_publication_source_producer_lookups(
    const PreparedBirModule& prepared,
    const PreparedControlFlowFunction& function);

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

}  // namespace c4c::backend::prepare

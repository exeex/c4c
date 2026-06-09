#pragma once

#include "control_flow.hpp"
#include "names.hpp"
#include "publication_plans.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace c4c::backend::prepare {

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

}  // namespace c4c::backend::prepare

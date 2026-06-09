#include "comparison.hpp"

#include "select_chain_lookups.hpp"

namespace c4c::backend::prepare {
namespace {

[[nodiscard]] std::optional<ValueNameId> existing_prepared_value_name_id(
    const PreparedNameTables& names,
    const bir::Value& value) {
  if (value.kind != bir::Value::Kind::Named || value.name.empty()) {
    return std::nullopt;
  }
  const auto value_name = names.value_names.find(value.name);
  if (value_name == kInvalidValueName) {
    return std::nullopt;
  }
  return value_name;
}

}  // namespace

std::optional<PreparedFusedCompareOperandProducer>
find_prepared_fused_compare_operand_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index) {
  if (value.kind == bir::Value::Kind::Immediate) {
    return PreparedFusedCompareOperandProducer{
        .kind = PreparedEdgePublicationSourceProducerKind::Immediate,
        .integer_constant = value.immediate,
    };
  }
  const auto value_name = existing_prepared_value_name_id(names, value);
  if (!value_name.has_value()) {
    return std::nullopt;
  }
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              *value_name,
                                              value.type,
                                              before_instruction_index);
  if (!producer.has_value()) {
    return std::nullopt;
  }
  PreparedFusedCompareOperandProducer result{
      .kind = producer->producer.kind,
      .instruction = producer->instruction,
      .instruction_index = producer->instruction_index,
      .value_name = *value_name,
      .cast = producer->producer.cast,
      .load_local = producer->producer.load_local,
      .load_global = producer->producer.load_global,
      .binary = producer->producer.binary,
      .select = producer->producer.select,
  };
  result.integer_constant =
      evaluate_prepared_same_block_integer_constant(names,
                                                    source_producers,
                                                    block_label,
                                                    block,
                                                    value,
                                                    before_instruction_index);
  return result;
}

std::optional<PreparedFusedCompareOperandProducerFacts>
find_prepared_fused_compare_operand_producer_facts(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const PreparedBranchCondition& branch_condition,
    std::size_t before_instruction_index) {
  if (branch_condition.kind != PreparedBranchConditionKind::FusedCompare ||
      !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value()) {
    return std::nullopt;
  }

  PreparedFusedCompareOperandProducerFacts facts{
      .lhs = find_prepared_fused_compare_operand_producer(names,
                                                          source_producers,
                                                          block_label,
                                                          block,
                                                          *branch_condition.lhs,
                                                          before_instruction_index),
      .rhs = find_prepared_fused_compare_operand_producer(names,
                                                          source_producers,
                                                          block_label,
                                                          block,
                                                          *branch_condition.rhs,
                                                          before_instruction_index),
  };
  if (!facts.lhs.has_value() && !facts.rhs.has_value()) {
    return std::nullopt;
  }
  return facts;
}

std::optional<PreparedMaterializedConditionProducer>
find_prepared_materialized_condition_producer(
    const PreparedNameTables& names,
    const PreparedEdgePublicationSourceProducerLookups* source_producers,
    BlockLabelId block_label,
    const bir::Block* block,
    const bir::Value& condition_value,
    std::size_t before_instruction_index) {
  if (condition_value.kind != bir::Value::Kind::Named) {
    return std::nullopt;
  }
  const auto condition_value_name =
      existing_prepared_value_name_id(names, condition_value);
  if (!condition_value_name.has_value()) {
    return std::nullopt;
  }
  const auto producer =
      find_prepared_same_block_scalar_producer(names,
                                              source_producers,
                                              block_label,
                                              block,
                                              *condition_value_name,
                                              condition_value.type,
                                              before_instruction_index);
  if (!producer.has_value() ||
      producer->producer.kind != PreparedEdgePublicationSourceProducerKind::Binary ||
      producer->producer.binary == nullptr) {
    return std::nullopt;
  }
  return PreparedMaterializedConditionProducer{
      .binary = producer->producer.binary,
      .instruction_index = producer->instruction_index,
      .condition_value_name = *condition_value_name,
  };
}

}  // namespace c4c::backend::prepare

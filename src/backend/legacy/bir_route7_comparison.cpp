#include "bir.hpp"
#include "bir_private.hpp"

namespace c4c::backend::bir {

Route7ComparisonOperandRecord route7_comparison_operand_record(
    const Block* block,
    const Value& value,
    std::size_t before_instruction_index,
    Route7ComparisonOperandRole role) {
  Route7ComparisonOperandRecord record{
      .status = Route7ComparisonStatus::Unavailable,
      .role = role,
      .before_instruction_index = before_instruction_index,
      .value = route1_source_value_identity(value),
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (value.kind == Value::Kind::Immediate) {
    record.available = true;
    record.status = Route7ComparisonStatus::Available;
    record.producer_kind = ComparisonProducerKind::Immediate;
    record.integer_constant = value.immediate;
    return record;
  }
  if (value.kind != Value::Kind::Named || value.name.empty()) {
    record.status = Route7ComparisonStatus::NoMatch;
    return record;
  }
  const auto producer =
      find_unique_comparison_producer(*block, value, before_instruction_index);
  if (producer.ambiguous) {
    record.status = Route7ComparisonStatus::DuplicateProducer;
    return record;
  }
  if (producer.inst == nullptr || producer.produced_value == nullptr) {
    record.status = Route7ComparisonStatus::MissingOperandProducer;
    return record;
  }
  const auto kind = comparison_producer_kind_for_inst(*producer.inst);
  if (kind == ComparisonProducerKind::Unknown) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  record.producer_kind = kind;
  record.producer_instruction = producer.inst;
  record.producer_instruction_index = producer.instruction_index;
  record.produced_value = route1_source_value_identity(*producer.produced_value);
  record.integer_constant = evaluate_comparison_integer_constant(
      *block, value, before_instruction_index);
  return record;
}

Route7ComparisonInstructionRecord route7_comparison_instruction_record(
    const Block* block,
    std::size_t instruction_index) {
  Route7ComparisonInstructionRecord record{
      .status = Route7ComparisonStatus::Unavailable,
      .instruction_index = instruction_index,
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (instruction_index >= block->insts.size()) {
    record.status = Route7ComparisonStatus::MissingInstruction;
    return record;
  }
  record.instruction = &block->insts[instruction_index];
  const auto* binary = std::get_if<BinaryInst>(record.instruction);
  if (binary == nullptr) {
    record.status = Route7ComparisonStatus::WrongInstruction;
    return record;
  }
  record.binary = binary;
  record.condition_value = route1_source_value_identity(binary->result);
  record.predicate = binary->opcode;
  record.compare_type = binary_operand_type(*binary);
  record.lhs_value = route1_source_value_identity(binary->lhs);
  record.rhs_value = route1_source_value_identity(binary->rhs);
  if (!is_comparison_binary_opcode(binary->opcode)) {
    record.status = Route7ComparisonStatus::NonComparison;
    return record;
  }
  record.lhs = route7_comparison_operand_record(
      block, binary->lhs, instruction_index, Route7ComparisonOperandRole::Lhs);
  record.rhs = route7_comparison_operand_record(
      block, binary->rhs, instruction_index, Route7ComparisonOperandRole::Rhs);
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  return record;
}

Route7BranchConditionRecord route7_branch_condition_record(const Block* block) {
  Route7BranchConditionRecord record{
      .status = Route7ComparisonStatus::Unavailable,
  };
  if (block == nullptr) {
    record.status = Route7ComparisonStatus::MissingBlock;
    return record;
  }
  record.block_label = block->label;
  record.block_label_id = block->label_id;
  if (block->terminator.kind != TerminatorKind::CondBranch) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  record.condition_value =
      route1_source_value_identity(block->terminator.condition);
  record.true_label = block->terminator.true_label;
  record.true_label_id = block->terminator.true_label_id;
  record.false_label = block->terminator.false_label;
  record.false_label_id = block->terminator.false_label_id;
  if (block->terminator.condition.kind != Value::Kind::Named ||
      block->terminator.condition.name.empty()) {
    record.status = Route7ComparisonStatus::MissingConditionValue;
    return record;
  }
  const auto producer = find_unique_comparison_producer(
      *block, block->terminator.condition, block->insts.size());
  if (producer.ambiguous) {
    record.status = Route7ComparisonStatus::DuplicateProducer;
    return record;
  }
  if (producer.inst == nullptr) {
    record.status = Route7ComparisonStatus::AbsentProvenance;
    return record;
  }
  const auto* binary = std::get_if<BinaryInst>(producer.inst);
  if (binary == nullptr || !is_comparison_binary_opcode(binary->opcode)) {
    record.status = Route7ComparisonStatus::NonComparison;
    return record;
  }
  record.comparison =
      route7_comparison_instruction_record(block, producer.instruction_index);
  if (!record.comparison) {
    record.status = record.comparison.status;
    return record;
  }
  record.available = true;
  record.status = Route7ComparisonStatus::Available;
  record.kind = Route7BranchConditionKind::FusedCompare;
  return record;
}

Route7ComparisonConditionIndex route7_build_comparison_condition_index(
    const Function& function) {
  Route7ComparisonConditionIndex index{
      .function = &function,
  };
  for (const auto& block : function.blocks) {
    index.comparison_records.reserve(index.comparison_records.size() +
                                     block.insts.size());
    for (std::size_t instruction_index = 0;
         instruction_index < block.insts.size();
         ++instruction_index) {
      auto record = route7_comparison_instruction_record(
          &block, instruction_index);
      index.comparison_records.push_back(record);
      if (record.status == Route7ComparisonStatus::Available ||
          record.status == Route7ComparisonStatus::NonComparison) {
        if (record.lhs.status != Route7ComparisonStatus::Unavailable) {
          index.operand_records.push_back(record.lhs);
        }
        if (record.rhs.status != Route7ComparisonStatus::Unavailable) {
          index.operand_records.push_back(record.rhs);
        }
      }
    }
    const auto branch_record = route7_branch_condition_record(&block);
    if (branch_record.status != Route7ComparisonStatus::Unavailable) {
      index.branch_condition_records.push_back(branch_record);
    }
  }
  return index;
}

Route7ComparisonConditionIndex route7_build_comparison_condition_index(
    const Block& block) {
  Route7ComparisonConditionIndex index{
      .block = &block,
  };
  index.comparison_records.reserve(block.insts.size());
  for (std::size_t instruction_index = 0; instruction_index < block.insts.size();
       ++instruction_index) {
    auto record =
        route7_comparison_instruction_record(&block, instruction_index);
    index.comparison_records.push_back(record);
    if (record.status == Route7ComparisonStatus::Available ||
        record.status == Route7ComparisonStatus::NonComparison) {
      if (record.lhs.status != Route7ComparisonStatus::Unavailable) {
        index.operand_records.push_back(record.lhs);
      }
      if (record.rhs.status != Route7ComparisonStatus::Unavailable) {
        index.operand_records.push_back(record.rhs);
      }
    }
  }
  const auto branch_record = route7_branch_condition_record(&block);
  if (branch_record.status != Route7ComparisonStatus::Unavailable) {
    index.branch_condition_records.push_back(branch_record);
  }
  return index;
}

}  // namespace c4c::backend::bir

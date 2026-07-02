#pragma once

#include "bir.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

namespace c4c::backend::bir {

[[nodiscard]] const Value* produced_value_for_comparison_producer(
    const Inst& inst);
[[nodiscard]] ComparisonProducerKind comparison_producer_kind_for_inst(
    const Inst& inst);
[[nodiscard]] bool is_comparison_binary_opcode(BinaryOpcode opcode);

struct SameBlockComparisonProducer {
  const Inst* inst = nullptr;
  const Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  bool ambiguous = false;
};

[[nodiscard]] SameBlockComparisonProducer find_unique_comparison_producer(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index);
[[nodiscard]] std::optional<std::int64_t> evaluate_comparison_integer_constant(
    const Block& block,
    const Value& value,
    std::size_t before_instruction_index,
    unsigned depth = 0);
[[nodiscard]] const CallInst* indexed_call_inst(
    const Block& block,
    const CallInst& call,
    std::size_t call_instruction_index);

[[nodiscard]] inline bool route_block_matches(std::string_view record_label,
                                              BlockLabelId record_label_id,
                                              const Block& block) {
  if (record_label_id != kInvalidBlockLabel ||
      block.label_id != kInvalidBlockLabel) {
    return record_label_id == block.label_id;
  }
  return record_label == block.label;
}

}  // namespace c4c::backend::bir

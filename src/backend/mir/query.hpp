#pragma once

#include "../bir/bir.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace c4c::backend::mir {

namespace bir = c4c::backend::bir;

enum class SameBlockProducerKind {
  Unknown,
  Binary,
  Cast,
  Select,
  LoadLocal,
  LoadGlobal,
};

struct SameBlockProducerRecord {
  const bir::Inst* inst = nullptr;
  std::size_t instruction_index = 0;
  SameBlockProducerKind kind = SameBlockProducerKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return inst != nullptr; }
};

struct SameBlockProducerIndex {
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return producer != nullptr; }
};

struct SameBlockBinaryProducer {
  const bir::BinaryInst* binary = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return binary != nullptr; }
};

struct SameBlockSelectProducer {
  const bir::SelectInst* select = nullptr;
  std::size_t instruction_index = 0;

  [[nodiscard]] explicit operator bool() const { return select != nullptr; }
};

struct SameBlockIntegerConstant {
  std::int64_t value = 0;
  unsigned depth = 0;
};

struct DependencyTraversalRecord {
  const bir::Inst* producer = nullptr;
  std::size_t instruction_index = 0;
  const bir::Value* source_value = nullptr;
  unsigned depth = 0;
  SameBlockProducerKind kind = SameBlockProducerKind::Unknown;

  [[nodiscard]] explicit operator bool() const { return producer != nullptr; }
};

using DependencyPredicate = bool (*)(const DependencyTraversalRecord& record);

[[nodiscard]] SameBlockProducerKind same_block_producer_kind(const bir::Inst& inst);

[[nodiscard]] SameBlockBinaryProducer find_same_block_binary_producer(
    const bir::Block* block,
    const bir::Value& value);

[[nodiscard]] SameBlockSelectProducer find_same_block_select_producer(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index);

[[nodiscard]] SameBlockProducerRecord find_same_block_named_producer_record(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index);

[[nodiscard]] const bir::Inst* find_same_block_named_producer(
    const bir::Block* block,
    std::string_view value_name,
    std::size_t before_instruction_index);

[[nodiscard]] std::optional<SameBlockProducerIndex> producer_instruction_index_record(
    const bir::Block* block,
    const bir::Inst* producer);

[[nodiscard]] std::optional<std::size_t> producer_instruction_index(
    const bir::Block* block,
    const bir::Inst* producer);

[[nodiscard]] std::optional<SameBlockIntegerConstant>
evaluate_same_block_integer_constant(
    const bir::Block* block,
    const bir::Value& value,
    unsigned depth = 0);

[[nodiscard]] bool select_chain_contains_dependency(
    const bir::Block* block,
    const bir::Value& value,
    std::size_t before_instruction_index,
    DependencyPredicate matches,
    unsigned depth = 0);

}  // namespace c4c::backend::mir

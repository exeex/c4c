#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct Value;

enum class BirViewStatus : unsigned char {
  Available,
  Unavailable,
  Incomplete,
  Ambiguous,
};

enum class BirProducerKind : unsigned char {
  Unknown,
  LoadLocal,
  LoadGlobal,
  Cast,
  Binary,
  SelectMaterialization,
};

struct BirProducerResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  BirProducerKind kind = BirProducerKind::Unknown;
  const Value* produced_value = nullptr;
  std::size_t instruction_index = 0;
  std::string_view block_label;
  bool scalar_materialization_available = false;
  std::optional<std::int64_t> immediate_integer_constant;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirProducerView {
 public:
  BirProducerView() = default;

  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirProducerView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}

  std::shared_ptr<const Implementation> implementation_;

  friend BirProducerView make_bir_producer_view(const Block& block);
  friend BirProducerResult find_same_block_producer(
      const BirProducerView& view,
      const Value& value,
      std::size_t before_instruction_index);
};

[[nodiscard]] BirProducerView make_bir_producer_view(const Block& block);

[[nodiscard]] BirProducerResult find_same_block_producer(
    const BirProducerView& view,
    const Value& value,
    std::size_t before_instruction_index);

}  // namespace c4c::backend::bir

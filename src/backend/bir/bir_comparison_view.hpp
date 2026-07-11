#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace c4c::backend::bir {

enum class BinaryOpcode : unsigned char;
struct BinaryInst;
struct Block;
struct Value;

struct BirComparisonResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  const BinaryInst* comparison = nullptr;
  BinaryOpcode predicate;
  const Value* left_operand = nullptr;
  const Value* right_operand = nullptr;
  const Value* materialized_condition = nullptr;
  bool used_by_branch = false;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirComparisonView {
 public:
  BirComparisonView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirComparisonView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirComparisonView make_bir_comparison_view(const Block&);
  friend BirComparisonResult find_comparison(
      const BirComparisonView&, std::size_t);
};

[[nodiscard]] BirComparisonView make_bir_comparison_view(const Block& block);
[[nodiscard]] BirComparisonResult find_comparison(
    const BirComparisonView& view, std::size_t instruction_number);

}  // namespace c4c::backend::bir

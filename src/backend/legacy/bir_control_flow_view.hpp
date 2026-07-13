#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct Function;
struct PhiInst;
struct SelectInst;
struct Value;

struct BirControlFlowResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  const Block* block = nullptr;
  const Block* related_block = nullptr;
  const SelectInst* selection = nullptr;
  const PhiInst* join = nullptr;
  const Value* condition = nullptr;
  const Value* true_value = nullptr;
  const Value* false_value = nullptr;
  const Value* join_source = nullptr;
  std::string_view relationship_name;
  std::size_t instruction_number = 0;
  bool complete = false;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirControlFlowView {
 public:
  BirControlFlowView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirControlFlowView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirControlFlowView make_bir_control_flow_view(const Function&);
  friend BirControlFlowResult find_selection(
      const BirControlFlowView&, const Block&, std::size_t);
  friend BirControlFlowResult find_branch_condition(
      const BirControlFlowView&, const Block&);
  friend BirControlFlowResult find_block_relationship(
      const BirControlFlowView&, const Block&, const Block&);
  friend BirControlFlowResult find_join_source(
      const BirControlFlowView&, const Block&, std::size_t, const Block&);
};

[[nodiscard]] BirControlFlowView make_bir_control_flow_view(const Function& function);
[[nodiscard]] BirControlFlowResult find_selection(
    const BirControlFlowView& view, const Block& block, std::size_t instruction_number);
[[nodiscard]] BirControlFlowResult find_branch_condition(
    const BirControlFlowView& view, const Block& block);
[[nodiscard]] BirControlFlowResult find_block_relationship(
    const BirControlFlowView& view, const Block& from, const Block& to);
[[nodiscard]] BirControlFlowResult find_join_source(
    const BirControlFlowView& view,
    const Block& block,
    std::size_t instruction_number,
    const Block& from);

}  // namespace c4c::backend::bir

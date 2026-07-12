#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct CallInst;
struct Value;

struct BirCallBoundaryResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  const CallInst* call = nullptr;
  std::string_view callee;
  const Value* callee_value = nullptr;
  const Value* argument = nullptr;
  std::string_view dependency_name;
  bool has_argument_source_relationship = false;
  const Value* result = nullptr;
  std::size_t argument_number = 0;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirCallBoundaryView {
 public:
  BirCallBoundaryView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirCallBoundaryView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirCallBoundaryView make_bir_call_boundary_view(const Block&);
  friend BirCallBoundaryResult find_call(const BirCallBoundaryView&, std::size_t);
  friend BirCallBoundaryResult find_call_argument(
      const BirCallBoundaryView&, std::size_t, std::size_t);
};

[[nodiscard]] BirCallBoundaryView make_bir_call_boundary_view(const Block& block);
[[nodiscard]] BirCallBoundaryResult find_call(
    const BirCallBoundaryView& view, std::size_t instruction_number);
[[nodiscard]] BirCallBoundaryResult find_call_argument(
    const BirCallBoundaryView& view,
    std::size_t instruction_number,
    std::size_t argument_number);

}  // namespace c4c::backend::bir

#pragma once

#include "bir_producer_view.hpp"

#include <cstddef>
#include <memory>
#include <utility>

namespace c4c::backend::bir {

struct Block;
struct Terminator;
struct Value;

struct BirReturnResult {
  BirViewStatus status = BirViewStatus::Unavailable;
  const Terminator* return_identity = nullptr;
  const Value* returned_value = nullptr;
  const Value* provenance_value = nullptr;
  std::size_t lane_number = 0;
  std::size_t lane_count = 0;
  bool chain_complete = false;

  [[nodiscard]] explicit operator bool() const {
    return status == BirViewStatus::Available;
  }
};

class BirReturnView {
 public:
  BirReturnView() = default;
  [[nodiscard]] explicit operator bool() const { return implementation_ != nullptr; }

 private:
  struct Implementation;
  explicit BirReturnView(std::shared_ptr<const Implementation> implementation)
      : implementation_(std::move(implementation)) {}
  std::shared_ptr<const Implementation> implementation_;

  friend BirReturnView make_bir_return_view(const Block&);
  friend BirReturnResult find_return(const BirReturnView&);
  friend BirReturnResult find_return_lane(const BirReturnView&, std::size_t);
};

[[nodiscard]] BirReturnView make_bir_return_view(const Block& block);
[[nodiscard]] BirReturnResult find_return(const BirReturnView& view);
[[nodiscard]] BirReturnResult find_return_lane(
    const BirReturnView& view, std::size_t lane_number);

}  // namespace c4c::backend::bir

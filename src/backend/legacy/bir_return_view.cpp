#include "bir.hpp"
#include "bir_return_view.hpp"

namespace c4c::backend::bir {

struct BirReturnView::Implementation {
  const Block* block = nullptr;
};

namespace {
bool complete_value(const Value& value) {
  return value.kind == Value::Kind::Immediate || !value.name.empty();
}
}  // namespace

BirReturnView make_bir_return_view(const Block& block) {
  return BirReturnView(std::make_shared<BirReturnView::Implementation>(
      BirReturnView::Implementation{.block = &block}));
}

BirReturnResult find_return(const BirReturnView& view) {
  BirReturnResult result;
  if (!view.implementation_ || view.implementation_->block == nullptr) return result;
  const auto& terminator = view.implementation_->block->terminator;
  if (terminator.kind != TerminatorKind::Return) return result;
  result.return_identity = &terminator;
  result.lane_count = terminator.return_lanes.size();
  if (terminator.value && !terminator.return_lanes.empty()) {
    result.status = BirViewStatus::Ambiguous;
    return result;
  }
  if (terminator.value) {
    result.returned_value = &*terminator.value;
    result.provenance_value = result.returned_value;
    result.chain_complete = complete_value(*result.returned_value);
    result.status = result.chain_complete ? BirViewStatus::Available
                                          : BirViewStatus::Incomplete;
    return result;
  }
  if (!terminator.return_lanes.empty()) {
    result.status = BirViewStatus::Incomplete;
    return result;
  }
  result.chain_complete = true;
  result.status = BirViewStatus::Available;
  return result;
}

BirReturnResult find_return_lane(
    const BirReturnView& view, std::size_t lane_number) {
  auto result = find_return(view);
  if (result.status == BirViewStatus::Unavailable ||
      result.status == BirViewStatus::Ambiguous) return result;
  const auto& lanes = view.implementation_->block->terminator.return_lanes;
  if (lane_number >= lanes.size()) {
    result.status = BirViewStatus::Unavailable;
    return result;
  }
  result.lane_number = lane_number;
  result.returned_value = &lanes[lane_number];
  result.provenance_value = result.returned_value;
  result.chain_complete = complete_value(*result.returned_value);
  result.status = result.chain_complete ? BirViewStatus::Available
                                        : BirViewStatus::Incomplete;
  return result;
}

}  // namespace c4c::backend::bir

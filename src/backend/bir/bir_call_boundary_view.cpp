#include "bir.hpp"
#include "bir_call_boundary_view.hpp"

namespace c4c::backend::bir {

struct BirCallBoundaryView::Implementation {
  const Block* block = nullptr;
};

BirCallBoundaryView make_bir_call_boundary_view(const Block& block) {
  return BirCallBoundaryView(
      std::make_shared<BirCallBoundaryView::Implementation>(
          BirCallBoundaryView::Implementation{.block = &block}));
}

BirCallBoundaryResult find_call(
    const BirCallBoundaryView& view, std::size_t instruction_number) {
  BirCallBoundaryResult result;
  if (!view.implementation_ || view.implementation_->block == nullptr ||
      instruction_number >= view.implementation_->block->insts.size()) {
    return result;
  }
  const auto* call =
      std::get_if<CallInst>(&view.implementation_->block->insts[instruction_number]);
  if (call == nullptr) {
    return result;
  }
  result.call = call;
  result.callee = call->callee;
  result.callee_value = call->callee_value ? &*call->callee_value : nullptr;
  result.result = call->result ? &*call->result : nullptr;
  if (result.callee.empty() && result.callee_value == nullptr) {
    result.status = BirViewStatus::Incomplete;
    return result;
  }
  result.status = BirViewStatus::Available;
  return result;
}

BirCallBoundaryResult find_call_argument(
    const BirCallBoundaryView& view,
    std::size_t instruction_number,
    std::size_t argument_number) {
  auto result = find_call(view, instruction_number);
  if (!result) {
    return result;
  }
  if (argument_number >= result.call->args.size()) {
    result.status = BirViewStatus::Unavailable;
    return result;
  }
  result.argument_number = argument_number;
  result.argument = &result.call->args[argument_number];
  const CallArgumentSourceRelationship* relationship = nullptr;
  for (const auto& candidate : result.call->arg_sources) {
    if (candidate.arg_index != argument_number) {
      continue;
    }
    if (relationship != nullptr) {
      result.status = BirViewStatus::Ambiguous;
      return result;
    }
    relationship = &candidate;
  }
  if (relationship == nullptr) {
    result.status = BirViewStatus::Incomplete;
    return result;
  }
  if (relationship->source_value_name) {
    result.dependency_name = *relationship->source_value_name;
  } else if (relationship->source_base_value_name) {
    result.dependency_name = *relationship->source_base_value_name;
  } else if (relationship->direct_global_select_chain_dependency) {
    result.dependency_name =
        relationship->direct_global_select_chain_dependency->source_value_name;
  }
  return result;
}

}  // namespace c4c::backend::bir

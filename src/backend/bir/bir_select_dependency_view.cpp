#include "bir_select_dependency_view.hpp"

#include "bir.hpp"
#include "bir_producer_view.hpp"

#include <algorithm>
#include <variant>

namespace c4c::backend::bir {
namespace {

struct DependencyWalkResult {
  BirSelectDependencyStatus status = BirSelectDependencyStatus::Incomplete;
  const Value* dependency_value = nullptr;
  const LoadGlobalInst* dependency_load = nullptr;
  std::size_t dependency_instruction_index = 0;
};

BirSelectDependencyStatus walk_status(BirViewStatus status) {
  switch (status) {
    case BirViewStatus::Unavailable:
      return BirSelectDependencyStatus::Unavailable;
    case BirViewStatus::Incomplete:
      return BirSelectDependencyStatus::Incomplete;
    case BirViewStatus::Ambiguous:
      return BirSelectDependencyStatus::Ambiguous;
    case BirViewStatus::Available:
      break;
  }
  return BirSelectDependencyStatus::Incomplete;
}

DependencyWalkResult walk_dependency(const BirProducerView& view,
                                     const Block& block,
                                     const Value& value,
                                     std::size_t before_instruction_index,
                                     unsigned depth) {
  if (value.kind == Value::Kind::Immediate) {
    return {.status = BirSelectDependencyStatus::CompleteNoDependency};
  }
  if (depth > 64U || value.kind != Value::Kind::Named || value.name.empty()) {
    return {.status = BirSelectDependencyStatus::Incomplete};
  }

  const auto producer =
      find_same_block_producer(view, value, before_instruction_index);
  if (producer.status != BirViewStatus::Available) {
    return {.status = walk_status(producer.status)};
  }
  if (producer.produced_value == nullptr ||
      producer.instruction_index >= block.insts.size()) {
    return {.status = BirSelectDependencyStatus::Incomplete};
  }
  const auto& instruction = block.insts[producer.instruction_index];
  if (const auto* load = std::get_if<LoadGlobalInst>(&instruction)) {
    return {
        .status = BirSelectDependencyStatus::CompleteDirectGlobal,
        .dependency_value = &load->result,
        .dependency_load = load,
        .dependency_instruction_index = producer.instruction_index,
    };
  }

  const auto inspect = [&](const Value& operand) {
    return walk_dependency(view, block, operand, producer.instruction_index,
                           depth + 1U);
  };
  const auto inspect_pair = [&](const Value& first, const Value& second) {
    auto result = inspect(first);
    if (result.status != BirSelectDependencyStatus::CompleteNoDependency) {
      return result;
    }
    return inspect(second);
  };
  if (const auto* select = std::get_if<SelectInst>(&instruction)) {
    return inspect_pair(select->true_value, select->false_value);
  }
  if (const auto* cast = std::get_if<CastInst>(&instruction)) {
    return inspect(cast->operand);
  }
  if (const auto* binary = std::get_if<BinaryInst>(&instruction)) {
    return inspect_pair(binary->lhs, binary->rhs);
  }
  return {.status = BirSelectDependencyStatus::CompleteNoDependency};
}

}  // namespace

BirSelectDependencyResult find_bir_select_dependency(
    BirSelectDependencyRequest request) {
  BirSelectDependencyResult result{
      .block = request.block,
      .block_label = request.block != nullptr ? request.block->label
                                               : std::string_view{},
      .root_value = request.root_value,
      .before_instruction_index = request.before_instruction_index,
  };
  if (request.block == nullptr || request.root_value == nullptr) {
    result.status = BirSelectDependencyStatus::Unavailable;
    return result;
  }
  if (!request.block_label.empty() && request.block_label != request.block->label) {
    result.status = BirSelectDependencyStatus::Mismatched;
    return result;
  }
  if (request.root_value->kind != Value::Kind::Named ||
      request.root_value->name.empty()) {
    result.status = BirSelectDependencyStatus::Incomplete;
    return result;
  }

  const auto before =
      std::min(request.before_instruction_index, request.block->insts.size());
  const auto view = make_bir_producer_view(*request.block);
  const auto root = find_same_block_producer(view, *request.root_value, before);
  if (root.status != BirViewStatus::Available) {
    result.status = walk_status(root.status);
    return result;
  }
  if (root.produced_value == nullptr || root.instruction_index >= before ||
      root.instruction_index >= request.block->insts.size()) {
    result.status = BirSelectDependencyStatus::Incomplete;
    return result;
  }
  if (root.produced_value->name != request.root_value->name ||
      root.produced_value->type != request.root_value->type) {
    result.status = BirSelectDependencyStatus::Mismatched;
    return result;
  }

  result.root_value = root.produced_value;
  result.root_instruction_index = root.instruction_index;
  const auto dependency = walk_dependency(
      view, *request.block, *root.produced_value, before, 0U);
  result.status = dependency.status;
  result.dependency_value = dependency.dependency_value;
  result.dependency_load = dependency.dependency_load;
  result.dependency_instruction_index = dependency.dependency_instruction_index;
  return result;
}

}  // namespace c4c::backend::bir

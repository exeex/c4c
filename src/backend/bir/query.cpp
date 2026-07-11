#include "query.hpp"

#include "bir.hpp"

#include <algorithm>
#include <variant>

namespace c4c::backend::bir {

BirSameBlockGlobalLoadResult find_same_block_global_load(
    BirSameBlockGlobalLoadRequest request) {
  if (!request) {
    return {.status = BirViewStatus::Incomplete};
  }

  const std::string_view value_name =
      request.value != nullptr ? request.value->name : request.value_name;
  const TypeKind value_type =
      request.value != nullptr ? request.value->type : request.value_type;
  if (value_name.empty()) {
    return {.status = BirViewStatus::Incomplete};
  }

  const auto view = make_bir_memory_access_view(*request.block);
  BirSameBlockGlobalLoadResult match;
  const auto limit =
      std::min(request.before_instruction_index, request.block->insts.size());
  for (std::size_t index = 0; index < limit; ++index) {
    const auto candidate = find_memory_access(view, index);
    if (candidate.status == BirViewStatus::Ambiguous) {
      return {.status = BirViewStatus::Ambiguous};
    }
    if (!candidate || candidate.result_value == nullptr ||
        candidate.result_value_name != value_name) {
      continue;
    }
    if (candidate.result_value->type != value_type) {
      continue;
    }
    if (candidate.kind != BirMemoryAccessKind::LoadGlobal ||
        candidate.base_kind != BirMemoryBaseKind::GlobalSymbol) {
      return {.status = BirViewStatus::Incomplete};
    }
    const auto* load = std::get_if<LoadGlobalInst>(candidate.instruction);
    if (load == nullptr) {
      return {.status = BirViewStatus::Incomplete};
    }
    if (match.status == BirViewStatus::Available) {
      return {.status = BirViewStatus::Ambiguous};
    }
    match = BirSameBlockGlobalLoadResult{
        .status = BirViewStatus::Available,
        .access = candidate,
        .load = load,
        .result_value = candidate.result_value,
    };
  }
  return match;
}

}  // namespace c4c::backend::bir

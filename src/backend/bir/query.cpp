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

namespace {

[[nodiscard]] bool same_local_slot(const BirMemoryAccessResult& lhs,
                                   const BirMemoryAccessResult& rhs) {
  if (!lhs || !rhs || lhs.base_kind != BirMemoryBaseKind::LocalSlot ||
      rhs.base_kind != BirMemoryBaseKind::LocalSlot) {
    return false;
  }
  if (lhs.local_slot_id != c4c::kInvalidSlotName &&
      rhs.local_slot_id != c4c::kInvalidSlotName) {
    return lhs.local_slot_id == rhs.local_slot_id;
  }
  return !lhs.local_slot_name.empty() &&
         lhs.local_slot_name == rhs.local_slot_name;
}

}  // namespace

BirSameBlockLoadLocalResult find_same_block_load_local_source(
    BirSameBlockLoadLocalRequest request) {
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
  const auto limit =
      std::min(request.before_instruction_index, request.block->insts.size());
  BirSameBlockLoadLocalResult match;
  for (std::size_t index = 0; index < limit; ++index) {
    const auto candidate = find_memory_access(view, index);
    if (candidate.status == BirViewStatus::Ambiguous) {
      return {.status = BirViewStatus::Ambiguous};
    }
    if (!candidate || candidate.result_value == nullptr ||
        candidate.result_value_name != value_name ||
        (value_type != TypeKind::Void &&
         candidate.result_value->type != value_type)) {
      continue;
    }
    if (candidate.kind != BirMemoryAccessKind::LoadLocal ||
        candidate.base_kind != BirMemoryBaseKind::LocalSlot) {
      return {.status = BirViewStatus::Incomplete};
    }
    const auto* load = std::get_if<LoadLocalInst>(candidate.instruction);
    if (load == nullptr) {
      return {.status = BirViewStatus::Incomplete};
    }
    if (match.status == BirViewStatus::Available) {
      return {.status = BirViewStatus::Ambiguous};
    }
    match = BirSameBlockLoadLocalResult{
        .status = BirViewStatus::Available,
        .access = candidate,
        .load = load,
        .result_value = candidate.result_value,
    };
  }
  if (!match) {
    return match;
  }
  for (std::size_t index = match.access.instruction_index + 1U; index < limit;
       ++index) {
    const auto candidate = find_memory_access(view, index);
    if (candidate.status == BirViewStatus::Ambiguous) {
      return {.status = BirViewStatus::Ambiguous};
    }
    if (candidate && candidate.kind == BirMemoryAccessKind::StoreLocal &&
        same_local_slot(match.access, candidate)) {
      return {.status = BirViewStatus::Incomplete};
    }
  }
  return match;
}

BirSameBlockStoreLocalSourceResult find_same_block_store_local_source(
    BirSameBlockLoadLocalRequest request) {
  if (!request) {
    return {.status = BirViewStatus::Incomplete};
  }
  const std::string_view value_name =
      request.value != nullptr ? request.value->name : request.value_name;
  const TypeKind value_type =
      request.value != nullptr ? request.value->type : request.value_type;
  if (value_name.empty() || value_type == TypeKind::Void) {
    return {.status = BirViewStatus::Incomplete};
  }

  const auto load_result = find_same_block_load_local_source(request);
  if (!load_result) {
    return {.status = load_result.status};
  }
  if (load_result.access.size_bytes == 0) {
    return {.status = BirViewStatus::Incomplete};
  }

  const auto view = make_bir_memory_access_view(*request.block);
  for (std::size_t index = load_result.access.instruction_index; index-- > 0;) {
    const auto candidate = find_memory_access(view, index);
    if (candidate.status == BirViewStatus::Ambiguous) {
      return {.status = BirViewStatus::Ambiguous};
    }
    if (candidate.status == BirViewStatus::Incomplete) {
      return {.status = BirViewStatus::Incomplete};
    }
    if (!candidate || candidate.kind != BirMemoryAccessKind::StoreLocal ||
        candidate.base_kind != BirMemoryBaseKind::LocalSlot ||
        !same_local_slot(load_result.access, candidate)) {
      continue;
    }
    if (candidate.byte_offset != load_result.access.byte_offset ||
        candidate.size_bytes != load_result.access.size_bytes) {
      const auto candidate_end = candidate.byte_offset +
                                 static_cast<std::int64_t>(candidate.size_bytes);
      const auto load_end = load_result.access.byte_offset +
                            static_cast<std::int64_t>(load_result.access.size_bytes);
      if (candidate.byte_offset < load_end &&
          load_result.access.byte_offset < candidate_end) {
        return {.status = BirViewStatus::Incomplete};
      }
      continue;
    }
    const auto* store = candidate.instruction != nullptr
                            ? std::get_if<StoreLocalInst>(candidate.instruction)
                            : nullptr;
    if (store == nullptr || candidate.stored_value == nullptr ||
        candidate.stored_value != &store->value || candidate.size_bytes == 0) {
      return {.status = BirViewStatus::Incomplete};
    }
    return {
        .status = BirViewStatus::Available,
        .load_access = load_result.access,
        .store_access = candidate,
        .load = load_result.load,
        .store = store,
        .loaded_value = load_result.result_value,
        .stored_value = candidate.stored_value,
    };
  }
  return {.status = BirViewStatus::Unavailable};
}

}  // namespace c4c::backend::bir

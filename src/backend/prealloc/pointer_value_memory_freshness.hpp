#pragma once

#include "addressing.hpp"
#include "value_locations.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace c4c::backend::prepare {

enum class PreparedPointerValueMemoryUseMode {
  Unknown,
  Load,
  Store,
};

[[nodiscard]] constexpr std::string_view prepared_pointer_value_memory_use_mode_name(
    PreparedPointerValueMemoryUseMode mode) {
  switch (mode) {
    case PreparedPointerValueMemoryUseMode::Unknown:
      return "unknown";
    case PreparedPointerValueMemoryUseMode::Load:
      return "load";
    case PreparedPointerValueMemoryUseMode::Store:
      return "store";
  }
  return "unknown";
}

struct PreparedPointerValueMemoryFreshnessAuthority {
  PreparedValueFreshnessAuthority freshness;
  PreparedPointerValueMemoryUseMode use_mode =
      PreparedPointerValueMemoryUseMode::Unknown;
  FunctionNameId function_name = kInvalidFunctionName;
  BlockLabelId block_label = kInvalidBlockLabel;
  std::size_t inst_index = 0;
  std::int64_t byte_offset = 0;
  std::size_t size_bytes = 0;
  std::size_t align_bytes = 0;
  bool can_use_base_plus_offset = false;
  bir::AddressSpace address_space = bir::AddressSpace::Default;
  bool is_volatile = false;
  bir::MemoryProvenanceBaseIdentityKind provenance_base_kind =
      bir::MemoryProvenanceBaseIdentityKind::Unknown;
  LinkNameId provenance_base_link_name = kInvalidLinkName;
  SlotNameId provenance_base_slot_name = kInvalidSlotName;
  bir::MemoryLayoutAuthorityKind layout_authority =
      bir::MemoryLayoutAuthorityKind::Unknown;
  bir::MemoryObjectExtent object_extent;
  bir::MemoryByteRange requested_range;
  bir::MemoryRangeVerdict range_verdict =
      bir::MemoryRangeVerdict::UnknownCompatible;
  bir::MemoryDynamicArrayRangeVerdict dynamic_array_verdict =
      bir::MemoryDynamicArrayRangeVerdict::Unknown;
};

struct PreparedPointerValueMemoryFreshnessQuery {
  const PreparedMemoryAccess* access = nullptr;
  PreparedValueId pointer_value_id = 0;
  ValueNameId pointer_value_name = kInvalidValueName;
  PreparedPointerValueMemoryUseMode use_mode =
      PreparedPointerValueMemoryUseMode::Unknown;
  std::vector<PreparedPointerValueMemoryFreshnessAuthority> candidates;
};

struct PreparedPointerValueMemoryFreshnessQueryResult {
  PreparedValueFreshnessQueryStatus status =
      PreparedValueFreshnessQueryStatus::NoCandidate;
  const PreparedPointerValueMemoryFreshnessAuthority* authority = nullptr;
};

[[nodiscard]] inline PreparedPointerValueMemoryUseMode
prepared_pointer_value_memory_use_mode(const PreparedMemoryAccess& access) {
  if (access.result_value_name.has_value() && !access.stored_value_name.has_value()) {
    return PreparedPointerValueMemoryUseMode::Load;
  }
  if (access.stored_value_name.has_value() && !access.result_value_name.has_value()) {
    return PreparedPointerValueMemoryUseMode::Store;
  }
  return PreparedPointerValueMemoryUseMode::Unknown;
}

[[nodiscard]] inline bool prepared_pointer_value_memory_has_required_support(
    const PreparedMemoryAccess& access,
    PreparedValueId pointer_value_id,
    ValueNameId pointer_value_name,
    PreparedPointerValueMemoryUseMode use_mode) {
  (void)pointer_value_id;
  return pointer_value_name != kInvalidValueName &&
         use_mode != PreparedPointerValueMemoryUseMode::Unknown &&
         prepared_pointer_value_memory_use_mode(access) == use_mode &&
         access.address.base_kind == PreparedAddressBaseKind::PointerValue &&
         access.address.pointer_value_name.has_value() &&
         *access.address.pointer_value_name == pointer_value_name &&
         access.address.can_use_base_plus_offset &&
         access.address.size_bytes != 0 &&
         access.address.align_bytes != 0;
}

[[nodiscard]] inline PreparedPointerValueMemoryFreshnessAuthority
make_prepared_pointer_value_memory_freshness_authority(
    const PreparedMemoryAccess& access,
    PreparedValueId pointer_value_id,
    PreparedPointerValueMemoryUseMode use_mode) {
  const auto pointer_value_name =
      access.address.pointer_value_name.value_or(kInvalidValueName);
  const auto& provenance = access.address.provenance;
  return PreparedPointerValueMemoryFreshnessAuthority{
      .freshness =
          PreparedValueFreshnessAuthority{
              .value_id = pointer_value_id,
              .value_name = pointer_value_name,
              .use_kind = PreparedValueFreshnessUseKind::PointerValueMemoryUse,
              .source_kind =
                  PreparedValueFreshnessSourceKind::PointerValueMemoryAccess,
              .proof_kind =
                  PreparedValueFreshnessProofKind::PointerValueMemoryAuthority,
              .rank = PreparedValueFreshnessSourceRank::PointerValueMemory,
              .reference =
                  PreparedValueFreshnessSourceReference{
                      .block_label = access.block_label,
                      .instruction_index = access.inst_index,
                  },
          },
      .use_mode = use_mode,
      .function_name = access.function_name,
      .block_label = access.block_label,
      .inst_index = access.inst_index,
      .byte_offset = access.address.byte_offset,
      .size_bytes = access.address.size_bytes,
      .align_bytes = access.address.align_bytes,
      .can_use_base_plus_offset = access.address.can_use_base_plus_offset,
      .address_space = access.address_space,
      .is_volatile = access.is_volatile,
      .provenance_base_kind = provenance.base_identity.kind,
      .provenance_base_link_name = provenance.base_identity.link_name_id,
      .provenance_base_slot_name = provenance.base_identity.slot_name_id,
      .layout_authority = provenance.layout_authority,
      .object_extent = provenance.object_extent,
      .requested_range = provenance.requested_range,
      .range_verdict = provenance.range_verdict,
      .dynamic_array_verdict = provenance.dynamic_array.verdict,
  };
}

[[nodiscard]] inline bool prepared_pointer_value_memory_authority_matches_access(
    const PreparedPointerValueMemoryFreshnessAuthority& authority,
    const PreparedMemoryAccess& access,
    PreparedValueId pointer_value_id,
    ValueNameId pointer_value_name,
    PreparedPointerValueMemoryUseMode use_mode) {
  if (!prepared_pointer_value_memory_has_required_support(
          access, pointer_value_id, pointer_value_name, use_mode)) {
    return false;
  }
  const auto& provenance = access.address.provenance;
  return authority.use_mode == use_mode &&
         authority.function_name == access.function_name &&
         authority.block_label == access.block_label &&
         authority.inst_index == access.inst_index &&
         authority.byte_offset == access.address.byte_offset &&
         authority.size_bytes == access.address.size_bytes &&
         authority.align_bytes == access.address.align_bytes &&
         authority.can_use_base_plus_offset == access.address.can_use_base_plus_offset &&
         authority.address_space == access.address_space &&
         authority.is_volatile == access.is_volatile &&
         authority.provenance_base_kind == provenance.base_identity.kind &&
         authority.provenance_base_link_name == provenance.base_identity.link_name_id &&
         authority.provenance_base_slot_name == provenance.base_identity.slot_name_id &&
         authority.layout_authority == provenance.layout_authority &&
         authority.object_extent.completeness ==
             provenance.object_extent.completeness &&
         authority.object_extent.size_bytes ==
             provenance.object_extent.size_bytes &&
         authority.object_extent.size_known == provenance.object_extent.size_known &&
         authority.requested_range.available ==
             provenance.requested_range.available &&
         authority.requested_range.begin == provenance.requested_range.begin &&
         authority.requested_range.size_bytes ==
             provenance.requested_range.size_bytes &&
         authority.requested_range.end == provenance.requested_range.end &&
         authority.requested_range.end_available ==
             provenance.requested_range.end_available &&
         authority.requested_range.overflowed ==
             provenance.requested_range.overflowed &&
         authority.range_verdict == provenance.range_verdict &&
         authority.dynamic_array_verdict == provenance.dynamic_array.verdict &&
         authority.freshness.value_id == pointer_value_id &&
         authority.freshness.value_name == pointer_value_name &&
         authority.freshness.use_kind ==
             PreparedValueFreshnessUseKind::PointerValueMemoryUse &&
         authority.freshness.source_kind ==
             PreparedValueFreshnessSourceKind::PointerValueMemoryAccess &&
         authority.freshness.proof_kind ==
             PreparedValueFreshnessProofKind::PointerValueMemoryAuthority &&
         authority.freshness.rank == PreparedValueFreshnessSourceRank::PointerValueMemory &&
         authority.freshness.reference.block_label == access.block_label &&
         authority.freshness.reference.instruction_index == access.inst_index;
}

[[nodiscard]] inline PreparedPointerValueMemoryFreshnessQueryResult
find_prepared_pointer_value_memory_freshness_authority(
    const PreparedPointerValueMemoryFreshnessQuery& query) {
  if (query.access == nullptr ||
      !prepared_pointer_value_memory_has_required_support(
          *query.access,
          query.pointer_value_id,
          query.pointer_value_name,
          query.use_mode)) {
    return PreparedPointerValueMemoryFreshnessQueryResult{
        .status = PreparedValueFreshnessQueryStatus::NoCandidate,
    };
  }

  std::vector<PreparedValueFreshnessAuthority> freshness_candidates;
  std::vector<const PreparedPointerValueMemoryFreshnessAuthority*> matched;
  freshness_candidates.reserve(query.candidates.size());
  matched.reserve(query.candidates.size());
  for (const auto& candidate : query.candidates) {
    if (!prepared_pointer_value_memory_authority_matches_access(
            candidate,
            *query.access,
            query.pointer_value_id,
            query.pointer_value_name,
            query.use_mode)) {
      continue;
    }
    matched.push_back(&candidate);
    freshness_candidates.push_back(candidate.freshness);
  }

  const auto selected = find_prepared_value_freshness_authority(
      PreparedValueFreshnessQuery{
          .value_id = query.pointer_value_id,
          .value_name = query.pointer_value_name,
          .use_kind = PreparedValueFreshnessUseKind::PointerValueMemoryUse,
          .block_label = query.access->block_label,
          .instruction_index = query.access->inst_index,
          .candidates = freshness_candidates,
      });
  if (!prepared_value_freshness_query_selected(selected)) {
    return PreparedPointerValueMemoryFreshnessQueryResult{
        .status = selected.status,
    };
  }

  if (!matched.empty()) {
    return PreparedPointerValueMemoryFreshnessQueryResult{
        .status = PreparedValueFreshnessQueryStatus::Selected,
        .authority = matched.front(),
    };
  }
  return PreparedPointerValueMemoryFreshnessQueryResult{
      .status = PreparedValueFreshnessQueryStatus::InvalidCandidate,
  };
}

[[nodiscard]] inline bool prepared_pointer_value_memory_freshness_available(
    const PreparedPointerValueMemoryFreshnessQuery& query) {
  const auto selected = find_prepared_pointer_value_memory_freshness_authority(query);
  return selected.status == PreparedValueFreshnessQueryStatus::Selected &&
         selected.authority != nullptr;
}

}  // namespace c4c::backend::prepare

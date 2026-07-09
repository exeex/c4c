#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include "../../shared/text_id_table.hpp"

namespace c4c::backend::bir {

enum class MemoryProvenanceBaseIdentityKind : unsigned char {
  Unknown,
  UnknownRuntimeBase,
  LocalSlot,
  GlobalSymbol,
  PointerValue,
  FormalParameter,
  ByvalParameter,
  SretParameter,
  StringConstant,
};

enum class MemoryObjectExtentCompleteness : unsigned char {
  Unknown,
  Complete,
  Partial,
};

enum class MemoryLayoutAuthorityKind : unsigned char {
  Unknown,
  StructuredLayout,
  ScalarLayout,
  ByteStorageAggregate,
  StringConstantBytes,
  StringConstantLabelPointer,
  RenderedTypeFallback,
  OpaqueCompatibility,
};

enum class MemoryRangeVerdict : unsigned char {
  UnknownCompatible,
  ProvenInBounds,
  ProvenOutOfBounds,
};

enum class MemoryDynamicArrayRangeVerdict : unsigned char {
  Unknown,
  BoundedByElementCount,
  Unbounded,
};

struct MemoryProvenanceBaseIdentity {
  MemoryProvenanceBaseIdentityKind kind = MemoryProvenanceBaseIdentityKind::Unknown;
  std::string spelling;
  Value value;
  LinkNameId link_name_id = kInvalidLinkName;
  SlotNameId slot_name_id = kInvalidSlotName;
};

struct MemoryObjectExtent {
  MemoryObjectExtentCompleteness completeness = MemoryObjectExtentCompleteness::Unknown;
  std::size_t size_bytes = 0;
  bool size_known = false;
};

struct MemoryByteRange {
  bool available = false;
  std::int64_t begin = 0;
  std::size_t size_bytes = 0;
  std::int64_t end = 0;
  bool end_available = false;
  bool overflowed = false;
};

[[nodiscard]] inline MemoryByteRange make_memory_byte_range(std::int64_t byte_offset,
                                                           std::size_t size_bytes) {
  MemoryByteRange range{
      .available = true,
      .begin = byte_offset,
      .size_bytes = size_bytes,
  };
  if (size_bytes >
      static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())) {
    range.overflowed = true;
    return range;
  }

  const auto delta = static_cast<std::int64_t>(size_bytes);
  if (byte_offset > std::numeric_limits<std::int64_t>::max() - delta) {
    range.overflowed = true;
    return range;
  }

  range.end = byte_offset + delta;
  range.end_available = true;
  return range;
}

struct MemoryDynamicArrayFacts {
  bool available = false;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  std::size_t base_byte_offset = 0;
  Value index;
  MemoryDynamicArrayRangeVerdict verdict = MemoryDynamicArrayRangeVerdict::Unknown;
};

struct MemoryAccessProvenance {
  MemoryProvenanceBaseIdentity base_identity;
  MemoryObjectExtent object_extent;
  MemoryByteRange requested_range;
  MemoryLayoutAuthorityKind layout_authority = MemoryLayoutAuthorityKind::Unknown;
  MemoryDynamicArrayFacts dynamic_array;
  MemoryRangeVerdict range_verdict = MemoryRangeVerdict::UnknownCompatible;
};

inline void prove_memory_dynamic_array_range(MemoryAccessProvenance& provenance) {
  provenance.dynamic_array.verdict = MemoryDynamicArrayRangeVerdict::Unknown;
  const auto& requested = provenance.requested_range;
  const auto& dynamic_array = provenance.dynamic_array;
  if (!dynamic_array.available || dynamic_array.element_count == 0 ||
      dynamic_array.element_stride_bytes == 0 || !requested.available ||
      requested.overflowed || !requested.end_available || requested.begin < 0 ||
      requested.end < requested.begin) {
    return;
  }

  if (dynamic_array.element_count >
      std::numeric_limits<std::size_t>::max() / dynamic_array.element_stride_bytes) {
    return;
  }
  const auto array_size_bytes =
      dynamic_array.element_count * dynamic_array.element_stride_bytes;
  if (dynamic_array.base_byte_offset >
      std::numeric_limits<std::size_t>::max() - array_size_bytes) {
    return;
  }

  const auto begin = static_cast<std::size_t>(requested.begin);
  const auto end = static_cast<std::size_t>(requested.end);
  const auto array_begin = dynamic_array.base_byte_offset;
  const auto array_end = dynamic_array.base_byte_offset + array_size_bytes;
  provenance.dynamic_array.verdict =
      begin >= array_begin && end <= array_end
          ? MemoryDynamicArrayRangeVerdict::BoundedByElementCount
          : MemoryDynamicArrayRangeVerdict::Unbounded;
}

inline void prove_memory_access_requested_range(MemoryAccessProvenance& provenance) {
  provenance.range_verdict = MemoryRangeVerdict::UnknownCompatible;
  prove_memory_dynamic_array_range(provenance);
  if (!provenance.requested_range.available ||
      !provenance.object_extent.size_known ||
      provenance.object_extent.completeness != MemoryObjectExtentCompleteness::Complete) {
    return;
  }
  if (provenance.requested_range.overflowed ||
      !provenance.requested_range.end_available ||
      provenance.requested_range.begin < 0 ||
      provenance.requested_range.end < provenance.requested_range.begin) {
    provenance.range_verdict = MemoryRangeVerdict::ProvenOutOfBounds;
    return;
  }

  const auto begin = static_cast<std::size_t>(provenance.requested_range.begin);
  const auto end = static_cast<std::size_t>(provenance.requested_range.end);
  provenance.range_verdict =
      begin <= provenance.object_extent.size_bytes &&
              end <= provenance.object_extent.size_bytes
          ? MemoryRangeVerdict::ProvenInBounds
          : MemoryRangeVerdict::ProvenOutOfBounds;
}

}  // namespace c4c::backend::bir

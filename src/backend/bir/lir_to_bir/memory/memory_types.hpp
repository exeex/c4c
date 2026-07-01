#pragma once

#include "../../lir_to_bir.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::backend::lir_to_bir_detail {
struct GlobalAddress;
}

namespace c4c::backend {

struct GlobalPointerSlotKey {
  // LinkNameId is the authority when present. Raw/no-id compatibility entries
  // keep kInvalidLinkName so textual LIR operands remain distinguishable from
  // metadata-rich generated globals that share the same spelling.
  LinkNameId link_name_id = kInvalidLinkName;
  std::string global_name;
  std::size_t byte_offset = 0;

  bool operator==(const GlobalPointerSlotKey& other) const {
    return link_name_id == other.link_name_id && global_name == other.global_name &&
           byte_offset == other.byte_offset;
  }
};

struct GlobalPointerSlotKeyHash {
  std::size_t operator()(const GlobalPointerSlotKey& key) const {
    return std::hash<LinkNameId>{}(key.link_name_id) ^
           (std::hash<std::string>{}(key.global_name) << 1) ^
           (std::hash<std::size_t>{}(key.byte_offset) << 2);
  }
};

struct LocalSlotAddress {
  // Route-local slot spelling used by memory/provenance lowering.
  std::string slot_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::int64_t byte_offset = 0;
  // Compatibility LIR type spellings retained for aggregate pointer layout.
  std::string storage_type_text;
  std::string type_text;
  std::vector<std::string> array_element_slots;
  std::size_t array_base_index = 0;
  std::string source_object_name;
  std::string derivation_result_name;
};

struct LocalArraySlots {
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::vector<std::string> element_slots;
};

struct DynamicLocalPointerArrayAccess {
  std::vector<std::string> element_slots;
  bir::Value index;
};

struct DynamicLocalAggregateArrayAccess {
  // Compatibility LIR type spelling retained for element layout.
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  // Aggregate field slots are keyed by byte offset; values are route-local slot
  // spellings.
  std::unordered_map<std::size_t, std::string> leaf_slots;
  bir::Value index;
};

struct DynamicPointerValueArrayAccess {
  bir::Value base_value;
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
  bir::MemoryAccessProvenance provenance;
};

struct LocalPointerArrayBase {
  std::vector<std::string> element_slots;
  std::size_t base_index = 0;
  std::string source_object_name;
  std::string derivation_result_name;
};

struct DynamicGlobalPointerArrayAccess {
  // Final spelling for a LinkNameId-backed global already resolved upstream.
  std::string global_name;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct DynamicGlobalAggregateArrayAccess {
  // Final global spelling plus compatibility element type text used for layout.
  std::string global_name;
  LinkNameId link_name_id = kInvalidLinkName;
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct DynamicGlobalScalarArrayAccess {
  // Final spelling for a LinkNameId-backed global already resolved upstream.
  std::string global_name;
  LinkNameId link_name_id = kInvalidLinkName;
  bir::TypeKind element_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
  std::size_t outer_element_count = 0;
  std::size_t outer_element_stride_bytes = 0;
  bir::Value outer_index;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct LocalAggregateSlots {
  // Compatibility LIR type spellings retained for aggregate slot layout.
  std::string storage_type_text;
  std::string type_text;
  std::size_t base_byte_offset = 0;
  // Aggregate field slots are keyed by byte offset; values are route-local slot
  // spellings.
  std::unordered_map<std::size_t, std::string> leaf_slots;
};

struct PointerAddress {
  bir::Value base_value;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
  std::size_t dynamic_element_count = 0;
  std::size_t dynamic_element_stride_bytes = 0;
  // Compatibility LIR type spellings retained for pointer provenance layout.
  std::string storage_type_text;
  std::string type_text;
  bir::TypeKind loaded_pointer_value_type = bir::TypeKind::Void;
  std::string loaded_pointer_type_text;
  bool aarch64_variadic_fp_register_save_area = false;
  bool loaded_pointer_aarch64_variadic_fp_register_save_area = false;
  bir::MemoryAccessProvenance provenance;
};

[[nodiscard]] inline bir::MemoryObjectExtent complete_memory_object_extent(
    std::size_t size_bytes) {
  return bir::MemoryObjectExtent{
      .completeness = bir::MemoryObjectExtentCompleteness::Complete,
      .size_bytes = size_bytes,
      .size_known = true,
  };
}

[[nodiscard]] inline bir::MemoryAccessProvenance memory_provenance_for_base(
    bir::MemoryProvenanceBaseIdentityKind kind,
    std::string spelling,
    bir::Value value = {},
    std::optional<std::size_t> complete_extent_size = std::nullopt) {
  bir::MemoryAccessProvenance provenance;
  provenance.base_identity = bir::MemoryProvenanceBaseIdentity{
      .kind = kind,
      .spelling = std::move(spelling),
      .value = std::move(value),
  };
  if (complete_extent_size.has_value()) {
    provenance.object_extent = complete_memory_object_extent(*complete_extent_size);
  }
  return provenance;
}

[[nodiscard]] inline bir::MemoryAccessProvenance pointer_value_base_provenance(
    const bir::Value& value,
    std::optional<std::size_t> complete_extent_size = std::nullopt) {
  return memory_provenance_for_base(
      bir::MemoryProvenanceBaseIdentityKind::PointerValue,
      value.name,
      value,
      complete_extent_size);
}

[[nodiscard]] inline bir::MemoryAccessProvenance unknown_runtime_base_provenance(
    const bir::Value& value) {
  return memory_provenance_for_base(
      bir::MemoryProvenanceBaseIdentityKind::UnknownRuntimeBase,
      value.name,
      value);
}

struct AggregateArrayExtent {
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
};

struct LocalAggregateGepTarget {
  std::string type_text;
  std::int64_t byte_offset = 0;
};

// The following string-keyed maps are memory/provenance side tables keyed by
// route-local SSA/slot spellings or final global spellings while lowering one
// function. They do not replace LinkNameId on emitted BIR global operations.
using GlobalPointerMap = std::unordered_map<std::string, lir_to_bir_detail::GlobalAddress>;
using GlobalObjectPointerMap = std::unordered_map<std::string, lir_to_bir_detail::GlobalAddress>;
using GlobalAddressIntMap = std::unordered_map<std::string, lir_to_bir_detail::GlobalAddress>;
using GlobalObjectAddressIntMap =
    std::unordered_map<std::string, lir_to_bir_detail::GlobalAddress>;
using LocalAddressSlots = std::unordered_map<std::string, lir_to_bir_detail::GlobalAddress>;
using LocalSlotAddressSlots = std::unordered_map<std::string, LocalSlotAddress>;
using LocalSlotPointerValues = std::unordered_map<std::string, LocalSlotAddress>;
using GlobalAddressSlots =
    std::unordered_map<std::string, std::optional<lir_to_bir_detail::GlobalAddress>>;
using AddressedGlobalPointerSlots =
    std::unordered_map<GlobalPointerSlotKey,
                       std::optional<lir_to_bir_detail::GlobalAddress>,
                       GlobalPointerSlotKeyHash>;
using LocalArraySlotMap = std::unordered_map<std::string, LocalArraySlots>;
using DynamicLocalPointerArrayMap =
    std::unordered_map<std::string, DynamicLocalPointerArrayAccess>;
using DynamicLocalAggregateArrayMap =
    std::unordered_map<std::string, DynamicLocalAggregateArrayAccess>;
using DynamicPointerValueArrayMap =
    std::unordered_map<std::string, DynamicPointerValueArrayAccess>;
using LocalPointerArrayBaseMap = std::unordered_map<std::string, LocalPointerArrayBase>;
using DynamicGlobalPointerArrayMap =
    std::unordered_map<std::string, DynamicGlobalPointerArrayAccess>;
using DynamicGlobalAggregateArrayMap =
    std::unordered_map<std::string, DynamicGlobalAggregateArrayAccess>;
using DynamicGlobalScalarArrayMap =
    std::unordered_map<std::string, DynamicGlobalScalarArrayAccess>;
using LocalAggregateSlotMap = std::unordered_map<std::string, LocalAggregateSlots>;
using LocalAggregateFieldSet = std::unordered_set<std::string>;
using LocalPointerValueAliasMap = std::unordered_map<std::string, bir::Value>;
using PointerAddressMap = std::unordered_map<std::string, PointerAddress>;
using PointerAddressIntMap = std::unordered_map<std::string, PointerAddress>;
using GlobalPointerValueSlots = std::unordered_map<std::string, std::optional<PointerAddress>>;
using AddressedGlobalPointerValueSlots =
    std::unordered_map<GlobalPointerSlotKey, std::optional<PointerAddress>, GlobalPointerSlotKeyHash>;

}  // namespace c4c::backend

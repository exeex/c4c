#pragma once

#include "../../lir_to_bir.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace c4c::backend::lir_to_bir_detail {
struct GlobalAddress;
}

namespace c4c::backend {

struct GlobalPointerSlotKey {
  // LinkNameId-backed global resolution stores final spelling here because the
  // memory maps bridge to raw LIR operands and target byte offsets.
  std::string global_name;
  std::size_t byte_offset = 0;

  bool operator==(const GlobalPointerSlotKey& other) const {
    return global_name == other.global_name && byte_offset == other.byte_offset;
  }
};

struct GlobalPointerSlotKeyHash {
  std::size_t operator()(const GlobalPointerSlotKey& key) const {
    return std::hash<std::string>{}(key.global_name) ^
           (std::hash<std::size_t>{}(key.byte_offset) << 1);
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
};

struct LocalPointerArrayBase {
  std::vector<std::string> element_slots;
  std::size_t base_index = 0;
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
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct DynamicGlobalScalarArrayAccess {
  // Final spelling for a LinkNameId-backed global already resolved upstream.
  std::string global_name;
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
};

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

#pragma once

#include "../../lir_to_bir.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace c4c::backend {

struct GlobalPointerSlotKey {
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
  std::string slot_name;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::int64_t byte_offset = 0;
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
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
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
  std::string global_name;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct DynamicGlobalAggregateArrayAccess {
  std::string global_name;
  std::string element_type_text;
  std::size_t byte_offset = 0;
  std::size_t element_count = 0;
  std::size_t element_stride_bytes = 0;
  bir::Value index;
};

struct DynamicGlobalScalarArrayAccess {
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
  std::string storage_type_text;
  std::string type_text;
  std::size_t base_byte_offset = 0;
  std::unordered_map<std::size_t, std::string> leaf_slots;
};

struct PointerAddress {
  bir::Value base_value;
  bir::TypeKind value_type = bir::TypeKind::Void;
  std::size_t byte_offset = 0;
  std::size_t dynamic_element_count = 0;
  std::size_t dynamic_element_stride_bytes = 0;
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

}  // namespace c4c::backend

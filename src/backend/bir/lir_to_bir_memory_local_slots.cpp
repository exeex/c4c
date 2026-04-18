#include "lir_to_bir.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::parse_i64;

std::optional<std::vector<std::string>> BirFunctionLowerer::collect_local_scalar_array_slots(
    std::string_view type_text,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  const auto layout = compute_aggregate_type_layout(
      c4c::codegen::lir::trim_lir_arg_text(type_text), type_decls);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar || element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  std::size_t slot_count = layout.array_count;
  struct RepeatedAggregateExtent {
    std::size_t element_count = 0;
    std::size_t element_stride_bytes = 0;
  };
  const auto find_repeated_extent =
      [&](const auto& self,
          std::string_view storage_type_text,
          std::size_t target_offset,
          std::string_view repeated_type_text) -> std::optional<RepeatedAggregateExtent> {
    const auto storage_layout = compute_aggregate_type_layout(storage_type_text, type_decls);
    if (storage_layout.kind == AggregateTypeLayout::Kind::Invalid ||
        target_offset >= storage_layout.size_bytes) {
      return std::nullopt;
    }

    switch (storage_layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto nested_element_layout =
            compute_aggregate_type_layout(storage_layout.element_type_text, type_decls);
        if (nested_element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            nested_element_layout.size_bytes == 0) {
          return std::nullopt;
        }
        const auto element_index = target_offset / nested_element_layout.size_bytes;
        if (element_index >= storage_layout.array_count) {
          return std::nullopt;
        }
        const auto nested_offset = target_offset % nested_element_layout.size_bytes;
        if (nested_offset == 0 &&
            c4c::codegen::lir::trim_lir_arg_text(storage_layout.element_type_text) ==
                c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
          return RepeatedAggregateExtent{
              .element_count = storage_layout.array_count - element_index,
              .element_stride_bytes = nested_element_layout.size_bytes,
          };
        }
        return self(self, storage_layout.element_type_text, nested_offset, repeated_type_text);
      }
      case AggregateTypeLayout::Kind::Struct:
        for (std::size_t index = 0; index < storage_layout.fields.size(); ++index) {
          const auto field_begin = storage_layout.fields[index].byte_offset;
          const auto field_end =
              index + 1 < storage_layout.fields.size() ? storage_layout.fields[index + 1].byte_offset
                                                       : storage_layout.size_bytes;
          if (target_offset < field_begin || target_offset >= field_end) {
            continue;
          }
          return self(self,
                      storage_layout.fields[index].type_text,
                      target_offset - field_begin,
                      repeated_type_text);
        }
        return std::nullopt;
      default:
        return std::nullopt;
    }
  };
  if (const auto extent = find_repeated_extent(find_repeated_extent,
                                               aggregate_slots.storage_type_text,
                                               aggregate_slots.base_byte_offset,
                                               type_text);
      extent.has_value() && extent->element_stride_bytes == layout.size_bytes) {
    slot_count *= extent->element_count;
  }

  element_slots.reserve(slot_count);
  for (std::size_t index = 0; index < slot_count; ++index) {
    const auto slot_it = aggregate_slots.leaf_slots.find(
        aggregate_slots.base_byte_offset + index * element_layout.size_bytes);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    element_slots.push_back(slot_it->second);
  }
  return element_slots;
}

bool BirFunctionLowerer::is_local_array_element_slot(std::string_view slot_name,
                                                     const LocalArraySlotMap& local_array_slots) {
  for (const auto& [array_name, array_slots] : local_array_slots) {
    (void)array_name;
    if (std::find(array_slots.element_slots.begin(),
                  array_slots.element_slots.end(),
                  slot_name) != array_slots.element_slots.end()) {
      return true;
    }
  }
  return false;
}

std::optional<std::pair<std::size_t, bir::TypeKind>> BirFunctionLowerer::parse_local_array_type(
    std::string_view text) {
  if (text.size() < 6 || text.front() != '[' || text.back() != ']') {
    return std::nullopt;
  }

  const auto x_pos = text.find(" x ");
  if (x_pos == std::string_view::npos || x_pos <= 1) {
    return std::nullopt;
  }

  const auto count = parse_i64(text.substr(1, x_pos - 1));
  if (!count.has_value() || *count <= 0) {
    return std::nullopt;
  }

  const auto element_type =
      lower_scalar_or_function_pointer_type(text.substr(x_pos + 3, text.size() - x_pos - 4));
  if (!element_type.has_value()) {
    return std::nullopt;
  }

  return std::pair<std::size_t, bir::TypeKind>{static_cast<std::size_t>(*count), *element_type};
}

}  // namespace c4c::backend

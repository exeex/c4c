#include "lir_to_bir.hpp"

#include <algorithm>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend {

using DynamicGlobalAggregateArrayAccess = BirFunctionLowerer::DynamicGlobalAggregateArrayAccess;
using DynamicGlobalPointerArrayAccess = BirFunctionLowerer::DynamicGlobalPointerArrayAccess;
using DynamicLocalAggregateArrayAccess = BirFunctionLowerer::DynamicLocalAggregateArrayAccess;
using DynamicLocalPointerArrayAccess = BirFunctionLowerer::DynamicLocalPointerArrayAccess;
using GlobalAddress = BirFunctionLowerer::GlobalAddress;
using GlobalPointerSlotKey = BirFunctionLowerer::GlobalPointerSlotKey;
using LocalArraySlots = BirFunctionLowerer::LocalArraySlots;
using LocalPointerArrayBase = BirFunctionLowerer::LocalPointerArrayBase;
using lir_to_bir_detail::compute_aggregate_type_layout;
using lir_to_bir_detail::GlobalInfo;
using lir_to_bir_detail::is_known_function_symbol;
using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::parse_i64;
using lir_to_bir_detail::parse_typed_operand;
using lir_to_bir_detail::resolve_index_operand;
using lir_to_bir_detail::type_size_bytes;

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

std::optional<bir::Value> BirFunctionLowerer::lower_zero_initializer_value(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(false);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(0);
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(0);
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(0);
    case bir::TypeKind::Ptr:
      return bir::Value{
          .kind = bir::Value::Kind::Immediate,
          .type = bir::TypeKind::Ptr,
          .immediate = 0,
          .immediate_bits = 0,
      };
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> BirFunctionLowerer::lower_repeated_byte_initializer_value(
    bir::TypeKind type,
    std::uint8_t fill_byte) {
  if (fill_byte == 0) {
    return lower_zero_initializer_value(type);
  }

  switch (type) {
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(static_cast<std::int8_t>(fill_byte));
    case bir::TypeKind::I32:
    case bir::TypeKind::I64:
      break;
    default:
      return std::nullopt;
  }

  const auto bit_width = integer_type_bit_width(type);
  if (!bit_width.has_value() || *bit_width % 8u != 0u) {
    return std::nullopt;
  }

  std::uint64_t repeated_bits = 0;
  for (unsigned shift = 0; shift < *bit_width; shift += 8u) {
    repeated_bits |= std::uint64_t{fill_byte} << shift;
  }

  if (*bit_width < 64u) {
    return make_integer_immediate(type, sign_extend_bits(repeated_bits, *bit_width));
  }

  if ((repeated_bits & (std::uint64_t{1} << 63u)) == 0u) {
    return bir::Value::immediate_i64(static_cast<std::int64_t>(repeated_bits));
  }
  if (repeated_bits == (std::uint64_t{1} << 63u)) {
    return bir::Value::immediate_i64(std::numeric_limits<std::int64_t>::min());
  }
  return bir::Value::immediate_i64(-static_cast<std::int64_t>((~repeated_bits) + 1u));
}

std::optional<bir::Value> BirFunctionLowerer::lower_typed_index_value(
    const ParsedTypedOperand& index_operand,
    const ValueMap& value_aliases) {
  const auto index_type = lower_integer_type(index_operand.type_text);
  if (!index_type.has_value() ||
      (*index_type != bir::TypeKind::I32 && *index_type != bir::TypeKind::I64)) {
    return std::nullopt;
  }
  return BirFunctionLowerer::lower_value(index_operand.operand, *index_type, value_aliases);
}

std::optional<bir::Value> BirFunctionLowerer::make_index_immediate(bir::TypeKind type,
                                                                   std::size_t value) {
  switch (type) {
    case bir::TypeKind::I32:
      return bir::Value::immediate_i32(static_cast<std::int32_t>(value));
    case bir::TypeKind::I64:
      return bir::Value::immediate_i64(static_cast<std::int64_t>(value));
    default:
      return std::nullopt;
  }
}

std::optional<bir::Value> BirFunctionLowerer::synthesize_value_array_selects(
    std::string_view result_name,
    const std::vector<bir::Value>& element_values,
    const bir::Value& index_value,
    std::vector<bir::Inst>* lowered_insts) {
  if (element_values.empty() ||
      (index_value.type != bir::TypeKind::I32 && index_value.type != bir::TypeKind::I64)) {
    return std::nullopt;
  }
  if (element_values.size() == 1) {
    return element_values.front();
  }

  bir::Value current = element_values.back();
  for (std::size_t rev_index = element_values.size() - 1; rev_index-- > 0;) {
    const auto compare_rhs = make_index_immediate(index_value.type, rev_index);
    if (!compare_rhs.has_value()) {
      return std::nullopt;
    }

    const bool is_final = rev_index == 0;
    const std::string select_name =
        is_final ? std::string(result_name)
                 : std::string(result_name) + ".sel" + std::to_string(rev_index);
    lowered_insts->push_back(bir::SelectInst{
        .predicate = bir::BinaryOpcode::Eq,
        .result = bir::Value::named(current.type, select_name),
        .compare_type = index_value.type,
        .lhs = index_value,
        .rhs = *compare_rhs,
        .true_value = element_values[rev_index],
        .false_value = current,
    });
    current = bir::Value::named(current.type, select_name);
  }
  return current;
}

std::optional<bir::Value> BirFunctionLowerer::synthesize_pointer_array_selects(
    std::string_view result_name,
    const std::vector<bir::Value>& element_values,
    const bir::Value& index_value,
    std::vector<bir::Inst>* lowered_insts) {
  if (element_values.empty()) {
    return std::nullopt;
  }
  for (const auto& value : element_values) {
    if (value.type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
  }
  return synthesize_value_array_selects(result_name, element_values, index_value, lowered_insts);
}

std::optional<std::vector<bir::Value>> BirFunctionLowerer::collect_local_pointer_values(
    const std::vector<std::string>& element_slots,
    const LocalPointerValueAliasMap& local_pointer_value_aliases) {
  if (element_slots.empty()) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(element_slots.size());
  for (const auto& slot_name : element_slots) {
    const auto alias_it = local_pointer_value_aliases.find(slot_name);
    if (alias_it == local_pointer_value_aliases.end() || alias_it->second.type != bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    element_values.push_back(alias_it->second);
  }
  return element_values;
}

std::optional<std::vector<bir::Value>> BirFunctionLowerer::collect_global_array_pointer_values(
    const DynamicGlobalPointerArrayAccess& access,
    const GlobalTypes& global_types) {
  const auto global_it = global_types.find(access.global_name);
  if (global_it == global_types.end() || access.element_count == 0) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(access.element_count);
  const auto element_stride =
      access.element_stride_bytes == 0 ? type_size_bytes(bir::TypeKind::Ptr)
                                       : access.element_stride_bytes;
  for (std::size_t index = 0; index < access.element_count; ++index) {
    const auto offset = access.byte_offset + index * element_stride;
    const auto init_it = global_it->second.pointer_initializer_offsets.find(offset);
    if (init_it == global_it->second.pointer_initializer_offsets.end() ||
        init_it->second.value_type != bir::TypeKind::Ptr || init_it->second.byte_offset != 0) {
      return std::nullopt;
    }
    element_values.push_back(
        bir::Value::named(bir::TypeKind::Ptr, "@" + init_it->second.global_name));
  }
  return element_values;
}

std::optional<BirFunctionLowerer::AggregateArrayExtent>
BirFunctionLowerer::find_repeated_aggregate_extent_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    std::string_view repeated_type_text,
    const TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      if (nested_offset == 0 &&
          c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text) ==
              c4c::codegen::lir::trim_lir_arg_text(repeated_type_text)) {
        return AggregateArrayExtent{
            .element_count = layout.array_count - element_index,
            .element_stride_bytes = element_layout.size_bytes,
        };
      }
      return find_repeated_aggregate_extent_at_offset(
          layout.element_type_text, nested_offset, repeated_type_text, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_repeated_aggregate_extent_at_offset(layout.fields[index].type_text,
                                                        target_offset - field_begin,
                                                        repeated_type_text,
                                                        type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_relative_gep_target(
    std::string_view type_text,
    std::size_t base_byte_offset,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
  std::size_t byte_offset = base_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
      if (index_pos + 1 != gep.indices.size()) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    if (index_pos == 0) {
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  return LocalAggregateGepTarget{
      .type_text = std::string(current_type),
      .byte_offset = byte_offset,
  };
}

std::optional<std::size_t> BirFunctionLowerer::find_pointer_array_length_at_offset(
    std::string_view type_text,
    std::size_t target_offset,
    const TypeDeclMap& type_decls) {
  const auto layout = compute_aggregate_type_layout(type_text, type_decls);
  if (layout.kind == AggregateTypeLayout::Kind::Invalid || target_offset >= layout.size_bytes) {
    return std::nullopt;
  }

  switch (layout.kind) {
    case AggregateTypeLayout::Kind::Array: {
      const auto element_layout =
          compute_aggregate_type_layout(layout.element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
          element_layout.size_bytes == 0) {
        return std::nullopt;
      }
      if (target_offset == 0 &&
          element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
          element_layout.scalar_type == bir::TypeKind::Ptr) {
        return layout.array_count;
      }
      const auto element_index = target_offset / element_layout.size_bytes;
      if (element_index >= layout.array_count) {
        return std::nullopt;
      }
      const auto nested_offset = target_offset % element_layout.size_bytes;
      return find_pointer_array_length_at_offset(
          layout.element_type_text, nested_offset, type_decls);
    }
    case AggregateTypeLayout::Kind::Struct:
      for (std::size_t index = 0; index < layout.fields.size(); ++index) {
        const auto field_begin = layout.fields[index].byte_offset;
        const auto field_end =
            index + 1 < layout.fields.size() ? layout.fields[index + 1].byte_offset
                                             : layout.size_bytes;
        if (target_offset < field_begin || target_offset >= field_end) {
          continue;
        }
        return find_pointer_array_length_at_offset(
            layout.fields[index].type_text, target_offset - field_begin, type_decls);
      }
      return std::nullopt;
    default:
      return std::nullopt;
  }
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_global_gep_address(
    std::string_view global_name,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
  std::size_t byte_offset = 0;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0) {
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  return GlobalAddress{
      .global_name = std::string(global_name),
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_relative_global_gep_address(
    const GlobalAddress& base_address,
    std::string_view type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  auto current_type = c4c::codegen::lir::trim_lir_arg_text(type_text);
  std::size_t byte_offset = base_address.byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
      if (index_pos + 1 != gep.indices.size()) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    if (index_pos == 0) {
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = layout.element_type_text;
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = layout.fields[static_cast<std::size_t>(*index_value)].type_text;
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  return GlobalAddress{
      .global_name = base_address.global_name,
      .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                        ? leaf_layout.scalar_type
                        : bir::TypeKind::Void,
      .byte_offset = byte_offset,
  };
}

std::optional<DynamicGlobalPointerArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_pointer_array_access(
    std::string_view global_name,
    std::string_view base_type_text,
    std::size_t initial_byte_offset,
    bool relative_base,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls) {
  if (gep.indices.empty()) {
    return std::nullopt;
  }

  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = initial_byte_offset;
  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid || layout.size_bytes == 0) {
      return std::nullopt;
    }

    if (index_pos == 0 && !relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    if (index_pos == 0 && relative_base) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value < 0) {
        return std::nullopt;
      }
      byte_offset += static_cast<std::size_t>(*index_value) * layout.size_bytes;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 ||
              static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value() || index_pos + 1 != gep.indices.size() ||
            element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            element_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }
        return DynamicGlobalPointerArrayAccess{
            .global_name = std::string(global_name),
            .byte_offset = byte_offset,
            .element_count = layout.array_count,
            .index = *lowered_index,
        };
      }
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<DynamicGlobalAggregateArrayAccess>
BirFunctionLowerer::resolve_global_dynamic_aggregate_array_access(
    const GlobalAddress& base_address,
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const GlobalTypes& global_types,
    const TypeDeclMap& type_decls) {
  if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto base_layout =
      compute_aggregate_type_layout(c4c::codegen::lir::trim_lir_arg_text(base_type_text), type_decls);
  if (base_layout.kind != AggregateTypeLayout::Kind::Struct &&
      base_layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices.front());
  if (!parsed_index.has_value() ||
      resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
    return std::nullopt;
  }

  const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
  if (!lowered_index.has_value()) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(base_address.global_name);
  if (global_it == global_types.end()) {
    return std::nullopt;
  }

  const auto extent = find_repeated_aggregate_extent_at_offset(
      global_it->second.type_text, base_address.byte_offset, base_type_text, type_decls);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicGlobalAggregateArrayAccess{
      .global_name = base_address.global_name,
      .element_type_text =
          std::string(c4c::codegen::lir::trim_lir_arg_text(base_type_text)),
      .byte_offset = base_address.byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .index = *lowered_index,
  };
}

void BirFunctionLowerer::record_pointer_global_object_alias(
    std::string_view result_name,
    const GlobalInfo& global_info,
    const GlobalTypes& global_types,
    GlobalObjectPointerMap& global_object_pointer_slots) {
  if (global_info.initializer_symbol_name.empty() ||
      global_info.initializer_offset_type != bir::TypeKind::Void ||
      global_info.initializer_byte_offset != 0) {
    return;
  }

  const auto pointee_it = global_types.find(global_info.initializer_symbol_name);
  if (pointee_it == global_types.end() || !pointee_it->second.supports_direct_value ||
      pointee_it->second.value_type != bir::TypeKind::Ptr) {
    return;
  }

  global_object_pointer_slots[std::string(result_name)] = GlobalAddress{
      .global_name = global_info.initializer_symbol_name,
      .value_type = bir::TypeKind::Ptr,
      .byte_offset = 0,
  };
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_pointer_store_address(
    const c4c::codegen::lir::LirOperand& operand,
    const GlobalPointerMap& global_pointer_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
    const std::string global_name = operand.str().substr(1);
    const auto global_it = global_types.find(global_name);
    if (global_it == global_types.end()) {
      if (!is_known_function_symbol(global_name, function_symbols)) {
        return std::nullopt;
      }
      return GlobalAddress{
          .global_name = global_name,
          .value_type = bir::TypeKind::Ptr,
          .byte_offset = 0,
      };
    }
    if (!global_it->second.supports_linear_addressing) {
      return std::nullopt;
    }
    return GlobalAddress{
        .global_name = global_name,
        .value_type = global_it->second.value_type,
        .byte_offset = 0,
    };
  }

  if (operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
    return std::nullopt;
  }

  const auto global_ptr_it = global_pointer_slots.find(operand.str());
  if (global_ptr_it == global_pointer_slots.end()) {
    return std::nullopt;
  }
  return global_ptr_it->second;
}

std::optional<std::string> BirFunctionLowerer::resolve_local_aggregate_gep_slot(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (layout.kind == AggregateTypeLayout::Kind::Scalar) {
        if (*index_value != 0) {
          return std::nullopt;
        }
        continue;
      }
      if (*index_value != 0) {
        return std::nullopt;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto leaf_layout = compute_aggregate_type_layout(current_type, type_decls);
  if (leaf_layout.kind != AggregateTypeLayout::Kind::Scalar) {
    return std::nullopt;
  }

  const auto slot_it = aggregate_slots.leaf_slots.find(byte_offset);
  if (slot_it == aggregate_slots.leaf_slots.end()) {
    return std::nullopt;
  }
  return slot_it->second;
}

std::optional<std::vector<std::string>>
BirFunctionLowerer::resolve_local_aggregate_pointer_array_slots(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (*index_value != 0) {
        const auto extent = find_repeated_aggregate_extent_at_offset(
            aggregate_slots.storage_type_text,
            aggregate_slots.base_byte_offset,
            base_type_text,
            type_decls);
        if (!extent.has_value() ||
            static_cast<std::size_t>(*index_value) >= extent->element_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * extent->element_stride_bytes;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        if (element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
            element_layout.size_bytes != 0 && *index_value == 0 &&
            &raw_index == &gep.indices.back()) {
          std::vector<std::string> element_slots;
          element_slots.reserve(layout.array_count);
          for (std::size_t index = 0; index < layout.array_count; ++index) {
            const auto slot_it =
                aggregate_slots.leaf_slots.find(byte_offset + index * element_layout.size_bytes);
            if (slot_it == aggregate_slots.leaf_slots.end()) {
              return std::nullopt;
            }
            element_slots.push_back(slot_it->second);
          }
          return element_slots;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  const auto layout = compute_aggregate_type_layout(current_type, type_decls);
  if (layout.kind != AggregateTypeLayout::Kind::Array) {
    return std::nullopt;
  }
  const auto element_layout = compute_aggregate_type_layout(layout.element_type_text, type_decls);
  if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
      element_layout.scalar_type != bir::TypeKind::Ptr || element_layout.size_bytes == 0) {
    return std::nullopt;
  }

  std::vector<std::string> element_slots;
  element_slots.reserve(layout.array_count);
  for (std::size_t index = 0; index < layout.array_count; ++index) {
    const auto slot_it =
        aggregate_slots.leaf_slots.find(byte_offset + index * element_layout.size_bytes);
    if (slot_it == aggregate_slots.leaf_slots.end()) {
      return std::nullopt;
    }
    element_slots.push_back(slot_it->second);
  }
  return element_slots;
}

std::optional<DynamicLocalPointerArrayAccess>
BirFunctionLowerer::resolve_local_aggregate_dynamic_pointer_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (std::size_t index_pos = 0; index_pos < gep.indices.size(); ++index_pos) {
    const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
      if (!index_value.has_value() || *index_value != 0) {
        return std::nullopt;
      }
      saw_base_index = true;
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            element_layout.size_bytes == 0) {
          return std::nullopt;
        }

        if (const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
            index_value.has_value()) {
          if (*index_value < 0 ||
              static_cast<std::size_t>(*index_value) >= layout.array_count) {
            return std::nullopt;
          }
          byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
          current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
          break;
        }

        const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
        if (!lowered_index.has_value()) {
          return std::nullopt;
        }

        std::size_t element_leaf_offset = 0;
        std::string_view element_type =
            c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        for (std::size_t tail_pos = index_pos + 1; tail_pos < gep.indices.size(); ++tail_pos) {
          const auto tail_index = parse_typed_operand(gep.indices[tail_pos]);
          if (!tail_index.has_value()) {
            return std::nullopt;
          }
          const auto tail_value = resolve_index_operand(tail_index->operand, value_aliases);
          if (!tail_value.has_value() || *tail_value < 0) {
            return std::nullopt;
          }

          const auto tail_layout = compute_aggregate_type_layout(element_type, type_decls);
          if (tail_layout.kind == AggregateTypeLayout::Kind::Invalid ||
              tail_layout.size_bytes == 0 || tail_layout.align_bytes == 0) {
            return std::nullopt;
          }

          switch (tail_layout.kind) {
            case AggregateTypeLayout::Kind::Array: {
              const auto nested_layout =
                  compute_aggregate_type_layout(tail_layout.element_type_text, type_decls);
              if (nested_layout.kind == AggregateTypeLayout::Kind::Invalid ||
                  static_cast<std::size_t>(*tail_value) >= tail_layout.array_count) {
                return std::nullopt;
              }
              element_leaf_offset +=
                  static_cast<std::size_t>(*tail_value) * nested_layout.size_bytes;
              element_type =
                  c4c::codegen::lir::trim_lir_arg_text(tail_layout.element_type_text);
              break;
            }
            case AggregateTypeLayout::Kind::Struct:
              if (static_cast<std::size_t>(*tail_value) >= tail_layout.fields.size()) {
                return std::nullopt;
              }
              element_leaf_offset +=
                  tail_layout.fields[static_cast<std::size_t>(*tail_value)].byte_offset;
              element_type = c4c::codegen::lir::trim_lir_arg_text(
                  tail_layout.fields[static_cast<std::size_t>(*tail_value)].type_text);
              break;
            default:
              return std::nullopt;
          }
        }

        const auto final_layout = compute_aggregate_type_layout(element_type, type_decls);
        if (final_layout.kind != AggregateTypeLayout::Kind::Scalar ||
            final_layout.scalar_type != bir::TypeKind::Ptr) {
          return std::nullopt;
        }

        std::vector<std::string> element_slots;
        element_slots.reserve(layout.array_count);
        for (std::size_t element_index = 0; element_index < layout.array_count; ++element_index) {
          const auto slot_it = aggregate_slots.leaf_slots.find(
              byte_offset + element_index * element_layout.size_bytes + element_leaf_offset);
          if (slot_it == aggregate_slots.leaf_slots.end()) {
            return std::nullopt;
          }
          element_slots.push_back(slot_it->second);
        }
        return DynamicLocalPointerArrayAccess{
            .element_slots = std::move(element_slots),
            .index = *lowered_index,
        };
      }
      case AggregateTypeLayout::Kind::Struct: {
        const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
        if (!index_value.has_value() || *index_value < 0 ||
            static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      }
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<BirFunctionLowerer::LocalAggregateGepTarget>
BirFunctionLowerer::resolve_local_aggregate_gep_target(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::string_view current_type = c4c::codegen::lir::trim_lir_arg_text(base_type_text);
  std::size_t byte_offset = aggregate_slots.base_byte_offset;
  bool saw_base_index = false;

  for (const auto& raw_index : gep.indices) {
    const auto parsed_index = parse_typed_operand(raw_index);
    if (!parsed_index.has_value()) {
      return std::nullopt;
    }
    const auto index_value = resolve_index_operand(parsed_index->operand, value_aliases);
    if (!index_value.has_value() || *index_value < 0) {
      return std::nullopt;
    }

    const auto layout = compute_aggregate_type_layout(current_type, type_decls);
    if (layout.kind == AggregateTypeLayout::Kind::Invalid ||
        layout.size_bytes == 0 || layout.align_bytes == 0) {
      return std::nullopt;
    }

    if (!saw_base_index) {
      saw_base_index = true;
      if (*index_value != 0) {
        const auto extent = find_repeated_aggregate_extent_at_offset(
            aggregate_slots.storage_type_text,
            aggregate_slots.base_byte_offset,
            base_type_text,
            type_decls);
        if (!extent.has_value() ||
            static_cast<std::size_t>(*index_value) >= extent->element_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * extent->element_stride_bytes;
      }
      continue;
    }

    switch (layout.kind) {
      case AggregateTypeLayout::Kind::Array: {
        const auto element_layout =
            compute_aggregate_type_layout(layout.element_type_text, type_decls);
        if (element_layout.kind == AggregateTypeLayout::Kind::Invalid ||
            static_cast<std::size_t>(*index_value) >= layout.array_count) {
          return std::nullopt;
        }
        byte_offset += static_cast<std::size_t>(*index_value) * element_layout.size_bytes;
        current_type = c4c::codegen::lir::trim_lir_arg_text(layout.element_type_text);
        break;
      }
      case AggregateTypeLayout::Kind::Struct:
        if (static_cast<std::size_t>(*index_value) >= layout.fields.size()) {
          return std::nullopt;
        }
        byte_offset += layout.fields[static_cast<std::size_t>(*index_value)].byte_offset;
        current_type = c4c::codegen::lir::trim_lir_arg_text(
            layout.fields[static_cast<std::size_t>(*index_value)].type_text);
        break;
      default:
        return std::nullopt;
    }
  }

  return LocalAggregateGepTarget{
      .type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(current_type)),
      .byte_offset = byte_offset,
  };
}

std::optional<DynamicLocalAggregateArrayAccess>
BirFunctionLowerer::resolve_local_dynamic_aggregate_array_access(
    std::string_view base_type_text,
    const c4c::codegen::lir::LirGepOp& gep,
    const ValueMap& value_aliases,
    const TypeDeclMap& type_decls,
    const LocalAggregateSlots& aggregate_slots) {
  std::size_t index_pos = 0;
  if (gep.indices.size() == 2) {
    const auto base_index = parse_typed_operand(gep.indices.front());
    if (!base_index.has_value()) {
      return std::nullopt;
    }
    const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
    if (!base_imm.has_value() || *base_imm != 0) {
      return std::nullopt;
    }
    index_pos = 1;
  } else if (gep.indices.size() != 1) {
    return std::nullopt;
  }

  const auto parsed_index = parse_typed_operand(gep.indices[index_pos]);
  if (!parsed_index.has_value() ||
      resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
    return std::nullopt;
  }

  const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
  if (!lowered_index.has_value()) {
    return std::nullopt;
  }

  const auto extent = find_repeated_aggregate_extent_at_offset(
      aggregate_slots.storage_type_text,
      aggregate_slots.base_byte_offset,
      base_type_text,
      type_decls);
  if (!extent.has_value()) {
    return std::nullopt;
  }

  return DynamicLocalAggregateArrayAccess{
      .element_type_text = std::string(c4c::codegen::lir::trim_lir_arg_text(base_type_text)),
      .byte_offset = aggregate_slots.base_byte_offset,
      .element_count = extent->element_count,
      .element_stride_bytes = extent->element_stride_bytes,
      .leaf_slots = aggregate_slots.leaf_slots,
      .index = *lowered_index,
  };
}

std::optional<bir::Value> BirFunctionLowerer::resolve_local_aggregate_pointer_value_alias(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    if (local_aggregate_slots.find(operand.str()) != local_aggregate_slots.end()) {
      return bir::Value::named(bir::TypeKind::Ptr, operand.str());
    }
    return BirFunctionLowerer::lower_value(operand, bir::TypeKind::Ptr, value_aliases);
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return std::nullopt;
  }

  const std::string symbol_name = operand.str().substr(1);
  if (!is_known_function_symbol(symbol_name, function_symbols)) {
    return std::nullopt;
  }
  return bir::Value::named(bir::TypeKind::Ptr, "@" + symbol_name);
}

std::optional<bir::Value> BirFunctionLowerer::lower_call_pointer_arg_value(
    const c4c::codegen::lir::LirOperand& operand,
    const ValueMap& value_aliases,
    const LocalAggregateSlotMap& local_aggregate_slots,
    const GlobalTypes& global_types,
    const FunctionSymbolSet& function_symbols) {
  if (operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
    if (local_aggregate_slots.find(operand.str()) != local_aggregate_slots.end()) {
      return bir::Value::named(bir::TypeKind::Ptr, operand.str());
    }
    return BirFunctionLowerer::lower_value(operand, bir::TypeKind::Ptr, value_aliases);
  }
  if (operand.kind() != c4c::codegen::lir::LirOperandKind::Global) {
    return std::nullopt;
  }

  const std::string symbol_name = operand.str().substr(1);
  if (!is_known_function_symbol(symbol_name, function_symbols) &&
      global_types.find(symbol_name) == global_types.end()) {
    return std::nullopt;
  }
  return bir::Value::named(bir::TypeKind::Ptr, "@" + symbol_name);
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_honest_pointer_base(
    const GlobalAddress& address,
    const GlobalTypes& global_types) {
  if (address.value_type != bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  const auto global_it = global_types.find(address.global_name);
  if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
      global_it->second.value_type != bir::TypeKind::Ptr ||
      !global_it->second.known_global_address.has_value()) {
    return std::nullopt;
  }

  auto resolved = *global_it->second.known_global_address;
  resolved.byte_offset += address.byte_offset;

  const auto base_it = global_types.find(resolved.global_name);
  if (base_it == global_types.end() || !base_it->second.supports_linear_addressing ||
      resolved.byte_offset >= base_it->second.storage_size_bytes) {
    return std::nullopt;
  }

  return resolved;
}

std::optional<GlobalAddress> BirFunctionLowerer::resolve_honest_addressed_global_access(
    const GlobalAddress& address,
    bir::TypeKind accessed_type,
    const GlobalTypes& global_types) {
  if (accessed_type == bir::TypeKind::Ptr) {
    return std::nullopt;
  }

  auto resolved = resolve_honest_pointer_base(address, global_types);
  if (!resolved.has_value()) {
    return std::nullopt;
  }
  resolved->value_type = accessed_type;
  return resolved;
}

GlobalPointerSlotKey BirFunctionLowerer::make_global_pointer_slot_key(const GlobalAddress& address) {
  return GlobalPointerSlotKey{
      .global_name = address.global_name,
      .byte_offset = address.byte_offset,
  };
}

bool BirFunctionLowerer::ensure_local_scratch_slot(std::string_view slot_name,
                                                   bir::TypeKind type,
                                                   std::size_t align_bytes) {
  const std::string owned_slot_name(slot_name);
  const auto slot_it = local_slot_types_.find(owned_slot_name);
  if (slot_it != local_slot_types_.end()) {
    return slot_it->second == type;
  }

  const auto slot_size = type_size_bytes(type);
  if (slot_size == 0) {
    return false;
  }

  local_slot_types_.emplace(owned_slot_name, type);
  lowered_function_.local_slots.push_back(bir::LocalSlot{
      .name = owned_slot_name,
      .type = type,
      .size_bytes = slot_size,
      .align_bytes = align_bytes == 0 ? slot_size : align_bytes,
  });
  return true;
}

bool BirFunctionLowerer::lower_scalar_or_local_memory_inst(
    const c4c::codegen::lir::LirInst& inst,
    std::vector<bir::Inst>* lowered_insts) {
  auto& value_aliases = value_aliases_;
  auto& compare_exprs = compare_exprs_;
  auto& aggregate_value_aliases = aggregate_value_aliases_;
  auto& local_slot_types = local_slot_types_;
  auto& local_pointer_slots = local_pointer_slots_;
  auto& local_array_slots = local_array_slots_;
  auto& local_pointer_array_bases = local_pointer_array_bases_;
  auto& dynamic_local_pointer_arrays = dynamic_local_pointer_arrays_;
  auto& dynamic_local_aggregate_arrays = dynamic_local_aggregate_arrays_;
  auto& dynamic_pointer_value_arrays = dynamic_pointer_value_arrays_;
  auto& local_aggregate_slots = local_aggregate_slots_;
  auto& local_aggregate_field_slots = local_aggregate_field_slots_;
  auto& local_pointer_value_aliases = local_pointer_value_aliases_;
  auto& pointer_value_addresses = pointer_value_addresses_;
  auto& local_address_slots = local_address_slots_;
  auto& local_slot_address_slots = local_slot_address_slots_;
  auto& local_slot_pointer_values = local_slot_pointer_values_;
  auto& global_address_slots = global_address_slots_;
  auto& addressed_global_pointer_slots = addressed_global_pointer_slots_;
  auto& global_pointer_slots = global_pointer_slots_;
  auto& dynamic_global_pointer_arrays = dynamic_global_pointer_arrays_;
  auto& dynamic_global_aggregate_arrays = dynamic_global_aggregate_arrays_;
  auto& global_object_pointer_slots = global_object_pointer_slots_;
  auto& global_address_ints = global_address_ints_;
  auto& global_object_address_ints = global_object_address_ints_;
  const auto& aggregate_params = aggregate_params_;
  const auto& global_types = global_types_;
  const auto& function_symbols = function_symbols_;
  const auto& type_decls = type_decls_;
  auto* lowered_function = &lowered_function_;
  if (lower_scalar_compare_inst(inst, value_aliases, compare_exprs, lowered_insts)) {
    return true;
  }

  if (const auto* cast = std::get_if<c4c::codegen::lir::LirCastOp>(&inst)) {
    if (cast->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::PtrToInt &&
        cast->to_type.str() == "i64") {
      if (cast->operand.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = cast->operand.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end()) {
          return false;
        }
        if (global_it->second.supports_linear_addressing) {
          global_address_ints[cast->result.str()] = GlobalAddress{
              .global_name = global_name,
              .value_type = global_it->second.value_type,
              .byte_offset = 0,
          };
          return true;
        }
        if (!global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        global_object_address_ints[cast->result.str()] = GlobalAddress{
            .global_name = global_name,
            .value_type = bir::TypeKind::Ptr,
            .byte_offset = 0,
        };
        return true;
      }

      if (cast->operand.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }

      const auto global_object_it = global_object_pointer_slots.find(cast->operand.str());
      if (global_object_it != global_object_pointer_slots.end()) {
        global_object_address_ints[cast->result.str()] = global_object_it->second;
        return true;
      }

      const auto global_ptr_it = global_pointer_slots.find(cast->operand.str());
      if (global_ptr_it == global_pointer_slots.end()) {
        return false;
      }
      global_address_ints[cast->result.str()] = global_ptr_it->second;
      return true;
    }

    if (cast->kind == c4c::codegen::lir::LirCastKind::IntToPtr &&
        cast->from_type.str() == "i64" && cast->to_type.str() == "ptr" &&
        cast->operand.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto global_object_addr_it = global_object_address_ints.find(cast->operand.str());
      if (global_object_addr_it != global_object_address_ints.end()) {
        global_object_pointer_slots[cast->result.str()] = global_object_addr_it->second;
        return true;
      }

      const auto global_addr_it = global_address_ints.find(cast->operand.str());
      if (global_addr_it == global_address_ints.end()) {
        return false;
      }
      global_pointer_slots[cast->result.str()] = global_addr_it->second;
      return true;
    }

    const auto opcode = lower_cast_opcode(cast->kind);
    const auto from_type = lower_integer_type(cast->from_type.str());
    const auto to_type = lower_integer_type(cast->to_type.str());
    if (!opcode.has_value() || !from_type.has_value() || !to_type.has_value()) {
      return false;
    }

    const auto operand = lower_value(cast->operand, *from_type, value_aliases);
    if (!operand.has_value()) {
      return false;
    }

    lowered_insts->push_back(bir::CastInst{
        .opcode = *opcode,
        .result = bir::Value::named(*to_type, cast->result.str()),
        .operand = *operand,
    });
    return true;
  }

  if (const auto* bin = std::get_if<c4c::codegen::lir::LirBinOp>(&inst)) {
    if (bin->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const auto opcode = lower_scalar_binary_opcode(bin->opcode);
    const auto value_type = lower_integer_type(bin->type_str.str());
    if (!opcode.has_value() || !value_type.has_value()) {
      return false;
    }

    if (*opcode == bir::BinaryOpcode::Sub && *value_type == bir::TypeKind::I64 &&
        bin->lhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue &&
        bin->rhs.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto lhs_addr_it = global_address_ints.find(bin->lhs.str());
      const auto rhs_addr_it = global_address_ints.find(bin->rhs.str());
      if (lhs_addr_it != global_address_ints.end() && rhs_addr_it != global_address_ints.end() &&
          lhs_addr_it->second.global_name == rhs_addr_it->second.global_name) {
        const auto lhs_offset = static_cast<std::int64_t>(lhs_addr_it->second.byte_offset);
        const auto rhs_offset = static_cast<std::int64_t>(rhs_addr_it->second.byte_offset);
        value_aliases[bin->result.str()] = bir::Value::immediate_i64(lhs_offset - rhs_offset);
        return true;
      }
    }

    const auto lhs = lower_value(bin->lhs, *value_type, value_aliases);
    const auto rhs = lower_value(bin->rhs, *value_type, value_aliases);
    if (!lhs.has_value() || !rhs.has_value()) {
      return false;
    }

    if (*value_type == bir::TypeKind::I64 && lhs->kind == bir::Value::Kind::Immediate &&
        rhs->kind == bir::Value::Kind::Immediate) {
      const auto folded = fold_i64_binary_immediates(*opcode, lhs->immediate, rhs->immediate);
      if (folded.has_value()) {
        value_aliases[bin->result.str()] = *folded;
        return true;
      }
    }

    lowered_insts->push_back(bir::BinaryInst{
        .opcode = *opcode,
        .result = bir::Value::named(*value_type, bin->result.str()),
        .operand_type = *value_type,
        .lhs = *lhs,
        .rhs = *rhs,
    });
    return true;
  }

  if (lower_runtime_intrinsic_inst(inst, value_aliases, lowered_insts)) {
    return true;
  }

  if (const auto* alloca = std::get_if<c4c::codegen::lir::LirAllocaOp>(&inst)) {
    if (alloca->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
      return false;
    }

    const std::string slot_name = alloca->result.str();
    if (!alloca->count.str().empty()) {
      if (!context_.options.preserve_dynamic_alloca) {
        return false;
      }
      const auto element_type = lower_scalar_or_function_pointer_type(alloca->type_str.str());
      const auto lowered_count = lower_value(alloca->count, bir::TypeKind::I64, value_aliases);
      if (!element_type.has_value() || !lowered_count.has_value()) {
        return false;
      }
      const auto type_text =
          std::string(c4c::codegen::lir::trim_lir_arg_text(alloca->type_str.str()));
      lowered_insts->push_back(bir::CallInst{
          .result = bir::Value::named(bir::TypeKind::Ptr, slot_name),
          .callee = "llvm.dynamic_alloca." + type_text,
          .args = {*lowered_count},
          .arg_types = {bir::TypeKind::I64},
          .return_type = bir::TypeKind::Ptr,
      });
      pointer_value_addresses[slot_name] = PointerAddress{
          .base_value = bir::Value::named(bir::TypeKind::Ptr, slot_name),
          .value_type = *element_type,
          .byte_offset = 0,
          .storage_type_text = type_text,
          .type_text = type_text,
      };
      return true;
    }

    const auto slot_type = lower_scalar_or_function_pointer_type(alloca->type_str.str());
    if (local_slot_types.find(slot_name) != local_slot_types.end() ||
        local_array_slots.find(slot_name) != local_array_slots.end()) {
      return false;
    }

    if (slot_type.has_value()) {
      local_slot_types.emplace(slot_name, *slot_type);
      local_pointer_slots.emplace(slot_name, slot_name);
      lowered_function->local_slots.push_back(bir::LocalSlot{
          .name = slot_name,
          .type = *slot_type,
          .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
      });
      return true;
    }

    const auto array_type = parse_local_array_type(alloca->type_str.str());
    if (array_type.has_value()) {
      LocalArraySlots array_slots{.element_type = array_type->second};
      array_slots.element_slots.reserve(array_type->first);
      for (std::size_t index = 0; index < array_type->first; ++index) {
        const std::string element_slot = slot_name + "." + std::to_string(index);
        local_slot_types.emplace(element_slot, array_type->second);
        local_pointer_slots.emplace(element_slot, element_slot);
        array_slots.element_slots.push_back(element_slot);
        lowered_function->local_slots.push_back(bir::LocalSlot{
            .name = element_slot,
            .type = array_type->second,
            .align_bytes = alloca->align > 0 ? static_cast<std::size_t>(alloca->align) : 0,
        });
      }
      local_array_slots.emplace(slot_name, std::move(array_slots));
      return true;
    }

    return declare_local_aggregate_slots(alloca->type_str.str(),
                                         slot_name,
                                         alloca->align > 0 ? static_cast<std::size_t>(alloca->align)
                                                           : 0);
  }

  if (const auto* gep = std::get_if<c4c::codegen::lir::LirGepOp>(&inst)) {
    if (gep->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         gep->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    std::string resolved_slot;
    if (gep->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = gep->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_linear_addressing) {
        return false;
      }
      const auto resolved_address = resolve_global_gep_address(
          global_name, global_it->second.type_text, *gep, value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          global_name, global_it->second.type_text, 0, false, *gep, value_aliases, type_decls);
      if (!dynamic_array.has_value()) {
        return false;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    }

    if (const auto aggregate_it = local_aggregate_slots.find(gep->ptr.str());
        aggregate_it != local_aggregate_slots.end()) {
      bool established_aggregate_subobject = false;
      const auto resolved_target = resolve_local_aggregate_gep_target(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (resolved_target.has_value()) {
        const auto target_layout =
            compute_aggregate_type_layout(resolved_target->type_text, type_decls);
        if (target_layout.kind == AggregateTypeLayout::Kind::Struct ||
            target_layout.kind == AggregateTypeLayout::Kind::Array) {
          local_aggregate_slots[gep->result.str()] = LocalAggregateSlots{
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = std::move(resolved_target->type_text),
              .base_byte_offset = resolved_target->byte_offset,
              .leaf_slots = aggregate_it->second.leaf_slots,
          };
          established_aggregate_subobject = true;
        }
      }
      const auto pointer_array_slots = resolve_local_aggregate_pointer_array_slots(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (pointer_array_slots.has_value()) {
        const auto resolved_slot = resolve_local_aggregate_gep_slot(
            aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
        if (resolved_slot.has_value()) {
          local_pointer_slots[gep->result.str()] = *resolved_slot;
        }
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = std::move(*pointer_array_slots),
            .base_index = 0,
        };
        return true;
      }
      const auto resolved_slot = resolve_local_aggregate_gep_slot(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (resolved_slot.has_value()) {
        local_pointer_slots[gep->result.str()] = *resolved_slot;
        return true;
      }
      if (established_aggregate_subobject) {
        return true;
      }
      const auto dynamic_array = resolve_local_aggregate_dynamic_pointer_array_access(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (dynamic_array.has_value()) {
        dynamic_local_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
        return true;
      }
      const auto dynamic_aggregate = resolve_local_dynamic_aggregate_array_access(
          aggregate_it->second.type_text, *gep, value_aliases, type_decls, aggregate_it->second);
      if (!dynamic_aggregate.has_value()) {
        return false;
      }
      const auto element_layout =
          compute_aggregate_type_layout(dynamic_aggregate->element_type_text, type_decls);
      if (element_layout.kind == AggregateTypeLayout::Kind::Struct ||
          element_layout.kind == AggregateTypeLayout::Kind::Array) {
        std::vector<bir::Value> element_values;
        element_values.reserve(dynamic_aggregate->element_count);
        for (std::size_t element_index = 0;
             element_index < dynamic_aggregate->element_count;
             ++element_index) {
          const std::string element_name =
              gep->result.str() + ".elt" + std::to_string(element_index);
          local_aggregate_slots[element_name] = LocalAggregateSlots{
              .storage_type_text = aggregate_it->second.storage_type_text,
              .type_text = dynamic_aggregate->element_type_text,
              .base_byte_offset =
                  dynamic_aggregate->byte_offset +
                  element_index * dynamic_aggregate->element_stride_bytes,
              .leaf_slots = aggregate_it->second.leaf_slots,
          };
          element_values.push_back(bir::Value::named(bir::TypeKind::Ptr, element_name));
        }
        const auto selected_value = synthesize_pointer_array_selects(
            gep->result.str(), element_values, dynamic_aggregate->index, lowered_insts);
        if (!selected_value.has_value()) {
          return false;
        }
        if (selected_value->kind != bir::Value::Kind::Named ||
            selected_value->name != gep->result.str()) {
          value_aliases[gep->result.str()] = *selected_value;
        }
      }
      dynamic_local_aggregate_arrays[gep->result.str()] = std::move(*dynamic_aggregate);
      return true;
    }

    if (const auto array_it = local_array_slots.find(gep->ptr.str()); array_it != local_array_slots.end()) {
      if (gep->indices.size() != 2) {
        return false;
      }
      const auto base_index = parse_typed_operand(gep->indices[0]);
      const auto elem_index = parse_typed_operand(gep->indices[1]);
      if (!base_index.has_value() || !elem_index.has_value()) {
        return false;
      }
      const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
      const auto elem_imm = resolve_index_operand(elem_index->operand, value_aliases);
      if (!base_imm.has_value() || *base_imm != 0) {
        return false;
      }
      if (elem_imm.has_value()) {
        if (*elem_imm < 0 ||
            static_cast<std::size_t>(*elem_imm) >= array_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot = array_it->second.element_slots[static_cast<std::size_t>(*elem_imm)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_it->second.element_slots,
            .base_index = static_cast<std::size_t>(*elem_imm),
        };
      } else {
        const auto elem_value = lower_typed_index_value(*elem_index, value_aliases);
        if (!elem_value.has_value() || array_it->second.element_type != bir::TypeKind::Ptr) {
          return false;
        }
        dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
            .element_slots = array_it->second.element_slots,
            .index = *elem_value,
        };
        return true;
      }
    } else if (const auto array_base_it = local_pointer_array_bases.find(gep->ptr.str());
               array_base_it != local_pointer_array_bases.end() &&
               local_pointer_slots.find(gep->ptr.str()) == local_pointer_slots.end()) {
      if (gep->indices.empty() || gep->indices.size() > 2) {
        return false;
      }
      std::size_t index_pos = 0;
      if (gep->indices.size() == 2) {
        const auto base_index = parse_typed_operand(gep->indices[0]);
        if (!base_index.has_value()) {
          return false;
        }
        const auto base_imm = resolve_index_operand(base_index->operand, value_aliases);
        if (!base_imm.has_value() || *base_imm != 0) {
          return false;
        }
        index_pos = 1;
      }
      const auto parsed_index = parse_typed_operand(gep->indices[index_pos]);
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);
      if (index_imm.has_value()) {
        const auto final_index = static_cast<std::int64_t>(array_base_it->second.base_index) + *index_imm;
        if (final_index < 0 ||
            static_cast<std::size_t>(final_index) >= array_base_it->second.element_slots.size()) {
          return false;
        }
        resolved_slot =
            array_base_it->second.element_slots[static_cast<std::size_t>(final_index)];
        local_pointer_array_bases[gep->result.str()] = LocalPointerArrayBase{
            .element_slots = array_base_it->second.element_slots,
            .base_index = static_cast<std::size_t>(final_index),
        };
      } else {
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value() || array_base_it->second.base_index != 0) {
          return false;
        }
        if (array_base_it->second.element_slots.empty()) {
          return false;
        }
        const auto slot_type_it = local_slot_types.find(array_base_it->second.element_slots.front());
        if (slot_type_it == local_slot_types.end()) {
          return false;
        }
        if (slot_type_it->second == bir::TypeKind::Ptr) {
          dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
              .element_slots = array_base_it->second.element_slots,
              .index = *index_value,
          };
          return true;
        }

        const auto element_size = type_size_bytes(slot_type_it->second);
        if (element_size == 0) {
          return false;
        }

        std::optional<std::string> element_type_text;
        switch (slot_type_it->second) {
          case bir::TypeKind::I1:
            element_type_text = "i1";
            break;
          case bir::TypeKind::I8:
            element_type_text = "i8";
            break;
          case bir::TypeKind::I32:
            element_type_text = "i32";
            break;
          case bir::TypeKind::I64:
            element_type_text = "i64";
            break;
          default:
            return false;
        }

        DynamicLocalAggregateArrayAccess access{
            .element_type_text = *element_type_text,
            .byte_offset = 0,
            .element_count = array_base_it->second.element_slots.size(),
            .element_stride_bytes = element_size,
        };
        for (std::size_t element_index = 0;
             element_index < array_base_it->second.element_slots.size();
             ++element_index) {
          const auto& slot_name = array_base_it->second.element_slots[element_index];
          const auto element_slot_type_it = local_slot_types.find(slot_name);
          if (element_slot_type_it == local_slot_types.end() ||
              element_slot_type_it->second != slot_type_it->second) {
            return false;
          }
          access.leaf_slots.emplace(element_index * element_size, slot_name);
        }
        dynamic_local_aggregate_arrays[gep->result.str()] = std::move(access);
        return true;
      }
    } else if (const auto global_ptr_it = global_pointer_slots.find(gep->ptr.str());
               global_ptr_it != global_pointer_slots.end()) {
      auto base_address = global_ptr_it->second;
      if (const auto honest_base = resolve_honest_pointer_base(base_address, global_types);
          honest_base.has_value()) {
        base_address = *honest_base;
      }
      const auto resolved_address = resolve_relative_global_gep_address(
          base_address, gep->element_type.str(), *gep, value_aliases, type_decls);
      if (resolved_address.has_value()) {
        global_pointer_slots[gep->result.str()] = *resolved_address;
        return true;
      }
      const auto dynamic_array = resolve_global_dynamic_pointer_array_access(
          base_address.global_name,
          gep->element_type.str(),
          base_address.byte_offset,
          true,
          *gep,
          value_aliases,
          type_decls);
      if (!dynamic_array.has_value()) {
        const auto dynamic_aggregate = resolve_global_dynamic_aggregate_array_access(
            base_address, gep->element_type.str(), *gep, value_aliases, global_types, type_decls);
        if (dynamic_aggregate.has_value()) {
          dynamic_global_aggregate_arrays[gep->result.str()] = std::move(*dynamic_aggregate);
          return true;
        }
        if (gep->indices.size() != 1 || base_address.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        const auto parsed_index = parse_typed_operand(gep->indices.front());
        if (!parsed_index.has_value()) {
          return false;
        }
        const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
        if (!index_value.has_value()) {
          return false;
        }
        const auto global_it = global_types.find(base_address.global_name);
        if (global_it == global_types.end()) {
          return false;
        }
        const auto array_length = find_pointer_array_length_at_offset(
            global_it->second.type_text, base_address.byte_offset, type_decls);
        if (!array_length.has_value()) {
          return false;
        }
        dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
            .global_name = base_address.global_name,
            .byte_offset = base_address.byte_offset,
            .element_count = *array_length,
            .index = *index_value,
        };
        return true;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = std::move(*dynamic_array);
      return true;
    } else if (const auto global_aggregate_it = dynamic_global_aggregate_arrays.find(gep->ptr.str());
               global_aggregate_it != dynamic_global_aggregate_arrays.end()) {
      const auto element_leaf = resolve_relative_global_gep_address(
          GlobalAddress{},
          global_aggregate_it->second.element_type_text,
          *gep,
          value_aliases,
          type_decls);
      if (!element_leaf.has_value() || element_leaf->value_type != bir::TypeKind::Ptr) {
        return false;
      }
      dynamic_global_pointer_arrays[gep->result.str()] = DynamicGlobalPointerArrayAccess{
          .global_name = global_aggregate_it->second.global_name,
          .byte_offset = global_aggregate_it->second.byte_offset + element_leaf->byte_offset,
          .element_count = global_aggregate_it->second.element_count,
          .element_stride_bytes = global_aggregate_it->second.element_stride_bytes,
          .index = global_aggregate_it->second.index,
      };
      return true;
    } else if (const auto local_aggregate_it = dynamic_local_aggregate_arrays.find(gep->ptr.str());
               local_aggregate_it != dynamic_local_aggregate_arrays.end()) {
      std::vector<std::string> element_slots;
      element_slots.reserve(local_aggregate_it->second.element_count);
      for (std::size_t element_index = 0;
           element_index < local_aggregate_it->second.element_count;
           ++element_index) {
        LocalAggregateSlots element_aggregate{
            .storage_type_text = local_aggregate_it->second.element_type_text,
            .type_text = local_aggregate_it->second.element_type_text,
            .base_byte_offset =
                local_aggregate_it->second.byte_offset +
                element_index * local_aggregate_it->second.element_stride_bytes,
            .leaf_slots = local_aggregate_it->second.leaf_slots,
        };
        const auto element_slot = resolve_local_aggregate_gep_slot(
            element_aggregate.type_text, *gep, value_aliases, type_decls, element_aggregate);
        if (!element_slot.has_value()) {
          return false;
        }
        element_slots.push_back(*element_slot);
      }
      dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
          .element_slots = std::move(element_slots),
          .index = local_aggregate_it->second.index,
      };
      return true;
    } else if (const auto local_slot_ptr_it = local_slot_pointer_values.find(gep->ptr.str());
               local_slot_ptr_it != local_slot_pointer_values.end()) {
      const auto resolved_target = resolve_relative_gep_target(
          gep->element_type.str(),
          local_slot_ptr_it->second.byte_offset,
          *gep,
          value_aliases,
          type_decls);
      if (!resolved_target.has_value()) {
        return false;
      }
      const auto leaf_layout = compute_aggregate_type_layout(resolved_target->type_text, type_decls);
      local_slot_pointer_values[gep->result.str()] = LocalSlotAddress{
          .slot_name = local_slot_ptr_it->second.slot_name,
          .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                            ? leaf_layout.scalar_type
                            : bir::TypeKind::Void,
          .byte_offset = resolved_target->byte_offset,
          .storage_type_text = local_slot_ptr_it->second.storage_type_text,
          .type_text = std::move(resolved_target->type_text),
      };
      return true;
    } else {
      const auto addressed_ptr_it = pointer_value_addresses.find(gep->ptr.str());
      const auto ptr_it = local_pointer_slots.find(gep->ptr.str());
      if (addressed_ptr_it != pointer_value_addresses.end() || ptr_it == local_pointer_slots.end()) {
        const auto base_pointer = addressed_ptr_it != pointer_value_addresses.end()
                                      ? std::optional<bir::Value>(addressed_ptr_it->second.base_value)
                                      : lower_value(gep->ptr, bir::TypeKind::Ptr, value_aliases);
        if (base_pointer.has_value()) {
          const auto base_byte_offset = addressed_ptr_it != pointer_value_addresses.end()
                                            ? addressed_ptr_it->second.byte_offset
                                            : 0;
          const auto resolved_target = resolve_relative_gep_target(
              gep->element_type.str(), base_byte_offset, *gep, value_aliases, type_decls);
          if (resolved_target.has_value()) {
            const auto leaf_layout =
                compute_aggregate_type_layout(resolved_target->type_text, type_decls);
            pointer_value_addresses[gep->result.str()] = PointerAddress{
                .base_value = *base_pointer,
                .value_type = leaf_layout.kind == AggregateTypeLayout::Kind::Scalar
                                  ? leaf_layout.scalar_type
                                  : bir::TypeKind::Void,
                .byte_offset = resolved_target->byte_offset,
                .storage_type_text =
                    addressed_ptr_it != pointer_value_addresses.end()
                        ? addressed_ptr_it->second.storage_type_text
                        : std::string(c4c::codegen::lir::trim_lir_arg_text(gep->element_type.str())),
                .type_text = std::move(resolved_target->type_text),
            };
            return true;
          }

          if (addressed_ptr_it != pointer_value_addresses.end() && gep->indices.size() == 1 &&
              !addressed_ptr_it->second.storage_type_text.empty() &&
              !addressed_ptr_it->second.type_text.empty()) {
            const auto parsed_index = parse_typed_operand(gep->indices.front());
            if (parsed_index.has_value() &&
                !resolve_index_operand(parsed_index->operand, value_aliases).has_value()) {
              const auto lowered_index = lower_typed_index_value(*parsed_index, value_aliases);
              if (lowered_index.has_value()) {
                const auto extent = find_repeated_aggregate_extent_at_offset(
                    addressed_ptr_it->second.storage_type_text,
                    addressed_ptr_it->second.byte_offset,
                    addressed_ptr_it->second.type_text,
                    type_decls);
                const auto element_layout = compute_aggregate_type_layout(
                    addressed_ptr_it->second.type_text, type_decls);
                if (extent.has_value() &&
                    element_layout.kind == AggregateTypeLayout::Kind::Scalar &&
                    element_layout.scalar_type != bir::TypeKind::Void) {
                  dynamic_pointer_value_arrays[gep->result.str()] = DynamicPointerValueArrayAccess{
                      .base_value = addressed_ptr_it->second.base_value,
                      .element_type = element_layout.scalar_type,
                      .byte_offset = addressed_ptr_it->second.byte_offset,
                      .element_count = extent->element_count,
                      .element_stride_bytes = extent->element_stride_bytes,
                      .index = *lowered_index,
                  };
                  return true;
                }
              }
            }
          }
        }
      }
      if (ptr_it == local_pointer_slots.end() || gep->indices.size() != 1) {
        return false;
      }
      const auto slot_it = local_slot_types.find(ptr_it->second);
      if (slot_it == local_slot_types.end()) {
        return false;
      }

      const auto parsed_index = parse_typed_operand(gep->indices.front());
      if (!parsed_index.has_value()) {
        return false;
      }
      const auto index_imm = resolve_index_operand(parsed_index->operand, value_aliases);

      const auto dot = ptr_it->second.rfind('.');
      if (dot == std::string::npos) {
        if (!index_imm.has_value() || *index_imm != 0) {
          return false;
        }
        resolved_slot = ptr_it->second;
      } else {
        const auto base_name = ptr_it->second.substr(0, dot);
        const auto base_array_it = local_array_slots.find(base_name);
        const auto base_offset = parse_i64(std::string_view(ptr_it->second).substr(dot + 1));
        if (!base_offset.has_value()) {
          return false;
        }
        if (base_array_it != local_array_slots.end()) {
          if (index_imm.has_value()) {
            const auto final_index = *base_offset + *index_imm;
            if (final_index < 0 ||
                static_cast<std::size_t>(final_index) >= base_array_it->second.element_slots.size()) {
              return false;
            }
            resolved_slot =
                base_array_it->second.element_slots[static_cast<std::size_t>(final_index)];
          } else {
            const auto parsed_index = parse_typed_operand(gep->indices.front());
            if (!parsed_index.has_value()) {
              return false;
            }
            const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
            if (!index_value.has_value() || *base_offset != 0 ||
                base_array_it->second.element_type != bir::TypeKind::Ptr) {
              return false;
            }
            dynamic_local_pointer_arrays[gep->result.str()] = DynamicLocalPointerArrayAccess{
                .element_slots = base_array_it->second.element_slots,
                .index = *index_value,
            };
            return true;
          }
        } else if (const auto base_aggregate_it = local_aggregate_slots.find(base_name);
                   base_aggregate_it != local_aggregate_slots.end()) {
          const auto slot_size = type_size_bytes(slot_it->second);
          if (slot_size == 0) {
            return false;
          }
          if (!index_imm.has_value()) {
            const auto index_value = lower_typed_index_value(*parsed_index, value_aliases);
            if (!index_value.has_value() || *base_offset < 0) {
              return false;
            }

            const auto extent = find_repeated_aggregate_extent_at_offset(
                base_aggregate_it->second.storage_type_text,
                static_cast<std::size_t>(*base_offset),
                render_type(slot_it->second),
                type_decls);
            if (!extent.has_value()) {
              return false;
            }

            dynamic_local_aggregate_arrays[gep->result.str()] = DynamicLocalAggregateArrayAccess{
                .element_type_text = render_type(slot_it->second),
                .byte_offset = static_cast<std::size_t>(*base_offset),
                .element_count = extent->element_count,
                .element_stride_bytes = extent->element_stride_bytes,
                .leaf_slots = base_aggregate_it->second.leaf_slots,
                .index = *index_value,
            };
            return true;
          }
          const auto final_byte_offset =
              *base_offset + *index_imm * static_cast<std::int64_t>(slot_size);
          if (final_byte_offset < 0) {
            return false;
          }
          const auto leaf_it =
              base_aggregate_it->second.leaf_slots.find(static_cast<std::size_t>(final_byte_offset));
          if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
            return false;
          }
          resolved_slot = leaf_it->second;
        } else {
          return false;
        }
      }
    }

    local_pointer_slots[gep->result.str()] = std::move(resolved_slot);
    return true;
  }

  if (const auto* store = std::get_if<c4c::codegen::lir::LirStoreOp>(&inst)) {
    if (store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
        store->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global) {
      return false;
    }

    const auto value_type = lower_scalar_or_function_pointer_type(store->type_str.str());
    if (!value_type.has_value()) {
      const auto aggregate_layout = lower_byval_aggregate_layout(store->type_str.str(), type_decls);
      if (!aggregate_layout.has_value() ||
          store->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
          store->val.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }

      const auto target_aggregate_it = local_aggregate_slots.find(store->ptr.str());
      if (target_aggregate_it == local_aggregate_slots.end()) {
        return false;
      }

      if (const auto source_param_it = aggregate_params.find(store->val.str());
          source_param_it != aggregate_params.end()) {
        const auto leaf_slots = collect_sorted_leaf_slots(target_aggregate_it->second);
        for (const auto& [byte_offset, slot_name] : leaf_slots) {
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end()) {
            return false;
          }
          const auto slot_size = type_size_bytes(slot_type_it->second);
          if (slot_size == 0) {
            return false;
          }

          const std::string temp_name =
              store->ptr.str() + ".byval.copy." + std::to_string(byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(slot_type_it->second, temp_name),
              .slot_name = slot_name,
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                      .base_value = bir::Value::named(bir::TypeKind::Ptr, store->val.str()),
                      .byte_offset = static_cast<std::int64_t>(byte_offset),
                      .size_bytes = slot_size,
                      .align_bytes = std::max(slot_size, source_param_it->second.layout.align_bytes),
                  },
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = slot_name,
              .value = bir::Value::named(slot_type_it->second, temp_name),
          });
        }
        return true;
      }

      const auto source_alias_it = aggregate_value_aliases.find(store->val.str());
      if (source_alias_it == aggregate_value_aliases.end()) {
        return false;
      }
      const auto source_aggregate_it = local_aggregate_slots.find(source_alias_it->second);
      if (source_aggregate_it == local_aggregate_slots.end()) {
        return false;
      }
      return append_local_aggregate_copy_from_slots(source_aggregate_it->second,
                                                    target_aggregate_it->second,
                                                    store->ptr.str() + ".aggregate.copy",
                                                    lowered_insts);
    }

    if (*value_type == bir::TypeKind::Ptr &&
        store->val.kind() == c4c::codegen::lir::LirOperandKind::Global &&
        store->ptr.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      const auto ptr_it = local_pointer_slots.find(store->ptr.str());
      if (ptr_it == local_pointer_slots.end()) {
        return false;
      }
      const auto slot_it = local_slot_types.find(ptr_it->second);
      if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
        return false;
      }
      const std::string global_name = store->val.str().substr(1);
      if (!is_known_function_symbol(global_name, function_symbols)) {
        return false;
      }
      local_slot_address_slots.erase(ptr_it->second);
      local_address_slots[ptr_it->second] = GlobalAddress{
          .global_name = global_name,
          .value_type = bir::TypeKind::Ptr,
          .byte_offset = 0,
      };
      return true;
    }

    const auto value = lower_value(store->val, *value_type, value_aliases);
    if (!value.has_value()) {
      return false;
    }

    const auto append_addressed_store =
        [&](std::string_view scratch_prefix, const bir::MemoryAddress& address) -> bool {
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return false;
      }
      const std::string scratch_slot = std::string(scratch_prefix) + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return false;
      }
      lowered_insts->push_back(bir::StoreLocalInst{
          .slot_name = scratch_slot,
          .value = *value,
          .address = address,
      });
      return true;
    };

    const auto append_dynamic_pointer_array_store =
        [&](std::string_view scratch_prefix,
            const DynamicPointerValueArrayAccess& access) -> bool {
      if (access.element_type != *value_type || access.element_count == 0) {
        return false;
      }

      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return false;
      }

      for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
        const std::string element_name =
            std::string(scratch_prefix) + ".elt" + std::to_string(element_index);
        const std::string load_slot = element_name + ".load.addr";
        if (!ensure_local_scratch_slot(load_slot, *value_type, slot_size)) {
          return false;
        }
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = load_slot,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = access.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        access.byte_offset + element_index * access.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });

        bir::Value stored_value = *value;
        if (access.element_count > 1) {
          const auto compare_rhs = make_index_immediate(access.index.type, element_index);
          if (!compare_rhs.has_value()) {
            return false;
          }
          const std::string select_name =
              std::string(scratch_prefix) + ".store" + std::to_string(element_index);
          lowered_insts->push_back(bir::SelectInst{
              .predicate = bir::BinaryOpcode::Eq,
              .result = bir::Value::named(*value_type, select_name),
              .compare_type = access.index.type,
              .lhs = access.index,
              .rhs = *compare_rhs,
              .true_value = *value,
              .false_value = bir::Value::named(*value_type, element_name),
          });
          stored_value = bir::Value::named(*value_type, select_name);
        }

        const std::string store_slot = element_name + ".store.addr";
        if (!ensure_local_scratch_slot(store_slot, *value_type, slot_size)) {
          return false;
        }
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = store_slot,
            .value = stored_value,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = access.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        access.byte_offset + element_index * access.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });
      }
      return true;
    };

    const auto append_dynamic_local_aggregate_store =
        [&](std::string_view scratch_prefix,
            const DynamicLocalAggregateArrayAccess& access) -> bool {
      if (access.element_count == 0) {
        return false;
      }

      const auto element_layout = compute_aggregate_type_layout(access.element_type_text, type_decls);
      if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
          element_layout.scalar_type != *value_type) {
        return false;
      }

      for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
        const auto leaf_offset = access.byte_offset + element_index * access.element_stride_bytes;
        const auto leaf_slot_it = access.leaf_slots.find(leaf_offset);
        if (leaf_slot_it == access.leaf_slots.end()) {
          return false;
        }

        const auto slot_it = local_slot_types.find(leaf_slot_it->second);
        if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
          return false;
        }

        const std::string element_name =
            std::string(scratch_prefix) + ".elt" + std::to_string(element_index);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = leaf_slot_it->second,
        });

        bir::Value stored_value = *value;
        if (access.element_count > 1) {
          const auto compare_rhs = make_index_immediate(access.index.type, element_index);
          if (!compare_rhs.has_value()) {
            return false;
          }
          const std::string select_name =
              std::string(scratch_prefix) + ".store" + std::to_string(element_index);
          lowered_insts->push_back(bir::SelectInst{
              .predicate = bir::BinaryOpcode::Eq,
              .result = bir::Value::named(*value_type, select_name),
              .compare_type = access.index.type,
              .lhs = access.index,
              .rhs = *compare_rhs,
              .true_value = *value,
              .false_value = bir::Value::named(*value_type, element_name),
          });
          stored_value = bir::Value::named(*value_type, select_name);
        }

        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = leaf_slot_it->second,
            .value = stored_value,
        });
      }
      return true;
    };

    if (const auto addressed_ptr_it = pointer_value_addresses.find(store->ptr.str());
        addressed_ptr_it != pointer_value_addresses.end()) {
      if (addressed_ptr_it->second.value_type != *value_type) {
        return false;
      }
      return append_addressed_store(
          store->ptr.str(),
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
              .base_value = addressed_ptr_it->second.base_value,
              .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
              .size_bytes = type_size_bytes(*value_type),
              .align_bytes = type_size_bytes(*value_type),
          });
    }

    if (const auto local_slot_ptr_it = local_slot_pointer_values.find(store->ptr.str());
        local_slot_ptr_it != local_slot_pointer_values.end()) {
      if (local_slot_ptr_it->second.value_type != *value_type) {
        return false;
      }
      if (local_slot_ptr_it->second.byte_offset == 0) {
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = local_slot_ptr_it->second.slot_name,
            .value = *value,
        });
        return true;
      }
      return append_addressed_store(
          store->ptr.str(),
          bir::MemoryAddress{
              .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
              .base_name = local_slot_ptr_it->second.slot_name,
              .byte_offset = static_cast<std::int64_t>(local_slot_ptr_it->second.byte_offset),
              .size_bytes = type_size_bytes(*value_type),
              .align_bytes = type_size_bytes(*value_type),
          });
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(store->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      return append_dynamic_pointer_array_store(store->ptr.str(), dynamic_ptr_it->second);
    }

    if (const auto dynamic_local_aggregate_it = dynamic_local_aggregate_arrays.find(store->ptr.str());
        dynamic_local_aggregate_it != dynamic_local_aggregate_arrays.end()) {
      return append_dynamic_local_aggregate_store(store->ptr.str(), dynamic_local_aggregate_it->second);
    }

    if (store->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = store->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        global_address_slots[global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_name,
          .value = *value,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(store->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != *value_type) {
          return false;
        }
        global_address_slots[global_object_it->second.global_name] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
        lowered_insts->push_back(bir::StoreGlobalInst{
            .global_name = global_object_it->second.global_name,
            .value = *value,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(store->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (global_ptr_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        addressed_global_pointer_slots[make_global_pointer_slot_key(global_ptr_it->second)] =
            resolve_pointer_store_address(
                store->val, global_pointer_slots, global_types, function_symbols);
      }
      lowered_insts->push_back(bir::StoreGlobalInst{
          .global_name = global_ptr_it->second.global_name,
          .value = *value,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(store->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (tracks_pointer_value_slot) {
      if (*value_type == bir::TypeKind::Ptr) {
        const auto pointer_alias = resolve_local_aggregate_pointer_value_alias(
            store->val, value_aliases, local_aggregate_slots, function_symbols);
        if (pointer_alias.has_value()) {
          local_pointer_value_aliases[ptr_it->second] = *pointer_alias;
        } else {
          local_pointer_value_aliases.erase(ptr_it->second);
        }
      } else {
        local_pointer_value_aliases.erase(ptr_it->second);
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto global_addr_it = global_address_ints.find(store->val.str());
        if (global_addr_it != global_address_ints.end()) {
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = global_addr_it->second;
          return true;
        }
      }
      local_slot_address_slots.erase(ptr_it->second);
      local_address_slots.erase(ptr_it->second);
    } else if (*value_type == bir::TypeKind::Ptr) {
      bool stored_local_slot_address = false;
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::Global) {
        const std::string global_name = store->val.str().substr(1);
        const auto global_it = global_types.find(global_name);
        if (global_it == global_types.end()) {
          if (!is_known_function_symbol(global_name, function_symbols)) {
            return false;
          }
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = GlobalAddress{
              .global_name = global_name,
              .value_type = bir::TypeKind::Ptr,
              .byte_offset = 0,
          };
          return true;
        }
        if (!global_it->second.supports_linear_addressing) {
          return false;
        }
        local_slot_address_slots.erase(ptr_it->second);
        local_address_slots[ptr_it->second] = GlobalAddress{
            .global_name = global_name,
            .value_type = global_it->second.value_type,
            .byte_offset = 0,
        };
        return true;
      }
      if (store->val.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        const auto local_slot_ptr_val_it = local_slot_pointer_values.find(store->val.str());
        if (local_slot_ptr_val_it != local_slot_pointer_values.end()) {
          local_slot_address_slots[ptr_it->second] = local_slot_ptr_val_it->second;
          local_address_slots.erase(ptr_it->second);
          stored_local_slot_address = true;
        }
        const auto local_ptr_it = local_pointer_slots.find(store->val.str());
        if (local_ptr_it != local_pointer_slots.end() &&
            local_slot_ptr_val_it == local_slot_pointer_values.end()) {
          const auto source_slot_it = local_slot_types.find(local_ptr_it->second);
          if (source_slot_it == local_slot_types.end()) {
            return false;
          }
          local_slot_address_slots[ptr_it->second] = LocalSlotAddress{
              .slot_name = local_ptr_it->second,
              .value_type = source_slot_it->second,
              .byte_offset = 0,
              .storage_type_text = render_type(source_slot_it->second),
              .type_text = render_type(source_slot_it->second),
          };
          local_address_slots.erase(ptr_it->second);
          stored_local_slot_address = true;
        }
        const auto global_ptr_it = global_pointer_slots.find(store->val.str());
        if (global_ptr_it != global_pointer_slots.end()) {
          local_slot_address_slots.erase(ptr_it->second);
          local_address_slots[ptr_it->second] = global_ptr_it->second;
          return true;
        }
      }
      if (!stored_local_slot_address) {
        local_slot_address_slots.erase(ptr_it->second);
        local_address_slots.erase(ptr_it->second);
      }
    }

    lowered_insts->push_back(bir::StoreLocalInst{
        .slot_name = ptr_it->second,
        .value = *value,
    });
    return true;
  }

  if (const auto* load = std::get_if<c4c::codegen::lir::LirLoadOp>(&inst)) {
    if (load->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        (load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue &&
         load->ptr.kind() != c4c::codegen::lir::LirOperandKind::Global)) {
      return false;
    }

    const auto value_type = lower_scalar_or_function_pointer_type(load->type_str.str());
    if (!value_type.has_value()) {
      const auto aggregate_layout = lower_byval_aggregate_layout(load->type_str.str(), type_decls);
      if (!aggregate_layout.has_value() ||
          load->ptr.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }
      if (local_aggregate_slots.find(load->ptr.str()) == local_aggregate_slots.end()) {
        return false;
      }
      aggregate_value_aliases[load->result.str()] = load->ptr.str();
      return true;
    }

    if (load->ptr.kind() == c4c::codegen::lir::LirOperandKind::Global) {
      const std::string global_name = load->ptr.str().substr(1);
      const auto global_it = global_types.find(global_name);
      if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
          global_it->second.value_type != *value_type) {
        return false;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto runtime_it = global_address_slots.find(global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_name,
      });
      return true;
    }

    if (*value_type == bir::TypeKind::Ptr) {
      if (const auto global_object_it = global_object_pointer_slots.find(load->ptr.str());
          global_object_it != global_object_pointer_slots.end()) {
        const auto global_it = global_types.find(global_object_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        if (const auto runtime_it = global_address_slots.find(global_object_it->second.global_name);
            runtime_it != global_address_slots.end()) {
          if (runtime_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *runtime_it->second;
          }
        } else if (global_it->second.known_global_address.has_value()) {
          global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
          record_pointer_global_object_alias(
              load->result.str(), global_it->second, global_types, global_object_pointer_slots);
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_object_it->second.global_name,
            .byte_offset = global_object_it->second.byte_offset,
        });
        return true;
      }
    }

    if (const auto global_ptr_it = global_pointer_slots.find(load->ptr.str());
        global_ptr_it != global_pointer_slots.end()) {
      if (*value_type != bir::TypeKind::Ptr) {
        if (const auto honest_address = resolve_honest_addressed_global_access(
                global_ptr_it->second, *value_type, global_types);
            honest_address.has_value()) {
          lowered_insts->push_back(bir::LoadGlobalInst{
              .result = bir::Value::named(*value_type, load->result.str()),
              .global_name = honest_address->global_name,
              .byte_offset = honest_address->byte_offset,
          });
          return true;
        }
      }

      if (global_ptr_it->second.value_type != *value_type) {
        if (global_ptr_it->second.value_type != bir::TypeKind::Ptr ||
            global_ptr_it->second.byte_offset != 0) {
          return false;
        }
        const auto global_it = global_types.find(global_ptr_it->second.global_name);
        if (global_it == global_types.end() || !global_it->second.supports_direct_value ||
            global_it->second.value_type != bir::TypeKind::Ptr) {
          return false;
        }
        lowered_insts->push_back(bir::LoadGlobalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .global_name = global_ptr_it->second.global_name,
        });
        return true;
      }
      if (*value_type == bir::TypeKind::Ptr) {
        const auto addressed_it =
            addressed_global_pointer_slots.find(make_global_pointer_slot_key(global_ptr_it->second));
        if (addressed_it != addressed_global_pointer_slots.end()) {
          if (addressed_it->second.has_value()) {
            global_pointer_slots[load->result.str()] = *addressed_it->second;
          }
        } else {
          const auto global_it = global_types.find(global_ptr_it->second.global_name);
          if (global_it == global_types.end()) {
            return false;
          }
          if (global_ptr_it->second.byte_offset == 0 &&
              global_it->second.supports_direct_value &&
              global_it->second.value_type == bir::TypeKind::Ptr) {
            if (const auto runtime_it = global_address_slots.find(global_ptr_it->second.global_name);
                runtime_it != global_address_slots.end()) {
              if (runtime_it->second.has_value()) {
                global_pointer_slots[load->result.str()] = *runtime_it->second;
              }
            } else if (global_it->second.known_global_address.has_value()) {
              global_pointer_slots[load->result.str()] = *global_it->second.known_global_address;
              record_pointer_global_object_alias(
                  load->result.str(), global_it->second, global_types, global_object_pointer_slots);
            }
          }
          const auto pointer_init_it =
              global_it->second.pointer_initializer_offsets.find(global_ptr_it->second.byte_offset);
          if (pointer_init_it != global_it->second.pointer_initializer_offsets.end()) {
            global_pointer_slots[load->result.str()] = pointer_init_it->second;
          }
        }
      }
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .global_name = global_ptr_it->second.global_name,
          .byte_offset = global_ptr_it->second.byte_offset,
      });
      return true;
    }

    if (const auto addressed_ptr_it = pointer_value_addresses.find(load->ptr.str());
        addressed_ptr_it != pointer_value_addresses.end()) {
      if (addressed_ptr_it->second.value_type != *value_type) {
        return false;
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return false;
      }
      const std::string scratch_slot = load->result.str() + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return false;
      }
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .slot_name = std::move(scratch_slot),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                  .base_value = addressed_ptr_it->second.base_value,
                  .byte_offset = static_cast<std::int64_t>(addressed_ptr_it->second.byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = slot_size,
              },
      });
      return true;
    }

    if (const auto local_slot_ptr_it = local_slot_pointer_values.find(load->ptr.str());
        local_slot_ptr_it != local_slot_pointer_values.end()) {
      if (local_slot_ptr_it->second.value_type != *value_type) {
        return false;
      }
      if (local_slot_ptr_it->second.byte_offset == 0) {
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, load->result.str()),
            .slot_name = local_slot_ptr_it->second.slot_name,
        });
        return true;
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return false;
      }
      const std::string scratch_slot = load->result.str() + ".addr";
      if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
        return false;
      }
      lowered_insts->push_back(bir::LoadLocalInst{
          .result = bir::Value::named(*value_type, load->result.str()),
          .slot_name = std::move(scratch_slot),
          .address =
              bir::MemoryAddress{
                  .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                  .base_name = local_slot_ptr_it->second.slot_name,
                  .byte_offset = static_cast<std::int64_t>(local_slot_ptr_it->second.byte_offset),
                  .size_bytes = slot_size,
                  .align_bytes = slot_size,
              },
      });
      return true;
    }

    if (const auto dynamic_ptr_it = dynamic_pointer_value_arrays.find(load->ptr.str());
        dynamic_ptr_it != dynamic_pointer_value_arrays.end()) {
      if (dynamic_ptr_it->second.element_type != *value_type ||
          dynamic_ptr_it->second.element_count == 0) {
        return false;
      }
      const auto slot_size = type_size_bytes(*value_type);
      if (slot_size == 0) {
        return false;
      }

      std::vector<bir::Value> element_values;
      element_values.reserve(dynamic_ptr_it->second.element_count);
      for (std::size_t element_index = 0;
           element_index < dynamic_ptr_it->second.element_count;
           ++element_index) {
        const std::string element_name =
            load->result.str() + ".elt" + std::to_string(element_index);
        const std::string scratch_slot = element_name + ".addr";
        if (!ensure_local_scratch_slot(scratch_slot, *value_type, slot_size)) {
          return false;
        }
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = scratch_slot,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
                    .base_value = dynamic_ptr_it->second.base_value,
                    .byte_offset = static_cast<std::int64_t>(
                        dynamic_ptr_it->second.byte_offset +
                        element_index * dynamic_ptr_it->second.element_stride_bytes),
                    .size_bytes = slot_size,
                    .align_bytes = slot_size,
                },
        });
        element_values.push_back(bir::Value::named(*value_type, element_name));
      }

      const auto selected_value = synthesize_value_array_selects(
          load->result.str(), element_values, dynamic_ptr_it->second.index, lowered_insts);
      if (!selected_value.has_value()) {
        return false;
      }
      if (selected_value->kind == bir::Value::Kind::Named &&
          selected_value->name == load->result.str()) {
        return true;
      }
      value_aliases[load->result.str()] = *selected_value;
      return true;
    }

    if (const auto dynamic_local_aggregate_it = dynamic_local_aggregate_arrays.find(load->ptr.str());
        dynamic_local_aggregate_it != dynamic_local_aggregate_arrays.end()) {
      if (dynamic_local_aggregate_it->second.element_count == 0) {
        return false;
      }

      const auto element_layout =
          compute_aggregate_type_layout(dynamic_local_aggregate_it->second.element_type_text, type_decls);
      if (element_layout.kind != AggregateTypeLayout::Kind::Scalar ||
          element_layout.scalar_type != *value_type) {
        return false;
      }

      std::vector<bir::Value> element_values;
      element_values.reserve(dynamic_local_aggregate_it->second.element_count);
      for (std::size_t element_index = 0;
           element_index < dynamic_local_aggregate_it->second.element_count;
           ++element_index) {
        const auto leaf_offset =
            dynamic_local_aggregate_it->second.byte_offset +
            element_index * dynamic_local_aggregate_it->second.element_stride_bytes;
        const auto leaf_slot_it = dynamic_local_aggregate_it->second.leaf_slots.find(leaf_offset);
        if (leaf_slot_it == dynamic_local_aggregate_it->second.leaf_slots.end()) {
          return false;
        }

        const auto slot_it = local_slot_types.find(leaf_slot_it->second);
        if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
          return false;
        }

        const std::string element_name =
            load->result.str() + ".elt" + std::to_string(element_index);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(*value_type, element_name),
            .slot_name = leaf_slot_it->second,
        });
        element_values.push_back(bir::Value::named(*value_type, element_name));
      }

      const auto selected_value = synthesize_value_array_selects(
          load->result.str(), element_values, dynamic_local_aggregate_it->second.index, lowered_insts);
      if (!selected_value.has_value()) {
        return false;
      }
      if (selected_value->kind == bir::Value::Kind::Named &&
          selected_value->name == load->result.str()) {
        return true;
      }
      value_aliases[load->result.str()] = *selected_value;
      return true;
    }

    const auto ptr_it = local_pointer_slots.find(load->ptr.str());
    if (ptr_it == local_pointer_slots.end()) {
      if (*value_type == bir::TypeKind::Ptr) {
        if (const auto local_array_it = dynamic_local_pointer_arrays.find(load->ptr.str());
            local_array_it != dynamic_local_pointer_arrays.end()) {
          const auto element_values = collect_local_pointer_values(
              local_array_it->second.element_slots, local_pointer_value_aliases);
          if (!element_values.has_value()) {
            return false;
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, local_array_it->second.index, lowered_insts);
          if (!selected_value.has_value()) {
            return false;
          }
          if (selected_value->kind == bir::Value::Kind::Named &&
              selected_value->name == load->result.str()) {
            return true;
          }
          value_aliases[load->result.str()] = *selected_value;
          return true;
        }
        if (const auto global_array_it = dynamic_global_pointer_arrays.find(load->ptr.str());
            global_array_it != dynamic_global_pointer_arrays.end()) {
          const auto element_values =
              collect_global_array_pointer_values(global_array_it->second, global_types);
          if (!element_values.has_value()) {
            return false;
          }
          const auto selected_value = synthesize_pointer_array_selects(
              load->result.str(), *element_values, global_array_it->second.index, lowered_insts);
          if (!selected_value.has_value()) {
            return false;
          }
          if (selected_value->kind == bir::Value::Kind::Named &&
              selected_value->name == load->result.str()) {
            return true;
          }
          value_aliases[load->result.str()] = *selected_value;
          return true;
        }
      }
      return false;
    }

    const auto slot_it = local_slot_types.find(ptr_it->second);
    if (slot_it == local_slot_types.end() || slot_it->second != *value_type) {
      return false;
    }

    const bool tracks_pointer_value_slot =
        local_aggregate_field_slots.find(ptr_it->second) != local_aggregate_field_slots.end() ||
        is_local_array_element_slot(ptr_it->second, local_array_slots);
    if (*value_type == bir::TypeKind::Ptr && tracks_pointer_value_slot) {
      const auto pointer_alias = local_pointer_value_aliases.find(ptr_it->second);
      if (pointer_alias != local_pointer_value_aliases.end()) {
        value_aliases[load->result.str()] = pointer_alias->second;
        if (const auto addr_it = local_address_slots.find(ptr_it->second);
            addr_it != local_address_slots.end()) {
          global_pointer_slots[load->result.str()] = addr_it->second;
        }
        return true;
      }
    }

    if (*value_type == bir::TypeKind::I64) {
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        global_address_ints[load->result.str()] = addr_it->second;
        return true;
      }
    } else if (*value_type == bir::TypeKind::Ptr) {
      if (const auto local_slot_it = local_slot_address_slots.find(ptr_it->second);
          local_slot_it != local_slot_address_slots.end()) {
        local_slot_pointer_values[load->result.str()] = local_slot_it->second;
      }
      if (const auto addr_it = local_address_slots.find(ptr_it->second);
          addr_it != local_address_slots.end()) {
        if (addr_it->second.byte_offset == 0 &&
            is_known_function_symbol(addr_it->second.global_name, function_symbols)) {
          value_aliases[load->result.str()] =
              bir::Value::named(bir::TypeKind::Ptr, "@" + addr_it->second.global_name);
          global_pointer_slots[load->result.str()] = addr_it->second;
          return true;
        }
        if (const auto honest_base = resolve_honest_pointer_base(addr_it->second, global_types);
            honest_base.has_value() && honest_base->byte_offset == 0) {
          value_aliases[load->result.str()] =
              bir::Value::named(bir::TypeKind::Ptr, "@" + honest_base->global_name);
          global_pointer_slots[load->result.str()] = *honest_base;
          return true;
        }
        global_pointer_slots[load->result.str()] = addr_it->second;
      }
    }

    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(*value_type, load->result.str()),
        .slot_name = ptr_it->second,
    });
    return true;
  }

  const auto try_lower_immediate_local_memset =
      [&](std::string_view dst_operand, std::uint8_t fill_byte, std::size_t fill_size_bytes) -> bool {
    struct LocalMemsetArrayView {
      bir::TypeKind element_type = bir::TypeKind::Void;
      std::vector<std::string> element_slots;
      std::size_t base_index = 0;
    };
    struct LocalMemsetScalarSlot {
      std::string slot_name;
      std::size_t size_bytes = 0;
    };

    const auto fill_leaf_slots =
        [&](const auto& leaf_slots, std::size_t total_size_bytes) -> bool {
      if (fill_size_bytes > total_size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& [byte_offset, slot_name] : leaf_slots) {
        if (covered_bytes == fill_size_bytes) {
          break;
        }
        if (byte_offset != covered_bytes) {
          return false;
        }

        const auto slot_type_it = local_slot_types.find(slot_name);
        if (slot_type_it == local_slot_types.end()) {
          return false;
        }
        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0 || byte_offset + slot_size > fill_size_bytes) {
          return false;
        }

        const auto fill_value =
            lower_repeated_byte_initializer_value(slot_type_it->second, fill_byte);
        if (!fill_value.has_value()) {
          return false;
        }
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = slot_name,
            .value = *fill_value,
        });
        covered_bytes += slot_size;
      }
      return covered_bytes == fill_size_bytes;
    };

    const auto resolve_local_memset_array_view =
        [&](std::string_view operand) -> std::optional<LocalMemsetArrayView> {
      if (const auto array_it = local_array_slots.find(std::string(operand));
          array_it != local_array_slots.end()) {
        return LocalMemsetArrayView{
            .element_type = array_it->second.element_type,
            .element_slots = array_it->second.element_slots,
            .base_index = 0,
        };
      }
      if (const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
          array_base_it != local_pointer_array_bases.end() &&
          array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
        const auto slot_type_it =
            local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
        if (slot_type_it != local_slot_types.end()) {
          return LocalMemsetArrayView{
              .element_type = slot_type_it->second,
              .element_slots = array_base_it->second.element_slots,
              .base_index = array_base_it->second.base_index,
          };
        }
      }

      const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
      if (ptr_slot_it == local_pointer_slots.end()) {
        return std::nullopt;
      }
      const auto dot = ptr_slot_it->second.rfind('.');
      if (dot == std::string::npos) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const std::string base_name = ptr_slot_it->second.substr(0, dot);
      const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
      if (!base_offset.has_value() || *base_offset < 0) {
        return std::nullopt;
      }

      if (const auto base_array_it = local_array_slots.find(base_name);
          base_array_it != local_array_slots.end()) {
        const auto base_index = static_cast<std::size_t>(*base_offset);
        if (base_index >= base_array_it->second.element_slots.size() ||
            base_array_it->second.element_type != slot_type_it->second) {
          return std::nullopt;
        }
        return LocalMemsetArrayView{
            .element_type = slot_type_it->second,
            .element_slots = base_array_it->second.element_slots,
            .base_index = base_index,
        };
      }

      const auto base_aggregate_it = local_aggregate_slots.find(base_name);
      if (base_aggregate_it == local_aggregate_slots.end()) {
        return std::nullopt;
      }

      const auto extent = find_repeated_aggregate_extent_at_offset(
          base_aggregate_it->second.storage_type_text,
          static_cast<std::size_t>(*base_offset),
          render_type(slot_type_it->second),
          type_decls);
      if (!extent.has_value()) {
        return std::nullopt;
      }

      std::vector<std::string> element_slots;
      element_slots.reserve(extent->element_count);
      for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
        const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
            static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
        if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
          return std::nullopt;
        }
        element_slots.push_back(leaf_it->second);
      }

      return LocalMemsetArrayView{
          .element_type = slot_type_it->second,
          .element_slots = std::move(element_slots),
          .base_index = 0,
      };
    };
    const auto resolve_local_memset_scalar_slot =
        [&](std::string_view operand) -> std::optional<LocalMemsetScalarSlot> {
      const auto ptr_it = local_pointer_slots.find(std::string(operand));
      if (ptr_it == local_pointer_slots.end()) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return std::nullopt;
      }

      return LocalMemsetScalarSlot{
          .slot_name = ptr_it->second,
          .size_bytes = slot_size,
      };
    };

    if (const auto aggregate_it = local_aggregate_slots.find(std::string(dst_operand));
        aggregate_it != local_aggregate_slots.end()) {
      const auto aggregate_layout =
          lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
      if (!aggregate_layout.has_value()) {
        return false;
      }
      return fill_leaf_slots(
          collect_sorted_leaf_slots(aggregate_it->second), aggregate_layout->size_bytes);
    }

    const auto array_view = resolve_local_memset_array_view(dst_operand);
    if (array_view.has_value()) {
      if (array_view->base_index > array_view->element_slots.size()) {
        return false;
      }

      const auto element_size = type_size_bytes(array_view->element_type);
      if (element_size == 0) {
        return false;
      }

      const auto target_count = array_view->element_slots.size() - array_view->base_index;
      std::vector<std::pair<std::size_t, std::string>> leaf_slots;
      leaf_slots.reserve(target_count);
      for (std::size_t index = 0; index < target_count; ++index) {
        leaf_slots.emplace_back(index * element_size,
                                array_view->element_slots[array_view->base_index + index]);
      }
      return fill_leaf_slots(leaf_slots, target_count * element_size);
    }

    const auto scalar_slot = resolve_local_memset_scalar_slot(dst_operand);
    if (!scalar_slot.has_value()) {
      return false;
    }

    std::vector<std::pair<std::size_t, std::string>> leaf_slots;
    leaf_slots.emplace_back(0, scalar_slot->slot_name);
    return fill_leaf_slots(leaf_slots, scalar_slot->size_bytes);
  };

  if (const auto* memcpy = std::get_if<c4c::codegen::lir::LirMemcpyOp>(&inst)) {
    if (memcpy->dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        memcpy->src.kind() != c4c::codegen::lir::LirOperandKind::SsaValue ||
        memcpy->is_volatile) {
      return false;
    }

    const auto copy_size = lower_value(memcpy->size, bir::TypeKind::I64, value_aliases);
    if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
        copy_size->immediate < 0) {
      return false;
    }
    const auto requested_size = static_cast<std::size_t>(copy_size->immediate);

    const std::string dst_operand(memcpy->dst.str());
    const std::string src_operand(memcpy->src.str());
    struct LocalMemcpyArrayView {
      bir::TypeKind element_type = bir::TypeKind::Void;
      std::vector<std::string> element_slots;
      std::size_t base_index = 0;
    };
    struct LocalMemcpyLeaf {
      std::size_t byte_offset = 0;
      bir::TypeKind type = bir::TypeKind::Void;
      std::string slot_name;
    };
    struct LocalMemcpyLeafView {
      std::size_t size_bytes = 0;
      std::vector<LocalMemcpyLeaf> leaves;
    };
    struct LocalMemcpyScalarSlot {
      std::string slot_name;
      bir::TypeKind type = bir::TypeKind::Void;
      std::size_t size_bytes = 0;
      std::size_t align_bytes = 0;
    };
    const auto resolve_local_memcpy_array_view =
        [&](std::string_view operand) -> std::optional<LocalMemcpyArrayView> {
      if (const auto array_it = local_array_slots.find(std::string(operand));
          array_it != local_array_slots.end()) {
        return LocalMemcpyArrayView{
            .element_type = array_it->second.element_type,
            .element_slots = array_it->second.element_slots,
            .base_index = 0,
        };
      }
      const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
      if (array_base_it != local_pointer_array_bases.end() &&
          array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
        const auto slot_type_it =
            local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
        if (slot_type_it != local_slot_types.end()) {
          return LocalMemcpyArrayView{
              .element_type = slot_type_it->second,
              .element_slots = array_base_it->second.element_slots,
              .base_index = array_base_it->second.base_index,
          };
        }
      }

      const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
      if (ptr_slot_it == local_pointer_slots.end()) {
        return std::nullopt;
      }
      const auto dot = ptr_slot_it->second.rfind('.');
      if (dot == std::string::npos) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const std::string base_name = ptr_slot_it->second.substr(0, dot);
      const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
      if (!base_offset.has_value() || *base_offset < 0) {
        return std::nullopt;
      }

      if (const auto base_array_it = local_array_slots.find(base_name);
          base_array_it != local_array_slots.end()) {
        const auto base_index = static_cast<std::size_t>(*base_offset);
        if (base_index >= base_array_it->second.element_slots.size() ||
            base_array_it->second.element_type != slot_type_it->second) {
          return std::nullopt;
        }
        return LocalMemcpyArrayView{
            .element_type = slot_type_it->second,
            .element_slots = base_array_it->second.element_slots,
            .base_index = base_index,
        };
      }

      const auto base_aggregate_it = local_aggregate_slots.find(base_name);
      if (base_aggregate_it == local_aggregate_slots.end()) {
        return std::nullopt;
      }

      const auto extent = find_repeated_aggregate_extent_at_offset(
          base_aggregate_it->second.storage_type_text,
          static_cast<std::size_t>(*base_offset),
          render_type(slot_type_it->second),
          type_decls);
      if (!extent.has_value()) {
        return std::nullopt;
      }

      std::vector<std::string> element_slots;
      element_slots.reserve(extent->element_count);
      for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
        const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
            static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
        if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
          return std::nullopt;
        }
        element_slots.push_back(leaf_it->second);
      }

      return LocalMemcpyArrayView{
          .element_type = slot_type_it->second,
          .element_slots = std::move(element_slots),
          .base_index = 0,
      };
    };
    const auto resolve_local_memcpy_leaf_view =
        [&](std::string_view operand) -> std::optional<LocalMemcpyLeafView> {
      if (const auto aggregate_it = local_aggregate_slots.find(std::string(operand));
          aggregate_it != local_aggregate_slots.end()) {
        const auto aggregate_layout =
            lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
        if (!aggregate_layout.has_value()) {
          return std::nullopt;
        }
        LocalMemcpyLeafView view{
            .size_bytes = aggregate_layout->size_bytes,
        };
        const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
        view.leaves.reserve(leaf_slots.size());
        for (const auto& [byte_offset, slot_name] : leaf_slots) {
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end()) {
            return std::nullopt;
          }
          view.leaves.push_back(LocalMemcpyLeaf{
              .byte_offset = byte_offset,
              .type = slot_type_it->second,
              .slot_name = slot_name,
          });
        }
        return view;
      }

      const auto array_view = resolve_local_memcpy_array_view(operand);
      if (!array_view.has_value() || array_view->base_index > array_view->element_slots.size()) {
        return std::nullopt;
      }

      const auto element_size = type_size_bytes(array_view->element_type);
      if (element_size == 0) {
        return std::nullopt;
      }

      const auto count = array_view->element_slots.size() - array_view->base_index;
      LocalMemcpyLeafView view{
          .size_bytes = count * element_size,
      };
      view.leaves.reserve(count);
      for (std::size_t index = 0; index < count; ++index) {
        const auto& slot_name = array_view->element_slots[array_view->base_index + index];
        const auto slot_type_it = local_slot_types.find(slot_name);
        if (slot_type_it == local_slot_types.end() || slot_type_it->second != array_view->element_type) {
          return std::nullopt;
        }
        view.leaves.push_back(LocalMemcpyLeaf{
            .byte_offset = index * element_size,
            .type = array_view->element_type,
            .slot_name = slot_name,
        });
      }
      return view;
    };
    const auto resolve_local_memcpy_scalar_slot =
        [&](std::string_view operand) -> std::optional<LocalMemcpyScalarSlot> {
      const auto ptr_it = local_pointer_slots.find(std::string(operand));
      if (ptr_it == local_pointer_slots.end()) {
        return std::nullopt;
      }

      const auto slot_type_it = local_slot_types.find(ptr_it->second);
      if (slot_type_it == local_slot_types.end()) {
        return std::nullopt;
      }

      const auto slot_size = type_size_bytes(slot_type_it->second);
      if (slot_size == 0) {
        return std::nullopt;
      }

      auto slot_align = slot_size;
      if (const auto slot_it = std::find_if(lowered_function->local_slots.begin(),
                                            lowered_function->local_slots.end(),
                                            [&](const bir::LocalSlot& slot) {
                                              return slot.name == ptr_it->second;
                                            });
          slot_it != lowered_function->local_slots.end() && slot_it->align_bytes != 0) {
        slot_align = slot_it->align_bytes;
      }

      return LocalMemcpyScalarSlot{
          .slot_name = ptr_it->second,
          .type = slot_type_it->second,
          .size_bytes = slot_size,
          .align_bytes = slot_align,
      };
    };
    const auto append_local_leaf_copy = [&](const LocalMemcpyLeafView& source_view,
                                            const LocalMemcpyLeafView& target_view) -> bool {
      const auto collect_requested_prefix =
          [&](const LocalMemcpyLeafView& view) -> std::optional<std::vector<LocalMemcpyLeaf>> {
        if (requested_size > view.size_bytes) {
          return std::nullopt;
        }

        std::vector<LocalMemcpyLeaf> prefix;
        prefix.reserve(view.leaves.size());
        std::size_t covered_bytes = 0;
        for (const auto& leaf : view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          if (leaf.byte_offset != covered_bytes) {
            return std::nullopt;
          }

          const auto leaf_size = type_size_bytes(leaf.type);
          if (leaf_size == 0 || leaf.byte_offset + leaf_size > requested_size) {
            return std::nullopt;
          }

          prefix.push_back(leaf);
          covered_bytes += leaf_size;
        }

        if (covered_bytes != requested_size) {
          return std::nullopt;
        }
        return prefix;
      };

      const auto source_prefix = collect_requested_prefix(source_view);
      const auto target_prefix = collect_requested_prefix(target_view);
      if (!source_prefix.has_value() || !target_prefix.has_value() ||
          source_prefix->size() != target_prefix->size()) {
        return false;
      }

      for (std::size_t index = 0; index < target_prefix->size(); ++index) {
        const auto& source_leaf = (*source_prefix)[index];
        const auto& target_leaf = (*target_prefix)[index];
        if (source_leaf.byte_offset != target_leaf.byte_offset || source_leaf.type != target_leaf.type) {
          return false;
        }
        const std::string copy_name =
            dst_operand + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(target_leaf.type, copy_name),
            .slot_name = source_leaf.slot_name,
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = target_leaf.slot_name,
            .value = bir::Value::named(target_leaf.type, copy_name),
        });
      }
      return true;
    };
    const auto append_leaf_view_to_scalar_slot =
        [&](const LocalMemcpyLeafView& source_view,
            const LocalMemcpyScalarSlot& target_slot) -> bool {
      if (requested_size != target_slot.size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& source_leaf : source_view.leaves) {
        if (covered_bytes == requested_size) {
          break;
        }
        const auto leaf_size = type_size_bytes(source_leaf.type);
        if (leaf_size == 0 || source_leaf.byte_offset != covered_bytes ||
            source_leaf.byte_offset + leaf_size > requested_size) {
          return false;
        }

        const std::string copy_name =
            target_slot.slot_name + ".memcpy.copy." + std::to_string(source_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(source_leaf.type, copy_name),
            .slot_name = source_leaf.slot_name,
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = source_leaf.slot_name,
            .value = bir::Value::named(source_leaf.type, copy_name),
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                    .base_name = target_slot.slot_name,
                    .byte_offset = static_cast<std::int64_t>(source_leaf.byte_offset),
                    .size_bytes = leaf_size,
                    .align_bytes = std::min(target_slot.align_bytes, leaf_size),
                },
        });
        covered_bytes += leaf_size;
      }
      return covered_bytes == requested_size;
    };
    const auto append_scalar_slot_to_leaf_view =
        [&](const LocalMemcpyScalarSlot& source_slot,
            const LocalMemcpyLeafView& target_view) -> bool {
      if (requested_size != source_slot.size_bytes) {
        return false;
      }

      std::size_t covered_bytes = 0;
      for (const auto& target_leaf : target_view.leaves) {
        if (covered_bytes == requested_size) {
          break;
        }
        const auto leaf_size = type_size_bytes(target_leaf.type);
        if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
            target_leaf.byte_offset + leaf_size > requested_size) {
          return false;
        }

        const std::string copy_name =
            target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
        lowered_insts->push_back(bir::LoadLocalInst{
            .result = bir::Value::named(target_leaf.type, copy_name),
            .slot_name = target_leaf.slot_name,
            .address =
                bir::MemoryAddress{
                    .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                    .base_name = source_slot.slot_name,
                    .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                    .size_bytes = leaf_size,
                    .align_bytes = std::min(source_slot.align_bytes, leaf_size),
                },
        });
        lowered_insts->push_back(bir::StoreLocalInst{
            .slot_name = target_leaf.slot_name,
            .value = bir::Value::named(target_leaf.type, copy_name),
        });
        covered_bytes += leaf_size;
      }
      return covered_bytes == requested_size;
    };
    const auto target_view = resolve_local_memcpy_leaf_view(dst_operand);
    if (target_view.has_value()) {
      const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
      if (source_view.has_value() && append_local_leaf_copy(*source_view, *target_view)) {
        return true;
      }
      const auto source_scalar_slot = resolve_local_memcpy_scalar_slot(src_operand);
      return source_scalar_slot.has_value() &&
             append_scalar_slot_to_leaf_view(*source_scalar_slot, *target_view);
    }

    if (const auto target_scalar_slot = resolve_local_memcpy_scalar_slot(dst_operand);
        target_scalar_slot.has_value()) {
      const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
      return source_view.has_value() &&
             append_leaf_view_to_scalar_slot(*source_view, *target_scalar_slot);
    }

    return false;
  }

  if (const auto* memset = std::get_if<c4c::codegen::lir::LirMemsetOp>(&inst)) {
    if (memset->dst.kind() != c4c::codegen::lir::LirOperandKind::SsaValue || memset->is_volatile) {
      return false;
    }
    const auto fill_value = lower_value(memset->byte_val, bir::TypeKind::I8, value_aliases);
    const auto fill_size = lower_value(memset->size, bir::TypeKind::I64, value_aliases);
    if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
        !fill_size.has_value() ||
        fill_size->kind != bir::Value::Kind::Immediate || fill_size->immediate < 0) {
      return false;
    }

    return try_lower_immediate_local_memset(
        memset->dst.str(),
        static_cast<std::uint8_t>(fill_value->immediate & 0xff),
        static_cast<std::size_t>(fill_size->immediate));
  }

  if (const auto* call = std::get_if<c4c::codegen::lir::LirCallOp>(&inst)) {

    const auto return_info = lower_return_info_from_type(call->return_type.str(), type_decls);
    if (!return_info.has_value()) {
      return false;
    }

    std::vector<bir::Value> lowered_args;
    std::vector<bir::TypeKind> lowered_arg_types;
    std::vector<bir::CallArgAbiInfo> lowered_arg_abi;
    std::optional<std::string> callee_name;
    std::optional<bir::Value> callee_value;
    bool is_indirect_call = false;
    bool is_variadic_call = false;
    std::optional<std::string> sret_slot_name;

    const auto try_lower_direct_memcpy_call =
        [&](std::string_view symbol_name,
            const ParsedTypedCall& typed_call) -> std::optional<bool> {
      if (symbol_name != "memcpy") {
        return std::nullopt;
      }
      if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
        return false;
      }

      const auto copy_size =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                      bir::TypeKind::I64,
                      value_aliases);
      if (!copy_size.has_value() || copy_size->kind != bir::Value::Kind::Immediate ||
          copy_size->immediate < 0) {
        return false;
      }
      const auto requested_size = static_cast<std::size_t>(copy_size->immediate);

      const std::string dst_operand(typed_call.args[0].operand);
      const std::string src_operand(typed_call.args[1].operand);
      struct LocalMemcpyArrayView {
        bir::TypeKind element_type = bir::TypeKind::Void;
        std::vector<std::string> element_slots;
        std::size_t base_index = 0;
      };
      struct LocalMemcpyLeaf {
        std::size_t byte_offset = 0;
        bir::TypeKind type = bir::TypeKind::Void;
        std::string slot_name;
      };
      struct LocalMemcpyLeafView {
        std::size_t size_bytes = 0;
        std::vector<LocalMemcpyLeaf> leaves;
      };
      struct LocalMemcpyScalarSlot {
        std::string slot_name;
        bir::TypeKind type = bir::TypeKind::Void;
        std::size_t size_bytes = 0;
        std::size_t align_bytes = 0;
      };
      const auto alias_memcpy_result = [&]() -> bool {
        if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
          if (return_info->returned_via_sret || return_info->type != bir::TypeKind::Ptr) {
            return false;
          }
          value_aliases[call->result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
          return true;
        }
        return return_info->type == bir::TypeKind::Void;
      };
      const auto resolve_local_memcpy_array_view =
          [&](std::string_view operand) -> std::optional<LocalMemcpyArrayView> {
        if (const auto array_it = local_array_slots.find(std::string(operand));
            array_it != local_array_slots.end()) {
          return LocalMemcpyArrayView{
              .element_type = array_it->second.element_type,
              .element_slots = array_it->second.element_slots,
              .base_index = 0,
          };
        }
        const auto array_base_it = local_pointer_array_bases.find(std::string(operand));
        if (array_base_it != local_pointer_array_bases.end() &&
            array_base_it->second.base_index < array_base_it->second.element_slots.size()) {
          const auto slot_type_it =
              local_slot_types.find(array_base_it->second.element_slots[array_base_it->second.base_index]);
          if (slot_type_it != local_slot_types.end()) {
            return LocalMemcpyArrayView{
                .element_type = slot_type_it->second,
                .element_slots = array_base_it->second.element_slots,
                .base_index = array_base_it->second.base_index,
            };
          }
        }

        const auto ptr_slot_it = local_pointer_slots.find(std::string(operand));
        if (ptr_slot_it == local_pointer_slots.end()) {
          return std::nullopt;
        }
        const auto dot = ptr_slot_it->second.rfind('.');
        if (dot == std::string::npos) {
          return std::nullopt;
        }

        const auto slot_type_it = local_slot_types.find(ptr_slot_it->second);
        if (slot_type_it == local_slot_types.end()) {
          return std::nullopt;
        }

        const std::string base_name = ptr_slot_it->second.substr(0, dot);
        const auto base_offset = parse_i64(std::string_view(ptr_slot_it->second).substr(dot + 1));
        if (!base_offset.has_value() || *base_offset < 0) {
          return std::nullopt;
        }

        if (const auto base_array_it = local_array_slots.find(base_name);
            base_array_it != local_array_slots.end()) {
          const auto base_index = static_cast<std::size_t>(*base_offset);
          if (base_index >= base_array_it->second.element_slots.size() ||
              base_array_it->second.element_type != slot_type_it->second) {
            return std::nullopt;
          }
          return LocalMemcpyArrayView{
              .element_type = slot_type_it->second,
              .element_slots = base_array_it->second.element_slots,
              .base_index = base_index,
          };
        }

        const auto base_aggregate_it = local_aggregate_slots.find(base_name);
        if (base_aggregate_it == local_aggregate_slots.end()) {
          return std::nullopt;
        }

        const auto extent = find_repeated_aggregate_extent_at_offset(
            base_aggregate_it->second.storage_type_text,
            static_cast<std::size_t>(*base_offset),
            render_type(slot_type_it->second),
            type_decls);
        if (!extent.has_value()) {
          return std::nullopt;
        }

        std::vector<std::string> element_slots;
        element_slots.reserve(extent->element_count);
        for (std::size_t element_index = 0; element_index < extent->element_count; ++element_index) {
          const auto leaf_it = base_aggregate_it->second.leaf_slots.find(
              static_cast<std::size_t>(*base_offset) + element_index * extent->element_stride_bytes);
          if (leaf_it == base_aggregate_it->second.leaf_slots.end()) {
            return std::nullopt;
          }
          element_slots.push_back(leaf_it->second);
        }

        return LocalMemcpyArrayView{
            .element_type = slot_type_it->second,
            .element_slots = std::move(element_slots),
            .base_index = 0,
        };
      };
      const auto resolve_local_memcpy_leaf_view =
          [&](std::string_view operand) -> std::optional<LocalMemcpyLeafView> {
        if (const auto aggregate_it = local_aggregate_slots.find(std::string(operand));
            aggregate_it != local_aggregate_slots.end()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(aggregate_it->second.type_text, type_decls);
          if (!aggregate_layout.has_value()) {
            return std::nullopt;
          }
          LocalMemcpyLeafView view{
              .size_bytes = aggregate_layout->size_bytes,
          };
          const auto leaf_slots = collect_sorted_leaf_slots(aggregate_it->second);
          view.leaves.reserve(leaf_slots.size());
          for (const auto& [byte_offset, slot_name] : leaf_slots) {
            const auto slot_type_it = local_slot_types.find(slot_name);
            if (slot_type_it == local_slot_types.end()) {
              return std::nullopt;
            }
            view.leaves.push_back(LocalMemcpyLeaf{
                .byte_offset = byte_offset,
                .type = slot_type_it->second,
                .slot_name = slot_name,
            });
          }
          return view;
        }

        const auto array_view = resolve_local_memcpy_array_view(operand);
        if (!array_view.has_value() || array_view->base_index > array_view->element_slots.size()) {
          return std::nullopt;
        }

        const auto element_size = type_size_bytes(array_view->element_type);
        if (element_size == 0) {
          return std::nullopt;
        }

        const auto count = array_view->element_slots.size() - array_view->base_index;
        LocalMemcpyLeafView view{
            .size_bytes = count * element_size,
        };
        view.leaves.reserve(count);
        for (std::size_t index = 0; index < count; ++index) {
          const auto& slot_name = array_view->element_slots[array_view->base_index + index];
          const auto slot_type_it = local_slot_types.find(slot_name);
          if (slot_type_it == local_slot_types.end() || slot_type_it->second != array_view->element_type) {
            return std::nullopt;
          }
          view.leaves.push_back(LocalMemcpyLeaf{
              .byte_offset = index * element_size,
              .type = slot_type_it->second,
              .slot_name = slot_name,
          });
        }
        return view;
      };
      const auto resolve_local_memcpy_scalar_slot =
          [&](std::string_view operand) -> std::optional<LocalMemcpyScalarSlot> {
        const auto ptr_it = local_pointer_slots.find(std::string(operand));
        if (ptr_it == local_pointer_slots.end()) {
          return std::nullopt;
        }

        const auto slot_type_it = local_slot_types.find(ptr_it->second);
        if (slot_type_it == local_slot_types.end()) {
          return std::nullopt;
        }

        const auto slot_size = type_size_bytes(slot_type_it->second);
        if (slot_size == 0) {
          return std::nullopt;
        }

        auto slot_align = slot_size;
        if (const auto slot_it = std::find_if(lowered_function->local_slots.begin(),
                                              lowered_function->local_slots.end(),
                                              [&](const bir::LocalSlot& slot) {
                                                return slot.name == ptr_it->second;
                                              });
            slot_it != lowered_function->local_slots.end() && slot_it->align_bytes != 0) {
          slot_align = slot_it->align_bytes;
        }

        return LocalMemcpyScalarSlot{
            .slot_name = ptr_it->second,
            .type = slot_type_it->second,
            .size_bytes = slot_size,
            .align_bytes = slot_align,
        };
      };
      const auto append_local_leaf_copy = [&](const LocalMemcpyLeafView& source_view,
                                              const LocalMemcpyLeafView& target_view) -> bool {
        const auto collect_requested_prefix =
            [&](const LocalMemcpyLeafView& view) -> std::optional<std::vector<LocalMemcpyLeaf>> {
          if (requested_size > view.size_bytes) {
            return std::nullopt;
          }

          std::vector<LocalMemcpyLeaf> prefix;
          prefix.reserve(view.leaves.size());
          std::size_t covered_bytes = 0;
          for (const auto& leaf : view.leaves) {
            if (covered_bytes == requested_size) {
              break;
            }
            if (leaf.byte_offset != covered_bytes) {
              return std::nullopt;
            }

            const auto leaf_size = type_size_bytes(leaf.type);
            if (leaf_size == 0 || leaf.byte_offset + leaf_size > requested_size) {
              return std::nullopt;
            }

            prefix.push_back(leaf);
            covered_bytes += leaf_size;
          }

          if (covered_bytes != requested_size) {
            return std::nullopt;
          }
          return prefix;
        };

        const auto source_prefix = collect_requested_prefix(source_view);
        const auto target_prefix = collect_requested_prefix(target_view);
        if (!source_prefix.has_value() || !target_prefix.has_value() ||
            source_prefix->size() != target_prefix->size()) {
          return false;
        }

        for (std::size_t index = 0; index < target_prefix->size(); ++index) {
          const auto& source_leaf = (*source_prefix)[index];
          const auto& target_leaf = (*target_prefix)[index];
          if (source_leaf.byte_offset != target_leaf.byte_offset || source_leaf.type != target_leaf.type) {
            return false;
          }
          const std::string copy_name =
              dst_operand + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(target_leaf.type, copy_name),
              .slot_name = source_leaf.slot_name,
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = target_leaf.slot_name,
              .value = bir::Value::named(target_leaf.type, copy_name),
          });
        }
        return true;
      };
      const auto append_leaf_view_to_scalar_slot =
          [&](const LocalMemcpyLeafView& source_view,
              const LocalMemcpyScalarSlot& target_slot) -> bool {
        if (requested_size != target_slot.size_bytes) {
          return false;
        }

        std::size_t covered_bytes = 0;
        for (const auto& source_leaf : source_view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          const auto leaf_size = type_size_bytes(source_leaf.type);
          if (leaf_size == 0 || source_leaf.byte_offset != covered_bytes ||
              source_leaf.byte_offset + leaf_size > requested_size) {
            return false;
          }

          const std::string copy_name =
              target_slot.slot_name + ".memcpy.copy." + std::to_string(source_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(source_leaf.type, copy_name),
              .slot_name = source_leaf.slot_name,
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = source_leaf.slot_name,
              .value = bir::Value::named(source_leaf.type, copy_name),
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                      .base_name = target_slot.slot_name,
                      .byte_offset = static_cast<std::int64_t>(source_leaf.byte_offset),
                      .size_bytes = leaf_size,
                      .align_bytes = std::min(target_slot.align_bytes, leaf_size),
                  },
          });
          covered_bytes += leaf_size;
        }
        return covered_bytes == requested_size;
      };
      const auto append_scalar_slot_to_leaf_view =
          [&](const LocalMemcpyScalarSlot& source_slot,
              const LocalMemcpyLeafView& target_view) -> bool {
        if (requested_size != source_slot.size_bytes) {
          return false;
        }

        std::size_t covered_bytes = 0;
        for (const auto& target_leaf : target_view.leaves) {
          if (covered_bytes == requested_size) {
            break;
          }
          const auto leaf_size = type_size_bytes(target_leaf.type);
          if (leaf_size == 0 || target_leaf.byte_offset != covered_bytes ||
              target_leaf.byte_offset + leaf_size > requested_size) {
            return false;
          }

          const std::string copy_name =
              target_leaf.slot_name + ".memcpy.copy." + std::to_string(target_leaf.byte_offset);
          lowered_insts->push_back(bir::LoadLocalInst{
              .result = bir::Value::named(target_leaf.type, copy_name),
              .slot_name = target_leaf.slot_name,
              .address =
                  bir::MemoryAddress{
                      .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
                      .base_name = source_slot.slot_name,
                      .byte_offset = static_cast<std::int64_t>(target_leaf.byte_offset),
                      .size_bytes = leaf_size,
                      .align_bytes = std::min(source_slot.align_bytes, leaf_size),
                  },
          });
          lowered_insts->push_back(bir::StoreLocalInst{
              .slot_name = target_leaf.slot_name,
              .value = bir::Value::named(target_leaf.type, copy_name),
          });
          covered_bytes += leaf_size;
        }
        return covered_bytes == requested_size;
      };
      const auto target_view = resolve_local_memcpy_leaf_view(dst_operand);
      if (target_view.has_value()) {
        const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
        if (source_view.has_value() && append_local_leaf_copy(*source_view, *target_view)) {
          return alias_memcpy_result();
        }
        const auto source_scalar_slot = resolve_local_memcpy_scalar_slot(src_operand);
        if (!source_scalar_slot.has_value() ||
            !append_scalar_slot_to_leaf_view(*source_scalar_slot, *target_view)) {
          return false;
        }
        return alias_memcpy_result();
      }

      if (const auto target_scalar_slot = resolve_local_memcpy_scalar_slot(dst_operand);
          target_scalar_slot.has_value()) {
        const auto source_view = resolve_local_memcpy_leaf_view(src_operand);
        if (!source_view.has_value() ||
            !append_leaf_view_to_scalar_slot(*source_view, *target_scalar_slot)) {
          return false;
        }
        return alias_memcpy_result();
      }

      return false;
    };
    const auto try_lower_direct_memset_call =
        [&](std::string_view symbol_name,
            const ParsedTypedCall& typed_call) -> std::optional<bool> {
      if (symbol_name != "memset") {
        return std::nullopt;
      }
      if (typed_call.args.size() != 3 || typed_call.param_types.size() != 3 ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[0]) != "ptr" ||
          c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[2]) != "i64") {
        return false;
      }

      const auto fill_type =
          lower_integer_type(c4c::codegen::lir::trim_lir_arg_text(typed_call.param_types[1]));
      if (!fill_type.has_value()) {
        return false;
      }

      const auto fill_value =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[1].operand)),
                      *fill_type,
                      value_aliases);
      const auto fill_size =
          lower_value(c4c::codegen::lir::LirOperand(std::string(typed_call.args[2].operand)),
                      bir::TypeKind::I64,
                      value_aliases);
      if (!fill_value.has_value() || fill_value->kind != bir::Value::Kind::Immediate ||
          !fill_size.has_value() ||
          fill_size->kind != bir::Value::Kind::Immediate || fill_size->immediate < 0) {
        return false;
      }

      const std::string dst_operand(typed_call.args[0].operand);
      if (!try_lower_immediate_local_memset(
              dst_operand,
              static_cast<std::uint8_t>(fill_value->immediate & 0xff),
              static_cast<std::size_t>(fill_size->immediate))) {
        return false;
      }
      if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
        if (return_info->returned_via_sret || return_info->type != bir::TypeKind::Ptr) {
          return false;
        }
        value_aliases[call->result.str()] = bir::Value::named(bir::TypeKind::Ptr, dst_operand);
        return true;
      }
      return return_info->type == bir::TypeKind::Void;
    };

    if (return_info->returned_via_sret) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }
      sret_slot_name = call->result.str();
      if (!declare_local_aggregate_slots(call->return_type.str(),
                                         *sret_slot_name,
                                         return_info->align_bytes)) {
        return false;
      }
      aggregate_value_aliases[*sret_slot_name] = *sret_slot_name;
      lowered_arg_types.push_back(bir::TypeKind::Ptr);
      lowered_args.push_back(bir::Value::named(bir::TypeKind::Ptr, *sret_slot_name));
      lowered_arg_abi.push_back(bir::CallArgAbiInfo{
          .type = bir::TypeKind::Ptr,
          .size_bytes = return_info->size_bytes,
          .align_bytes = return_info->align_bytes,
          .primary_class = bir::AbiValueClass::Memory,
          .sret_pointer = true,
      });
    }

    if (const auto direct_callee = c4c::codegen::lir::parse_lir_direct_global_callee(call->callee);
        direct_callee.has_value()) {
      if (const auto inferred_call = parse_typed_call(*call); inferred_call.has_value()) {
        if (const auto lowered_memset = try_lower_direct_memset_call(*direct_callee, *inferred_call);
            lowered_memset.has_value()) {
          return *lowered_memset;
        }
        if (const auto lowered_memcpy = try_lower_direct_memcpy_call(*direct_callee, *inferred_call);
            lowered_memcpy.has_value()) {
          return *lowered_memcpy;
        }
      }
    }

    if (const auto parsed_call = parse_direct_global_typed_call(*call);
        parsed_call.has_value()) {
      if (const auto lowered_memset =
              try_lower_direct_memset_call(parsed_call->symbol_name, parsed_call->typed_call);
          lowered_memset.has_value()) {
        return *lowered_memset;
      }
      if (const auto lowered_memcpy =
              try_lower_direct_memcpy_call(parsed_call->symbol_name, parsed_call->typed_call);
          lowered_memcpy.has_value()) {
        return *lowered_memcpy;
      }
      callee_name = std::string(parsed_call->symbol_name);
      is_variadic_call = parsed_call->is_variadic;
      lowered_args.reserve(parsed_call->typed_call.args.size());
      lowered_arg_types.reserve(parsed_call->typed_call.param_types.size());
      lowered_arg_abi.reserve(parsed_call->typed_call.param_types.size());
      for (std::size_t index = 0; index < parsed_call->typed_call.args.size(); ++index) {
        const auto trimmed_param_type =
            c4c::codegen::lir::trim_lir_arg_text(parsed_call->typed_call.param_types[index]);
        if (trimmed_param_type == "ptr") {
          const auto arg = lower_call_pointer_arg_value(
              c4c::codegen::lir::LirOperand(std::string(parsed_call->typed_call.args[index].operand)),
              value_aliases,
              local_aggregate_slots,
              global_types,
              function_symbols);
          if (!arg.has_value()) {
            return false;
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(*arg);
          lowered_arg_abi.push_back(bir::CallArgAbiInfo{.type = bir::TypeKind::Ptr});
          continue;
        }
        const auto arg_type =
            lower_scalar_or_function_pointer_type(parsed_call->typed_call.param_types[index]);
        if (!arg_type.has_value()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(parsed_call->typed_call.param_types[index], type_decls);
          if (!aggregate_layout.has_value()) {
            return false;
          }
          const std::string operand_text(parsed_call->typed_call.args[index].operand);
          const auto aggregate_alias_it = aggregate_value_aliases.find(operand_text);
          if (aggregate_alias_it == aggregate_value_aliases.end() ||
              local_aggregate_slots.find(aggregate_alias_it->second) == local_aggregate_slots.end()) {
            return false;
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(bir::Value::named(bir::TypeKind::Ptr, aggregate_alias_it->second));
          lowered_arg_abi.push_back(bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = aggregate_layout->size_bytes,
              .align_bytes = aggregate_layout->align_bytes,
              .primary_class = bir::AbiValueClass::Memory,
              .byval_copy = true,
          });
          continue;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(
                            std::string(parsed_call->typed_call.args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return false;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
        lowered_arg_abi.push_back(bir::CallArgAbiInfo{.type = *arg_type});
      }
    } else if (const auto parsed_call = parse_typed_call(*call);
               parsed_call.has_value() &&
               call->callee.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      callee_value = lower_value(call->callee, bir::TypeKind::Ptr, value_aliases);
      if (!callee_value.has_value()) {
        return false;
      }
      lowered_args.reserve(parsed_call->args.size());
      lowered_arg_types.reserve(parsed_call->param_types.size());
      lowered_arg_abi.reserve(parsed_call->param_types.size());
      for (std::size_t index = 0; index < parsed_call->args.size(); ++index) {
        const auto trimmed_param_type =
            c4c::codegen::lir::trim_lir_arg_text(parsed_call->param_types[index]);
        if (trimmed_param_type == "ptr") {
          const auto arg = lower_call_pointer_arg_value(
              c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
              value_aliases,
              local_aggregate_slots,
              global_types,
              function_symbols);
          if (!arg.has_value()) {
            return false;
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(*arg);
          lowered_arg_abi.push_back(bir::CallArgAbiInfo{.type = bir::TypeKind::Ptr});
          continue;
        }
        const auto arg_type = lower_scalar_or_function_pointer_type(parsed_call->param_types[index]);
        if (!arg_type.has_value()) {
          const auto aggregate_layout =
              lower_byval_aggregate_layout(parsed_call->param_types[index], type_decls);
          if (!aggregate_layout.has_value()) {
            return false;
          }
          const std::string operand_text(parsed_call->args[index].operand);
          const auto aggregate_alias_it = aggregate_value_aliases.find(operand_text);
          if (aggregate_alias_it == aggregate_value_aliases.end() ||
              local_aggregate_slots.find(aggregate_alias_it->second) == local_aggregate_slots.end()) {
            return false;
          }
          lowered_arg_types.push_back(bir::TypeKind::Ptr);
          lowered_args.push_back(bir::Value::named(bir::TypeKind::Ptr, aggregate_alias_it->second));
          lowered_arg_abi.push_back(bir::CallArgAbiInfo{
              .type = bir::TypeKind::Ptr,
              .size_bytes = aggregate_layout->size_bytes,
              .align_bytes = aggregate_layout->align_bytes,
              .primary_class = bir::AbiValueClass::Memory,
              .byval_copy = true,
          });
          continue;
        }
        const auto arg =
            lower_value(c4c::codegen::lir::LirOperand(std::string(parsed_call->args[index].operand)),
                        *arg_type,
                        value_aliases);
        if (!arg.has_value()) {
          return false;
        }
        lowered_arg_types.push_back(*arg_type);
        lowered_args.push_back(*arg);
        lowered_arg_abi.push_back(bir::CallArgAbiInfo{.type = *arg_type});
      }
      is_indirect_call = true;
    } else if (const auto zero_arg_callee =
                   c4c::codegen::lir::parse_lir_direct_global_callee(call->callee);
               zero_arg_callee.has_value() &&
               c4c::codegen::lir::trim_lir_arg_text(call->args_str).empty()) {
      callee_name = std::string(*zero_arg_callee);
    } else {
      return false;
    }

    bir::CallInst lowered_call;
    if (!return_info->returned_via_sret && return_info->type != bir::TypeKind::Void) {
      if (call->result.kind() != c4c::codegen::lir::LirOperandKind::SsaValue) {
        return false;
      }
      lowered_call.result = bir::Value::named(return_info->type, call->result.str());
    } else if (call->result.kind() == c4c::codegen::lir::LirOperandKind::SsaValue) {
      if (!return_info->returned_via_sret) {
        return false;
      }
    }
    if (is_indirect_call) {
      lowered_call.callee_value = std::move(*callee_value);
    } else {
      lowered_call.callee = std::move(*callee_name);
    }
    lowered_call.args = std::move(lowered_args);
    lowered_call.arg_types = std::move(lowered_arg_types);
    lowered_call.arg_abi = std::move(lowered_arg_abi);
    lowered_call.return_type_name =
        return_info->returned_via_sret ? "void" : std::string(call->return_type.str());
    lowered_call.return_type = return_info->type;
    lowered_call.is_indirect = is_indirect_call;
    lowered_call.is_variadic = is_variadic_call;
    lowered_insts->push_back(std::move(lowered_call));
    return true;
  }

  return false;
}
}  // namespace c4c::backend

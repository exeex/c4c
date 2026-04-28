#include "../lowering.hpp"

#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend {

using lir_to_bir_detail::lower_integer_type;
using lir_to_bir_detail::type_size_bytes;

std::optional<bir::Value> BirFunctionLowerer::lower_zero_initializer_value(bir::TypeKind type) {
  switch (type) {
    case bir::TypeKind::I1:
      return bir::Value::immediate_i1(false);
    case bir::TypeKind::I8:
      return bir::Value::immediate_i8(0);
    case bir::TypeKind::I16:
      return bir::Value::immediate_i16(0);
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
    case bir::TypeKind::I16:
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

std::optional<bir::Value> BirFunctionLowerer::load_dynamic_global_scalar_array_value(
    std::string_view result_name,
    bir::TypeKind value_type,
    const DynamicGlobalScalarArrayAccess& access,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_type != value_type || access.outer_element_count == 0 ||
      access.element_count == 0) {
    return std::nullopt;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return std::nullopt;
  }

  std::vector<bir::Value> outer_values;
  outer_values.reserve(access.outer_element_count);
  for (std::size_t outer_index = 0; outer_index < access.outer_element_count; ++outer_index) {
    std::vector<bir::Value> element_values;
    element_values.reserve(access.element_count);
    for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
      const std::string element_name =
          std::string(result_name) + ".outer" + std::to_string(outer_index) + ".elt" +
          std::to_string(element_index);
      // Dynamic scalar-array materialization is fed by compatibility access
      // text rather than the structured global table, so this remains an
      // explicitly unresolved LinkNameId boundary.
      lowered_insts->push_back(bir::LoadGlobalInst{
          .result = bir::Value::named(value_type, element_name),
          .global_name = access.global_name,
          .byte_offset = access.byte_offset + outer_index * access.outer_element_stride_bytes +
                         element_index * access.element_stride_bytes,
          .align_bytes = slot_size,
      });
      element_values.push_back(bir::Value::named(value_type, element_name));
    }

    bir::Value outer_value;
    if (element_values.size() == 1) {
      outer_value = element_values.front();
    } else {
      const auto selected_inner = synthesize_value_array_selects(
          std::string(result_name) + ".outer" + std::to_string(outer_index),
          element_values,
          access.index,
          lowered_insts);
      if (!selected_inner.has_value()) {
        return std::nullopt;
      }
      outer_value = *selected_inner;
    }
    outer_values.push_back(std::move(outer_value));
  }

  if (outer_values.size() == 1) {
    return outer_values.front();
  }
  return synthesize_value_array_selects(
      result_name, outer_values, access.outer_index, lowered_insts);
}

std::optional<bir::Value> BirFunctionLowerer::load_dynamic_pointer_value_array_value(
    std::string_view result_name,
    bir::TypeKind value_type,
    const DynamicPointerValueArrayAccess& access,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_type != value_type || access.element_count == 0) {
    return std::nullopt;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return std::nullopt;
  }

  std::vector<bir::Value> element_values;
  element_values.reserve(access.element_count);
  for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
    const std::string element_name = std::string(result_name) + ".elt" + std::to_string(element_index);
    const std::string scratch_slot = element_name + ".addr";
    if (!ensure_local_scratch_slot(scratch_slot, value_type, slot_size)) {
      return std::nullopt;
    }
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(value_type, element_name),
        .slot_name = scratch_slot,
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
    element_values.push_back(bir::Value::named(value_type, element_name));
  }

  return synthesize_value_array_selects(result_name, element_values, access.index, lowered_insts);
}

std::optional<bool> BirFunctionLowerer::try_lower_dynamic_pointer_array_load(
    std::string_view result_name,
    std::string_view ptr_name,
    const DynamicLocalPointerArrayMap& dynamic_local_pointer_arrays,
    const DynamicGlobalPointerArrayMap& dynamic_global_pointer_arrays,
    const LocalPointerValueAliasMap& local_pointer_value_aliases,
    const GlobalTypes& global_types,
    ValueMap* value_aliases,
    std::vector<bir::Inst>* lowered_insts) {
  auto record_selected_value = [&](const bir::Value& selected_value) -> bool {
    if (selected_value.kind == bir::Value::Kind::Named && selected_value.name == result_name) {
      return true;
    }
    (*value_aliases)[std::string(result_name)] = selected_value;
    return true;
  };

  if (const auto local_array_it = dynamic_local_pointer_arrays.find(std::string(ptr_name));
      local_array_it != dynamic_local_pointer_arrays.end()) {
    const auto element_values = collect_local_pointer_values(local_array_it->second.element_slots,
                                                             local_pointer_value_aliases);
    if (!element_values.has_value()) {
      return false;
    }
    const auto selected_value = synthesize_pointer_array_selects(
        result_name, *element_values, local_array_it->second.index, lowered_insts);
    if (!selected_value.has_value()) {
      return false;
    }
    return record_selected_value(*selected_value);
  }

  if (const auto global_array_it = dynamic_global_pointer_arrays.find(std::string(ptr_name));
      global_array_it != dynamic_global_pointer_arrays.end()) {
    const auto element_values =
        collect_global_array_pointer_values(global_array_it->second, global_types);
    if (!element_values.has_value()) {
      return false;
    }
    const auto selected_value = synthesize_pointer_array_selects(
        result_name, *element_values, global_array_it->second.index, lowered_insts);
    if (!selected_value.has_value()) {
      return false;
    }
    return record_selected_value(*selected_value);
  }

  return std::nullopt;
}

bool BirFunctionLowerer::append_dynamic_pointer_value_array_store(
    std::string_view scratch_prefix,
    bir::TypeKind value_type,
    const bir::Value& value,
    const DynamicPointerValueArrayAccess& access,
    std::vector<bir::Inst>* lowered_insts) {
  if (access.element_type != value_type || access.element_count == 0) {
    return false;
  }

  const auto slot_size = type_size_bytes(value_type);
  if (slot_size == 0) {
    return false;
  }

  for (std::size_t element_index = 0; element_index < access.element_count; ++element_index) {
    const std::string element_name =
        std::string(scratch_prefix) + ".elt" + std::to_string(element_index);
    const std::string load_slot = element_name + ".load.addr";
    if (!ensure_local_scratch_slot(load_slot, value_type, slot_size)) {
      return false;
    }
    lowered_insts->push_back(bir::LoadLocalInst{
        .result = bir::Value::named(value_type, element_name),
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

    bir::Value stored_value = value;
    if (access.element_count > 1) {
      const auto compare_rhs = make_index_immediate(access.index.type, element_index);
      if (!compare_rhs.has_value()) {
        return false;
      }
      const std::string select_name =
          std::string(scratch_prefix) + ".store" + std::to_string(element_index);
      lowered_insts->push_back(bir::SelectInst{
          .predicate = bir::BinaryOpcode::Eq,
          .result = bir::Value::named(value_type, select_name),
          .compare_type = access.index.type,
          .lhs = access.index,
          .rhs = *compare_rhs,
          .true_value = value,
          .false_value = bir::Value::named(value_type, element_name),
      });
      stored_value = bir::Value::named(value_type, select_name);
    }

    const std::string store_slot = element_name + ".store.addr";
    if (!ensure_local_scratch_slot(store_slot, value_type, slot_size)) {
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
}

}  // namespace c4c::backend

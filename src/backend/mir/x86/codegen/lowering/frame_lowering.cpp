#include "frame_lowering.hpp"

#include "memory_lowering.hpp"

#include <algorithm>

namespace c4c::backend::x86 {

namespace {

std::size_t align_up(std::size_t value, std::size_t align) {
  if (align <= 1) {
    return value;
  }
  const auto remainder = value % align;
  return remainder == 0 ? value : value + (align - remainder);
}

}  // namespace

std::optional<std::size_t> find_prepared_authoritative_value_stack_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  return find_prepared_authoritative_named_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      function_name,
      value_name);
}

std::optional<std::size_t> find_prepared_value_home_frame_offset(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr || value_name.empty()) {
    return std::nullopt;
  }

  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(*prepared_names, *function_locations, value_name);
  if (home == nullptr || home->kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
    return std::nullopt;
  }
  if (home->slot_id.has_value()) {
    const auto frame_slot_it = local_layout.frame_slot_offsets.find(*home->slot_id);
    if (frame_slot_it != local_layout.frame_slot_offsets.end()) {
      return frame_slot_it->second;
    }
  }
  if (home->offset_bytes.has_value()) {
    return *home->offset_bytes;
  }
  return std::nullopt;
}

std::optional<PreparedNamedHomeSelection> resolve_prepared_named_home_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name) {
  if (prepared_names == nullptr || function_locations == nullptr || value_name.empty()) {
    return std::nullopt;
  }

  const auto* home =
      c4c::backend::prepare::find_prepared_value_home(*prepared_names, *function_locations, value_name);
  if (home == nullptr) {
    return std::nullopt;
  }

  PreparedNamedHomeSelection selection;
  if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::Register &&
      home->register_name.has_value()) {
    selection.register_name = *home->register_name;
  } else if (home->kind == c4c::backend::prepare::PreparedValueHomeKind::StackSlot) {
    selection.frame_offset = find_prepared_value_home_frame_offset(
        local_layout, prepared_names, function_locations, value_name);
  }

  if (!selection.register_name.has_value() && !selection.frame_offset.has_value()) {
    return std::nullopt;
  }
  return selection;
}

std::optional<std::size_t> resolve_prepared_local_slot_base_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view slot_name) {
  if (const auto slot_it = local_layout.offsets.find(slot_name); slot_it != local_layout.offsets.end()) {
    return slot_it->second;
  }
  return find_prepared_value_home_frame_offset(
      local_layout, prepared_names, function_locations, slot_name);
}

std::optional<std::size_t> resolve_prepared_named_payload_frame_offset_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    c4c::FunctionNameId function_name,
    std::string_view pointer_name) {
  if (prepared_names == nullptr || function_locations == nullptr || pointer_name.empty()) {
    return std::nullopt;
  }

  std::string slice_zero_name;
  std::string_view slice_zero_query = pointer_name;
  if (!c4c::backend::prepare::parse_prepared_slot_slice_name(pointer_name).has_value()) {
    slice_zero_name = std::string(pointer_name) + ".0";
    slice_zero_query = slice_zero_name;
  }

  if (const auto slice_zero_home_offset = find_prepared_value_home_frame_offset(
          local_layout, prepared_names, function_locations, slice_zero_query);
      slice_zero_home_offset.has_value()) {
    return slice_zero_home_offset;
  }
  if (const auto exact_home_offset = find_prepared_value_home_frame_offset(
          local_layout, prepared_names, function_locations, pointer_name);
      exact_home_offset.has_value()) {
    return exact_home_offset;
  }
  if (const auto slice_zero_offset = resolve_prepared_local_slot_base_offset_if_supported(
          local_layout, prepared_names, function_locations, slice_zero_query);
      slice_zero_offset.has_value()) {
    return slice_zero_offset;
  }
  if (const auto exact_offset = resolve_prepared_local_slot_base_offset_if_supported(
          local_layout, prepared_names, function_locations, pointer_name);
      exact_offset.has_value()) {
    return exact_offset;
  }
  if (stack_layout != nullptr && function_name != c4c::kInvalidFunctionName) {
    return c4c::backend::prepare::find_prepared_stack_frame_offset_by_name(
        *prepared_names, *stack_layout, function_name, pointer_name);
  }

  return std::nullopt;
}

std::optional<std::string> render_prepared_named_stack_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::string_view value_name,
    std::string_view size_name,
    std::size_t stack_byte_bias) {
  if (value_name.empty() || size_name.empty()) {
    return std::nullopt;
  }
  if (const auto slot_it = local_layout.offsets.find(value_name); slot_it != local_layout.offsets.end()) {
    return render_prepared_stack_memory_operand(slot_it->second + stack_byte_bias, size_name);
  }
  if (const auto frame_offset = find_prepared_value_home_frame_offset(
          local_layout, prepared_names, function_locations, value_name);
      frame_offset.has_value()) {
    return render_prepared_stack_memory_operand(*frame_offset + stack_byte_bias, size_name);
  }
  return std::nullopt;
}

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
  std::size_t max_frame_slot_end = 0;
  const c4c::FunctionNameId function_name_id =
      prepared_names == nullptr ? c4c::kInvalidFunctionName
                                : c4c::backend::prepare::resolve_prepared_function_name_id(
                                      *prepared_names, function.name)
                                      .value_or(c4c::kInvalidFunctionName);
  for (const auto& slot : function.local_slots) {
    if (slot.type != c4c::backend::bir::TypeKind::I8 &&
        slot.type != c4c::backend::bir::TypeKind::I16 &&
        slot.type != c4c::backend::bir::TypeKind::I32 &&
        slot.type != c4c::backend::bir::TypeKind::I64 &&
        slot.type != c4c::backend::bir::TypeKind::F32 &&
        slot.type != c4c::backend::bir::TypeKind::F64 &&
        slot.type != c4c::backend::bir::TypeKind::F128 &&
        slot.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto slot_size = slot.size_bytes != 0
                               ? slot.size_bytes
                               : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::F64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::F128 ? 16u
                                  : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                  : slot.type == c4c::backend::bir::TypeKind::F32 ? 4u
                                  : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                  : 1u);
    const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
    if (slot_size == 0 || slot_size > 16 || slot_align > 16) {
      return std::nullopt;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.offsets.emplace(slot.name, next_offset);
    next_offset += slot_size;
    max_align = std::max(max_align, slot_align);
  }
  if (stack_layout != nullptr) {
    for (const auto& frame_slot : stack_layout->frame_slots) {
      if (frame_slot.function_name != function_name_id) {
        continue;
      }
      layout.frame_slot_offsets.emplace(frame_slot.slot_id, frame_slot.offset_bytes);
      max_frame_slot_end =
          std::max(max_frame_slot_end, frame_slot.offset_bytes + frame_slot.size_bytes);
    }
  }
  if (function_locations != nullptr) {
    for (const auto& home : function_locations->value_homes) {
      if (home.kind != c4c::backend::prepare::PreparedValueHomeKind::StackSlot ||
          !home.offset_bytes.has_value()) {
        continue;
      }
      max_frame_slot_end = std::max(max_frame_slot_end, *home.offset_bytes + std::size_t{8});
    }
  }
  if (function_addressing != nullptr) {
    if (function_name_id == c4c::kInvalidFunctionName ||
        function_addressing->function_name != function_name_id) {
      return std::nullopt;
    }
    const auto required_frame_alignment =
        std::max<std::size_t>(16, function_addressing->frame_alignment_bytes);
    layout.frame_alignment = required_frame_alignment;
    layout.frame_size = align_up(
        std::max({function_addressing->frame_size_bytes, next_offset, max_frame_slot_end}),
        required_frame_alignment);
    return layout;
  }
  layout.frame_alignment = max_align;
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    c4c::TargetArch prepared_arch) {
  return build_prepared_module_local_slot_layout(
      function, stack_layout, function_addressing, prepared_names, nullptr, prepared_arch);
}

}  // namespace c4c::backend::x86

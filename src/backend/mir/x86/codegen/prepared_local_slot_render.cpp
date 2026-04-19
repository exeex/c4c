#include "x86_codegen.hpp"

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

std::optional<PreparedModuleLocalSlotLayout> build_prepared_module_local_slot_layout(
    const c4c::backend::bir::Function& function,
    c4c::TargetArch prepared_arch) {
  if (prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }
  PreparedModuleLocalSlotLayout layout;
  std::size_t next_offset = 0;
  std::size_t max_align = 16;
  for (const auto& slot : function.local_slots) {
    if (slot.type != c4c::backend::bir::TypeKind::I8 &&
        slot.type != c4c::backend::bir::TypeKind::I16 &&
        slot.type != c4c::backend::bir::TypeKind::I32 &&
        slot.type != c4c::backend::bir::TypeKind::I64 &&
        slot.type != c4c::backend::bir::TypeKind::Ptr) {
      return std::nullopt;
    }
    const auto slot_size = slot.size_bytes != 0
                               ? slot.size_bytes
                               : (slot.type == c4c::backend::bir::TypeKind::Ptr ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I64 ? 8u
                                  : slot.type == c4c::backend::bir::TypeKind::I16 ? 2u
                                  : slot.type == c4c::backend::bir::TypeKind::I32 ? 4u
                                                                                  : 1u);
    const auto slot_align = slot.align_bytes != 0 ? slot.align_bytes : slot_size;
    if (slot_size == 0 || slot_size > 8 || slot_align > 16) {
      return std::nullopt;
    }
    next_offset = align_up(next_offset, slot_align);
    layout.offsets.emplace(slot.name, next_offset);
    next_offset += slot_size;
    max_align = std::max(max_align, slot_align);
  }
  layout.frame_size = align_up(next_offset, max_align);
  return layout;
}

std::string render_prepared_stack_memory_operand(std::size_t byte_offset,
                                                 std::string_view size_name) {
  if (byte_offset == 0) {
    return std::string(size_name) + " PTR [rsp]";
  }
  return std::string(size_name) + " PTR [rsp + " + std::to_string(byte_offset) + "]";
}

std::string render_prepared_stack_address_expr(std::size_t byte_offset) {
  if (byte_offset == 0) {
    return "[rsp]";
  }
  return "[rsp + " + std::to_string(byte_offset) + "]";
}

std::optional<std::string> render_prepared_local_address_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const std::optional<c4c::backend::bir::MemoryAddress>& address,
    std::string_view size_name) {
  if (!address.has_value() ||
      address->base_kind != c4c::backend::bir::MemoryAddress::BaseKind::LocalSlot) {
    return std::nullopt;
  }
  const auto slot_it = local_layout.offsets.find(address->base_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  const auto signed_byte_offset =
      static_cast<std::int64_t>(slot_it->second) + address->byte_offset;
  if (signed_byte_offset < 0) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(static_cast<std::size_t>(signed_byte_offset),
                                              size_name);
}

std::optional<std::string> render_prepared_local_slot_memory_operand_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    std::string_view slot_name,
    std::size_t stack_byte_bias,
    std::string_view size_name) {
  const auto slot_it = local_layout.offsets.find(slot_name);
  if (slot_it == local_layout.offsets.end()) {
    return std::nullopt;
  }
  return render_prepared_stack_memory_operand(slot_it->second + stack_byte_bias, size_name);
}

std::optional<PreparedBoundedMultiDefinedCallLaneModuleRender>
render_prepared_bounded_multi_defined_call_lane_module_if_supported(
    const std::vector<const c4c::backend::bir::Function*>& defined_functions,
    c4c::TargetArch prepared_arch,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        render_trivial_defined_function,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Function&)>&
        minimal_function_return_register,
    const std::function<std::string(const c4c::backend::bir::Function&)>&
        minimal_function_asm_prefix,
    const std::function<bool(std::string_view)>& has_string_constant,
    const std::function<bool(std::string_view)>& has_same_module_global,
    const std::function<std::string(std::string_view)>& render_private_data_label,
    const std::function<std::string(std::string_view)>& render_asm_symbol_name) {
  if (defined_functions.size() <= 1 || prepared_arch != c4c::TargetArch::X86_64) {
    return std::nullopt;
  }

  PreparedBoundedMultiDefinedCallLaneModuleRender rendered;
  std::unordered_set<std::string> emitted_string_names;
  std::unordered_set<std::string> emitted_same_module_global_names;
  bool saw_bounded_entry = false;

  for (const auto* candidate : defined_functions) {
    if (const auto rendered_trivial = render_trivial_defined_function(*candidate);
        rendered_trivial.has_value()) {
      rendered.rendered_functions += *rendered_trivial;
      continue;
    }

    if (saw_bounded_entry || candidate->return_type != c4c::backend::bir::TypeKind::I32 ||
        !candidate->params.empty() || candidate->blocks.size() != 1 ||
        candidate->blocks.front().label != "entry" ||
        candidate->blocks.front().terminator.kind != c4c::backend::bir::TerminatorKind::Return ||
        !candidate->blocks.front().terminator.value.has_value()) {
      return std::nullopt;
    }

    const auto candidate_return_register = minimal_function_return_register(*candidate);
    if (!candidate_return_register.has_value()) {
      return std::nullopt;
    }

    const auto candidate_layout =
        build_prepared_module_local_slot_layout(*candidate, prepared_arch);
    if (!candidate_layout.has_value()) {
      return std::nullopt;
    }
    const auto rendered_candidate =
        render_prepared_bounded_multi_defined_call_lane_body_if_supported(
            *candidate, defined_functions, *candidate_layout, *candidate_return_register,
            has_string_constant, has_same_module_global, render_private_data_label,
            render_asm_symbol_name);
    if (!rendered_candidate.has_value()) {
      return std::nullopt;
    }
    for (const auto& string_name : rendered_candidate->used_string_names) {
      if (emitted_string_names.insert(string_name).second) {
        rendered.used_string_names.push_back(string_name);
      }
    }
    for (const auto& global_name : rendered_candidate->used_same_module_global_names) {
      if (emitted_same_module_global_names.insert(global_name).second) {
        rendered.used_same_module_global_names.push_back(global_name);
      }
    }
    rendered.rendered_functions +=
        minimal_function_asm_prefix(*candidate) + rendered_candidate->body;
    saw_bounded_entry = true;
  }

  if (!saw_bounded_entry) {
    return std::nullopt;
  }

  return rendered;
}

std::optional<std::string>
render_prepared_bounded_multi_defined_call_lane_data_if_supported(
    const PreparedBoundedMultiDefinedCallLaneModuleRender& rendered_module,
    const c4c::backend::bir::Module& module,
    const std::unordered_set<std::string_view>& helper_global_names,
    const std::function<const c4c::backend::bir::StringConstant*(std::string_view)>&
        find_string_constant,
    const std::function<std::string(const c4c::backend::bir::StringConstant&)>&
        emit_string_constant_data,
    const std::function<std::optional<std::string>(const c4c::backend::bir::Global&)>&
        emit_same_module_global_data) {
  std::string rendered_data;
  std::unordered_set<std::string_view> used_same_module_globals(
      rendered_module.used_same_module_global_names.begin(),
      rendered_module.used_same_module_global_names.end());
  for (const auto& string_name : rendered_module.used_string_names) {
    const auto* string_constant = find_string_constant(string_name);
    if (string_constant == nullptr) {
      return std::nullopt;
    }
    rendered_data += emit_string_constant_data(*string_constant);
  }
  for (const auto& global : module.globals) {
    if (used_same_module_globals.find(global.name) == used_same_module_globals.end() &&
        helper_global_names.find(global.name) == helper_global_names.end()) {
      continue;
    }
    const auto rendered_global_data = emit_same_module_global_data(global);
    if (!rendered_global_data.has_value()) {
      return std::nullopt;
    }
    rendered_data += *rendered_global_data;
  }
  return rendered_data;
}

}  // namespace c4c::backend::x86

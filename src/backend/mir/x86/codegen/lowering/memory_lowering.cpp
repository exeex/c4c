#include "memory_lowering.hpp"

#include "frame_lowering.hpp"

namespace c4c::backend::x86 {

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

std::optional<std::string> render_prepared_value_home_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name) {
  const auto frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      function_name,
      value_name);
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset);
}

std::optional<std::string> render_prepared_named_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view value_name,
    std::size_t stack_byte_bias) {
  auto frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
      local_layout,
      stack_layout,
      function_addressing,
      prepared_names,
      function_locations,
      function_name,
      value_name);
  if (!frame_offset.has_value()) {
    frame_offset = find_prepared_value_home_frame_offset(
        local_layout, prepared_names, function_locations, value_name);
  }
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset + stack_byte_bias);
}

std::optional<std::string> render_prepared_named_payload_stack_address_if_supported(
    const PreparedModuleLocalSlotLayout& local_layout,
    const c4c::backend::prepare::PreparedStackLayout* stack_layout,
    const c4c::backend::prepare::PreparedAddressingFunction* function_addressing,
    const c4c::backend::prepare::PreparedNameTables* prepared_names,
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    c4c::FunctionNameId function_name,
    std::string_view pointer_name,
    std::size_t stack_byte_bias) {
  auto frame_offset = resolve_prepared_named_payload_frame_offset_if_supported(
      local_layout,
      prepared_names,
      function_locations,
      stack_layout,
      function_name,
      pointer_name);
  if (!frame_offset.has_value()) {
    frame_offset = find_prepared_authoritative_value_stack_offset_if_supported(
        local_layout,
        stack_layout,
        function_addressing,
        prepared_names,
        function_locations,
        function_name,
        pointer_name);
  }
  if (!frame_offset.has_value()) {
    return std::nullopt;
  }
  return render_prepared_stack_address_expr(*frame_offset + stack_byte_bias);
}

}  // namespace c4c::backend::x86

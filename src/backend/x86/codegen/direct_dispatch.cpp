#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

std::optional<std::string> try_emit_direct_bir_helper_module(
    const c4c::backend::bir::Module& module) {
  if (const auto asm_text = try_emit_minimal_affine_return_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_countdown_loop_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_scalar_global_load_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_extern_scalar_global_load_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_scalar_global_store_reload_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_global_store_return_and_entry_return_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_global_two_field_struct_store_sub_sub_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_repeated_printf_local_i32_calls_bir_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

std::optional<std::string> try_emit_direct_prepared_lir_helper_module(
    const c4c::codegen::lir::LirModule& module) {
  if (const auto asm_text = try_emit_minimal_constant_branch_return_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_temp_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_void_helper_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_void_return_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_void_extern_call_return_imm_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_param_slot_add_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_seventh_param_stack_add_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_mixed_reg_stack_param_add_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_register_aggregate_param_slot_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_extern_zero_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_arg_call_module(module); asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_helper_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_second_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_second_local_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_first_local_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_arg_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_first_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_second_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_two_arg_both_local_double_rewrite_call_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_variadic_sum2_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_variadic_double_bytes_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_counted_printf_ternary_loop_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_local_buffer_string_copy_printf_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_repeated_printf_immediates_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  if (const auto asm_text = try_emit_minimal_string_literal_char_module(module);
      asm_text.has_value()) {
    return asm_text;
  }
  return std::nullopt;
}

}  // namespace c4c::backend::x86

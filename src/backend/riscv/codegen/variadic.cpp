// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs
// The shared RISC-V C++ codegen surface does not exist yet, so this file keeps
// the Rust method boundaries as a source-level mirror while still translating
// the local variadic arithmetic into concrete C++ helpers.

#include <cstddef>
#include <cstdint>

namespace c4c::backend::riscv::codegen {

namespace {

constexpr std::size_t kRiscvVaListSlotBytes = 8;
constexpr std::size_t kRiscvVaListLongDoubleBytes = 16;

std::int64_t riscv_va_start_offset(std::size_t va_named_gp_count,
                                   std::size_t va_named_stack_bytes) {
  if (va_named_gp_count >= 8) {
    return 64 + static_cast<std::int64_t>(va_named_stack_bytes);
  }
  return static_cast<std::int64_t>(va_named_gp_count) * static_cast<std::int64_t>(kRiscvVaListSlotBytes);
}

std::size_t riscv_va_arg_step_bytes(bool is_long_double) {
  return is_long_double ? kRiscvVaListLongDoubleBytes : kRiscvVaListSlotBytes;
}

}  // namespace

// Source-level mirrors of the Rust impl methods:
//
// pub(super) fn emit_va_arg_impl(&mut self, dest: &Value, va_list_ptr: &Value, result_ty: IrType)
// pub(super) fn emit_va_start_impl(&mut self, va_list_ptr: &Value)
// pub(super) fn emit_va_copy_impl(&mut self, dest_ptr: &Value, src_ptr: &Value)
//
// The concrete codegen body still depends on the broader RiscvCodegen / State
// surface, which is not yet shared as C++ headers. The helpers above preserve
// the local arithmetic:
// - va_start stores a pointer to the next variadic argument, with the stack
//   offset derived from the number of named GP arguments and named stack bytes.
// - va_arg advances by 8 bytes for normal scalar args and 16 bytes for long
//   double values.
//
// A future pass can wire these helpers into the translated codegen surface once
// the shared backend API exists.

std::int64_t riscv_va_start_stack_offset(std::size_t va_named_gp_count,
                                         std::size_t va_named_stack_bytes) {
  return riscv_va_start_offset(va_named_gp_count, va_named_stack_bytes);
}

std::size_t riscv_va_arg_stack_step_bytes(bool is_long_double) {
  return riscv_va_arg_step_bytes(is_long_double);
}

}  // namespace c4c::backend::riscv::codegen

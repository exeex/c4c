#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_va_arg_impl(const Value& dest,
                                  const Value& va_list_ptr,
                                  IrType result_ty) {
  // The ref implementation branches between register-save-area and overflow
  // memory loads, with a special x87 path for long double.
  this->state.emit("    <va_arg>");
  this->store_rax_to(dest);
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_va_arg_struct_impl(const Value& dest_ptr,
                                         const Value& va_list_ptr,
                                         std::size_t size) {
  this->state.emit("    <va_arg-struct>");
}

void X86Codegen::emit_va_arg_struct_ex_impl(const Value& dest_ptr,
                                            const Value& va_list_ptr,
                                            std::size_t size,
                                            bool from_register_area) {
  this->state.emit("    <va_arg-struct-ex>");
}

void X86Codegen::emit_va_start_impl(const Value& va_list_ptr) {
  this->state.emit("    <va_start>");
}

void X86Codegen::emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr) {
  this->state.emit("    <va_copy>");
}

void X86Codegen::emit_partial_copy(std::int64_t offset, std::size_t remaining) {
  this->state.emit("    <partial-copy>");
}

}  // namespace c4c::backend::x86

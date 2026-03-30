#include "emit.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size) {
  if (val.has_value() && this->current_return_type.is_long_double()) {
    this->state.emit("    <return-long-double>");
    this->emit_epilogue_and_ret_impl(frame_size);
    return;
  }
  // Default lowering covers the normal integer and floating return paths.
  emit_return_default(*this, val, frame_size);
}

IrType X86Codegen::current_return_type_impl() const {
  return this->current_return_type;
}

void X86Codegen::emit_return_f32_to_reg_impl() { this->state.emit("    movd %eax, %xmm0"); }
void X86Codegen::emit_return_f64_to_reg_impl() { this->state.emit("    movq %rax, %xmm0"); }

void X86Codegen::emit_return_i128_to_regs_impl() {
  // The ref backend leaves INTEGER+INTEGER untouched and only reshuffles mixed
  // integer/SSE returns.
  if (this->func_ret_classes.size() == 2) {
    this->state.emit("    <i128-return-shuffle>");
  }
}

void X86Codegen::emit_return_f128_to_reg_impl() {
  this->state.emit("    pushq %rax");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_return_int_to_reg_impl() {
  // rax already carries the scalar integer return value.
}

void X86Codegen::emit_get_return_f64_second_impl(const Value& dest) {
  this->state.emit("    <load-return-f64-second>");
}

void X86Codegen::emit_set_return_f64_second_impl(const Operand& src) {
  this->state.emit("    <set-return-f64-second>");
}

void X86Codegen::emit_get_return_f32_second_impl(const Value& dest) {
  this->state.emit("    <load-return-f32-second>");
}

void X86Codegen::emit_set_return_f32_second_impl(const Operand& src) {
  this->state.emit("    <set-return-f32-second>");
}

void X86Codegen::emit_get_return_f128_second_impl(const Value& dest) {
  this->state.emit("    <load-return-f128-second>");
}

void X86Codegen::emit_set_return_f128_second_impl(const Operand& src) {
  this->state.emit("    <set-return-f128-second>");
}

}  // namespace c4c::backend::x86

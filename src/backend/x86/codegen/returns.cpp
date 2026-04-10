#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unwired_translated_returns_owner() {
  throw std::logic_error(
      "x86 translated returns owner methods are compiled for symbol/link coverage, but the exporter-backed X86Codegen return state is not wired yet");
}

}  // namespace

void X86Codegen::emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size) {
  (void)val;
  (void)frame_size;
  throw_unwired_translated_returns_owner();
}

IrType X86Codegen::current_return_type_impl() const {
  throw_unwired_translated_returns_owner();
  return static_cast<IrType>(0);
}

void X86Codegen::emit_return_f32_to_reg_impl() { throw_unwired_translated_returns_owner(); }
void X86Codegen::emit_return_f64_to_reg_impl() { throw_unwired_translated_returns_owner(); }
void X86Codegen::emit_return_i128_to_regs_impl() { throw_unwired_translated_returns_owner(); }
void X86Codegen::emit_return_f128_to_reg_impl() { throw_unwired_translated_returns_owner(); }

void X86Codegen::emit_return_int_to_reg_impl() { throw_unwired_translated_returns_owner(); }

void X86Codegen::emit_get_return_f64_second_impl(const Value& dest) {
  (void)dest;
  throw_unwired_translated_returns_owner();
}

void X86Codegen::emit_set_return_f64_second_impl(const Operand& src) {
  (void)src;
  throw_unwired_translated_returns_owner();
}

void X86Codegen::emit_get_return_f32_second_impl(const Value& dest) {
  (void)dest;
  throw_unwired_translated_returns_owner();
}

void X86Codegen::emit_set_return_f32_second_impl(const Operand& src) {
  (void)src;
  throw_unwired_translated_returns_owner();
}

void X86Codegen::emit_get_return_f128_second_impl(const Value& dest) {
  (void)dest;
  throw_unwired_translated_returns_owner();
}

void X86Codegen::emit_set_return_f128_second_impl(const Operand& src) {
  (void)src;
  throw_unwired_translated_returns_owner();
}

}  // namespace c4c::backend::x86

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

void X86Codegen::emit_return_f32_to_reg_impl() { this->state.emit("    movd %eax, %xmm0"); }

void X86Codegen::emit_return_f64_to_reg_impl() { this->state.emit("    movq %rax, %xmm0"); }

void X86Codegen::emit_return_i128_to_regs_impl() {
  if (this->call_ret_classes.size() != 2) {
    return;
  }

  const auto first = this->call_ret_classes[0];
  const auto second = this->call_ret_classes[1];
  if (first == EightbyteClass::Integer && second == EightbyteClass::Sse) {
    this->state.emit("    movq %rdx, %xmm0");
  } else if (first == EightbyteClass::Sse && second == EightbyteClass::Integer) {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movq %rdx, %rax");
  } else if (first == EightbyteClass::Sse && second == EightbyteClass::Sse) {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movq %rdx, %xmm1");
  }
}

void X86Codegen::emit_return_f128_to_reg_impl() {
  this->state.emit("    pushq %rax");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_return_int_to_reg_impl() {}

void X86Codegen::emit_get_return_f64_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movsd %xmm1, " + std::to_string(slot->raw) + "(%rbp)");
  }
}

void X86Codegen::emit_set_return_f64_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    movsd " + std::to_string(slot->raw) + "(%rbp), %xmm1");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    movq %rax, %xmm1");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_get_return_f32_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movss %xmm1, " + std::to_string(slot->raw) + "(%rbp)");
  }
}

void X86Codegen::emit_set_return_f32_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    movss " + std::to_string(slot->raw) + "(%rbp), %xmm1");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    movd %eax, %xmm1");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_get_return_f128_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    fstpt " + std::to_string(slot->raw) + "(%rbp)");
    this->state.emit("    fldt " + std::to_string(slot->raw) + "(%rbp)");
    this->state.emit("    subq $8, %rsp");
    this->state.emit("    fstpl (%rsp)");
    this->state.emit("    popq %rax");
    this->state.reg_cache.set_acc(dest.raw, false);
    this->state.f128_direct_slots.insert(dest.raw);
    return;
  }
  this->state.emit("    fstp %st(0)");
}

void X86Codegen::emit_set_return_f128_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    fldt " + std::to_string(slot->raw) + "(%rbp)");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    pushq %rax");
  this->state.emit("    fildq (%rsp)");
  this->state.emit("    addq $8, %rsp");
}

}  // namespace c4c::backend::x86

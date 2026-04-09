#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_float_binop_impl(const Value& dest,
                                       FloatOp op,
                                       const Operand& lhs,
                                       const Operand& rhs,
                                       IrType ty) {
  if (ty == IrType::F128) {
    this->emit_f128_load_to_x87(&lhs);
    this->emit_f128_load_to_x87(&rhs);
    const char* x87_op = op == FloatOp::Add ? "faddp"
                        : op == FloatOp::Sub ? "fsubrp"
                        : op == FloatOp::Mul ? "fmulp"
                                             : "fdivrp";
    this->state.emit(std::string("    ") + x87_op + " %st, %st(1)");
    if (auto dest_slot = this->state.get_slot(dest.0)) {
      this->state.out.emit_instr_rbp("    fstpt", dest_slot->0);
      this->state.out.emit_instr_rbp("    fldt", dest_slot->0);
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.set_acc(dest.0, false);
      this->state.f128_direct_slots.insert(dest.0);
    } else {
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.invalidate_acc();
      this->store_rax_to(dest);
    }
    return;
  }

  this->operand_to_rax(lhs);
  this->state.emit(ty == IrType::F32 ? "    movd %eax, %xmm0" : "    movq %rax, %xmm0");
  this->operand_to_rcx(rhs);
  this->state.emit(ty == IrType::F32 ? "    movd %ecx, %xmm1" : "    movq %rcx, %xmm1");
  const char* mnemonic = this->emit_float_binop_mnemonic_impl(op);
  const char* suffix = ty == IrType::F32 ? "ss" : "sd";
  this->state.emit(std::string("    ") + mnemonic + suffix + " %xmm1, %xmm0");
  this->state.emit(ty == IrType::F32 ? "    movd %xmm0, %eax" : "    movq %xmm0, %rax");
  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_float_binop_impl_impl(const std::string& /*mnemonic*/, IrType /*ty*/) {
}

const char* X86Codegen::emit_float_binop_mnemonic_impl(FloatOp op) const {
  switch (op) {
    case FloatOp::Add: return "add";
    case FloatOp::Sub: return "sub";
    case FloatOp::Mul: return "mul";
    case FloatOp::Div: return "div";
  }
  return "add";
}

void X86Codegen::emit_unaryop_impl(const Value& dest,
                                   IrUnaryOp op,
                                   const Operand& src,
                                   IrType ty) {
  if (ty == IrType::F128 && op == IrUnaryOp::Neg) {
    this->emit_f128_load_to_x87(&src);
    this->state.emit("    fchs");
    if (auto dest_slot = this->state.get_slot(dest.0)) {
      this->state.out.emit_instr_rbp("    fstpt", dest_slot->0);
      this->state.out.emit_instr_rbp("    fldt", dest_slot->0);
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.set_acc(dest.0, false);
      this->state.f128_direct_slots.insert(dest.0);
    } else {
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.invalidate_acc();
      this->store_rax_to(dest);
    }
    return;
  }

  emit_unaryop_default(*this, dest, op, src, ty);
}

}  // namespace c4c::backend::x86

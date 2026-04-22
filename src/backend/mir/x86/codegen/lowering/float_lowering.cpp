#include "float_lowering.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

void emit_unaryop_default(X86Codegen& codegen,
                          const Value& dest,
                          IrUnaryOp op,
                          const Operand& src,
                          IrType ty) {
  codegen.operand_to_rax(src);
  switch (op) {
    case IrUnaryOp::Neg:
      if (ty == IrType::F32 || ty == IrType::F64) {
        codegen.emit_float_neg_impl(ty);
      } else {
        codegen.emit_int_neg_impl(ty);
      }
      break;
    case IrUnaryOp::Not:
      codegen.emit_int_not_impl(ty);
      break;
    case IrUnaryOp::Clz:
      codegen.emit_int_clz_impl(ty);
      break;
    case IrUnaryOp::Ctz:
      codegen.emit_int_ctz_impl(ty);
      break;
    case IrUnaryOp::Bswap:
      codegen.emit_int_bswap_impl(ty);
      break;
    case IrUnaryOp::Popcount:
      codegen.emit_int_popcount_impl(ty);
      break;
  }
  codegen.store_rax_to(dest);
}

void X86Codegen::emit_float_neg_impl(IrType ty) {
  if (ty == IrType::F32) {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    movl $0x80000000, %ecx");
    this->state.emit("    movd %ecx, %xmm1");
    this->state.emit("    xorps %xmm1, %xmm0");
    this->state.emit("    movd %xmm0, %eax");
  } else {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movabsq $-9223372036854775808, %rcx");
    this->state.emit("    movq %rcx, %xmm1");
    this->state.emit("    xorpd %xmm1, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
  }
}

void X86Codegen::emit_float_binop_impl(const Value& dest,
                                       FloatOp op,
                                       const Operand& lhs,
                                       const Operand& rhs,
                                       IrType ty) {
  if (ty == IrType::F128) {
    this->emit_f128_load_to_x87(lhs);
    this->emit_f128_load_to_x87(rhs);
    const char* x87_op = op == FloatOp::Add ? "faddp"
                        : op == FloatOp::Sub ? "fsubrp"
                        : op == FloatOp::Mul ? "fmulp"
                                             : "fdivrp";
    this->state.emit(std::string("    ") + x87_op + " %st, %st(1)");
    this->emit_f128_load_finish(dest);
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
  throw std::logic_error("x86 emit_float_binop_impl should not be called directly");
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
    this->emit_f128_load_to_x87(src);
    this->state.emit("    fchs");
    this->emit_f128_load_finish(dest);
    return;
  }

  emit_unaryop_default(*this, dest, op, src, ty);
}

}  // namespace c4c::backend::x86

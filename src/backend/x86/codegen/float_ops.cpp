#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

bool use_32bit_unary(IrType ty) {
  return ty == IrType::I32 || ty == IrType::F32;
}

}  // namespace

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

  this->operand_to_rax(src);
  switch (op) {
    case IrUnaryOp::Neg:
      if (ty == IrType::F32 || ty == IrType::F64) {
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
      } else {
        this->state.emit(use_32bit_unary(ty) ? "    negl %eax" : "    negq %rax");
      }
      break;
    case IrUnaryOp::Not:
      this->state.emit(use_32bit_unary(ty) ? "    notl %eax" : "    notq %rax");
      break;
    case IrUnaryOp::Clz:
      this->state.emit(use_32bit_unary(ty) ? "    lzcntl %eax, %eax"
                                           : "    lzcntq %rax, %rax");
      break;
    case IrUnaryOp::Ctz:
      this->state.emit(use_32bit_unary(ty) ? "    tzcntl %eax, %eax"
                                           : "    tzcntq %rax, %rax");
      break;
    case IrUnaryOp::Bswap:
      this->state.emit(use_32bit_unary(ty) ? "    bswapl %eax" : "    bswapq %rax");
      break;
    case IrUnaryOp::Popcount:
      this->state.emit(use_32bit_unary(ty) ? "    popcntl %eax, %eax"
                                           : "    popcntq %rax, %rax");
      break;
  }
  this->store_rax_to(dest);
}

}  // namespace c4c::backend::x86

#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

enum class IntUnaryWidth : unsigned char {
  W16,
  W32,
  W64,
};

std::optional<IntUnaryWidth> int_unary_width(IrType ty) {
  switch (ty) {
    case IrType::I16:
    case IrType::U16:
      return IntUnaryWidth::W16;
    case IrType::I32:
    case IrType::U32:
    case IrType::F32:
      return IntUnaryWidth::W32;
    case IrType::I64:
    case IrType::U64:
      return IntUnaryWidth::W64;
    default:
      return std::nullopt;
  }
}

const char* unary_neg_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    negw %ax";
    case IntUnaryWidth::W32:
      return "    negl %eax";
    case IntUnaryWidth::W64:
      return "    negq %rax";
  }
  return "    negq %rax";
}

const char* unary_not_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    notw %ax";
    case IntUnaryWidth::W32:
      return "    notl %eax";
    case IntUnaryWidth::W64:
      return "    notq %rax";
  }
  return "    notq %rax";
}

const char* unary_clz_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    lzcntw %ax, %ax";
    case IntUnaryWidth::W32:
      return "    lzcntl %eax, %eax";
    case IntUnaryWidth::W64:
      return "    lzcntq %rax, %rax";
  }
  return "    lzcntq %rax, %rax";
}

const char* unary_ctz_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    tzcntw %ax, %ax";
    case IntUnaryWidth::W32:
      return "    tzcntl %eax, %eax";
    case IntUnaryWidth::W64:
      return "    tzcntq %rax, %rax";
  }
  return "    tzcntq %rax, %rax";
}

const char* unary_bswap_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    rolw $8, %ax";
    case IntUnaryWidth::W32:
      return "    bswapl %eax";
    case IntUnaryWidth::W64:
      return "    bswapq %rax";
  }
  return "    bswapq %rax";
}

const char* unary_popcount_mnemonic(IrType ty) {
  switch (int_unary_width(ty).value_or(IntUnaryWidth::W64)) {
    case IntUnaryWidth::W16:
      return "    popcntw %ax, %ax";
    case IntUnaryWidth::W32:
      return "    popcntl %eax, %eax";
    case IntUnaryWidth::W64:
      return "    popcntq %rax, %rax";
  }
  return "    popcntq %rax, %rax";
}

}  // namespace

void emit_unaryop_default(X86Codegen& codegen,
                          const Value& dest,
                          IrUnaryOp op,
                          const Operand& src,
                          IrType ty) {
  codegen.operand_to_rax(src);
  switch (op) {
    case IrUnaryOp::Neg:
      if (ty == IrType::F32 || ty == IrType::F64) {
        if (ty == IrType::F32) {
          codegen.state.emit("    movd %eax, %xmm0");
          codegen.state.emit("    movl $0x80000000, %ecx");
          codegen.state.emit("    movd %ecx, %xmm1");
          codegen.state.emit("    xorps %xmm1, %xmm0");
          codegen.state.emit("    movd %xmm0, %eax");
        } else {
          codegen.state.emit("    movq %rax, %xmm0");
          codegen.state.emit("    movabsq $-9223372036854775808, %rcx");
          codegen.state.emit("    movq %rcx, %xmm1");
          codegen.state.emit("    xorpd %xmm1, %xmm0");
          codegen.state.emit("    movq %xmm0, %rax");
        }
      } else {
        codegen.state.emit(unary_neg_mnemonic(ty));
      }
      break;
    case IrUnaryOp::Not:
      codegen.state.emit(unary_not_mnemonic(ty));
      break;
    case IrUnaryOp::Clz:
      codegen.state.emit(unary_clz_mnemonic(ty));
      break;
    case IrUnaryOp::Ctz:
      codegen.state.emit(unary_ctz_mnemonic(ty));
      break;
    case IrUnaryOp::Bswap:
      codegen.state.emit(unary_bswap_mnemonic(ty));
      break;
    case IrUnaryOp::Popcount:
      codegen.state.emit(unary_popcount_mnemonic(ty));
      break;
  }
  codegen.store_rax_to(dest);
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

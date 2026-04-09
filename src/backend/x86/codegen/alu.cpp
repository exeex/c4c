#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

const char* shift_mnemonic_32(IrBinOp op) {
  switch (op) {
    case IrBinOp::Shl: return "shll";
    case IrBinOp::AShr: return "sarl";
    case IrBinOp::LShr: return "shrl";
    default: return "shll";
  }
}

const char* shift_mnemonic_64(IrBinOp op) {
  switch (op) {
    case IrBinOp::Shl: return "shlq";
    case IrBinOp::AShr: return "sarq";
    case IrBinOp::LShr: return "shrq";
    default: return "shlq";
  }
}

}  // namespace

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

void X86Codegen::emit_int_neg_impl(IrType /*ty*/) { this->state.emit("    negq %rax"); }
void X86Codegen::emit_int_not_impl(IrType /*ty*/) { this->state.emit("    notq %rax"); }

void X86Codegen::emit_int_clz_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    lzcntl %eax, %eax"
                                                           : "    lzcntq %rax, %rax");
}

void X86Codegen::emit_int_ctz_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    tzcntl %eax, %eax"
                                                           : "    tzcntq %rax, %rax");
}

void X86Codegen::emit_int_bswap_impl(IrType ty) {
  if (ty == IrType::I16 || ty == IrType::U16) {
    this->state.emit("    rolw $8, %ax");
  } else if (ty == IrType::I32 || ty == IrType::U32) {
    this->state.emit("    bswapl %eax");
  } else {
    this->state.emit("    bswapq %rax");
  }
}

void X86Codegen::emit_int_popcount_impl(IrType ty) {
  this->state.emit(ty == IrType::I32 || ty == IrType::U32 ? "    popcntl %eax, %eax"
                                                           : "    popcntq %rax, %rax");
}

void X86Codegen::emit_int_binop_impl(const Value& dest,
                                     IrBinOp op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty) {
  const bool use_32bit = ty == IrType::I32 || ty == IrType::U32;
  const bool is_unsigned = ty.is_unsigned();

  if (auto dest_phys = this->dest_reg(dest)) {
    const bool is_simple_alu = op == IrBinOp::Add || op == IrBinOp::Sub ||
                               op == IrBinOp::And || op == IrBinOp::Or ||
                               op == IrBinOp::Xor || op == IrBinOp::Mul;
    if (is_simple_alu) {
      this->emit_alu_reg_direct(op, lhs, rhs, dest_phys.value(), use_32bit, is_unsigned);
      return;
    }
    if (op == IrBinOp::Shl || op == IrBinOp::AShr || op == IrBinOp::LShr) {
      this->emit_shift_reg_direct(op, lhs, rhs, dest_phys.value(), use_32bit, is_unsigned);
      return;
    }
  }

  if (this->try_emit_acc_immediate(dest, op, lhs, rhs, use_32bit, is_unsigned)) {
    return;
  }

  this->operand_to_rax(lhs);
  this->operand_to_rcx(rhs);

  switch (op) {
    case IrBinOp::Add:
    case IrBinOp::Sub:
    case IrBinOp::Mul:
      if (use_32bit) {
        const char* mnem = op == IrBinOp::Add ? "add"
                            : op == IrBinOp::Sub ? "sub"
                            : "imul";
        this->state.emit(std::string("    ") + mnem + "l %ecx, %eax");
        if (!is_unsigned) {
          this->state.emit("    cltq");
        }
      } else {
        const char* mnem = op == IrBinOp::Add ? "add"
                            : op == IrBinOp::Sub ? "sub"
                            : "imul";
        this->state.emit(std::string("    ") + mnem + "q %rcx, %rax");
      }
      break;
    case IrBinOp::SDiv:
      if (use_32bit) {
        this->state.emit("    cltd");
        this->state.emit("    idivl %ecx");
        this->state.emit("    cltq");
      } else {
        this->state.emit("    cqto");
        this->state.emit("    idivq %rcx");
      }
      break;
    case IrBinOp::UDiv:
      this->state.emit("    xorl %edx, %edx");
      this->state.emit(use_32bit ? "    divl %ecx" : "    divq %rcx");
      break;
    case IrBinOp::SRem:
      if (use_32bit) {
        this->state.emit("    cltd");
        this->state.emit("    idivl %ecx");
        this->state.emit("    movl %edx, %eax");
        this->state.emit("    cltq");
      } else {
        this->state.emit("    cqto");
        this->state.emit("    idivq %rcx");
        this->state.emit("    movq %rdx, %rax");
      }
      break;
    case IrBinOp::URem:
      this->state.emit("    xorl %edx, %edx");
      if (use_32bit) {
        this->state.emit("    divl %ecx");
        this->state.emit("    movl %edx, %eax");
      } else {
        this->state.emit("    divq %rcx");
        this->state.emit("    movq %rdx, %rax");
      }
      break;
    case IrBinOp::And:
    case IrBinOp::Or:
    case IrBinOp::Xor:
      this->state.emit(op == IrBinOp::And ? "    andq %rcx, %rax"
                                          : op == IrBinOp::Or ? "    orq %rcx, %rax"
                                                              : "    xorq %rcx, %rax");
      break;
    case IrBinOp::Shl:
    case IrBinOp::AShr:
    case IrBinOp::LShr:
      if (use_32bit) {
        const char* mnem32 = shift_mnemonic_32(op);
        this->state.emit(std::string("    ") + mnem32 + " %cl, %eax");
        if (!is_unsigned && op != IrBinOp::LShr) {
          this->state.emit("    cltq");
        }
      } else {
        const char* mnem64 = shift_mnemonic_64(op);
        this->state.emit(std::string("    ") + mnem64 + " %cl, %rax");
      }
      break;
  }

  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_copy_i128_impl(const Value& dest, const Operand& src) {
  this->operand_to_rax_rdx(src);
  this->store_rax_rdx_to(dest);
}

}  // namespace c4c::backend::x86

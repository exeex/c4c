#include "x86_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

void X86Codegen::emit_i128_prep_binop_impl(const Operand& lhs, const Operand& rhs) {
  this->prep_i128_binop(lhs, rhs);
}

void X86Codegen::emit_i128_add_impl() {
  this->state.emit("    addq %rcx, %rax");
  this->state.emit("    adcq %rsi, %rdx");
}

void X86Codegen::emit_i128_sub_impl() {
  this->state.emit("    subq %rcx, %rax");
  this->state.emit("    sbbq %rsi, %rdx");
}

void X86Codegen::emit_i128_mul_impl() {
  this->state.emit("    pushq %rdx");
  this->state.emit("    pushq %rax");
  this->state.emit("    movq %rcx, %r8");
  this->state.emit("    movq %rsi, %r9");
  this->state.emit("    popq %rax");
  this->state.emit("    popq %rdi");
  this->state.emit("    movq %rdi, %rcx");
  this->state.emit("    imulq %r8, %rcx");
  this->state.emit("    movq %rax, %rsi");
  this->state.emit("    imulq %r9, %rsi");
  this->state.emit("    mulq %r8");
  this->state.emit("    addq %rcx, %rdx");
  this->state.emit("    addq %rsi, %rdx");
}

void X86Codegen::emit_i128_and_impl() {
  this->state.emit("    andq %rcx, %rax");
  this->state.emit("    andq %rsi, %rdx");
}

void X86Codegen::emit_i128_or_impl() {
  this->state.emit("    orq %rcx, %rax");
  this->state.emit("    orq %rsi, %rdx");
}

void X86Codegen::emit_i128_xor_impl() {
  this->state.emit("    xorq %rcx, %rax");
  this->state.emit("    xorq %rsi, %rdx");
}

void X86Codegen::emit_i128_shl_impl() {
  this->state.emit("    shldq %cl, %rax, %rdx");
  this->state.emit("    shlq %cl, %rax");
  this->state.emit("    testb $64, %cl");
  this->state.emit("    je 1f");
  this->state.emit("    movq %rax, %rdx");
  this->state.emit("    xorl %eax, %eax");
  this->state.emit("1:");
}

void X86Codegen::emit_i128_lshr_impl() {
  this->state.emit("    shrdq %cl, %rdx, %rax");
  this->state.emit("    shrq %cl, %rdx");
  this->state.emit("    testb $64, %cl");
  this->state.emit("    je 1f");
  this->state.emit("    movq %rdx, %rax");
  this->state.emit("    xorl %edx, %edx");
  this->state.emit("1:");
}

void X86Codegen::emit_i128_ashr_impl() {
  this->state.emit("    shrdq %cl, %rdx, %rax");
  this->state.emit("    sarq %cl, %rdx");
  this->state.emit("    testb $64, %cl");
  this->state.emit("    je 1f");
  this->state.emit("    movq %rdx, %rax");
  this->state.emit("    sarq $63, %rdx");
  this->state.emit("1:");
}

void X86Codegen::emit_i128_prep_shift_lhs_impl(const Operand& lhs) {
  this->operand_to_rax_rdx(lhs);
}

void X86Codegen::emit_i128_shl_const_impl(std::uint32_t amount) {
  const std::uint32_t shift = amount & 127;
  if (shift == 0) {
    return;
  }
  if (shift == 64) {
    this->state.emit("    movq %rax, %rdx");
    this->state.emit("    xorl %eax, %eax");
    return;
  }
  if (shift > 64) {
    this->state.emit(std::string("    shlq $") + std::to_string(shift - 64) + ", %rax");
    this->state.emit("    movq %rax, %rdx");
    this->state.emit("    xorl %eax, %eax");
    return;
  }
  this->state.emit(std::string("    shldq $") + std::to_string(shift) + ", %rax, %rdx");
  this->state.emit(std::string("    shlq $") + std::to_string(shift) + ", %rax");
}

void X86Codegen::emit_i128_lshr_const_impl(std::uint32_t amount) {
  const std::uint32_t shift = amount & 127;
  if (shift == 0) {
    return;
  }
  if (shift == 64) {
    this->state.emit("    movq %rdx, %rax");
    this->state.emit("    xorl %edx, %edx");
    return;
  }
  if (shift > 64) {
    this->state.emit(std::string("    shrq $") + std::to_string(shift - 64) + ", %rdx");
    this->state.emit("    movq %rdx, %rax");
    this->state.emit("    xorl %edx, %edx");
    return;
  }
  this->state.emit(std::string("    shrdq $") + std::to_string(shift) + ", %rdx, %rax");
  this->state.emit(std::string("    shrq $") + std::to_string(shift) + ", %rdx");
}

void X86Codegen::emit_i128_ashr_const_impl(std::uint32_t amount) {
  const std::uint32_t shift = amount & 127;
  if (shift == 0) {
    return;
  }
  if (shift == 64) {
    this->state.emit("    movq %rdx, %rax");
    this->state.emit("    sarq $63, %rdx");
    return;
  }
  if (shift > 64) {
    this->state.emit(std::string("    sarq $") + std::to_string(shift - 64) + ", %rdx");
    this->state.emit("    movq %rdx, %rax");
    this->state.emit("    sarq $63, %rdx");
    return;
  }
  this->state.emit(std::string("    shrdq $") + std::to_string(shift) + ", %rdx, %rax");
  this->state.emit(std::string("    sarq $") + std::to_string(shift) + ", %rdx");
}

void X86Codegen::emit_i128_divrem_call_impl(const std::string& func_name,
                                            const Operand& lhs,
                                            const Operand& rhs) {
  this->operand_to_rax_rdx(rhs);
  this->state.emit("    pushq %rdx");
  this->state.emit("    pushq %rax");
  this->operand_to_rax_rdx(lhs);
  this->state.emit("    movq %rax, %rdi");
  this->state.emit("    movq %rdx, %rsi");
  this->state.emit("    popq %rdx");
  this->state.emit("    popq %rcx");
  this->state.emit(std::string("    call ") + func_name + "@PLT");
}

void X86Codegen::emit_i128_store_result_impl(const Value& dest) {
  this->store_rax_rdx_to(dest);
}

void X86Codegen::emit_i128_to_float_call_impl(const Operand& src,
                                              bool from_signed,
                                              IrType to_ty) {
  this->operand_to_rax_rdx(src);
  this->state.emit("    movq %rax, %rdi");
  this->state.emit("    movq %rdx, %rsi");
  const char* func_name = nullptr;
  if (from_signed && to_ty == IrType::F64) {
    func_name = "__floattidf";
  } else if (from_signed && to_ty == IrType::F32) {
    func_name = "__floattisf";
  } else if (!from_signed && to_ty == IrType::F64) {
    func_name = "__floatuntidf";
  } else if (!from_signed && to_ty == IrType::F32) {
    func_name = "__floatuntisf";
  } else {
    throw std::invalid_argument("unsupported i128-to-float conversion");
  }
  this->state.emit(std::string("    call ") + func_name + "@PLT");
  this->state.reg_cache.invalidate_all();
  if (to_ty == IrType::F32) {
    this->state.emit("    movd %xmm0, %eax");
  } else {
    this->state.emit("    movq %xmm0, %rax");
  }
}

void X86Codegen::emit_float_to_i128_call_impl(const Operand& src,
                                              bool to_signed,
                                              IrType from_ty) {
  this->operand_to_rax(src);
  if (from_ty == IrType::F32) {
    this->state.emit("    movd %eax, %xmm0");
  } else {
    this->state.emit("    movq %rax, %xmm0");
  }
  const char* func_name = nullptr;
  if (to_signed && from_ty == IrType::F64) {
    func_name = "__fixdfti";
  } else if (to_signed && from_ty == IrType::F32) {
    func_name = "__fixsfti";
  } else if (!to_signed && from_ty == IrType::F64) {
    func_name = "__fixunsdfti";
  } else if (!to_signed && from_ty == IrType::F32) {
    func_name = "__fixunssfti";
  } else {
    throw std::invalid_argument("unsupported float-to-i128 conversion");
  }
  this->state.emit(std::string("    call ") + func_name + "@PLT");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_i128_cmp_eq_impl(bool is_ne) {
  this->state.emit("    xorq %rcx, %rax");
  this->state.emit("    xorq %rsi, %rdx");
  this->state.emit("    orq %rdx, %rax");
  this->state.emit(is_ne ? "    setne %al" : "    sete %al");
  this->state.emit("    movzbq %al, %rax");
}

void X86Codegen::emit_i128_cmp_ordered_impl(IrCmpOp op) {
  this->state.emit("    cmpq %rsi, %rdx");
  const char* set_hi = nullptr;
  switch (op) {
    case IrCmpOp::Slt:
    case IrCmpOp::Sle:
      set_hi = "setl";
      break;
    case IrCmpOp::Sgt:
    case IrCmpOp::Sge:
      set_hi = "setg";
      break;
    case IrCmpOp::Ult:
    case IrCmpOp::Ule:
      set_hi = "setb";
      break;
    case IrCmpOp::Ugt:
    case IrCmpOp::Uge:
      set_hi = "seta";
      break;
    default:
      throw std::invalid_argument("i128 ordered cmp got equality op");
  }
  this->state.emit(std::string("    ") + set_hi + " %r8b");
  this->state.emit("    jne 1f");
  this->state.emit("    cmpq %rcx, %rax");
  const char* set_lo = nullptr;
  switch (op) {
    case IrCmpOp::Slt:
    case IrCmpOp::Ult:
      set_lo = "setb";
      break;
    case IrCmpOp::Sle:
    case IrCmpOp::Ule:
      set_lo = "setbe";
      break;
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
      set_lo = "seta";
      break;
    case IrCmpOp::Sge:
    case IrCmpOp::Uge:
      set_lo = "setae";
      break;
    default:
      throw std::invalid_argument("i128 ordered cmp got equality op");
  }
  this->state.emit(std::string("    ") + set_lo + " %r8b");
  this->state.emit("1:");
  this->state.emit("    movzbq %r8b, %rax");
}

void X86Codegen::emit_i128_cmp_store_result_impl(const Value& dest) {
  this->store_rax_to(dest);
}

void X86Codegen::emit_load_acc_pair_impl(const Operand& op) { this->operand_to_rax_rdx(op); }
void X86Codegen::emit_store_acc_pair_impl(const Value& dest) { this->store_rax_rdx_to(dest); }
void X86Codegen::emit_sign_extend_acc_high_impl() { this->state.emit("    cqto"); }
void X86Codegen::emit_zero_acc_high_impl() { this->state.emit("    xorl %edx, %edx"); }
void X86Codegen::emit_store_pair_to_slot_impl(StackSlot slot) {
  this->state.emit(std::string("    movq %rax, ") + std::to_string(slot.raw) + "(%rbp)");
  this->state.emit(std::string("    movq %rdx, ") + std::to_string(slot.raw + 8) + "(%rbp)");
}
void X86Codegen::emit_load_pair_from_slot_impl(StackSlot slot) {
  this->state.emit(std::string("    movq ") + std::to_string(slot.raw) + "(%rbp), %rax");
  this->state.emit(std::string("    movq ") + std::to_string(slot.raw + 8) + "(%rbp), %rdx");
}
void X86Codegen::emit_save_acc_pair_impl() {
  this->state.emit("    movq %rax, %rsi");
  this->state.emit("    movq %rdx, %rdi");
}
void X86Codegen::emit_store_pair_indirect_impl() {
  this->state.emit("    movq %rsi, (%rcx)");
  this->state.emit("    movq %rdi, 8(%rcx)");
}
void X86Codegen::emit_load_pair_indirect_impl() {
  this->state.emit("    movq (%rcx), %rax");
  this->state.emit("    movq 8(%rcx), %rdx");
}
void X86Codegen::emit_i128_neg_impl() {
  this->state.emit("    notq %rax");
  this->state.emit("    notq %rdx");
  this->state.emit("    addq $1, %rax");
  this->state.emit("    adcq $0, %rdx");
}
void X86Codegen::emit_i128_not_impl() {
  this->state.emit("    notq %rax");
  this->state.emit("    notq %rdx");
}

}  // namespace c4c::backend::x86

#include "x86_codegen.hpp"

#include <limits>
#include <string>

namespace c4c::backend::x86 {

namespace {

const char* cmp_setcc(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq: return "sete";
    case IrCmpOp::Ne: return "setne";
    case IrCmpOp::Slt: return "setl";
    case IrCmpOp::Sle: return "setle";
    case IrCmpOp::Sgt: return "setg";
    case IrCmpOp::Sge: return "setge";
    case IrCmpOp::Ult: return "setb";
    case IrCmpOp::Ule: return "setbe";
    case IrCmpOp::Ugt: return "seta";
    case IrCmpOp::Uge: return "setae";
  }
  return "sete";
}

const char* cmp_jcc(IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq: return "je";
    case IrCmpOp::Ne: return "jne";
    case IrCmpOp::Slt: return "jl";
    case IrCmpOp::Sle: return "jle";
    case IrCmpOp::Sgt: return "jg";
    case IrCmpOp::Sge: return "jge";
    case IrCmpOp::Ult: return "jb";
    case IrCmpOp::Ule: return "jbe";
    case IrCmpOp::Ugt: return "ja";
    case IrCmpOp::Uge: return "jae";
  }
  return "je";
}

void emit_int_cmp_insn_typed(X86Codegen& codegen,
                             const Operand& lhs,
                             const Operand& rhs,
                             bool use_32bit) {
  codegen.operand_to_rax(lhs);
  codegen.operand_to_rcx(rhs);
  codegen.state.emit(use_32bit ? "    cmpl %ecx, %eax" : "    cmpq %rcx, %rax");
}

void emit_select_cond_to_rdx(X86Codegen& codegen, const Operand& cond) {
  if (cond.immediate.has_value()) {
    const auto value = *cond.immediate;
    if (value == 0) {
      codegen.state.emit("    xorl %edx, %edx");
    } else if (value >= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min()) &&
               value <= static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max())) {
      codegen.state.out.emit_instr_imm_reg("    movq", value, "rdx");
    } else {
      codegen.state.emit("    movabsq $" + std::to_string(value) + ", %rdx");
    }
    return;
  }
  codegen.operand_to_reg(cond, "rdx");
}

void emit_float_cmp_result(X86CodegenState& state, IrCmpOp op) {
  switch (op) {
    case IrCmpOp::Eq:
      state.emit("    setnp %al");
      state.emit("    sete %cl");
      state.emit("    andb %cl, %al");
      return;
    case IrCmpOp::Ne:
      state.emit("    setp %al");
      state.emit("    setne %cl");
      state.emit("    orb %cl, %al");
      return;
    case IrCmpOp::Slt:
    case IrCmpOp::Ult:
    case IrCmpOp::Sgt:
    case IrCmpOp::Ugt:
      state.emit("    seta %al");
      return;
    case IrCmpOp::Sle:
    case IrCmpOp::Ule:
    case IrCmpOp::Sge:
    case IrCmpOp::Uge:
      state.emit("    setae %al");
      return;
  }
}

}  // namespace

void X86Codegen::emit_float_cmp_impl(const Value& dest,
                                     IrCmpOp op,
                                     const Operand& lhs,
                                     const Operand& rhs,
                                     IrType ty) {
  if (ty == IrType::F128) {
    this->emit_f128_cmp_impl(dest, op, lhs, rhs);
    return;
  }

  const bool is_f32 = ty == IrType::F32;
  const bool swap_operands = op == IrCmpOp::Slt || op == IrCmpOp::Ult || op == IrCmpOp::Sle ||
                             op == IrCmpOp::Ule;
  const Operand& first = swap_operands ? rhs : lhs;
  const Operand& second = swap_operands ? lhs : rhs;

  this->operand_to_rax(first);
  this->state.emit(is_f32 ? "    movd %eax, %xmm0" : "    movq %rax, %xmm0");
  this->operand_to_rcx(second);
  this->state.emit(is_f32 ? "    movd %ecx, %xmm1" : "    movq %rcx, %xmm1");
  this->state.emit(is_f32 ? "    ucomiss %xmm1, %xmm0" : "    ucomisd %xmm1, %xmm0");
  emit_float_cmp_result(this->state, op);
  this->state.emit("    movzbq %al, %rax");
  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_f128_cmp_impl(const Value& dest,
                                    IrCmpOp op,
                                    const Operand& lhs,
                                    const Operand& rhs) {
  const bool swap_x87 = op == IrCmpOp::Slt || op == IrCmpOp::Ult || op == IrCmpOp::Sle ||
                        op == IrCmpOp::Ule;
  const Operand& first_x87 = swap_x87 ? lhs : rhs;
  const Operand& second_x87 = swap_x87 ? rhs : lhs;

  this->emit_f128_load_to_x87(first_x87);
  this->emit_f128_load_to_x87(second_x87);
  this->state.emit("    fucomip %st(1), %st");
  this->state.emit("    fstp %st(0)");
  emit_float_cmp_result(this->state, op);
  this->state.emit("    movzbq %al, %rax");
  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_int_cmp_impl(const Value& dest,
                                   IrCmpOp op,
                                   const Operand& lhs,
                                   const Operand& rhs,
                                   IrType ty) {
  const bool use_32bit = ty == IrType::I32 || ty == IrType::U32;
  emit_int_cmp_insn_typed(*this, lhs, rhs, use_32bit);
  this->state.emit(std::string("    ") + cmp_setcc(op) + " %al");
  this->state.emit("    movzbq %al, %rax");
  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

void X86Codegen::emit_fused_cmp_branch_impl(IrCmpOp op,
                                            const Operand& lhs,
                                            const Operand& rhs,
                                            IrType ty,
                                            const std::string& true_label,
                                            const std::string& false_label) {
  const bool use_32bit = ty == IrType::I32 || ty == IrType::U32;
  emit_int_cmp_insn_typed(*this, lhs, rhs, use_32bit);
  this->state.emit(std::string("    ") + cmp_jcc(op) + " " + true_label);
  this->state.emit("    jmp " + false_label);
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_fused_cmp_branch_blocks_impl(IrCmpOp op,
                                                   const Operand& lhs,
                                                   const Operand& rhs,
                                                   IrType ty,
                                                   BlockId true_block,
                                                   BlockId false_block) {
  const bool use_32bit = ty == IrType::I32 || ty == IrType::U32;
  emit_int_cmp_insn_typed(*this, lhs, rhs, use_32bit);
  this->state.emit(std::string("    ") + cmp_jcc(op) + " .L" + std::to_string(true_block.raw));
  this->state.emit("    jmp .L" + std::to_string(false_block.raw));
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_cond_branch_blocks_impl(const Operand& cond,
                                              BlockId true_block,
                                              BlockId false_block) {
  this->operand_to_rax(cond);
  this->state.emit("    testq %rax, %rax");
  this->state.emit("    jne .L" + std::to_string(true_block.raw));
  this->state.emit("    jmp .L" + std::to_string(false_block.raw));
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_select_impl(const Value& dest,
                                  const Operand& cond,
                                  const Operand& true_val,
                                  const Operand& false_val,
                                  IrType /*ty*/) {
  this->operand_to_rax(false_val);
  this->operand_to_rcx(true_val);
  emit_select_cond_to_rdx(*this, cond);
  this->state.emit("    testq %rdx, %rdx");
  this->state.emit("    cmovne %rcx, %rax");
  this->state.reg_cache.invalidate_acc();
  this->store_rax_to(dest);
}

}  // namespace c4c::backend::x86

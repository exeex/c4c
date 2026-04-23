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

std::optional<std::int64_t> X86Codegen::emit_f128_resolve_addr(const SlotAddr& addr,
                                                               std::uint32_t ptr_id,
                                                               std::int64_t offset) {
  switch (addr.kind) {
    case SlotAddr::Kind::Direct: return addr.slot.raw + offset;
    case SlotAddr::Kind::OverAligned:
      this->emit_alloca_aligned_addr_impl(addr.slot, addr.value_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      return std::nullopt;
    case SlotAddr::Kind::Indirect:
      this->emit_load_ptr_from_slot_impl(addr.slot, ptr_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      return std::nullopt;
  }
  return std::nullopt;
}

void X86Codegen::emit_f128_fldt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  if (const auto rbp_off = this->emit_f128_resolve_addr(addr, ptr_id, offset)) {
    this->state.out.emit_instr_rbp("    fld", *rbp_off);
    return;
  }
  this->state.emit("    fld (%rcx)");
}

void X86Codegen::emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  if (const auto rbp_off = this->emit_f128_resolve_addr(addr, ptr_id, offset)) {
    this->state.out.emit_instr_rbp("    fstp", *rbp_off);
    return;
  }
  this->state.emit("    fstp (%rcx)");
}

void X86Codegen::emit_f128_store_raw_bytes(const SlotAddr& addr,
                                           std::uint32_t ptr_id,
                                           std::int64_t offset,
                                           std::uint64_t lo,
                                           std::uint64_t hi) {
  switch (addr.kind) {
    case SlotAddr::Kind::Direct: {
      const auto rbp_off = addr.slot.raw + offset;
      this->state.out.emit_instr_imm_reg("    movabsq", static_cast<std::int64_t>(lo), "rax");
      this->state.emit("    movq %rax, " + std::to_string(rbp_off) + "(%rbp)");
      if (hi != 0) {
        this->state.out.emit_instr_imm_reg("    movq", static_cast<std::int64_t>(hi), "rax");
      } else {
        this->state.emit("    xorl %eax, %eax");
      }
      this->state.emit("    movq %rax, " + std::to_string(rbp_off + 8) + "(%rbp)");
      return;
    }
    case SlotAddr::Kind::OverAligned:
      this->emit_alloca_aligned_addr_impl(addr.slot, addr.value_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      this->state.out.emit_instr_imm_reg("    movabsq", static_cast<std::int64_t>(lo), "rax");
      this->state.emit("    movq %rax, (%rcx)");
      if (hi != 0) {
        this->state.out.emit_instr_imm_reg("    movq", static_cast<std::int64_t>(hi), "rax");
      } else {
        this->state.emit("    xorl %eax, %eax");
      }
      this->state.emit("    movq %rax, 8(%rcx)");
      return;
    case SlotAddr::Kind::Indirect:
      this->emit_load_ptr_from_slot_impl(addr.slot, ptr_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      this->state.out.emit_instr_imm_reg("    movabsq", static_cast<std::int64_t>(lo), "rax");
      this->state.emit("    movq %rax, (%rcx)");
      if (hi != 0) {
        this->state.out.emit_instr_imm_reg("    movq", static_cast<std::int64_t>(hi), "rax");
      } else {
        this->state.emit("    xorl %eax, %eax");
      }
      this->state.emit("    movq %rax, 8(%rcx)");
      return;
  }
}

void X86Codegen::emit_f128_store_f64_via_x87(const SlotAddr& addr,
                                             std::uint32_t ptr_id,
                                             std::int64_t offset) {
  switch (addr.kind) {
    case SlotAddr::Kind::Direct:
      this->state.emit("    pushq %rax");
      this->state.emit("    fldl (%rsp)");
      this->state.emit("    addq $8, %rsp");
      this->state.out.emit_instr_rbp("    fstp", addr.slot.raw + offset);
      return;
    case SlotAddr::Kind::OverAligned:
      this->state.emit("    movq %rax, %rdx");
      this->emit_alloca_aligned_addr_impl(addr.slot, addr.value_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      this->state.emit("    pushq %rdx");
      this->state.emit("    fldl (%rsp)");
      this->state.emit("    addq $8, %rsp");
      this->state.emit("    fstp (%rcx)");
      return;
    case SlotAddr::Kind::Indirect:
      this->state.emit("    movq %rax, %rdx");
      this->emit_load_ptr_from_slot_impl(addr.slot, ptr_id);
      if (offset != 0) {
        this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
      }
      this->state.emit("    pushq %rdx");
      this->state.emit("    fldl (%rsp)");
      this->state.emit("    addq $8, %rsp");
      this->state.emit("    fstp (%rcx)");
      return;
  }
}

void X86Codegen::emit_f128_load_finish(const Value& dest) {
  if (const auto dest_slot = this->state.get_slot(dest.raw)) {
    this->state.out.emit_instr_rbp("    fstp", dest_slot->raw);
    this->state.out.emit_instr_rbp("    fld", dest_slot->raw);
    this->state.emit("    subq $8, %rsp");
    this->state.emit("    fstpl (%rsp)");
    this->state.emit("    popq %rax");
    this->state.reg_cache.set_acc(dest.raw, false);
    this->state.f128_direct_slots.insert(dest.raw);
    return;
  }

  this->state.emit("    subq $8, %rsp");
  this->state.emit("    fstpl (%rsp)");
  this->state.emit("    popq %rax");
  this->emit_store_result_impl(dest);
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

void X86Codegen::emit_f128_load_to_x87(const Operand& operand) {
  if (const auto words = this->state.get_f128_constant_words(operand.raw)) {
    this->state.emit("    subq $16, %rsp");
    this->state.out.emit_instr_imm_reg("    movabsq", static_cast<std::int64_t>((*words)[0]), "rax");
    this->state.emit("    movq %rax, (%rsp)");
    if ((*words)[1] != 0) {
      this->state.out.emit_instr_imm_reg("    movq", static_cast<std::int64_t>((*words)[1]), "rax");
    } else {
      this->state.emit("    xorl %eax, %eax");
    }
    this->state.emit("    movq %rax, 8(%rsp)");
    this->state.emit("    fld (%rsp)");
    this->state.emit("    addq $16, %rsp");
    this->state.reg_cache.invalidate_all();
    return;
  }

  if (this->state.f128_direct_slots.find(operand.raw) != this->state.f128_direct_slots.end()) {
    if (const auto slot = this->state.get_slot(operand.raw)) {
      this->state.out.emit_instr_rbp("    fld", slot->raw);
      return;
    }
  }

  if (const auto slot = this->state.get_slot(operand.raw)) {
    if (this->state.is_alloca(operand.raw)) {
      this->state.out.emit_instr_rbp("    fld", slot->raw);
      return;
    }

    this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
  } else {
    this->operand_to_rax(operand);
  }

  this->state.emit("    subq $8, %rsp");
  this->state.emit("    movq %rax, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    addq $8, %rsp");
  this->state.reg_cache.invalidate_all();
}

}  // namespace c4c::backend::x86

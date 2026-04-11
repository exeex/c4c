#include "x86_codegen.hpp"

#include <limits>

namespace c4c::backend::x86 {

namespace {

bool is_float_ty(IrType ty) {
  return ty == IrType::F32 || ty == IrType::F64 || ty == IrType::F128;
}

bool is_signed_ty(IrType ty) {
  return ty == IrType::I8 || ty == IrType::I32 || ty == IrType::I64 || ty == IrType::I128;
}

std::string fresh_local_label(std::string_view prefix) {
  static std::uint64_t next_label_id = 0;
  return std::string(".L") + std::string(prefix) + "_" + std::to_string(next_label_id++);
}

std::size_t type_size_bytes(IrType ty) {
  switch (ty) {
    case IrType::Void: return 0;
    case IrType::I8: return 1;
    case IrType::I32:
    case IrType::F32: return 4;
    case IrType::I64:
    case IrType::Ptr:
    case IrType::F64: return 8;
    case IrType::I128:
    case IrType::F128: return 16;
  }
  return 0;
}

bool needs_signed_narrow_fixup(IrType to_ty) {
  return to_ty == IrType::I8 || to_ty == IrType::I32;
}

void emit_signed_narrow_fixup(X86Codegen& codegen, IrType to_ty) {
  switch (to_ty) {
    case IrType::I8:
      codegen.state.emit("    movsbq %al, %rax");
      return;
    case IrType::I32:
      codegen.state.emit("    movslq %eax, %rax");
      return;
    default: return;
  }
}

}  // namespace

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
    this->state.out.emit_instr_rbp("    fldt", *rbp_off);
    return;
  }
  this->state.emit("    fldt (%rcx)");
}

void X86Codegen::emit_f128_fstpt(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  if (const auto rbp_off = this->emit_f128_resolve_addr(addr, ptr_id, offset)) {
    this->state.out.emit_instr_rbp("    fstpt", *rbp_off);
    return;
  }
  this->state.emit("    fstpt (%rcx)");
}

void X86Codegen::emit_f128_store_raw_bytes(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  (void)addr;
  (void)ptr_id;
  (void)offset;
  this->state.emit("    <store-f128-bytes>");
}

void X86Codegen::emit_f128_store_f64_via_x87(const SlotAddr& addr, std::uint32_t ptr_id, std::int64_t offset) {
  switch (addr.kind) {
    case SlotAddr::Kind::Direct:
      this->state.emit("    pushq %rax");
      this->state.emit("    fldl (%rsp)");
      this->state.emit("    addq $8, %rsp");
      this->state.out.emit_instr_rbp("    fstpt", addr.slot.raw + offset);
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
      this->state.emit("    fstpt (%rcx)");
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
      this->state.emit("    fstpt (%rcx)");
      return;
  }
}

void X86Codegen::emit_f128_load_finish(const Value& dest) {
  if (const auto dest_slot = this->state.get_slot(dest.raw)) {
    this->state.out.emit_instr_rbp("    fstpt", dest_slot->raw);
    this->state.out.emit_instr_rbp("    fldt", dest_slot->raw);
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

void X86Codegen::emit_f128_to_int_from_memory(const SlotAddr& addr, IrType to_ty) {
  switch (addr.kind) {
    case SlotAddr::Kind::Direct:
      this->state.out.emit_instr_rbp("    fldt", addr.slot.raw);
      break;
    case SlotAddr::Kind::OverAligned:
      this->emit_alloca_aligned_addr_impl(addr.slot, addr.value_id);
      this->state.emit("    fldt (%rcx)");
      break;
    case SlotAddr::Kind::Indirect:
      this->emit_load_ptr_from_slot_impl(addr.slot, 0);
      this->state.emit("    fldt (%rcx)");
      break;
  }
  this->emit_f128_st0_to_int(to_ty);
}

void X86Codegen::emit_f128_st0_to_int(IrType to_ty) {
  this->state.emit("    subq $8, %rsp");
  this->state.emit("    fisttpq (%rsp)");
  this->state.emit("    movq (%rsp), %rax");
  this->state.emit("    addq $8, %rsp");

  if (needs_signed_narrow_fixup(to_ty)) {
    emit_signed_narrow_fixup(*this, to_ty);
  }
}

void X86Codegen::emit_cast_instrs_x86(IrType from_ty, IrType to_ty) {
  if (to_ty == IrType::F128 && !is_float_ty(from_ty)) {
    this->emit_int_to_f128_cast(from_ty);
    return;
  }
  if (from_ty == IrType::F128 && !is_float_ty(to_ty)) {
    this->emit_f128_to_int_cast(to_ty);
    return;
  }
  if (from_ty == IrType::F128 && to_ty == IrType::F32) {
    this->emit_f128_to_f32_cast();
    return;
  }
  if (from_ty == IrType::F32 && to_ty == IrType::F128) {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    cvtss2sd %xmm0, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
    return;
  }

  this->emit_generic_cast(from_ty, to_ty);
}

void X86Codegen::emit_int_to_f128_cast(IrType from_ty) {
  if (is_signed_ty(from_ty) || from_ty == IrType::Ptr || type_size_bytes(from_ty) < 8) {
    if (type_size_bytes(from_ty) < 8) {
      this->emit_extend_to_rax(from_ty);
    }
    this->emit_fild_to_f64_via_stack();
    return;
  }

  this->state.emit("    <int-to-f128-unsigned64>");
}

void X86Codegen::emit_f128_to_int_cast(IrType to_ty) {
  if (is_signed_ty(to_ty) || to_ty == IrType::Ptr) {
    this->emit_fisttp_from_f64_via_stack();
    if (type_size_bytes(to_ty) < 8 && to_ty != IrType::Ptr) {
      this->emit_sign_extend_to_rax(to_ty);
    }
    return;
  }

  this->emit_f128_to_u64_cast();
  if (type_size_bytes(to_ty) < 8) {
    this->emit_zero_extend_to_rax(to_ty);
  }
}

void X86Codegen::emit_f128_to_u64_cast() {
  const auto big_label = fresh_local_label("ld2u_big");
  const auto done_label = fresh_local_label("ld2u_done");
  this->state.emit("    subq $8, %rsp");
  this->state.emit("    movq %rax, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.out.emit_instr_imm_reg("    movabsq", 4890909195324358656LL, "rcx");
  this->state.emit("    movq %rcx, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    fcomip %st(1), %st");
  this->state.emit("    jbe " + big_label);
  this->state.emit("    fisttpq (%rsp)");
  this->state.emit("    movq (%rsp), %rax");
  this->state.emit("    addq $8, %rsp");
  this->state.emit("    jmp " + done_label);
  this->state.emit(big_label + ":");
  this->state.out.emit_instr_imm_reg("    movabsq", 4890909195324358656LL, "rcx");
  this->state.emit("    movq %rcx, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    fsubrp %st, %st(1)");
  this->state.emit("    fisttpq (%rsp)");
  this->state.emit("    movq (%rsp), %rax");
  this->state.emit("    addq $8, %rsp");
  this->state.out.emit_instr_imm_reg("    movabsq", std::numeric_limits<std::int64_t>::min(), "rcx");
  this->state.emit("    addq %rcx, %rax");
  this->state.emit(done_label + ":");
}

void X86Codegen::emit_f128_to_f32_cast() {
  this->state.emit("    subq $8, %rsp");
  this->state.emit("    movq %rax, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    fstps (%rsp)");
  this->state.emit("    movl (%rsp), %eax");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_fild_to_f64_via_stack() {
  this->state.emit("    subq $8, %rsp");
  this->state.emit("    movq %rax, (%rsp)");
  this->state.emit("    fildq (%rsp)");
  this->state.emit("    fstpl (%rsp)");
  this->state.emit("    movq (%rsp), %rax");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_fisttp_from_f64_via_stack() {
  this->state.emit("    subq $8, %rsp");
  this->state.emit("    movq %rax, (%rsp)");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    fisttpq (%rsp)");
  this->state.emit("    movq (%rsp), %rax");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_extend_to_rax(IrType ty) {
  this->emit_sign_extend_to_rax(ty);
}

void X86Codegen::emit_sign_extend_to_rax(IrType ty) {
  switch (ty) {
    case IrType::I8:
      this->state.emit("    movsbq %al, %rax");
      return;
    case IrType::I32:
      this->state.emit("    movslq %eax, %rax");
      return;
    default: return;
  }
}

void X86Codegen::emit_zero_extend_to_rax(IrType ty) {
  switch (ty) {
    case IrType::I8:
      this->state.emit("    movzbq %al, %rax");
      return;
    case IrType::I32:
      this->state.emit("    movl %eax, %eax");
      return;
    default: return;
  }
}

void X86Codegen::emit_generic_cast(IrType from_ty, IrType to_ty) {
  if (from_ty == to_ty) {
    return;
  }

  if (from_ty == IrType::F32 && to_ty == IrType::F64) {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    cvtss2sd %xmm0, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
    return;
  }

  if (from_ty == IrType::F64 && to_ty == IrType::F32) {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    cvtsd2ss %xmm0, %xmm0");
    this->state.emit("    movd %xmm0, %eax");
    return;
  }

  if (!is_float_ty(from_ty) && (to_ty == IrType::F32 || to_ty == IrType::F64)) {
    if (from_ty == IrType::I8 || from_ty == IrType::I32) {
      this->emit_sign_extend_to_rax(from_ty);
    }
    this->emit_int_to_float_conv(to_ty == IrType::F64);
    return;
  }

  if ((from_ty == IrType::F32 || from_ty == IrType::F64) && !is_float_ty(to_ty)) {
    if (from_ty == IrType::F64) {
      this->state.emit("    movq %rax, %xmm0");
      this->state.emit("    cvttsd2siq %xmm0, %rax");
    } else {
      this->state.emit("    movd %eax, %xmm0");
      this->state.emit("    cvttss2siq %xmm0, %rax");
    }

    if (to_ty != IrType::I64 && to_ty != IrType::Ptr) {
      this->emit_sign_extend_to_rax(to_ty);
    }
    return;
  }

  if (!is_float_ty(from_ty) && !is_float_ty(to_ty) && to_ty == IrType::I64) {
    this->emit_sign_extend_to_rax(from_ty);
  }
}

void X86Codegen::emit_float_to_unsigned(bool from_f64, bool to_u64, IrType to_ty) {
  if (from_f64) {
    this->state.emit("    movq %rax, %xmm0");
    if (to_u64) {
      const auto big_label = fresh_local_label("f2u_big");
      const auto done_label = fresh_local_label("f2u_done");
      this->state.out.emit_instr_imm_reg("    movabsq", 4890909195324358656LL, "rcx");
      this->state.emit("    movq %rcx, %xmm1");
      this->state.emit("    ucomisd %xmm1, %xmm0");
      this->state.emit("    jae " + big_label);
      this->state.emit("    cvttsd2siq %xmm0, %rax");
      this->state.emit("    jmp " + done_label);
      this->state.emit(big_label + ":");
      this->state.emit("    subsd %xmm1, %xmm0");
      this->state.emit("    cvttsd2siq %xmm0, %rax");
      this->state.out.emit_instr_imm_reg("    movabsq", std::numeric_limits<std::int64_t>::min(), "rcx");
      this->state.emit("    addq %rcx, %rax");
      this->state.emit(done_label + ":");
    } else {
      this->state.emit("    cvttsd2siq %xmm0, %rax");
    }
  } else {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    cvttss2siq %xmm0, %rax");
  }

  if (!to_u64) {
    this->emit_zero_extend_to_rax(to_ty);
  }
}

void X86Codegen::emit_int_to_float_conv(bool to_f64) {
  if (to_f64) {
    this->state.emit("    cvtsi2sdq %rax, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
    return;
  }

  this->state.emit("    cvtsi2ssq %rax, %xmm0");
  this->state.emit("    movd %xmm0, %eax");
}

void X86Codegen::emit_u64_to_float(bool to_f64) {
  const auto big_label = fresh_local_label("u2f_big");
  const auto done_label = fresh_local_label("u2f_done");
  this->state.emit("    testq %rax, %rax");
  this->state.emit("    js " + big_label);
  if (to_f64) {
    this->state.emit("    cvtsi2sdq %rax, %xmm0");
  } else {
    this->state.emit("    cvtsi2ssq %rax, %xmm0");
  }
  this->state.emit("    jmp " + done_label);
  this->state.emit(big_label + ":");
  this->state.emit("    movq %rax, %rcx");
  this->state.emit("    shrq $1, %rax");
  this->state.emit("    andq $1, %rcx");
  this->state.emit("    orq %rcx, %rax");
  if (to_f64) {
    this->state.emit("    cvtsi2sdq %rax, %xmm0");
    this->state.emit("    addsd %xmm0, %xmm0");
  } else {
    this->state.emit("    cvtsi2ssq %rax, %xmm0");
    this->state.emit("    addss %xmm0, %xmm0");
  }
  this->state.emit(done_label + ":");
  if (to_f64) {
    this->state.emit("    movq %xmm0, %rax");
  } else {
    this->state.emit("    movd %xmm0, %eax");
  }
}
void X86Codegen::emit_f128_load_to_x87(const Operand& operand) {
  if (this->state.f128_direct_slots.find(operand.raw) != this->state.f128_direct_slots.end()) {
    if (const auto slot = this->state.get_slot(operand.raw)) {
      this->state.out.emit_instr_rbp("    fldt", slot->raw);
      return;
    }
  }

  if (const auto slot = this->state.get_slot(operand.raw)) {
    if (this->state.is_alloca(operand.raw)) {
      this->state.out.emit_instr_rbp("    fldt", slot->raw);
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

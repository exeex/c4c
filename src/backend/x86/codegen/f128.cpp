#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

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
  this->state.emit("    <f128-to-int-from-memory>");
}

void X86Codegen::emit_f128_st0_to_int(IrType to_ty) {
  this->state.emit("    <f128-st0-to-int>");
}

void X86Codegen::emit_cast_instrs_x86(IrType from_ty, IrType to_ty) {
  this->state.emit("    <x86-cast-helper>");
}

void X86Codegen::emit_int_to_f128_cast(IrType from_ty) { this->state.emit("    <int-to-f128-cast>"); }
void X86Codegen::emit_f128_to_int_cast(IrType to_ty) { this->state.emit("    <f128-to-int-cast>"); }
void X86Codegen::emit_f128_to_u64_cast() { this->state.emit("    <f128-to-u64-cast>"); }
void X86Codegen::emit_f128_to_f32_cast() { this->state.emit("    <f128-to-f32-cast>"); }
void X86Codegen::emit_fild_to_f64_via_stack() { this->state.emit("    <fild-to-f64>"); }
void X86Codegen::emit_fisttp_from_f64_via_stack() { this->state.emit("    <fisttp-from-f64>"); }
void X86Codegen::emit_extend_to_rax(IrType ty) { this->state.emit("    <extend-to-rax>"); }
void X86Codegen::emit_sign_extend_to_rax(IrType ty) { this->state.emit("    <sign-extend-to-rax>"); }
void X86Codegen::emit_zero_extend_to_rax(IrType ty) { this->state.emit("    <zero-extend-to-rax>"); }
void X86Codegen::emit_generic_cast(IrType from_ty, IrType to_ty) { this->state.emit("    <generic-cast>"); }
void X86Codegen::emit_float_to_unsigned(bool from_f64, bool to_u64, IrType to_ty) { this->state.emit("    <float-to-unsigned>"); }
void X86Codegen::emit_int_to_float_conv(bool to_f64) { this->state.emit("    <int-to-float-conv>"); }
void X86Codegen::emit_u64_to_float(bool to_f64) { this->state.emit("    <u64-to-float>"); }
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

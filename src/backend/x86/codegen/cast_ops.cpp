#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_cast_instrs_impl(IrType from_ty, IrType to_ty) {
  // The ref backend routes every cast through a target-specific helper.
  this->emit_cast_instrs_x86(from_ty, to_ty);
}

void X86Codegen::emit_cast_impl(const Value& dest,
                                const Operand& src,
                                IrType from_ty,
                                IrType to_ty) {
  // Casts to long double are handled through x87 so the destination slot
  // receives a full 80-bit value.
  if (to_ty == IrType::F128 && from_ty != IrType::F128 && !is_i128_type(from_ty)) {
    if (auto dest_slot = this->state.get_slot(dest.0)) {
      if (from_ty == IrType::F64) {
        this->operand_to_rax(src);
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fldl (%rsp)");
        this->state.emit("    addq $8, %rsp");
      } else if (from_ty == IrType::F32) {
        this->operand_to_rax(src);
        this->state.emit("    subq $4, %rsp");
        this->state.emit("    movl %eax, (%rsp)");
        this->state.emit("    flds (%rsp)");
        this->state.emit("    addq $4, %rsp");
      } else if (from_ty.is_signed() || (!from_ty.is_float() && !from_ty.is_unsigned())) {
        this->operand_to_rax(src);
        if (from_ty.size() < 8) {
          this->emit_cast_instrs_x86(from_ty, IrType::I64);
        }
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
      } else {
        // Unsigned integer to long double uses a split path in the ref code.
        this->operand_to_rax(src);
        if (from_ty.size() < 8) {
          this->emit_cast_instrs_x86(from_ty, IrType::I64);
        }
        const auto big_label = this->state.fresh_label("u2f128_big");
        const auto done_label = this->state.fresh_label("u2f128_done");
        this->state.emit("    testq %rax, %rax");
        this->state.out.emit_jcc_label("    js", &big_label);
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
        this->state.out.emit_jmp_label(&done_label);
        this->state.out.emit_named_label(&big_label);
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
        this->state.emit("    subq $16, %rsp");
        this->state.out.emit_instr_imm_reg("    movabsq", -9223372036854775808LL, "rax");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.out.emit_instr_imm_reg("    movq", 0x403F, "rax");
        this->state.emit("    movq %rax, 8(%rsp)");
        this->state.emit("    fldt (%rsp)");
        this->state.emit("    addq $16, %rsp");
        this->state.emit("    faddp %st, %st(1)");
        this->state.out.emit_named_label(&done_label);
      }

      this->state.out.emit_instr_rbp("    fstpt", dest_slot->0);
      this->state.out.emit_instr_rbp("    fldt", dest_slot->0);
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.set_acc(dest.0, false);
      this->state.f128_direct_slots.insert(dest.0);
      return;
    }
  }

  if (from_ty == IrType::F128 && (to_ty == IrType::F64 || to_ty == IrType::F32)) {
    this->emit_f128_load_to_x87(&src);
    if (to_ty == IrType::F64) {
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    movq (%rsp), %rax");
      this->state.emit("    addq $8, %rsp");
    } else {
      this->state.emit("    subq $4, %rsp");
      this->state.emit("    fstps (%rsp)");
      this->state.emit("    movl (%rsp), %eax");
      this->state.emit("    addq $4, %rsp");
    }
    this->state.reg_cache.invalidate_acc();
    this->store_rax_to(dest);
    return;
  }

  if (from_ty == IrType::F128 && !to_ty.is_float() && !is_i128_type(to_ty)) {
    if (auto v = std::get_if<Value>(&src)) {
      if (this->state.f128_direct_slots.contains(&v->0)) {
        if (auto slot = this->state.get_slot(v->0)) {
          this->emit_f128_to_int_from_memory(&SlotAddr::Direct(*slot), to_ty);
          this->store_rax_to(dest);
          return;
        }
      }
      if (auto source = this->state.get_f128_source(v->0)) {
        if (auto addr = this->state.resolve_slot_addr(std::get<0>(*source))) {
          this->emit_f128_to_int_from_memory(&addr, to_ty);
          this->store_rax_to(dest);
          return;
        }
      }
    }
  }

  // Everything else falls back to the target-neutral default lowering.
  emit_cast_default(*this, dest, src, from_ty, to_ty);
}

}  // namespace c4c::backend::x86

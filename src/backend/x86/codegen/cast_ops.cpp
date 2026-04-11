#include "x86_codegen.hpp"

#include <limits>

namespace c4c::backend::x86 {

namespace {

bool is_float_ty(IrType ty) {
  return ty == IrType::F32 || ty == IrType::F64 || ty == IrType::F128;
}

bool is_unsigned_ty(IrType ty) {
  return ty == IrType::I8 || ty == IrType::I32 || ty == IrType::I64;
}

bool is_i128_ty(IrType ty) {
  return ty == IrType::I128;
}

bool is_signed_or_ptr_ty(IrType ty) {
  return ty == IrType::Ptr || ty == IrType::I8 || ty == IrType::I32 || ty == IrType::I64;
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

std::string fresh_local_label(std::string_view prefix) {
  static std::uint64_t next_label_id = 0;
  return std::string(".L") + std::string(prefix) + "_" + std::to_string(next_label_id++);
}

}  // namespace

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
  if (to_ty == IrType::F128 && from_ty != IrType::F128 && !is_i128_ty(from_ty)) {
    if (const auto dest_slot = this->state.get_slot(dest.raw)) {
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
      } else if (is_signed_or_ptr_ty(from_ty) || (!is_float_ty(from_ty) && !is_unsigned_ty(from_ty))) {
        this->operand_to_rax(src);
        if (type_size_bytes(from_ty) < 8) {
          this->emit_cast_instrs_x86(from_ty, IrType::I64);
        }
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
      } else {
        // Unsigned integer to long double uses a split path in the ref code.
        this->operand_to_rax(src);
        if (type_size_bytes(from_ty) < 8) {
          this->emit_cast_instrs_x86(from_ty, IrType::I64);
        }
        const auto big_label = fresh_local_label("u2f128_big");
        const auto done_label = fresh_local_label("u2f128_done");
        this->state.emit("    testq %rax, %rax");
        this->state.emit("    js " + big_label);
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
        this->state.emit("    jmp " + done_label);
        this->state.emit(big_label + ":");
        this->state.emit("    subq $8, %rsp");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.emit("    fildq (%rsp)");
        this->state.emit("    addq $8, %rsp");
        this->state.emit("    subq $16, %rsp");
        this->state.out.emit_instr_imm_reg("    movabsq", std::numeric_limits<std::int64_t>::min(), "rax");
        this->state.emit("    movq %rax, (%rsp)");
        this->state.out.emit_instr_imm_reg("    movq", 0x403F, "rax");
        this->state.emit("    movq %rax, 8(%rsp)");
        this->state.emit("    fldt (%rsp)");
        this->state.emit("    addq $16, %rsp");
        this->state.emit("    faddp %st, %st(1)");
        this->state.emit(done_label + ":");
      }

      this->state.out.emit_instr_rbp("    fstpt", dest_slot->raw);
      this->state.out.emit_instr_rbp("    fldt", dest_slot->raw);
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.set_acc(dest.raw, false);
      this->state.f128_direct_slots.insert(dest.raw);
      return;
    }
  }

  if (from_ty == IrType::F128 && (to_ty == IrType::F64 || to_ty == IrType::F32)) {
    this->emit_f128_load_to_x87(src);
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

  if (from_ty == IrType::F128 && !is_float_ty(to_ty) && !is_i128_ty(to_ty)) {
    if (this->state.f128_direct_slots.find(src.raw) != this->state.f128_direct_slots.end()) {
      if (const auto slot = this->state.get_slot(src.raw)) {
        this->emit_f128_to_int_from_memory(SlotAddr::Direct(*slot), to_ty);
        this->store_rax_to(dest);
        return;
      }
    }
    if (const auto source = this->state.get_f128_source(src.raw)) {
      if (const auto addr = this->state.resolve_slot_addr(*source)) {
        this->emit_f128_to_int_from_memory(*addr, to_ty);
        this->store_rax_to(dest);
        return;
      }
    }
    this->emit_f128_load_to_x87(src);
    this->emit_f128_st0_to_int(to_ty);
    this->store_rax_to(dest);
    return;
  }

  // Everything else falls back to the target-neutral default lowering.
  this->operand_to_rax(src);
  this->emit_cast_instrs_x86(from_ty, to_ty);
  this->store_rax_to(dest);
}

}  // namespace c4c::backend::x86

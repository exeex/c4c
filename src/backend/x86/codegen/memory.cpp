#include "x86_codegen.hpp"
#include "../../regalloc.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

const char* segment_prefix(AddressSpace seg) {
  switch (seg) {
    case AddressSpace::SegGs: return "%gs:";
    case AddressSpace::SegFs: return "%fs:";
    case AddressSpace::Default:
      throw std::logic_error("segment-prefixed op called with default address space");
  }
  throw std::logic_error("unreachable x86 address space");
}

void emit_store_default(X86Codegen& codegen, const Operand& val, const Value& ptr, IrType ty) {
  codegen.emit_store_with_const_offset_impl(val, ptr, 0, ty);
}

void emit_load_default(X86Codegen& codegen, const Value& dest, const Value& ptr, IrType ty) {
  codegen.emit_load_with_const_offset_impl(dest, ptr, 0, ty);
}

}  // namespace

void X86Codegen::emit_store_impl(const Operand& val, const Value& ptr, IrType ty) {
  if (ty == IrType::F128) {
    if (const auto addr = this->state.resolve_slot_addr(ptr.raw)) {
      if (const auto words = this->state.get_f128_constant_words(val.raw)) {
        this->emit_f128_store_raw_bytes(*addr, ptr.raw, 0, (*words)[0], (*words)[1]);
        return;
      }
      if (this->state.f128_direct_slots.find(val.raw) != this->state.f128_direct_slots.end()) {
        if (const auto src_slot = this->state.get_slot(val.raw)) {
          this->state.out.emit_instr_rbp("    fldt", src_slot->raw);
          this->emit_f128_fstpt(*addr, ptr.raw, 0);
          return;
        }
      }
      this->operand_to_rax(val);
      this->emit_f128_store_f64_via_x87(*addr, ptr.raw, 0);
    }
    return;
  }
  emit_store_default(*this, val, ptr, ty);
}

void X86Codegen::emit_load_impl(const Value& dest, const Value& ptr, IrType ty) {
  if (ty == IrType::F128) {
    if (const auto addr = this->state.resolve_slot_addr(ptr.raw)) {
      this->emit_f128_fldt(*addr, ptr.raw, 0);
      this->emit_f128_load_finish(dest);
      this->state.track_f128_load(dest.raw, ptr.raw, 0);
    }
    return;
  }
  emit_load_default(*this, dest, ptr, ty);
}

void X86Codegen::emit_store_with_const_offset_impl(const Operand& val,
                                                   const Value& base,
                                                   std::int64_t offset,
                                                   IrType ty) {
  if (ty == IrType::F128) {
    if (const auto addr = this->state.resolve_slot_addr(base.raw)) {
      if (const auto words = this->state.get_f128_constant_words(val.raw)) {
        this->emit_f128_store_raw_bytes(*addr, base.raw, offset, (*words)[0], (*words)[1]);
        return;
      }
      if (this->state.f128_direct_slots.find(val.raw) != this->state.f128_direct_slots.end()) {
        if (const auto src_slot = this->state.get_slot(val.raw)) {
          this->state.out.emit_instr_rbp("    fldt", src_slot->raw);
          this->emit_f128_fstpt(*addr, base.raw, offset);
          return;
        }
      }
      this->operand_to_rax(val);
      this->emit_f128_store_f64_via_x87(*addr, base.raw, offset);
    }
    return;
  }

  this->operand_to_rax(val);
  if (const auto addr = this->state.resolve_slot_addr(base.raw)) {
    const char* store_instr = this->mov_store_for_type(ty);
    switch (addr->kind) {
      case SlotAddr::Kind::OverAligned:
        this->emit_save_acc_impl();
        this->emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
        this->emit_add_offset_to_addr_reg_impl(offset);
        this->emit_typed_store_indirect_impl(store_instr, ty);
        break;
      case SlotAddr::Kind::Direct:
        this->emit_typed_store_to_slot_impl(store_instr, ty, StackSlot{addr->slot.raw + offset});
        break;
      case SlotAddr::Kind::Indirect:
        if (const auto reg = this->state.assigned_reg_index(base.raw)) {
          const char* store_reg = this->reg_for_type("rax", ty);
          const char* reg_name = phys_reg_name(c4c::backend::PhysReg{*reg});
          if (offset != 0) {
            this->state.emit(std::string("    ") + store_instr + " %" + store_reg + ", " +
                             std::to_string(offset) + "(%" + reg_name + ")");
          } else {
            this->state.emit(std::string("    ") + store_instr + " %" + store_reg + ", (%" +
                             reg_name + ")");
          }
        } else {
          this->emit_save_acc_impl();
          this->emit_load_ptr_from_slot_impl(addr->slot, base.raw);
          if (offset != 0) {
            this->emit_add_offset_to_addr_reg_impl(offset);
          }
          this->emit_typed_store_indirect_impl(store_instr, ty);
        }
        break;
    }
  }
}

void X86Codegen::emit_load_with_const_offset_impl(const Value& dest,
                                                  const Value& base,
                                                  std::int64_t offset,
                                                  IrType ty) {
  if (ty == IrType::F128) {
    if (const auto addr = this->state.resolve_slot_addr(base.raw)) {
      this->emit_f128_fldt(*addr, base.raw, offset);
      this->emit_f128_load_finish(dest);
    }
    return;
  }

  if (const auto addr = this->state.resolve_slot_addr(base.raw)) {
    const char* load_instr = this->mov_load_for_type(ty);
    switch (addr->kind) {
      case SlotAddr::Kind::OverAligned:
        this->emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
        this->emit_add_offset_to_addr_reg_impl(offset);
        this->emit_typed_load_indirect_impl(load_instr);
        break;
      case SlotAddr::Kind::Direct:
        this->emit_typed_load_from_slot_impl(load_instr, StackSlot{addr->slot.raw + offset});
        break;
      case SlotAddr::Kind::Indirect:
        if (const auto reg = this->state.assigned_reg_index(base.raw)) {
          const char* reg_name = phys_reg_name(c4c::backend::PhysReg{*reg});
          const char* dest_reg = std::string(load_instr) == "movl" ? "%eax" : "%rax";
          if (offset != 0) {
            this->state.emit(std::string("    ") + load_instr + " " + std::to_string(offset) +
                             "(%" + reg_name + "), " + dest_reg);
          } else {
            this->state.emit(std::string("    ") + load_instr + " (%" + reg_name + "), " + dest_reg);
          }
        } else {
          this->emit_load_ptr_from_slot_impl(addr->slot, base.raw);
          if (offset != 0) {
            this->emit_add_offset_to_addr_reg_impl(offset);
          }
          this->emit_typed_load_indirect_impl(load_instr);
        }
        break;
    }
    this->store_rax_to(dest);
  }
}

void X86Codegen::emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot) {
  this->state.emit(std::string("    ") + instr + " %" + this->reg_for_type("rax", ty) +
                   ", " + std::to_string(slot.raw) + "(%rbp)");
}

void X86Codegen::emit_typed_load_from_slot_impl(const char* instr, StackSlot slot) {
  const char* dest_reg = std::string(instr) == "movl" ? "%eax" : "%rax";
  this->state.emit(std::string("    ") + instr + " " + std::to_string(slot.raw) +
                   "(%rbp), " + dest_reg);
}

void X86Codegen::emit_save_acc_impl() { this->state.emit("    movq %rax, %rdx"); }

void X86Codegen::emit_load_ptr_from_slot_impl(StackSlot slot, std::uint32_t val_id) {
  if (const auto reg = this->state.assigned_reg_index(val_id)) {
    this->state.emit(std::string("    movq %") + phys_reg_name(c4c::backend::PhysReg{*reg}) + ", %rcx");
  } else {
    this->state.out.emit_instr_rbp_reg("    movq", slot.raw, "rcx");
  }
}

void X86Codegen::emit_typed_store_indirect_impl(const char* instr, IrType ty) {
  this->state.emit(std::string("    ") + instr + " %" + this->reg_for_type("rdx", ty) + ", (%rcx)");
}

void X86Codegen::emit_typed_load_indirect_impl(const char* instr) {
  const char* dest_reg = std::string(instr) == "movl" ? "%eax" : "%rax";
  this->state.emit(std::string("    ") + instr + " (%rcx), " + dest_reg);
}

void X86Codegen::emit_add_offset_to_addr_reg_impl(std::int64_t offset) {
  if (offset != 0) {
    this->state.out.emit_instr_imm_reg("    addq", offset, "rcx");
  }
}

void X86Codegen::emit_alloca_addr_to(const char* reg, std::uint32_t val_id, std::int64_t offset) {
  if (const auto align = this->state.alloca_over_align(val_id)) {
    this->state.out.emit_instr_rbp_reg("    leaq", offset, reg);
    this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(*align - 1), reg);
    this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(*align), reg);
  } else {
    this->state.out.emit_instr_rbp_reg("    leaq", offset, reg);
  }
}

void X86Codegen::emit_slot_addr_to_secondary_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    this->emit_alloca_addr_to("rcx", val_id, slot.raw);
  } else if (const auto reg = this->state.assigned_reg_index(val_id)) {
    this->state.emit(std::string("    movq %") + phys_reg_name(c4c::backend::PhysReg{*reg}) + ", %rcx");
  } else {
    this->state.out.emit_instr_rbp_reg("    movq", slot.raw, "rcx");
  }
}

void X86Codegen::emit_add_secondary_to_acc_impl() {
  this->state.emit("    addq %rcx, %rax");
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_gep_direct_const_impl(StackSlot slot, std::int64_t offset) {
  this->state.out.emit_instr_rbp_reg("    leaq", slot.raw + offset, "rax");
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_gep_indirect_const_impl(StackSlot slot, std::int64_t offset, std::uint32_t val_id) {
  if (const auto reg = this->state.assigned_reg_index(val_id)) {
    this->state.emit(std::string("    movq %") + phys_reg_name(c4c::backend::PhysReg{*reg}) + ", %rax");
  } else {
    this->state.out.emit_instr_rbp_reg("    movq", slot.raw, "rax");
  }
  if (offset != 0) {
    this->state.out.emit_instr_mem_reg("    leaq", offset, "rax", "rax");
  }
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_gep_add_const_to_acc_impl(std::int64_t offset) {
  if (offset != 0) {
    this->state.out.emit_instr_imm_reg("    addq", offset, "rax");
  }
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_add_imm_to_acc_impl(std::int64_t imm) {
  this->state.out.emit_instr_imm_reg("    addq", imm, "rax");
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_round_up_acc_to_16_impl() {
  this->state.emit("    addq $15, %rax");
  this->state.emit("    andq $-16, %rax");
  this->state.reg_cache.invalidate_all();
}
void X86Codegen::emit_sub_sp_by_acc_impl() { this->state.emit("    subq %rax, %rsp"); }
void X86Codegen::emit_mov_sp_to_acc_impl() {
  this->state.emit("    movq %rsp, %rax");
  this->state.reg_cache.invalidate_all();
}
void X86Codegen::emit_mov_acc_to_sp_impl() {
  this->state.emit("    movq %rax, %rsp");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_align_acc_impl(std::size_t align) {
  this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(align - 1), "rax");
  this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(align), "rax");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_memcpy_load_dest_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    this->emit_alloca_addr_to("rdi", val_id, slot.raw);
  } else if (const auto reg = this->state.assigned_reg_index(val_id)) {
    this->state.emit(std::string("    movq %") + phys_reg_name(c4c::backend::PhysReg{*reg}) + ", %rdi");
  } else {
    this->state.out.emit_instr_rbp_reg("    movq", slot.raw, "rdi");
  }
}

void X86Codegen::emit_memcpy_load_src_addr_impl(StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    this->emit_alloca_addr_to("rsi", val_id, slot.raw);
  } else if (const auto reg = this->state.assigned_reg_index(val_id)) {
    this->state.emit(std::string("    movq %") + phys_reg_name(c4c::backend::PhysReg{*reg}) + ", %rsi");
  } else {
    this->state.out.emit_instr_rbp_reg("    movq", slot.raw, "rsi");
  }
}

void X86Codegen::emit_alloca_aligned_addr_impl(StackSlot slot, std::uint32_t val_id) {
  const auto align = this->state.alloca_over_align(val_id).value();
  this->state.out.emit_instr_rbp_reg("    leaq", slot.raw, "rcx");
  this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(align - 1), "rcx");
  this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(align), "rcx");
}

void X86Codegen::emit_alloca_aligned_addr_to_acc_impl(StackSlot slot, std::uint32_t val_id) {
  const auto align = this->state.alloca_over_align(val_id).value();
  this->state.out.emit_instr_rbp_reg("    leaq", slot.raw, "rax");
  this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(align - 1), "rax");
  this->state.out.emit_instr_imm_reg("    andq", -static_cast<std::int64_t>(align), "rax");
  this->state.reg_cache.invalidate_acc();
}

void X86Codegen::emit_acc_to_secondary_impl() { this->state.emit("    movq %rax, %rcx"); }
void X86Codegen::emit_memcpy_store_dest_from_acc_impl() { this->state.emit("    movq %rcx, %rdi"); }
void X86Codegen::emit_memcpy_store_src_from_acc_impl() { this->state.emit("    movq %rcx, %rsi"); }

void X86Codegen::emit_memcpy_impl_impl(std::size_t size) {
  this->state.out.emit_instr_imm_reg("    movq", static_cast<std::int64_t>(size), "rcx");
  this->state.emit("    rep movsb");
}

void X86Codegen::emit_seg_load_impl(const Value& dest, const Value& ptr, IrType ty, AddressSpace seg) {
  const char* prefix = segment_prefix(seg);
  this->operand_to_rax(Operand{ptr.raw});
  this->state.emit("    movq %rax, %rcx");
  this->state.emit(std::string("    ") + this->mov_load_for_type(ty) + " " + prefix + "(%rcx), " +
                   std::string("%") + this->reg_for_type("rax", ty));
  this->store_rax_to(dest);
}

void X86Codegen::emit_seg_load_symbol_impl(const Value& dest, const std::string& sym, IrType ty, AddressSpace seg) {
  const char* prefix = segment_prefix(seg);
  this->state.emit(std::string("    ") + this->mov_load_for_type(ty) + " " + prefix + sym + "(%rip), " +
                   std::string("%") + this->reg_for_type("rax", ty));
  this->store_rax_to(dest);
}

void X86Codegen::emit_seg_store_impl(const Operand& val, const Value& ptr, IrType ty, AddressSpace seg) {
  const char* prefix = segment_prefix(seg);
  this->operand_to_rax(val);
  this->state.emit("    movq %rax, %rdx");
  this->operand_to_rax(Operand{ptr.raw});
  this->state.emit("    movq %rax, %rcx");
  this->state.emit(std::string("    ") + this->mov_store_for_type(ty) + " %" +
                   this->reg_for_type("rdx", ty) + ", " + prefix + "(%rcx)");
}

void X86Codegen::emit_seg_store_symbol_impl(const Operand& val, const std::string& sym, IrType ty, AddressSpace seg) {
  const char* prefix = segment_prefix(seg);
  this->operand_to_rax(val);
  this->state.emit(std::string("    ") + this->mov_store_for_type(ty) + " %" +
                   this->reg_for_type("rax", ty) + ", " + prefix + sym + "(%rip)");
}

}  // namespace c4c::backend::x86

// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/memory.rs
// Structural mirror of the ref Rust source. This slice makes the first bounded
// memory-helper seam real without widening into the broader translated memory
// owner body.

#include "riscv_codegen.hpp"
#include "../../f128_softfloat.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>

namespace c4c::backend::riscv::codegen {

namespace {

std::uint64_t next_memcpy_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

constexpr bool fits_imm12(std::int64_t value) {
  return value >= -2048 && value <= 2047;
}

void emit_add_s0_to_reg(RiscvCodegenState& state, const char* reg, std::int64_t offset) {
  if (fits_imm12(offset)) {
    state.emit("    addi " + std::string(reg) + ", s0, " + std::to_string(offset));
    return;
  }

  state.emit("    li t6, " + std::to_string(offset));
  state.emit("    add " + std::string(reg) + ", s0, t6");
}

void emit_add_imm_to_reg(RiscvCodegenState& state, const char* reg, std::int64_t imm) {
  if (fits_imm12(imm)) {
    state.emit("    addi " + std::string(reg) + ", " + std::string(reg) + ", " + std::to_string(imm));
    return;
  }

  state.emit("    li t6, " + std::to_string(imm));
  state.emit("    add " + std::string(reg) + ", " + std::string(reg) + ", t6");
}

void emit_and_reg_with_neg_align(RiscvCodegenState& state, const char* reg, std::size_t align) {
  const auto neg_align = -static_cast<std::int64_t>(align);
  if (fits_imm12(neg_align)) {
    state.emit("    andi " + std::string(reg) + ", " + std::string(reg) + ", " + std::to_string(neg_align));
    return;
  }

  state.emit("    li t6, " + std::to_string(neg_align));
  state.emit("    and " + std::string(reg) + ", " + std::string(reg) + ", t6");
}

}  // namespace

void RiscvCodegen::emit_store_impl(const Operand& val, const Value& ptr, IrType ty) {
  if (ty == IrType::F128) {
    c4c::backend::emit_riscv_f128_store(*this, val, ptr);
    return;
  }

  emit_store_with_const_offset_impl(val, ptr, 0, ty);
}

void RiscvCodegen::emit_load_impl(const Value& dest, const Value& ptr, IrType ty) {
  if (ty == IrType::F128) {
    c4c::backend::emit_riscv_f128_load(*this, dest, ptr);
    return;
  }

  emit_load_with_const_offset_impl(dest, ptr, 0, ty);
}

void RiscvCodegen::emit_store_with_const_offset_impl(
    const Operand& val, const Value& base, std::int64_t offset, IrType ty) {
  if (ty == IrType::F128) {
    c4c::backend::emit_riscv_f128_store_with_offset(*this, val, base, offset);
    return;
  }

  operand_to_t0(val);
  const auto addr = state.resolve_slot_addr(base.raw);
  if (!addr.has_value()) {
    return;
  }

  const char* store_instr = store_for_type(ty);
  switch (addr->kind) {
    case SlotAddr::Kind::OverAligned:
      state.emit("    mv t3, t0");
      emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
      emit_add_offset_to_addr_reg_impl(offset);
      state.emit("    " + std::string(store_instr) + " t3, 0(t5)");
      return;
    case SlotAddr::Kind::Direct:
      emit_typed_store_to_slot_impl(store_instr, ty, StackSlot{addr->slot.raw + offset});
      return;
    case SlotAddr::Kind::Indirect:
      state.emit("    mv t3, t0");
      emit_load_ptr_from_slot_impl(addr->slot, static_cast<std::uint32_t>(base.raw));
      if (offset != 0) {
        emit_add_offset_to_addr_reg_impl(offset);
      }
      state.emit("    " + std::string(store_instr) + " t3, 0(t5)");
      return;
  }
}

void RiscvCodegen::emit_load_with_const_offset_impl(
    const Value& dest, const Value& base, std::int64_t offset, IrType ty) {
  if (ty == IrType::F128) {
    c4c::backend::emit_riscv_f128_load_with_offset(*this, dest, base, offset);
    return;
  }

  const auto addr = state.resolve_slot_addr(base.raw);
  if (!addr.has_value()) {
    return;
  }

  const char* load_instr = load_for_type(ty);
  switch (addr->kind) {
    case SlotAddr::Kind::OverAligned:
      emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
      emit_add_offset_to_addr_reg_impl(offset);
      state.emit("    " + std::string(load_instr) + " t0, 0(t5)");
      break;
    case SlotAddr::Kind::Direct:
      emit_typed_load_from_slot_impl(load_instr, StackSlot{addr->slot.raw + offset});
      break;
    case SlotAddr::Kind::Indirect:
      emit_load_ptr_from_slot_impl(addr->slot, static_cast<std::uint32_t>(base.raw));
      if (offset != 0) {
        emit_add_offset_to_addr_reg_impl(offset);
      }
      state.emit("    " + std::string(load_instr) + " t0, 0(t5)");
      break;
  }

  store_t0_to(dest);
}

const char* RiscvCodegen::store_for_type(IrType ty) {
  switch (ty) {
    case IrType::I8: return "sb";
    case IrType::I16:
    case IrType::U16: return "sh";
    case IrType::I32:
    case IrType::U32:
    case IrType::F32: return "sw";
    case IrType::Void:
    case IrType::I64:
    case IrType::I128:
    case IrType::U64:
    case IrType::Ptr:
    case IrType::F64:
    case IrType::F128: return "sd";
  }

  return "sd";
}

const char* RiscvCodegen::load_for_type(IrType ty) {
  switch (ty) {
    case IrType::I8: return "lb";
    case IrType::I16: return "lh";
    case IrType::U16: return "lhu";
    case IrType::I32: return "lw";
    case IrType::U32:
    case IrType::F32: return "lwu";
    case IrType::Void:
    case IrType::I64:
    case IrType::I128:
    case IrType::U64:
    case IrType::Ptr:
    case IrType::F64:
    case IrType::F128: return "ld";
  }

  return "ld";
}

void RiscvCodegen::emit_typed_store_to_slot_impl(const char* instr, IrType ty, StackSlot slot) {
  static_cast<void>(ty);
  emit_store_to_s0("t0", slot.raw, instr);
}

void RiscvCodegen::emit_typed_load_from_slot_impl(const char* instr, StackSlot slot) {
  emit_load_from_s0("t0", slot.raw, instr);
}

void RiscvCodegen::emit_add_offset_to_addr_reg_impl(std::int64_t offset) {
  if (fits_imm12(offset)) {
    state.emit("    addi t5, t5, " + std::to_string(offset));
    return;
  }

  state.emit("    li t6, " + std::to_string(offset));
  state.emit("    add t5, t5, t6");
}

void RiscvCodegen::emit_add_imm_to_acc_impl(std::int64_t imm) {
  if (fits_imm12(imm)) {
    state.emit("    addi t0, t0, " + std::to_string(imm));
    return;
  }

  state.emit("    li t1, " + std::to_string(imm));
  state.emit("    add t0, t0, t1");
}

void RiscvCodegen::emit_round_up_acc_to_16_impl() {
  state.emit("    addi t0, t0, 15");
  state.emit("    andi t0, t0, -16");
}

void RiscvCodegen::emit_sub_sp_by_acc_impl() { state.emit("    sub sp, sp, t0"); }

void RiscvCodegen::emit_mov_sp_to_acc_impl() { state.emit("    mv t0, sp"); }

void RiscvCodegen::emit_mov_acc_to_sp_impl() { state.emit("    mv sp, t0"); }

void RiscvCodegen::emit_align_acc_impl(std::size_t align) {
  state.emit("    addi t0, t0, " + std::to_string(static_cast<std::int64_t>(align - 1)));
  state.emit("    andi t0, t0, -" + std::to_string(static_cast<std::int64_t>(align)));
}

void RiscvCodegen::emit_alloca_aligned_addr_impl(StackSlot slot, std::uint32_t val_id) {
  const auto align = state.alloca_over_align(static_cast<int>(val_id));
  if (!align.has_value()) {
    throw std::runtime_error("alloca must have over-alignment for aligned addr emission");
  }

  emit_add_s0_to_reg(state, "t5", slot.raw);
  emit_add_imm_to_reg(state, "t5", static_cast<std::int64_t>(*align - 1));
  emit_and_reg_with_neg_align(state, "t5", *align);
}

void RiscvCodegen::emit_alloca_aligned_addr_to_acc_impl(StackSlot slot, std::uint32_t val_id) {
  const auto align = state.alloca_over_align(static_cast<int>(val_id));
  if (!align.has_value()) {
    throw std::runtime_error("alloca must have over-alignment for aligned addr emission");
  }

  emit_add_s0_to_reg(state, "t0", slot.raw);
  emit_add_imm_to_reg(state, "t0", static_cast<std::int64_t>(*align - 1));
  emit_and_reg_with_neg_align(state, "t0", *align);
}

void RiscvCodegen::emit_load_ptr_from_slot_impl(StackSlot slot, std::uint32_t val_id) {
  if (const auto reg = state.assigned_reg(static_cast<int>(val_id))) {
    state.emit("    mv t5, " + std::string(callee_saved_name(*reg)));
    return;
  }

  emit_load_from_s0("t5", slot.raw, "ld");
}

void RiscvCodegen::emit_slot_addr_to_secondary_impl(
    StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    emit_add_s0_to_reg(state, "t1", slot.raw);
    return;
  }

  if (const auto reg = state.assigned_reg(static_cast<int>(val_id))) {
    state.emit("    mv t1, " + std::string(callee_saved_name(*reg)));
    return;
  }

  emit_load_from_s0("t1", slot.raw, "ld");
}

void RiscvCodegen::emit_gep_direct_const_impl(StackSlot slot, std::int64_t offset) {
  emit_add_s0_to_reg(state, "t0", slot.raw + offset);
}

void RiscvCodegen::emit_gep_indirect_const_impl(
    StackSlot slot, std::int64_t offset, std::uint32_t val_id) {
  if (const auto reg = state.assigned_reg(static_cast<int>(val_id))) {
    state.emit("    mv t0, " + std::string(callee_saved_name(*reg)));
  } else {
    emit_load_from_s0("t0", slot.raw, "ld");
  }

  if (offset != 0) {
    emit_add_imm_to_acc_impl(offset);
  }
}

void RiscvCodegen::emit_memcpy_load_dest_addr_impl(
    StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    emit_add_s0_to_reg(state, "t1", slot.raw);
    return;
  }

  if (const auto reg = state.assigned_reg(static_cast<int>(val_id))) {
    state.emit("    mv t1, " + std::string(callee_saved_name(*reg)));
    return;
  }

  emit_load_from_s0("t1", slot.raw, "ld");
}

void RiscvCodegen::emit_memcpy_load_src_addr_impl(
    StackSlot slot, bool is_alloca, std::uint32_t val_id) {
  if (is_alloca) {
    emit_add_s0_to_reg(state, "t2", slot.raw);
    return;
  }

  if (const auto reg = state.assigned_reg(static_cast<int>(val_id))) {
    state.emit("    mv t2, " + std::string(callee_saved_name(*reg)));
    return;
  }

  emit_load_from_s0("t2", slot.raw, "ld");
}

void RiscvCodegen::emit_memcpy_impl_impl(std::size_t size) {
  const auto label_id = next_memcpy_label_id();
  const auto loop_label = ".Lmemcpy_loop_" + std::to_string(label_id);
  const auto done_label = ".Lmemcpy_done_" + std::to_string(label_id);

  state.emit("    li t3, " + std::to_string(size));
  state.emit(loop_label + ":");
  state.emit("    beqz t3, " + done_label);
  state.emit("    lbu t4, 0(t2)");
  state.emit("    sb t4, 0(t1)");
  state.emit("    addi t1, t1, 1");
  state.emit("    addi t2, t2, 1");
  state.emit("    addi t3, t3, -1");
  state.emit("    j " + loop_label);
  state.emit(done_label + ":");
}

// //! RiscvCodegen: memory operations (load, store, memcpy, GEP, stack).
//
// use crate::ir::reexports::{Operand, Value};
// use crate::common::types::IrType;
// use crate::backend::state::{StackSlot, SlotAddr};
// use super::emit::{RiscvCodegen, callee_saved_name};
//
// impl RiscvCodegen {
//     // ---- Store/Load overrides ----
//
//     pub(super) fn emit_store_impl(&mut self, val: &Operand, ptr: &Value, ty: IrType) {
//         if ty == IrType::F128 {
//             crate::backend::f128_softfloat::f128_emit_store(self, val, ptr);
//             return;
//         }
//         crate::backend::traits::emit_store_default(self, val, ptr, ty);
//     }
//
//     pub(super) fn emit_load_impl(&mut self, dest: &Value, ptr: &Value, ty: IrType) {
//         if ty == IrType::F128 {
//             crate::backend::f128_softfloat::f128_emit_load(self, dest, ptr);
//             return;
//         }
//         crate::backend::traits::emit_load_default(self, dest, ptr, ty);
//     }
//
//     pub(super) fn emit_store_with_const_offset_impl(&mut self, val: &Operand, base: &Value, offset: i64, ty: IrType) {
//         if ty == IrType::F128 {
//             crate::backend::f128_softfloat::f128_emit_store_with_offset(self, val, base, offset);
//             return;
//         }
//         // For non-F128, emit the operand then use the default path.
//         self.operand_to_t0(val);
//         let addr = self.state.resolve_slot_addr(base.0);
//         if let Some(addr) = addr {
//             let store_instr = Self::store_for_type(ty);
//             match addr {
//                 SlotAddr::OverAligned(slot, id) => {
//                     self.state.emit("    mv t3, t0");
//                     self.emit_alloca_aligned_addr_impl(slot, id);
//                     self.emit_add_offset_to_addr_reg_impl(offset);
//                     self.state.emit_fmt(format_args!("    {} t3, 0(t5)", store_instr));
//                 }
//                 SlotAddr::Direct(slot) => {
//                     let folded_slot = StackSlot(slot.0 + offset);
//                     self.emit_store_to_s0("t0", folded_slot.0, store_instr);
//                 }
//                 SlotAddr::Indirect(slot) => {
//                     self.state.emit("    mv t3, t0");
//                     self.emit_load_ptr_from_slot_impl(slot, base.0);
//                     if offset != 0 {
//                         self.emit_add_offset_to_addr_reg_impl(offset);
//                     }
//                     self.state.emit_fmt(format_args!("    {} t3, 0(t5)", store_instr));
//                 }
//             }
//         }
//     }
//
//     pub(super) fn emit_load_with_const_offset_impl(&mut self, dest: &Value, base: &Value, offset: i64, ty: IrType) {
//         if ty == IrType::F128 {
//             crate::backend::f128_softfloat::f128_emit_load_with_offset(self, dest, base, offset);
//             return;
//         }
//         // Default path for non-F128.
//         let addr = self.state.resolve_slot_addr(base.0);
//         if let Some(addr) = addr {
//             let load_instr = Self::load_for_type(ty);
//             match addr {
//                 SlotAddr::OverAligned(slot, id) => {
//                     self.emit_alloca_aligned_addr_impl(slot, id);
//                     self.emit_add_offset_to_addr_reg_impl(offset);
//                     self.state.emit_fmt(format_args!("    {} t0, 0(t5)", load_instr));
//                 }
//                 SlotAddr::Direct(slot) => {
//                     let folded_slot = StackSlot(slot.0 + offset);
//                     self.emit_load_from_s0("t0", folded_slot.0, load_instr);
//                 }
//                 SlotAddr::Indirect(slot) => {
//                     self.emit_load_ptr_from_slot_impl(slot, base.0);
//                     if offset != 0 {
//                         self.emit_add_offset_to_addr_reg_impl(offset);
//                     }
//                     self.state.emit_fmt(format_args!("    {} t0, 0(t5)", load_instr));
//                 }
//             }
//             self.store_t0_to(dest);
//         }
//     }
//
//     // ---- Typed store/load to/from slot ----
//
//     pub(super) fn emit_typed_store_to_slot_impl(&mut self, instr: &'static str, _ty: IrType, slot: StackSlot) {
//         self.emit_store_to_s0("t0", slot.0, instr);
//     }
//
//     pub(super) fn emit_typed_load_from_slot_impl(&mut self, instr: &'static str, slot: StackSlot) {
//         self.emit_load_from_s0("t0", slot.0, instr);
//     }
//
//     // ---- Pointer/address helpers ----
//
//     pub(super) fn emit_load_ptr_from_slot_impl(&mut self, slot: StackSlot, val_id: u32) {
//         // Check register allocation: use callee-saved register if available.
//         if let Some(&reg) = self.reg_assignments.get(&val_id) {
//             let reg_name = callee_saved_name(reg);
//             self.state.emit_fmt(format_args!("    mv t5, {}", reg_name));
//         } else {
//             self.emit_load_from_s0("t5", slot.0, "ld");
//         }
//     }
//
//     pub(super) fn emit_add_offset_to_addr_reg_impl(&mut self, offset: i64) {
//         if Self::fits_imm12(offset) {
//             self.state.emit_fmt(format_args!("    addi t5, t5, {}", offset));
//         } else {
//             self.state.emit_fmt(format_args!("    li t6, {}", offset));
//             self.state.emit("    add t5, t5, t6");
//         }
//     }
//
//     pub(super) fn emit_slot_addr_to_secondary_impl(&mut self, slot: StackSlot, is_alloca: bool, val_id: u32) {
//         if is_alloca {
//             self.emit_alloca_addr("t1", val_id, slot.0);
//         } else if let Some(&reg) = self.reg_assignments.get(&val_id) {
//             let reg_name = callee_saved_name(reg);
//             self.state.emit_fmt(format_args!("    mv t1, {}", reg_name));
//         } else {
//             self.emit_load_from_s0("t1", slot.0, "ld");
//         }
//     }
//
//     // ---- GEP ----
//
//     pub(super) fn emit_gep_direct_const_impl(&mut self, slot: StackSlot, offset: i64) {
//         let folded = slot.0 + offset;
//         self.emit_addi_s0("t0", folded);
//     }
//
//     pub(super) fn emit_gep_indirect_const_impl(&mut self, slot: StackSlot, offset: i64, val_id: u32) {
//         if let Some(&reg) = self.reg_assignments.get(&val_id) {
//             let reg_name = callee_saved_name(reg);
//             self.state.emit_fmt(format_args!("    mv t0, {}", reg_name));
//         } else {
//             self.emit_load_from_s0("t0", slot.0, "ld");
//         }
//         if offset != 0 {
//             self.emit_add_imm_to_acc_impl(offset);
//         }
//     }
//
//     pub(super) fn emit_add_imm_to_acc_impl(&mut self, imm: i64) {
//         if (-2048..=2047).contains(&imm) {
//             self.state.emit_fmt(format_args!("    addi t0, t0, {}", imm));
//         } else {
//             self.state.emit_fmt(format_args!("    li t1, {}", imm));
//             self.state.emit("    add t0, t0, t1");
//         }
//     }
//
//     // ---- Alloca alignment ----
//
//     pub(super) fn emit_alloca_aligned_addr_impl(&mut self, slot: StackSlot, val_id: u32) {
//         let align = self.state.alloca_over_align(val_id)
//             .expect("alloca must have over-alignment for aligned addr emission");
//         // Compute s0 + slot_offset into t5 (pointer register)
//         self.emit_addi_s0("t5", slot.0);
//         // Align: t5 = (t5 + align-1) & -align
//         self.state.emit_fmt(format_args!("    li t6, {}", align - 1));
//         self.state.emit("    add t5, t5, t6");
//         self.state.emit_fmt(format_args!("    li t6, -{}", align));
//         self.state.emit("    and t5, t5, t6");
//     }
//
//     pub(super) fn emit_alloca_aligned_addr_to_acc_impl(&mut self, slot: StackSlot, val_id: u32) {
//         let align = self.state.alloca_over_align(val_id)
//             .expect("alloca must have over-alignment for aligned addr emission");
//         // Compute s0 + slot_offset into t0 (accumulator)
//         self.emit_addi_s0("t0", slot.0);
//         // Align: t0 = (t0 + align-1) & -align
//         self.state.emit_fmt(format_args!("    li t6, {}", align - 1));
//         self.state.emit("    add t0, t0, t6");
//         self.state.emit_fmt(format_args!("    li t6, -{}", align));
//         self.state.emit("    and t0, t0, t6");
//         // t0 now holds an aligned address, not any previous SSA value.
//         self.state.reg_cache.invalidate_acc();
//     }
//
//     // ---- Memcpy ----
//
//     pub(super) fn emit_memcpy_load_dest_addr_impl(&mut self, slot: StackSlot, is_alloca: bool, val_id: u32) {
//         if is_alloca {
//             self.emit_alloca_addr("t1", val_id, slot.0);
//         } else if let Some(&reg) = self.reg_assignments.get(&val_id) {
//             let reg_name = callee_saved_name(reg);
//             self.state.emit_fmt(format_args!("    mv t1, {}", reg_name));
//         } else {
//             self.emit_load_from_s0("t1", slot.0, "ld");
//         }
//     }
//
//     pub(super) fn emit_memcpy_load_src_addr_impl(&mut self, slot: StackSlot, is_alloca: bool, val_id: u32) {
//         if is_alloca {
//             self.emit_alloca_addr("t2", val_id, slot.0);
//         } else if let Some(&reg) = self.reg_assignments.get(&val_id) {
//             let reg_name = callee_saved_name(reg);
//             self.state.emit_fmt(format_args!("    mv t2, {}", reg_name));
//         } else {
//             self.emit_load_from_s0("t2", slot.0, "ld");
//         }
//     }
//
//     pub(super) fn emit_memcpy_impl_impl(&mut self, size: usize) {
//         let label_id = self.state.next_label_id();
//         let loop_label = format!(".Lmemcpy_loop_{}", label_id);
//         let done_label = format!(".Lmemcpy_done_{}", label_id);
//         self.state.emit_fmt(format_args!("    li t3, {}", size));
//         self.state.emit_fmt(format_args!("{}:", loop_label));
//         self.state.emit_fmt(format_args!("    beqz t3, {}", done_label));
//         self.state.emit("    lbu t4, 0(t2)");
//         self.state.emit("    sb t4, 0(t1)");
//         self.state.emit("    addi t1, t1, 1");
//         self.state.emit("    addi t2, t2, 1");
//         self.state.emit("    addi t3, t3, -1");
//         self.state.emit_fmt(format_args!("    j {}", loop_label));
//         self.state.emit_fmt(format_args!("{}:", done_label));
//     }
//
//     // ---- DynAlloca helpers ----
//
//     pub(super) fn emit_round_up_acc_to_16_impl(&mut self) {
//         self.state.emit("    addi t0, t0, 15");
//         self.state.emit("    andi t0, t0, -16");
//     }
//
//     pub(super) fn emit_sub_sp_by_acc_impl(&mut self) {
//         self.state.emit("    sub sp, sp, t0");
//     }
//
//     pub(super) fn emit_mov_sp_to_acc_impl(&mut self) {
//         self.state.emit("    mv t0, sp");
//     }
//
//     pub(super) fn emit_mov_acc_to_sp_impl(&mut self) {
//         self.state.emit("    mv sp, t0");
//     }
//
//     pub(super) fn emit_align_acc_impl(&mut self, align: usize) {
//         self.state.emit_fmt(format_args!("    addi t0, t0, {}", align - 1));
//         self.state.emit_fmt(format_args!("    andi t0, t0, -{}", align));
//     }
// }

}  // namespace c4c::backend::riscv::codegen

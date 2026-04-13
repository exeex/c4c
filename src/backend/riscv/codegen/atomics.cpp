// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/atomics.rs
// Structural mirror of the ref Rust source; a dedicated RISC-V C++ codegen
// header is not present yet, so this file keeps the translation local and
// method-shaped without inventing a fake public interface.
//
// //! RISC-V sub-word atomic operations, atomic load/store/RMW/CAS, fence,
// //! and software implementations of CLZ/CTZ/BSWAP/POPCOUNT builtins.
// //
// // use crate::ir::reexports::{AtomicOrdering, AtomicRmwOp, Operand, Value};
// // use crate::common::types::IrType;
// // use crate::backend::state::CodegenState;
// // use super::emit::RiscvCodegen;
// //
// // impl RiscvCodegen {
// //     pub(super) fn amo_ordering(ordering: AtomicOrdering) -> &'static str { ... }
// //     pub(super) fn amo_width_suffix(ty: IrType) -> &'static str { ... }
// //     pub(super) fn sign_extend_riscv(state: &mut CodegenState, ty: IrType) { ... }
// //     pub(super) fn is_subword_type(ty: IrType) -> bool { ... }
// //     pub(super) fn subword_bits(ty: IrType) -> u32 { ... }
// //     pub(super) fn emit_subword_atomic_rmw(&mut self, op: AtomicRmwOp, ty: IrType, aq_rl: &str) { ... }
// //     pub(super) fn emit_subword_atomic_cmpxchg(&mut self, ty: IrType, aq_rl: &str, returns_bool: bool) { ... }
// //     pub(super) fn emit_clz(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_ctz(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_bswap(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_popcount(&mut self, ty: IrType) { ... }
// //     pub(super) fn emit_atomic_rmw_impl(&mut self, dest: &Value, op: AtomicRmwOp, ptr: &Operand, val: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_atomic_cmpxchg_impl(&mut self, dest: &Value, ptr: &Operand, expected: &Operand, desired: &Operand, ty: IrType, ordering: AtomicOrdering, _failure_ordering: AtomicOrdering, returns_bool: bool) { ... }
// //     pub(super) fn emit_atomic_load_impl(&mut self, dest: &Value, ptr: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_atomic_store_impl(&mut self, ptr: &Operand, val: &Operand, ty: IrType, ordering: AtomicOrdering) { ... }
// //     pub(super) fn emit_fence_impl(&mut self, ordering: AtomicOrdering) { ... }
// // }

#include "riscv_codegen.hpp"

#include <stdexcept>

namespace c4c::backend::riscv::codegen {

namespace {

std::uint64_t next_riscv_atomic_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

std::string next_riscv_atomic_label(std::string_view prefix) {
  return ".L" + std::string(prefix) + "_" + std::to_string(next_riscv_atomic_label_id());
}

const char* riscv_subword_atomic_load_instr(IrType ty) {
  switch (ty) {
    case IrType::I8: return "lb";
    case IrType::I16: return "lh";
    case IrType::U16: return "lhu";
    default:
      throw std::invalid_argument("riscv_subword_atomic_load_instr requires an I8/I16/U16 type");
  }
}

const char* riscv_subword_atomic_store_instr(IrType ty) {
  switch (ty) {
    case IrType::I8: return "sb";
    case IrType::I16:
    case IrType::U16: return "sh";
    default:
      throw std::invalid_argument("riscv_subword_atomic_store_instr requires an I8/I16/U16 type");
  }
}

void riscv_mask_subword_reg(RiscvCodegenState& state, const char* reg, std::uint32_t bits) {
  if (bits == 8) {
    state.emit("    andi " + std::string(reg) + ", " + std::string(reg) + ", 0xff");
    return;
  }

  state.emit("    slli " + std::string(reg) + ", " + std::string(reg) + ", 48");
  state.emit("    srli " + std::string(reg) + ", " + std::string(reg) + ", 48");
}

void riscv_extract_subword_atomic_result(RiscvCodegenState& state, std::uint32_t bits) {
  state.emit("    srlw t0, t0, a3");
  if (bits == 8) {
    state.emit("    andi t0, t0, 0xff");
    return;
  }

  state.emit("    slli t0, t0, 48");
  state.emit("    srli t0, t0, 48");
}

const char* riscv_atomic_load_ordering_suffix(AtomicOrdering ordering) {
  switch (ordering) {
    case AtomicOrdering::Relaxed:
    case AtomicOrdering::Release:
      return "";
    case AtomicOrdering::Acquire:
      return ".aq";
    case AtomicOrdering::AcqRel:
    case AtomicOrdering::SeqCst:
      return ".aqrl";
  }

  return "";
}

}  // namespace

const char* riscv_amo_ordering_suffix(AtomicOrdering ordering) {
  switch (ordering) {
    case AtomicOrdering::Relaxed: return "";
    case AtomicOrdering::Acquire: return ".aq";
    case AtomicOrdering::Release: return ".rl";
    case AtomicOrdering::AcqRel:
    case AtomicOrdering::SeqCst: return ".aqrl";
  }

  return "";
}

const char* riscv_amo_width_suffix(IrType ty) {
  switch (ty) {
    case IrType::I32:
    case IrType::U32:
      return "w";
    default:
      return "d";
  }
}

void riscv_sign_extend_atomic_result(RiscvCodegenState& state, IrType ty) {
  switch (ty) {
    case IrType::I8:
      state.emit("    slli t0, t0, 56");
      state.emit("    srai t0, t0, 56");
      return;
    case IrType::I16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srai t0, t0, 48");
      return;
    case IrType::U16:
      state.emit("    slli t0, t0, 48");
      state.emit("    srli t0, t0, 48");
      return;
    case IrType::I32:
      state.emit("    sext.w t0, t0");
      return;
    default:
      return;
  }
}

bool riscv_is_atomic_subword_type(IrType ty) {
  return ty == IrType::I8 || ty == IrType::I16 || ty == IrType::U16;
}

std::uint32_t riscv_atomic_subword_bits(IrType ty) {
  switch (ty) {
    case IrType::I8: return 8;
    case IrType::I16:
    case IrType::U16: return 16;
    default:
      throw std::invalid_argument("riscv_atomic_subword_bits requires an I8/I16/U16 type");
  }
}

void RiscvCodegen::emit_subword_atomic_rmw(AtomicRmwOp op, IrType ty, const char* aq_rl) {
  const auto bits = riscv_atomic_subword_bits(ty);
  const auto loop_label = next_riscv_atomic_label("sw_rmw_loop");

  state.emit("    andi a2, t1, -4");
  state.emit("    andi a3, t1, 3");
  state.emit("    slli a3, a3, 3");
  if (bits == 8) {
    state.emit("    li a4, 0xff");
  } else {
    state.emit("    lui a4, 16");
    state.emit("    addiw a4, a4, -1");
  }
  state.emit("    sllw a4, a4, a3");
  state.emit("    not a5, a4");
  riscv_mask_subword_reg(state, "t2", bits);
  state.emit("    sllw t2, t2, a3");

  state.emit(loop_label + ":");
  state.emit("    lr.w" + std::string(aq_rl) + " t0, (a2)");

  switch (op) {
    case AtomicRmwOp::Xchg:
      state.emit("    mv t3, t2");
      state.emit("    and t4, t0, a5");
      state.emit("    or t4, t4, t3");
      break;
    case AtomicRmwOp::TestAndSet:
      state.emit("    li t3, 1");
      state.emit("    sllw t3, t3, a3");
      state.emit("    and t4, t0, a5");
      state.emit("    or t4, t4, t3");
      break;
    case AtomicRmwOp::Add:
      state.emit("    and t3, t0, a4");
      state.emit("    add t3, t3, t2");
      state.emit("    and t3, t3, a4");
      state.emit("    and t4, t0, a5");
      state.emit("    or t4, t4, t3");
      break;
    case AtomicRmwOp::Sub:
      state.emit("    and t3, t0, a4");
      state.emit("    sub t3, t3, t2");
      state.emit("    and t3, t3, a4");
      state.emit("    and t4, t0, a5");
      state.emit("    or t4, t4, t3");
      break;
    case AtomicRmwOp::And:
      state.emit("    or t3, t2, a5");
      state.emit("    and t4, t0, t3");
      break;
    case AtomicRmwOp::Or:
      state.emit("    and t3, t2, a4");
      state.emit("    or t4, t0, t3");
      break;
    case AtomicRmwOp::Xor:
      state.emit("    and t3, t2, a4");
      state.emit("    xor t4, t0, t3");
      break;
    case AtomicRmwOp::Nand:
      state.emit("    and t3, t0, a4");
      state.emit("    and t3, t3, t2");
      state.emit("    not t3, t3");
      state.emit("    and t3, t3, a4");
      state.emit("    and t4, t0, a5");
      state.emit("    or t4, t4, t3");
      break;
  }

  state.emit("    sc.w" + std::string(aq_rl) + " t5, t4, (a2)");
  state.emit("    bnez t5, " + loop_label);
  riscv_extract_subword_atomic_result(state, bits);
}

void RiscvCodegen::emit_subword_atomic_cmpxchg(IrType ty, const char* aq_rl, bool returns_bool) {
  const auto bits = riscv_atomic_subword_bits(ty);
  const auto loop_label = next_riscv_atomic_label("sw_cas_loop");
  const auto fail_label = next_riscv_atomic_label("sw_cas_fail");
  const auto done_label = next_riscv_atomic_label("sw_cas_done");

  state.emit("    andi a2, t1, -4");
  state.emit("    andi a3, t1, 3");
  state.emit("    slli a3, a3, 3");
  if (bits == 8) {
    state.emit("    li a4, 0xff");
  } else {
    state.emit("    lui a4, 16");
    state.emit("    addiw a4, a4, -1");
  }
  state.emit("    sllw a4, a4, a3");
  state.emit("    not a5, a4");
  riscv_mask_subword_reg(state, "t2", bits);
  state.emit("    sllw t2, t2, a3");
  riscv_mask_subword_reg(state, "t3", bits);
  state.emit("    sllw t3, t3, a3");

  state.emit(loop_label + ":");
  state.emit("    lr.w" + std::string(aq_rl) + " t0, (a2)");
  state.emit("    and t4, t0, a4");
  state.emit("    bne t4, t2, " + fail_label);
  state.emit("    and t4, t0, a5");
  state.emit("    or t4, t4, t3");
  state.emit("    sc.w" + std::string(aq_rl) + " t5, t4, (a2)");
  state.emit("    bnez t5, " + loop_label);
  if (returns_bool) {
    state.emit("    li t0, 1");
  } else {
    riscv_extract_subword_atomic_result(state, bits);
  }
  state.emit("    j " + done_label);
  state.emit(fail_label + ":");
  if (returns_bool) {
    state.emit("    li t0, 0");
  } else {
    riscv_extract_subword_atomic_result(state, bits);
  }
  state.emit(done_label + ":");
}

void RiscvCodegen::emit_clz(IrType ty) {
  const auto bits = (ty == IrType::I32 || ty == IrType::U32) ? 32u : 64u;
  const auto loop_label = next_riscv_atomic_label("clz_loop");
  const auto done_label = next_riscv_atomic_label("clz_done");
  const auto zero_label = next_riscv_atomic_label("clz_zero");

  if (bits == 32) {
    state.emit("    slli t0, t0, 32");
    state.emit("    srli t0, t0, 32");
  }

  state.emit("    beqz t0, " + zero_label);
  state.emit("    li t1, 0");
  state.emit("    li t2, 1");
  state.emit("    slli t2, t2, " + std::to_string(bits - 1));
  state.emit(loop_label + ":");
  state.emit("    and t3, t0, t2");
  state.emit("    bnez t3, " + done_label);
  state.emit("    srli t2, t2, 1");
  state.emit("    addi t1, t1, 1");
  state.emit("    j " + loop_label);
  state.emit(zero_label + ":");
  state.emit("    li t1, " + std::to_string(bits));
  state.emit(done_label + ":");
  state.emit("    mv t0, t1");
}

void RiscvCodegen::emit_ctz(IrType ty) {
  const auto bits = (ty == IrType::I32 || ty == IrType::U32) ? 32u : 64u;
  const auto loop_label = next_riscv_atomic_label("ctz_loop");
  const auto done_label = next_riscv_atomic_label("ctz_done");

  state.emit("    li t1, 0");
  state.emit(loop_label + ":");
  state.emit("    li t2, " + std::to_string(bits));
  state.emit("    beq t1, t2, " + done_label);
  state.emit("    andi t3, t0, 1");
  state.emit("    bnez t3, " + done_label);
  state.emit("    srli t0, t0, 1");
  state.emit("    addi t1, t1, 1");
  state.emit("    j " + loop_label);
  state.emit(done_label + ":");
  state.emit("    mv t0, t1");
}

void RiscvCodegen::emit_bswap(IrType ty) {
  switch (ty) {
    case IrType::I16:
    case IrType::U16:
      state.emit("    andi t1, t0, 0xff");
      state.emit("    slli t1, t1, 8");
      state.emit("    srli t0, t0, 8");
      state.emit("    andi t0, t0, 0xff");
      state.emit("    or t0, t0, t1");
      return;
    case IrType::I32:
    case IrType::U32:
      state.emit("    mv t1, t0");
      state.emit("    andi t2, t1, 0xff");
      state.emit("    slli t0, t2, 24");
      state.emit("    srli t2, t1, 8");
      state.emit("    andi t2, t2, 0xff");
      state.emit("    slli t2, t2, 16");
      state.emit("    or t0, t0, t2");
      state.emit("    srli t2, t1, 16");
      state.emit("    andi t2, t2, 0xff");
      state.emit("    slli t2, t2, 8");
      state.emit("    or t0, t0, t2");
      state.emit("    srli t2, t1, 24");
      state.emit("    andi t2, t2, 0xff");
      state.emit("    or t0, t0, t2");
      state.emit("    slli t0, t0, 32");
      state.emit("    srli t0, t0, 32");
      return;
    default:
      state.emit("    mv t1, t0");
      state.emit("    li t0, 0");
      for (std::uint32_t i = 0; i < 8; ++i) {
        const auto src_shift = i * 8;
        const auto dst_shift = (7 - i) * 8;
        state.emit("    srli t2, t1, " + std::to_string(src_shift));
        state.emit("    andi t2, t2, 0xff");
        if (dst_shift > 0) {
          state.emit("    slli t2, t2, " + std::to_string(dst_shift));
        }
        state.emit("    or t0, t0, t2");
      }
      return;
  }
}

void RiscvCodegen::emit_popcount(IrType ty) {
  const auto loop_label = next_riscv_atomic_label("popcnt_loop");
  const auto done_label = next_riscv_atomic_label("popcnt_done");

  if (ty == IrType::I32 || ty == IrType::U32) {
    state.emit("    slli t0, t0, 32");
    state.emit("    srli t0, t0, 32");
  }

  state.emit("    li t1, 0");
  state.emit(loop_label + ":");
  state.emit("    beqz t0, " + done_label);
  state.emit("    addi t2, t0, -1");
  state.emit("    and t0, t0, t2");
  state.emit("    addi t1, t1, 1");
  state.emit("    j " + loop_label);
  state.emit(done_label + ":");
  state.emit("    mv t0, t1");
}

void RiscvCodegen::emit_atomic_rmw_impl(const Value& dest,
                                        AtomicRmwOp op,
                                        const Operand& ptr,
                                        const Operand& val,
                                        IrType ty,
                                        AtomicOrdering ordering) {
  operand_to_t0(ptr);
  state.emit("    mv t1, t0");
  operand_to_t0(val);
  state.emit("    mv t2, t0");

  const auto* aq_rl = riscv_amo_ordering_suffix(ordering);
  if (riscv_is_atomic_subword_type(ty)) {
    emit_subword_atomic_rmw(op, ty, aq_rl);
  } else {
    switch (op) {
      case AtomicRmwOp::Add:
        state.emit("    amoadd." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::Sub:
        state.emit("    neg t2, t2");
        state.emit("    amoadd." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::And:
        state.emit("    amoand." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::Or:
        state.emit("    amoor." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::Xor:
        state.emit("    amoxor." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::Xchg:
        state.emit("    amoswap." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
      case AtomicRmwOp::Nand: {
        const auto loop_label = next_riscv_atomic_label("atomic_nand");
        state.emit(loop_label + ":");
        state.emit("    lr." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, (t1)");
        state.emit("    and t3, t0, t2");
        state.emit("    not t3, t3");
        state.emit("    sc." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t4, t3, (t1)");
        state.emit("    bnez t4, " + loop_label);
        break;
      }
      case AtomicRmwOp::TestAndSet:
        state.emit("    li t2, 1");
        state.emit("    amoswap." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, t2, (t1)");
        break;
    }
  }

  riscv_sign_extend_atomic_result(state, ty);
  store_t0_to(dest);
}

void RiscvCodegen::emit_atomic_cmpxchg_impl(const Value& dest,
                                            const Operand& ptr,
                                            const Operand& expected,
                                            const Operand& desired,
                                            IrType ty,
                                            AtomicOrdering ordering,
                                            AtomicOrdering /*failure_ordering*/,
                                            bool returns_bool) {
  operand_to_t0(ptr);
  state.emit("    mv t1, t0");
  operand_to_t0(desired);
  state.emit("    mv t3, t0");
  operand_to_t0(expected);
  state.emit("    mv t2, t0");

  const auto* aq_rl = riscv_amo_ordering_suffix(ordering);
  if (riscv_is_atomic_subword_type(ty)) {
    emit_subword_atomic_cmpxchg(ty, aq_rl, returns_bool);
  } else {
    const auto loop_label = next_riscv_atomic_label("cas_loop");
    const auto fail_label = next_riscv_atomic_label("cas_fail");
    const auto done_label = next_riscv_atomic_label("cas_done");

    state.emit(loop_label + ":");
    state.emit("    lr." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t0, (t1)");
    state.emit("    bne t0, t2, " + fail_label);
    state.emit("    sc." + std::string(riscv_amo_width_suffix(ty)) + aq_rl + " t4, t3, (t1)");
    state.emit("    bnez t4, " + loop_label);
    if (returns_bool) {
      state.emit("    li t0, 1");
    }
    state.emit("    j " + done_label);
    state.emit(fail_label + ":");
    if (returns_bool) {
      state.emit("    li t0, 0");
    }
    state.emit(done_label + ":");
  }

  store_t0_to(dest);
}

void RiscvCodegen::emit_atomic_load_impl(const Value& dest,
                                         const Operand& ptr,
                                         IrType ty,
                                         AtomicOrdering ordering) {
  operand_to_t0(ptr);
  if (riscv_is_atomic_subword_type(ty)) {
    if (ordering == AtomicOrdering::SeqCst) {
      state.emit("    fence rw, rw");
    }
    state.emit("    " + std::string(riscv_subword_atomic_load_instr(ty)) + " t0, 0(t0)");
    if (ordering == AtomicOrdering::Acquire || ordering == AtomicOrdering::AcqRel ||
        ordering == AtomicOrdering::SeqCst) {
      state.emit("    fence r, rw");
    }
  } else {
    state.emit("    lr." + std::string(riscv_amo_width_suffix(ty)) +
               riscv_atomic_load_ordering_suffix(ordering) + " t0, (t0)");
    riscv_sign_extend_atomic_result(state, ty);
  }
  store_t0_to(dest);
}

void RiscvCodegen::emit_atomic_store_impl(const Operand& ptr,
                                          const Operand& val,
                                          IrType ty,
                                          AtomicOrdering ordering) {
  operand_to_t0(val);
  state.emit("    mv t1, t0");
  operand_to_t0(ptr);
  if (riscv_is_atomic_subword_type(ty)) {
    if (ordering == AtomicOrdering::Release || ordering == AtomicOrdering::AcqRel ||
        ordering == AtomicOrdering::SeqCst) {
      state.emit("    fence rw, w");
    }
    state.emit("    " + std::string(riscv_subword_atomic_store_instr(ty)) + " t1, 0(t0)");
    if (ordering == AtomicOrdering::SeqCst) {
      state.emit("    fence rw, rw");
    }
    return;
  }
  state.emit("    amoswap." + std::string(riscv_amo_width_suffix(ty)) +
             riscv_amo_ordering_suffix(ordering) + " zero, t1, (t0)");
}

void RiscvCodegen::emit_fence_impl(AtomicOrdering ordering) {
  switch (ordering) {
    case AtomicOrdering::Relaxed:
      return;
    case AtomicOrdering::Acquire:
      state.emit("    fence r, rw");
      return;
    case AtomicOrdering::Release:
      state.emit("    fence rw, w");
      return;
    case AtomicOrdering::AcqRel:
    case AtomicOrdering::SeqCst:
      state.emit("    fence rw, rw");
      return;
  }
}

}  // namespace c4c::backend::riscv::codegen

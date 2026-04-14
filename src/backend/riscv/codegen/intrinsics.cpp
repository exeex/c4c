// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs
// The shared RISC-V C++ intrinsics activation surface now lives in
// riscv_codegen.hpp. The direct dispatcher owner remains parked while this
// file only exposes the translated helper family through the shared codegen
// class surface.

#include "riscv_codegen.hpp"

#include <string>
#include <string_view>

namespace c4c::backend::riscv::codegen {

namespace {

std::uint64_t next_riscv_intrinsics_label_id() {
  static std::uint64_t next_label_id = 0;
  return next_label_id++;
}

std::string next_riscv_intrinsics_label(std::string_view prefix) {
  return ".L" + std::string(prefix) + "_" + std::to_string(next_riscv_intrinsics_label_id());
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

void move_if_needed(RiscvCodegenState& state, const char* dst, const char* src) {
  if (std::string_view(dst) == src) {
    return;
  }
  state.emit("    mv " + std::string(dst) + ", " + std::string(src));
}

}  // namespace

void RiscvCodegen::emit_rv_binary_128(std::string_view op) {
  state.emit("    ld t1, 0(t0)");
  state.emit("    ld t2, 8(t0)");
  state.emit("    ld t3, 0(a7)");
  state.emit("    ld t4, 8(a7)");
  state.emit("    " + std::string(op) + " t1, t1, t3");
  state.emit("    " + std::string(op) + " t2, t2, t4");
  state.emit("    sd t1, 0(a5)");
  state.emit("    sd t2, 8(a5)");
}

void RiscvCodegen::emit_rv_zero_byte_mask(std::string_view src_reg, std::string_view dst_reg) {
  state.emit("    li t5, 0x0101010101010101");
  state.emit("    li t6, 0x8080808080808080");
  state.emit("    sub " + std::string(dst_reg) + ", " + std::string(src_reg) + ", t5");
  state.emit("    not t5, " + std::string(src_reg));
  state.emit("    and " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  state.emit("    and " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t6");
  state.emit("    srli " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", 7");
  state.emit("    slli t5, " + std::string(dst_reg) + ", 1");
  state.emit("    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  state.emit("    slli t5, " + std::string(dst_reg) + ", 2");
  state.emit("    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  state.emit("    slli t5, " + std::string(dst_reg) + ", 4");
  state.emit("    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
}

void RiscvCodegen::emit_rv_cmpeq_bytes() {
  state.emit("    mv a6, t0");
  state.emit("    ld t1, 0(a6)");
  state.emit("    ld t2, 0(a7)");
  state.emit("    xor t1, t1, t2");
  emit_rv_zero_byte_mask("t1", "t3");
  state.emit("    ld t1, 8(a6)");
  state.emit("    ld t2, 8(a7)");
  state.emit("    xor t1, t1, t2");
  emit_rv_zero_byte_mask("t1", "t4");
  state.emit("    sd t3, 0(a5)");
  state.emit("    sd t4, 8(a5)");
}

void RiscvCodegen::emit_rv_cmpeq_dwords() {
  state.emit("    mv a6, t0");
  state.emit("    ld t1, 0(a6)");
  state.emit("    ld t2, 0(a7)");
  state.emit("    slli t3, t1, 32");
  state.emit("    slli t4, t2, 32");
  state.emit("    srli t3, t3, 32");
  state.emit("    srli t4, t4, 32");
  state.emit("    sub t3, t3, t4");
  state.emit("    snez t3, t3");
  state.emit("    neg t3, t3");
  state.emit("    not t3, t3");
  state.emit("    slli t3, t3, 32");
  state.emit("    srli t3, t3, 32");
  state.emit("    srli t5, t1, 32");
  state.emit("    srli t6, t2, 32");
  state.emit("    sub t5, t5, t6");
  state.emit("    snez t5, t5");
  state.emit("    neg t5, t5");
  state.emit("    not t5, t5");
  state.emit("    slli t5, t5, 32");
  state.emit("    or t3, t3, t5");
  state.emit("    sd t3, 0(a5)");
  state.emit("    ld t1, 8(a6)");
  state.emit("    ld t2, 8(a7)");
  state.emit("    slli t3, t1, 32");
  state.emit("    slli t4, t2, 32");
  state.emit("    srli t3, t3, 32");
  state.emit("    srli t4, t4, 32");
  state.emit("    sub t3, t3, t4");
  state.emit("    snez t3, t3");
  state.emit("    neg t3, t3");
  state.emit("    not t3, t3");
  state.emit("    slli t3, t3, 32");
  state.emit("    srli t3, t3, 32");
  state.emit("    srli t5, t1, 32");
  state.emit("    srli t6, t2, 32");
  state.emit("    sub t5, t5, t6");
  state.emit("    snez t5, t5");
  state.emit("    neg t5, t5");
  state.emit("    not t5, t5");
  state.emit("    slli t5, t5, 32");
  state.emit("    or t3, t3, t5");
  state.emit("    sd t3, 8(a5)");
}

void RiscvCodegen::emit_rv_binary_128_bytewise() {
  state.emit("    mv a6, t0");
  state.emit("    ld t1, 0(a6)");
  state.emit("    ld t2, 0(a7)");
  emit_rv_psubusb_8bytes("t1", "t2", "t3");
  state.emit("    ld t1, 8(a6)");
  state.emit("    ld t2, 8(a7)");
  emit_rv_psubusb_8bytes("t1", "t2", "t4");
  state.emit("    sd t3, 0(a5)");
  state.emit("    sd t4, 8(a5)");
}

void RiscvCodegen::emit_rv_psubusb_8bytes(std::string_view a_reg,
                                          std::string_view b_reg,
                                          std::string_view dst_reg) {
  state.emit("    li " + std::string(dst_reg) + ", 0");
  for (int i = 0; i < 8; ++i) {
    const int shift = i * 8;
    if (shift == 0) {
      state.emit("    andi t5, " + std::string(a_reg) + ", 0xff");
      state.emit("    andi t6, " + std::string(b_reg) + ", 0xff");
    } else {
      state.emit("    srli t5, " + std::string(a_reg) + ", " + std::to_string(shift));
      state.emit("    andi t5, t5, 0xff");
      state.emit("    srli t6, " + std::string(b_reg) + ", " + std::to_string(shift));
      state.emit("    andi t6, t6, 0xff");
    }
    const std::string skip_label = "psub_skip_" + std::to_string(i);
    state.emit("    bltu t5, t6, " + skip_label);
    state.emit("    sub t5, t5, t6");
    if (shift > 0) {
      state.emit("    slli t5, t5, " + std::to_string(shift));
    }
    state.emit("    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
    state.emit(skip_label + ":");
  }
}

void RiscvCodegen::emit_rv_psubsb_8bytes(std::string_view a_reg,
                                         std::string_view b_reg,
                                         std::string_view dst_reg) {
  state.emit("    li " + std::string(dst_reg) + ", 0");
  for (int i = 0; i < 8; ++i) {
    const int shift = i * 8;
    if (shift == 0) {
      state.emit("    slli t5, " + std::string(a_reg) + ", 56");
      state.emit("    slli t6, " + std::string(b_reg) + ", 56");
    } else {
      state.emit("    slli t5, " + std::string(a_reg) + ", " + std::to_string(56 - shift));
      state.emit("    slli t6, " + std::string(b_reg) + ", " + std::to_string(56 - shift));
    }
    state.emit("    srai t5, t5, 56");
    state.emit("    srai t6, t6, 56");
    state.emit("    sub t5, t5, t6");
    const std::string no_hi = "psubsb_noclamp_hi_" + std::to_string(i);
    const std::string no_lo = "psubsb_noclamp_lo_" + std::to_string(i);
    const std::string done = "psubsb_done_" + std::to_string(i);
    state.emit("    li t6, 127");
    state.emit("    ble t5, t6, " + no_hi);
    state.emit("    li t5, 127");
    state.emit("    j " + done);
    state.emit(no_hi + ":");
    state.emit("    li t6, -128");
    state.emit("    bge t5, t6, " + no_lo);
    state.emit("    li t5, -128");
    state.emit(no_lo + ":");
    state.emit(done + ":");
    state.emit("    andi t5, t5, 0xff");
    if (shift > 0) {
      state.emit("    slli t5, t5, " + std::to_string(shift));
    }
    state.emit("    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  }
}

void RiscvCodegen::emit_rv_psubsb_128() {
  state.emit("    mv a6, t0");
  state.emit("    ld t1, 0(a6)");
  state.emit("    ld t2, 0(a7)");
  emit_rv_psubsb_8bytes("t1", "t2", "t3");
  state.emit("    ld t1, 8(a6)");
  state.emit("    ld t2, 8(a7)");
  emit_rv_psubsb_8bytes("t1", "t2", "t4");
  state.emit("    sd t3, 0(a5)");
  state.emit("    sd t4, 8(a5)");
}

void RiscvCodegen::emit_rv_pmovmskb() {
  state.emit("    ld t1, 0(t0)");
  state.emit("    ld t2, 8(t0)");
  state.emit("    li t3, 0x8080808080808080");
  state.emit("    and t1, t1, t3");
  state.emit("    and t2, t2, t3");
  state.emit("    li t3, 0x0002040810204081");
  state.emit("    mul t1, t1, t3");
  state.emit("    srli t1, t1, 56");
  state.emit("    mul t2, t2, t3");
  state.emit("    srli t2, t2, 56");
  state.emit("    slli t2, t2, 8");
  state.emit("    or t0, t1, t2");
}

void RiscvCodegen::emit_intrinsic_rv(const std::optional<Value>& dest,
                                     const IntrinsicOp& op,
                                     const std::optional<Value>& dest_ptr,
                                     const std::vector<Operand>& args) {
  auto load_pointer_value = [&](const Value& value, const char* reg) {
    const auto addr = state.resolve_slot_addr(value.raw);
    if (!addr.has_value()) {
      return;
    }

    switch (addr->kind) {
      case SlotAddr::Kind::Direct:
        state.emit("    ld " + std::string(reg) + ", " + std::to_string(addr->slot.raw) + "(s0)");
        return;
      case SlotAddr::Kind::Indirect:
        if (const auto assigned = state.assigned_reg(value.raw)) {
          state.emit("    mv " + std::string(reg) + ", " + callee_saved_name(*assigned));
        } else {
          state.emit("    ld " + std::string(reg) + ", " + std::to_string(addr->slot.raw) + "(s0)");
        }
        return;
      case SlotAddr::Kind::OverAligned:
        if (std::string_view(reg) == "t0") {
          emit_alloca_aligned_addr_to_acc_impl(addr->slot, addr->value_id);
        } else {
          emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
          move_if_needed(state, reg, "t5");
        }
        return;
    }
  };

  auto load_pointer_operand = [&](const Operand& operand, const char* reg) {
    if (operand.kind == Operand::Kind::Value) {
      if (const auto addr = state.resolve_slot_addr(operand.raw)) {
        switch (addr->kind) {
          case SlotAddr::Kind::Direct:
            state.emit("    ld " + std::string(reg) + ", " +
                       std::to_string(addr->slot.raw) + "(s0)");
            return;
          case SlotAddr::Kind::Indirect:
            if (const auto assigned = state.assigned_reg(operand.raw)) {
              state.emit("    mv " + std::string(reg) + ", " + callee_saved_name(*assigned));
            } else {
              state.emit("    ld " + std::string(reg) + ", " +
                         std::to_string(addr->slot.raw) + "(s0)");
            }
            return;
          case SlotAddr::Kind::OverAligned:
            if (std::string_view(reg) == "t0") {
              emit_alloca_aligned_addr_to_acc_impl(addr->slot, addr->value_id);
            } else {
              emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
              move_if_needed(state, reg, "t5");
            }
            return;
        }
      }
    }

    operand_to_t0(operand);
    move_if_needed(state, reg, "t0");
  };

  auto load_vector_binary_packet = [&]() -> bool {
    if (!dest_ptr.has_value() || args.size() < 2) {
      return false;
    }

    load_pointer_operand(args[0], "t0");
    load_pointer_operand(args[1], "a7");
    load_pointer_value(*dest_ptr, "a5");
    return true;
  };

  auto zero_vector_dest = [&]() {
    if (!dest_ptr.has_value()) {
      return;
    }

    load_pointer_value(*dest_ptr, "t0");
    state.emit("    sd zero, 0(t0)");
    state.emit("    sd zero, 8(t0)");
  };

  switch (op) {
    case IntrinsicOp::Lfence:
    case IntrinsicOp::Mfence:
      state.emit("    fence iorw, iorw");
      return;
    case IntrinsicOp::Sfence:
      state.emit("    fence ow, ow");
      return;
    case IntrinsicOp::Pause:
      state.emit("    fence.tso");
      return;
    case IntrinsicOp::Clflush:
      state.emit("    fence iorw, iorw");
      return;
    case IntrinsicOp::Movnti:
      if (dest_ptr.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    mv t1, t0");
        load_pointer_value(*dest_ptr, "t0");
        state.emit("    sw t1, 0(t0)");
      }
      return;
    case IntrinsicOp::Movnti64:
      if (dest_ptr.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    mv t1, t0");
        load_pointer_value(*dest_ptr, "t0");
        state.emit("    sd t1, 0(t0)");
      }
      return;
    case IntrinsicOp::Movntdq:
    case IntrinsicOp::Movntpd:
    case IntrinsicOp::Storedqu:
      if (dest_ptr.has_value() && !args.empty()) {
        load_pointer_operand(args[0], "t0");
        state.emit("    ld t1, 0(t0)");
        state.emit("    ld t2, 8(t0)");
        load_pointer_value(*dest_ptr, "t5");
        state.emit("    sd t1, 0(t5)");
        state.emit("    sd t2, 8(t5)");
      }
      return;
    case IntrinsicOp::Loaddqu:
      if (dest_ptr.has_value() && !args.empty()) {
        load_pointer_operand(args[0], "t0");
        state.emit("    ld t1, 0(t0)");
        state.emit("    ld t2, 8(t0)");
        load_pointer_value(*dest_ptr, "t5");
        state.emit("    sd t1, 0(t5)");
        state.emit("    sd t2, 8(t5)");
      }
      return;
    case IntrinsicOp::Pcmpeqb128:
      if (load_vector_binary_packet()) {
        emit_rv_cmpeq_bytes();
      }
      return;
    case IntrinsicOp::Pcmpeqd128:
      if (load_vector_binary_packet()) {
        emit_rv_cmpeq_dwords();
      }
      return;
    case IntrinsicOp::Psubusb128:
      if (load_vector_binary_packet()) {
        emit_rv_binary_128_bytewise();
      }
      return;
    case IntrinsicOp::Psubsb128:
      if (load_vector_binary_packet()) {
        emit_rv_psubsb_128();
      }
      return;
    case IntrinsicOp::Por128:
      if (load_vector_binary_packet()) {
        emit_rv_binary_128("or");
      }
      return;
    case IntrinsicOp::Pand128:
      if (load_vector_binary_packet()) {
        emit_rv_binary_128("and");
      }
      return;
    case IntrinsicOp::Pxor128:
      if (load_vector_binary_packet()) {
        emit_rv_binary_128("xor");
      }
      return;
    case IntrinsicOp::Pmovmskb128:
      if (dest.has_value() && !args.empty()) {
        load_pointer_operand(args[0], "t0");
        emit_rv_pmovmskb();
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::SetEpi8:
      if (dest_ptr.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    andi t0, t0, 0xff");
        state.emit("    li t1, 0x0101010101010101");
        state.emit("    mul t0, t0, t1");
        load_pointer_value(*dest_ptr, "t1");
        state.emit("    sd t0, 0(t1)");
        state.emit("    sd t0, 8(t1)");
      }
      return;
    case IntrinsicOp::SetEpi32:
      if (dest_ptr.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    slli t1, t0, 32");
        state.emit("    slli t0, t0, 32");
        state.emit("    srli t0, t0, 32");
        state.emit("    or t0, t0, t1");
        load_pointer_value(*dest_ptr, "t1");
        state.emit("    sd t0, 0(t1)");
        state.emit("    sd t0, 8(t1)");
      }
      return;
    case IntrinsicOp::Crc32_8:
    case IntrinsicOp::Crc32_16:
    case IntrinsicOp::Crc32_32:
    case IntrinsicOp::Crc32_64:
      if (dest.has_value() && args.size() >= 2) {
        const auto num_bits = op == IntrinsicOp::Crc32_8 ? 8
                              : op == IntrinsicOp::Crc32_16 ? 16
                              : op == IntrinsicOp::Crc32_32 ? 32
                                                            : 64;
        const auto loop_label = next_riscv_intrinsics_label("crc_loop");
        const auto done_label = next_riscv_intrinsics_label("crc_done");
        const auto skip_label = next_riscv_intrinsics_label("crc_skip");

        operand_to_t0(args[0]);
        state.emit("    mv t3, t0");
        operand_to_t0(args[1]);
        state.emit("    mv t4, t0");
        state.emit("    xor t3, t3, t4");
        state.emit("    li t5, 0x82F63B78");
        state.emit("    li t6, " + std::to_string(num_bits));
        state.emit(loop_label + ":");
        state.emit("    beqz t6, " + done_label);
        state.emit("    andi t0, t3, 1");
        state.emit("    srli t3, t3, 1");
        state.emit("    beqz t0, " + skip_label);
        state.emit("    xor t3, t3, t5");
        state.emit(skip_label + ":");
        state.emit("    addi t6, t6, -1");
        state.emit("    j " + loop_label);
        state.emit(done_label + ":");
        state.emit("    slli t3, t3, 32");
        state.emit("    srli t3, t3, 32");
        state.emit("    mv t0, t3");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::FrameAddress:
      if (dest.has_value()) {
        state.emit("    mv t0, s0");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::ReturnAddress:
      if (dest.has_value()) {
        state.emit("    ld t0, -8(s0)");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::ThreadPointer:
      if (dest.has_value()) {
        state.emit("    mv t0, tp");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::SqrtF64:
      if (dest.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    fmv.d.x ft0, t0");
        state.emit("    fsqrt.d ft0, ft0");
        state.emit("    fmv.x.d t0, ft0");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::SqrtF32:
      if (dest.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    fmv.w.x ft0, t0");
        state.emit("    fsqrt.s ft0, ft0");
        state.emit("    fmv.x.w t0, ft0");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::FabsF64:
      if (dest.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    fmv.d.x ft0, t0");
        state.emit("    fabs.d ft0, ft0");
        state.emit("    fmv.x.d t0, ft0");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::FabsF32:
      if (dest.has_value() && !args.empty()) {
        operand_to_t0(args[0]);
        state.emit("    fmv.w.x ft0, t0");
        state.emit("    fabs.s ft0, ft0");
        state.emit("    fmv.x.w t0, ft0");
        store_t0_to(*dest);
      }
      return;
    case IntrinsicOp::Aesenc128:
    case IntrinsicOp::Aesenclast128:
    case IntrinsicOp::Aesdec128:
    case IntrinsicOp::Aesdeclast128:
    case IntrinsicOp::Aesimc128:
    case IntrinsicOp::Aeskeygenassist128:
    case IntrinsicOp::Pclmulqdq128:
    case IntrinsicOp::Pslldqi128:
    case IntrinsicOp::Psrldqi128:
    case IntrinsicOp::Psllqi128:
    case IntrinsicOp::Psrlqi128:
    case IntrinsicOp::Pshufd128:
    case IntrinsicOp::Loadldi128:
    case IntrinsicOp::Paddw128:
    case IntrinsicOp::Psubw128:
    case IntrinsicOp::Pmulhw128:
    case IntrinsicOp::Pmaddwd128:
    case IntrinsicOp::Pcmpgtw128:
    case IntrinsicOp::Pcmpgtb128:
    case IntrinsicOp::Paddd128:
    case IntrinsicOp::Psubd128:
    case IntrinsicOp::Packssdw128:
    case IntrinsicOp::Packsswb128:
    case IntrinsicOp::Packuswb128:
    case IntrinsicOp::Punpcklbw128:
    case IntrinsicOp::Punpckhbw128:
    case IntrinsicOp::Punpcklwd128:
    case IntrinsicOp::Punpckhwd128:
    case IntrinsicOp::Psllwi128:
    case IntrinsicOp::Psrlwi128:
    case IntrinsicOp::Psrawi128:
    case IntrinsicOp::Psradi128:
    case IntrinsicOp::Pslldi128:
    case IntrinsicOp::Psrldi128:
    case IntrinsicOp::SetEpi16:
    case IntrinsicOp::Pinsrw128:
    case IntrinsicOp::Pextrw128:
    case IntrinsicOp::Pinsrd128:
    case IntrinsicOp::Pextrd128:
    case IntrinsicOp::Pinsrb128:
    case IntrinsicOp::Pextrb128:
    case IntrinsicOp::Pinsrq128:
    case IntrinsicOp::Pextrq128:
    case IntrinsicOp::Storeldi128:
    case IntrinsicOp::Cvtsi128Si32:
    case IntrinsicOp::Cvtsi32Si128:
    case IntrinsicOp::Cvtsi128Si64:
    case IntrinsicOp::Pshuflw128:
    case IntrinsicOp::Pshufhw128:
      zero_vector_dest();
      return;
    default:
      return;
  }
}

// Source-level mirrors of the Rust impl methods:
//
// pub(super) fn emit_intrinsic_rv(...)
// pub(super) fn emit_rv_binary_128(...)
// pub(super) fn emit_rv_cmpeq_bytes(...)
// pub(super) fn emit_rv_zero_byte_mask(...)
// pub(super) fn emit_rv_cmpeq_dwords(...)
// pub(super) fn emit_rv_binary_128_bytewise(...)
// pub(super) fn emit_rv_psubusb_8bytes(...)
// pub(super) fn emit_rv_psubsb_128(...)
// pub(super) fn emit_rv_psubsb_8bytes(...)
// pub(super) fn emit_rv_pmovmskb(...)
//
// The dispatch layer itself remains parked even though the shared
// `RiscvCodegen`, `Operand`, `Value`, and `IntrinsicOp` surfaces now exist in
// the common header. The methods above preserve the translated helper logic
// without claiming the direct `emit_intrinsic_rv(...)` owner packet yet.

}  // namespace c4c::backend::riscv::codegen

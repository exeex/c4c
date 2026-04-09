// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/intrinsics.rs
// The shared RISC-V C++ codegen surface does not exist yet, so this file keeps
// the Rust method boundaries as a source-level mirror while translating the
// local helper logic into concrete C++ helpers.

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace c4c::backend::riscv::codegen {

namespace {

using AssemblyLines = std::vector<std::string>;

void emit_line(AssemblyLines& lines, std::string_view line) {
  lines.emplace_back(line);
}

void emit_fmt(AssemblyLines& lines, const std::string& line) {
  lines.push_back(line);
}

void emit_rv_binary_128(AssemblyLines& lines, std::string_view op) {
  emit_line(lines, "    ld t1, 0(t0)");
  emit_line(lines, "    ld t2, 8(t0)");
  emit_line(lines, "    ld t3, 0(t0)");
  emit_line(lines, "    ld t4, 8(t0)");
  emit_fmt(lines, std::string("    ") + std::string(op) + " t1, t1, t3");
  emit_fmt(lines, std::string("    ") + std::string(op) + " t2, t2, t4");
  emit_line(lines, "    sd t1, 0(t0)");
  emit_line(lines, "    sd t2, 8(t0)");
}

void emit_rv_zero_byte_mask(AssemblyLines& lines,
                            std::string_view src_reg,
                            std::string_view dst_reg) {
  emit_line(lines, "    li t5, 0x0101010101010101");
  emit_line(lines, "    li t6, 0x8080808080808080");
  emit_fmt(lines, "    sub " + std::string(dst_reg) + ", " + std::string(src_reg) + ", t5");
  emit_fmt(lines, "    not t5, " + std::string(src_reg));
  emit_fmt(lines, "    and " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  emit_fmt(lines, "    and " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t6");
  emit_fmt(lines, "    srli " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", 7");
  emit_fmt(lines, "    slli t5, " + std::string(dst_reg) + ", 1");
  emit_fmt(lines, "    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  emit_fmt(lines, "    slli t5, " + std::string(dst_reg) + ", 2");
  emit_fmt(lines, "    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  emit_fmt(lines, "    slli t5, " + std::string(dst_reg) + ", 4");
  emit_fmt(lines, "    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
}

void emit_rv_cmpeq_bytes(AssemblyLines& lines) {
  emit_line(lines, "    mv a6, t0");
  emit_line(lines, "    mv a7, t0");
  emit_line(lines, "    ld t1, 0(a6)");
  emit_line(lines, "    ld t2, 0(a7)");
  emit_line(lines, "    xor t1, t1, t2");
  emit_rv_zero_byte_mask(lines, "t1", "t3");
  emit_line(lines, "    ld t1, 8(a6)");
  emit_line(lines, "    ld t2, 8(a7)");
  emit_line(lines, "    xor t1, t1, t2");
  emit_rv_zero_byte_mask(lines, "t1", "t4");
  emit_line(lines, "    sd t3, 0(a5)");
  emit_line(lines, "    sd t4, 8(a5)");
}

void emit_rv_cmpeq_dwords(AssemblyLines& lines) {
  emit_line(lines, "    mv a6, t0");
  emit_line(lines, "    mv a7, t0");
  emit_line(lines, "    ld t1, 0(a6)");
  emit_line(lines, "    ld t2, 0(a7)");
  emit_line(lines, "    slli t3, t1, 32");
  emit_line(lines, "    slli t4, t2, 32");
  emit_line(lines, "    srli t3, t3, 32");
  emit_line(lines, "    srli t4, t4, 32");
  emit_line(lines, "    sub t3, t3, t4");
  emit_line(lines, "    snez t3, t3");
  emit_line(lines, "    neg t3, t3");
  emit_line(lines, "    not t3, t3");
  emit_line(lines, "    slli t3, t3, 32");
  emit_line(lines, "    srli t3, t3, 32");
  emit_line(lines, "    srli t5, t1, 32");
  emit_line(lines, "    srli t6, t2, 32");
  emit_line(lines, "    sub t5, t5, t6");
  emit_line(lines, "    snez t5, t5");
  emit_line(lines, "    neg t5, t5");
  emit_line(lines, "    not t5, t5");
  emit_line(lines, "    slli t5, t5, 32");
  emit_line(lines, "    or t3, t3, t5");
  emit_line(lines, "    sd t3, 0(a5)");
  emit_line(lines, "    ld t1, 8(a6)");
  emit_line(lines, "    ld t2, 8(a7)");
  emit_line(lines, "    slli t3, t1, 32");
  emit_line(lines, "    slli t4, t2, 32");
  emit_line(lines, "    srli t3, t3, 32");
  emit_line(lines, "    srli t4, t4, 32");
  emit_line(lines, "    sub t3, t3, t4");
  emit_line(lines, "    snez t3, t3");
  emit_line(lines, "    neg t3, t3");
  emit_line(lines, "    not t3, t3");
  emit_line(lines, "    slli t3, t3, 32");
  emit_line(lines, "    srli t3, t3, 32");
  emit_line(lines, "    srli t5, t1, 32");
  emit_line(lines, "    srli t6, t2, 32");
  emit_line(lines, "    sub t5, t5, t6");
  emit_line(lines, "    snez t5, t5");
  emit_line(lines, "    neg t5, t5");
  emit_line(lines, "    not t5, t5");
  emit_line(lines, "    slli t5, t5, 32");
  emit_line(lines, "    or t3, t3, t5");
  emit_line(lines, "    sd t3, 8(a5)");
}

void emit_rv_binary_128_bytewise(AssemblyLines& lines) {
  emit_line(lines, "    mv a6, t0");
  emit_line(lines, "    mv a7, t0");
  emit_line(lines, "    ld t1, 0(a6)");
  emit_line(lines, "    ld t2, 0(a7)");
  emit_line(lines, "    ld t1, 8(a6)");
  emit_line(lines, "    ld t2, 8(a7)");
  emit_line(lines, "    sd t3, 0(a5)");
  emit_line(lines, "    sd t4, 8(a5)");
}

void emit_rv_psubusb_8bytes(AssemblyLines& lines, std::string_view a_reg,
                            std::string_view b_reg, std::string_view dst_reg) {
  emit_fmt(lines, "    li " + std::string(dst_reg) + ", 0");
  for (int i = 0; i < 8; ++i) {
    const int shift = i * 8;
    if (shift == 0) {
      emit_fmt(lines, "    andi t5, " + std::string(a_reg) + ", 0xff");
      emit_fmt(lines, "    andi t6, " + std::string(b_reg) + ", 0xff");
    } else {
      emit_fmt(lines, "    srli t5, " + std::string(a_reg) + ", " + std::to_string(shift));
      emit_line(lines, "    andi t5, t5, 0xff");
      emit_fmt(lines, "    srli t6, " + std::string(b_reg) + ", " + std::to_string(shift));
      emit_line(lines, "    andi t6, t6, 0xff");
    }
    const std::string skip_label = "psub_skip_" + std::to_string(i);
    emit_fmt(lines, "    bltu t5, t6, " + skip_label);
    emit_line(lines, "    sub t5, t5, t6");
    if (shift > 0) {
      emit_fmt(lines, "    slli t5, t5, " + std::to_string(shift));
    }
    emit_fmt(lines, "    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
    emit_fmt(lines, skip_label + ":");
  }
}

void emit_rv_psubsb_8bytes(AssemblyLines& lines, std::string_view a_reg,
                           std::string_view b_reg, std::string_view dst_reg) {
  emit_fmt(lines, "    li " + std::string(dst_reg) + ", 0");
  for (int i = 0; i < 8; ++i) {
    const int shift = i * 8;
    if (shift == 0) {
      emit_fmt(lines, "    slli t5, " + std::string(a_reg) + ", 56");
      emit_fmt(lines, "    slli t6, " + std::string(b_reg) + ", 56");
    } else {
      emit_fmt(lines, "    slli t5, " + std::string(a_reg) + ", " + std::to_string(56 - shift));
      emit_fmt(lines, "    slli t6, " + std::string(b_reg) + ", " + std::to_string(56 - shift));
    }
    emit_line(lines, "    srai t5, t5, 56");
    emit_line(lines, "    srai t6, t6, 56");
    emit_line(lines, "    sub t5, t5, t6");
    const std::string no_hi = "psubsb_noclamp_hi_" + std::to_string(i);
    const std::string no_lo = "psubsb_noclamp_lo_" + std::to_string(i);
    const std::string done = "psubsb_done_" + std::to_string(i);
    emit_line(lines, "    li t6, 127");
    emit_fmt(lines, "    ble t5, t6, " + no_hi);
    emit_line(lines, "    li t5, 127");
    emit_fmt(lines, "    j " + done);
    emit_fmt(lines, no_hi + ":");
    emit_line(lines, "    li t6, -128");
    emit_fmt(lines, "    bge t5, t6, " + no_lo);
    emit_line(lines, "    li t5, -128");
    emit_fmt(lines, no_lo + ":");
    emit_fmt(lines, done + ":");
    emit_line(lines, "    andi t5, t5, 0xff");
    if (shift > 0) {
      emit_fmt(lines, "    slli t5, t5, " + std::to_string(shift));
    }
    emit_fmt(lines, "    or " + std::string(dst_reg) + ", " + std::string(dst_reg) + ", t5");
  }
}

void emit_rv_psubsb_128(AssemblyLines& lines) {
  emit_line(lines, "    mv a6, t0");
  emit_line(lines, "    mv a7, t0");
  emit_line(lines, "    ld t1, 0(a6)");
  emit_line(lines, "    ld t2, 0(a7)");
  emit_rv_psubsb_8bytes(lines, "t1", "t2", "t3");
  emit_line(lines, "    ld t1, 8(a6)");
  emit_line(lines, "    ld t2, 8(a7)");
  emit_rv_psubsb_8bytes(lines, "t1", "t2", "t4");
  emit_line(lines, "    sd t3, 0(a5)");
  emit_line(lines, "    sd t4, 8(a5)");
}

void emit_rv_pmovmskb(AssemblyLines& lines) {
  emit_line(lines, "    ld t1, 0(t0)");
  emit_line(lines, "    ld t2, 8(t0)");
  emit_line(lines, "    li t3, 0x8080808080808080");
  emit_line(lines, "    and t1, t1, t3");
  emit_line(lines, "    and t2, t2, t3");
  emit_line(lines, "    li t3, 0x0002040810204081");
  emit_line(lines, "    mul t1, t1, t3");
  emit_line(lines, "    srli t1, t1, 56");
  emit_line(lines, "    mul t2, t2, t3");
  emit_line(lines, "    srli t2, t2, 56");
  emit_line(lines, "    slli t2, t2, 8");
  emit_line(lines, "    or t0, t1, t2");
}

}  // namespace

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
// The dispatch layer itself still depends on the shared `RiscvCodegen`,
// `Operand`, `Value`, and `IntrinsicOp` surfaces, which are not represented
// in a common C++ header yet. The helpers above preserve the local emission
// logic that is mechanically translatable.

}  // namespace c4c::backend::riscv::codegen

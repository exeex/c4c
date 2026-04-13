// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/returns.rs
// Structural mirror of the ref Rust source. This slice makes the bounded
// helper-level return-path surface real without claiming the full RV64 owner
// body is wired yet.

#include "riscv_codegen.hpp"

#include <cstring>
#include <string>

namespace c4c::backend::riscv::codegen {

namespace {

template <typename IntT, typename FloatT>
IntT bit_pattern(FloatT value) {
  static_assert(sizeof(IntT) == sizeof(FloatT));
  IntT bits = 0;
  std::memcpy(&bits, &value, sizeof(bits));
  return bits;
}

std::string format_s0_mem(const char* instr, const char* reg, std::int64_t offset) {
  return std::string(instr) + " " + reg + ", " + std::to_string(offset) + "(s0)";
}

}  // namespace

void RiscvCodegenState::emit(std::string line) { asm_lines.push_back(line); }

std::optional<StackSlot> RiscvCodegenState::get_slot(int value_id) const {
  const auto it = slots.find(value_id);
  if (it == slots.end()) {
    return std::nullopt;
  }
  return it->second;
}

void RiscvCodegenState::mark_needs_got_for_addr(std::string name) {
  got_addr_symbols.insert(std::move(name));
}

bool RiscvCodegenState::needs_got_for_addr(std::string_view name) const {
  return got_addr_symbols.find(std::string(name)) != got_addr_symbols.end();
}

void RiscvCodegen::emit_store_to_s0(const char* reg, std::int64_t offset, const char* instr) {
  state.emit("    " + format_s0_mem(instr, reg, offset));
}

void RiscvCodegen::emit_load_from_s0(const char* reg, std::int64_t offset, const char* instr) {
  state.emit("    " + format_s0_mem(instr, reg, offset));
}

void RiscvCodegen::operand_to_t0(const Operand& src) {
  switch (src.kind) {
    case Operand::Kind::Value:
      if (const auto slot = state.get_slot(src.raw)) {
        emit_load_from_s0("t0", slot->raw, "ld");
      }
      return;
    case Operand::Kind::ImmI64:
      state.emit("    li t0, " + std::to_string(src.imm_i64));
      return;
    case Operand::Kind::F32Const:
      state.emit("    li t0, " +
                 std::to_string(static_cast<std::int64_t>(bit_pattern<std::uint32_t>(src.f32))));
      return;
    case Operand::Kind::F64Const:
      state.emit("    li t0, " +
                 std::to_string(static_cast<std::int64_t>(bit_pattern<std::uint64_t>(src.f64))));
      return;
  }
}

void RiscvCodegen::store_t0_to(const Value& dest) {
  if (const auto reg = state.assigned_reg(dest.raw)) {
    state.emit("    mv " + std::string(callee_saved_name(*reg)) + ", t0");
    return;
  }

  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("t0", slot->raw, "sd");
  }
}

void RiscvCodegen::emit_return_impl(const std::optional<Operand>& value, std::int64_t frame_size) {
  if (value.has_value() && current_return_type == IrType::F128) {
    emit_f128_operand_to_a0_a1(*value);
    emit_epilogue_and_ret_impl(frame_size);
    return;
  }

  c4c::backend::emit_riscv_return_default(*this, value ? &value.value() : nullptr, frame_size);
}

void RiscvCodegen::emit_return_i128_to_regs_impl() {
  state.emit("    mv a0, t0");
  state.emit("    mv a1, t1");
}

void RiscvCodegen::emit_return_f128_to_reg_impl() {
  state.emit("    fmv.d.x fa0, t0");
  state.emit("    call __extenddftf2");
}

void RiscvCodegen::emit_return_f32_to_reg_impl() { state.emit("    fmv.w.x fa0, t0"); }

void RiscvCodegen::emit_return_f64_to_reg_impl() { state.emit("    fmv.d.x fa0, t0"); }

void RiscvCodegen::emit_return_int_to_reg_impl() { state.emit("    mv a0, t0"); }

void RiscvCodegen::emit_get_return_f64_second_impl(const Value& dest) {
  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("fa1", slot->raw, "fsd");
  }
}

void RiscvCodegen::emit_set_return_f64_second_impl(const Operand& src) {
  switch (src.kind) {
    case Operand::Kind::Value:
      if (const auto slot = state.get_slot(src.raw)) {
        emit_load_from_s0("fa1", slot->raw, "fld");
      }
      return;
    case Operand::Kind::F64Const:
      state.emit(
          "    li t0, " +
          std::to_string(static_cast<std::int64_t>(bit_pattern<std::uint64_t>(src.f64))));
      state.emit("    fmv.d.x fa1, t0");
      return;
    case Operand::Kind::ImmI64:
    case Operand::Kind::F32Const:
      operand_to_t0(src);
      state.emit("    fmv.d.x fa1, t0");
      return;
  }
}

void RiscvCodegen::emit_get_return_f32_second_impl(const Value& dest) {
  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("fa1", slot->raw, "fsw");
  }
}

void RiscvCodegen::emit_set_return_f32_second_impl(const Operand& src) {
  switch (src.kind) {
    case Operand::Kind::Value:
      if (const auto slot = state.get_slot(src.raw)) {
        emit_load_from_s0("fa1", slot->raw, "flw");
      }
      return;
    case Operand::Kind::F32Const:
      state.emit(
          "    li t0, " +
          std::to_string(static_cast<std::int64_t>(bit_pattern<std::uint32_t>(src.f32))));
      state.emit("    fmv.w.x fa1, t0");
      return;
    case Operand::Kind::ImmI64:
    case Operand::Kind::F64Const:
      operand_to_t0(src);
      state.emit("    fmv.w.x fa1, t0");
      return;
  }
}

}  // namespace c4c::backend::riscv::codegen

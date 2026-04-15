// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/variadic.rs
// This slice makes the bounded variadic owner bodies real through the landed
// shared RV64 header and prologue-owned setup surface.

#include "riscv_codegen.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace c4c::backend::riscv::codegen {

namespace {

constexpr std::size_t kRiscvVaListSlotBytes = 8;
constexpr std::size_t kRiscvVaListLongDoubleBytes = 16;

std::int64_t riscv_va_start_offset(std::size_t va_named_gp_count,
                                   std::size_t va_named_stack_bytes) {
  if (va_named_gp_count >= 8) {
    return 64 + static_cast<std::int64_t>(va_named_stack_bytes);
  }
  return static_cast<std::int64_t>(va_named_gp_count) * static_cast<std::int64_t>(kRiscvVaListSlotBytes);
}

std::size_t riscv_va_arg_step_bytes(bool is_long_double) {
  return is_long_double ? kRiscvVaListLongDoubleBytes : kRiscvVaListSlotBytes;
}

constexpr bool fits_imm12(std::int64_t value) { return value >= -2048 && value <= 2047; }

void emit_add_s0_to_reg(RiscvCodegenState& state, const char* reg, std::int64_t offset) {
  if (fits_imm12(offset)) {
    state.emit("    addi " + std::string(reg) + ", s0, " + std::to_string(offset));
    return;
  }

  state.emit("    li t6, " + std::to_string(offset));
  state.emit("    add " + std::string(reg) + ", s0, t6");
}

bool emit_value_address_or_load(RiscvCodegenState& state, const Value& value, const char* reg) {
  if (state.is_alloca(value.raw)) {
    if (const auto slot = state.get_slot(value.raw)) {
      emit_add_s0_to_reg(state, reg, slot->raw);
      return true;
    }
  }

  if (const auto assigned = state.assigned_reg(value.raw)) {
    state.emit("    mv " + std::string(reg) + ", " + callee_saved_name(*assigned));
    return true;
  }

  if (const auto slot = state.get_slot(value.raw)) {
    state.emit("    ld " + std::string(reg) + ", " + std::to_string(slot->raw) + "(s0)");
    return true;
  }

  return false;
}

}  // namespace

void RiscvCodegen::emit_va_arg_impl(const Value& dest, const Value& va_list_ptr, IrType result_ty) {
  if (!emit_value_address_or_load(state, va_list_ptr, "t1")) {
    return;
  }

  state.emit("    ld t2, 0(t1)");
  if (result_ty == IrType::F128) {
    state.emit("    addi t2, t2, 15");
    state.emit("    andi t2, t2, -16");
    state.emit("    ld a0, 0(t2)");
    state.emit("    ld a1, 8(t2)");
    state.emit("    addi t2, t2, 16");
    state.emit("    sd t2, 0(t1)");
    state.emit("    call __trunctfdf2");
    state.emit("    fmv.x.d t0, fa0");
  } else {
    state.emit("    ld t0, 0(t2)");
    state.emit("    addi t2, t2, " +
               std::to_string(static_cast<std::int64_t>(riscv_va_arg_step_bytes(false))));
    state.emit("    sd t2, 0(t1)");
  }

  store_t0_to(dest);
}

void RiscvCodegen::emit_va_start_impl(const Value& va_list_ptr) {
  if (!emit_value_address_or_load(state, va_list_ptr, "t0")) {
    return;
  }

  emit_add_s0_to_reg(state, "t1", riscv_va_start_offset(va_named_gp_count, va_named_stack_bytes));
  state.emit("    sd t1, 0(t0)");
}

void RiscvCodegen::emit_va_copy_impl(const Value& dest_ptr, const Value& src_ptr) {
  if (!emit_value_address_or_load(state, src_ptr, "t1")) {
    return;
  }
  state.emit("    ld t2, 0(t1)");

  if (!emit_value_address_or_load(state, dest_ptr, "t0")) {
    return;
  }
  state.emit("    sd t2, 0(t0)");
}

}  // namespace c4c::backend::riscv::codegen

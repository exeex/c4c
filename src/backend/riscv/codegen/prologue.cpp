// Translated from /workspaces/c4c/ref/claudes-c-compiler/src/backend/riscv/codegen/prologue.rs
// Structural mirror of the ref Rust source. This slice makes only the
// prologue-owned return-state and return-exit seam real enough for later
// returns work; broader stack sizing and parameter storage remain parked.

#include "riscv_codegen.hpp"

#include <string>

namespace c4c::backend::riscv::codegen {

std::int64_t RiscvCodegen::aligned_frame_size_impl(std::int64_t raw_space) const {
  return (raw_space + 15) & ~std::int64_t{15};
}

void RiscvCodegen::emit_prologue_impl(const IrFunction& func, std::int64_t frame_size) {
  is_variadic = func.is_variadic;
  current_return_type = func.return_type;
  current_frame_size = frame_size;
  variadic_save_area_bytes = func.is_variadic ? riscv_variadic_reg_save_area_size() : 0;
  va_named_gp_count = func.is_variadic ? riscv_variadic_named_gp_count(func.va_named_gp_count) : 0;
  va_named_stack_bytes = func.is_variadic ? func.va_named_stack_bytes : 0;

  const auto total_alloc = frame_size + variadic_save_area_bytes;
  if (total_alloc <= 0) {
    return;
  }

  state.emit("    addi sp, sp, -" + std::to_string(total_alloc));
  state.emit("    sd ra, " + std::to_string(frame_size - 8) + "(sp)");
  state.emit("    sd s0, " + std::to_string(frame_size - 16) + "(sp)");
  state.emit("    addi s0, sp, " + std::to_string(frame_size));

  if (is_variadic) {
    for (std::size_t index = 0; index < RISCV_ARG_REGS.size(); ++index) {
      state.emit("    sd " + std::string(RISCV_ARG_REGS[index]) + ", " +
                 std::to_string(riscv_variadic_gp_save_offset(index)) + "(s0)");
    }
  }

  for (std::size_t index = 0; index < used_callee_saved.size(); ++index) {
    const auto reg_name = callee_saved_name(used_callee_saved[index]);
    const auto offset = -frame_size + static_cast<std::int64_t>(index) * 8;
    emit_store_to_s0(reg_name, offset, "sd");
  }
}

void RiscvCodegen::emit_epilogue_impl(std::int64_t frame_size) {
  const auto effective_frame_size = frame_size > 0 ? frame_size : current_frame_size;
  const auto total_alloc = effective_frame_size + variadic_save_area_bytes;
  if (total_alloc <= 0) {
    return;
  }

  for (std::size_t index = 0; index < used_callee_saved.size(); ++index) {
    const auto reg_name = callee_saved_name(used_callee_saved[index]);
    const auto offset = -effective_frame_size + static_cast<std::int64_t>(index) * 8;
    emit_load_from_s0(reg_name, offset, "ld");
  }

  state.emit("    ld ra, -8(s0)");
  state.emit("    ld s0, -16(s0)");
  state.emit("    addi sp, sp, " + std::to_string(total_alloc));
}

void RiscvCodegen::emit_epilogue_and_ret_impl(std::int64_t frame_size) {
  emit_epilogue_impl(frame_size);
  state.emit("    ret");
}

IrType RiscvCodegen::current_return_type_impl() const { return current_return_type; }

}  // namespace c4c::backend::riscv::codegen

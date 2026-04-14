#include <cstddef>
#include <string>
#include "riscv_codegen.hpp"

namespace c4c::backend::riscv::codegen {

namespace {

inline constexpr std::array<const char*, 8> RISCV_FLOAT_ARG_REGS = {
    "fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",
};

std::size_t compute_stack_arg_space(const std::vector<CallArgClass>& arg_classes) {
  std::size_t total = 0;
  for (const auto& cls : arg_classes) {
    if (!cls.is_stack()) {
      continue;
    }
    if (std::holds_alternative<CallArgClass::F128Stack>(cls.value) ||
        std::holds_alternative<CallArgClass::I128Stack>(cls.value)) {
      total = (total + 15) & ~std::size_t(15);
    }
    total += cls.stack_bytes();
  }
  return (total + 15) & ~std::size_t(15);
}

void align_wide_stack_offset(std::size_t& offset) { offset = (offset + 15) & ~std::size_t(15); }

void emit_addi_sp(RiscvCodegenState& state, std::int64_t amount) {
  if (amount >= -2048 && amount <= 2047) {
    state.emit("    addi sp, sp, " + std::to_string(amount));
    return;
  }

  state.emit("    li t6, " + std::to_string(amount));
  state.emit("    add sp, sp, t6");
}

void emit_store_to_sp(RiscvCodegenState& state, const char* reg, std::int64_t offset, const char* instr) {
  state.emit("    " + std::string(instr) + " " + reg + ", " + std::to_string(offset) + "(sp)");
}

void emit_load_from_sp(RiscvCodegenState& state, const char* reg, std::int64_t offset, const char* instr) {
  state.emit("    " + std::string(instr) + " " + reg + ", " + std::to_string(offset) + "(sp)");
}

void emit_addi_from_s0(RiscvCodegenState& state, const char* reg, std::int64_t offset) {
  if (offset == 0) {
    state.emit("    mv " + std::string(reg) + ", s0");
    return;
  }

  if (offset >= -2048 && offset <= 2047) {
    state.emit("    addi " + std::string(reg) + ", s0, " + std::to_string(offset));
    return;
  }

  state.emit("    li " + std::string(reg) + ", " + std::to_string(offset));
  state.emit("    add " + std::string(reg) + ", s0, " + reg);
}

void emit_load_from_reg(RiscvCodegenState& state, const char* reg, const char* base, std::int64_t offset, const char* instr) {
  state.emit("    " + std::string(instr) + " " + reg + ", " + std::to_string(offset) + "(" + base + ")");
}

}  // namespace

CallAbiConfig RiscvCodegen::call_abi_config_impl() const {
  return CallAbiConfig{
      .max_int_regs = 8,
      .max_float_regs = 8,
      .align_i128_pairs = true,
      .f128_in_fp_regs = false,
      .f128_in_gp_pairs = true,
      .variadic_floats_in_gp = true,
      .large_struct_by_ref = true,
      .use_sysv_struct_classification = false,
      .use_riscv_float_struct_classification = true,
      .allow_struct_split_reg_stack = true,
      .align_struct_pairs = true,
      .sret_uses_dedicated_reg = false,
  };
}

std::size_t RiscvCodegen::emit_call_compute_stack_space_impl(
    const std::vector<CallArgClass>& arg_classes,
    const std::vector<IrType>& /*arg_types*/) const {
  return compute_stack_arg_space(arg_classes);
}

std::size_t RiscvCodegen::emit_call_f128_pre_convert_impl(
    const std::vector<Operand>& args,
    const std::vector<CallArgClass>& arg_classes,
    const std::vector<IrType>& /*arg_types*/,
    std::size_t /*stack_arg_space*/) {
  std::size_t f128_temp_space = 0;
  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    if (!std::holds_alternative<CallArgClass::F128Reg>(arg_classes[i].value)) {
      continue;
    }
    if (args[i].kind == Operand::Kind::Value) {
      f128_temp_space += 16;
    }
  }

  if (f128_temp_space == 0) {
    return 0;
  }

  emit_addi_sp(state, -static_cast<std::int64_t>(f128_temp_space));

  std::int64_t temp_offset = 0;
  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    if (!std::holds_alternative<CallArgClass::F128Reg>(arg_classes[i].value) ||
        args[i].kind != Operand::Kind::Value) {
      continue;
    }
    emit_f128_operand_to_a0_a1(args[i]);
    emit_store_to_sp(state, "a0", temp_offset, "sd");
    emit_store_to_sp(state, "a1", temp_offset + 8, "sd");
    temp_offset += 16;
  }

  return f128_temp_space;
}

std::int64_t RiscvCodegen::emit_call_stack_args_impl(const std::vector<Operand>& args,
                                                     const std::vector<CallArgClass>& arg_classes,
                                                     const std::vector<IrType>& /*arg_types*/,
                                                     std::size_t stack_arg_space,
                                                     std::size_t /*fptr_spill*/,
                                                     std::size_t /*f128_temp_space*/) {
  if (stack_arg_space == 0) {
    return 0;
  }

  emit_addi_sp(state, -static_cast<std::int64_t>(stack_arg_space));

  const auto materialize_aggregate_base_in_t0 = [&](const Operand& arg) {
    if (arg.kind != Operand::Kind::Value) {
      operand_to_t0(arg);
      return;
    }

    if (const auto reg = state.assigned_reg(arg.raw)) {
      state.emit("    mv t0, " + std::string(callee_saved_name(*reg)));
      return;
    }

    if (const auto slot = state.get_slot(arg.raw)) {
      if (state.is_alloca(arg.raw)) {
        emit_addi_from_s0(state, "t0", slot->raw);
      } else {
        emit_load_from_s0("t0", slot->raw, "ld");
      }
      return;
    }

    state.emit("    li t0, 0");
  };

  std::size_t offset = 0;
  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    if (const auto* byval_stack = std::get_if<CallArgClass::StructByValStack>(&arg_classes[i].value)) {
      materialize_aggregate_base_in_t0(args[i]);
      const auto n_dwords = (byval_stack->size + 7) / 8;
      for (std::size_t qi = 0; qi < n_dwords; ++qi) {
        const auto src_off = static_cast<std::int64_t>(qi * 8);
        emit_load_from_reg(state, "t1", "t0", src_off, "ld");
        emit_store_to_sp(state, "t1", static_cast<std::int64_t>(offset) + src_off, "sd");
      }
      offset += n_dwords * 8;
      continue;
    }

    if (const auto* large_stack = std::get_if<CallArgClass::LargeStructStack>(&arg_classes[i].value)) {
      materialize_aggregate_base_in_t0(args[i]);
      const auto n_dwords = (large_stack->size + 7) / 8;
      for (std::size_t qi = 0; qi < n_dwords; ++qi) {
        const auto src_off = static_cast<std::int64_t>(qi * 8);
        emit_load_from_reg(state, "t1", "t0", src_off, "ld");
        emit_store_to_sp(state, "t1", static_cast<std::int64_t>(offset) + src_off, "sd");
      }
      offset += n_dwords * 8;
      continue;
    }

    if (const auto* split_stack = std::get_if<CallArgClass::StructSplitRegStack>(&arg_classes[i].value)) {
      materialize_aggregate_base_in_t0(args[i]);
      const auto stack_bytes = split_stack->size > 8 ? split_stack->size - 8 : 0;
      const auto stack_dwords = (stack_bytes + 7) / 8;
      for (std::size_t qi = 0; qi < stack_dwords; ++qi) {
        const auto src_off = static_cast<std::int64_t>(8 + qi * 8);
        emit_load_from_reg(state, "t1", "t0", src_off, "ld");
        emit_store_to_sp(state, "t1", static_cast<std::int64_t>(offset + qi * 8), "sd");
      }
      offset += stack_dwords * 8;
      continue;
    }

    if (std::holds_alternative<CallArgClass::I128Stack>(arg_classes[i].value)) {
      align_wide_stack_offset(offset);
      emit_load_acc_pair_impl(args[i]);
      emit_store_to_sp(state, "t0", static_cast<std::int64_t>(offset), "sd");
      emit_store_to_sp(state, "t1", static_cast<std::int64_t>(offset + 8), "sd");
      offset += 16;
      continue;
    }

    if (std::holds_alternative<CallArgClass::F128Stack>(arg_classes[i].value)) {
      align_wide_stack_offset(offset);
      emit_f128_operand_to_a0_a1(args[i]);
      emit_store_to_sp(state, "a0", static_cast<std::int64_t>(offset), "sd");
      emit_store_to_sp(state, "a1", static_cast<std::int64_t>(offset + 8), "sd");
      offset += 16;
      continue;
    }

    if (std::holds_alternative<CallArgClass::Stack>(arg_classes[i].value)) {
      operand_to_t0(args[i]);
      emit_store_to_sp(state, "t0", static_cast<std::int64_t>(offset), "sd");
      offset += 8;
    }
  }

  return 0;
}

void RiscvCodegen::emit_call_reg_args_impl(
    const std::vector<Operand>& args,
    const std::vector<CallArgClass>& arg_classes,
    const std::vector<IrType>& arg_types,
    std::int64_t /*total_sp_adjust*/,
    std::size_t /*f128_temp_space*/,
    std::size_t stack_arg_space,
    const std::vector<std::optional<RiscvFloatClass>>& classes) {
  constexpr std::array<const char*, 3> temp_regs = {"t3", "t4", "t5"};

  const auto materialize_aggregate_base_in_t0 = [&](const Operand& arg) {
    if (arg.kind != Operand::Kind::Value) {
      operand_to_t0(arg);
      return;
    }

    if (const auto reg = state.assigned_reg(arg.raw)) {
      state.emit("    mv t0, " + std::string(callee_saved_name(*reg)));
      return;
    }

    if (const auto slot = state.get_slot(arg.raw)) {
      if (state.is_alloca(arg.raw)) {
        emit_addi_from_s0(state, "t0", slot->raw);
      } else {
        emit_load_from_s0("t0", slot->raw, "ld");
      }
      return;
    }

    state.emit("    li t0, 0");
  };

  const auto emit_float_struct_load = [&](std::size_t fp_idx, std::int64_t offset, bool is_double) {
    state.emit("    " + std::string(is_double ? "fld " : "flw ") + RISCV_FLOAT_ARG_REGS[fp_idx] +
               ", " + std::to_string(offset) + "(t0)");
  };

  std::vector<std::pair<std::size_t, const char*>> staged_gp;
  std::vector<std::pair<std::size_t, std::size_t>> deferred_gp;
  std::size_t temp_i = 0;

  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    if (const auto* int_reg = std::get_if<CallArgClass::IntReg>(&arg_classes[i].value)) {
      if (temp_i < temp_regs.size()) {
        operand_to_t0(args[i]);
        state.emit("    mv " + std::string(temp_regs[temp_i]) + ", t0");
        staged_gp.emplace_back(int_reg->reg_idx, temp_regs[temp_i]);
        ++temp_i;
      } else {
        deferred_gp.emplace_back(int_reg->reg_idx, i);
      }
      continue;
    }

    if (const auto* float_reg = std::get_if<CallArgClass::FloatReg>(&arg_classes[i].value)) {
      operand_to_t0(args[i]);
      const auto arg_ty = i < arg_types.size() ? arg_types[i] : IrType::F64;
      const char* float_reg_name = RISCV_FLOAT_ARG_REGS[float_reg->reg_idx];
      if (arg_ty == IrType::F32) {
        state.emit("    fmv.w.x " + std::string(float_reg_name) + ", t0");
      } else {
        state.emit("    fmv.d.x " + std::string(float_reg_name) + ", t0");
      }
    }
  }

  for (const auto& [target_idx, temp_reg] : staged_gp) {
    state.emit("    mv " + std::string(RISCV_ARG_REGS[target_idx]) + ", " + temp_reg);
  }

  for (const auto& [target_idx, arg_idx] : deferred_gp) {
    operand_to_t0(args[arg_idx]);
    state.emit("    mv " + std::string(RISCV_ARG_REGS[target_idx]) + ", t0");
  }

  std::int64_t f128_var_temp_offset = 0;
  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    const auto* f128_reg = std::get_if<CallArgClass::F128Reg>(&arg_classes[i].value);
    if (f128_reg == nullptr) {
      continue;
    }

    if (args[i].kind == Operand::Kind::Value) {
      const auto offset = static_cast<std::int64_t>(stack_arg_space) + f128_var_temp_offset;
      emit_load_from_sp(state, RISCV_ARG_REGS[f128_reg->reg_idx], offset, "ld");
      emit_load_from_sp(state, RISCV_ARG_REGS[f128_reg->reg_idx + 1], offset + 8, "ld");
      f128_var_temp_offset += 16;
      continue;
    }

    emit_f128_operand_to_a0_a1(args[i]);
    if (f128_reg->reg_idx != 0) {
      state.emit("    mv " + std::string(RISCV_ARG_REGS[f128_reg->reg_idx]) + ", a0");
    }
    if (f128_reg->reg_idx + 1 != 1) {
      state.emit("    mv " + std::string(RISCV_ARG_REGS[f128_reg->reg_idx + 1]) + ", a1");
    }
  }

  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    const auto* i128_reg = std::get_if<CallArgClass::I128RegPair>(&arg_classes[i].value);
    if (i128_reg == nullptr) {
      continue;
    }

    emit_load_acc_pair_impl(args[i]);
    state.emit("    mv " + std::string(RISCV_ARG_REGS[i128_reg->base_reg_idx]) + ", t0");
    state.emit("    mv " + std::string(RISCV_ARG_REGS[i128_reg->base_reg_idx + 1]) + ", t1");
  }

  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    const auto* byval_reg = std::get_if<CallArgClass::StructByValReg>(&arg_classes[i].value);
    if (byval_reg == nullptr) {
      continue;
    }

    materialize_aggregate_base_in_t0(args[i]);
    state.emit("    ld " + std::string(RISCV_ARG_REGS[byval_reg->base_reg_idx]) + ", 0(t0)");
    if (byval_reg->size > 8) {
      state.emit("    ld " + std::string(RISCV_ARG_REGS[byval_reg->base_reg_idx + 1]) + ", 8(t0)");
    }
  }

  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    const auto* split_reg = std::get_if<CallArgClass::StructSplitRegStack>(&arg_classes[i].value);
    if (split_reg == nullptr) {
      continue;
    }

    materialize_aggregate_base_in_t0(args[i]);
    state.emit("    ld " + std::string(RISCV_ARG_REGS[split_reg->reg_idx]) + ", 0(t0)");
  }

  for (std::size_t i = 0; i < args.size() && i < arg_classes.size(); ++i) {
    const bool needs_float_ptr =
        std::holds_alternative<CallArgClass::StructSseReg>(arg_classes[i].value) ||
        std::holds_alternative<CallArgClass::StructMixedIntSseReg>(arg_classes[i].value) ||
        std::holds_alternative<CallArgClass::StructMixedSseIntReg>(arg_classes[i].value);
    if (!needs_float_ptr) {
      continue;
    }

    const auto* rv_class = i < classes.size() && classes[i].has_value() ? &*classes[i] : nullptr;
    materialize_aggregate_base_in_t0(args[i]);

    if (const auto* sse_reg = std::get_if<CallArgClass::StructSseReg>(&arg_classes[i].value)) {
      bool lo_is_double = sse_reg->size > 4;
      bool hi_is_double = true;
      if (rv_class != nullptr) {
        if (const auto* one_float = std::get_if<RiscvFloatClass::OneFloat>(&rv_class->value)) {
          lo_is_double = one_float->is_double;
          hi_is_double = false;
        } else if (const auto* two_floats =
                       std::get_if<RiscvFloatClass::TwoFloats>(&rv_class->value)) {
          lo_is_double = two_floats->lo_is_double;
          hi_is_double = two_floats->hi_is_double;
        }
      }

      emit_float_struct_load(sse_reg->lo_fp_idx, 0, lo_is_double);
      if (sse_reg->hi_fp_idx.has_value()) {
        emit_float_struct_load(*sse_reg->hi_fp_idx, lo_is_double ? 8 : 4, hi_is_double);
      }
      continue;
    }

    if (const auto* mixed_int =
            std::get_if<CallArgClass::StructMixedIntSseReg>(&arg_classes[i].value)) {
      bool float_is_double = true;
      std::size_t int_size = 8;
      std::size_t float_offset = 8;
      if (rv_class != nullptr) {
        if (const auto* int_and_float =
                std::get_if<RiscvFloatClass::IntAndFloat>(&rv_class->value)) {
          float_is_double = int_and_float->float_is_double;
          int_size = int_and_float->int_size;
          float_offset = int_and_float->float_offset;
        }
      }

      state.emit("    " + std::string(int_size <= 4 ? "lw " : "ld ") +
                 RISCV_ARG_REGS[mixed_int->int_reg_idx] + ", 0(t0)");
      emit_float_struct_load(mixed_int->fp_reg_idx, static_cast<std::int64_t>(float_offset),
                             float_is_double);
      continue;
    }

    if (const auto* mixed_float =
            std::get_if<CallArgClass::StructMixedSseIntReg>(&arg_classes[i].value)) {
      bool float_is_double = true;
      std::size_t int_offset = 8;
      std::size_t int_size = 8;
      if (rv_class != nullptr) {
        if (const auto* float_and_int =
                std::get_if<RiscvFloatClass::FloatAndInt>(&rv_class->value)) {
          float_is_double = float_and_int->float_is_double;
          int_offset = float_and_int->int_offset;
          int_size = float_and_int->int_size;
        }
      }

      emit_float_struct_load(mixed_float->fp_reg_idx, 0, float_is_double);
      state.emit("    " + std::string(int_size <= 4 ? "lw " : "ld ") +
                 RISCV_ARG_REGS[mixed_float->int_reg_idx] + ", " +
                 std::to_string(int_offset) + "(t0)");
    }
  }
}

void RiscvCodegen::emit_call_instruction_impl(const std::optional<std::string>& direct_name,
                                              const std::optional<Operand>& func_ptr,
                                              bool /*indirect*/,
                                              std::size_t /*stack_arg_space*/) {
  if (direct_name.has_value()) {
    state.emit("    call " + *direct_name);
    return;
  }

  if (!func_ptr.has_value()) {
    return;
  }

  operand_to_t0(*func_ptr);
  state.emit("    mv t2, t0");
  state.emit("    jalr ra, t2, 0");
}

void RiscvCodegen::emit_call_cleanup_impl(std::size_t stack_arg_space,
                                          std::size_t f128_temp_space,
                                          bool /*indirect*/) {
  if (f128_temp_space > 0 && stack_arg_space == 0) {
    emit_addi_sp(state, static_cast<std::int64_t>(f128_temp_space));
  }
  if (stack_arg_space > 0) {
    const auto cleanup =
        static_cast<std::int64_t>(stack_arg_space + f128_temp_space);
    emit_addi_sp(state, cleanup);
  }
}

void RiscvCodegen::emit_call_store_result_impl(const Value& dest, IrType return_type) {
  if (return_type == IrType::I128) {
    if (const auto slot = state.get_slot(dest.raw)) {
      emit_store_to_s0("a0", slot->raw, "sd");
      emit_store_to_s0("a1", slot->raw + 8, "sd");
    }
    return;
  }

  if (return_type == IrType::F128) {
    f128_store_result_and_truncate(dest);
    return;
  }

  if (return_type == IrType::F32) {
    if (const auto slot = state.get_slot(dest.raw)) {
      state.emit("    fmv.x.w t0, fa0");
      emit_store_to_s0("t0", slot->raw, "sd");
    }
    return;
  }

  if (return_type == IrType::F64) {
    if (const auto slot = state.get_slot(dest.raw)) {
      state.emit("    fmv.x.d t0, fa0");
      emit_store_to_s0("t0", slot->raw, "sd");
    }
    return;
  }

  if (const auto reg = state.assigned_reg(dest.raw)) {
    state.emit("    mv " + std::string(callee_saved_name(*reg)) + ", a0");
    return;
  }

  if (const auto slot = state.get_slot(dest.raw)) {
    emit_store_to_s0("a0", slot->raw, "sd");
  }
}

// Broader aggregate/struct call lowering remains parked until the current
// outgoing-call family route widens beyond the scalar and wide-scalar staging
// packets now landed here.

}  // namespace c4c::backend::riscv::codegen

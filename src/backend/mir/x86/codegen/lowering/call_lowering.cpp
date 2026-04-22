#include "call_lowering.hpp"

namespace c4c::backend::x86 {

std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index) {
  if (function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
      *function_locations, c4c::backend::prepare::PreparedMovePhase::BeforeCall, block_index,
      instruction_index);
  if (before_call_bundle == nullptr) {
    return std::nullopt;
  }
  for (const auto& move : before_call_bundle->moves) {
    if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::Register ||
        move.destination_abi_index != std::optional<std::size_t>{arg_index} ||
        !move.destination_register_name.has_value()) {
      continue;
    }
    return *move.destination_register_name;
  }
  for (const auto& binding : before_call_bundle->abi_bindings) {
    if (binding.destination_kind !=
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        binding.destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::Register ||
        binding.destination_abi_index != std::optional<std::size_t>{arg_index} ||
        !binding.destination_register_name.has_value()) {
      continue;
    }
    return *binding.destination_register_name;
  }
  return std::nullopt;
}

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index) {
  if (function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* before_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
      *function_locations, c4c::backend::prepare::PreparedMovePhase::BeforeCall, block_index,
      instruction_index);
  if (before_call_bundle == nullptr) {
    return std::nullopt;
  }
  for (const auto& move : before_call_bundle->moves) {
    if (move.destination_kind != c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_storage_kind != c4c::backend::prepare::PreparedMoveStorageKind::StackSlot ||
        move.destination_abi_index != std::optional<std::size_t>{arg_index} ||
        !move.destination_stack_offset_bytes.has_value()) {
      continue;
    }
    return move.destination_stack_offset_bytes;
  }
  for (const auto& binding : before_call_bundle->abi_bindings) {
    if (binding.destination_kind !=
            c4c::backend::prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        binding.destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::StackSlot ||
        binding.destination_abi_index != std::optional<std::size_t>{arg_index} ||
        !binding.destination_stack_offset_bytes.has_value()) {
      continue;
    }
    return binding.destination_stack_offset_bytes;
  }
  return std::nullopt;
}

std::optional<std::string> select_prepared_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index) {
  return select_prepared_call_argument_abi_register_if_supported(
      function_locations, 0, instruction_index, arg_index);
}

std::optional<std::size_t> select_prepared_call_argument_abi_stack_offset_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index) {
  return select_prepared_call_argument_abi_stack_offset_if_supported(
      function_locations, 0, instruction_index, arg_index);
}

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    std::size_t arg_index) {
  const auto abi_register = select_prepared_call_argument_abi_register_if_supported(
      function_locations, block_index, instruction_index, arg_index);
  if (!abi_register.has_value()) {
    return std::nullopt;
  }
  return narrow_i32_register(*abi_register);
}

std::optional<std::string> select_prepared_i32_call_argument_abi_register_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    std::size_t arg_index) {
  return select_prepared_i32_call_argument_abi_register_if_supported(
      function_locations, 0, instruction_index, arg_index);
}

std::optional<PreparedCallResultAbiSelection> select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home) {
  if (function_locations == nullptr) {
    return std::nullopt;
  }
  const auto* after_call_bundle = c4c::backend::prepare::find_prepared_move_bundle(
      *function_locations, c4c::backend::prepare::PreparedMovePhase::AfterCall, block_index,
      instruction_index);
  if (after_call_bundle == nullptr) {
    return std::nullopt;
  }
  const auto* after_call_move = [&]() -> const c4c::backend::prepare::PreparedMoveResolution* {
    if (result_home != nullptr) {
      for (const auto& move : after_call_bundle->moves) {
        if (move.destination_kind ==
                c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi &&
            move.to_value_id == result_home->value_id) {
          return &move;
        }
      }
    }
    for (const auto& move : after_call_bundle->moves) {
      if (move.destination_kind ==
          c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi) {
        return &move;
      }
    }
    return nullptr;
  }();
  if (after_call_move != nullptr && after_call_move->destination_register_name.has_value()) {
    return PreparedCallResultAbiSelection{
        .move = after_call_move,
        .abi_register = *after_call_move->destination_register_name,
    };
  }
  for (const auto& binding : after_call_bundle->abi_bindings) {
    if (binding.destination_kind !=
            c4c::backend::prepare::PreparedMoveDestinationKind::CallResultAbi ||
        binding.destination_storage_kind !=
            c4c::backend::prepare::PreparedMoveStorageKind::Register ||
        !binding.destination_register_name.has_value()) {
      continue;
    }
    return PreparedCallResultAbiSelection{
        .move = nullptr,
        .abi_register = *binding.destination_register_name,
    };
  }
  return std::nullopt;
}

std::optional<PreparedCallResultAbiSelection> select_prepared_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home) {
  return select_prepared_call_result_abi_if_supported(
      function_locations, 0, instruction_index, result_home);
}

std::optional<PreparedI32CallResultAbiSelection> select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t block_index,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home) {
  const auto selection = select_prepared_call_result_abi_if_supported(
      function_locations, block_index, instruction_index, result_home);
  if (!selection.has_value()) {
    return std::nullopt;
  }
  const auto abi_register = narrow_i32_register(selection->abi_register);
  if (!abi_register.has_value()) {
    return std::nullopt;
  }
  return PreparedI32CallResultAbiSelection{
      .move = selection->move,
      .abi_register = std::move(*abi_register),
  };
}

std::optional<PreparedI32CallResultAbiSelection> select_prepared_i32_call_result_abi_if_supported(
    const c4c::backend::prepare::PreparedValueLocationFunction* function_locations,
    std::size_t instruction_index,
    const c4c::backend::prepare::PreparedValueHome* result_home) {
  return select_prepared_i32_call_result_abi_if_supported(
      function_locations, 0, instruction_index, result_home);
}

CallAbiConfig X86Codegen::call_abi_config_impl() const {
  return CallAbiConfig{
      .max_int_regs = 6,
      .max_float_regs = 8,
      .align_i128_pairs = false,
      .f128_in_fp_regs = false,
      .f128_in_gp_pairs = false,
      .variadic_floats_in_gp = false,
      .large_struct_by_ref = false,
      .use_sysv_struct_classification = true,
      .use_riscv_float_struct_classification = false,
      // Match the ref SysV x86_64 ABI contract: no partial GP-register plus
      // caller-stack aggregate split until the backend policy itself changes.
      .allow_struct_split_reg_stack = x86_allow_struct_split_reg_stack(),
      .align_struct_pairs = false,
      .sret_uses_dedicated_reg = false,
  };
}

std::size_t X86Codegen::emit_call_compute_stack_space_impl(const std::vector<CallArgClass>& arg_classes,
                                                           const std::vector<IrType>& /*arg_types*/) const {
  std::size_t pushed_bytes = 0;
  for (const auto& klass : arg_classes) {
    if (klass.is_stack()) {
      pushed_bytes += 8;
    }
  }
  return pushed_bytes + ((pushed_bytes % 16 != 0) ? 8 : 0);
}

std::int64_t X86Codegen::emit_call_stack_args_impl(const std::vector<Operand>& args,
                                                   const std::vector<CallArgClass>& arg_classes,
                                                   const std::vector<IrType>& /*arg_types*/,
                                                   std::size_t stack_arg_space,
                                                   std::size_t /*fptr_spill*/,
                                                   std::size_t /*f128_temp_space*/) {
  const bool need_align_pad = (stack_arg_space % 16) != 0;
  if (need_align_pad) {
    this->state.emit("    subq $8, %rsp");
  }

  for (std::size_t i = args.size(); i-- > 0;) {
    if (!arg_classes[i].is_stack()) {
      continue;
    }
    this->operand_to_rax(args[i]);
    this->state.emit("    pushq %rax");
  }

  return need_align_pad ? 8 : 0;
}

void X86Codegen::emit_call_reg_args_impl(const std::vector<Operand>& args,
                                         const std::vector<CallArgClass>& arg_classes,
                                         const std::vector<IrType>& arg_types,
                                         std::int64_t /*total_sp_adjust*/,
                                         std::size_t /*f128_temp_space*/,
                                         std::size_t /*stack_arg_space*/,
                                         const std::vector<std::optional<RiscvFloatClass>>& /*classes*/) {
  std::size_t gp_reg_idx = 0;
  std::size_t fp_reg_idx = 0;
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (!arg_classes[i].is_register()) {
      continue;
    }

    this->operand_to_rax(args[i]);
    const bool is_float = arg_types[i] == IrType::F32 || arg_types[i] == IrType::F64;
    if (is_float) {
      this->state.emit(std::string("    movq %rax, %") + x86_float_arg_reg_name(fp_reg_idx++));
    } else {
      this->state.emit(std::string("    movq %rax, %") + x86_arg_reg_name(gp_reg_idx++));
    }
  }
  if (fp_reg_idx > 0) {
    this->state.out.emit_instr_imm_reg("    movb", static_cast<std::int64_t>(fp_reg_idx), "al");
  } else {
    this->state.emit("    xorl %eax, %eax");
  }
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_call_instruction_impl(const std::optional<std::string>& direct_name,
                                            const std::optional<Operand>& func_ptr,
                                            bool /*indirect*/,
                                            std::size_t /*stack_arg_space*/) {
  if (direct_name.has_value()) {
    this->state.emit(std::string("    call ") + *direct_name);
  } else if (func_ptr.has_value()) {
    this->operand_to_rax(*func_ptr);
    this->state.emit("    call *%rax");
  }
}

void X86Codegen::emit_call_cleanup_impl(std::size_t stack_arg_space,
                                        std::size_t /*f128_temp_space*/,
                                        bool /*indirect*/) {
  const bool need_align_pad = (stack_arg_space % 16) != 0;
  const std::size_t total_cleanup = stack_arg_space + (need_align_pad ? 8 : 0);
  if (total_cleanup > 0) {
    this->state.out.emit_instr_imm_reg("    addq", static_cast<std::int64_t>(total_cleanup), "rsp");
  }
}

void X86Codegen::set_call_ret_eightbyte_classes_impl(const std::vector<EightbyteClass>& classes) {
  this->call_ret_classes = classes;
}

void X86Codegen::emit_call_store_result_impl(const Value& dest, IrType return_type) {
  if (return_type == IrType::I128) {
    if (this->call_ret_classes.size() == 2) {
      const auto c0 = this->call_ret_classes[0];
      const auto c1 = this->call_ret_classes[1];
      if (c0 == EightbyteClass::Integer && c1 == EightbyteClass::Sse) {
        if (const auto slot = this->state.get_slot(dest.raw)) {
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
          this->state.emit("    movq %xmm0, %rdx");
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw + 8, "rdx");
        }
        this->state.reg_cache.invalidate_all();
        return;
      }
      if (c0 == EightbyteClass::Sse && c1 == EightbyteClass::Integer) {
        if (const auto slot = this->state.get_slot(dest.raw)) {
          this->state.emit("    movq %xmm0, %rdx");
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rdx");
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw + 8, "rax");
        }
        this->state.reg_cache.invalidate_all();
        return;
      }
      if (c0 == EightbyteClass::Sse && c1 == EightbyteClass::Sse) {
        if (const auto slot = this->state.get_slot(dest.raw)) {
          this->state.emit("    movq %xmm0, %rax");
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw, "rax");
          this->state.emit("    movq %xmm1, %rax");
          this->state.out.emit_instr_rbp_reg("    movq", slot->raw + 8, "rax");
        }
        this->state.reg_cache.invalidate_all();
        return;
      }
    }
    this->store_rax_rdx_to(dest);
    return;
  }

  if (return_type == IrType::F32) {
    this->emit_call_move_f32_to_acc_impl();
    this->store_rax_to(dest);
  } else if (return_type == IrType::F64) {
    this->emit_call_move_f64_to_acc_impl();
    this->store_rax_to(dest);
  } else if (return_type == IrType::F128) {
    if (const auto slot = this->state.get_slot(dest.raw)) {
      this->state.out.emit_instr_rbp("    fstp", slot->raw);
      this->state.out.emit_instr_rbp("    fld", slot->raw);
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->state.reg_cache.set_acc(dest.raw, false);
      this->state.f128_direct_slots.insert(dest.raw);
    } else {
      this->state.emit("    subq $8, %rsp");
      this->state.emit("    fstpl (%rsp)");
      this->state.emit("    popq %rax");
      this->store_rax_to(dest);
    }
  } else {
    this->store_rax_to(dest);
  }
}

void X86Codegen::emit_call_store_i128_result_impl(const Value& dest) {
  this->store_rax_rdx_to(dest);
}

}  // namespace c4c::backend::x86

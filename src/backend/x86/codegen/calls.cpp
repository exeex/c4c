#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

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
                                         const std::vector<IrType>& /*arg_types*/,
                                         std::int64_t /*total_sp_adjust*/,
                                         std::size_t /*f128_temp_space*/,
                                         std::size_t /*stack_arg_space*/,
                                         const std::vector<std::optional<RiscvFloatClass>>& /*classes*/) {
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (arg_classes[i].is_register()) {
      this->operand_to_rax(args[i]);
      this->state.emit(std::string("    movq %rax, %") + x86_arg_reg_name(i));
    }
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
      this->state.out.emit_instr_rbp("    fstpt", slot->raw);
      this->state.out.emit_instr_rbp("    fldt", slot->raw);
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

void X86Codegen::emit_call_move_f32_to_acc_impl() { this->state.emit("    movd %xmm0, %eax"); }
void X86Codegen::emit_call_move_f64_to_acc_impl() { this->state.emit("    movq %xmm0, %rax"); }

}  // namespace c4c::backend::x86

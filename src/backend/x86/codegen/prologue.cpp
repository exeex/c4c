#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

std::string_view scalar_param_ref_type_name(IrType ty) {
  switch (ty) {
    case IrType::I32:
      return "i32";
    case IrType::U32:
      return "u32";
    case IrType::F32:
      return "f32";
    case IrType::I64:
      return "i64";
    case IrType::U64:
      return "u64";
    case IrType::F64:
      return "f64";
    case IrType::Ptr:
      return "ptr";
    default:
      return "";
  }
}

}  // namespace

std::int64_t X86Codegen::calculate_stack_space_impl(const IrFunction& func) {
  // Mirror the ref pipeline: classify parameters, run register allocation, and
  // then account for stack slots and callee-saved registers.
  this->is_variadic = func.is_variadic;
  const auto config = this->call_abi_config_impl();
  const auto classification = classify_params_full(func, config);
  this->num_named_int_params = 0;
  this->num_named_fp_params = 0;
  for (const auto& klass : classification.classes) {
    this->num_named_int_params += klass.gp_reg_count();
    if (klass.is_float_reg()) {
      ++this->num_named_fp_params;
    }
  }
  this->num_named_stack_bytes = named_params_stack_bytes(classification.classes);

  std::vector<PhysReg> asm_clobbered_regs;
  collect_inline_asm_callee_saved_x86(func, asm_clobbered_regs);
  auto available_regs = filter_available_regs(X86_CALLEE_SAVED, asm_clobbered_regs);

  bool has_indirect_call = false;
  // This slice wires the translated caller-saved pruning helper into the live
  // prologue/regalloc path for the indirect-call case without pulling the full
  // translated prologue owner into the build yet.
  for (const auto& block : func.blocks) {
    for (const auto& inst : block.instructions) {
      if (inst.is_call_indirect()) {
        has_indirect_call = true;
      }
    }
  }
  const std::vector<PhysReg> caller_saved_regs =
      x86_prune_caller_saved_regs(has_indirect_call, false, false);

  auto [reg_assigned, cached_liveness] =
      run_regalloc_and_merge_clobbers(func,
                                      available_regs,
                                      caller_saved_regs,
                                      asm_clobbered_regs,
                                      this->reg_assignments,
                                      this->used_callee_saved,
                                      false);

  std::int64_t space = calculate_stack_space_common(
      this->state,
      func,
      0,
      [](std::int64_t current, std::int64_t alloc_size, std::int64_t align) {
        const std::int64_t effective_align = align > 0 ? std::max<std::int64_t>(align, 8) : 8;
        const std::int64_t alloc = (alloc_size + 7) & ~7;
        const std::int64_t new_space =
            ((current + alloc + effective_align - 1) / effective_align) * effective_align;
        return std::pair<std::int64_t, std::int64_t>{-new_space, new_space};
      },
      reg_assigned,
      X86_CALLEE_SAVED,
      cached_liveness,
      false);

  if (func.is_variadic) {
    space += x86_variadic_reg_save_area_size(this->no_sse);
    this->reg_save_area_offset = -space;
  }

  space += static_cast<std::int64_t>(this->used_callee_saved.size()) * 8;
  return space;
}

std::int64_t X86Codegen::aligned_frame_size_impl(std::int64_t raw_space) const {
  return x86_aligned_frame_size(raw_space);
}

void X86Codegen::emit_prologue_impl(const IrFunction& func, std::int64_t frame_size) {
  this->current_return_type = func.return_type;
  this->func_ret_classes = func.ret_eightbyte_classes;
  if (this->state.cf_protection_branch) {
    this->state.emit("    endbr64");
  }
  this->state.emit("    pushq %rbp");
  this->state.emit("    movq %rsp, %rbp");
  if (frame_size > 0) {
    if (x86_needs_stack_probe(frame_size)) {
      const auto probe_label = this->state.fresh_label("stack_probe");
      const auto page_size = x86_stack_probe_page_size();
      this->state.out.emit_instr_imm_reg("    movq", frame_size, "r11");
      this->state.out.emit_named_label(&probe_label);
      this->state.out.emit_instr_imm_reg("    subq", page_size, "rsp");
      this->state.emit("    orl $0, (%rsp)");
      this->state.out.emit_instr_imm_reg("    subq", page_size, "r11");
      this->state.out.emit_instr_imm_reg("    cmpq", page_size, "r11");
      this->state.out.emit_jcc_label("    ja", &probe_label);
      this->state.emit("    subq %r11, %rsp");
      this->state.emit("    orl $0, (%rsp)");
    } else {
      this->state.out.emit_instr_imm_reg("    subq", frame_size, "rsp");
    }
  }
  for (std::size_t index = 0; index < this->used_callee_saved.size(); ++index) {
    const auto reg = this->used_callee_saved[index];
    this->state.out.emit_instr_reg_rbp(
        "    movq", phys_reg_name(reg), x86_callee_saved_slot_offset(frame_size, index));
  }
  if (func.is_variadic) {
    static constexpr const char* kVariadicGpRegs[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
    for (std::size_t index = 0; index < std::size(kVariadicGpRegs); ++index) {
      this->state.out.emit_instr_reg_rbp(
          "    movq",
          kVariadicGpRegs[index],
          x86_variadic_gp_save_offset(this->reg_save_area_offset, index));
    }
    if (!this->no_sse) {
      for (std::size_t index = 0; index < 8; ++index) {
        this->state.emit_fmt(
            format_args!("    movdqu %xmm{}, {}(%rbp)",
                         index,
                         x86_variadic_sse_save_offset(this->reg_save_area_offset, index)));
      }
    }
  }
}

void X86Codegen::emit_epilogue_impl(std::int64_t frame_size) {
  for (std::size_t index = 0; index < this->used_callee_saved.size(); ++index) {
    const auto reg = this->used_callee_saved[index];
    this->state.out.emit_instr_rbp_reg(
        "    movq", x86_callee_saved_slot_offset(frame_size, index), phys_reg_name(reg));
  }
  this->state.emit("    movq %rbp, %rsp");
  this->state.emit("    popq %rbp");
}

void X86Codegen::emit_store_params_impl(const IrFunction& func) {
  const auto config = this->call_abi_config_impl();
  const auto classification = classify_params_full(func, config);
  this->state.param_classes = classification.classes;
  this->state.param_pre_stored.clear();
}

void X86Codegen::emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty) {
  if (param_idx >= this->state.param_classes.size()) {
    return;
  }
  if (this->state.param_pre_stored.contains(param_idx)) {
    return;
  }
  const auto scalar_type = scalar_param_ref_type_name(ty);
  if (scalar_type.empty()) {
    return;
  }

  const auto& param_class = this->state.param_classes[param_idx];
  if (const auto* reg = std::get_if<ParamClass::IntReg>(&param_class.data)) {
    const auto* load_instr = x86_param_ref_scalar_load_instr(scalar_type);
    const auto* src_reg = x86_param_ref_scalar_arg_reg(reg->reg_idx, scalar_type);
    const auto* dest_reg = x86_param_ref_scalar_dest_reg(scalar_type);
    if (load_instr[0] != '\0' && src_reg[0] != '\0' && dest_reg[0] != '\0') {
      this->state.emit_fmt(format_args!("    {} %{}, {}", load_instr, src_reg, dest_reg));
      this->store_rax_to(dest);
      return;
    }
  }
  if (const auto* reg = std::get_if<ParamClass::FloatReg>(&param_class.data)) {
    const auto* move_instr = x86_param_ref_float_reg_move_instr(scalar_type);
    const auto* src_reg = x86_param_ref_float_arg_reg(reg->reg_idx, scalar_type);
    const auto* dest_reg = x86_param_ref_scalar_dest_reg(scalar_type);
    if (move_instr[0] != '\0' && src_reg[0] != '\0' && dest_reg[0] != '\0') {
      this->state.emit_fmt(format_args!("    {} %{}, {}", move_instr, src_reg, dest_reg));
      this->store_rax_to(dest);
      return;
    }
  }
  if (const auto* stack = std::get_if<ParamClass::StackScalar>(&param_class.data)) {
    const auto* load_instr = x86_param_ref_scalar_load_instr(scalar_type);
    const auto stack_operand = x86_param_ref_scalar_stack_operand(stack->offset, scalar_type);
    const auto* dest_reg = x86_param_ref_scalar_dest_reg(scalar_type);
    if (load_instr[0] != '\0' && !stack_operand.empty() && dest_reg[0] != '\0') {
      this->state.emit_fmt(format_args!("    {} {}, {}", load_instr, stack_operand, dest_reg));
      this->store_rax_to(dest);
      return;
    }
  }
}

void X86Codegen::emit_epilogue_and_ret_impl(std::int64_t frame_size) {
  this->emit_epilogue_impl(frame_size);
  this->state.emit(this->state.function_return_thunk ? "    jmp __x86_return_thunk"
                                                     : "    ret");
}

const char* X86Codegen::store_instr_for_type_impl(IrType ty) const {
  return this->mov_store_for_type(ty);
}

const char* X86Codegen::load_instr_for_type_impl(IrType ty) const {
  return this->mov_load_for_type(ty);
}

}  // namespace c4c::backend::x86

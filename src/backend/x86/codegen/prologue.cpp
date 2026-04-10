#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

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

  std::vector<PhysReg> caller_saved_regs = X86_CALLER_SAVED;
  // The x86 ref backend tightens caller-saved usage when it sees indirect calls,
  // i128 work, or atomics.
  for (const auto& block : func.blocks) {
    for (const auto& inst : block.instructions) {
      if (inst.is_call_indirect()) {
        // The ref backend prunes some caller-saved registers here.
        caller_saved_regs.clear();
      }
    }
  }

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
    this->state.emit("    subq <frame>, %rsp");
  }
  for (const auto& reg : this->used_callee_saved) {
    this->state.emit("    movq <callee-saved>, <stack-slot>");
  }
}

void X86Codegen::emit_epilogue_impl(std::int64_t frame_size) {
  for (const auto& reg : this->used_callee_saved) {
    this->state.emit("    movq <stack-slot>, <callee-saved>");
  }
  this->state.emit("    movq %rbp, %rsp");
  this->state.emit("    popq %rbp");
}

void X86Codegen::emit_store_params_impl(const IrFunction& func) {
  // The ref backend preloads ABI parameters into stack slots and, when safe,
  // directly into assigned callee-saved registers.
  this->state.emit("    <store-parameters>");
}

void X86Codegen::emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty) {
  if (param_idx >= this->state.param_classes.size()) {
    return;
  }
  if (this->state.param_pre_stored.contains(param_idx)) {
    return;
  }
  this->state.emit("    <load-parameter>");
  this->store_rax_to(dest);
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

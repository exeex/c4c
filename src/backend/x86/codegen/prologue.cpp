#include "x86_codegen.hpp"
#include "../../regalloc.hpp"

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

std::optional<StackSlot> lookup_param_slot(X86Codegen& codegen, std::string_view param_name) {
  const auto slot_name = x86_param_slot_name(param_name);
  if (slot_name.empty()) {
    return std::nullopt;
  }
  // The current transitional x86 state is keyed by value id, not slot name.
  // Keep the compile boundary honest without inventing a lossy name->slot map.
  (void)codegen;
  (void)slot_name;
  return std::nullopt;
}

[[maybe_unused]] void emit_struct_sse_param_store(X86Codegen& codegen,
                                                  std::int64_t slot_offset,
                                                  std::size_t lo_fp_idx,
                                                  std::optional<std::size_t> hi_fp_idx) {
  const auto* lo_src_reg = x86_param_struct_sse_arg_reg(lo_fp_idx);
  if (lo_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", lo_src_reg, x86_param_struct_sse_dest_offset(slot_offset, 0));
  }
  if (!hi_fp_idx.has_value()) {
    return;
  }
  const auto* hi_src_reg = x86_param_struct_sse_arg_reg(*hi_fp_idx);
  if (hi_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", hi_src_reg, x86_param_struct_sse_dest_offset(slot_offset, 1));
  }
}

[[maybe_unused]] void emit_struct_mixed_int_sse_param_store(X86Codegen& codegen,
                                                            std::int64_t slot_offset,
                                                            std::size_t int_reg_idx,
                                                            std::size_t fp_reg_idx) {
  const auto* int_src_reg = x86_param_struct_mixed_int_sse_int_arg_reg(int_reg_idx);
  if (int_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", int_src_reg, x86_param_struct_mixed_int_sse_int_dest_offset(slot_offset));
  }
  const auto* fp_src_reg = x86_param_struct_mixed_int_sse_fp_arg_reg(fp_reg_idx);
  if (fp_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", fp_src_reg, x86_param_struct_mixed_int_sse_fp_dest_offset(slot_offset));
  }
}

[[maybe_unused]] void emit_struct_mixed_sse_int_param_store(X86Codegen& codegen,
                                                            std::int64_t slot_offset,
                                                            std::size_t fp_reg_idx,
                                                            std::size_t int_reg_idx) {
  const auto* fp_src_reg = x86_param_struct_mixed_sse_int_fp_arg_reg(fp_reg_idx);
  if (fp_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", fp_src_reg, x86_param_struct_mixed_sse_int_fp_dest_offset(slot_offset));
  }
  const auto* int_src_reg = x86_param_struct_mixed_sse_int_int_arg_reg(int_reg_idx);
  if (int_src_reg[0] != '\0') {
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", int_src_reg, x86_param_struct_mixed_sse_int_int_dest_offset(slot_offset));
  }
}

[[maybe_unused]] void emit_struct_byval_reg_param_store(X86Codegen& codegen,
                                                        std::int64_t slot_offset,
                                                        std::size_t base_reg_idx,
                                                        std::size_t size_bytes) {
  const auto qword_count = x86_param_struct_reg_qword_count(size_bytes);
  for (std::size_t qword_index = 0; qword_index < qword_count; ++qword_index) {
    const auto* src_reg = x86_param_struct_reg_arg_reg(base_reg_idx, qword_index);
    if (src_reg[0] == '\0') {
      continue;
    }
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", src_reg, x86_param_struct_reg_dest_offset(slot_offset, qword_index));
  }
}

[[maybe_unused]] void emit_struct_stack_param_store(X86Codegen& codegen,
                                                    std::int64_t slot_offset,
                                                    std::int64_t stack_offset,
                                                    std::size_t size_bytes) {
  const auto qword_count = x86_param_aggregate_copy_qword_count(size_bytes);
  for (std::size_t qword_index = 0; qword_index < qword_count; ++qword_index) {
    codegen.state.out.emit_instr_rbp_reg(
        "    movq", x86_param_aggregate_copy_src_offset(stack_offset, qword_index), "rax");
    codegen.state.out.emit_instr_reg_rbp(
        "    movq", "rax", x86_param_aggregate_copy_dest_offset(slot_offset, qword_index));
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
  const auto callee_saved_regs = x86_callee_saved_regs();
  auto available_regs = filter_available_regs(callee_saved_regs, asm_clobbered_regs);

  bool has_indirect_call = false;
  bool has_i128_ops = false;
  bool has_atomic_rmw = false;
  // This slice wires the translated caller-saved pruning helper into the live
  // prologue/regalloc path without widening into a broader owner reset.
  for (const auto& block : func.blocks) {
    for (const auto& inst : block.instructions) {
      if (inst.is_call_indirect()) {
        has_indirect_call = true;
      }
      switch (inst.kind) {
        case IrInstruction::Kind::BinOp:
        case IrInstruction::Kind::UnaryOp:
        case IrInstruction::Kind::Cmp:
        case IrInstruction::Kind::Store:
          has_i128_ops = has_i128_ops || inst.ty == IrType::I128;
          break;
        case IrInstruction::Kind::Cast:
          has_i128_ops = has_i128_ops || inst.from_ty == IrType::I128 || inst.to_ty == IrType::I128;
          break;
        case IrInstruction::Kind::AtomicRmw:
          has_atomic_rmw = true;
          break;
        default:
          break;
      }
    }
  }
  const std::vector<PhysReg> caller_saved_regs =
      x86_prune_caller_saved_regs(has_indirect_call, has_i128_ops, has_atomic_rmw);

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
      callee_saved_regs,
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
      const auto probe_label = std::string(".Lstack_probe_") + std::to_string(frame_size);
      const auto page_size = x86_stack_probe_page_size();
      this->state.out.emit_instr_imm_reg("    movq", frame_size, "r11");
      this->state.emit(probe_label + ":");
      this->state.out.emit_instr_imm_reg("    subq", page_size, "rsp");
      this->state.emit("    orl $0, (%rsp)");
      this->state.out.emit_instr_imm_reg("    subq", page_size, "r11");
      this->state.out.emit_instr_imm_reg("    cmpq", page_size, "r11");
      this->state.emit("    ja " + probe_label);
      this->state.emit("    subq %r11, %rsp");
      this->state.emit("    orl $0, (%rsp)");
    } else {
      this->state.out.emit_instr_imm_reg("    subq", frame_size, "rsp");
    }
  }
  for (std::size_t index = 0; index < this->used_callee_saved.size(); ++index) {
    const auto reg = c4c::backend::PhysReg{this->used_callee_saved[index]};
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
        this->state.emit(std::string("    movdqu %xmm") + std::to_string(index) + ", " +
                         std::to_string(x86_variadic_sse_save_offset(this->reg_save_area_offset,
                                                                      index)) +
                         "(%rbp)");
      }
    }
  }
}

void X86Codegen::emit_epilogue_impl(std::int64_t frame_size) {
  for (std::size_t index = 0; index < this->used_callee_saved.size(); ++index) {
    const auto reg = c4c::backend::PhysReg{this->used_callee_saved[index]};
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

  std::unordered_map<std::uint32_t, std::size_t> reg_param_use_counts;
  for (const auto& param : func.params) {
    if (const auto assigned = this->reg_assignments.find(param.name);
        assigned != this->reg_assignments.end()) {
      ++reg_param_use_counts[assigned->second];
    }
  }

  for (std::size_t index = 0; index < func.params.size() && index < classification.classes.size();
       ++index) {
    const auto param_slot = lookup_param_slot(*this, func.params[index].name);
    const auto assigned = this->reg_assignments.find(func.params[index].name);
    if (assigned == this->reg_assignments.end()) {
      if (param_slot.has_value()) {
        if (const auto* reg =
                std::get_if<ParamClass::StructByValReg>(&classification.classes[index].data)) {
          emit_struct_byval_reg_param_store(*this, param_slot->raw, reg->base_reg_idx, reg->size);
        } else if (const auto* reg =
                std::get_if<ParamClass::StructSseReg>(&classification.classes[index].data)) {
          emit_struct_sse_param_store(*this, param_slot->raw, reg->lo_fp_idx, reg->hi_fp_idx);
        } else if (const auto* reg = std::get_if<ParamClass::StructMixedIntSseReg>(
                       &classification.classes[index].data)) {
          emit_struct_mixed_int_sse_param_store(
              *this, param_slot->raw, reg->int_reg_idx, reg->fp_reg_idx);
        } else if (const auto* reg = std::get_if<ParamClass::StructMixedSseIntReg>(
                       &classification.classes[index].data)) {
          emit_struct_mixed_sse_int_param_store(
              *this, param_slot->raw, reg->fp_reg_idx, reg->int_reg_idx);
        } else if (const auto* stack =
                       std::get_if<ParamClass::StructStack>(&classification.classes[index].data)) {
          emit_struct_stack_param_store(*this, param_slot->raw, stack->offset, stack->size);
        } else if (const auto* stack = std::get_if<ParamClass::LargeStructStack>(
                       &classification.classes[index].data)) {
          emit_struct_stack_param_store(*this, param_slot->raw, stack->offset, stack->size);
        }
      }
      continue;
    }
    const auto assigned_param_count = reg_param_use_counts[assigned->second];
    const auto assigned_reg = c4c::backend::PhysReg{assigned->second};
    if (x86_param_can_prestore_direct_to_reg(false, assigned_reg, assigned_param_count)) {
      if (const auto* reg = std::get_if<ParamClass::IntReg>(&classification.classes[index].data)) {
        const auto* move_instr = x86_param_prestore_move_instr();
        const auto* src_reg = x86_param_prestore_arg_reg(reg->reg_idx);
        const auto* dest_reg = x86_param_prestore_dest_reg(assigned_reg);
        if (move_instr[0] != '\0' && src_reg[0] != '\0' && dest_reg[0] != '\0') {
          this->state.emit(std::string("    ") + move_instr + " %" + src_reg + ", %" + dest_reg);
          x86_mark_param_prestored(this->state.param_pre_stored, index);
        }
      } else if (const auto* reg =
                     std::get_if<ParamClass::FloatReg>(&classification.classes[index].data)) {
        const auto scalar_type = scalar_param_ref_type_name(func.params[index].type);
        const auto* move_instr = x86_param_prestore_float_move_instr(scalar_type);
        const auto* src_reg = x86_param_prestore_float_arg_reg(reg->reg_idx, scalar_type);
        const auto* dest_reg = x86_param_prestore_dest_reg(assigned_reg, scalar_type);
        if (move_instr[0] != '\0' && src_reg[0] != '\0' && dest_reg[0] != '\0') {
          this->state.emit(std::string("    ") + move_instr + " %" + src_reg + ", %" + dest_reg);
          x86_mark_param_prestored(this->state.param_pre_stored, index);
        }
      }
    }
  }
}

void X86Codegen::emit_param_ref_impl(const Value& dest, std::size_t param_idx, IrType ty) {
  if (param_idx >= this->state.param_classes.size()) {
    return;
  }
  if (x86_param_is_prestored(this->state.param_pre_stored, param_idx)) {
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
      this->state.emit(std::string("    ") + load_instr + " %" + src_reg + ", " + dest_reg);
      this->store_rax_to(dest);
      return;
    }
  }
  if (const auto* reg = std::get_if<ParamClass::FloatReg>(&param_class.data)) {
    const auto* move_instr = x86_param_ref_float_reg_move_instr(scalar_type);
    const auto* src_reg = x86_param_ref_float_arg_reg(reg->reg_idx, scalar_type);
    const auto* dest_reg = x86_param_ref_scalar_dest_reg(scalar_type);
    if (move_instr[0] != '\0' && src_reg[0] != '\0' && dest_reg[0] != '\0') {
      this->state.emit(std::string("    ") + move_instr + " %" + src_reg + ", " + dest_reg);
      this->store_rax_to(dest);
      return;
    }
  }
  if (const auto* stack = std::get_if<ParamClass::StackScalar>(&param_class.data)) {
    const auto* load_instr = x86_param_ref_scalar_load_instr(scalar_type);
    const auto stack_operand = x86_param_ref_scalar_stack_operand(stack->offset, scalar_type);
    const auto* dest_reg = x86_param_ref_scalar_dest_reg(scalar_type);
    if (load_instr[0] != '\0' && !stack_operand.empty() && dest_reg[0] != '\0') {
      this->state.emit(std::string("    ") + load_instr + " " + stack_operand + ", " + dest_reg);
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

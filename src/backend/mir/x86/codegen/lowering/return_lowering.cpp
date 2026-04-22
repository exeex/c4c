#include "return_lowering.hpp"

namespace c4c::backend::x86 {

namespace {

void emit_return_epilogue_and_ret(X86Codegen& codegen, std::int64_t frame_size) {
  for (std::size_t index = 0; index < codegen.used_callee_saved.size(); ++index) {
    const auto reg = c4c::backend::PhysReg{codegen.used_callee_saved[index]};
    codegen.state.out.emit_instr_rbp_reg(
        "    movq", x86_callee_saved_slot_offset(frame_size, index), phys_reg_name(reg));
  }
  codegen.state.emit("    movq %rbp, %rsp");
  codegen.state.emit("    popq %rbp");
  codegen.state.emit(codegen.state.function_return_thunk ? "    jmp __x86_return_thunk"
                                                         : "    ret");
}

}  // namespace

void X86Codegen::emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size) {
  if (val.has_value() && this->current_return_type == IrType::F128) {
    const auto value_id = val->raw;
    if (this->state.f128_direct_slots.find(value_id) != this->state.f128_direct_slots.end()) {
      if (const auto slot = this->state.get_slot(value_id)) {
        this->state.out.emit_instr_rbp("    fld", slot->raw);
        emit_return_epilogue_and_ret(*this, frame_size);
        return;
      }
    }
    if (const auto ptr_id = this->state.get_f128_source(value_id)) {
      if (const auto addr = this->state.resolve_slot_addr(*ptr_id)) {
        switch (addr->kind) {
          case SlotAddr::Kind::Direct:
            this->state.out.emit_instr_rbp("    fld", addr->slot.raw);
            break;
          case SlotAddr::Kind::OverAligned:
            this->emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
            this->state.emit("    fld (%rcx)");
            break;
          case SlotAddr::Kind::Indirect:
            this->emit_load_ptr_from_slot_impl(addr->slot, *ptr_id);
            this->state.emit("    fld (%rcx)");
            break;
        }
        emit_return_epilogue_and_ret(*this, frame_size);
        return;
      }
    }
    if (this->state.get_f128_constant_words(value_id).has_value()) {
      this->emit_f128_load_to_x87(*val);
      emit_return_epilogue_and_ret(*this, frame_size);
      return;
    }
  }

  if (val.has_value()) {
    if (this->current_return_type == IrType::I128) {
      this->operand_to_rax_rdx(*val);
      this->emit_return_i128_to_regs_impl();
      emit_return_epilogue_and_ret(*this, frame_size);
      return;
    }

    this->emit_load_operand_impl(*val);
    if (this->current_return_type == IrType::F128) {
      this->emit_return_f128_to_reg_impl();
    } else if (this->current_return_type == IrType::F32) {
      this->emit_return_f32_to_reg_impl();
    } else if (this->current_return_type == IrType::F64) {
      this->emit_return_f64_to_reg_impl();
    } else {
      this->emit_return_int_to_reg_impl();
    }
  }

  emit_return_epilogue_and_ret(*this, frame_size);
}

IrType X86Codegen::current_return_type_impl() const { return this->current_return_type; }

void X86Codegen::emit_return_i128_to_regs_impl() {
  if (this->func_ret_classes.size() != 2) {
    return;
  }

  const auto first = this->func_ret_classes[0];
  const auto second = this->func_ret_classes[1];
  if (first == EightbyteClass::Integer && second == EightbyteClass::Sse) {
    this->state.emit("    movq %rdx, %xmm0");
  } else if (first == EightbyteClass::Sse && second == EightbyteClass::Integer) {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movq %rdx, %rax");
  } else if (first == EightbyteClass::Sse && second == EightbyteClass::Sse) {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movq %rdx, %xmm1");
  }
}

void X86Codegen::emit_return_int_to_reg_impl() {}

}  // namespace c4c::backend::x86

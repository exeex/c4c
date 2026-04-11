#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

void emit_bounded_epilogue_and_ret(X86Codegen& codegen) {
  codegen.state.emit("    movq %rbp, %rsp");
  codegen.state.emit("    popq %rbp");
  codegen.state.emit("    ret");
}

}  // namespace

void X86Codegen::emit_return_impl(const std::optional<Operand>& val, std::int64_t frame_size) {
  (void)frame_size;

  if (val.has_value() && this->current_return_type == IrType::F128) {
    const auto value_id = val->raw;
    if (this->state.f128_direct_slots.find(value_id) != this->state.f128_direct_slots.end()) {
      if (const auto slot = this->state.get_slot(value_id)) {
        this->state.out.emit_instr_rbp("    fldt", slot->raw);
        emit_bounded_epilogue_and_ret(*this);
        return;
      }
    }
    if (const auto ptr_id = this->state.get_f128_source(value_id)) {
      if (const auto addr = this->state.resolve_slot_addr(*ptr_id)) {
        switch (addr->kind) {
          case SlotAddr::Kind::Direct:
            this->state.out.emit_instr_rbp("    fldt", addr->slot.raw);
            break;
          case SlotAddr::Kind::OverAligned:
            this->emit_alloca_aligned_addr_impl(addr->slot, addr->value_id);
            this->state.emit("    fldt (%rcx)");
            break;
          case SlotAddr::Kind::Indirect:
            this->emit_load_ptr_from_slot_impl(addr->slot, *ptr_id);
            this->state.emit("    fldt (%rcx)");
            break;
        }
        emit_bounded_epilogue_and_ret(*this);
        return;
      }
    }
  }

  emit_bounded_epilogue_and_ret(*this);
}

IrType X86Codegen::current_return_type_impl() const { return this->current_return_type; }

void X86Codegen::emit_return_f32_to_reg_impl() { this->state.emit("    movd %eax, %xmm0"); }

void X86Codegen::emit_return_f64_to_reg_impl() { this->state.emit("    movq %rax, %xmm0"); }

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

void X86Codegen::emit_return_f128_to_reg_impl() {
  this->state.emit("    pushq %rax");
  this->state.emit("    fldl (%rsp)");
  this->state.emit("    addq $8, %rsp");
}

void X86Codegen::emit_return_int_to_reg_impl() {}

void X86Codegen::emit_get_return_f64_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movsd %xmm1, " + std::to_string(slot->raw) + "(%rbp)");
  }
}

void X86Codegen::emit_set_return_f64_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    movsd " + std::to_string(slot->raw) + "(%rbp), %xmm1");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    movq %rax, %xmm1");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_get_return_f32_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    movss %xmm1, " + std::to_string(slot->raw) + "(%rbp)");
  }
}

void X86Codegen::emit_set_return_f32_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    movss " + std::to_string(slot->raw) + "(%rbp), %xmm1");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    movd %eax, %xmm1");
  this->state.reg_cache.invalidate_all();
}

void X86Codegen::emit_get_return_f128_second_impl(const Value& dest) {
  if (const auto slot = this->state.get_slot(dest.raw)) {
    this->state.emit("    fstpt " + std::to_string(slot->raw) + "(%rbp)");
    this->state.emit("    fldt " + std::to_string(slot->raw) + "(%rbp)");
    this->state.emit("    subq $8, %rsp");
    this->state.emit("    fstpl (%rsp)");
    this->state.emit("    popq %rax");
    this->state.reg_cache.set_acc(dest.raw, false);
    this->state.f128_direct_slots.insert(dest.raw);
    return;
  }
  this->state.emit("    fstp %st(0)");
}

void X86Codegen::emit_set_return_f128_second_impl(const Operand& src) {
  if (const auto slot = this->state.get_slot(src.raw)) {
    this->state.emit("    fldt " + std::to_string(slot->raw) + "(%rbp)");
    return;
  }
  this->operand_to_rax(src);
  this->state.emit("    pushq %rax");
  this->state.emit("    fildq (%rsp)");
  this->state.emit("    addq $8, %rsp");
}

}  // namespace c4c::backend::x86

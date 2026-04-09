#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

void X86Codegen::emit_atomic_rmw_impl(const Value& dest,
                                      AtomicRmwOp op,
                                      const Operand& ptr,
                                      const Operand& val,
                                      IrType ty,
                                      AtomicOrdering /*ordering*/) {
  this->operand_to_rax(ptr);
  this->state.emit("    movq %rax, %rcx");
  this->operand_to_rax(val);
  this->state.reg_cache.invalidate_all();

  const auto size_suffix = this->type_suffix(ty);
  const auto val_reg = this->reg_for_type("rax", ty);
  switch (op) {
    case AtomicRmwOp::Add:
      this->state.emit("    lock xadd <ty> %rax, (%rcx)");
      break;
    case AtomicRmwOp::Xchg:
      this->state.emit("    xchg <ty> %rax, (%rcx)");
      break;
    case AtomicRmwOp::TestAndSet:
      this->state.emit("    movb $1, %al");
      this->state.emit("    xchgb %al, (%rcx)");
      this->state.emit("    movzbl %al, %eax");
      break;
    case AtomicRmwOp::Sub:
    case AtomicRmwOp::And:
    case AtomicRmwOp::Or:
    case AtomicRmwOp::Xor:
    case AtomicRmwOp::Nand:
      this->emit_x86_atomic_op_loop(ty, "logical-or-arithmetic-rmw");
      break;
  }

  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_cmpxchg_impl(const Value& dest,
                                          const Operand& ptr,
                                          const Operand& expected,
                                          const Operand& desired,
                                          IrType ty,
                                          AtomicOrdering /*success_ordering*/,
                                          AtomicOrdering /*failure_ordering*/,
                                          bool returns_bool) {
  this->operand_to_rax(ptr);
  this->state.emit("    movq %rax, %rcx");
  this->operand_to_rax(desired);
  this->state.emit("    movq %rax, %rdx");
  this->operand_to_rax(expected);
  this->state.reg_cache.invalidate_all();
  this->state.emit("    lock cmpxchg <ty> %rdx, (%rcx)");
  if (returns_bool) {
    this->state.emit("    sete %al");
    this->state.emit("    movzbl %al, %eax");
  }
  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_load_impl(const Value& dest,
                                       const Operand& ptr,
                                       IrType ty,
                                       AtomicOrdering /*ordering*/) {
  this->operand_to_rax(ptr);
  this->state.reg_cache.invalidate_all();
  this->state.emit("    <atomic-load>");
  this->store_rax_to(dest);
}

void X86Codegen::emit_atomic_store_impl(const Operand& ptr,
                                        const Operand& val,
                                        IrType ty,
                                        AtomicOrdering ordering) {
  this->operand_to_rax(val);
  this->state.emit("    movq %rax, %rdx");
  this->operand_to_rax(ptr);
  this->state.reg_cache.invalidate_all();
  this->state.emit("    <atomic-store>");
  if (ordering == AtomicOrdering::SeqCst) {
    this->state.emit("    mfence");
  }
}

void X86Codegen::emit_fence_impl(AtomicOrdering ordering) {
  if (ordering != AtomicOrdering::Relaxed) {
    this->state.emit("    mfence");
  }
}

}  // namespace c4c::backend::x86

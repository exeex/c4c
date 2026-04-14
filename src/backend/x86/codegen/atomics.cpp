#include "x86_codegen.hpp"

namespace c4c::backend::x86 {

namespace {

const char* atomic_load_dest_reg(IrType ty) {
  switch (ty) {
    case IrType::U32:
    case IrType::F32:
      return "%eax";
    default:
      return "%rax";
  }
}

}  // namespace

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
      this->state.emit(std::string("    lock xadd") + size_suffix + " %" + val_reg + ", (%rcx)");
      break;
    case AtomicRmwOp::Xchg:
      this->state.emit(std::string("    xchg") + size_suffix + " %" + val_reg + ", (%rcx)");
      break;
    case AtomicRmwOp::TestAndSet:
      this->state.emit("    movb $1, %al");
      this->state.emit("    xchgb %al, (%rcx)");
      this->state.emit("    movzbl %al, %eax");
      break;
    case AtomicRmwOp::Sub:
      this->emit_x86_atomic_op_loop(ty, "sub");
      break;
    case AtomicRmwOp::And:
      this->emit_x86_atomic_op_loop(ty, "and");
      break;
    case AtomicRmwOp::Or:
      this->emit_x86_atomic_op_loop(ty, "or");
      break;
    case AtomicRmwOp::Xor:
      this->emit_x86_atomic_op_loop(ty, "xor");
      break;
    case AtomicRmwOp::Nand:
      this->emit_x86_atomic_op_loop(ty, "nand");
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
  const auto size_suffix = this->type_suffix(ty);
  const auto desired_reg = this->reg_for_type("rdx", ty);
  this->state.emit(std::string("    lock cmpxchg") + size_suffix + " %" + desired_reg + ", (%rcx)");
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
  const auto load_instr = this->mov_load_for_type(ty);
  this->state.emit(std::string("    ") + load_instr + " (%rax), " + atomic_load_dest_reg(ty));
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
  const auto store_reg = this->reg_for_type("rdx", ty);
  const auto store_instr = this->mov_store_for_type(ty);
  this->state.emit(std::string("    ") + store_instr + " %" + store_reg + ", (%rax)");
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

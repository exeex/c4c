#include "lowering/scalar_lowering.hpp"

namespace c4c::backend::x86 {

// Dormant compatibility surface: canonical integer unary/binop lowering now
// lives in `lowering/scalar_lowering.cpp`. Keep explicit non-scalar holdouts
// here until later packets move them to their final owners.

void X86Codegen::emit_float_neg_impl(IrType ty) {
  if (ty == IrType::F32) {
    this->state.emit("    movd %eax, %xmm0");
    this->state.emit("    movl $0x80000000, %ecx");
    this->state.emit("    movd %ecx, %xmm1");
    this->state.emit("    xorps %xmm1, %xmm0");
    this->state.emit("    movd %xmm0, %eax");
  } else {
    this->state.emit("    movq %rax, %xmm0");
    this->state.emit("    movabsq $-9223372036854775808, %rcx");
    this->state.emit("    movq %rcx, %xmm1");
    this->state.emit("    xorpd %xmm1, %xmm0");
    this->state.emit("    movq %xmm0, %rax");
  }
}

void X86Codegen::emit_copy_i128_impl(const Value& dest, const Operand& src) {
  this->operand_to_rax_rdx(src);
  this->store_rax_rdx_to(dest);
}

}  // namespace c4c::backend::x86

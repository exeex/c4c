#include "lowering/float_lowering.hpp"

namespace c4c::backend::x86 {

// Dormant compatibility surface: canonical float lowering now lives in
// `lowering/float_lowering.cpp`. Keep the explicit i128 holdout here until a
// later packet moves it to its final owner.

void X86Codegen::emit_copy_i128_impl(const Value& dest, const Operand& src) {
  this->operand_to_rax_rdx(src);
  this->store_rax_rdx_to(dest);
}

}  // namespace c4c::backend::x86

// Mechanical C++-shaped translation of ref/claudes-c-compiler/src/backend/x86/codegen/peephole/mod.rs
// x86-64 assembly peephole optimizer entry point.

#include "peephole.hpp"

#include <utility>

namespace c4c::backend::x86::codegen::peephole {

std::string peephole_optimize(std::string asm_text) {
  return passes::peephole_optimize(std::move(asm_text));
}

}  // namespace c4c::backend::x86::codegen::peephole

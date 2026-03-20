#include "lir_printer.hpp"

namespace c4c::codegen::lir {

std::string print_llvm(const LirModule& /*mod*/) {
  // Stage 0: no-op.
  // Stage 2+ will implement full LLVM IR rendering from the LirModule.
  return {};
}

}  // namespace c4c::codegen::lir

#include "x86_codegen.hpp"
#include "peephole/peephole.hpp"

#include "../../backend.hpp"
#include "../../lowering/lir_to_bir.hpp"

#include <stdexcept>

namespace c4c::backend::x86 {

namespace {

[[noreturn]] void throw_unsupported_direct_bir_module() {
  throw std::invalid_argument(
      "x86 backend emitter does not support this direct BIR module; only the affine-return subset lowers natively");
}

[[noreturn]] void throw_x86_rewrite_in_progress() {
  throw std::invalid_argument(
      "x86 backend emitter rewrite in progress: move ownership into the translated sibling codegen translation units instead of adding emit.cpp-local matchers");
}

}  // namespace

std::string emit_module(const c4c::backend::bir::Module& module) {
  (void)module;
  throw_unsupported_direct_bir_module();
}

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  if (const auto lowered = c4c::backend::try_lower_to_bir(module); lowered.has_value()) {
    return emit_module(*lowered);
  }
  throw_x86_rewrite_in_progress();
}

assembler::AssembleResult assemble_module(const c4c::codegen::lir::LirModule& module,
                                          const std::string& output_path) {
  return assembler::assemble(assembler::AssembleRequest{
      .asm_text = emit_module(module),
      .output_path = output_path,
  });
}

}  // namespace c4c::backend::x86

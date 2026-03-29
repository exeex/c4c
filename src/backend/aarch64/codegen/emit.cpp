#include "emit.hpp"

#include "../../lir_adapter.hpp"
#include "../../../codegen/lir/lir_printer.hpp"

#include <stdexcept>

namespace c4c::backend::aarch64 {

namespace {

[[noreturn]] void fail_unsupported(const char* detail) {
  throw std::invalid_argument(std::string("aarch64 backend emitter does not support ") +
                              detail);
}

void validate_inst(const c4c::codegen::lir::LirInst& inst) {
  if (std::holds_alternative<c4c::codegen::lir::LirSelectOp>(inst)) {
    fail_unsupported("non-ALU/non-branch/non-call/non-memory instructions");
  }
}

void validate_block(const c4c::codegen::lir::LirBlock& block) {
  for (const auto& inst : block.insts) {
    validate_inst(inst);
  }
}

void validate_function(const c4c::codegen::lir::LirFunction& function) {
  for (const auto& inst : function.alloca_insts) {
    validate_inst(inst);
  }
  for (const auto& block : function.blocks) {
    validate_block(block);
  }
}

void validate_module(const c4c::codegen::lir::LirModule& module) {
  for (const auto& function : module.functions) {
    validate_function(function);
  }
}

}  // namespace

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  validate_module(module);
  try {
    return c4c::backend::render_module(c4c::backend::adapt_minimal_module(module));
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return c4c::codegen::lir::print_llvm(module);
}

}  // namespace c4c::backend::aarch64

#include "emitter.hpp"

#include "../lir_adapter.hpp"

#include <stdexcept>

namespace c4c::backend::aarch64 {

std::string emit_module(const c4c::codegen::lir::LirModule& module) {
  try {
    return c4c::backend::render_module(
        c4c::backend::adapt_return_only_module(module));
  } catch (const std::invalid_argument& ex) {
    throw std::invalid_argument(
        std::string("aarch64 backend scaffold only supports the return-only LIR adapter slice: ") +
        ex.what());
  }
}

}  // namespace c4c::backend::aarch64

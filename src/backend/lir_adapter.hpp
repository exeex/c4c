#pragma once

#include "lowering/lir_to_backend_ir.hpp"

namespace c4c::backend {

inline BackendModule adapt_minimal_module(const c4c::codegen::lir::LirModule& module) {
  return lower_lir_to_backend_module(module);
}

}  // namespace c4c::backend

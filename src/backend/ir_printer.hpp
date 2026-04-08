#pragma once

#include <string>

namespace c4c::backend {

struct BackendModule;

std::string print_backend_module(const BackendModule& module);

// Compatibility shim for older callers during the Step 4 backend_ir cleanup.
inline std::string print_backend_ir(const BackendModule& module) {
  return print_backend_module(module);
}

}  // namespace c4c::backend

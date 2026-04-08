#pragma once

#include <string>

namespace c4c::backend {

struct BackendModule;

bool validate_backend_module(const BackendModule& module, std::string* error);

// Compatibility shim for older callers during the Step 4 backend_ir cleanup.
inline bool validate_backend_ir(const BackendModule& module, std::string* error) {
  return validate_backend_module(module, error);
}

}  // namespace c4c::backend

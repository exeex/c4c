#pragma once

#include "ir.hpp"
#include <string>

#include "target.hpp"

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {

BackendModule lower_to_backend_ir(const c4c::codegen::lir::LirModule& module);

struct BackendModuleInput {
  explicit BackendModuleInput(const BackendModule& backend_module,
                              const c4c::codegen::lir::LirModule* legacy_fallback_in = nullptr)
      : owned_module(backend_module),
        module(&owned_module),
        legacy_fallback(legacy_fallback_in) {}

  explicit BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
      : owned_module(),
        module(nullptr),
        legacy_fallback(&lir_module) {}

  BackendModule owned_module;
  const BackendModule* module = nullptr;
  const c4c::codegen::lir::LirModule* legacy_fallback = nullptr;
};

struct BackendOptions {
  Target target;
};

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

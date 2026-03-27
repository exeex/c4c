#pragma once

#include <string>

#include "target.hpp"

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {

struct BackendModuleInput {
  const c4c::codegen::lir::LirModule& module;
};

struct BackendOptions {
  Target target;
};

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

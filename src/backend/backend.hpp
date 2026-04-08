#pragma once

#include "bir.hpp"
#include <string>
#include <variant>

#include "target.hpp"

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {

struct BackendModuleInput {
  explicit BackendModuleInput(const bir::Module& bir_module);
  explicit BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module);
  BackendModuleInput(BackendModuleInput&&) noexcept = default;
  BackendModuleInput& operator=(BackendModuleInput&&) noexcept = default;
  BackendModuleInput(const BackendModuleInput&) = default;
  BackendModuleInput& operator=(const BackendModuleInput&) = default;
  ~BackendModuleInput() = default;

  const bir::Module* bir_module() const {
    return std::get_if<bir::Module>(&module_);
  }
  const c4c::codegen::lir::LirModule* lir_module() const {
    if (const auto* lir_module =
            std::get_if<const c4c::codegen::lir::LirModule*>(&module_)) {
      return *lir_module;
    }
    return nullptr;
  }

 private:
  std::variant<bir::Module, const c4c::codegen::lir::LirModule*> module_;
};

struct BackendOptions {
  Target target;
};

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

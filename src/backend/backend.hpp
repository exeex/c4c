#pragma once

#include "bir.hpp"
#include <memory>
#include <string>

#include "target.hpp"

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {

enum class BackendLoweringRoute : unsigned char {
  BirPreloweredModule,
  BirFromLirEntry,
};

struct BackendModuleInput {
  explicit BackendModuleInput(const bir::Module& bir_module);
  explicit BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module);
  BackendModuleInput(BackendModuleInput&&) noexcept;
  BackendModuleInput& operator=(BackendModuleInput&&) noexcept;
  BackendModuleInput(const BackendModuleInput&) = delete;
  BackendModuleInput& operator=(const BackendModuleInput&) = delete;
  ~BackendModuleInput();

  const bir::Module* bir_module() const { return bir_module_; }
  const c4c::codegen::lir::LirModule* lir_module() const { return lir_module_; }

 private:
  std::unique_ptr<bir::Module> owned_bir_module_;
  const bir::Module* bir_module_ = nullptr;
  const c4c::codegen::lir::LirModule* lir_module_ = nullptr;
};

struct BackendOptions {
  Target target;
};

BackendLoweringRoute select_lowering_route(const BackendModuleInput& input,
                                           const BackendOptions& options);

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

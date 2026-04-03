#pragma once

#include "bir.hpp"
#include <memory>
#include <string>

#include "target.hpp"

namespace c4c::codegen::lir {
struct LirModule;
}

namespace c4c::backend {

struct BackendModule;

BackendModule lower_lir_to_backend_module(const c4c::codegen::lir::LirModule& module);

enum class BackendPipeline : unsigned char {
  Legacy,
  Bir,
};

struct BackendModuleInput {
  explicit BackendModuleInput(const BackendModule& backend_module,
                              const c4c::codegen::lir::LirModule* legacy_fallback_in = nullptr);
  explicit BackendModuleInput(const bir::Module& bir_module,
                              const c4c::codegen::lir::LirModule* legacy_fallback_in = nullptr);
  explicit BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module);
  BackendModuleInput(BackendModuleInput&&) noexcept;
  BackendModuleInput& operator=(BackendModuleInput&&) noexcept;
  BackendModuleInput(const BackendModuleInput&) = delete;
  BackendModuleInput& operator=(const BackendModuleInput&) = delete;
  ~BackendModuleInput();

  const BackendModule* legacy_module() const { return legacy_module_; }
  const bir::Module* bir_module() const { return bir_module_; }
  const c4c::codegen::lir::LirModule* legacy_fallback() const { return legacy_fallback_; }

 private:
  std::unique_ptr<BackendModule> owned_legacy_module_;
  std::unique_ptr<bir::Module> owned_bir_module_;
  const BackendModule* legacy_module_ = nullptr;
  const bir::Module* bir_module_ = nullptr;
  const c4c::codegen::lir::LirModule* legacy_fallback_ = nullptr;
};

struct BackendOptions {
  Target target;
  BackendPipeline pipeline = BackendPipeline::Legacy;
};

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

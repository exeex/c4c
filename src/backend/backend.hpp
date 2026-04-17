#pragma once

#include "bir/bir.hpp"
#include "bir/lir_to_bir.hpp"
#include "prealloc/prealloc.hpp"
#include "target.hpp"

#include <functional>
#include <string>
#include <variant>

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

  bool holds_bir_module() const {
    return std::holds_alternative<bir::Module>(module_);
  }
  bool holds_lir_module() const {
    return std::holds_alternative<std::reference_wrapper<const c4c::codegen::lir::LirModule>>(
        module_);
  }

  const bir::Module& bir_module() const {
    return std::get<bir::Module>(module_);
  }
  const c4c::codegen::lir::LirModule& lir_module() const {
    return std::get<std::reference_wrapper<const c4c::codegen::lir::LirModule>>(module_)
        .get();
  }

 private:
  std::variant<bir::Module, std::reference_wrapper<const c4c::codegen::lir::LirModule>>
      module_;
};

struct BackendOptions {
  Target target;
  bool emit_semantic_bir = false;
};

struct BackendAssembleResult {
  std::string staged_text;
  std::string output_path;
  bool object_emitted = false;
  std::string error;
};

[[nodiscard]] c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    Target target);

// Current public BIR entrypoint. For x86_64 today this renders prepared
// semantic BIR text; it does not enter the target-local x86 backend emitter yet.
std::string emit_target_bir_module(const bir::Module& module, Target public_target);

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   Target public_target);

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    Target public_target,
    const std::string& output_path);

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options);

}  // namespace c4c::backend

#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "ir_printer.hpp"
#include "lir_adapter.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <memory>

namespace c4c::backend {

namespace {

class BackendEmitter {
 public:
  virtual ~BackendEmitter() = default;
  virtual std::string emit(const BackendModule& module,
                           const c4c::codegen::lir::LirModule* legacy_fallback) const = 0;
};

class Aarch64BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::aarch64::emit_module(module, legacy_fallback);
  }
};

class X86BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::x86::emit_module(module, legacy_fallback);
  }
};

class PassthroughBackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const BackendModule& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    if (legacy_fallback != nullptr) {
      return c4c::codegen::lir::print_llvm(*legacy_fallback);
    }
    return c4c::backend::print_backend_ir(module);
  }
};

std::unique_ptr<BackendEmitter> make_backend(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return std::make_unique<X86BackendEmitter>();
    case Target::Riscv64:
      return std::make_unique<PassthroughBackendEmitter>();
    case Target::Aarch64:
      return std::make_unique<Aarch64BackendEmitter>();
  }
  return nullptr;
}

std::string emit_legacy_module(const c4c::codegen::lir::LirModule& module,
                               Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_module(module);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_module(module);
    case Target::Riscv64:
      return c4c::codegen::lir::print_llvm(module);
  }
  return {};
}

}  // namespace

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.module == nullptr) {
    if (input.legacy_fallback == nullptr) {
      return {};
    }
    return emit_legacy_module(*input.legacy_fallback, options.target);
  }

  auto backend = make_backend(options.target);
  return backend->emit(*input.module, input.legacy_fallback);
}

}  // namespace c4c::backend

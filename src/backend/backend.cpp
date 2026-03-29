#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <memory>

namespace c4c::backend {

namespace {

class BackendEmitter {
 public:
  virtual ~BackendEmitter() = default;
  virtual std::string emit(const c4c::codegen::lir::LirModule& module) const = 0;
};

class Aarch64BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const c4c::codegen::lir::LirModule& module) const override {
    return c4c::backend::aarch64::emit_module(module);
  }
};

class X86BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const c4c::codegen::lir::LirModule& module) const override {
    return c4c::backend::x86::emit_module(module);
  }
};

class PassthroughBackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const c4c::codegen::lir::LirModule& module) const override {
    return c4c::codegen::lir::print_llvm(module);
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

}  // namespace

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  auto backend = make_backend(options.target);
  return backend->emit(input.module);
}

}  // namespace c4c::backend

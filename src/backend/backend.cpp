#include "backend.hpp"
#include "aarch64/emitter.hpp"

#include "../codegen/lir/ir.hpp"

#include <memory>
#include <stdexcept>

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

std::unique_ptr<BackendEmitter> make_backend(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
    case Target::Riscv64:
      throw std::invalid_argument(
          "backend ref-shape translation is currently only wired for AArch64");
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

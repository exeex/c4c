#include "backend.hpp"
#include "lir_adapter.hpp"

#include "../codegen/lir/ir.hpp"
#include "../codegen/lir/lir_printer.hpp"

#include <memory>
#include <stdexcept>

namespace c4c::backend {

namespace {

class BackendEmitter {
 public:
  virtual ~BackendEmitter() = default;
  virtual std::string emit(const c4c::codegen::lir::LirModule& module) const = 0;
};

class LlvmTextBackendEmitter final : public BackendEmitter {
 public:
  explicit LlvmTextBackendEmitter(Target target) : target_(target) {}

  std::string emit(const c4c::codegen::lir::LirModule& module) const override {
    (void)target_;
    try {
      auto adapted = adapt_return_only_module(module);
      return render_module(adapted);
    } catch (const std::invalid_argument&) {
      return c4c::codegen::lir::print_llvm(module);
    }
  }

 private:
  Target target_;
};

std::unique_ptr<BackendEmitter> make_backend(Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
    case Target::Aarch64:
    case Target::Riscv64:
      return std::make_unique<LlvmTextBackendEmitter>(target);
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

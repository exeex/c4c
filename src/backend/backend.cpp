#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "lowering/lir_to_backend_ir.hpp"
#include "lowering/lir_to_bir.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <stdexcept>
#include <memory>

namespace c4c::backend {

namespace {

class BackendEmitter {
 public:
  virtual ~BackendEmitter() = default;
  virtual std::string emit(const bir::Module& module,
                           const c4c::codegen::lir::LirModule* legacy_fallback) const = 0;
};

class Aarch64BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const bir::Module& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::aarch64::emit_module(module, legacy_fallback);
  }
};

class X86BackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const bir::Module& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    return c4c::backend::x86::emit_module(module, legacy_fallback);
  }
};

class PassthroughBackendEmitter final : public BackendEmitter {
 public:
  std::string emit(const bir::Module& module,
                   const c4c::codegen::lir::LirModule* legacy_fallback) const override {
    if (legacy_fallback != nullptr &&
        !c4c::backend::try_lower_to_bir(*legacy_fallback).has_value()) {
      return c4c::codegen::lir::print_llvm(*legacy_fallback);
    }
    return c4c::backend::bir::print(module);
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

std::string emit_direct_lir_or_llvm_fallback(const c4c::codegen::lir::LirModule& module,
                                             Target target) {
  try {
    switch (target) {
      case Target::X86_64:
      case Target::I686:
        return c4c::backend::x86::emit_module(module);
      case Target::Aarch64:
        return c4c::backend::aarch64::emit_module(module);
      case Target::Riscv64:
        return c4c::codegen::lir::print_llvm(module);
    }
  } catch (const c4c::backend::LirAdapterError& ex) {
    if (ex.kind() != c4c::backend::LirAdapterErrorKind::Unsupported) {
      throw;
    }
  }
  return c4c::codegen::lir::print_llvm(module);
}

bool is_direct_bir_subset_error(const std::invalid_argument& ex) {
  return std::string_view(ex.what()).find("does not support this direct BIR module") !=
         std::string_view::npos;
}

}  // namespace

BackendModuleInput::BackendModuleInput(
    const bir::Module& bir_module,
    const c4c::codegen::lir::LirModule* legacy_fallback_in)
    : owned_bir_module_(std::make_unique<bir::Module>(bir_module)),
      bir_module_(owned_bir_module_.get()),
      legacy_fallback_(legacy_fallback_in) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : legacy_fallback_(&lir_module) {}

BackendModuleInput::BackendModuleInput(BackendModuleInput&&) noexcept = default;
BackendModuleInput& BackendModuleInput::operator=(BackendModuleInput&&) noexcept = default;
BackendModuleInput::~BackendModuleInput() = default;

BackendLoweringRoute select_lowering_route(const BackendModuleInput& input,
                                           const BackendOptions& options) {
  (void)options;
  if (input.bir_module() != nullptr) {
    return BackendLoweringRoute::BirPreloweredModule;
  }
  return BackendLoweringRoute::BirFromLirEntry;
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  const auto route = select_lowering_route(input, options);
  if (route == BackendLoweringRoute::BirFromLirEntry) {
    if (input.legacy_fallback() == nullptr) {
      return {};
    }
    auto bir_module = c4c::backend::try_lower_to_bir(*input.legacy_fallback());
    if (!bir_module.has_value()) {
      return emit_direct_lir_or_llvm_fallback(*input.legacy_fallback(), options.target);
    }
    if (options.target == Target::Riscv64) {
      return c4c::backend::bir::print(*bir_module);
    }
    auto backend = make_backend(options.target);
    try {
      return backend->emit(*bir_module, nullptr);
    } catch (const std::invalid_argument& ex) {
      if (!is_direct_bir_subset_error(ex)) {
        throw;
      }
      return emit_direct_lir_or_llvm_fallback(*input.legacy_fallback(), options.target);
    }
  }

  if (route == BackendLoweringRoute::BirPreloweredModule) {
    if (options.target == Target::Riscv64) {
      return c4c::backend::bir::print(*input.bir_module());
    }
    auto backend = make_backend(options.target);
    return backend->emit(*input.bir_module(), input.legacy_fallback());
  }

  throw std::logic_error("unreachable backend lowering route");
}

}  // namespace c4c::backend

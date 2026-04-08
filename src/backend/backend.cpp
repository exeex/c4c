#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "lowering/lir_to_bir.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

#include <memory>
#include <stdexcept>

namespace c4c::backend {

namespace {

std::string emit_native_bir_module(const bir::Module& module, Target target) {
  switch (target) {
    case Target::X86_64:
    case Target::I686:
      return c4c::backend::x86::emit_module(module);
    case Target::Aarch64:
      return c4c::backend::aarch64::emit_module(module);
    case Target::Riscv64:
      throw std::logic_error("riscv64 native BIR emission is handled by the caller");
  }
  throw std::logic_error("unreachable backend target");
}

std::string emit_direct_lir_or_llvm_fallback(const c4c::codegen::lir::LirModule& module,
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
  throw std::logic_error("unreachable backend target");
}

bool is_direct_bir_subset_error(const std::invalid_argument& ex) {
  return std::string_view(ex.what()).find("does not support this direct BIR module") !=
         std::string_view::npos;
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : owned_bir_module_(std::make_unique<bir::Module>(bir_module)),
      bir_module_(owned_bir_module_.get()) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : lir_module_(&lir_module) {}

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
    if (input.lir_module() == nullptr) {
      return {};
    }
    const auto& lir_module = *input.lir_module();
    auto bir_module = c4c::backend::try_lower_to_bir(lir_module);
    if (!bir_module.has_value()) {
      return emit_direct_lir_or_llvm_fallback(lir_module, options.target);
    }
    if (options.target == Target::Riscv64) {
      if (!lir_module.globals.empty() ||
          !lir_module.string_pool.empty() ||
          !lir_module.extern_decls.empty()) {
        return c4c::codegen::lir::print_llvm(lir_module);
      }
      return c4c::backend::bir::print(*bir_module);
    }
    try {
      return emit_native_bir_module(*bir_module, options.target);
    } catch (const std::invalid_argument& ex) {
      if (!is_direct_bir_subset_error(ex)) {
        throw;
      }
      if (options.target == Target::Riscv64) {
        return emit_direct_lir_or_llvm_fallback(lir_module, options.target);
      }
      throw;
    }
  }

  if (route == BackendLoweringRoute::BirPreloweredModule) {
    if (options.target == Target::Riscv64) {
      return c4c::backend::bir::print(*input.bir_module());
    }
    return emit_native_bir_module(*input.bir_module(), options.target);
  }

  throw std::logic_error("unreachable backend lowering route");
}

}  // namespace c4c::backend

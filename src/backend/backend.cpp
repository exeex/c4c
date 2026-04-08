#include "backend.hpp"
#include "aarch64/codegen/emit.hpp"
#include "bir_printer.hpp"
#include "lowering/lir_to_bir.hpp"
#include "x86/codegen/emit.hpp"

#include "../codegen/lir/lir_printer.hpp"
#include "../codegen/lir/ir.hpp"

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

std::string render_bir_module(const bir::Module& module, Target target) {
  if (target == Target::Riscv64) {
    return c4c::backend::bir::print(module);
  }
  return emit_native_bir_module(module, target);
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    return render_bir_module(input.bir_module(), options.target);
  }

  const auto& lir_module = input.lir_module();
  auto bir_module = c4c::backend::try_lower_to_bir(lir_module);
  if (!bir_module.has_value()) {
    switch (options.target) {
      case Target::X86_64:
      case Target::I686:
        return c4c::backend::x86::emit_module(lir_module);
      case Target::Aarch64:
        return c4c::backend::aarch64::emit_module(lir_module);
      case Target::Riscv64:
        return c4c::codegen::lir::print_llvm(lir_module);
    }
    throw std::logic_error("unreachable backend target");
  }
  if (options.target == Target::Riscv64) {
    if (!lir_module.globals.empty() ||
        !lir_module.string_pool.empty() ||
        !lir_module.extern_decls.empty()) {
      return c4c::codegen::lir::print_llvm(lir_module);
    }
  }
  return render_bir_module(*bir_module, options.target);
}

}  // namespace c4c::backend

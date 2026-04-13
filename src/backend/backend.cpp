#include "backend.hpp"

#include "bir_printer.hpp"

#include "../codegen/lir/lir_printer.hpp"

#include <stdexcept>

namespace c4c::backend {

namespace {

Target resolve_public_lir_target(const c4c::codegen::lir::LirModule& module,
                                 Target public_target) {
  if (public_target != Target::X86_64) {
    return public_target;
  }

  auto target = target_from_triple(module.target_triple);
  if (target != Target::I686) {
    target = Target::X86_64;
  }
  return target;
}

std::string emit_bootstrap_lir_module(const c4c::codegen::lir::LirModule& module,
                                      Target target) {
  (void)target;
  return c4c::codegen::lir::print_llvm(module);
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

c4c::codegen::lir::LirModule prepare_lir_module_for_target(
    const c4c::codegen::lir::LirModule& module,
    Target target) {
  return c4c::backend::prepare::prepare_lir_module_with_options(module, target).module;
}

c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    Target target) {
  return c4c::backend::prepare::prepare_bir_module_with_options(module, target).module;
}

std::string emit_target_bir_module(const bir::Module& module, Target public_target) {
  (void)public_target;
  const auto prepared =
      c4c::backend::prepare::prepare_bir_module_with_options(module, public_target);
  return c4c::backend::bir::print(prepared.module);
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   Target public_target) {
  const auto target = resolve_public_lir_target(module, public_target);
  return emit_module(BackendModuleInput{module}, BackendOptions{.target = target});
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    Target public_target,
    const std::string& output_path) {
  const auto emitted = emit_target_lir_module(module, public_target);
  return BackendAssembleResult{
      .staged_text = emitted,
      .output_path = output_path,
      .object_emitted = false,
      .error = "backend bootstrap mode does not assemble objects yet",
  };
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    return emit_target_bir_module(input.bir_module(), options.target);
  }

  const auto& lir_module = input.lir_module();
  const auto target = resolve_public_lir_target(lir_module, options.target);

  auto lowering = c4c::backend::try_lower_to_bir_with_options(
      lir_module, c4c::backend::BirLoweringOptions{});
  auto prepared = c4c::backend::prepare::prepare_lir_module_with_options(
      lir_module, target, c4c::backend::prepare::PrepareOptions{});

  if (lowering.module.has_value()) {
    const auto prepared_bir = c4c::backend::prepare::prepare_bir_module_with_options(
        *lowering.module, target, c4c::backend::prepare::PrepareOptions{});
    return c4c::backend::bir::print(prepared_bir.module);
  }
  return emit_bootstrap_lir_module(prepared.module, target);
}

}  // namespace c4c::backend

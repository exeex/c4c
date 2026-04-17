#include "backend.hpp"

#include "bir/bir_printer.hpp"
#include "mir/x86/codegen/x86_codegen.hpp"

#include "../codegen/lir/lir_printer.hpp"

#include <optional>

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

bool is_x86_target(Target target) {
  return target == Target::X86_64 || target == Target::I686;
}

bool is_x86_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::X86_64 ||
         target_profile.arch == c4c::TargetArch::I686;
}

// The active public backend entry still stops at prepared semantic BIR text.
// Keep this helper name honest until x86 is wired to a real backend-side handoff.
std::string render_prepared_bir_text(const c4c::backend::bir::Module& module) {
  return c4c::backend::bir::print(module);
}

c4c::backend::prepare::PreparedBirModule prepare_semantic_bir_pipeline(
    const c4c::backend::bir::Module& module,
    Target target) {
  return c4c::backend::prepare::prepare_semantic_bir_module_with_options(
      module, c4c::backend::target_profile_from_backend_target(target, module.target_triple));
}

c4c::backend::prepare::PreparedBirModule prepare_semantic_bir_pipeline(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile) {
  return c4c::backend::prepare::prepare_semantic_bir_module_with_options(module, target_profile);
}

}  // namespace

BackendModuleInput::BackendModuleInput(const bir::Module& bir_module)
    : module_(bir_module) {}

BackendModuleInput::BackendModuleInput(const c4c::codegen::lir::LirModule& lir_module)
    : module_(std::cref(lir_module)) {}

c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    const c4c::TargetProfile& target_profile) {
  return prepare_semantic_bir_pipeline(module, target_profile).module;
}

c4c::backend::bir::Module prepare_bir_module_for_target(
    const c4c::backend::bir::Module& module,
    Target target) {
  return prepare_semantic_bir_pipeline(module, target).module;
}

std::string emit_target_bir_module(const bir::Module& module,
                                   const c4c::TargetProfile& target_profile) {
  const auto prepared = prepare_semantic_bir_pipeline(module, target_profile);
  if (is_x86_target(target_profile)) {
    return c4c::backend::x86::emit_prepared_module(prepared);
  }
  return render_prepared_bir_text(prepared.module);
}

std::string emit_target_bir_module(const bir::Module& module, Target public_target) {
  return emit_target_bir_module(
      module, c4c::backend::target_profile_from_backend_target(public_target, module.target_triple));
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   const c4c::TargetProfile& target_profile) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (lowering.module.has_value()) {
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    if (is_x86_target(target_profile)) {
      return c4c::backend::x86::emit_prepared_module(prepared_bir);
    }
    return render_prepared_bir_text(prepared_bir.module);
  }
  return emit_bootstrap_lir_module(module, target_from_profile(target_profile));
}

std::string emit_target_lir_module(const c4c::codegen::lir::LirModule& module,
                                   Target public_target) {
  const auto target = resolve_public_lir_target(module, public_target);
  return emit_target_lir_module(module,
                                c4c::backend::target_profile_from_backend_target(
                                    target, module.target_triple));
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    const c4c::TargetProfile& target_profile,
    const std::string& output_path) {
  const auto emitted = emit_target_lir_module(module, target_profile);
  return BackendAssembleResult{
      .staged_text = emitted,
      .output_path = output_path,
      .object_emitted = false,
      .error = "backend bootstrap mode does not assemble objects yet",
  };
}

BackendAssembleResult assemble_target_lir_module(
    const c4c::codegen::lir::LirModule& module,
    Target public_target,
    const std::string& output_path) {
  return assemble_target_lir_module(
      module,
      c4c::backend::target_profile_from_backend_target(public_target, module.target_triple),
      output_path);
}

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(input.bir_module());
    }
    return emit_target_bir_module(
        input.bir_module(),
        c4c::backend::target_profile_from_backend_target(options.target,
                                                         input.bir_module().target_triple));
  }

  const auto& lir_module = input.lir_module();
  const auto target = resolve_public_lir_target(lir_module, options.target);

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = options.emit_semantic_bir;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);

  if (lowering.module.has_value()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(*lowering.module);
    }
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target);
    if (is_x86_target(target)) {
      return c4c::backend::x86::emit_prepared_module(prepared_bir);
    }
    return render_prepared_bir_text(prepared_bir.module);
  }
  return emit_bootstrap_lir_module(lir_module, target);
}

}  // namespace c4c::backend

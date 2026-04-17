#include "backend.hpp"

#include "bir/bir_printer.hpp"
#include "mir/x86/codegen/x86_codegen.hpp"

#include "../codegen/lir/lir_printer.hpp"

#include <optional>

namespace c4c::backend {

namespace {

const c4c::TargetProfile& profile_or_default(const c4c::TargetProfile& target_profile,
                                             c4c::TargetProfile& storage,
                                             std::string_view module_triple) {
  if (target_profile.arch != c4c::TargetArch::Unknown) {
    return target_profile;
  }
  storage = c4c::target_profile_from_triple(
      module_triple.empty() ? c4c::default_host_target_triple() : module_triple);
  return storage;
}

c4c::TargetProfile resolve_public_lir_target_profile(const c4c::codegen::lir::LirModule& module,
                                                     const c4c::TargetProfile& public_target) {
  c4c::TargetProfile module_profile_storage;
  const auto& requested = public_target.arch != c4c::TargetArch::Unknown
                              ? public_target
                              : (module.target_profile.arch != c4c::TargetArch::Unknown
                                     ? module.target_profile
                                     : profile_or_default(public_target,
                                                          module_profile_storage,
                                                          std::string_view{}));
  if (requested.arch != c4c::TargetArch::X86_64) {
    return requested;
  }

  if (module.target_profile.arch == c4c::TargetArch::Unknown) {
    return requested;
  }

  const auto module_profile = module.target_profile;
  if (module_profile.arch == c4c::TargetArch::I686) {
    return module_profile;
  }
  return requested;
}

std::string emit_bootstrap_lir_module(const c4c::codegen::lir::LirModule& module,
                                      const c4c::TargetProfile& target_profile) {
  (void)target_profile;
  return c4c::codegen::lir::print_llvm(module);
}

bool is_x86_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::X86_64 ||
         target_profile.arch == c4c::TargetArch::I686;
}

std::string make_x86_lir_handoff_failure_message(
    const c4c::backend::BirLoweringResult& lowering) {
  std::string message =
      "x86 backend emit path requires semantic lir_to_bir lowering before the canonical prepared-module handoff";
  for (auto it = lowering.notes.rbegin(); it != lowering.notes.rend(); ++it) {
    if (it->phase == "module" || it->phase == "function") {
      message += ": ";
      message += it->message;
      break;
    }
  }
  return message;
}

// The active public backend entry still stops at prepared semantic BIR text.
// Keep this helper name honest until x86 is wired to a real backend-side handoff.
std::string render_prepared_bir_text(const c4c::backend::bir::Module& module) {
  return c4c::backend::bir::print(module);
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

std::string emit_target_bir_module(const bir::Module& module,
                                   const c4c::TargetProfile& target_profile) {
  const auto prepared = prepare_semantic_bir_pipeline(module, target_profile);
  if (is_x86_target(target_profile)) {
    return c4c::backend::x86::emit_prepared_module(prepared);
  }
  return render_prepared_bir_text(prepared.module);
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
  if (is_x86_target(target_profile)) {
    throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
  }
  return emit_bootstrap_lir_module(module, target_profile);
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

std::string emit_module(const BackendModuleInput& input,
                        const BackendOptions& options) {
  if (input.holds_bir_module()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(input.bir_module());
    }
    c4c::TargetProfile target_profile_storage;
    return emit_target_bir_module(
        input.bir_module(),
        profile_or_default(options.target_profile,
                           target_profile_storage,
                           input.bir_module().target_triple));
  }

  const auto& lir_module = input.lir_module();
  const auto target_profile = resolve_public_lir_target_profile(lir_module, options.target_profile);

  c4c::backend::BirLoweringOptions lowering_options{};
  lowering_options.preserve_dynamic_alloca = options.emit_semantic_bir;
  auto lowering = c4c::backend::try_lower_to_bir_with_options(lir_module, lowering_options);

  if (lowering.module.has_value()) {
    if (options.emit_semantic_bir) {
      return c4c::backend::bir::print(*lowering.module);
    }
    const auto prepared_bir = prepare_semantic_bir_pipeline(*lowering.module, target_profile);
    if (is_x86_target(target_profile)) {
      return c4c::backend::x86::emit_prepared_module(prepared_bir);
    }
    return render_prepared_bir_text(prepared_bir.module);
  }
  if (is_x86_target(target_profile)) {
    throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
  }
  return emit_bootstrap_lir_module(lir_module, target_profile);
}

}  // namespace c4c::backend

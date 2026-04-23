#include "x86_codegen_api.hpp"

#include "../module/module_emit.hpp"

namespace c4c::backend::x86::api {

namespace {

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

}  // namespace

std::string emit_module(const c4c::backend::bir::Module& module,
                        const c4c::TargetProfile& target_profile) {
  return emit_prepared_module(
      c4c::backend::prepare::prepare_semantic_bir_module_with_options(
          module, target_profile, c4c::backend::prepare::PrepareOptions{}));
}

std::string emit_module(const c4c::codegen::lir::LirModule& module,
                        const c4c::TargetProfile& target_profile) {
  c4c::backend::BirLoweringOptions lowering_options{};
  auto lowering = c4c::backend::try_lower_to_bir_with_options(module, lowering_options);
  if (!lowering.module.has_value()) {
    throw std::invalid_argument(make_x86_lir_handoff_failure_message(lowering));
  }
  return emit_module(*lowering.module, target_profile);
}

std::string emit_prepared_module(const c4c::backend::prepare::PreparedBirModule& module) {
  return c4c::backend::x86::module::emit_prepared_module_text(module);
}

}  // namespace c4c::backend::x86::api

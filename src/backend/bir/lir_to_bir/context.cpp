#include "lowering.hpp"

#include <utility>

namespace c4c::backend {

void BirLoweringContext::note(std::string phase, std::string message) {
  notes.push_back(BirLoweringNote{
      .phase = std::move(phase),
      .message = std::move(message),
  });
}

BirLoweringContext make_lowering_context(const c4c::codegen::lir::LirModule& module,
                                         const BirLoweringOptions& options) {
  const auto target_profile = module.target_profile.arch == c4c::TargetArch::Unknown
                                  ? c4c::target_profile_from_triple(c4c::default_host_target_triple())
                                  : module.target_profile;
  return BirLoweringContext{
      .lir_module = module,
      .target_profile = target_profile,
      .options = options,
      .notes = {},
  };
}

}  // namespace c4c::backend

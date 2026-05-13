#include "module.hpp"

#include <utility>

namespace c4c::backend::aarch64::module {

BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile =
      c4c::backend::aarch64::abi::resolve_target_profile(prepared);
  if (auto error = c4c::backend::aarch64::abi::validate_prepared_module_handoff(prepared)) {
    return BuildResult{.module = std::nullopt, .error = std::move(error)};
  }
  return BuildResult{.module = Module{.prepared = &prepared, .target_profile = target_profile},
                     .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module

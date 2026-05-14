#include "module.hpp"

#include <optional>
#include <utility>

namespace c4c::backend::aarch64::module {

BuildResult build(const prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile = abi::resolve_target_profile(prepared);
  if (auto error = abi::validate_prepared_module_handoff(prepared)) {
    return BuildResult{.module = std::nullopt, .error = std::move(error)};
  }

  Module module{
      .prepared = &prepared,
      .target_profile = target_profile,
      .mir = MachineModule{},
      .data = ModuleDataRecords{},
      .compatibility = CompatibilityProjection{},
      .functions = {},
  };
  return BuildResult{.module = std::move(module), .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module

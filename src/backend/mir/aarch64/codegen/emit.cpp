#include "emit.hpp"

#include "compatibility_projection.hpp"
#include "traversal.hpp"

#include <optional>
#include <utility>

namespace c4c::backend::aarch64::codegen {

module::BuildResult build_module(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile = module::abi::resolve_target_profile(prepared);
  if (auto error = module::abi::validate_prepared_module_handoff(prepared)) {
    return module::BuildResult{.module = std::nullopt, .error = std::move(error)};
  }

  module::Module built_module{
      .prepared = &prepared,
      .target_profile = target_profile,
      .mir = module::MachineModule{},
      .data = module::ModuleDataRecords{},
      .compatibility = module::CompatibilityProjection{},
      .functions = {},
  };
  module::ModuleLoweringDiagnostics diagnostics;
  built_module.mir.functions =
      lower_prepared_functions(prepared, target_profile, diagnostics);
  built_module.functions =
      derive_compatibility_function_records(prepared, built_module.mir.functions);
  built_module.compatibility = derive_compatibility_projection(built_module.functions);
  return module::BuildResult{.module = std::move(built_module), .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::codegen

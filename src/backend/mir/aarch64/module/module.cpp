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
  ModuleLoweringDiagnostics diagnostics;
  module.mir.functions = lower_prepared_functions(prepared, target_profile, diagnostics);
  for (const auto& function : module.mir.functions) {
    module.functions.push_back(FunctionRecord{
        .function_name = function.function_name,
        .label = prepare::prepared_function_name(prepared.names, function.function_name),
        .mir = function,
        .machine_nodes = {},
    });
  }
  module.compatibility.functions = module.functions;
  return BuildResult{.module = std::move(module), .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module

#include "module.hpp"

#include <optional>
#include <utility>

namespace c4c::backend::aarch64::module {
namespace {

[[nodiscard]] std::vector<codegen::InstructionRecord> selected_compatibility_nodes(
    const MachineFunction& function) {
  std::vector<codegen::InstructionRecord> nodes;
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.instructions) {
      if (instruction.target.family == codegen::InstructionFamily::Return) {
        continue;
      }
      if (instruction.target.selection.status !=
          codegen::MachineNodeSelectionStatus::Selected) {
        continue;
      }
      nodes.push_back(instruction.target);
    }
  }
  return nodes;
}

}  // namespace

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
        .machine_nodes = selected_compatibility_nodes(function),
    });
  }
  module.compatibility.functions = module.functions;
  return BuildResult{.module = std::move(module), .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::module

#include "emit.hpp"

#include "traversal.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {
namespace {

[[nodiscard]] std::vector<InstructionRecord> selected_compatibility_nodes(
    const module::MachineFunction& function) {
  std::vector<InstructionRecord> nodes;
  for (const auto& block : function.blocks) {
    for (const auto& instruction : block.instructions) {
      if (instruction.target.family == InstructionFamily::Return) {
        continue;
      }
      if (instruction.target.selection.status != MachineNodeSelectionStatus::Selected) {
        continue;
      }
      nodes.push_back(instruction.target);
    }
  }
  return nodes;
}

}  // namespace

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
  for (const auto& function : built_module.mir.functions) {
    built_module.functions.push_back(module::FunctionRecord{
        .function_name = function.function_name,
        .label = module::prepare::prepared_function_name(prepared.names,
                                                         function.function_name),
        .mir = function,
        .machine_nodes = selected_compatibility_nodes(function),
    });
  }
  built_module.compatibility.functions = built_module.functions;
  return module::BuildResult{.module = std::move(built_module), .error = std::nullopt};
}

}  // namespace c4c::backend::aarch64::codegen

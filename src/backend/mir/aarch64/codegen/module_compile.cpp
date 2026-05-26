#include "module_compile.hpp"

#include "traversal.hpp"

#include <optional>
#include <utility>
#include <vector>

namespace c4c::backend::aarch64::codegen {

namespace {

[[nodiscard]] module::BuildResult rejected_handoff_result(
    module::abi::HandoffError error) {
  return module::BuildResult{.module = std::nullopt, .error = std::move(error)};
}

[[nodiscard]] module::Module make_module_shell(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const c4c::TargetProfile& target_profile) {
  return module::Module{
      .prepared = &prepared,
      .target_profile = target_profile,
      .mir = module::MachineModule{},
      .data = module::ModuleDataRecords{},
      .compatibility = module::CompatibilityProjection{},
      .functions = {},
  };
}

// Compatibility-only flat views for legacy MIR tests and migration callers.
// Terminal assembly printing must walk module::MachineModule through the shared
// MIR printer and MachineInstructionPrinter instead.
[[nodiscard]] std::vector<module::FunctionRecord> derive_compatibility_function_records(
    const c4c::backend::prepare::PreparedBirModule& prepared,
    const std::vector<module::MachineFunction>& functions) {
  std::vector<module::FunctionRecord> records;
  records.reserve(functions.size());
  for (const auto& function : functions) {
    records.push_back(module::FunctionRecord{
        .function_name = function.function_name,
        .label = module::prepare::prepared_function_name(prepared.names,
                                                         function.function_name),
        .mir = function,
        .machine_nodes = module::selected_machine_nodes(function),
    });
  }
  return records;
}

[[nodiscard]] module::CompatibilityProjection derive_compatibility_projection(
    const std::vector<module::FunctionRecord>& functions) {
  return module::CompatibilityProjection{.functions = functions};
}

void lower_module_body(
    module::Module& built_module,
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  module::ModuleLoweringDiagnostics diagnostics;
  built_module.mir.functions =
      lower_prepared_functions(prepared, built_module.target_profile, diagnostics);
  built_module.functions =
      derive_compatibility_function_records(prepared, built_module.mir.functions);
  built_module.compatibility = derive_compatibility_projection(built_module.functions);
}

[[nodiscard]] module::BuildResult accepted_module_result(module::Module built_module) {
  return module::BuildResult{.module = std::move(built_module), .error = std::nullopt};
}

}  // namespace

CompileResult build_module(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  const c4c::TargetProfile target_profile = module::abi::resolve_target_profile(prepared);
  if (auto error = module::abi::validate_prepared_module_handoff(prepared)) {
    return rejected_handoff_result(std::move(*error));
  }

  module::Module built_module = make_module_shell(prepared, target_profile);
  lower_module_body(built_module, prepared);
  return accepted_module_result(std::move(built_module));
}

CompileResult compile_prepared_module(
    const c4c::backend::prepare::PreparedBirModule& prepared) {
  return build_module(prepared);
}

}  // namespace c4c::backend::aarch64::codegen

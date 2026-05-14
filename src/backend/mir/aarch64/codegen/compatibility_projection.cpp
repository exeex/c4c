#include "compatibility_projection.hpp"

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

std::vector<module::FunctionRecord> derive_compatibility_function_records(
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
        .machine_nodes = selected_compatibility_nodes(function),
    });
  }
  return records;
}

module::CompatibilityProjection derive_compatibility_projection(
    const std::vector<module::FunctionRecord>& functions) {
  return module::CompatibilityProjection{.functions = functions};
}

}  // namespace c4c::backend::aarch64::codegen

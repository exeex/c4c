#include "module.hpp"

#include "../codegen/module_compile.hpp"

namespace c4c::backend::aarch64::module {

std::vector<codegen::InstructionRecord> selected_machine_nodes(
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

BuildResult build(const prepare::PreparedBirModule& prepared) {
  return codegen::build_module(prepared);
}

}  // namespace c4c::backend::aarch64::module

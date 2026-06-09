#include "module.hpp"

#include "../codegen/module_compile.hpp"
#include "../../../prealloc/value_locations.hpp"

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

const prepare::PreparedAfterCallResultLaneBinding*
find_prepared_after_call_result_lane_binding(const FunctionLoweringContext& context,
                                             std::size_t block_index,
                                             std::size_t instruction_index,
                                             prepare::PreparedValueId value_id) {
  return prepare::find_indexed_prepared_after_call_result_lane_binding(
      context.move_bundle_lookups,
      block_index,
      instruction_index,
      value_id);
}

BuildResult build(const prepare::PreparedBirModule& prepared) {
  return codegen::build_module(prepared);
}

}  // namespace c4c::backend::aarch64::module

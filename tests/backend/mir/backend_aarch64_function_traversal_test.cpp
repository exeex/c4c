#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_control_flow_with_two_functions() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto first_function = prepared.names.function_names.intern("first");
  const auto second_function = prepared.names.function_names.intern("second");
  const auto first_entry = prepared.names.block_labels.intern("first.entry");
  const auto first_exit = prepared.names.block_labels.intern("first.exit");
  const auto second_entry = prepared.names.block_labels.intern("second.entry");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = first_function,
      .blocks = {prepare::PreparedControlFlowBlock{
                     .block_label = first_entry,
                     .terminator_kind = bir::TerminatorKind::Branch,
                     .branch_target_label = first_exit,
                 },
                 prepare::PreparedControlFlowBlock{
                     .block_label = first_exit,
                     .terminator_kind = bir::TerminatorKind::Return,
                 }},
  });
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = second_function,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = second_entry,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });

  return prepared;
}

int build_prepared_module_traverses_functions_and_blocks_into_mir() {
  auto prepared = prepared_control_flow_with_two_functions();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared traversal module to build");
  }

  const auto& functions = result.module->mir.functions;
  if (functions.size() != 2) {
    return fail("expected traversal to create one MIR function per prepared function");
  }
  if (functions[0].function_name != prepared.control_flow.functions[0].function_name ||
      functions[1].function_name != prepared.control_flow.functions[1].function_name) {
    return fail("expected traversal to preserve prepared function order and identity");
  }
  if (functions[0].blocks.size() != 2 || functions[1].blocks.size() != 1) {
    return fail("expected traversal to preserve prepared block counts");
  }
  if (functions[0].blocks[0].block_label !=
          prepared.control_flow.functions[0].blocks[0].block_label ||
      functions[0].blocks[0].index != 0 ||
      functions[0].blocks[1].block_label !=
          prepared.control_flow.functions[0].blocks[1].block_label ||
      functions[0].blocks[1].index != 1 ||
      functions[1].blocks[0].block_label !=
          prepared.control_flow.functions[1].blocks[0].block_label ||
      functions[1].blocks[0].index != 0) {
    return fail("expected traversal to preserve block labels and prepared block indexes");
  }
  for (const auto& function : functions) {
    for (const auto& block : function.blocks) {
      if (!block.instructions.empty()) {
        return fail("expected traversal slice to leave instruction lowering empty");
      }
    }
  }
  if (result.module->functions.size() != functions.size() ||
      result.module->compatibility.functions.size() != functions.size()) {
    return fail("expected compatibility projections to be derived from canonical functions");
  }
  for (const auto& record : result.module->functions) {
    if (!record.machine_nodes.empty()) {
      return fail("expected traversal slice not to fake flat machine nodes");
    }
  }
  return 0;
}

}  // namespace

int main() {
  return build_prepared_module_traverses_functions_and_blocks_into_mir();
}

#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
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

prepare::PreparedBirModule prepared_control_flow_with_critical_edge_copy_obligation() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("critical_edge_copy");
  const auto predecessor = prepared.names.block_labels.intern("pred");
  const auto successor = prepared.names.block_labels.intern("succ");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = predecessor,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
      .parallel_copy_bundles =
          {prepare::PreparedParallelCopyBundle{
              .predecessor_label = predecessor,
              .successor_label = successor,
              .execution_site =
                  prepare::PreparedParallelCopyExecutionSite::CriticalEdge,
              .moves = {prepare::PreparedParallelCopyMove{}},
              .steps = {prepare::PreparedParallelCopyStep{}},
          }},
  });

  return prepared;
}

int compile_prepared_module_traverses_functions_and_blocks_into_mir() {
  auto prepared = prepared_control_flow_with_two_functions();

  const auto result = aarch64_codegen::compile_prepared_module(prepared);
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
  if (!functions[0].blocks[0].instructions.empty()) {
    return fail("expected unsupported branch terminator to remain diagnostic-only");
  }
  if (functions[0].blocks[1].instructions.size() != 1 ||
      functions[1].blocks[0].instructions.size() != 1) {
    return fail("expected prepared return terminators to lower to one instruction each");
  }
  const auto& first_return = functions[0].blocks[1].instructions.front();
  const auto& second_return = functions[1].blocks[0].instructions.front();
  if (!first_return.origin.has_value() || !second_return.origin.has_value() ||
      first_return.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      second_return.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      first_return.origin->block_label != functions[0].blocks[1].block_label ||
      second_return.origin->block_label != functions[1].blocks[0].block_label ||
      first_return.target.family !=
          aarch64_module::codegen::InstructionFamily::Return ||
      second_return.target.family !=
          aarch64_module::codegen::InstructionFamily::Return ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          first_return.target.payload) ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          second_return.target.payload)) {
    return fail("expected traversal returns to carry canonical return records");
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

int lower_prepared_functions_reports_shared_unplaced_copy_obligation() {
  auto prepared = prepared_control_flow_with_critical_edge_copy_obligation();
  aarch64_module::ModuleLoweringDiagnostics diagnostics;

  const auto functions = aarch64_codegen::lower_prepared_functions(
      prepared,
      c4c::default_target_profile(c4c::TargetArch::Aarch64),
      diagnostics);

  if (functions.size() != 1) {
    return fail("expected traversal to still produce the prepared function");
  }

  const auto& function = prepared.control_flow.functions.front();
  bool found_shared_obligation = false;
  for (const auto& diagnostic : diagnostics.entries) {
    if (diagnostic.kind != aarch64_module::ModuleLoweringDiagnosticKind::
                               PreparedObjectConsumerContractViolation) {
      continue;
    }
    if (diagnostic.function_name != function.function_name ||
        diagnostic.block_label != function.parallel_copy_bundles.front()
                                      .predecessor_label) {
      return fail("expected unplaced copy diagnostic to preserve target context");
    }
    if (!diagnostic.prepared_consumer_category.has_value() ||
        *diagnostic.prepared_consumer_category !=
            prepare::PreparedObjectConsumerDiagnosticCategory::
                UnsupportedParallelCopyExecutionSite) {
      return fail("expected AArch64 diagnostic to preserve shared category");
    }
    if (diagnostic.message.find(
            "prepared critical-edge parallel-copy obligation has no "
            "target-consumable block event") == std::string::npos) {
      return fail("expected AArch64 diagnostic to preserve shared message");
    }
    found_shared_obligation = true;
  }

  if (!found_shared_obligation) {
    return fail("expected AArch64 traversal to report shared unplaced obligation");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int result = compile_prepared_module_traverses_functions_and_blocks_into_mir();
      result != 0) {
    return result;
  }
  return lower_prepared_functions_reports_shared_unplaced_copy_obligation();
}

#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_return_block() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("return.fn");
  const auto entry_label = prepared.names.block_labels.intern("return.entry");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Return,
      }},
  });
  return prepared;
}

prepare::PreparedBirModule prepared_with_branch_block() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("branch.fn");
  const auto entry_label = prepared.names.block_labels.intern("branch.entry");
  const auto exit_label = prepared.names.block_labels.intern("branch.exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::Branch,
          .branch_target_label = exit_label,
      }},
  });
  return prepared;
}

int direct_dispatch_lowers_prepared_return_to_canonical_machine_instruction() {
  auto prepared = prepared_with_return_block();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 7);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected direct return dispatch to emit exactly one instruction");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected return instruction origin to preserve prepared identity");
  }
  if (instruction.target.family !=
          aarch64_module::codegen::InstructionFamily::Return ||
      instruction.target.surface !=
          aarch64_module::codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.target.selection.status !=
          aarch64_module::codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 7 ||
      !std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          instruction.target.payload)) {
    return fail("expected return instruction target to be canonical selected return record");
  }
  return 0;
}

int module_build_lowers_prepared_return_without_flat_compatibility_nodes() {
  auto prepared = prepared_with_return_block();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared return module to build");
  }

  const auto& module = *result.module;
  if (module.mir.functions.size() != 1 || module.mir.functions.front().blocks.size() != 1 ||
      module.mir.functions.front().blocks.front().instructions.size() != 1) {
    return fail("expected module build to lower one prepared return instruction");
  }
  if (!std::holds_alternative<aarch64_module::codegen::ReturnInstructionRecord>(
          module.mir.functions.front().blocks.front().instructions.front().target.payload)) {
    return fail("expected module MIR to carry return instruction target record");
  }
  if (module.functions.size() != 1 ||
      module.functions.front().mir.blocks.front().instructions.size() != 1 ||
      !module.functions.front().machine_nodes.empty() ||
      !module.compatibility.functions.front().machine_nodes.empty()) {
    return fail("expected compatibility projection to mirror MIR but not fake machine nodes");
  }
  return 0;
}

int unsupported_branch_terminator_stays_diagnostic_only() {
  auto prepared = prepared_with_branch_block();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty()) {
    return fail("expected unsupported branch terminator to emit no machine instruction");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().function_name != function_cf.function_name ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected unsupported branch to remain a typed diagnostic");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status =
          direct_dispatch_lowers_prepared_return_to_canonical_machine_instruction();
      status != 0) {
    return status;
  }
  if (const int status =
          module_build_lowers_prepared_return_without_flat_compatibility_nodes();
      status != 0) {
    return status;
  }
  if (const int status = unsupported_branch_terminator_stays_diagnostic_only();
      status != 0) {
    return status;
  }
  return 0;
}

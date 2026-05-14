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
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_with_unconditional_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("branch.fn");
  const auto entry_label = prepared.names.block_labels.intern("branch.entry");
  const auto exit_label = prepared.names.block_labels.intern("branch.exit");
  const auto function_link_name = prepared.module.names.link_names.intern("branch.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("branch.entry");
  const auto bir_exit_label = prepared.module.names.block_labels.intern("branch.exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = exit_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = exit_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
  });

  bir::Block entry;
  entry.label = "branch.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::BranchTerminator{
      .target_label = "branch.exit",
      .target_label_id = bir_exit_label,
  };

  bir::Block exit;
  exit.label = "branch.exit";
  exit.label_id = bir_exit_label;
  exit.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "branch.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(exit);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_conditional_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("cond.fn");
  const auto entry_label = prepared.names.block_labels.intern("cond.entry");
  const auto then_label = prepared.names.block_labels.intern("cond.then");
  const auto else_label = prepared.names.block_labels.intern("cond.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto function_link_name = prepared.module.names.link_names.intern("cond.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("cond.entry");
  const auto bir_then_label = prepared.module.names.block_labels.intern("cond.then");
  const auto bir_else_label = prepared.module.names.block_labels.intern("cond.else");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Eq,
          .compare_type = bir::TypeKind::I32,
          .lhs = bir::Value::immediate_i32(1),
          .rhs = bir::Value::immediate_i32(1),
          .can_fuse_with_branch = true,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{9},
          .function_name = function_name,
          .value_name = condition_name,
      }},
  });

  bir::Block entry;
  entry.label = "cond.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "cond.then",
      .false_label = "cond.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "cond.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

int direct_dispatch_lowers_unconditional_branch_to_selected_node() {
  auto prepared = prepared_with_unconditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_module::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_module::make_block_lowering_context(function_context, block_cf, 3);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_module::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected unconditional branch dispatch to emit one selected instruction");
  }
  if (block.successors.size() != 1 ||
      block.successors.front().target_label != function_cf.blocks[1].block_label ||
      block.successors.front().kind != mir::MachineBlockSuccessorKind::Unconditional ||
      !block.successors.front().origin.has_value() ||
      block.successors.front().origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors.front().origin->function_name != function_cf.function_name ||
      block.successors.front().origin->block_label != block_cf.block_label) {
    return fail("expected unconditional branch dispatch to record one typed successor");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::Branch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 3) {
    return fail("expected branch instruction target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || branch->conditional || branch->target_pair.has_value() ||
      !branch->condition_record.has_value() ||
      branch->condition_record->form != aarch64_codegen::BranchConditionForm::Unconditional ||
      branch->target.function_name != function_cf.function_name ||
      branch->target.block_label != function_cf.blocks[1].block_label ||
      branch->target.condition_value_id.has_value()) {
    return fail("expected branch payload to preserve prepared destination label identity");
  }
  return 0;
}

int module_build_keeps_branch_node_without_restoring_legacy_return_nodes() {
  auto prepared = prepared_with_unconditional_branch();
  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared branch module to build");
  }

  const auto& module = *result.module;
  if (module.mir.functions.size() != 1 || module.mir.functions.front().blocks.size() != 2 ||
      module.mir.functions.front().blocks[0].instructions.size() != 1 ||
      module.mir.functions.front().blocks[1].instructions.size() != 1 ||
      module.mir.functions.front().blocks[0].successors.size() != 1 ||
      module.mir.functions.front().blocks[1].successors.size() != 0) {
    return fail("expected module build to lower branch block and return block");
  }
  if (module.mir.functions.front().blocks[0].successors.front().target_label !=
      module.mir.functions.front().blocks[1].block_label) {
    return fail("expected module build to keep branch successor target label identity");
  }
  if (module.functions.front().machine_nodes.size() != 1 ||
      module.functions.front().machine_nodes.front().family !=
          aarch64_codegen::InstructionFamily::Branch ||
      module.compatibility.functions.front().machine_nodes.size() != 1) {
    return fail("expected compatibility nodes to include selected branch but not return");
  }
  return 0;
}

int conditional_branch_control_stays_fail_closed() {
  auto prepared = prepared_with_conditional_branch();
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
      result.emitted_instructions != 0 || !block.instructions.empty() ||
      !block.successors.empty()) {
    return fail("expected conditional branch to remain unsupported for this slice");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().function_name != function_cf.function_name ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected conditional branch to fail closed with a typed diagnostic");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = direct_dispatch_lowers_unconditional_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status =
          module_build_keeps_branch_node_without_restoring_legacy_return_nodes();
      status != 0) {
    return status;
  }
  if (const int status = conditional_branch_control_stays_fail_closed(); status != 0) {
    return status;
  }
  return 0;
}

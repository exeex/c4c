#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
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

prepare::PreparedBirModule prepared_with_fused_compare_conditional_branch(
    bool can_fuse_with_branch = true) {
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
          .can_fuse_with_branch = can_fuse_with_branch,
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

prepare::PreparedRegisterPlacement caller_saved_gpr(std::size_t slot_index) {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = slot_index,
      .contiguous_width = 1,
  };
}

prepare::PreparedBirModule prepared_with_materialized_bool_conditional_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("mb.fn");
  const auto entry_label = prepared.names.block_labels.intern("mb.entry");
  const auto then_label = prepared.names.block_labels.intern("mb.then");
  const auto else_label = prepared.names.block_labels.intern("mb.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto function_link_name = prepared.module.names.link_names.intern("mb.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("mb.entry");
  const auto bir_then_label = prepared.module.names.block_labels.intern("mb.then");
  const auto bir_else_label = prepared.module.names.block_labels.intern("mb.else");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = then_label,
                  .false_label = else_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = then_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = else_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
          .condition_value = condition,
          .can_fuse_with_branch = false,
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
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{9},
          .value_name = condition_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_placement = caller_saved_gpr(0),
      }},
  });

  bir::Block entry;
  entry.label = "mb.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "mb.then",
      .false_label = "mb.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Block then_block;
  then_block.label = "mb.then";
  then_block.label_id = bir_then_label;
  then_block.terminator = bir::ReturnTerminator{};

  bir::Block else_block;
  else_block.label = "mb.else";
  else_block.label_id = bir_else_label;
  else_block.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "mb.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(then_block);
  function.blocks.push_back(else_block);
  prepared.module.functions.push_back(function);
  return prepared;
}

int direct_dispatch_lowers_unconditional_branch_to_selected_node() {
  auto prepared = prepared_with_unconditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 3);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

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

int direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node() {
  auto prepared = prepared_with_materialized_bool_conditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 4);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected materialized-bool conditional branch to emit one selected instruction");
  }
  if (block.successors.size() != 2 ||
      block.successors[0].target_label != block_cf.true_label ||
      block.successors[0].kind != mir::MachineBlockSuccessorKind::ConditionalTrue ||
      block.successors[1].target_label != block_cf.false_label ||
      block.successors[1].kind != mir::MachineBlockSuccessorKind::ConditionalFalse ||
      !block.successors[0].origin.has_value() ||
      !block.successors[1].origin.has_value() ||
      block.successors[0].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[1].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[0].origin->function_name != function_cf.function_name ||
      block.successors[1].origin->block_label != block_cf.block_label) {
    return fail("expected materialized-bool branch dispatch to record true/false successors");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason != mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected conditional branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::ConditionalBranch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 4) {
    return fail("expected conditional branch target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || !branch->conditional || !branch->target_pair.has_value() ||
      !branch->condition.has_value() || !branch->condition_record.has_value() ||
      branch->condition_record->form != aarch64_codegen::BranchConditionForm::MaterializedBool ||
      branch->target_pair->true_target.block_label != block_cf.true_label ||
      branch->target_pair->false_target.block_label != block_cf.false_label) {
    return fail("expected conditional branch payload to preserve prepared condition targets");
  }
  if (branch->condition->kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected materialized-bool condition operand to be a typed register");
  }
  const auto* condition_register =
      std::get_if<aarch64_codegen::RegisterOperand>(&branch->condition->payload);
  if (condition_register == nullptr ||
      condition_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      !condition_register->value_id.has_value() ||
      *condition_register->value_id != prepare::PreparedValueId{9} ||
      condition_register->value_name != prepared.names.value_names.intern("%cond") ||
      !condition_register->expected_view.has_value() ||
      *condition_register->expected_view != aarch64_abi::RegisterView::W) {
    return fail("expected materialized-bool condition to resolve from typed register authority");
  }
  return 0;
}

int direct_dispatch_lowers_fusable_compare_branch_to_selected_node() {
  auto prepared = prepared_with_fused_compare_conditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 5);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected fusable fused-compare branch to emit one selected instruction");
  }
  if (block.successors.size() != 2 ||
      block.successors[0].target_label != block_cf.true_label ||
      block.successors[0].kind != mir::MachineBlockSuccessorKind::ConditionalTrue ||
      block.successors[1].target_label != block_cf.false_label ||
      block.successors[1].kind != mir::MachineBlockSuccessorKind::ConditionalFalse ||
      !block.successors[0].origin.has_value() ||
      !block.successors[1].origin.has_value() ||
      block.successors[0].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[1].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[0].origin->function_name != function_cf.function_name ||
      block.successors[1].origin->function_name != function_cf.function_name ||
      block.successors[0].origin->block_label != block_cf.block_label ||
      block.successors[1].origin->block_label != block_cf.block_label) {
    return fail("expected fusable compare branch dispatch to record true/false successors");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason != mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected compare branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::CompareBranch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 5) {
    return fail("expected compare branch target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || !branch->conditional || !branch->target_pair.has_value() ||
      !branch->condition_record.has_value() || branch->condition.has_value() ||
      branch->target_pair->true_target.block_label != block_cf.true_label ||
      branch->target_pair->false_target.block_label != block_cf.false_label) {
    return fail("expected compare branch payload to preserve prepared branch targets");
  }

  const auto& condition = *branch->condition_record;
  if (condition.form != aarch64_codegen::BranchConditionForm::FusedCompare ||
      !condition.can_fuse_with_branch || !condition.predicate.has_value() ||
      !condition.compare_operands.has_value() ||
      condition.predicate->source_predicate != bir::BinaryOpcode::Eq ||
      condition.predicate->compare_type != bir::TypeKind::I32 ||
      condition.compare_operands->compare_type != bir::TypeKind::I32 ||
      condition.compare_operands->lhs.source_value != bir::Value::immediate_i32(1) ||
      condition.compare_operands->rhs.source_value != bir::Value::immediate_i32(1)) {
    return fail("expected compare branch condition record to preserve compare facts");
  }
  if (!condition.compare_branch_candidate.has_value() ||
      condition.compare_branch_candidate->kind !=
          aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch ||
      !condition.compare_branch_candidate->can_fuse_with_branch ||
      !condition.compare_branch_candidate->target_pair.has_value()) {
    return fail("expected compare branch condition to retain fusable candidate metadata");
  }
  if (instruction.target.operands.size() != 5 ||
      instruction.target.operands[0].kind != aarch64_codegen::OperandKind::BranchTarget ||
      instruction.target.operands[1].kind != aarch64_codegen::OperandKind::BranchTarget ||
      instruction.target.operands[2].kind != aarch64_codegen::OperandKind::PreparedValue ||
      instruction.target.operands[3].kind != aarch64_codegen::OperandKind::Immediate ||
      instruction.target.operands[4].kind != aarch64_codegen::OperandKind::Immediate) {
    return fail("expected compare branch node operands to carry targets and compare inputs");
  }
  return 0;
}

int module_build_keeps_branch_node_without_restoring_legacy_return_nodes() {
  auto prepared = prepared_with_unconditional_branch();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
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

int non_fusable_compare_branch_control_stays_fail_closed() {
  auto prepared = prepared_with_fused_compare_conditional_branch(false);
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty() ||
      !block.successors.empty()) {
    return fail("expected non-fusable compare branch to remain unsupported for this slice");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().function_name != function_cf.function_name ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected non-fusable compare branch to fail closed with a typed diagnostic");
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
  if (const int status =
          direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_fusable_compare_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status = non_fusable_compare_branch_control_stays_fail_closed();
      status != 0) {
    return status;
  }
  return 0;
}

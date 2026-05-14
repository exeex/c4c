#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <iostream>
#include <string_view>
#include <unordered_map>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

c4c::TargetProfile riscv_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::Riscv64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedControlFlowFunction* find_control_flow_function(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const c4c::FunctionNameId function_name_id =
      prepared.names.function_names.find(function_name);
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_control_flow_function(
      prepared.control_flow, function_name_id);
}

prepare::PreparedControlFlowFunction* find_control_flow_function(
    prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const c4c::FunctionNameId function_name_id =
      prepared.names.function_names.find(function_name);
  if (function_name_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  for (auto& function : prepared.control_flow.functions) {
    if (function.function_name == function_name_id) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedParallelCopyBundle* find_parallel_copy_bundle(
    prepare::PreparedBirModule& prepared,
    const char* function_name,
    const char* predecessor_label,
    const char* successor_label) {
  auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return nullptr;
  }
  const auto predecessor_id = prepared.names.block_labels.find(predecessor_label);
  const auto successor_id = prepared.names.block_labels.find(successor_label);
  if (predecessor_id == c4c::kInvalidBlockLabel ||
      successor_id == c4c::kInvalidBlockLabel) {
    return nullptr;
  }
  for (auto& bundle : control_flow->parallel_copy_bundles) {
    if (bundle.predecessor_label == predecessor_id &&
        bundle.successor_label == successor_id) {
      return &bundle;
    }
  }
  return nullptr;
}

const prepare::PreparedBranchCondition* find_branch_condition(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedControlFlowFunction& control_flow,
    const char* block_label) {
  const auto block_label_id = prepare::resolve_prepared_block_label_id(
      prepared.names, block_label);
  if (!block_label_id.has_value()) {
    return nullptr;
  }
  return prepare::find_prepared_branch_condition(control_flow, *block_label_id);
}

bir::Function* find_function(prepare::PreparedBirModule& prepared, const char* function_name) {
  for (auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

bir::Block* find_block(bir::Function& function, const char* block_label) {
  for (auto& block : function.blocks) {
    if (block.label == block_label) {
      return &block;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule legalize_two_way_branch_join_module() {
  bir::Module module;

  bir::Function function;
  function.name = "branch_join_prepare_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks = {
      std::move(entry),
      std::move(is_zero),
      std::move(is_nonzero),
      std::move(join),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule legalize_short_circuit_or_guard_module() {
  bir::Module module;

  bir::Function function;
  function.name = "short_circuit_or_prepare_contract";
  function.return_type = bir::TypeKind::I32;
  function.local_slots.push_back(bir::LocalSlot{
      .name = "%lv.u.0",
      .type = bir::TypeKind::I32,
      .size_bytes = 4,
      .align_bytes = 4,
      .is_address_taken = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t0.addr",
      .value = bir::Value::immediate_i32(1),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "%t1.addr",
      .value = bir::Value::immediate_i32(3),
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .slot_name = "%t3.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t4"),
      .true_label = "logic.skip.8",
      .false_label = "logic.rhs.7",
  };

  bir::Block logic_rhs;
  logic_rhs.label = "logic.rhs.7";
  logic_rhs.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .slot_name = "%t12.addr",
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::LocalSlot,
          .base_name = "%lv.u.0",
          .size_bytes = 4,
          .align_bytes = 4,
      },
  });
  logic_rhs.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t13"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t12"),
      .rhs = bir::Value::immediate_i32(3),
  });
  logic_rhs.terminator = bir::BranchTerminator{.target_label = "logic.rhs.end.9"};

  bir::Block logic_rhs_end;
  logic_rhs_end.label = "logic.rhs.end.9";
  logic_rhs_end.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_skip;
  logic_skip.label = "logic.skip.8";
  logic_skip.terminator = bir::BranchTerminator{.target_label = "logic.end.10"};

  bir::Block logic_end;
  logic_end.label = "logic.end.10";
  logic_end.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t3"),
      .rhs = bir::Value::immediate_i32(3),
      .true_value = bir::Value::immediate_i32(1),
      .false_value = bir::Value::named(bir::TypeKind::I32, "%t13"),
  });
  logic_end.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%t17"),
      .rhs = bir::Value::immediate_i32(0),
  });
  logic_end.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "%t18"),
      .true_label = "block_1",
      .false_label = "block_2",
  };

  bir::Block block_1;
  block_1.label = "block_1";
  block_1.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(1)};

  bir::Block block_2;
  block_2.label = "block_2";
  block_2.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks = {
      std::move(entry),
      std::move(logic_rhs),
      std::move(logic_rhs_end),
      std::move(logic_skip),
      std::move(logic_end),
      std::move(block_1),
      std::move(block_2),
  };
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_out_of_ssa();
  return std::move(planner.prepared());
}

int check_authoritative_join_ownership(const prepare::PreparedBirModule& prepared,
                                       const char* function_name,
                                       const char* source_branch_label,
                                       const char* expected_join_label,
                                       const char* expected_true_predecessor,
                                       const char* expected_false_predecessor) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow publication for authoritative join ownership");
  }

  const auto* branch_condition =
      find_branch_condition(prepared, *control_flow, source_branch_label);
  if (branch_condition == nullptr) {
    return fail("expected authoritative source branch metadata");
  }

  const auto authoritative_join = prepare::find_authoritative_branch_owned_join_transfer(
      prepared.names, *control_flow, source_branch_label);
  if (!authoritative_join.has_value() || authoritative_join->join_transfer == nullptr ||
      authoritative_join->true_transfer == nullptr ||
      authoritative_join->false_transfer == nullptr) {
    return fail("expected authoritative join ownership to be recoverable from prepared metadata");
  }

  const auto& join_transfer = *authoritative_join->join_transfer;
  if (!join_transfer.source_branch_block_label.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) !=
          expected_join_label ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_branch_block_label) !=
          source_branch_label ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_true_incoming_label) !=
          expected_true_predecessor ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_false_incoming_label) !=
          expected_false_predecessor) {
    return fail("expected published join-transfer metadata to preserve authoritative ownership labels");
  }

  const auto true_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, branch_condition->true_label, join_transfer.join_block_label);
  const auto false_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, branch_condition->false_label, join_transfer.join_block_label);
  if (!true_lane.has_value() || !false_lane.has_value() ||
      *true_lane != authoritative_join->true_transfer->predecessor_label ||
      *false_lane != authoritative_join->false_transfer->predecessor_label ||
      prepare::prepared_block_label(prepared.names, *true_lane) != expected_true_predecessor ||
      prepare::prepared_block_label(prepared.names, *false_lane) != expected_false_predecessor) {
    return fail("expected prepared control-flow blocks to reproduce authoritative join predecessor ownership");
  }

  return 0;
}

int check_authoritative_join_continuation_targets() {
  auto prepared = legalize_short_circuit_or_guard_module();
  auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow publication for continuation target metadata");
  }

  const auto authoritative_join = prepare::find_authoritative_branch_owned_join_transfer(
      prepared.names, *control_flow, "entry");
  if (!authoritative_join.has_value() || authoritative_join->join_transfer == nullptr) {
    return fail("expected authoritative short-circuit join transfer for continuation target metadata");
  }

  const auto& join_transfer = *authoritative_join->join_transfer;
  if (!join_transfer.continuation_true_label.has_value() ||
      !join_transfer.continuation_false_label.has_value() ||
      prepare::prepared_block_label(prepared.names, *join_transfer.continuation_true_label) !=
          "block_1" ||
      prepare::prepared_block_label(prepared.names, *join_transfer.continuation_false_label) !=
          "block_2") {
    return fail("expected branch-owned join transfer to publish authoritative continuation labels");
  }

  auto* function = find_function(prepared, "short_circuit_or_prepare_contract");
  if (function == nullptr) {
    return fail("expected prepared short-circuit function for continuation target authority");
  }
  auto* join_block = find_block(*function, "logic.end.10");
  if (join_block == nullptr) {
    return fail("expected prepared short-circuit join block for continuation target authority");
  }
  join_block->insts.clear();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return prepare::prepared_block_label(prepared.names,
                                                            branch_condition.block_label) !=
                              "entry";
                     }),
      control_flow->branch_conditions.end());

  const auto continuation_targets = prepare::find_prepared_compare_join_continuation_targets(
      prepared.names, *control_flow, *function, "entry");
  if (!continuation_targets.has_value() ||
      prepare::prepared_block_label(prepared.names, continuation_targets->true_label) !=
          "block_1" ||
      prepare::prepared_block_label(prepared.names, continuation_targets->false_label) !=
          "block_2") {
    return fail("expected shared helper to prefer published continuation labels over recomputing join shape");
  }

  auto* mutable_control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (mutable_control_flow == nullptr) {
    return fail("expected mutable prepared short-circuit control flow for continuation target contract");
  }
  const auto authoritative_join_without_publication =
      prepare::find_authoritative_branch_owned_join_transfer(prepared.names,
                                                             *mutable_control_flow,
                                                             "entry");
  if (!authoritative_join_without_publication.has_value() ||
      authoritative_join_without_publication->join_transfer == nullptr) {
    return fail("expected authoritative join transfer before removing published continuation labels");
  }
  auto* published_join_transfer =
      const_cast<prepare::PreparedJoinTransfer*>(authoritative_join_without_publication->join_transfer);
  published_join_transfer->continuation_true_label.reset();
  published_join_transfer->continuation_false_label.reset();

  const auto unpublished_targets = prepare::find_prepared_compare_join_continuation_targets(
      prepared.names, *mutable_control_flow, *function, "entry");
  if (unpublished_targets.has_value()) {
    return fail("expected shared continuation-target helper to require published labels instead of recomputing authoritative join shape");
  }

  return 0;
}

int check_authoritative_parallel_copy_bundle_ownership(const prepare::PreparedBirModule& prepared,
                                                       const char* function_name,
                                                       const char* source_branch_label,
                                                       prepare::PreparedParallelCopyExecutionSite expected_true_site,
                                                       prepare::PreparedParallelCopyExecutionSite expected_false_site,
                                                       const char* expected_true_execution_block,
                                                       const char* expected_false_execution_block) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow publication for authoritative parallel-copy ownership");
  }

  const auto authoritative_bundles = prepare::find_authoritative_branch_owned_parallel_copy_bundles(
      prepared.names, *control_flow, source_branch_label);
  if (!authoritative_bundles.has_value() ||
      authoritative_bundles->join_transfer.join_transfer == nullptr ||
      authoritative_bundles->join_transfer.true_transfer == nullptr ||
      authoritative_bundles->join_transfer.false_transfer == nullptr ||
      authoritative_bundles->true_bundle == nullptr ||
      authoritative_bundles->false_bundle == nullptr) {
    return fail("expected authoritative parallel-copy bundle ownership to be recoverable from prepared metadata");
  }

  if (authoritative_bundles->true_bundle->predecessor_label !=
          authoritative_bundles->join_transfer.true_transfer->predecessor_label ||
      authoritative_bundles->true_bundle->successor_label !=
          authoritative_bundles->join_transfer.true_transfer->successor_label ||
      authoritative_bundles->false_bundle->predecessor_label !=
          authoritative_bundles->join_transfer.false_transfer->predecessor_label ||
      authoritative_bundles->false_bundle->successor_label !=
          authoritative_bundles->join_transfer.false_transfer->successor_label) {
    return fail("expected authoritative parallel-copy bundle helper to reuse published join-edge ownership");
  }

  if (authoritative_bundles->true_bundle->execution_site != expected_true_site ||
      authoritative_bundles->false_bundle->execution_site != expected_false_site) {
    return fail("expected authoritative parallel-copy bundle helper to preserve published execution-site ownership");
  }

  const auto true_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*authoritative_bundles->true_bundle);
  const auto false_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*authoritative_bundles->false_bundle);
  if ((expected_true_execution_block == nullptr) != !true_execution_block.has_value() ||
      (expected_false_execution_block == nullptr) != !false_execution_block.has_value()) {
    return fail("expected authoritative parallel-copy bundle helper to preserve direct execution-block ownership");
  }
  if ((expected_true_execution_block != nullptr &&
       prepare::prepared_block_label(prepared.names, *true_execution_block) !=
           expected_true_execution_block) ||
      (expected_false_execution_block != nullptr &&
       prepare::prepared_block_label(prepared.names, *false_execution_block) !=
           expected_false_execution_block)) {
    return fail("expected authoritative parallel-copy bundle helper to expose the published execution block directly");
  }

  return 0;
}

int check_authoritative_parallel_copy_execution_block_publication() {
  auto prepared = legalize_short_circuit_or_guard_module();
  auto* true_bundle = find_parallel_copy_bundle(
      prepared, "short_circuit_or_prepare_contract", "logic.skip.8", "logic.end.10");
  auto* false_bundle = find_parallel_copy_bundle(
      prepared, "short_circuit_or_prepare_contract", "logic.rhs.end.9", "logic.end.10");
  if (true_bundle == nullptr || false_bundle == nullptr) {
    return fail("expected mutable short-circuit bundles before removing execution-block publication");
  }

  const auto published_true_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*true_bundle);
  const auto published_false_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*false_bundle);
  if (!published_true_execution_block.has_value() ||
      !published_false_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *published_true_execution_block) !=
          "logic.skip.8" ||
      prepare::prepared_block_label(prepared.names, *published_false_execution_block) !=
          "logic.rhs.end.9") {
    return fail("expected short-circuit bundles to publish execution-block authority before removal");
  }

  true_bundle->execution_block_label.reset();
  false_bundle->execution_block_label.reset();

  const auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow after removing execution-block publication");
  }
  const auto authoritative_bundles = prepare::find_authoritative_branch_owned_parallel_copy_bundles(
      prepared.names, *control_flow, "entry");
  if (!authoritative_bundles.has_value() ||
      authoritative_bundles->true_bundle == nullptr ||
      authoritative_bundles->false_bundle == nullptr) {
    return fail("expected authoritative bundle lookup to remain available after removing execution-block publication");
  }
  if (prepare::published_prepared_parallel_copy_execution_block_label(
          *authoritative_bundles->true_bundle)
          .has_value() ||
      prepare::published_prepared_parallel_copy_execution_block_label(
          *authoritative_bundles->false_bundle)
          .has_value()) {
    return fail("expected authoritative bundle readers to require published execution-block labels instead of recomputing them");
  }

  return 0;
}

int check_authoritative_parallel_copy_bundle_lookup_requires_published_edge_ownership() {
  auto prepared = legalize_short_circuit_or_guard_module();
  auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow before removing published edge ownership");
  }

  const auto authoritative_join = prepare::find_authoritative_branch_owned_join_transfer(
      prepared.names, *control_flow, "entry");
  if (!authoritative_join.has_value() ||
      authoritative_join->true_transfer == nullptr ||
      authoritative_join->false_transfer == nullptr) {
    return fail("expected published branch-owned join transfer before removing edge ownership");
  }

  if (prepare::find_published_parallel_copy_bundle_for_edge_transfer(
          *control_flow, *authoritative_join->true_transfer) == nullptr ||
      prepare::find_published_parallel_copy_bundle_for_edge_transfer(
          *control_flow, *authoritative_join->false_transfer) == nullptr) {
    return fail("expected published bundle lookup seam to resolve branch-owned join bundles before mutation");
  }

  auto* mutable_true_transfer =
      const_cast<prepare::PreparedEdgeValueTransfer*>(authoritative_join->true_transfer);
  auto* mutable_false_transfer =
      const_cast<prepare::PreparedEdgeValueTransfer*>(authoritative_join->false_transfer);
  mutable_true_transfer->predecessor_label = c4c::kInvalidBlockLabel;
  mutable_false_transfer->successor_label = c4c::kInvalidBlockLabel;

  if (prepare::find_published_parallel_copy_bundle_for_edge_transfer(
          *control_flow, *mutable_true_transfer) != nullptr ||
      prepare::find_published_parallel_copy_bundle_for_edge_transfer(
          *control_flow, *mutable_false_transfer) != nullptr) {
    return fail("expected published bundle lookup seam to require published edge labels instead of recomputing bundle ownership");
  }

  const auto authoritative_bundles = prepare::find_authoritative_branch_owned_parallel_copy_bundles(
      prepared.names, *control_flow, "entry");
  if (authoritative_bundles.has_value()) {
    return fail("expected authoritative branch-owned bundle helper to fail once published edge ownership is removed");
  }

  return 0;
}

int check_materialized_compare_join_global_link_name_authority() {
  prepare::PreparedNameTables names;
  bir::NameTables bir_names;
  static_cast<void>(bir_names.link_names.intern("bir.domain.seed.a"));
  static_cast<void>(bir_names.link_names.intern("bir.domain.seed.b"));
  const c4c::LinkNameId global_id =
      bir_names.link_names.intern("same.module.global");
  const c4c::LinkNameId pointer_root_id =
      bir_names.link_names.intern("same.module.pointer");
  const c4c::LinkNameId other_global_id =
      bir_names.link_names.intern("other.module.global");
  const c4c::LinkNameId prepared_global_id =
      names.link_names.intern("same.module.global");
  if (prepared_global_id == global_id) {
    return fail("expected test fixture to keep BIR and prepared LinkNameId domains distinct");
  }

  bir::Function function;
  const std::unordered_map<std::string_view, const bir::BinaryInst*> named_binaries;

  bir::LoadGlobalInst direct_load{
      .result = bir::Value::named(bir::TypeKind::I32, "%direct"),
      .global_name = "same.module.global",
      .global_name_id = global_id,
  };
  std::unordered_map<std::string_view, const bir::LoadGlobalInst*> named_global_loads{
      {direct_load.result.name, &direct_load},
  };
  const auto direct_computed = prepare::classify_computed_value(
      names, bir_names, direct_load.result, function, named_binaries, named_global_loads);
  if (!direct_computed.has_value() ||
      direct_computed->base.kind != prepare::PreparedComputedBaseKind::GlobalI32Load ||
      direct_computed->base.global_name_id != prepared_global_id ||
      direct_computed->base.global_name != "same.module.global") {
    return fail("expected computed global load classification to resolve BIR LinkNameId authority into a prepared ID");
  }
  if (direct_computed->base.global_name.data() !=
      prepare::prepared_link_name(names, prepared_global_id).data()) {
    return fail("expected computed global load spelling to point into prepared name storage");
  }
  if (direct_computed->base.global_name.data() ==
      bir_names.link_names.spelling(global_id).data()) {
    return fail("expected computed global load spelling not to point into BIR name storage");
  }

  bir::LoadGlobalInst pointer_backed_load{
      .result = bir::Value::named(bir::TypeKind::I32, "%pointer_backed"),
      .global_name = "same.module.global",
      .global_name_id = global_id,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::GlobalSymbol,
          .base_name = "same.module.pointer",
          .base_link_name_id = pointer_root_id,
      },
  };
  named_global_loads = {
      {pointer_backed_load.result.name, &pointer_backed_load},
  };
  const auto pointer_backed_computed = prepare::classify_computed_value(
      names, bir_names, pointer_backed_load.result, function, named_binaries, named_global_loads);
  const c4c::LinkNameId prepared_pointer_root_id =
      names.link_names.find("same.module.pointer");
  if (!pointer_backed_computed.has_value() ||
      pointer_backed_computed->base.kind !=
          prepare::PreparedComputedBaseKind::PointerBackedGlobalI32Load ||
      pointer_backed_computed->base.global_name_id != prepared_global_id ||
      pointer_backed_computed->base.pointer_root_global_name_id != prepared_pointer_root_id ||
      pointer_backed_computed->base.pointer_root_global_name != "same.module.pointer") {
    return fail("expected pointer-backed computed global load classification to resolve address LinkNameId authority into prepared IDs");
  }
  if (pointer_backed_computed->base.pointer_root_global_name.data() !=
      prepare::prepared_link_name(names, prepared_pointer_root_id).data()) {
    return fail("expected pointer-root spelling to point into prepared name storage");
  }
  if (pointer_backed_computed->base.pointer_root_global_name.data() ==
      bir_names.link_names.spelling(pointer_root_id).data()) {
    return fail("expected pointer-root spelling not to point into BIR name storage");
  }

  bir::LoadGlobalInst mismatched_direct_load = direct_load;
  mismatched_direct_load.result = bir::Value::named(bir::TypeKind::I32, "%mismatch");
  mismatched_direct_load.global_name = "stale.raw.global";
  named_global_loads = {
      {mismatched_direct_load.result.name, &mismatched_direct_load},
  };
  if (prepare::classify_computed_value(
          names,
          bir_names,
          mismatched_direct_load.result,
          function,
          named_binaries,
          named_global_loads)
          .has_value()) {
    return fail("expected computed global load classification to reject LinkNameId/raw name disagreement");
  }

  bir::LoadGlobalInst mismatched_pointer_load = pointer_backed_load;
  mismatched_pointer_load.result =
      bir::Value::named(bir::TypeKind::I32, "%pointer_mismatch");
  mismatched_pointer_load.address->base_name = "stale.raw.pointer";
  named_global_loads = {
      {mismatched_pointer_load.result.name, &mismatched_pointer_load},
  };
  if (prepare::classify_computed_value(
          names,
          bir_names,
          mismatched_pointer_load.result,
          function,
          named_binaries,
          named_global_loads)
          .has_value()) {
    return fail("expected pointer-backed computed global load classification to reject address LinkNameId/raw name disagreement");
  }

  bir::Module module;
  module.names = bir_names;
  module.globals.push_back(bir::Global{
      .name = "same.module.global",
      .link_name_id = global_id,
      .type = bir::TypeKind::I32,
  });
  module.globals.push_back(bir::Global{
      .name = "same.module.pointer",
      .link_name_id = pointer_root_id,
      .type = bir::TypeKind::Ptr,
  });

  prepare::PreparedMaterializedCompareJoinRenderContract render_contract;
  render_contract.same_module_global_names = {"same.module.global"};
  render_contract.same_module_global_refs = {
      prepare::PreparedSameModuleGlobalRef{
          .name = "same.module.global",
          .name_id = prepared_global_id,
      },
  };
  const auto resolved_globals =
      prepare::resolve_prepared_materialized_compare_join_same_module_globals(
          names, module, render_contract);
  if (!resolved_globals.has_value() || resolved_globals->size() != 1 ||
      (*resolved_globals)[0] == nullptr || (*resolved_globals)[0]->link_name_id != global_id) {
    return fail("expected same-module global resolution to consume LinkNameId authority");
  }

  bir::Module mismatched_module;
  mismatched_module.names = bir_names;
  mismatched_module.globals.push_back(bir::Global{
      .name = "same.module.global",
      .link_name_id = other_global_id,
      .type = bir::TypeKind::I32,
  });
  if (prepare::resolve_prepared_materialized_compare_join_same_module_globals(
          names, mismatched_module, render_contract)
          .has_value()) {
    return fail("expected same-module global resolution to reject LinkNameId/raw name mismatch");
  }

  prepare::PreparedMaterializedCompareJoinRenderContract raw_compat_contract;
  raw_compat_contract.same_module_global_names = {"legacy.raw.global"};
  bir::Module raw_compat_module;
  raw_compat_module.names = bir_names;
  raw_compat_module.globals.push_back(bir::Global{
      .name = "legacy.raw.global",
      .type = bir::TypeKind::I32,
  });
  if (!prepare::resolve_prepared_materialized_compare_join_same_module_globals(
           names, raw_compat_module, raw_compat_contract)
           .has_value()) {
    return fail("expected raw same-module global name compatibility when LinkNameId is invalid");
  }

  prepare::PreparedMaterializedCompareJoinReturnArm pointer_return_arm;
  pointer_return_arm.context.selected_value = *pointer_backed_computed;
  pointer_return_arm.shape =
      prepare::classify_prepared_materialized_compare_join_return_shape(
          pointer_return_arm.context);
  const auto resolved_return = prepare::resolve_prepared_materialized_compare_join_return_arm(
      names, module, pointer_return_arm);
  if (!resolved_return.has_value() || resolved_return->global == nullptr ||
      resolved_return->pointer_root_global == nullptr ||
      resolved_return->global->link_name_id != global_id ||
      resolved_return->pointer_root_global->link_name_id != pointer_root_id) {
    return fail("expected materialized compare-join return-arm resolution to consume global LinkNameIds");
  }

  return 0;
}

}  // namespace

int main() {
  const auto prepared_two_way = legalize_two_way_branch_join_module();
  if (const int status = check_authoritative_join_ownership(prepared_two_way,
                                                            "branch_join_prepare_contract",
                                                            "entry",
                                                            "join",
                                                            "is_zero",
                                                            "is_nonzero");
      status != 0) {
    return status;
  }
  if (const int status = check_authoritative_parallel_copy_bundle_ownership(
          prepared_two_way,
          "branch_join_prepare_contract",
          "entry",
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          "is_zero",
          "is_nonzero");
      status != 0) {
    return status;
  }

  const auto prepared_short_circuit = legalize_short_circuit_or_guard_module();
  if (const int status = check_authoritative_join_ownership(prepared_short_circuit,
                                                            "short_circuit_or_prepare_contract",
                                                            "entry",
                                                            "logic.end.10",
                                                            "logic.skip.8",
                                                            "logic.rhs.end.9");
      status != 0) {
    return status;
  }
  if (const int status = check_authoritative_parallel_copy_bundle_ownership(
          prepared_short_circuit,
          "short_circuit_or_prepare_contract",
          "entry",
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator,
          "logic.skip.8",
          "logic.rhs.end.9");
      status != 0) {
    return status;
  }
  if (const int status = check_authoritative_parallel_copy_execution_block_publication();
      status != 0) {
    return status;
  }
  if (const int status =
          check_authoritative_parallel_copy_bundle_lookup_requires_published_edge_ownership();
      status != 0) {
    return status;
  }
  if (const int status = check_authoritative_join_continuation_targets(); status != 0) {
    return status;
  }
  if (const int status = check_materialized_compare_join_global_link_name_authority();
      status != 0) {
    return status;
  }

  return 0;
}

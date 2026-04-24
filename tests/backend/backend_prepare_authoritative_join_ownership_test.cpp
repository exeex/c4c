#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <iostream>

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
  if (const int status = check_authoritative_join_continuation_targets(); status != 0) {
    return status;
  }

  return 0;
}

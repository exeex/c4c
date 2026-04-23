#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

c4c::TargetProfile riscv_target_profile() {
  return c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");
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

prepare::PreparedBirModule legalize_loop_countdown_module() {
  bir::Module module;

  bir::Function function;
  function.name = "loop_countdown_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(3),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "next"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "counter"),
  };

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
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

prepare::PreparedBirModule legalize_parallel_copy_cycle_module() {
  bir::Module module;

  bir::Function function;
  function.name = "parallel_copy_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "a"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(1),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "b"),
          },
      },
  });
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "b"),
      .incomings = {
          bir::PhiIncoming{
              .label = "entry",
              .value = bir::Value::immediate_i32(2),
          },
          bir::PhiIncoming{
              .label = "body",
              .value = bir::Value::named(bir::TypeKind::I32, "a"),
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "a"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .true_label = "body",
      .false_label = "exit",
  };

  bir::Block body;
  body.label = "body";
  body.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "a"),
  };

  function.blocks = {
      std::move(entry),
      std::move(loop),
      std::move(body),
      std::move(exit),
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

bool expect_contains(const std::string& text,
                     const std::string& needle,
                     const char* description) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "[FAIL] missing " << description << ": " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
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

}  // namespace

int main() {
  auto prepared = legalize_short_circuit_or_guard_module();
  const std::string dump = prepare::print(prepared);

  if (!expect_contains(dump, "prepared.module target=riscv64-unknown-linux-gnu",
                       "module header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "--- prepared-bir ---", "prepared bir section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "prepared.func @short_circuit_or_prepare_contract",
                       "prepared function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "branch_condition entry", "branch condition entry")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "join_transfer logic.end.10", "join transfer")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "carrier=select_materialization",
                       "select materialization carrier")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump,
                       "ownership=authoritative_branch_pair incomings=2 edge_transfers=2",
                       "join transfer ownership summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_branch=entry", "source branch ownership")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "source_transfer_indexes=(0, 1)",
                       "join transfer source transfer indexes")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(dump, "continuation_targets=(block_1, block_2)",
                       "join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  auto* control_flow =
      find_control_flow_function(prepared, "short_circuit_or_prepare_contract");
  auto* function = find_function(prepared, "short_circuit_or_prepare_contract");
  if (control_flow == nullptr || function == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit control-flow fixture\n";
    return EXIT_FAILURE;
  }
  auto* join_block = find_block(*function, "logic.end.10");
  if (join_block == nullptr) {
    std::cerr << "[FAIL] missing prepared short-circuit join block fixture\n";
    return EXIT_FAILURE;
  }
  join_block->insts.clear();
  control_flow->branch_conditions.erase(
      std::remove_if(control_flow->branch_conditions.begin(),
                     control_flow->branch_conditions.end(),
                     [&](const prepare::PreparedBranchCondition& branch_condition) {
                       return prepare::prepared_block_label(prepared.names,
                                                            branch_condition.block_label) ==
                              "logic.rhs.7";
                     }),
      control_flow->branch_conditions.end());

  const std::string published_dump = prepare::print(prepared);
  if (!expect_contains(published_dump, "continuation_targets=(block_1, block_2)",
                       "published join transfer continuation targets")) {
    return EXIT_FAILURE;
  }

  const auto loop_prepared = legalize_loop_countdown_module();
  const std::string loop_dump = prepare::print(loop_prepared);
  if (!expect_contains(loop_dump, "prepared.func @loop_countdown_prepare_contract",
                       "loop printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "join_transfer loop result=counter kind=loop_carry carrier=edge_store_slot",
                       "loop-carry join header")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "ownership=per_edge incomings=2 edge_transfers=2 storage=counter.phi",
                       "slot-backed loop-carry authority")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [entry] -> 3", "loop entry incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump, "incoming [body] -> next", "loop backedge incoming")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(loop_dump,
                       "edge_transfer body -> loop incoming=next destination=counter storage=counter.phi",
                       "loop backedge edge transfer")) {
    return EXIT_FAILURE;
  }

  const auto parallel_copy_prepared = legalize_parallel_copy_cycle_module();
  const std::string parallel_copy_dump = prepare::print(parallel_copy_prepared);
  if (!expect_contains(parallel_copy_dump, "prepared.func @parallel_copy_prepare_contract",
                       "parallel-copy printer function section")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy entry -> loop has_cycle=no resolution=acyclic moves=2 steps=2",
                       "acyclic parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy body -> loop has_cycle=yes resolution=cycle_break moves=2 steps=3",
                       "cycle-break parallel-copy summary")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[0] save_destination_to_temp move_index=0 uses_cycle_temp_source=no",
                       "cycle temp save step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[1] move move_index=0 uses_cycle_temp_source=no",
                       "non-temp move step")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[2] move move_index=1 uses_cycle_temp_source=yes",
                       "temp-fed move step")) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

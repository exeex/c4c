#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/target.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

using c4c::backend::Target;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedLivenessFunction* find_liveness_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  for (const auto& function : prepared.liveness.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocFunction* find_regalloc_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  for (const auto& function : prepared.regalloc.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedLivenessValue* find_liveness_value(
    const prepare::PreparedLivenessFunction& function,
    std::string_view value_name) {
  for (const auto& value : function.values) {
    if (value.value_name == value_name) {
      return &value;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocValue* find_regalloc_value(
    const prepare::PreparedRegallocFunction& function,
    std::string_view value_name) {
  for (const auto& value : function.values) {
    if (value.value_name == value_name) {
      return &value;
    }
  }
  return nullptr;
}

const prepare::PreparedAllocationConstraint* find_constraint(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId value_id) {
  for (const auto& constraint : function.constraints) {
    if (constraint.value_id == value_id) {
      return &constraint;
    }
  }
  return nullptr;
}

bool has_interference_edge(const prepare::PreparedRegallocFunction& function,
                           prepare::PreparedValueId lhs_value_id,
                           prepare::PreparedValueId rhs_value_id) {
  for (const auto& edge : function.interference) {
    if ((edge.lhs_value_id == lhs_value_id && edge.rhs_value_id == rhs_value_id) ||
        (edge.lhs_value_id == rhs_value_id && edge.rhs_value_id == lhs_value_id)) {
      return true;
    }
  }
  return false;
}

bool has_spill_reload_op(const prepare::PreparedRegallocFunction& function,
                         prepare::PreparedValueId value_id,
                         prepare::PreparedSpillReloadOpKind kind) {
  for (const auto& op : function.spill_reload_ops) {
    if (op.value_id == value_id && op.op_kind == kind) {
      return true;
    }
  }
  return false;
}

prepare::PreparedBirModule prepare_phi_module() {
  bir::Module module;

  bir::Function function;
  function.name = "phi_liveness";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.flag",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.flag"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .true_label = "left",
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "right.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(20),
      .rhs = bir::Value::immediate_i32(2),
  });
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.v"),
      .incomings = {
          bir::PhiIncoming{.label = "left", .value = bir::Value::named(bir::TypeKind::I32, "left.v")},
          bir::PhiIncoming{.label = "right", .value = bir::Value::named(bir::TypeKind::I32, "right.v")},
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.v"),
      .rhs = bir::Value::immediate_i32(3),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "sum"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  planner.run_liveness();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_phi_module_with_regalloc() {
  auto prepared = prepare_phi_module();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_regalloc();
  return std::move(planner.prepared());
}

prepare::PreparedBirModule prepare_byval_home_slot_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "byval_home_slot";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "p.byval",
      .size_bytes = 8,
      .align_bytes = 8,
      .is_byval = true,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp0"),
      .operand_type = bir::TypeKind::Ptr,
      .lhs = bir::Value::named(bir::TypeKind::Ptr, "p.byval"),
      .rhs = bir::Value::named(bir::TypeKind::Ptr, "p.byval"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "cmp0"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_call_crossing_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "call_crossing_spillover";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(40),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "local0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(5),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "local1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(6),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "local0"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "local1"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge1"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_weighted_post_call_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "weighted_post_call_pressure";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(40),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_i32",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "low0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(5),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "low1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(6),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge.low"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "low0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "low1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "merge.low"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "result"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_evicted_spill_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "evicted_value_spill_ops";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(11),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_pair",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry0"),
               bir::Value::named(bir::TypeKind::I32, "carry1")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
                      .type = bir::TypeKind::I32,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .primary_class = bir::AbiValueClass::Integer,
                      .passed_in_register = true,
                  },
                  bir::CallArgAbiInfo{
                      .type = bir::TypeKind::I32,
                      .size_bytes = 4,
                      .align_bytes = 4,
                      .primary_class = bir::AbiValueClass::Integer,
                      .passed_in_register = true,
                  }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "local0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry0"),
      .rhs = bir::Value::immediate_i32(5),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot"),
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "carry1"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "result"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_loop_weighted_priority_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "loop_weighted_priority";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "seed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::BranchTerminator{.target_label = "loop"};

  bir::Block loop;
  loop.label = "loop";
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "loop.hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "seed"),
      .rhs = bir::Value::immediate_i32(2),
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "loop.keep"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loop.hot"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "seed"),
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "loop.cond"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loop.hot"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "loop.cond"),
      .true_label = "exit",
      .false_label = "loop",
  };

  bir::Block exit;
  exit.label = "exit";
  exit.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "loop.keep"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "seed"),
  });
  exit.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "result"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(loop));
  function.blocks.push_back(std::move(exit));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

int check_phi_predecessor_edge_liveness(const prepare::PreparedBirModule& prepared) {
  if (!prepared.stack_layout.objects.empty()) {
    return fail("expected no stack-layout objects for the phi-only test function");
  }

  const auto* function = find_liveness_function(prepared, "phi_liveness");
  if (function == nullptr) {
    return fail("expected liveness output for phi_liveness");
  }

  if (function->values.size() < 5) {
    return fail("expected liveness to track named BIR values independently of stack-layout objects");
  }

  const auto* left = find_liveness_value(*function, "left.v");
  const auto* right = find_liveness_value(*function, "right.v");
  const auto* phi = find_liveness_value(*function, "phi.v");
  const auto* sum = find_liveness_value(*function, "sum");
  if (left == nullptr || right == nullptr || phi == nullptr || sum == nullptr) {
    return fail("expected left.v, right.v, phi.v, and sum in prepared liveness");
  }

  if (left->definition_point != 2 || left->use_points != std::vector<std::size_t>{3}) {
    return fail("expected left.v to publish its definition point and predecessor-edge phi use point");
  }
  if (right->definition_point != 4 || right->use_points != std::vector<std::size_t>{5}) {
    return fail("expected right.v to publish its definition point and predecessor-edge phi use point");
  }
  if (phi->definition_point != 6 || phi->use_points != std::vector<std::size_t>{7}) {
    return fail("expected phi.v to publish its definition point and downstream use point");
  }
  if (sum->definition_point != 7 || sum->use_points != std::vector<std::size_t>{8}) {
    return fail("expected sum to publish its definition point and return use point");
  }

  if (!left->live_interval.has_value() || left->live_interval->start_point != 2 ||
      left->live_interval->end_point != 3) {
    return fail("expected left.v interval to end at the left predecessor terminator point");
  }
  if (!right->live_interval.has_value() || right->live_interval->start_point != 4 ||
      right->live_interval->end_point != 5) {
    return fail("expected right.v interval to end at the right predecessor terminator point");
  }
  if (!phi->live_interval.has_value() || phi->live_interval->start_point != 6 ||
      phi->live_interval->end_point != 7) {
    return fail("expected phi.v interval to begin at the phi definition point in the join block");
  }
  if (!sum->live_interval.has_value() || sum->live_interval->start_point != 7 ||
      sum->live_interval->end_point != 8) {
    return fail("expected sum interval to extend through the return terminator point");
  }

  if (phi->stack_object_id.has_value()) {
    return fail("expected phi.v to be a value-level record without a stack-object link");
  }
  if (function->call_points.size() != 0) {
    return fail("expected no call points in the phi-only test");
  }
  if (function->block_loop_depth.size() != 4) {
    return fail("expected loop-depth vector to cover all blocks");
  }

  return 0;
}

int check_phi_regalloc_seed_activation(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "phi_liveness");
  if (function == nullptr) {
    return fail("expected regalloc output for phi_liveness");
  }

  if (function->values.size() < 5) {
    return fail("expected regalloc to project value-level records from liveness");
  }

  const auto* phi = find_regalloc_value(*function, "phi.v");
  const auto* sum = find_regalloc_value(*function, "sum");
  const auto* left = find_regalloc_value(*function, "left.v");
  const auto* right = find_regalloc_value(*function, "right.v");
  if (phi == nullptr || sum == nullptr || left == nullptr || right == nullptr) {
    return fail("expected regalloc to seed left.v, right.v, phi.v, and sum from liveness");
  }

  if (phi->register_class != prepare::PreparedRegisterClass::General ||
      !phi->live_interval.has_value() || phi->live_interval->start_point != 6 ||
      phi->live_interval->end_point != 7) {
    return fail("expected phi.v regalloc seed to keep the liveness interval and a general-register class");
  }
  if (phi->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !phi->assigned_register.has_value() || !phi->spillable || phi->priority == 0 || phi->spill_weight <= 0.0) {
    return fail("expected phi.v regalloc seed to become an assigned register while publishing nonzero allocation priority");
  }
  if (left->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !left->assigned_register.has_value() ||
      right->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !right->assigned_register.has_value()) {
    return fail("expected left.v and right.v to reuse a real register assignment across disjoint live intervals");
  }
  if (left->assigned_register->register_name != right->assigned_register->register_name) {
    return fail("expected left.v and right.v to reuse the same register after interval expiry");
  }
  if (sum->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !sum->assigned_stack_slot.has_value()) {
    return fail("expected sum to receive a real stack-slot assignment when the active register is unavailable");
  }

  const auto* phi_constraint = find_constraint(*function, phi->value_id);
  if (phi_constraint == nullptr || phi_constraint->register_class != prepare::PreparedRegisterClass::General ||
      !phi_constraint->requires_register || phi_constraint->requires_home_slot ||
      phi_constraint->cannot_cross_call) {
    return fail("expected phi.v to publish a value-driven general-register allocation constraint");
  }

  if (!has_interference_edge(*function, phi->value_id, sum->value_id)) {
    return fail("expected phi.v and sum to interfere because their live intervals overlap");
  }
  if (has_interference_edge(*function, left->value_id, right->value_id)) {
    return fail("expected left.v and right.v to stay non-interfering because their live intervals are disjoint");
  }

  return 0;
}

int check_byval_home_slot_regalloc(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "byval_home_slot");
  if (function == nullptr) {
    return fail("expected regalloc output for byval_home_slot");
  }

  const auto* byval = find_regalloc_value(*function, "p.byval");
  const auto* cmp0 = find_regalloc_value(*function, "cmp0");
  if (byval == nullptr || cmp0 == nullptr) {
    return fail("expected byval param and compare result to appear in regalloc output");
  }

  if (byval->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !byval->assigned_stack_slot.has_value() || byval->assigned_register.has_value()) {
    return fail("expected the byval param to keep a real home-slot assignment instead of taking a register");
  }
  if (cmp0->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !cmp0->assigned_register.has_value()) {
    return fail("expected the compare result to receive a real register assignment");
  }

  const auto* byval_constraint = find_constraint(*function, byval->value_id);
  if (byval_constraint == nullptr || byval_constraint->register_class != prepare::PreparedRegisterClass::General ||
      byval_constraint->requires_register || !byval_constraint->requires_home_slot) {
    return fail("expected the byval param constraint to preserve the home-slot-only contract");
  }

  return 0;
}

int check_call_crossing_regalloc_spillover(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "call_crossing_spillover");
  if (function == nullptr) {
    return fail("expected regalloc output for call_crossing_spillover");
  }

  const auto* carry = find_regalloc_value(*function, "carry");
  const auto* local0 = find_regalloc_value(*function, "local0");
  const auto* local1 = find_regalloc_value(*function, "local1");
  if (carry == nullptr || local0 == nullptr || local1 == nullptr) {
    return fail("expected carry, local0, and local1 to appear in regalloc output");
  }

  if (!carry->crosses_call || !carry->assigned_register.has_value() ||
      carry->assigned_register->register_name != "s1") {
    return fail("expected the call-crossing carry value to take the first callee-saved register");
  }
  if (local0->crosses_call || !local0->assigned_register.has_value() ||
      local0->assigned_register->register_name != "t0") {
    return fail("expected the first post-call value to take the caller-saved seed register");
  }
  if (local1->crosses_call || !local1->assigned_register.has_value() ||
      local1->assigned_register->register_name != "s2") {
    return fail("expected caller-pool overflow to spill over into the remaining callee-saved register");
  }

  const auto* carry_constraint = find_constraint(*function, carry->value_id);
  const auto* local1_constraint = find_constraint(*function, local1->value_id);
  if (carry_constraint == nullptr || !carry_constraint->cannot_cross_call ||
      carry_constraint->preferred_register_names != std::vector<std::string>{"s1", "s2"} ||
      carry_constraint->forbidden_register_names != std::vector<std::string>{"t0"}) {
    return fail("expected the call-crossing constraint to prefer callee-saved registers and forbid the caller-saved seed");
  }
  if (local1_constraint == nullptr || local1_constraint->cannot_cross_call ||
      local1_constraint->preferred_register_names != std::vector<std::string>{"t0"}) {
    return fail("expected the non-call-crossing overflow value to keep the caller-saved preference before spillover");
  }

  return 0;
}

int check_weighted_post_call_regalloc(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "weighted_post_call_pressure");
  if (function == nullptr) {
    return fail("expected regalloc output for weighted_post_call_pressure");
  }

  const auto* carry = find_regalloc_value(*function, "carry");
  const auto* low0 = find_regalloc_value(*function, "low0");
  const auto* low1 = find_regalloc_value(*function, "low1");
  const auto* hot = find_regalloc_value(*function, "hot");
  if (carry == nullptr || low0 == nullptr || low1 == nullptr || hot == nullptr) {
    return fail("expected carry, low0, low1, and hot to appear in regalloc output");
  }

  if (!carry->assigned_register.has_value() || carry->assigned_register->register_name != "s1") {
    return fail("expected the call-crossing carry value to keep the protected callee-saved assignment");
  }
  if (hot->priority <= low0->priority || hot->priority <= low1->priority) {
    return fail("expected hot to publish a higher liveness-derived priority than the overflow locals");
  }
  if (!hot->assigned_register.has_value() || hot->assigned_register->register_name != "t0") {
    return fail("expected the higher-priority hot interval to evict into the caller-saved seed register");
  }
  if (!low0->assigned_register.has_value() || low0->assigned_register->register_name != "s2") {
    return fail("expected the displaced low0 interval to fall back to the remaining callee-saved spillover register");
  }
  if (low1->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !low1->assigned_stack_slot.has_value() || low1->assigned_register.has_value()) {
    return fail("expected the remaining lower-priority local to fall back to a real stack slot");
  }

  return 0;
}

int check_evicted_value_spill_ops(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "evicted_value_spill_ops");
  if (function == nullptr) {
    return fail("expected regalloc output for evicted_value_spill_ops");
  }

  const auto* carry0 = find_regalloc_value(*function, "carry0");
  const auto* carry1 = find_regalloc_value(*function, "carry1");
  const auto* local0 = find_regalloc_value(*function, "local0");
  const auto* hot = find_regalloc_value(*function, "hot");
  if (carry0 == nullptr || carry1 == nullptr || local0 == nullptr || hot == nullptr) {
    return fail("expected carry0, carry1, local0, and hot in evicted spill output");
  }

  if (!carry0->assigned_register.has_value() || carry0->assigned_register->register_name != "s1" ||
      !carry1->assigned_register.has_value() || carry1->assigned_register->register_name != "s2") {
    return fail("expected the call-crossing carries to occupy the protected callee-saved pool");
  }
  if (!hot->assigned_register.has_value() || hot->assigned_register->register_name != "t0") {
    return fail("expected the stronger hot interval to evict into the lone caller-saved seed register");
  }
  if (local0->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !local0->assigned_stack_slot.has_value() || local0->assigned_register.has_value()) {
    return fail("expected the evicted local0 value to fall back to a real stack slot");
  }
  if (!has_spill_reload_op(*function, local0->value_id, prepare::PreparedSpillReloadOpKind::Spill) ||
      !has_spill_reload_op(*function, local0->value_id, prepare::PreparedSpillReloadOpKind::Reload)) {
    return fail("expected the evicted local0 value to publish explicit spill and reload bookkeeping");
  }

  return 0;
}

int check_loop_weighted_priority(const prepare::PreparedBirModule& prepared) {
  const auto* liveness = find_liveness_function(prepared, "loop_weighted_priority");
  const auto* regalloc = find_regalloc_function(prepared, "loop_weighted_priority");
  if (liveness == nullptr || regalloc == nullptr) {
    return fail("expected liveness and regalloc output for loop_weighted_priority");
  }
  if (liveness->block_loop_depth.size() < 2 || liveness->block_loop_depth[1] == 0) {
    return fail("expected the loop block to publish nonzero loop depth");
  }

  const auto* liveness_hot = find_liveness_value(*liveness, "loop.hot");
  const auto* regalloc_hot = find_regalloc_value(*regalloc, "loop.hot");
  if (liveness_hot == nullptr || regalloc_hot == nullptr || !liveness_hot->live_interval.has_value()) {
    return fail("expected loop.hot to appear in both liveness and regalloc output with a live interval");
  }

  const std::size_t raw_priority_floor =
      liveness_hot->use_points.size() +
      (liveness_hot->live_interval->end_point - liveness_hot->live_interval->start_point) + 1U;
  if (regalloc_hot->priority <= raw_priority_floor ||
      regalloc_hot->spill_weight <= static_cast<double>(raw_priority_floor)) {
    return fail("expected loop.hot priority and spill weight to include loop-depth-weighted uses");
  }

  return 0;
}

}  // namespace

int main() {
  const auto liveness_prepared = prepare_phi_module();
  if (const int rc = check_phi_predecessor_edge_liveness(liveness_prepared); rc != 0) {
    return rc;
  }

  const auto regalloc_prepared = prepare_phi_module_with_regalloc();
  if (const int rc = check_phi_regalloc_seed_activation(regalloc_prepared); rc != 0) {
    return rc;
  }

  const auto byval_prepared = prepare_byval_home_slot_module_with_regalloc();
  if (const int rc = check_byval_home_slot_regalloc(byval_prepared); rc != 0) {
    return rc;
  }

  const auto call_crossing_prepared = prepare_call_crossing_module_with_regalloc();
  if (const int rc = check_call_crossing_regalloc_spillover(call_crossing_prepared); rc != 0) {
    return rc;
  }

  const auto weighted_post_call_prepared = prepare_weighted_post_call_module_with_regalloc();
  if (const int rc = check_weighted_post_call_regalloc(weighted_post_call_prepared); rc != 0) {
    return rc;
  }

  const auto evicted_spill_prepared = prepare_evicted_spill_module_with_regalloc();
  if (const int rc = check_evicted_value_spill_ops(evicted_spill_prepared); rc != 0) {
    return rc;
  }

  const auto loop_weighted_prepared = prepare_loop_weighted_priority_module_with_regalloc();
  if (const int rc = check_loop_weighted_priority(loop_weighted_prepared); rc != 0) {
    return rc;
  }
  return 0;
}

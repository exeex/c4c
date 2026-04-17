#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/codegen/lir/ir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/target.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

using c4c::backend::Target;
using c4c::backend::BirLoweringOptions;
using c4c::backend::try_lower_to_bir_with_options;
namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
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

const bir::Function* find_module_function(const prepare::PreparedBirModule& prepared,
                                          std::string_view function_name) {
  for (const auto& function : prepared.module.functions) {
    if (function.name == function_name) {
      return &function;
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

int count_spill_reload_ops(const prepare::PreparedRegallocFunction& function,
                           prepare::PreparedValueId value_id,
                           prepare::PreparedSpillReloadOpKind kind) {
  int count = 0;
  for (const auto& op : function.spill_reload_ops) {
    if (op.value_id == value_id && op.op_kind == kind) {
      ++count;
    }
  }
  return count;
}

const prepare::PreparedMoveResolution* find_move_resolution(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId from_value_id,
    prepare::PreparedValueId to_value_id) {
  for (const auto& move : function.move_resolution) {
    if (move.from_value_id == from_value_id && move.to_value_id == to_value_id) {
      return &move;
    }
  }
  return nullptr;
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

prepare::PreparedBirModule prepare_phi_join_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "phi_join_move_resolution";
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
      .result = bir::Value::named(bir::TypeKind::I32, "left.hot0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.hot1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(20),
      .rhs = bir::Value::immediate_i32(2),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.hot2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(30),
      .rhs = bir::Value::immediate_i32(3),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.feed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(40),
      .rhs = bir::Value::immediate_i32(4),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.keep0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "left.hot0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "left.hot1"),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.keep1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "left.hot1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "left.hot2"),
  });
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.keep2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "left.hot2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "left.hot0"),
  });
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "right.feed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(50),
      .rhs = bir::Value::immediate_i32(5),
  });
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.move"),
      .incomings = {
          bir::PhiIncoming{.label = "left",
                           .value = bir::Value::named(bir::TypeKind::I32, "left.feed")},
          bir::PhiIncoming{.label = "right",
                           .value = bir::Value::named(bir::TypeKind::I32, "right.feed")},
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "phi.use0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.move"),
      .rhs = bir::Value::immediate_i32(6),
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "phi.use1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.use0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "phi.move"),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "phi.use1"),
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
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
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

prepare::PreparedBirModule prepare_call_arg_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "call_arg_move_resolution";
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
      .lhs = bir::Value::immediate_i32(20),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "keep.arg"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(30),
      .rhs = bir::Value::immediate_i32(3),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "spill.arg"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(40),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "sink_quad",
      .args = {bir::Value::named(bir::TypeKind::I32, "carry0"),
               bir::Value::named(bir::TypeKind::I32, "carry1"),
               bir::Value::named(bir::TypeKind::I32, "keep.arg"),
               bir::Value::named(bir::TypeKind::I32, "spill.arg")},
      .arg_types = {bir::TypeKind::I32, bir::TypeKind::I32, bir::TypeKind::I32, bir::TypeKind::I32},
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
                  },
                  bir::CallArgAbiInfo{
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
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry0"),
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

prepare::PreparedBirModule prepare_call_result_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "call_result_move_resolution";
  function.return_type = bir::TypeKind::F32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::F32, "call.result"),
      .callee = "source_f32",
      .return_type_name = "f32",
      .return_type = bir::TypeKind::F32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::F32,
          .primary_class = bir::AbiValueClass::Sse,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F32, "call.result"),
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

prepare::PreparedBirModule prepare_return_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "return_move_resolution";
  function.return_type = bir::TypeKind::F32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "ret.value"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::immediate_f32_bits(0x3f800000U),
      .rhs = bir::Value::immediate_f32_bits(0x40000000U),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F32, "ret.value"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_return_same_storage_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "return_same_storage_resolution";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "ret.value"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(32),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "ret.value"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_legalize_aggregate_return_decl_module() {
  lir::LirModule module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  lir::LirFunction function;
  function.name = "aggregate_decl";
  function.is_declaration = true;
  function.signature_text = "declare { i32, i32 } @aggregate_decl()";
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target = Target::X86_64;

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  return std::move(planner.prepared());
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_call_result_module() {
  lir::LirModule module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  lir::LirExternDecl decl;
  decl.name = "source_f32";
  decl.return_type_str = "float";
  decl.return_type = "float";
  module.extern_decls.push_back(std::move(decl));

  lir::LirFunction function;
  function.name = "lowered_call_result_metadata";
  function.signature_text = "define float @lowered_call_result_metadata()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand("%call.result"),
      .return_type = "float",
      .callee = lir::LirOperand("@source_f32"),
      .callee_type_suffix = "()",
      .args_str = "",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%call.result"),
      .type_str = "float",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_legalize = true;
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
      .result = bir::Value::named(bir::TypeKind::I32, "late.merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "carry1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "late.merge"),
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

int check_phi_join_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "phi_join_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for phi_join_move_resolution");
  }

  const auto* left_feed = find_regalloc_value(*function, "left.feed");
  const auto* right_feed = find_regalloc_value(*function, "right.feed");
  const auto* phi = find_regalloc_value(*function, "phi.move");
  if (left_feed == nullptr || right_feed == nullptr || phi == nullptr) {
    return fail("expected phi join values to appear in regalloc output");
  }

  if (left_feed->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !left_feed->assigned_stack_slot.has_value() || left_feed->assigned_register.has_value()) {
    return fail("expected left.feed to fall back to a stack slot under join-side pressure");
  }
  if (phi->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !phi->assigned_register.has_value()) {
    return fail("expected the joined phi value to stay register-backed");
  }
  if (function->move_resolution.empty()) {
    return fail("expected the phi join to publish move-resolution bookkeeping");
  }

  const auto* move = find_move_resolution(*function, left_feed->value_id, phi->value_id);
  if (move == nullptr || move->block_index != 3 || move->instruction_index != 0 ||
      move->reason != "phi_join_stack_to_register") {
    return fail("expected the stack-backed left phi incoming to publish a join-time stack-to-register move");
  }
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      move->destination_abi_index.has_value() || move->destination_register_name.has_value()) {
    return fail("expected phi-join move resolution to keep the generic value destination surface");
  }
  if (right_feed->assigned_register.has_value() &&
      right_feed->assigned_register->register_name == phi->assigned_register->register_name &&
      find_move_resolution(*function, right_feed->value_id, phi->value_id) != nullptr) {
    return fail("expected matching register-backed phi incoming storage to skip redundant move resolution");
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
  const auto* carry_to_hot = find_move_resolution(*function, carry->value_id, hot->value_id);
  if (carry_to_hot == nullptr || carry_to_hot->block_index != 0 || carry_to_hot->instruction_index != 4 ||
      carry_to_hot->reason != "consumer_register_to_register") {
    return fail("expected the post-call carry use to publish a consumer-keyed register-to-register move record");
  }
  if (carry_to_hot->destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      carry_to_hot->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      carry_to_hot->destination_abi_index.has_value() ||
      carry_to_hot->destination_register_name.has_value()) {
    return fail("expected consumer move resolution to keep the generic value destination surface");
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
  if (count_spill_reload_ops(*function, local0->value_id, prepare::PreparedSpillReloadOpKind::Spill) != 1 ||
      count_spill_reload_ops(*function, local0->value_id, prepare::PreparedSpillReloadOpKind::Reload) != 2) {
    return fail("expected the evicted local0 value to publish one spill and a reload for each later use");
  }

  return 0;
}

int check_call_arg_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "call_arg_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for call_arg_move_resolution");
  }

  const auto* carry0 = find_regalloc_value(*function, "carry0");
  const auto* carry1 = find_regalloc_value(*function, "carry1");
  const auto* keep_arg = find_regalloc_value(*function, "keep.arg");
  const auto* spill_arg = find_regalloc_value(*function, "spill.arg");
  if (carry0 == nullptr || carry1 == nullptr || keep_arg == nullptr || spill_arg == nullptr) {
    return fail("expected call_arg_move_resolution values to appear in regalloc output");
  }

  if (!carry0->assigned_register.has_value() || carry0->assigned_register->register_name != "s1" ||
      !carry1->assigned_register.has_value() || carry1->assigned_register->register_name != "s2") {
    return fail("expected the call-crossing carry values to occupy the protected callee-saved pool");
  }
  if (!keep_arg->assigned_register.has_value() || keep_arg->assigned_register->register_name != "t0") {
    return fail("expected keep.arg to take the caller-saved argument register seed");
  }
  if (spill_arg->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !spill_arg->assigned_stack_slot.has_value() || spill_arg->assigned_register.has_value()) {
    return fail("expected spill.arg to fall back to a stack slot under call-argument pressure");
  }

  const auto* spill_move = find_move_resolution(*function, spill_arg->value_id, spill_arg->value_id);
  if (spill_move == nullptr || spill_move->block_index != 0 || spill_move->instruction_index != 4 ||
      spill_move->reason != "call_arg_stack_to_register") {
    return fail("expected the stack-backed named call argument to publish a call-site stack-to-register move");
  }
  if (spill_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      spill_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      spill_move->destination_abi_index != std::optional<std::size_t>{3} ||
      spill_move->destination_register_name != std::optional<std::string>{"a3"}) {
    return fail("expected the call-argument move to publish the concrete ABI argument destination");
  }
  if (find_move_resolution(*function, keep_arg->value_id, keep_arg->value_id) != nullptr) {
    return fail("expected register-backed call arguments to skip redundant call-site move resolution");
  }

  return 0;
}

int check_call_result_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "call_result_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for call_result_move_resolution");
  }

  const auto* call_result = find_regalloc_value(*function, "call.result");
  if (call_result == nullptr) {
    return fail("expected call.result to appear in regalloc output");
  }
  if (call_result->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !call_result->assigned_stack_slot.has_value() || call_result->assigned_register.has_value()) {
    return fail("expected the float call result to fall back to a stack slot when no float register seed exists");
  }

  const auto* move = find_move_resolution(*function, call_result->value_id, call_result->value_id);
  if (move == nullptr || move->block_index != 0 || move->instruction_index != 0 ||
      move->reason != "call_result_stack_to_register") {
    return fail("expected the stack-backed call result to publish a call-result stack-to-register move");
  }
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
      move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      move->destination_abi_index.has_value() ||
      move->destination_register_name != std::optional<std::string>{"fa0"}) {
    return fail("expected the call-result move to publish the concrete ABI return destination");
  }

  return 0;
}

int check_return_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "return_move_resolution");
  if (module_function == nullptr) {
    return fail("expected BIR output for return_move_resolution");
  }
  if (!module_function->return_abi.has_value() ||
      module_function->return_abi->type != bir::TypeKind::F32 ||
      module_function->return_abi->primary_class != bir::AbiValueClass::Sse) {
    return fail("expected return_move_resolution to publish explicit function return ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "return_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for return_move_resolution");
  }

  const auto* ret_value = find_regalloc_value(*function, "ret.value");
  if (ret_value == nullptr) {
    return fail("expected ret.value to appear in regalloc output");
  }
  if (ret_value->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !ret_value->assigned_stack_slot.has_value() || ret_value->assigned_register.has_value()) {
    return fail("expected the float return value to fall back to a stack slot when no float register seed exists");
  }

  const auto* move = find_move_resolution(*function, ret_value->value_id, ret_value->value_id);
  if (move == nullptr || move->block_index != 0 || move->instruction_index != 1 ||
      move->reason != "return_stack_to_register") {
    return fail("expected the stack-backed return value to publish a return-site stack-to-register move");
  }
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      move->destination_abi_index.has_value() ||
      move->destination_register_name != std::optional<std::string>{"fa0"}) {
    return fail("expected the return-site move to publish the function ABI destination surface");
  }

  return 0;
}

int check_return_same_storage_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "return_same_storage_resolution");
  if (module_function == nullptr || !module_function->return_abi.has_value() ||
      module_function->return_abi->type != bir::TypeKind::I32 ||
      module_function->return_abi->primary_class != bir::AbiValueClass::Integer) {
    return fail(
        "expected return_same_storage_resolution to preserve explicit function return ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "return_same_storage_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for return_same_storage_resolution");
  }

  const auto* ret_value = find_regalloc_value(*function, "ret.value");
  if (ret_value == nullptr) {
    return fail("expected ret.value to appear in regalloc output");
  }
  if (!ret_value->assigned_register.has_value() || ret_value->assigned_stack_slot.has_value()) {
    return fail("expected the integer return value to stay register-backed");
  }
  if (find_move_resolution(*function, ret_value->value_id, ret_value->value_id) != nullptr) {
    return fail("expected matching register-backed returns to skip redundant return-site move resolution");
  }

  return 0;
}

int check_lowered_aggregate_return_abi(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_module_function(prepared, "aggregate_decl");
  if (function == nullptr) {
    return fail("expected lowered aggregate_decl function metadata");
  }
  if (function->return_type != bir::TypeKind::Void || !function->is_declaration) {
    return fail("expected aggregate_decl to stay declaration-only with void lowered return storage");
  }
  if (function->params.empty() || !function->params.front().is_sret) {
    return fail("expected aggregate_decl lowering to publish an explicit sret parameter");
  }
  if (!function->return_abi.has_value() || !function->return_abi->returned_in_memory ||
      function->return_abi->primary_class != bir::AbiValueClass::Memory) {
    return fail("expected aggregate_decl to preserve lowering-owned memory return ABI metadata");
  }
  return 0;
}

int check_lowered_call_result_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "lowered_call_result_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail("expected lowered_call_result_metadata BIR output with one call");
  }

  const auto* call = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (call == nullptr || !call->result_abi.has_value() ||
      call->result_abi->type != bir::TypeKind::F32 ||
      call->result_abi->primary_class != bir::AbiValueClass::Sse) {
    return fail("expected lowered call to preserve explicit float result ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_call_result_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_call_result_metadata");
  }

  const auto* call_result = find_regalloc_value(*function, "%call.result");
  if (call_result == nullptr) {
    return fail("expected lowered call result to appear in regalloc output");
  }

  bool saw_call_result_move = false;
  for (const auto& move : function->move_resolution) {
    if (move.from_value_id != call_result->value_id || move.to_value_id != call_result->value_id ||
        move.reason != "call_result_stack_to_register") {
      continue;
    }
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.destination_abi_index.has_value() ||
        move.destination_register_name != std::optional<std::string>{"fa0"}) {
      return fail("expected lowered call-result move to target the concrete ABI return register");
    }
    saw_call_result_move = true;
    break;
  }
  if (!saw_call_result_move) {
    return fail("expected lowered call result to publish call-result move resolution");
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

  const auto phi_join_move_prepared = prepare_phi_join_move_module_with_regalloc();
  if (const int rc = check_phi_join_move_resolution(phi_join_move_prepared); rc != 0) {
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

  const auto call_arg_move_prepared = prepare_call_arg_move_module_with_regalloc();
  if (const int rc = check_call_arg_move_resolution(call_arg_move_prepared); rc != 0) {
    return rc;
  }

  const auto call_result_move_prepared = prepare_call_result_move_module_with_regalloc();
  if (const int rc = check_call_result_move_resolution(call_result_move_prepared); rc != 0) {
    return rc;
  }

  const auto return_move_prepared = prepare_return_move_module_with_regalloc();
  if (const int rc = check_return_move_resolution(return_move_prepared); rc != 0) {
    return rc;
  }

  const auto return_same_storage_prepared = prepare_return_same_storage_module_with_regalloc();
  if (const int rc = check_return_same_storage_resolution(return_same_storage_prepared); rc != 0) {
    return rc;
  }

  const auto aggregate_return_prepared = lower_and_legalize_aggregate_return_decl_module();
  if (!aggregate_return_prepared.has_value()) {
    return fail("expected aggregate declaration lowering to succeed");
  }
  if (const int rc = check_lowered_aggregate_return_abi(*aggregate_return_prepared); rc != 0) {
    return rc;
  }

  const auto lowered_call_result_prepared = lower_and_prepare_call_result_module();
  if (!lowered_call_result_prepared.has_value()) {
    return fail("expected lowered call-result module to succeed");
  }
  if (const int rc = check_lowered_call_result_abi(*lowered_call_result_prepared); rc != 0) {
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

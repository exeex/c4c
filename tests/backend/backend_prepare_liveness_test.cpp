#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/codegen/lir/ir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"
#include "src/target_profile.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>

namespace {

using c4c::backend::BirLoweringOptions;
using c4c::backend::try_lower_to_bir_with_options;
namespace bir = c4c::backend::bir;
namespace lir = c4c::codegen::lir;
namespace prepare = c4c::backend::prepare;

c4c::TargetProfile riscv_target_profile(
    std::string_view target_triple = "riscv64gc-unknown-linux-gnu") {
  return c4c::target_profile_from_triple(target_triple);
}

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const prepare::PreparedLivenessFunction* find_liveness_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  for (const auto& function : prepared.liveness.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocFunction* find_regalloc_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  for (const auto& function : prepared.regalloc.functions) {
    if (prepare::prepared_function_name(prepared.names, function.function_name) == function_name) {
      return &function;
    }
  }
  return nullptr;
}

const prepare::PreparedLivenessValue* find_liveness_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedLivenessFunction& function,
    std::string_view value_name) {
  for (const auto& value : function.values) {
    if (prepare::prepared_value_name(prepared.names, value.value_name) == value_name) {
      return &value;
    }
  }
  return nullptr;
}

const prepare::PreparedRegallocValue* find_regalloc_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedRegallocFunction& function,
    std::string_view value_name) {
  for (const auto& value : function.values) {
    if (prepare::prepared_value_name(prepared.names, value.value_name) == value_name) {
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

const prepare::PreparedMoveResolution* find_move_resolution_by_op_kind(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedMoveResolutionOpKind op_kind,
    prepare::PreparedValueId from_value_id,
    prepare::PreparedValueId to_value_id) {
  for (const auto& move : function.move_resolution) {
    if (move.op_kind == op_kind && move.from_value_id == from_value_id &&
        move.to_value_id == to_value_id) {
      return &move;
    }
  }
  return nullptr;
}

int count_move_resolution_reason_prefix_to_value(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId to_value_id,
    std::string_view reason_prefix) {
  int count = 0;
  for (const auto& move : function.move_resolution) {
    const std::string_view move_reason = move.reason;
    if (move.to_value_id == to_value_id &&
        move_reason.substr(0, reason_prefix.size()) == reason_prefix) {
      ++count;
    }
  }
  return count;
}

bool move_resolution_reason_has_prefix(const prepare::PreparedMoveResolution& move,
                                       std::string_view reason_prefix) {
  const std::string_view move_reason = move.reason;
  return move_reason.substr(0, reason_prefix.size()) == reason_prefix;
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
  prepared.target_profile = riscv_target_profile();

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
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep0"),
      .incomings = {
          bir::PhiIncoming{.label = "left",
                           .value = bir::Value::named(bir::TypeKind::I32, "left.hot0")},
          bir::PhiIncoming{.label = "right",
                           .value = bir::Value::named(bir::TypeKind::I32, "right.feed")},
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep1"),
      .incomings = {
          bir::PhiIncoming{.label = "left",
                           .value = bir::Value::named(bir::TypeKind::I32, "left.hot1")},
          bir::PhiIncoming{.label = "right",
                           .value = bir::Value::named(bir::TypeKind::I32, "right.feed")},
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep2"),
      .incomings = {
          bir::PhiIncoming{.label = "left",
                           .value = bir::Value::named(bir::TypeKind::I32, "left.hot2")},
          bir::PhiIncoming{.label = "right",
                           .value = bir::Value::named(bir::TypeKind::I32, "right.feed")},
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "phi.use0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.keep0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "phi.keep1"),
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "phi.use1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.keep2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "phi.move"),
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "phi.use2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "phi.use0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "phi.use1"),
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "phi.use2"),
  };

  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_phi_loop_cycle_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "phi_loop_cycle_move_resolution";
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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

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
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

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
  prepared.target_profile = x86_target_profile();

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
  module.target_profile = c4c::target_profile_from_triple("riscv64gc-unknown-linux-gnu");

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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_helper_call_result_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("riscv64gc-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "lowered_helper_call_result_metadata";
  function.signature_text = "define float @lowered_helper_call_result_metadata(float %arg)";
  function.params.emplace_back("%arg", c4c::TypeSpec{.base = c4c::TB_FLOAT});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand("%fabs.result"),
      .return_type = "float",
      .callee = lir::LirOperand("@llvm.fabs.float"),
      .callee_type_suffix = "(float)",
      .args_str = "float %arg",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%fabs.result"),
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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_soft_float_helper_call_result_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "lowered_helper_call_result_soft_float_metadata";
  function.signature_text = "define float @lowered_helper_call_result_soft_float_metadata(float %arg)";
  function.params.emplace_back("%arg", c4c::TypeSpec{.base = c4c::TB_FLOAT});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand("%fabs.result"),
      .return_type = "float",
      .callee = lir::LirOperand("@llvm.fabs.float"),
      .callee_type_suffix = "(float)",
      .args_str = "float %arg",
  });
  entry.terminator = lir::LirRet{
      .value_str = std::string("%fabs.result"),
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
  prepared.target_profile = module.target_profile;

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_helper_stackrestore_arg_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "lowered_helper_stackrestore_arg_metadata";
  function.signature_text = "define void @lowered_helper_stackrestore_arg_metadata()";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirStackSaveOp{
      .result = lir::LirOperand("%saved.ptr"),
  });
  entry.insts.push_back(lir::LirStackRestoreOp{
      .saved_ptr = lir::LirOperand("%saved.ptr"),
  });
  entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_helper_va_copy_arg_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "lowered_helper_va_copy_arg_metadata";
  function.signature_text = "define void @lowered_helper_va_copy_arg_metadata(ptr %dst, ptr %src)";
  function.params.emplace_back("%dst", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});
  function.params.emplace_back("%src", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirVaCopyOp{
      .dst_ptr = lir::LirOperand("%dst"),
      .src_ptr = lir::LirOperand("%src"),
  });
  entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_helper_va_arg_aggregate_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("riscv64-unknown-linux-gnu");

  lir::LirFunction function;
  function.name = "lowered_helper_va_arg_aggregate_metadata";
  function.signature_text = "define void @lowered_helper_va_arg_aggregate_metadata(ptr %ap)";
  function.params.emplace_back("%ap", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirVaArgOp{
      .result = lir::LirOperand("%pair"),
      .ap_ptr = lir::LirOperand("%ap"),
      .type_str = "{ i32, i32 }",
  });
  entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_helper_aggregate_call_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");

  lir::LirFunction decl;
  decl.name = "id_pair";
  decl.is_declaration = true;
  decl.signature_text = "declare { i32, i32 } @id_pair({ i32, i32 })";
  module.functions.push_back(std::move(decl));

  lir::LirFunction function;
  function.name = "lowered_helper_aggregate_call_metadata";
  function.signature_text = "define void @lowered_helper_aggregate_call_metadata({ i32, i32 } %p)";

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand("%pair.copy"),
      .return_type = "{ i32, i32 }",
      .callee = lir::LirOperand("@id_pair"),
      .callee_type_suffix = "({ i32, i32 })",
      .args_str = "{ i32, i32 } %p",
  });
  entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target_profile = x86_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

std::optional<prepare::PreparedBirModule> lower_and_prepare_same_module_variadic_global_byval_call_module() {
  lir::LirModule module;
  module.target_profile = c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
  module.type_decls.push_back("%pair = type { i32, i32 }");
  module.globals.push_back(lir::LirGlobal{
      .name = "gpair",
      .qualifier = "global ",
      .llvm_type = "%pair",
      .init_text = "{ i32 7, i32 9 }",
      .align_bytes = 4,
  });

  lir::LirFunction myprintf;
  myprintf.name = "myprintf";
  myprintf.signature_text = "define void @myprintf(ptr %format, ...)";

  lir::LirBlock myprintf_entry;
  myprintf_entry.label = "entry";
  myprintf_entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  myprintf.blocks.push_back(std::move(myprintf_entry));
  module.functions.push_back(std::move(myprintf));

  lir::LirFunction function;
  function.name = "lowered_same_module_variadic_global_byval_metadata";
  function.signature_text =
      "define void @lowered_same_module_variadic_global_byval_metadata(ptr %format)";
  function.params.emplace_back("%format", c4c::TypeSpec{.base = c4c::TB_VOID, .ptr_level = 1});

  lir::LirBlock entry;
  entry.label = "entry";
  entry.insts.push_back(lir::LirCallOp{
      .result = lir::LirOperand(""),
      .return_type = "void",
      .callee = lir::LirOperand("@myprintf"),
      .callee_type_suffix = "(ptr, ...)",
      .args_str = "ptr %format, ptr byval(%pair) @gpair",
  });
  entry.terminator = lir::LirRet{
      .type_str = "void",
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  auto lowered = try_lower_to_bir_with_options(module, BirLoweringOptions{});
  if (!lowered.module.has_value()) {
    return std::nullopt;
  }

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(*lowered.module);
  prepared.target_profile = x86_target_profile();

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
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
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
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "result"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "late.merge"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "result"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

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
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = false;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_cross_call_boundary_module_with_regalloc() {
  bir::Module module;

  bir::Function decl;
  decl.name = "boundary_helper";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::I32;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "cross_call_boundary";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "pre.only"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(3),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .callee = "boundary_helper",
      .args = {bir::Value::named(bir::TypeKind::I32, "pre.only")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "after"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "call.out"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "after"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_same_start_priority_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "same_start_priority";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.hot",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.mid0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.mid1",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.low",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.hot"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.mid0"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "mid.sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.mid0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.mid1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "tail0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.low"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "tail1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "tail0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "mid.sum"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "tail2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "tail1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.hot"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "tail2"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
  options.run_stack_layout = true;
  options.run_liveness = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_float_linear_scan_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "float_linear_scan";
  function.return_type = bir::TypeKind::F32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.hot",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.mid0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.mid1",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.low",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.hot"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "p.mid0"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot1"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "p.hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "mid.sum"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.mid0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "p.mid1"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "tail0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot1"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "p.low"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "tail1"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "tail0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "mid.sum"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "tail2"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "tail1"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "p.hot"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F32, "tail2"),
  };

  function.blocks.push_back(std::move(entry));
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = riscv_target_profile();

  prepare::PrepareOptions options;
  options.run_legalize = true;
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

  const auto* left = find_liveness_value(prepared, *function, "left.v");
  const auto* right = find_liveness_value(prepared, *function, "right.v");
  const auto* phi = find_liveness_value(prepared, *function, "phi.v");
  const auto* sum = find_liveness_value(prepared, *function, "sum");
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

  const auto* phi = find_regalloc_value(prepared, *function, "phi.v");
  const auto* sum = find_regalloc_value(prepared, *function, "sum");
  const auto* left = find_regalloc_value(prepared, *function, "left.v");
  const auto* right = find_regalloc_value(prepared, *function, "right.v");
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
  const bool sum_has_real_assignment =
      (sum->allocation_status == prepare::PreparedAllocationStatus::AssignedRegister &&
       sum->assigned_register.has_value()) ||
      (sum->allocation_status == prepare::PreparedAllocationStatus::AssignedStackSlot &&
       sum->assigned_stack_slot.has_value());
  if (!sum_has_real_assignment) {
    return fail("expected sum to receive a real allocation from the active regalloc path");
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

  const auto* byval = find_regalloc_value(prepared, *function, "p.byval");
  const auto* cmp0 = find_regalloc_value(prepared, *function, "cmp0");
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
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, function->function_name);
  if (control_flow == nullptr || control_flow->parallel_copy_bundles.empty()) {
    return fail("expected phi_join_move_resolution to publish prepared parallel-copy control-flow data");
  }

  const auto* left_feed = find_regalloc_value(prepared, *function, "left.feed");
  const auto* right_feed = find_regalloc_value(prepared, *function, "right.feed");
  const auto* phi = find_regalloc_value(prepared, *function, "phi.move");
  if (left_feed == nullptr || right_feed == nullptr || phi == nullptr) {
    return fail("expected phi join values to appear in regalloc output");
  }

  if (left_feed->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !left_feed->assigned_stack_slot.has_value() || left_feed->assigned_register.has_value()) {
    return fail("expected left.feed to fall back to a stack slot under join-side pressure");
  }
  const bool phi_has_real_assignment =
      (phi->allocation_status == prepare::PreparedAllocationStatus::AssignedRegister &&
       phi->assigned_register.has_value()) ||
      (phi->allocation_status == prepare::PreparedAllocationStatus::AssignedStackSlot &&
       phi->assigned_stack_slot.has_value());
  if (!phi_has_real_assignment) {
    return fail("expected the joined phi value to receive a real allocation");
  }
  if (function->move_resolution.empty()) {
    return fail("expected the phi join to publish move-resolution bookkeeping");
  }

  const auto* move = find_move_resolution(*function, left_feed->value_id, phi->value_id);
  if (move == nullptr || move->reason != "phi_join_stack_to_stack") {
    return fail("expected the stack-backed left phi incoming to publish prepared phi-join move resolution");
  }
  if (move->uses_cycle_temp_source) {
    return fail("expected the acyclic phi join move resolution to read directly from the incoming value");
  }
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      move->destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      move->destination_abi_index.has_value() || move->destination_register_name.has_value()) {
    return fail("expected phi-join move resolution to keep the generic value destination surface");
  }
  if (right_feed->assigned_register.has_value() &&
      right_feed->assigned_register->register_name == phi->assigned_register->register_name &&
      find_move_resolution(*function, right_feed->value_id, phi->value_id) != nullptr) {
    return fail("expected matching register-backed phi incoming storage to skip redundant move resolution");
  }
  if (count_move_resolution_reason_prefix_to_value(*function, phi->value_id, "consumer_") != 0) {
    return fail("expected prepared phi join results to avoid select-shaped consumer move reconstruction");
  }

  return 0;
}

int check_phi_loop_cycle_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "phi_loop_cycle_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for phi_loop_cycle_move_resolution");
  }
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, function->function_name);
  if (control_flow == nullptr) {
    return fail("expected phi_loop_cycle_move_resolution to publish prepared control-flow data");
  }
  const auto* body_bundle =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "body", "loop");
  if (body_bundle == nullptr || !body_bundle->has_cycle || body_bundle->moves.size() != 2 ||
      body_bundle->steps.size() != 3) {
    return fail("expected the loop backedge to publish an authoritative cycle-breaking bundle");
  }

  const auto* a = find_regalloc_value(prepared, *function, "a");
  const auto* b = find_regalloc_value(prepared, *function, "b");
  if (a == nullptr || b == nullptr) {
    return fail("expected loop phi values to appear in regalloc output");
  }
  if (function->move_resolution.empty()) {
    return fail("expected loop phi cycle resolution to publish move-resolution bookkeeping");
  }

  const auto* save_a_to_temp = find_move_resolution_by_op_kind(
      *function,
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
      a->value_id,
      a->value_id);
  if (save_a_to_temp == nullptr ||
      !move_resolution_reason_has_prefix(*save_a_to_temp, "phi_loop_carry_save_destination_to_temp")) {
    return fail("expected the loop backedge to publish an explicit move-resolution temp-save contract");
  }
  if (save_a_to_temp->uses_cycle_temp_source ||
      save_a_to_temp->destination_kind != prepare::PreparedMoveDestinationKind::Value) {
    return fail("expected the temp-save contract to publish a first-class save step without reopening a special destination surface");
  }

  const auto* b_to_a = find_move_resolution(*function, b->value_id, a->value_id);
  if (b_to_a == nullptr || !move_resolution_reason_has_prefix(*b_to_a, "phi_loop_carry_")) {
    return fail("expected the direct backedge move to use authoritative loop-carry move resolution");
  }
  if (b_to_a->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return fail("expected the direct backedge move to stay a normal move-resolution operation");
  }
  if (b_to_a->uses_cycle_temp_source) {
    return fail("expected the first backedge move to read directly from the original incoming value");
  }

  const auto* a_to_b = find_move_resolution(*function, a->value_id, b->value_id);
  if (a_to_b == nullptr ||
      !move_resolution_reason_has_prefix(*a_to_b, "phi_loop_carry_cycle_temp_")) {
    return fail("expected the cycle-broken backedge move to record the authoritative cycle-temp source");
  }
  if (a_to_b->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return fail("expected the cycle-broken backedge transfer to stay a normal move-resolution operation");
  }
  if (!a_to_b->uses_cycle_temp_source) {
    return fail("expected the cycle-broken backedge move to publish first-class cycle-temp sourcing");
  }

  if (count_move_resolution_reason_prefix_to_value(*function, a->value_id, "consumer_") != 0 ||
      count_move_resolution_reason_prefix_to_value(*function, b->value_id, "consumer_") != 0) {
    return fail("expected loop phi cycle values to avoid generic consumer move reconstruction");
  }

  return 0;
}

int check_call_crossing_regalloc_spillover(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "call_crossing_spillover");
  if (function == nullptr) {
    return fail("expected regalloc output for call_crossing_spillover");
  }

  const auto* carry = find_regalloc_value(prepared, *function, "carry");
  const auto* local0 = find_regalloc_value(prepared, *function, "local0");
  const auto* local1 = find_regalloc_value(prepared, *function, "local1");
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

  const auto* carry = find_regalloc_value(prepared, *function, "carry");
  const auto* low0 = find_regalloc_value(prepared, *function, "low0");
  const auto* low1 = find_regalloc_value(prepared, *function, "low1");
  const auto* hot = find_regalloc_value(prepared, *function, "hot");
  if (carry == nullptr || low0 == nullptr || low1 == nullptr || hot == nullptr) {
    return fail("expected carry, low0, low1, and hot to appear in regalloc output");
  }

  if (!carry->assigned_register.has_value() || carry->assigned_register->register_name != "s1") {
    return fail("expected the call-crossing carry value to keep the protected callee-saved assignment");
  }
  if (hot->priority == 0 || low0->priority == 0 || low1->priority == 0) {
    return fail("expected weighted_post_call_pressure values to publish nonzero allocation priority");
  }
  int stack_backed_count = 0;
  bool saw_t0 = false;
  bool saw_s2 = false;
  for (const prepare::PreparedRegallocValue* value : {hot, low0, low1}) {
    if (value->allocation_status == prepare::PreparedAllocationStatus::AssignedStackSlot &&
        value->assigned_stack_slot.has_value() && !value->assigned_register.has_value()) {
      ++stack_backed_count;
      continue;
    }
    if (!value->assigned_register.has_value()) {
      return fail("expected weighted_post_call_pressure values to keep real register-or-stack assignments");
    }
    saw_t0 = saw_t0 || value->assigned_register->register_name == "t0";
    saw_s2 = saw_s2 || value->assigned_register->register_name == "s2";
  }
  if (stack_backed_count != 1 || !saw_t0 || !saw_s2) {
    return fail("expected weighted_post_call_pressure to keep one stack-backed local plus the t0/s2 spillover registers active");
  }
  const auto* carry_to_hot = find_move_resolution(*function, carry->value_id, hot->value_id);
  if (carry_to_hot == nullptr || carry_to_hot->block_index != 0 || carry_to_hot->instruction_index != 4 ||
      carry_to_hot->reason != "consumer_register_to_stack") {
    return fail("expected the post-call carry use to publish a consumer-keyed register-to-stack move record");
  }
  if (carry_to_hot->destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      carry_to_hot->destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
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

  const auto* carry0 = find_regalloc_value(prepared, *function, "carry0");
  const auto* carry1 = find_regalloc_value(prepared, *function, "carry1");
  const auto* local0 = find_regalloc_value(prepared, *function, "local0");
  const auto* hot = find_regalloc_value(prepared, *function, "hot");
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

  const auto* carry0 = find_regalloc_value(prepared, *function, "carry0");
  const auto* carry1 = find_regalloc_value(prepared, *function, "carry1");
  const auto* keep_arg = find_regalloc_value(prepared, *function, "keep.arg");
  const auto* spill_arg = find_regalloc_value(prepared, *function, "spill.arg");
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
  const auto* keep_move = find_move_resolution(*function, keep_arg->value_id, keep_arg->value_id);
  if (keep_move == nullptr || keep_move->block_index != 0 || keep_move->instruction_index != 4 ||
      keep_move->reason != "call_arg_register_to_register") {
    return fail("expected register-backed call arguments to publish concrete ABI register transfers");
  }
  if (keep_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      keep_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      keep_move->destination_abi_index != std::optional<std::size_t>{2} ||
      keep_move->destination_register_name != std::optional<std::string>{"a2"}) {
    return fail("expected register-backed call arguments to publish the concrete ABI argument destination");
  }

  return 0;
}

int check_call_result_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "call_result_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for call_result_move_resolution");
  }

  const auto* call_result = find_regalloc_value(prepared, *function, "call.result");
  if (call_result == nullptr) {
    return fail("expected call.result to appear in regalloc output");
  }
  if (call_result->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !call_result->assigned_register.has_value() ||
      call_result->assigned_register->register_name != "ft0") {
    return fail("expected the float call result to take the temporary float caller-saved seed");
  }

  const auto* move = find_move_resolution(*function, call_result->value_id, call_result->value_id);
  if (move == nullptr || move->block_index != 0 || move->instruction_index != 0 ||
      move->reason != "call_result_register_to_register") {
    return fail("expected the register-backed call result to publish a call-result register move");
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

  const auto* ret_value = find_regalloc_value(prepared, *function, "ret.value");
  if (ret_value == nullptr) {
    return fail("expected ret.value to appear in regalloc output");
  }
  if (ret_value->allocation_status != prepare::PreparedAllocationStatus::AssignedRegister ||
      !ret_value->assigned_register.has_value() ||
      ret_value->assigned_register->register_name != "ft0") {
    return fail("expected the float return value to take the temporary float caller-saved seed");
  }

  const auto* move = find_move_resolution(*function, ret_value->value_id, ret_value->value_id);
  if (move == nullptr || move->block_index != 0 || move->instruction_index != 1 ||
      move->reason != "return_register_to_register") {
    return fail("expected the register-backed return value to publish a return-site register move");
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

  const auto* ret_value = find_regalloc_value(prepared, *function, "ret.value");
  if (ret_value == nullptr) {
    return fail("expected ret.value to appear in regalloc output");
  }
  if (!ret_value->assigned_register.has_value() || ret_value->assigned_stack_slot.has_value()) {
    return fail("expected the integer return value to stay register-backed");
  }
  const auto* move = find_move_resolution(*function, ret_value->value_id, ret_value->value_id);
  if (move == nullptr || move->block_index != 0 || move->instruction_index != 1 ||
      move->reason != "return_register_to_register") {
    return fail("expected register-backed returns to publish concrete ABI register transfers");
  }
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      move->destination_abi_index.has_value() ||
      move->destination_register_name != std::optional<std::string>{"a0"}) {
    return fail("expected register-backed returns to publish the concrete ABI return destination");
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

int check_lowered_helper_aggregate_call_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "lowered_helper_aggregate_call_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1) {
    return fail("expected lowered_helper_aggregate_call_metadata BIR output with one block");
  }

  const bir::CallInst* call = nullptr;
  int call_count = 0;
  for (const auto& inst : module_function->blocks.front().insts) {
    const auto* candidate = std::get_if<bir::CallInst>(&inst);
    if (candidate == nullptr) {
      continue;
    }
    ++call_count;
    call = candidate;
  }
  if (call_count != 1 || call == nullptr || call->callee != "id_pair" || call->arg_abi.size() != 2) {
    return fail("expected helper-built aggregate call BIR output");
  }
  if (call->arg_abi[0].type != bir::TypeKind::Ptr || !call->arg_abi[0].sret_pointer ||
      call->arg_abi[0].primary_class != bir::AbiValueClass::Memory ||
      call->arg_abi[0].size_bytes != 8 || call->arg_abi[0].align_bytes != 4) {
    return fail("expected helper-built aggregate return storage to preserve explicit sret-style ABI metadata");
  }
  if (call->arg_abi[1].type != bir::TypeKind::Ptr || !call->arg_abi[1].byval_copy ||
      call->arg_abi[1].primary_class != bir::AbiValueClass::Memory ||
      call->arg_abi[1].size_bytes != 8 || call->arg_abi[1].align_bytes != 4) {
    return fail("expected helper-built aggregate parameter copy to preserve explicit byval ABI metadata");
  }
  if (!call->sret_storage_name.has_value()) {
    return fail("expected helper-built aggregate call to preserve explicit sret storage identity");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_aggregate_call_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_aggregate_call_metadata");
  }

  bool saw_byval_move = false;
  for (const auto& move : function->move_resolution) {
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
        move.destination_abi_index != std::optional<std::size_t>{1}) {
      continue;
    }
    if ((move.destination_storage_kind == prepare::PreparedMoveStorageKind::StackSlot &&
         !move.destination_register_name.has_value() &&
         (move.reason == "call_arg_register_to_stack" ||
          move.reason == "call_arg_stack_to_stack")) ||
        (move.destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
         (move.reason == "call_arg_register_to_register" ||
          move.reason == "call_arg_stack_to_register"))) {
      saw_byval_move = true;
    }
  }
  if (!saw_byval_move) {
    return fail("expected helper-built aggregate byval argument to publish an authoritative ABI destination");
  }

  return 0;
}

int check_lowered_same_module_variadic_global_byval_call_abi(
    const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(
      prepared, "lowered_same_module_variadic_global_byval_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail(
        "expected lowered_same_module_variadic_global_byval_metadata BIR output with one call");
  }

  const auto* call = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (call == nullptr || call->callee != "myprintf" || !call->is_variadic ||
      call->args.size() != 2 || call->arg_abi.size() != 2) {
    return fail("expected same-module variadic aggregate call BIR output");
  }
  if (call->args[1] != bir::Value::named(bir::TypeKind::Ptr, "@gpair")) {
    return fail("expected same-module variadic aggregate call to preserve the global payload identity");
  }
  if (call->arg_abi[1].type != bir::TypeKind::Ptr || !call->arg_abi[1].byval_copy ||
      call->arg_abi[1].primary_class != bir::AbiValueClass::Memory ||
      call->arg_abi[1].size_bytes != 8 || call->arg_abi[1].align_bytes != 4) {
    return fail(
        "expected same-module variadic aggregate call to preserve explicit byval ABI metadata");
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

  const auto* call_result = find_regalloc_value(prepared, *function, "%call.result");
  if (call_result == nullptr) {
    return fail("expected lowered call result to appear in regalloc output");
  }

  bool saw_call_result_move = false;
  for (const auto& move : function->move_resolution) {
    if (move.from_value_id != call_result->value_id || move.to_value_id != call_result->value_id ||
        move.reason != "call_result_register_to_register") {
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

int check_lowered_helper_call_result_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "lowered_helper_call_result_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail("expected lowered_helper_call_result_metadata BIR output with one call");
  }

  const auto* call = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (call == nullptr || call->callee != "llvm.fabs.float" || !call->result_abi.has_value() ||
      call->result_abi->type != bir::TypeKind::F32 ||
      call->result_abi->primary_class != bir::AbiValueClass::Sse) {
    return fail("expected helper-built fabs call to preserve explicit float result ABI metadata");
  }
  if (call->arg_abi.size() != 1 || call->arg_abi.front().type != bir::TypeKind::F32 ||
      call->arg_abi.front().primary_class != bir::AbiValueClass::Sse ||
      !call->arg_abi.front().passed_in_register) {
    return fail("expected helper-built fabs call to preserve explicit float argument ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_call_result_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_call_result_metadata");
  }

  const auto* call_arg = find_regalloc_value(prepared, *function, "%arg");
  if (call_arg == nullptr) {
    return fail("expected helper-built fabs argument to appear in regalloc output");
  }
  const auto* arg_move = find_move_resolution(*function, call_arg->value_id, call_arg->value_id);
  if (arg_move == nullptr || arg_move->reason != "call_arg_register_to_register" ||
      arg_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      arg_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      arg_move->destination_abi_index != std::optional<std::size_t>{0} ||
      arg_move->destination_register_name != std::optional<std::string>{"fa0"}) {
    return fail("expected helper-built fabs argument to publish the concrete ABI argument destination");
  }

  const auto* call_result = find_regalloc_value(prepared, *function, "%fabs.result");
  if (call_result == nullptr) {
    return fail("expected helper-built fabs result to appear in regalloc output");
  }

  bool saw_call_result_move = false;
  for (const auto& move : function->move_resolution) {
    if (move.from_value_id != call_result->value_id || move.to_value_id != call_result->value_id ||
        move.reason != "call_result_register_to_register") {
      continue;
    }
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.destination_abi_index.has_value() ||
        move.destination_register_name != std::optional<std::string>{"fa0"}) {
      return fail("expected helper-built fabs call-result move to target the concrete ABI return register");
    }
    saw_call_result_move = true;
    break;
  }
  if (!saw_call_result_move) {
    return fail("expected helper-built fabs result to publish call-result move resolution");
  }

  return 0;
}

int check_lowered_helper_call_result_soft_float_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function =
      find_module_function(prepared, "lowered_helper_call_result_soft_float_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail("expected lowered_helper_call_result_soft_float_metadata BIR output with one call");
  }
  if (!module_function->return_abi.has_value() ||
      module_function->return_abi->type != bir::TypeKind::F32 ||
      module_function->return_abi->primary_class != bir::AbiValueClass::Integer) {
    return fail("expected soft-float function return ABI metadata to use integer-class routing");
  }

  const auto* call = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (call == nullptr || call->callee != "llvm.fabs.float" || !call->result_abi.has_value() ||
      call->result_abi->type != bir::TypeKind::F32 ||
      call->result_abi->primary_class != bir::AbiValueClass::Integer) {
    return fail("expected soft-float helper-built fabs call to preserve integer-class result ABI metadata");
  }
  if (call->arg_abi.size() != 1 || call->arg_abi.front().type != bir::TypeKind::F32 ||
      call->arg_abi.front().primary_class != bir::AbiValueClass::Integer ||
      !call->arg_abi.front().passed_in_register) {
    return fail("expected soft-float helper-built fabs call to preserve integer-class argument ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_call_result_soft_float_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_call_result_soft_float_metadata");
  }

  const auto* call_arg = find_regalloc_value(prepared, *function, "%arg");
  if (call_arg == nullptr) {
    return fail("expected soft-float helper-built fabs argument to appear in regalloc output");
  }
  const auto* arg_move = find_move_resolution(*function, call_arg->value_id, call_arg->value_id);
  const bool arg_already_in_a0 =
      call_arg->assigned_register.has_value() && !call_arg->assigned_stack_slot.has_value() &&
      call_arg->assigned_register->register_name == "a0";
  const bool arg_move_targets_a0 =
      arg_move != nullptr &&
      (arg_move->reason == "call_arg_stack_to_register" ||
       arg_move->reason == "call_arg_register_to_register") &&
      arg_move->destination_kind == prepare::PreparedMoveDestinationKind::CallArgumentAbi &&
      arg_move->destination_storage_kind == prepare::PreparedMoveStorageKind::Register &&
      arg_move->destination_abi_index == std::optional<std::size_t>{0} &&
      arg_move->destination_register_name == std::optional<std::string>{"a0"};
  if (!arg_already_in_a0 && !arg_move_targets_a0) {
    return fail("expected soft-float helper-built fabs argument to publish the concrete integer ABI destination");
  }

  const auto* call_result = find_regalloc_value(prepared, *function, "%fabs.result");
  if (call_result == nullptr) {
    return fail("expected soft-float helper-built fabs result to appear in regalloc output");
  }

  bool saw_call_result_move = false;
  for (const auto& move : function->move_resolution) {
    if (move.from_value_id != call_result->value_id || move.to_value_id != call_result->value_id ||
        (move.reason != "call_result_stack_to_register" &&
         move.reason != "call_result_register_to_register")) {
      continue;
    }
    if (move.destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
        move.destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
        move.destination_abi_index.has_value() ||
        move.destination_register_name != std::optional<std::string>{"a0"}) {
      return fail("expected soft-float helper-built fabs call-result move to target the integer ABI return register");
    }
    saw_call_result_move = true;
    break;
  }
  if (!saw_call_result_move) {
    return fail("expected soft-float helper-built fabs result to publish call-result move resolution");
  }

  return 0;
}

int check_lowered_helper_stackrestore_arg_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "lowered_helper_stackrestore_arg_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 2) {
    return fail("expected lowered_helper_stackrestore_arg_metadata BIR output with stacksave and stackrestore");
  }

  const auto* stackrestore = std::get_if<bir::CallInst>(&module_function->blocks.front().insts[1]);
  if (stackrestore == nullptr || stackrestore->callee != "llvm.stackrestore" ||
      stackrestore->arg_abi.size() != 1 || stackrestore->arg_abi.front().type != bir::TypeKind::Ptr ||
      stackrestore->arg_abi.front().primary_class != bir::AbiValueClass::Integer ||
      !stackrestore->arg_abi.front().passed_in_register) {
    return fail("expected helper-built stackrestore call to preserve explicit pointer argument ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_stackrestore_arg_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_stackrestore_arg_metadata");
  }

  const auto* saved_ptr = find_regalloc_value(prepared, *function, "%saved.ptr");
  if (saved_ptr == nullptr) {
    return fail("expected helper-built stacksave result to appear in regalloc output");
  }
  const auto* arg_move = find_move_resolution(*function, saved_ptr->value_id, saved_ptr->value_id);
  if (arg_move == nullptr || arg_move->reason != "call_arg_register_to_register" ||
      arg_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      arg_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      arg_move->destination_abi_index != std::optional<std::size_t>{0} ||
      arg_move->destination_register_name != std::optional<std::string>{"a0"}) {
    return fail("expected helper-built stackrestore argument to publish the concrete ABI argument destination");
  }

  return 0;
}

int check_lowered_helper_va_copy_arg_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function = find_module_function(prepared, "lowered_helper_va_copy_arg_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail("expected lowered_helper_va_copy_arg_metadata BIR output with one va_copy call");
  }

  const auto* va_copy = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (va_copy == nullptr || va_copy->callee != "llvm.va_copy.p0.p0" || va_copy->arg_abi.size() != 2 ||
      va_copy->arg_abi[0].type != bir::TypeKind::Ptr ||
      va_copy->arg_abi[1].type != bir::TypeKind::Ptr ||
      va_copy->arg_abi[0].primary_class != bir::AbiValueClass::Integer ||
      va_copy->arg_abi[1].primary_class != bir::AbiValueClass::Integer ||
      !va_copy->arg_abi[0].passed_in_register || !va_copy->arg_abi[1].passed_in_register) {
    return fail("expected helper-built va_copy call to preserve explicit pointer argument ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_va_copy_arg_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_va_copy_arg_metadata");
  }

  const auto* dst_ptr = find_regalloc_value(prepared, *function, "%dst");
  const auto* src_ptr = find_regalloc_value(prepared, *function, "%src");
  if (dst_ptr == nullptr || src_ptr == nullptr) {
    return fail("expected helper-built va_copy arguments to appear in regalloc output");
  }

  const auto* dst_move = find_move_resolution(*function, dst_ptr->value_id, dst_ptr->value_id);
  if (dst_move == nullptr || dst_move->reason != "call_arg_register_to_register" ||
      dst_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      dst_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      dst_move->destination_abi_index != std::optional<std::size_t>{0} ||
      dst_move->destination_register_name != std::optional<std::string>{"a0"}) {
    return fail("expected helper-built va_copy destination pointer to publish the concrete ABI argument destination");
  }

  const auto* src_move = find_move_resolution(*function, src_ptr->value_id, src_ptr->value_id);
  if (src_move == nullptr || src_move->reason != "call_arg_register_to_register" ||
      src_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      src_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      src_move->destination_abi_index != std::optional<std::size_t>{1} ||
      src_move->destination_register_name != std::optional<std::string>{"a1"}) {
    return fail("expected helper-built va_copy source pointer to publish the concrete ABI argument destination");
  }

  return 0;
}

int check_lowered_helper_va_arg_aggregate_abi(const prepare::PreparedBirModule& prepared) {
  const auto* module_function =
      find_module_function(prepared, "lowered_helper_va_arg_aggregate_metadata");
  if (module_function == nullptr || module_function->blocks.size() != 1 ||
      module_function->blocks.front().insts.size() != 1) {
    return fail("expected lowered_helper_va_arg_aggregate_metadata BIR output with one aggregate va_arg call");
  }

  const auto* va_arg = std::get_if<bir::CallInst>(&module_function->blocks.front().insts.front());
  if (va_arg == nullptr || va_arg->callee != "llvm.va_arg.aggregate" || va_arg->arg_abi.size() != 2) {
    return fail("expected helper-built aggregate va_arg BIR output");
  }
  if (va_arg->arg_abi[0].type != bir::TypeKind::Ptr || !va_arg->arg_abi[0].sret_pointer ||
      va_arg->arg_abi[0].primary_class != bir::AbiValueClass::Memory ||
      va_arg->arg_abi[0].size_bytes != 8 || va_arg->arg_abi[0].align_bytes != 4) {
    return fail("expected aggregate va_arg result storage to preserve explicit sret-style ABI metadata");
  }
  if (va_arg->arg_abi[1].type != bir::TypeKind::Ptr ||
      va_arg->arg_abi[1].primary_class != bir::AbiValueClass::Integer ||
      !va_arg->arg_abi[1].passed_in_register) {
    return fail("expected aggregate va_arg ap pointer to preserve explicit pointer argument ABI metadata");
  }

  const auto* function = find_regalloc_function(prepared, "lowered_helper_va_arg_aggregate_metadata");
  if (function == nullptr) {
    return fail("expected regalloc output for lowered_helper_va_arg_aggregate_metadata");
  }

  const auto* pair_ptr = find_regalloc_value(prepared, *function, "%pair");
  const auto* ap_ptr = find_regalloc_value(prepared, *function, "%ap");
  if (pair_ptr == nullptr || ap_ptr == nullptr) {
    return fail("expected helper-built aggregate va_arg values to appear in regalloc output");
  }

  const auto* pair_move = find_move_resolution(*function, pair_ptr->value_id, pair_ptr->value_id);
  if (pair_move == nullptr ||
      (pair_move->reason != "call_arg_register_to_stack" &&
       pair_move->reason != "call_arg_stack_to_stack") ||
      pair_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      pair_move->destination_storage_kind != prepare::PreparedMoveStorageKind::StackSlot ||
      pair_move->destination_abi_index != std::optional<std::size_t>{0} ||
      pair_move->destination_register_name.has_value()) {
    return fail("expected helper-built aggregate va_arg result storage to publish stack-backed ABI consumption");
  }

  const auto* ap_move = find_move_resolution(*function, ap_ptr->value_id, ap_ptr->value_id);
  if (ap_move == nullptr || ap_move->reason != "call_arg_register_to_register" ||
      ap_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      ap_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      ap_move->destination_abi_index != std::optional<std::size_t>{1} ||
      ap_move->destination_register_name != std::optional<std::string>{"a1"}) {
    return fail("expected helper-built aggregate va_arg ap pointer to publish the concrete ABI argument destination");
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

  const auto* liveness_hot = find_liveness_value(prepared, *liveness, "loop.hot");
  const auto* regalloc_hot = find_regalloc_value(prepared, *regalloc, "loop.hot");
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

int check_cross_call_boundary_classification(const prepare::PreparedBirModule& prepared) {
  const auto* liveness = find_liveness_function(prepared, "cross_call_boundary");
  const auto* regalloc = find_regalloc_function(prepared, "cross_call_boundary");
  if (liveness == nullptr || regalloc == nullptr) {
    return fail("expected liveness and regalloc output for cross_call_boundary");
  }

  const auto* pre_only_live = find_liveness_value(prepared, *liveness, "pre.only");
  const auto* carry_live = find_liveness_value(prepared, *liveness, "carry");
  const auto* call_out_live = find_liveness_value(prepared, *liveness, "call.out");
  const auto* pre_only_reg = find_regalloc_value(prepared, *regalloc, "pre.only");
  const auto* carry_reg = find_regalloc_value(prepared, *regalloc, "carry");
  const auto* call_out_reg = find_regalloc_value(prepared, *regalloc, "call.out");
  if (pre_only_live == nullptr || carry_live == nullptr || call_out_live == nullptr ||
      pre_only_reg == nullptr || carry_reg == nullptr || call_out_reg == nullptr) {
    return fail("expected cross_call_boundary values to appear in both liveness and regalloc output");
  }

  if (pre_only_live->crosses_call || pre_only_reg->crosses_call) {
    return fail("expected pre.only to die at the call boundary rather than count as call-crossing");
  }
  if (!carry_live->crosses_call || !carry_reg->crosses_call) {
    return fail("expected carry to stay live across the call boundary");
  }
  if (call_out_live->crosses_call || call_out_reg->crosses_call) {
    return fail("expected call.out to start after the call boundary rather than count as call-crossing");
  }
  if (!carry_reg->assigned_register.has_value() || carry_reg->assigned_register->register_name != "s1") {
    return fail("expected the true call-crossing carry value to take the protected callee-saved register");
  }

  return 0;
}

int check_same_start_priority_ordering(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "same_start_priority");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for same_start_priority");
  }

  const auto* hot = find_regalloc_value(prepared, *regalloc, "p.hot");
  const auto* mid0 = find_regalloc_value(prepared, *regalloc, "p.mid0");
  const auto* mid1 = find_regalloc_value(prepared, *regalloc, "p.mid1");
  const auto* low = find_regalloc_value(prepared, *regalloc, "p.low");
  if (hot == nullptr || mid0 == nullptr || mid1 == nullptr || low == nullptr) {
    return fail("expected same_start_priority params to appear in regalloc output");
  }

  if (hot->priority <= mid0->priority || hot->priority <= mid1->priority ||
      hot->priority <= low->priority) {
    return fail("expected p.hot to publish the strongest same-start linear-scan priority");
  }
  if (!hot->assigned_register.has_value() || hot->assigned_register->register_name != "t0") {
    return fail("expected same-start linear scan to give the hottest value the caller-saved seed first");
  }

  int callee_saved_count = 0;
  int stack_count = 0;
  for (const prepare::PreparedRegallocValue* value : {mid0, mid1, low}) {
    if (value->assigned_register.has_value() &&
        (value->assigned_register->register_name == "s1" ||
         value->assigned_register->register_name == "s2")) {
      ++callee_saved_count;
      continue;
    }
    if (value->allocation_status == prepare::PreparedAllocationStatus::AssignedStackSlot &&
        value->assigned_stack_slot.has_value() && !value->assigned_register.has_value()) {
      ++stack_count;
      continue;
    }
    return fail("expected same-start priority followers to fall into callee-saved spillover or stack");
  }
  if (callee_saved_count != 2 || stack_count != 1) {
    return fail("expected same-start linear scan to consume s1/s2 before stacking the weakest value");
  }

  return 0;
}

int check_float_linear_scan_pooling(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "float_linear_scan");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for float_linear_scan");
  }

  const auto* hot = find_regalloc_value(prepared, *regalloc, "p.hot");
  const auto* mid0 = find_regalloc_value(prepared, *regalloc, "p.mid0");
  const auto* mid1 = find_regalloc_value(prepared, *regalloc, "p.mid1");
  const auto* low = find_regalloc_value(prepared, *regalloc, "p.low");
  if (hot == nullptr || mid0 == nullptr || mid1 == nullptr || low == nullptr) {
    return fail("expected float_linear_scan params to appear in regalloc output");
  }

  if (hot->register_class != prepare::PreparedRegisterClass::Float ||
      mid0->register_class != prepare::PreparedRegisterClass::Float ||
      mid1->register_class != prepare::PreparedRegisterClass::Float ||
      low->register_class != prepare::PreparedRegisterClass::Float) {
    return fail("expected float_linear_scan values to classify into the float register bank");
  }
  if (!hot->assigned_register.has_value() || hot->assigned_register->register_name != "ft0") {
    return fail("expected float linear scan to use the float caller-saved seed first");
  }

  int callee_saved_count = 0;
  int stack_count = 0;
  for (const prepare::PreparedRegallocValue* value : {mid0, mid1, low}) {
    if (value->assigned_register.has_value() &&
        (value->assigned_register->register_name == "fs1" ||
         value->assigned_register->register_name == "fs2")) {
      ++callee_saved_count;
      continue;
    }
    if (value->allocation_status == prepare::PreparedAllocationStatus::AssignedStackSlot &&
        value->assigned_stack_slot.has_value() && !value->assigned_register.has_value()) {
      ++stack_count;
      continue;
    }
    return fail("expected float linear scan followers to use float spillover or stack fallback");
  }
  if (callee_saved_count != 2 || stack_count != 1) {
    return fail("expected float linear scan to mirror the temporary GPR-style pool policy");
  }

  return 0;
}

int check_vector_span_candidate_legality() {
  const auto profile = riscv_target_profile();
  const auto m2_spans =
      prepare::caller_saved_register_spans(profile, prepare::PreparedRegisterClass::Vector, 2);
  const auto m4_spans =
      prepare::caller_saved_register_spans(profile, prepare::PreparedRegisterClass::Vector, 4);
  const auto callee_m2_spans =
      prepare::callee_saved_register_spans(profile, prepare::PreparedRegisterClass::Vector, 2);

  if (m2_spans.size() < 2 || m4_spans.empty() || callee_m2_spans.empty()) {
    return fail("expected grouped vector candidate spans for caller/callee pools");
  }
  if (m2_spans[0].register_name != "v0" ||
      m2_spans[0].occupied_register_names != std::vector<std::string>{"v0", "v1"} ||
      m2_spans[1].register_name != "v2" ||
      m2_spans[1].occupied_register_names != std::vector<std::string>{"v2", "v3"}) {
    return fail("expected LMUL=2 vector candidates to stay contiguous and width-aligned");
  }
  if (m4_spans[0].register_name != "v0" ||
      m4_spans[0].occupied_register_names !=
          std::vector<std::string>{"v0", "v1", "v2", "v3"}) {
    return fail("expected LMUL=4 vector candidates to occupy four contiguous units from the base");
  }
  if (callee_m2_spans[0].register_name != "v16" ||
      callee_m2_spans[0].occupied_register_names != std::vector<std::string>{"v16", "v17"}) {
    return fail("expected callee vector candidates to start from the disjoint callee pool");
  }

  return 0;
}

int check_grouped_contract_occupancy_metadata(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "float_linear_scan");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for float_linear_scan grouped-contract metadata");
  }

  const auto* hot = find_regalloc_value(prepared, *regalloc, "p.hot");
  if (hot == nullptr || !hot->assigned_register.has_value()) {
    return fail("expected float_linear_scan hot value to publish an assigned register");
  }
  if (hot->assigned_register->contiguous_width != 1 ||
      hot->assigned_register->occupied_register_names != std::vector<std::string>{"ft0"}) {
    return fail("expected width=1 assignments to publish singleton occupied-unit metadata");
  }

  const auto* call_plan = prepare::find_prepared_call_plans(prepared, prepared.names.function_names.find("float_linear_scan"));
  (void)call_plan;
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

  const auto phi_loop_cycle_move_prepared = prepare_phi_loop_cycle_move_module_with_regalloc();
  if (const int rc = check_phi_loop_cycle_move_resolution(phi_loop_cycle_move_prepared); rc != 0) {
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

  const auto lowered_helper_aggregate_call_prepared = lower_and_prepare_helper_aggregate_call_module();
  if (!lowered_helper_aggregate_call_prepared.has_value()) {
    return fail("expected lowered helper aggregate-call module to succeed");
  }
  if (const int rc = check_lowered_helper_aggregate_call_abi(*lowered_helper_aggregate_call_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_same_module_variadic_global_byval_prepared =
      lower_and_prepare_same_module_variadic_global_byval_call_module();
  if (!lowered_same_module_variadic_global_byval_prepared.has_value()) {
    return fail("expected lowered same-module variadic aggregate-call module to succeed");
  }
  if (const int rc = check_lowered_same_module_variadic_global_byval_call_abi(
          *lowered_same_module_variadic_global_byval_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_call_result_prepared = lower_and_prepare_call_result_module();
  if (!lowered_call_result_prepared.has_value()) {
    return fail("expected lowered call-result module to succeed");
  }
  if (const int rc = check_lowered_call_result_abi(*lowered_call_result_prepared); rc != 0) {
    return rc;
  }

  const auto lowered_helper_call_result_prepared = lower_and_prepare_helper_call_result_module();
  if (!lowered_helper_call_result_prepared.has_value()) {
    return fail("expected lowered helper call-result module to succeed");
  }
  if (const int rc = check_lowered_helper_call_result_abi(*lowered_helper_call_result_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_soft_float_helper_call_result_prepared =
      lower_and_prepare_soft_float_helper_call_result_module();
  if (!lowered_soft_float_helper_call_result_prepared.has_value()) {
    return fail("expected lowered soft-float helper call-result module to succeed");
  }
  if (const int rc = check_lowered_helper_call_result_soft_float_abi(
          *lowered_soft_float_helper_call_result_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_helper_stackrestore_arg_prepared = lower_and_prepare_helper_stackrestore_arg_module();
  if (!lowered_helper_stackrestore_arg_prepared.has_value()) {
    return fail("expected lowered helper stackrestore-arg module to succeed");
  }
  if (const int rc = check_lowered_helper_stackrestore_arg_abi(*lowered_helper_stackrestore_arg_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_helper_va_copy_arg_prepared = lower_and_prepare_helper_va_copy_arg_module();
  if (!lowered_helper_va_copy_arg_prepared.has_value()) {
    return fail("expected lowered helper va_copy-arg module to succeed");
  }
  if (const int rc = check_lowered_helper_va_copy_arg_abi(*lowered_helper_va_copy_arg_prepared);
      rc != 0) {
    return rc;
  }

  const auto lowered_helper_va_arg_aggregate_prepared = lower_and_prepare_helper_va_arg_aggregate_module();
  if (!lowered_helper_va_arg_aggregate_prepared.has_value()) {
    return fail("expected lowered helper aggregate va_arg module to succeed");
  }
  if (const int rc =
          check_lowered_helper_va_arg_aggregate_abi(*lowered_helper_va_arg_aggregate_prepared);
      rc != 0) {
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

  const auto cross_call_boundary_prepared = prepare_cross_call_boundary_module_with_regalloc();
  if (const int rc = check_cross_call_boundary_classification(cross_call_boundary_prepared); rc != 0) {
    return rc;
  }

  const auto same_start_priority_prepared = prepare_same_start_priority_module_with_regalloc();
  if (const int rc = check_same_start_priority_ordering(same_start_priority_prepared); rc != 0) {
    return rc;
  }

  const auto float_linear_scan_prepared = prepare_float_linear_scan_module_with_regalloc();
  if (const int rc = check_float_linear_scan_pooling(float_linear_scan_prepared); rc != 0) {
    return rc;
  }
  if (const int rc = check_grouped_contract_occupancy_metadata(float_linear_scan_prepared); rc != 0) {
    return rc;
  }
  if (const int rc = check_vector_span_candidate_legality(); rc != 0) {
    return rc;
  }
  return 0;
}

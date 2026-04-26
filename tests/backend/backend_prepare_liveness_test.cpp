#include "src/backend/bir/bir.hpp"
#include "src/backend/bir/lir_to_bir.hpp"
#include "src/backend/mir/x86/x86.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/codegen/lir/ir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/target_register_profile.hpp"
#include "src/target_profile.hpp"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

bool is_named_i32(const bir::Value& value, std::string_view expected_name) {
  return value.kind == bir::Value::Kind::Named && value.type == bir::TypeKind::I32 &&
         value.name == expected_name;
}

c4c::BlockLabelId block_label_id(bir::Module& module, std::string_view label) {
  return module.names.block_labels.intern(label);
}

bir::Block make_block(bir::Module& module, std::string_view label) {
  return bir::Block{
      .label = std::string(label),
      .label_id = block_label_id(module, label),
  };
}

bir::BranchTerminator make_branch(bir::Module& module, std::string_view target_label) {
  return bir::BranchTerminator{
      .target_label = std::string(target_label),
      .target_label_id = block_label_id(module, target_label),
  };
}

bir::CondBranchTerminator make_cond_branch(bir::Module& module,
                                           bir::Value condition,
                                           std::string_view true_label,
                                           std::string_view false_label) {
  return bir::CondBranchTerminator{
      .condition = std::move(condition),
      .true_label = std::string(true_label),
      .false_label = std::string(false_label),
      .true_label_id = block_label_id(module, true_label),
      .false_label_id = block_label_id(module, false_label),
  };
}

bir::PhiIncoming make_phi_incoming(bir::Module& module,
                                   std::string_view label,
                                   bir::Value value) {
  return bir::PhiIncoming{
      .label = std::string(label),
      .value = std::move(value),
      .label_id = block_label_id(module, label),
  };
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

const prepare::PreparedStoragePlanFunction* find_storage_plan_function(
    const prepare::PreparedBirModule& prepared,
    std::string_view function_name) {
  const auto function_id = prepared.names.function_names.find(function_name);
  if (function_id == c4c::kInvalidFunctionName) {
    return nullptr;
  }
  return prepare::find_prepared_storage_plan(prepared, function_id);
}

const prepare::PreparedStoragePlanValue* find_storage_value(
    const prepare::PreparedBirModule& prepared,
    const prepare::PreparedStoragePlanFunction& function,
    std::string_view value_name) {
  for (const auto& value : function.values) {
    if (prepare::prepared_value_name(prepared.names, value.value_name) == value_name) {
      return &value;
    }
  }
  return nullptr;
}

void set_register_group_override(prepare::PreparedBirModule& prepared,
                                 std::string_view function_name,
                                 std::string_view value_name,
                                 prepare::PreparedRegisterClass register_class,
                                 std::size_t contiguous_width) {
  const auto function_id = prepared.names.function_names.find(function_name);
  const auto value_id = prepared.names.value_names.find(value_name);
  if (function_id == c4c::kInvalidFunctionName || value_id == c4c::kInvalidValueName) {
    return;
  }
  prepared.register_group_overrides.values.push_back(prepare::PreparedRegisterGroupOverride{
      .function_name = function_id,
      .value_name = value_id,
      .register_class = register_class,
      .contiguous_width = contiguous_width,
  });
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

bool contains_register_span(const std::vector<prepare::PreparedRegisterCandidateSpan>& spans,
                            const std::vector<std::string>& occupied_register_names) {
  for (const auto& span : spans) {
    if (span.occupied_register_names == occupied_register_names) {
      return true;
    }
  }
  return false;
}

bool spans_overlap(const std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
  for (const auto& lhs_register : lhs) {
    for (const auto& rhs_register : rhs) {
      if (lhs_register == rhs_register) {
        return true;
      }
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

const prepare::PreparedMoveResolution* find_move_bundle_move_by_op_kind(
    const prepare::PreparedMoveBundle& bundle,
    prepare::PreparedMoveResolutionOpKind op_kind,
    prepare::PreparedValueId from_value_id,
    prepare::PreparedValueId to_value_id) {
  for (const auto& move : bundle.moves) {
    if (move.op_kind == op_kind && move.from_value_id == from_value_id &&
        move.to_value_id == to_value_id) {
      return &move;
    }
  }
  return nullptr;
}

int count_value_move_resolution_to_value_with_authority(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId to_value_id,
    prepare::PreparedMoveAuthorityKind authority_kind) {
  int count = 0;
  for (const auto& move : function.move_resolution) {
    if (move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
        move.to_value_id == to_value_id && move.authority_kind == authority_kind) {
      ++count;
    }
  }
  return count;
}

int count_value_move_resolution_to_value_without_authority(
    const prepare::PreparedRegallocFunction& function,
    prepare::PreparedValueId to_value_id,
    prepare::PreparedMoveAuthorityKind authority_kind) {
  int count = 0;
  for (const auto& move : function.move_resolution) {
    if (move.destination_kind == prepare::PreparedMoveDestinationKind::Value &&
        move.to_value_id == to_value_id && move.authority_kind != authority_kind) {
      ++count;
    }
  }
  return count;
}

const prepare::PreparedAbiBinding* find_abi_binding(
    const prepare::PreparedMoveBundle& bundle,
    prepare::PreparedMoveDestinationKind destination_kind,
    std::optional<std::size_t> destination_abi_index) {
  for (const auto& binding : bundle.abi_bindings) {
    if (binding.destination_kind == destination_kind &&
        binding.destination_abi_index == destination_abi_index) {
      return &binding;
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

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.flag"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = make_cond_branch(module, bir::Value::named(bir::TypeKind::I32, "cond0"), "left", "right");

  auto left = make_block(module, "left");
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = make_branch(module, "join");

  auto right = make_block(module, "right");
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "right.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(20),
      .rhs = bir::Value::immediate_i32(2),
  });
  right.terminator = make_branch(module, "join");

  auto join = make_block(module, "join");
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.v"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.v")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.v")),
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

prepare::PreparedBirModule prepare_id_authoritative_phi_module() {
  bir::Module module;

  bir::Function function;
  function.name = "id_authoritative_phi_liveness";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.flag",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.label = "raw.entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.flag"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator =
      make_cond_branch(module, bir::Value::named(bir::TypeKind::I32, "cond0"), "left", "right");
  entry.terminator.true_label = "raw.left.target";
  entry.terminator.false_label = "raw.right.target";

  auto left = make_block(module, "left");
  left.label = "raw.left.block";
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "left.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = make_branch(module, "join");
  left.terminator.target_label = "raw.join.from.left";

  auto right = make_block(module, "right");
  right.label = "raw.right.block";
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "right.v"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(20),
      .rhs = bir::Value::immediate_i32(2),
  });
  right.terminator = make_branch(module, "join");
  right.terminator.target_label = "raw.join.from.right";

  auto join = make_block(module, "join");
  join.label = "raw.join.block";
  auto left_incoming =
      make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.v"));
  left_incoming.label = "raw.left.incoming";
  auto right_incoming =
      make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.v"));
  right_incoming.label = "raw.right.incoming";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.v"),
      .incomings = {std::move(left_incoming), std::move(right_incoming)},
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
  options.run_stack_layout = false;
  options.run_liveness = true;
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
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

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.flag"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = make_cond_branch(module, bir::Value::named(bir::TypeKind::I32, "cond0"), "left", "right");

  auto left = make_block(module, "left");
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
  left.terminator = make_branch(module, "join");

  auto right = make_block(module, "right");
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "right.feed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(50),
      .rhs = bir::Value::immediate_i32(5),
  });
  right.terminator = make_branch(module, "join");

  auto join = make_block(module, "join");
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.move"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.feed")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep0"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot0")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep1"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot1")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep2"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot2")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
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

prepare::PreparedBirModule prepare_select_materialized_join_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "select_materialized_join_move_resolution";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.flag",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I32, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.flag"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = make_cond_branch(module, bir::Value::named(bir::TypeKind::I32, "cond0"), "left", "right");

  auto left = make_block(module, "left");
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
  left.terminator = make_branch(module, "join");

  auto right = make_block(module, "right");
  right.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "right.feed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(50),
      .rhs = bir::Value::immediate_i32(5),
  });
  right.terminator = make_branch(module, "join");

  auto join = make_block(module, "join");
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.move"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.feed")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep0"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot0")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep1"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot1")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
      },
  });
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "phi.keep2"),
      .incomings = {
          make_phi_incoming(module, "left", bir::Value::named(bir::TypeKind::I32, "left.hot2")),
          make_phi_incoming(module, "right", bir::Value::named(bir::TypeKind::I32, "right.feed")),
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
  options.run_out_of_ssa = true;
  options.run_regalloc = true;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  return planner.run();
}

prepare::PreparedBirModule prepare_phi_loop_cycle_move_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "phi_loop_cycle_move_resolution";
  function.return_type = bir::TypeKind::I32;

  auto entry = make_block(module, "entry");
  entry.terminator = make_branch(module, "loop");

  auto loop = make_block(module, "loop");
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "a"),
      .incomings = {
          make_phi_incoming(module, "entry", bir::Value::immediate_i32(1)),
          make_phi_incoming(module, "body", bir::Value::named(bir::TypeKind::I32, "b")),
      },
  });
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "b"),
      .incomings = {
          make_phi_incoming(module, "entry", bir::Value::immediate_i32(2)),
          make_phi_incoming(module, "body", bir::Value::named(bir::TypeKind::I32, "a")),
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I1, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "a"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = make_cond_branch(module, bir::Value::named(bir::TypeKind::I1, "cmp0"), "body", "exit");

  auto body = make_block(module, "body");
  body.terminator = make_branch(module, "loop");

  auto exit = make_block(module, "exit");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

prepare::PreparedBirModule prepare_grouped_call_boundary_move_module_with_regalloc() {
  bir::Module module;

  bir::Function sink_decl;
  sink_decl.name = "sink_pair";
  sink_decl.is_declaration = true;
  sink_decl.return_type = bir::TypeKind::Void;
  sink_decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 8,
      .align_bytes = 8,
      .abi = bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      },
  });
  module.functions.push_back(std::move(sink_decl));

  bir::Function source_decl;
  source_decl.name = "source_pair";
  source_decl.is_declaration = true;
  source_decl.return_type = bir::TypeKind::I32;
  source_decl.return_abi = bir::CallResultAbiInfo{
      .type = bir::TypeKind::I32,
      .primary_class = bir::AbiValueClass::Integer,
  };
  module.functions.push_back(std::move(source_decl));

  bir::Function call_arg_function;
  call_arg_function.name = "grouped_call_arg_move_resolution";
  call_arg_function.return_type = bir::TypeKind::I32;
  auto call_arg_entry = make_block(module, "entry");
  call_arg_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "grouped.arg"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(10),
      .rhs = bir::Value::immediate_i32(20),
  });
  call_arg_entry.insts.push_back(bir::CallInst{
      .callee = "sink_pair",
      .args = {bir::Value::named(bir::TypeKind::I32, "grouped.arg")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 8,
          .align_bytes = 8,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  call_arg_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::immediate_i32(0),
  };
  call_arg_function.blocks.push_back(std::move(call_arg_entry));
  module.functions.push_back(std::move(call_arg_function));

  bir::Function call_result_function;
  call_result_function.name = "grouped_call_result_move_resolution";
  call_result_function.return_type = bir::TypeKind::I32;
  auto call_result_entry = make_block(module, "entry");
  call_result_entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "grouped.call.result"),
      .callee = "source_pair",
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  call_result_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "grouped.call.result"),
  };
  call_result_function.blocks.push_back(std::move(call_result_entry));
  module.functions.push_back(std::move(call_result_function));

  bir::Function return_function;
  return_function.name = "grouped_return_move_resolution";
  return_function.return_type = bir::TypeKind::I32;
  return_function.return_abi = bir::CallResultAbiInfo{
      .type = bir::TypeKind::I32,
      .primary_class = bir::AbiValueClass::Integer,
  };
  auto return_entry = make_block(module, "entry");
  return_entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "grouped.ret.value"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(2),
  });
  return_entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "grouped.ret.value"),
  };
  return_function.blocks.push_back(std::move(return_entry));
  module.functions.push_back(std::move(return_function));

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
  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded,
                              "grouped_call_arg_move_resolution",
                              "grouped.arg",
                              prepare::PreparedRegisterClass::General,
                              2);
  set_register_group_override(seeded,
                              "grouped_call_result_move_resolution",
                              "grouped.call.result",
                              prepare::PreparedRegisterClass::General,
                              2);
  set_register_group_override(seeded,
                              "grouped_return_move_resolution",
                              "grouped.ret.value",
                              prepare::PreparedRegisterClass::General,
                              2);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  regalloc_planner.publish_contract_plans();
  return std::move(regalloc_planner.prepared());
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

  auto entry = make_block(module, "entry");
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

prepare::PreparedBirModule prepare_grouped_evicted_spill_module_with_regalloc() {
  bir::Module module;

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "grouped_evicted_value_spill_ops";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "seed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(99),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
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
      .lhs = bir::Value::immediate_i32(6),
      .rhs = bir::Value::immediate_i32(7),
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
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "local0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "merge0"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "grouped_evicted_value_spill_ops", "carry",
                              prepare::PreparedRegisterClass::Vector, 16);
  set_register_group_override(seeded, "grouped_evicted_value_spill_ops", "local0",
                              prepare::PreparedRegisterClass::Vector, 16);
  set_register_group_override(seeded, "grouped_evicted_value_spill_ops", "hot",
                              prepare::PreparedRegisterClass::Vector, 16);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_general_grouped_evicted_spill_module_with_regalloc() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "general_grouped_evicted_value_spill_ops";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.carry"),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.seed"),
      .rhs = bir::Value::immediate_i32(2),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
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
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "hot.mix6"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "general_grouped_evicted_value_spill_ops", "carry",
                              prepare::PreparedRegisterClass::General, 2);
  set_register_group_override(seeded, "general_grouped_evicted_value_spill_ops", "hot",
                              prepare::PreparedRegisterClass::General, 2);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_float_grouped_evicted_spill_module_with_regalloc() {
  bir::Module module;
  module.target_triple = "riscv64-unknown-linux-gnu";

  bir::Function decl;
  decl.name = "spill_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "float_grouped_evicted_value_spill_ops";
  function.return_type = bir::TypeKind::F32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.carry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.seed",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::F32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "carry"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.carry"),
      .rhs = bir::Value::immediate_f32_bits(0x3f800000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "p.seed"),
      .rhs = bir::Value::immediate_f32_bits(0x40000000U),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "spill_sink",
      .args = {bir::Value::named(bir::TypeKind::F32, "p.arg")},
      .arg_types = {bir::TypeKind::F32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot"),
      .rhs = bir::Value::immediate_f32_bits(0x40e00000U),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix0"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix1"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix2"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix3"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix4"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "hot.mix5"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::F32, "merge"),
      .operand_type = bir::TypeKind::F32,
      .lhs = bir::Value::named(bir::TypeKind::F32, "carry"),
      .rhs = bir::Value::named(bir::TypeKind::F32, "hot.mix6"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::F32, "merge"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_stack_layout();
  planner.run_liveness();

  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "float_grouped_evicted_value_spill_ops", "carry",
                              prepare::PreparedRegisterClass::Float, 2);
  set_register_group_override(seeded, "float_grouped_evicted_value_spill_ops", "hot",
                              prepare::PreparedRegisterClass::Float, 2);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_loop_weighted_priority_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "loop_weighted_priority";
  function.return_type = bir::TypeKind::I32;

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "seed"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = make_branch(module, "loop");

  auto loop = make_block(module, "loop");
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
  loop.terminator = make_cond_branch(module, bir::Value::named(bir::TypeKind::I32, "loop.cond"), "exit", "loop");

  auto exit = make_block(module, "exit");
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

  auto entry = make_block(module, "entry");
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

prepare::PreparedBirModule prepare_indirect_call_preservation_module_with_regalloc() {
  bir::Module module;
  module.globals.push_back(bir::Global{
      .name = "indirect_fn_ptr",
      .type = bir::TypeKind::Ptr,
      .size_bytes = 8,
      .align_bytes = 8,
      .initializer_symbol_name = std::string("indirect_target"),
  });

  bir::Function target;
  target.name = "indirect_target";
  target.is_declaration = true;
  target.return_type = bir::TypeKind::I32;
  target.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(target));

  bir::Function function;
  function.name = "indirect_call_preservation";
  function.return_type = bir::TypeKind::I32;

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(9),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::Ptr, "%callee.ptr"),
      .global_name = "indirect_fn_ptr",
      .align_bytes = 8,
  });
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I32, "call.out"),
      .callee_value = bir::Value::named(bir::TypeKind::Ptr, "%callee.ptr"),
      .args = {bir::Value::named(bir::TypeKind::I32, "carry")},
      .arg_types = {bir::TypeKind::I32},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I32,
          .size_bytes = 4,
          .align_bytes = 4,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i32",
      .return_type = bir::TypeKind::I32,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I32,
          .primary_class = bir::AbiValueClass::Integer,
      },
      .is_indirect = true,
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
  options.run_legalize = false;
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

  auto entry = make_block(module, "entry");
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

  auto entry = make_block(module, "entry");
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

prepare::PreparedBirModule prepare_vector_grouped_linear_scan_module_with_regalloc() {
  bir::Module module;

  bir::Function function;
  function.name = "vector_grouped_linear_scan";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.m2",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.m4",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.m1",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.m2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.m4"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot0"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.m2"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "hot2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot1"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.m4"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "tail"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "hot2"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.m1"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "tail"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();
  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "vector_grouped_linear_scan", "p.m2",
                              prepare::PreparedRegisterClass::Vector, 2);
  set_register_group_override(seeded, "vector_grouped_linear_scan", "p.m4",
                              prepare::PreparedRegisterClass::Vector, 4);
  set_register_group_override(seeded, "vector_grouped_linear_scan", "p.m1",
                              prepare::PreparedRegisterClass::Vector, 1);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_vector_grouped_cross_call_module_with_regalloc() {
  bir::Module module;

  bir::Function decl;
  decl.name = "vec_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "vector_grouped_cross_call";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.vcarry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.local",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.vcarry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.arg"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "vec_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "post.local"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "post.local"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "out"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();
  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "vector_grouped_cross_call", "carry.pre",
                              prepare::PreparedRegisterClass::Vector, 2);
  set_register_group_override(seeded, "vector_grouped_cross_call", "post.local",
                              prepare::PreparedRegisterClass::Vector, 1);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
}

prepare::PreparedBirModule prepare_general_grouped_cross_call_module_with_regalloc() {
  bir::Module module;

  bir::Function decl;
  decl.name = "int_sink";
  decl.is_declaration = true;
  decl.return_type = bir::TypeKind::Void;
  decl.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "arg0",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  module.functions.push_back(std::move(decl));

  bir::Function function;
  function.name = "general_grouped_cross_call";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.gcarry",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.arg",
      .size_bytes = 4,
      .align_bytes = 4,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.local",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  auto entry = make_block(module, "entry");
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.gcarry"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.arg"),
  });
  entry.insts.push_back(bir::CallInst{
      .callee = "int_sink",
      .args = {bir::Value::named(bir::TypeKind::I32, "p.arg")},
      .arg_types = {bir::TypeKind::I32},
      .return_type_name = "void",
      .return_type = bir::TypeKind::Void,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "post.local"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "p.local"),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "out"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "carry.pre"),
      .rhs = bir::Value::named(bir::TypeKind::I32, "post.local"),
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "out"),
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
  options.run_regalloc = false;

  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  planner.run_stack_layout();
  planner.run_liveness();
  auto seeded = std::move(planner.prepared());
  set_register_group_override(seeded, "general_grouped_cross_call", "carry.pre",
                              prepare::PreparedRegisterClass::General, 2);

  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = true;
  prepare::BirPreAlloc regalloc_planner(std::move(seeded), options);
  regalloc_planner.run_regalloc();
  return std::move(regalloc_planner.prepared());
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

int check_id_authoritative_phi_liveness(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_liveness_function(prepared, "id_authoritative_phi_liveness");
  if (function == nullptr) {
    return fail("expected liveness output for id-authoritative phi fixture");
  }
  if (function->blocks.size() != 4) {
    return fail("expected id-authoritative phi fixture to publish all liveness blocks");
  }
  if (prepare::prepared_block_label(prepared.names, function->blocks[0].block_name) != "entry" ||
      prepare::prepared_block_label(prepared.names, function->blocks[1].block_name) != "left" ||
      prepare::prepared_block_label(prepared.names, function->blocks[2].block_name) != "right" ||
      prepare::prepared_block_label(prepared.names, function->blocks[3].block_name) != "join") {
    return fail("expected liveness block labels to come from BlockLabelId spelling");
  }
  if (function->blocks[0].successor_block_indices != std::vector<std::size_t>{1, 2} ||
      function->blocks[1].successor_block_indices != std::vector<std::size_t>{3} ||
      function->blocks[2].successor_block_indices != std::vector<std::size_t>{3} ||
      function->blocks[3].predecessor_block_indices != std::vector<std::size_t>{1, 2}) {
    return fail("expected liveness CFG edges to resolve through BlockLabelId authority");
  }

  const auto* left = find_liveness_value(prepared, *function, "left.v");
  const auto* right = find_liveness_value(prepared, *function, "right.v");
  if (left == nullptr || right == nullptr) {
    return fail("expected id-authoritative phi predecessor values in liveness");
  }
  if (left->use_points != std::vector<std::size_t>{3} ||
      right->use_points != std::vector<std::size_t>{5}) {
    return fail("expected phi incoming predecessor uses to resolve through BlockLabelId authority");
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
  const auto* value_locations =
      prepare::find_prepared_value_location_function(prepared, "phi_join_move_resolution");
  const auto* bir_function = find_module_function(prepared, "phi_join_move_resolution");
  if (control_flow == nullptr || control_flow->parallel_copy_bundles.empty()) {
    return fail("expected phi_join_move_resolution to publish prepared parallel-copy control-flow data");
  }
  if (value_locations == nullptr || bir_function == nullptr) {
    return fail("expected phi_join_move_resolution to publish prepared value-location move bundles");
  }

  const auto* left_parallel_copy =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "left", "join");
  const auto* right_parallel_copy =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "right", "join");
  if (left_parallel_copy == nullptr || right_parallel_copy == nullptr) {
    return fail("expected phi_join_move_resolution to publish both incoming join edges");
  }
  const auto left_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*left_parallel_copy);
  const auto right_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*right_parallel_copy);
  const auto left_execution_block_index =
      prepare::published_prepared_parallel_copy_execution_block_index(
          prepared.names, *bir_function, *left_parallel_copy);
  const auto right_execution_block_index =
      prepare::published_prepared_parallel_copy_execution_block_index(
          prepared.names, *bir_function, *right_parallel_copy);
  if (!left_execution_block.has_value() || !right_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *left_execution_block) != "left" ||
      prepare::prepared_block_label(prepared.names, *right_execution_block) != "right") {
    return fail("expected each join incoming bundle to publish its predecessor-owned execution block");
  }
  if (!left_execution_block_index.has_value() || !right_execution_block_index.has_value() ||
      *left_execution_block_index != 1 || *right_execution_block_index != 2) {
    return fail("expected phi join bundles to publish stable execution-block lookup indices");
  }
  const auto* left_move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, *bir_function, *value_locations, *left_parallel_copy);
  const auto* right_move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, *bir_function, *value_locations, *right_parallel_copy);
  if (left_move_bundle == nullptr || right_move_bundle == nullptr || left_move_bundle == right_move_bundle) {
    return fail("expected direct move-bundle lookup to keep distinct phi edges addressable through published source labels");
  }
  const auto left_label = prepared.names.block_labels.find("left");
  const auto right_label = prepared.names.block_labels.find("right");
  const auto join_label = prepared.names.block_labels.find("join");
  if (left_label == c4c::kInvalidBlockLabel || right_label == c4c::kInvalidBlockLabel ||
      join_label == c4c::kInvalidBlockLabel) {
    return fail("expected phi_join_move_resolution labels to resolve in prepared names");
  }
  if (left_move_bundle->source_parallel_copy_predecessor_label != left_label ||
      left_move_bundle->source_parallel_copy_successor_label != join_label ||
      right_move_bundle->source_parallel_copy_predecessor_label != right_label ||
      right_move_bundle->source_parallel_copy_successor_label != join_label) {
    return fail("expected the published phi join move bundles to keep their owning source edge");
  }
  if (left_move_bundle->block_index == right_move_bundle->block_index &&
      left_move_bundle->instruction_index == right_move_bundle->instruction_index) {
    return fail("expected predecessor-owned phi join bundles to remain distinct in value-location placement");
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
  if (move == nullptr ||
      !prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*move)) {
    return fail(
        "expected the stack-backed left phi incoming to publish explicit out-of-SSA move authority");
  }
  if (move->uses_cycle_temp_source) {
    return fail("expected the acyclic phi join move resolution to read directly from the incoming value");
  }
  const auto expected_phi_destination_kind =
      phi->assigned_register.has_value() ? prepare::PreparedMoveStorageKind::Register
                                         : prepare::PreparedMoveStorageKind::StackSlot;
  if (move->destination_kind != prepare::PreparedMoveDestinationKind::Value ||
      move->destination_storage_kind != expected_phi_destination_kind ||
      move->destination_abi_index.has_value() || move->destination_register_name.has_value()) {
    return fail("expected phi-join move resolution to keep the generic value destination surface");
  }
  const auto* right_step_move =
      prepare::find_prepared_parallel_copy_move_for_step(*right_parallel_copy, 0);
  const auto* right_resolution =
      prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(*right_move_bundle, 0);
  if (right_step_move == nullptr || right_resolution == nullptr) {
    return fail("expected the right-edge phi bundle to publish a direct step-resolution lookup");
  }
  if (right_step_move != &right_parallel_copy->moves[0]) {
    return fail("expected the right-edge phi bundle to keep step lookup anchored to the published move");
  }
  if (right_resolution->from_value_id != right_feed->value_id ||
      right_resolution->to_value_id != phi->value_id ||
      !prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*right_resolution)) {
    return fail("expected the right-edge phi bundle to resolve through published out-of-SSA authority");
  }
  if (right_feed->assigned_register.has_value() &&
      phi->assigned_register.has_value() &&
      right_feed->assigned_register->register_name == phi->assigned_register->register_name) {
    if (!right_resolution->coalesced_by_assigned_storage) {
      return fail("expected matching register-backed phi incoming storage to publish explicit coalescing");
    }
  } else if (right_resolution->coalesced_by_assigned_storage) {
    return fail("expected phi coalescing publication to stay limited to matching assigned storage");
  }
  const auto* right_function_move = find_move_resolution(*function, right_feed->value_id, phi->value_id);
  if (right_function_move == nullptr ||
      right_function_move->source_parallel_copy_step_index !=
          right_resolution->source_parallel_copy_step_index ||
      right_function_move->coalesced_by_assigned_storage !=
          right_resolution->coalesced_by_assigned_storage ||
      right_function_move->authority_kind != right_resolution->authority_kind) {
    return fail("expected right-edge phi consumer lookup to agree with the published move-bundle record");
  }
  if (count_value_move_resolution_to_value_without_authority(
          *function, phi->value_id, prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy) != 0) {
    return fail("expected prepared phi join results to avoid non-authoritative value-move reconstruction");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find(
          "move_bundle phase=block_entry authority=out_of_ssa_parallel_copy block_index=1 instruction_index=0 source_parallel_copy=left -> join") ==
          std::string::npos ||
      prepared_dump.find(
          "move_bundle phase=block_entry authority=out_of_ssa_parallel_copy block_index=2 instruction_index=0 source_parallel_copy=right -> join") ==
          std::string::npos) {
    return fail("expected prepared dumps to publish the owning source edge for phi join move bundles");
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
  const auto* value_locations =
      prepare::find_prepared_value_location_function(prepared, function->function_name);
  const auto* bir_function = find_module_function(prepared, "phi_loop_cycle_move_resolution");
  if (control_flow == nullptr || value_locations == nullptr || bir_function == nullptr) {
    return fail("expected phi_loop_cycle_move_resolution to publish prepared control-flow data");
  }
  const auto* body_bundle =
      prepare::find_prepared_parallel_copy_bundle(prepared.names, *control_flow, "body", "loop");
  if (body_bundle == nullptr || !body_bundle->has_cycle || body_bundle->moves.size() != 2 ||
      body_bundle->steps.size() != 3) {
    return fail("expected the loop backedge to publish an authoritative cycle-breaking bundle");
  }
  const auto body_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*body_bundle);
  const auto body_execution_block_index =
      prepare::published_prepared_parallel_copy_execution_block_index(
          prepared.names, *bir_function, *body_bundle);
  if (!body_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *body_execution_block) != "body") {
    return fail("expected the loop backedge bundle to publish direct execution-block authority");
  }
  if (!body_execution_block_index.has_value() || *body_execution_block_index != 2) {
    return fail("expected the loop backedge bundle to publish a stable execution-block lookup index");
  }
  if (!is_named_i32(body_bundle->moves[0].source_value, "b") ||
      !is_named_i32(body_bundle->moves[0].destination_value, "a") ||
      body_bundle->moves[0].carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      !body_bundle->moves[0].storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *body_bundle->moves[0].storage_name) != "a.phi" ||
      !is_named_i32(body_bundle->moves[1].source_value, "a") ||
      !is_named_i32(body_bundle->moves[1].destination_value, "b") ||
      body_bundle->moves[1].carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      !body_bundle->moves[1].storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *body_bundle->moves[1].storage_name) != "b.phi") {
    return fail("expected the loop backedge bundle to publish edge-store carrier moves for the phi swap");
  }
  if (body_bundle->steps[0].kind != prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp ||
      body_bundle->steps[0].move_index != 0 || body_bundle->steps[0].uses_cycle_temp_source ||
      body_bundle->steps[1].kind != prepare::PreparedParallelCopyStepKind::Move ||
      body_bundle->steps[1].move_index != 0 || body_bundle->steps[1].uses_cycle_temp_source ||
      body_bundle->steps[2].kind != prepare::PreparedParallelCopyStepKind::Move ||
      body_bundle->steps[2].move_index != 1 || !body_bundle->steps[2].uses_cycle_temp_source) {
    return fail("expected the loop backedge bundle to publish the cycle-break ordering directly");
  }
  const auto* saved_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 0);
  const auto* direct_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 1);
  const auto* temp_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 2);
  if (saved_step_move == nullptr || direct_step_move == nullptr || temp_step_move == nullptr) {
    return fail("expected the loop backedge bundle to publish a direct step-to-move lookup seam");
  }
  if (saved_step_move != &body_bundle->moves[0] || direct_step_move != &body_bundle->moves[0] ||
      temp_step_move != &body_bundle->moves[1]) {
    return fail("expected the loop backedge steps to resolve against the published carrier moves");
  }
  const auto* body_move_bundle = prepare::find_prepared_out_of_ssa_parallel_copy_move_bundle(
      prepared.names, *bir_function, *value_locations, *body_bundle);
  if (body_move_bundle == nullptr ||
      !prepare::prepared_move_bundle_has_out_of_ssa_parallel_copy_authority(*body_move_bundle)) {
    return fail(
        "expected the loop backedge bundle to publish a direct out-of-SSA move-bundle lookup seam");
  }
  if (body_move_bundle->moves.size() != 3) {
    return fail("expected the loop backedge move bundle to preserve the published cycle-break step count");
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
  if (save_a_to_temp == nullptr) {
    return fail("expected the published cycle-break save step to lower into move-resolution temp-save bookkeeping");
  }
  if (save_a_to_temp->uses_cycle_temp_source ||
      save_a_to_temp->destination_kind != prepare::PreparedMoveDestinationKind::Value) {
    return fail("expected the temp-save contract to publish a first-class save step without reopening a special destination surface");
  }
  if (!prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*save_a_to_temp)) {
    return fail("expected the temp-save contract to keep explicit out-of-SSA authority");
  }
  if (save_a_to_temp->block_index != body_move_bundle->block_index ||
      save_a_to_temp->instruction_index != body_move_bundle->instruction_index) {
    return fail("expected the published temp-save step to use the authoritative out-of-SSA move-bundle placement");
  }

  const auto* b_to_a = find_move_resolution(*function, b->value_id, a->value_id);
  if (b_to_a == nullptr) {
    return fail("expected the published direct backedge move to lower into move-resolution bookkeeping");
  }
  if (b_to_a->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return fail("expected the direct backedge move to stay a normal move-resolution operation");
  }
  if (b_to_a->uses_cycle_temp_source) {
    return fail("expected the first backedge move to read directly from the original incoming value");
  }
  if (!prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*b_to_a)) {
    return fail("expected the direct backedge move to keep explicit out-of-SSA authority");
  }
  if (b_to_a->block_index != body_move_bundle->block_index ||
      b_to_a->instruction_index != body_move_bundle->instruction_index) {
    return fail("expected the direct backedge move to share the authoritative out-of-SSA move-bundle placement");
  }

  const auto* a_to_b = find_move_resolution(*function, a->value_id, b->value_id);
  if (a_to_b == nullptr) {
    return fail("expected the published cycle-temp backedge move to lower into move-resolution bookkeeping");
  }
  if (a_to_b->op_kind != prepare::PreparedMoveResolutionOpKind::Move) {
    return fail("expected the cycle-broken backedge transfer to stay a normal move-resolution operation");
  }
  if (!a_to_b->uses_cycle_temp_source) {
    return fail("expected the cycle-broken backedge move to publish first-class cycle-temp sourcing");
  }
  if (!prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*a_to_b)) {
    return fail("expected the cycle-broken backedge transfer to keep explicit out-of-SSA authority");
  }
  if (a_to_b->block_index != body_move_bundle->block_index ||
      a_to_b->instruction_index != body_move_bundle->instruction_index) {
    return fail("expected the cycle-broken backedge move to share the authoritative out-of-SSA move-bundle placement");
  }

  const auto* bundled_save_a_to_temp = find_move_bundle_move_by_op_kind(
      *body_move_bundle,
      prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
      a->value_id,
      a->value_id);
  const auto* bundled_b_to_a = find_move_bundle_move_by_op_kind(
      *body_move_bundle,
      prepare::PreparedMoveResolutionOpKind::Move,
      b->value_id,
      a->value_id);
  const auto* bundled_a_to_b = find_move_bundle_move_by_op_kind(
      *body_move_bundle,
      prepare::PreparedMoveResolutionOpKind::Move,
      a->value_id,
      b->value_id);
  if (bundled_save_a_to_temp == nullptr || bundled_b_to_a == nullptr || bundled_a_to_b == nullptr) {
    return fail("expected the authoritative loop backedge move bundle to carry the save/direct/cycle-temp sequence");
  }
  if (prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(*body_move_bundle, 0) !=
          bundled_save_a_to_temp ||
      prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(*body_move_bundle, 1) !=
          bundled_b_to_a ||
      prepare::find_prepared_out_of_ssa_parallel_copy_move_for_step(*body_move_bundle, 2) !=
          bundled_a_to_b) {
    return fail("expected the authoritative loop backedge move bundle to publish per-step move lookup");
  }
  if (bundled_save_a_to_temp->uses_cycle_temp_source || bundled_b_to_a->uses_cycle_temp_source ||
      !bundled_a_to_b->uses_cycle_temp_source) {
    return fail("expected the authoritative loop backedge move bundle to preserve direct-then-cycle-temp sourcing");
  }

  if (count_value_move_resolution_to_value_without_authority(
          *function, a->value_id, prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy) != 0 ||
      count_value_move_resolution_to_value_without_authority(
          *function, b->value_id, prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy) != 0) {
    return fail("expected loop phi cycle values to avoid non-authoritative value-move reconstruction");
  }

  return 0;
}

int check_select_materialized_join_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "select_materialized_join_move_resolution");
  if (function == nullptr) {
    return fail("expected regalloc output for select_materialized_join_move_resolution");
  }
  const auto* control_flow =
      prepare::find_prepared_control_flow_function(prepared.control_flow, function->function_name);
  if (control_flow == nullptr) {
    return fail("expected select-materialized join regalloc proof to publish control-flow data");
  }

  const auto join_block_id = prepared.names.block_labels.find("join");
  if (join_block_id == c4c::kInvalidBlockLabel) {
    return fail("expected the select-materialized join block to stay interned");
  }
  const auto* join_transfer =
      prepare::find_prepared_join_transfer(prepared.names, *control_flow, join_block_id, "phi.move");
  if (join_transfer == nullptr ||
      prepare::effective_prepared_join_transfer_carrier_kind(*join_transfer) !=
          prepare::PreparedJoinTransferCarrierKind::SelectMaterialization) {
    return fail("expected phi.move to stay backed by published select-materialized join authority");
  }
  if (control_flow->parallel_copy_bundles.empty()) {
    return fail("expected select-materialized join regalloc proof to keep published parallel-copy bundles");
  }

  const auto* left_feed = find_regalloc_value(prepared, *function, "left.feed");
  const auto* right_feed = find_regalloc_value(prepared, *function, "right.feed");
  const auto* phi = find_regalloc_value(prepared, *function, "phi.move");
  if (left_feed == nullptr || right_feed == nullptr || phi == nullptr) {
    return fail("expected select-materialized join values to appear in regalloc output");
  }
  if (function->move_resolution.empty()) {
    return fail("expected select-materialized join regalloc proof to publish move-resolution bookkeeping");
  }

  if (count_value_move_resolution_to_value_without_authority(
          *function, phi->value_id, prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy) != 0) {
    return fail(
        "expected select-materialized join results to avoid non-authoritative value-move reconstruction");
  }

  const auto authoritative_phi_moves = count_value_move_resolution_to_value_with_authority(
      *function, phi->value_id, prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy);
  if (authoritative_phi_moves == 0) {
    return fail(
        "expected select-materialized join results to consume published out-of-SSA move authority");
  }

  const auto* left_move = find_move_resolution(*function, left_feed->value_id, phi->value_id);
  const auto* right_move = find_move_resolution(*function, right_feed->value_id, phi->value_id);
  const auto* authoritative_move = left_move != nullptr ? left_move : right_move;
  if (authoritative_move == nullptr ||
      !prepare::prepared_move_resolution_has_out_of_ssa_parallel_copy_authority(*authoritative_move)) {
    return fail(
        "expected select-materialized join move resolution to be sourced from explicit out-of-SSA authority");
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

int check_grouped_evicted_value_spill_ops(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "grouped_evicted_value_spill_ops");
  if (function == nullptr) {
    return fail("expected regalloc output for grouped_evicted_value_spill_ops");
  }

  const auto* carry = find_regalloc_value(prepared, *function, "carry");
  const auto* local0 = find_regalloc_value(prepared, *function, "local0");
  const auto* hot = find_regalloc_value(prepared, *function, "hot");
  if (carry == nullptr || local0 == nullptr || hot == nullptr) {
    return fail("expected grouped carry, local0, and hot in regalloc output");
  }
  if (carry->register_class != prepare::PreparedRegisterClass::Vector ||
      carry->register_group_width != 16 ||
      local0->register_class != prepare::PreparedRegisterClass::Vector ||
      local0->register_group_width != 16 ||
      hot->register_class != prepare::PreparedRegisterClass::Vector ||
      hot->register_group_width != 16) {
    return fail("expected grouped spill fixture to seed LMUL=16 vector widths");
  }

  const auto grouped_spill_it = std::find_if(
      function->spill_reload_ops.begin(),
      function->spill_reload_ops.end(),
      [](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.register_bank == prepare::PreparedRegisterBank::Vreg &&
               op.register_name == std::optional<std::string>{"v0"} &&
               op.contiguous_width == 16 &&
               op.occupied_register_names.size() == 16 &&
               op.occupied_register_names.front() == "v0" &&
               op.occupied_register_names.back() == "v15";
      });
  if (grouped_spill_it == function->spill_reload_ops.end()) {
    return fail("expected an LMUL=16 grouped spill to publish the only legal caller span");
  }

  const auto* spilled_value = grouped_spill_it->value_id == local0->value_id ? local0
                                    : grouped_spill_it->value_id == carry->value_id ? carry
                                    : grouped_spill_it->value_id == hot->value_id ? hot
                                                                                  : nullptr;
  if (spilled_value == nullptr || spilled_value->allocation_status !=
                                      prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !spilled_value->assigned_stack_slot.has_value() || spilled_value->assigned_register.has_value()) {
    return fail("expected the grouped spilled value to fall back to a real stack slot");
  }

  int spill_count = 0;
  int reload_count = 0;
  for (const auto& op : function->spill_reload_ops) {
    if (op.value_id != spilled_value->value_id) {
      continue;
    }
    if (op.register_bank != prepare::PreparedRegisterBank::Vreg ||
        op.register_name != std::optional<std::string>{"v0"} || op.contiguous_width != 16 ||
        op.occupied_register_names.size() != 16 || op.occupied_register_names.front() != "v0" ||
        op.occupied_register_names.back() != "v15" ||
        op.slot_id != std::optional<prepare::PreparedFrameSlotId>{spilled_value->assigned_stack_slot->slot_id} ||
        op.stack_offset_bytes !=
            std::optional<std::size_t>{spilled_value->assigned_stack_slot->offset_bytes}) {
      return fail("expected grouped spill/reload ops to publish vector span and stack-slot authority");
    }
    if (op.op_kind == prepare::PreparedSpillReloadOpKind::Spill) {
      ++spill_count;
    } else if (op.op_kind == prepare::PreparedSpillReloadOpKind::Reload) {
      ++reload_count;
    }
  }
  if (spill_count != 1 || reload_count < 1) {
    return fail("expected the grouped evicted value to publish one spill and at least one grouped reload");
  }

  return 0;
}

int check_general_grouped_evicted_value_spill_ops(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "general_grouped_evicted_value_spill_ops");
  if (function == nullptr) {
    return fail("expected regalloc output for general_grouped_evicted_value_spill_ops");
  }

  const auto* carry = find_regalloc_value(prepared, *function, "carry");
  const auto* hot = find_regalloc_value(prepared, *function, "hot");
  if (carry == nullptr || hot == nullptr) {
    return fail("expected grouped general carry and hot in regalloc output");
  }
  if (carry->register_class != prepare::PreparedRegisterClass::General ||
      carry->register_group_width != 2 ||
      hot->register_class != prepare::PreparedRegisterClass::General ||
      hot->register_group_width != 2) {
    return fail("expected grouped general spill fixture to seed width-2 general spans");
  }
  if (!hot->crosses_call || !hot->assigned_register.has_value() ||
      hot->assigned_register->register_name != "s1" ||
      hot->assigned_register->contiguous_width != 2 ||
      hot->assigned_register->occupied_register_names != std::vector<std::string>{"s1", "s2"}) {
    return fail("expected the hotter grouped general value to keep the only legal callee span");
  }

  const auto grouped_spill_it = std::find_if(
      function->spill_reload_ops.begin(),
      function->spill_reload_ops.end(),
      [carry](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.value_id == carry->value_id &&
               op.register_bank == prepare::PreparedRegisterBank::Gpr &&
               op.register_name == std::optional<std::string>{"s1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"s1", "s2"};
      });
  if (grouped_spill_it == function->spill_reload_ops.end()) {
    return fail("expected a grouped general spill to publish the full callee span");
  }
  if (carry->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !carry->assigned_stack_slot.has_value() || carry->assigned_register.has_value() ||
      !carry->spill_register_authority.has_value() ||
      carry->spill_register_authority->register_name != grouped_spill_it->register_name ||
      carry->spill_register_authority->reg_class != prepare::PreparedRegisterClass::General ||
      carry->spill_register_authority->contiguous_width != grouped_spill_it->contiguous_width ||
      carry->spill_register_authority->occupied_register_names !=
          grouped_spill_it->occupied_register_names) {
    return fail("expected the grouped general spilled value to keep truthful spill-register authority");
  }

  int spill_count = 0;
  int reload_count = 0;
  for (const auto& op : function->spill_reload_ops) {
    if (op.value_id != carry->value_id) {
      continue;
    }
    if (op.register_bank != prepare::PreparedRegisterBank::Gpr ||
        op.register_name != std::optional<std::string>{"s1"} || op.contiguous_width != 2 ||
        op.occupied_register_names != std::vector<std::string>{"s1", "s2"} ||
        op.slot_id != std::optional<prepare::PreparedFrameSlotId>{carry->assigned_stack_slot->slot_id} ||
        op.stack_offset_bytes !=
            std::optional<std::size_t>{carry->assigned_stack_slot->offset_bytes}) {
      return fail("expected grouped general spill/reload ops to publish span and stack-slot authority");
    }
    if (op.op_kind == prepare::PreparedSpillReloadOpKind::Spill) {
      ++spill_count;
    } else if (op.op_kind == prepare::PreparedSpillReloadOpKind::Reload) {
      ++reload_count;
    }
  }
  if (spill_count != 1 || reload_count < 1) {
    return fail("expected the grouped general spilled value to publish one spill and at least one reload");
  }

  return 0;
}

int check_float_grouped_evicted_value_spill_ops(const prepare::PreparedBirModule& prepared) {
  const auto* function = find_regalloc_function(prepared, "float_grouped_evicted_value_spill_ops");
  if (function == nullptr) {
    return fail("expected regalloc output for float_grouped_evicted_value_spill_ops");
  }

  const auto* carry = find_regalloc_value(prepared, *function, "carry");
  const auto* hot = find_regalloc_value(prepared, *function, "hot");
  if (carry == nullptr || hot == nullptr) {
    return fail("expected grouped float carry and hot in regalloc output");
  }
  if (carry->register_class != prepare::PreparedRegisterClass::Float ||
      carry->register_group_width != 2 ||
      hot->register_class != prepare::PreparedRegisterClass::Float ||
      hot->register_group_width != 2) {
    return fail("expected grouped float spill fixture to seed width-2 float spans");
  }
  if (!hot->crosses_call || !hot->assigned_register.has_value() ||
      hot->assigned_register->register_name != "fs1" ||
      hot->assigned_register->contiguous_width != 2 ||
      hot->assigned_register->occupied_register_names != std::vector<std::string>{"fs1", "fs2"}) {
    return fail("expected the hotter grouped float value to keep the only legal callee span");
  }

  const auto grouped_spill_it = std::find_if(
      function->spill_reload_ops.begin(),
      function->spill_reload_ops.end(),
      [carry](const prepare::PreparedSpillReloadOp& op) {
        return op.op_kind == prepare::PreparedSpillReloadOpKind::Spill &&
               op.value_id == carry->value_id &&
               op.register_bank == prepare::PreparedRegisterBank::Fpr &&
               op.register_name == std::optional<std::string>{"fs1"} &&
               op.contiguous_width == 2 &&
               op.occupied_register_names == std::vector<std::string>{"fs1", "fs2"};
      });
  if (grouped_spill_it == function->spill_reload_ops.end()) {
    return fail("expected a grouped float spill to publish the full callee span");
  }
  if (carry->allocation_status != prepare::PreparedAllocationStatus::AssignedStackSlot ||
      !carry->assigned_stack_slot.has_value() || carry->assigned_register.has_value() ||
      !carry->spill_register_authority.has_value() ||
      carry->spill_register_authority->register_name != grouped_spill_it->register_name ||
      carry->spill_register_authority->reg_class != prepare::PreparedRegisterClass::Float ||
      carry->spill_register_authority->contiguous_width != grouped_spill_it->contiguous_width ||
      carry->spill_register_authority->occupied_register_names !=
          grouped_spill_it->occupied_register_names) {
    return fail("expected the grouped float spilled value to keep truthful spill-register authority");
  }

  int spill_count = 0;
  int reload_count = 0;
  for (const auto& op : function->spill_reload_ops) {
    if (op.value_id != carry->value_id) {
      continue;
    }
    if (op.register_bank != prepare::PreparedRegisterBank::Fpr ||
        op.register_name != std::optional<std::string>{"fs1"} || op.contiguous_width != 2 ||
        op.occupied_register_names != std::vector<std::string>{"fs1", "fs2"} ||
        op.slot_id != std::optional<prepare::PreparedFrameSlotId>{carry->assigned_stack_slot->slot_id} ||
        op.stack_offset_bytes !=
            std::optional<std::size_t>{carry->assigned_stack_slot->offset_bytes}) {
      return fail("expected grouped float spill/reload ops to publish span and stack-slot authority");
    }
    if (op.op_kind == prepare::PreparedSpillReloadOpKind::Spill) {
      ++spill_count;
    } else if (op.op_kind == prepare::PreparedSpillReloadOpKind::Reload) {
      ++reload_count;
    }
  }
  if (spill_count != 1 || reload_count < 1) {
    return fail("expected the grouped float spilled value to publish one spill and at least one reload");
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

int check_grouped_call_boundary_move_resolution(const prepare::PreparedBirModule& prepared) {
  const auto* call_arg_function =
      find_regalloc_function(prepared, "grouped_call_arg_move_resolution");
  const auto* call_result_function =
      find_regalloc_function(prepared, "grouped_call_result_move_resolution");
  const auto* return_function =
      find_regalloc_function(prepared, "grouped_return_move_resolution");
  if (call_arg_function == nullptr || call_result_function == nullptr ||
      return_function == nullptr) {
    return fail("expected grouped call-boundary functions in regalloc output");
  }

  const auto* grouped_arg = find_regalloc_value(prepared, *call_arg_function, "grouped.arg");
  if (grouped_arg == nullptr) {
    return fail("expected grouped call argument value in regalloc output");
  }
  const auto* arg_move =
      find_move_resolution(*call_arg_function, grouped_arg->value_id, grouped_arg->value_id);
  if (arg_move == nullptr || arg_move->destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      arg_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      arg_move->destination_abi_index != std::optional<std::size_t>{0} ||
      arg_move->destination_register_name != std::optional<std::string>{"a0"} ||
      arg_move->destination_contiguous_width != 2 ||
      arg_move->destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected grouped call-argument moves to publish the full ABI register span");
  }
  const auto* call_arg_locations =
      prepare::find_prepared_value_location_function(prepared, "grouped_call_arg_move_resolution");
  const auto* before_call_bundle =
      call_arg_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_arg_locations,
                                               prepare::PreparedMovePhase::BeforeCall,
                                               0,
                                               1);
  const auto* arg_binding = before_call_bundle == nullptr
                                ? nullptr
                                : find_abi_binding(*before_call_bundle,
                                                   prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                                                   0);
  if (arg_binding == nullptr ||
      arg_binding->destination_register_name != std::optional<std::string>{"a0"} ||
      arg_binding->destination_contiguous_width != 2 ||
      arg_binding->destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected grouped call-argument ABI bindings to preserve grouped register authority");
  }
  const auto* call_arg_plans =
      prepare::find_prepared_call_plans(prepared, call_arg_function->function_name);
  if (call_arg_plans == nullptr || call_arg_plans->calls.size() != 1 ||
      call_arg_plans->calls.front().arguments.size() != 1) {
    return fail("expected grouped call-argument fixture to publish call plans");
  }
  const auto& arg_plan = call_arg_plans->calls.front().arguments.front();
  if (arg_plan.destination_register_name != std::optional<std::string>{"a0"} ||
      arg_plan.destination_contiguous_width != 2 ||
      arg_plan.destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected call plans to preserve grouped call-argument ABI span authority");
  }

  const auto* grouped_call_result =
      find_regalloc_value(prepared, *call_result_function, "grouped.call.result");
  if (grouped_call_result == nullptr) {
    return fail("expected grouped call result value in regalloc output");
  }
  const auto* result_move = find_move_resolution(
      *call_result_function, grouped_call_result->value_id, grouped_call_result->value_id);
  if (result_move == nullptr ||
      result_move->destination_kind != prepare::PreparedMoveDestinationKind::CallResultAbi ||
      result_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      result_move->destination_abi_index.has_value() ||
      result_move->destination_register_name != std::optional<std::string>{"a0"} ||
      result_move->destination_contiguous_width != 2 ||
      result_move->destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected grouped call-result moves to publish the full ABI register span");
  }
  const auto* call_result_locations =
      prepare::find_prepared_value_location_function(prepared, "grouped_call_result_move_resolution");
  const auto* after_call_bundle =
      call_result_locations == nullptr
          ? nullptr
          : prepare::find_prepared_move_bundle(*call_result_locations,
                                               prepare::PreparedMovePhase::AfterCall,
                                               0,
                                               0);
  const auto* result_binding =
      after_call_bundle == nullptr
          ? nullptr
          : find_abi_binding(*after_call_bundle,
                             prepare::PreparedMoveDestinationKind::CallResultAbi,
                             std::nullopt);
  if (result_binding == nullptr ||
      result_binding->destination_register_name != std::optional<std::string>{"a0"} ||
      result_binding->destination_contiguous_width != 2 ||
      result_binding->destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected grouped call-result ABI bindings to preserve grouped register authority");
  }
  const auto* call_result_plans =
      prepare::find_prepared_call_plans(prepared, call_result_function->function_name);
  if (call_result_plans == nullptr || call_result_plans->calls.size() != 1 ||
      !call_result_plans->calls.front().result.has_value()) {
    return fail("expected grouped call-result fixture to publish call plans");
  }
  const auto& result_plan = *call_result_plans->calls.front().result;
  if (result_plan.source_register_name != std::optional<std::string>{"a0"} ||
      result_plan.source_contiguous_width != 2 ||
      result_plan.source_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected call plans to preserve grouped call-result ABI span authority");
  }

  const auto* grouped_ret =
      find_regalloc_value(prepared, *return_function, "grouped.ret.value");
  if (grouped_ret == nullptr) {
    return fail("expected grouped return value in regalloc output");
  }
  const auto* return_move =
      find_move_resolution(*return_function, grouped_ret->value_id, grouped_ret->value_id);
  if (return_move == nullptr ||
      return_move->destination_kind != prepare::PreparedMoveDestinationKind::FunctionReturnAbi ||
      return_move->destination_storage_kind != prepare::PreparedMoveStorageKind::Register ||
      return_move->destination_abi_index.has_value() ||
      return_move->destination_register_name != std::optional<std::string>{"a0"} ||
      return_move->destination_contiguous_width != 2 ||
      return_move->destination_occupied_register_names != std::vector<std::string>{"a0", "a1"}) {
    return fail("expected grouped returns to publish the full ABI register span");
  }

  const std::string prepared_dump = prepare::print(prepared);
  const auto arg_summary_pos =
      prepared_dump.find("callsite block=0 inst=1 wrapper=direct_extern_fixed_arity");
  if (arg_summary_pos == std::string::npos ||
      prepared_dump.find("to=a0/w2[a0,a1]", arg_summary_pos) == std::string::npos) {
    return fail("expected prepared summary to publish grouped call-argument span authority");
  }
  if (prepared_dump.find("result bank=gpr from=register:a0/w2[a0,a1]") == std::string::npos) {
    return fail("expected prepared summary to publish grouped call-result source span authority");
  }
  const auto arg_detail_pos = prepared_dump.find("arg index=0 value_bank=gpr");
  if (arg_detail_pos == std::string::npos ||
      prepared_dump.find("dest_reg=a0 width=2 units=a0,a1", arg_detail_pos) == std::string::npos) {
    return fail("expected prepared call-plan detail to publish grouped call-argument span authority");
  }
  const auto result_detail_pos =
      prepared_dump.find("result value_bank=gpr source_storage=register");
  if (result_detail_pos == std::string::npos ||
      prepared_dump.find("source_reg=a0", result_detail_pos) == std::string::npos ||
      prepared_dump.find("width=2 units=a0,a1", result_detail_pos) == std::string::npos) {
    return fail("expected prepared call-plan detail to publish grouped call-result span authority");
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

  const auto* regalloc = find_regalloc_function(
      prepared, "lowered_same_module_variadic_global_byval_metadata");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for lowered_same_module_variadic_global_byval_metadata");
  }

  const auto* call_plans = prepare::find_prepared_call_plans(prepared, regalloc->function_name);
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail(
        "expected lowered_same_module_variadic_global_byval_metadata to publish one prepared call plan");
  }

  const auto* global_byval = find_regalloc_value(prepared, *regalloc, "@gpair");
  if (global_byval == nullptr) {
    return fail(
        "expected lowered_same_module_variadic_global_byval_metadata to publish the global byval source");
  }

  const auto& call_plan = call_plans->calls.front();
  if (call_plan.wrapper_kind != prepare::PreparedCallWrapperKind::SameModule ||
      call_plan.variadic_fpr_arg_register_count != 0 || call_plan.is_indirect ||
      !call_plan.direct_callee_name.has_value() || *call_plan.direct_callee_name != "myprintf" ||
      call_plan.indirect_callee.has_value() || call_plan.arguments.size() != 2) {
    return fail("expected same-module variadic aggregate call to publish same-module call-plan authority");
  }
  if (!call_plan.arguments[1].source_register_name.has_value()) {
    return fail(
        "expected same-module variadic aggregate call to keep the byval source materialized on the shared storage seam");
  }
  if (!call_plan.arguments[1].destination_register_name.has_value()) {
    return fail(
        "expected same-module variadic aggregate call to publish a concrete ABI destination for the byval argument");
  }
  if (call_plan.arguments[1].value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      call_plan.arguments[1].source_encoding != prepare::PreparedStorageEncodingKind::SymbolAddress ||
      call_plan.arguments[1].source_value_id !=
          std::optional<prepare::PreparedValueId>{global_byval->value_id} ||
      call_plan.arguments[1].source_symbol_name != std::optional<std::string>{"@gpair"} ||
      call_plan.arguments[1].source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
      call_plan.arguments[1].destination_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::AggregateAddress}) {
    return fail(
        "expected same-module variadic aggregate call to publish the symbol-backed byval argument authority");
  }

  const std::string prepared_dump = prepare::print(prepared);
  const std::string summary_arg_prefix =
      "arg1 bank=aggregate_address from=symbol_address:@gpair to=";
  const auto summary_arg_pos = prepared_dump.find(summary_arg_prefix);
  const std::string detail_arg_prefix =
      "arg index=1 value_bank=aggregate_address source_encoding=symbol_address";
  const auto detail_arg_pos = prepared_dump.find(detail_arg_prefix);
  if (prepared_dump.find(
          "callsite block=0 inst=0 wrapper=same_module callee=myprintf variadic_fpr_args=0 args=2") ==
          std::string::npos ||
      summary_arg_pos == std::string::npos) {
    return fail(
        "expected prepared printer summary to publish same-module variadic byval call authority");
  }
  if (prepared_dump.find('\n', summary_arg_pos + summary_arg_prefix.size()) ==
      summary_arg_pos + summary_arg_prefix.size()) {
    return fail(
        "expected prepared printer summary to publish a concrete ABI destination for the same-module variadic byval argument");
  }
  if (prepared_dump.find(
          "call block_index=0 inst_index=0 wrapper_kind=same_module variadic_fpr_arg_register_count=0 indirect=no callee=myprintf") ==
          std::string::npos ||
      detail_arg_pos == std::string::npos ||
      prepared_dump.find("source_symbol=@gpair", detail_arg_pos) == std::string::npos ||
      prepared_dump.find("dest_bank=aggregate_address", detail_arg_pos) == std::string::npos ||
      prepared_dump.find("dest_reg=", detail_arg_pos) == std::string::npos) {
    return fail(
        "expected prepared printer call-plan detail to publish same-module variadic byval authority");
  }

  return 0;
}

int check_x86_consumer_surface_reads_same_module_variadic_global_byval_authority(
    const prepare::PreparedBirModule& prepared) {
  const auto function_id = prepare::resolve_prepared_function_name_id(
      prepared.names, "lowered_same_module_variadic_global_byval_metadata");
  const auto* regalloc = find_regalloc_function(
      prepared, "lowered_same_module_variadic_global_byval_metadata");
  const auto* call_plans = function_id.has_value()
                               ? prepare::find_prepared_call_plans(prepared, *function_id)
                               : nullptr;
  const auto* storage = find_storage_plan_function(
      prepared, "lowered_same_module_variadic_global_byval_metadata");
  if (!function_id.has_value() || regalloc == nullptr || call_plans == nullptr ||
      storage == nullptr) {
    return fail(
        "expected same-module variadic aggregate call to publish regalloc, call, and storage authority");
  }

  const auto consumed = c4c::backend::x86::consume_plans(
      prepared, "lowered_same_module_variadic_global_byval_metadata");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != call_plans || consumed.regalloc != regalloc || consumed.storage != storage) {
    return fail(
        "expected x86 to read same-module variadic aggregate call authority directly from prepared plans");
  }

  const auto* global_byval = find_regalloc_value(prepared, *regalloc, "@gpair");
  const auto* format_storage = find_storage_value(prepared, *storage, "%format");
  const auto* call = c4c::backend::x86::find_consumed_call_plan(consumed, 0, 0);
  const auto* byval_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(consumed, 0, 0, 1);
  if (global_byval == nullptr || format_storage == nullptr || call == nullptr || byval_arg == nullptr ||
      consumed.dynamic_stack != nullptr || call->arguments.size() != 2) {
    return fail(
        "expected x86 same-module variadic fixture to expose direct call, argument, and storage authority");
  }

  const auto* format_arg =
      c4c::backend::x86::find_consumed_call_argument_plan(consumed, 0, 0, 0);
  if (format_arg == nullptr || format_arg != &call->arguments.front() || byval_arg != &call->arguments[1]) {
    return fail(
        "expected x86 same-module variadic fixture to resolve both arguments through the consumed call selectors");
  }
  if (call->wrapper_kind != prepare::PreparedCallWrapperKind::SameModule || call->is_indirect ||
      call->direct_callee_name != std::optional<std::string>{"myprintf"} ||
      call->variadic_fpr_arg_register_count != 0 || call->result.has_value()) {
    return fail("expected x86 same-module variadic fixture to preserve same-module wrapper authority");
  }
  if (format_arg->source_value_id != std::optional<prepare::PreparedValueId>{format_storage->value_id} ||
      format_arg->source_encoding != format_storage->encoding ||
      format_arg->source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{format_storage->bank}) {
    return fail(
        "expected x86 same-module variadic fixture to preserve the direct format-argument storage seam");
  }
  if (byval_arg->source_value_id != std::optional<prepare::PreparedValueId>{global_byval->value_id} ||
      byval_arg->source_symbol_name != std::optional<std::string>{"@gpair"} ||
      byval_arg->source_encoding != prepare::PreparedStorageEncodingKind::SymbolAddress ||
      byval_arg->value_bank != prepare::PreparedRegisterBank::AggregateAddress ||
      byval_arg->source_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{prepare::PreparedRegisterBank::Gpr} ||
      !byval_arg->source_register_name.has_value() ||
      byval_arg->destination_register_bank !=
          std::optional<prepare::PreparedRegisterBank>{
              prepare::PreparedRegisterBank::AggregateAddress} ||
      !byval_arg->destination_register_name.has_value()) {
    return fail(
        "expected x86 same-module variadic fixture to preserve the byval global source and ABI destination directly");
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

int check_indirect_call_preservation_publication(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "indirect_call_preservation");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for indirect_call_preservation");
  }

  const auto* call_plans = prepare::find_prepared_call_plans(prepared, regalloc->function_name);
  if (call_plans == nullptr || call_plans->calls.size() != 1) {
    return fail("expected indirect_call_preservation to publish one prepared call plan");
  }

  const auto* carry = find_regalloc_value(prepared, *regalloc, "carry");
  const auto* callee_ptr = find_regalloc_value(prepared, *regalloc, "%callee.ptr");
  if (carry == nullptr || callee_ptr == nullptr) {
    return fail("expected indirect_call_preservation values to appear in regalloc output");
  }

  const auto& call = call_plans->calls.front();
  if (!call.is_indirect || call.wrapper_kind != prepare::PreparedCallWrapperKind::Indirect ||
      !call.indirect_callee.has_value()) {
    return fail("expected indirect_call_preservation to publish indirect call authority");
  }
  if (call.indirect_callee->value_name != callee_ptr->value_name ||
      call.indirect_callee->value_id != std::optional<prepare::PreparedValueId>{callee_ptr->value_id} ||
      call.indirect_callee->encoding != prepare::PreparedStorageEncodingKind::Register ||
      call.indirect_callee->bank != prepare::PreparedRegisterBank::Gpr) {
    return fail("expected the prepared indirect callee publication to preserve the shared register-backed authority seam");
  }

  const auto preserved_it = std::find_if(
      call.preserved_values.begin(),
      call.preserved_values.end(),
      [&](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call.preserved_values.end()) {
    return fail("expected indirect_call_preservation to publish cross-call preservation for carry");
  }
  if (preserved_it->route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved_it->register_bank != std::optional<prepare::PreparedRegisterBank>{
                                   prepare::PreparedRegisterBank::Gpr}) {
    return fail("expected indirect_call_preservation to publish carry through the callee-saved preservation seam");
  }

  const std::string prepared_dump = prepare::print(prepared);
  if (prepared_dump.find(
          "callsite block=0 inst=2 wrapper=indirect variadic_fpr_args=0 args=1 indirect_callee=%callee.ptr indirect_home=register indirect_bank=gpr") ==
      std::string::npos ||
      prepared_dump.find("preserves=carry#") == std::string::npos) {
    return fail("expected prepared printer summary to publish indirect-callee and preserved-carry authority");
  }
  if (prepared_dump.find(
          "call block_index=0 inst_index=2 wrapper_kind=indirect variadic_fpr_arg_register_count=0 indirect=yes indirect_callee=%callee.ptr indirect_encoding=register indirect_bank=gpr") ==
      std::string::npos) {
    return fail("expected prepared printer call-plan detail to publish the indirect-callee authority seam");
  }
  if (prepared_dump.find("preserve value=carry value_id=") == std::string::npos ||
      prepared_dump.find("route=callee_saved_register") == std::string::npos) {
    return fail("expected prepared printer call-plan detail to publish the preserved carry route");
  }

  return 0;
}

int check_x86_consumer_surface_reads_indirect_call_preservation_authority(
    const prepare::PreparedBirModule& prepared) {
  const auto function_id =
      prepare::resolve_prepared_function_name_id(prepared.names, "indirect_call_preservation");
  const auto* regalloc = find_regalloc_function(prepared, "indirect_call_preservation");
  const auto* storage = find_storage_plan_function(prepared, "indirect_call_preservation");
  if (!function_id.has_value() || regalloc == nullptr || storage == nullptr) {
    return fail(
        "expected indirect_call_preservation to publish regalloc and storage authority");
  }

  const auto consumed =
      c4c::backend::x86::consume_plans(prepared, "indirect_call_preservation");
  if (consumed.frame != prepare::find_prepared_frame_plan(prepared, *function_id) ||
      consumed.dynamic_stack != prepare::find_prepared_dynamic_stack_plan(prepared, *function_id) ||
      consumed.calls != prepare::find_prepared_call_plans(prepared, *function_id) ||
      consumed.regalloc != regalloc || consumed.storage != storage) {
    return fail(
        "expected x86 to read indirect-call preservation authority directly from prepared plans");
  }

  const auto* carry = find_regalloc_value(prepared, *regalloc, "carry");
  const auto* callee_ptr = find_regalloc_value(prepared, *regalloc, "%callee.ptr");
  const auto* call_out = find_regalloc_value(prepared, *regalloc, "call.out");
  const auto* callee_storage = find_storage_value(prepared, *storage, "%callee.ptr");
  const auto* call = c4c::backend::x86::find_consumed_call_plan(consumed, 0, 2);
  const auto* arg = c4c::backend::x86::find_consumed_call_argument_plan(consumed, 0, 2, 0);
  const auto* result = c4c::backend::x86::find_consumed_call_result_plan(consumed, 0, 2);
  if (carry == nullptr || callee_ptr == nullptr || call_out == nullptr || callee_storage == nullptr ||
      call == nullptr || arg == nullptr || result == nullptr || consumed.dynamic_stack != nullptr) {
    return fail(
        "expected x86 indirect-call fixture to expose direct call, argument, result, and storage authority");
  }

  if (!call->is_indirect || call->wrapper_kind != prepare::PreparedCallWrapperKind::Indirect ||
      call->direct_callee_name.has_value() || !call->indirect_callee.has_value() ||
      arg != &call->arguments.front() || result != &*call->result) {
    return fail("expected x86 indirect-call fixture to preserve indirect call selectors");
  }
  if (call->indirect_callee->value_id != std::optional<prepare::PreparedValueId>{callee_ptr->value_id} ||
      call->indirect_callee->encoding != callee_storage->encoding ||
      call->indirect_callee->bank != callee_storage->bank ||
      !call->indirect_callee->register_name.has_value()) {
    return fail(
        "expected x86 indirect-call fixture to preserve the shared indirect-callee storage seam");
  }
  if (arg->source_value_id != std::optional<prepare::PreparedValueId>{carry->value_id} ||
      arg->value_bank != prepare::PreparedRegisterBank::Gpr ||
      result->destination_value_id != std::optional<prepare::PreparedValueId>{call_out->value_id} ||
      result->value_bank != prepare::PreparedRegisterBank::Gpr) {
    return fail(
        "expected x86 indirect-call fixture to preserve direct argument/result value identity");
  }

  const auto preserved_it = std::find_if(
      call->preserved_values.begin(),
      call->preserved_values.end(),
      [&](const prepare::PreparedCallPreservedValue& preserved) {
        return preserved.value_id == carry->value_id;
      });
  if (preserved_it == call->preserved_values.end() ||
      preserved_it->route != prepare::PreparedCallPreservationRoute::CalleeSavedRegister ||
      preserved_it->register_bank != std::optional<prepare::PreparedRegisterBank>{
                                   prepare::PreparedRegisterBank::Gpr}) {
    return fail(
        "expected x86 indirect-call fixture to preserve carry through the published callee-saved route");
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

int check_scalar_grouped_span_candidate_legality() {
  const auto profile = riscv_target_profile();
  const auto general_spans =
      prepare::callee_saved_register_spans(profile, prepare::PreparedRegisterClass::General, 2);
  const auto float_spans =
      prepare::callee_saved_register_spans(profile, prepare::PreparedRegisterClass::Float, 2);

  if (general_spans.size() != 1 ||
      general_spans[0].occupied_register_names != std::vector<std::string>{"s1", "s2"}) {
    return fail("expected grouped general candidate spans to preserve contiguous callee units");
  }
  if (float_spans.size() != 1 ||
      float_spans[0].occupied_register_names != std::vector<std::string>{"fs1", "fs2"}) {
    return fail("expected grouped float candidate spans to preserve contiguous callee units");
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
  return 0;
}

int check_vector_grouped_linear_scan(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "vector_grouped_linear_scan");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for vector_grouped_linear_scan");
  }

  const auto* m2 = find_regalloc_value(prepared, *regalloc, "p.m2");
  const auto* m4 = find_regalloc_value(prepared, *regalloc, "p.m4");
  const auto* m1 = find_regalloc_value(prepared, *regalloc, "p.m1");
  if (m2 == nullptr || m4 == nullptr || m1 == nullptr) {
    return fail("expected grouped vector params to appear in regalloc output");
  }
  if (m2->register_class != prepare::PreparedRegisterClass::Vector || m2->register_group_width != 2 ||
      m4->register_class != prepare::PreparedRegisterClass::Vector || m4->register_group_width != 4 ||
      m1->register_class != prepare::PreparedRegisterClass::Vector || m1->register_group_width != 1) {
    return fail("expected grouped vector overrides to reach regalloc seeds");
  }
  if (!m2->assigned_register.has_value() || !m4->assigned_register.has_value() ||
      !m1->assigned_register.has_value()) {
    return fail("expected grouped vector values to receive concrete register spans");
  }

  const auto caller_m2_spans = prepare::caller_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::Vector, 2);
  const auto caller_m4_spans = prepare::caller_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::Vector, 4);
  const auto caller_m1_spans = prepare::caller_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::Vector, 1);

  if (m2->assigned_register->contiguous_width != 2 ||
      !contains_register_span(caller_m2_spans, m2->assigned_register->occupied_register_names)) {
    return fail("expected LMUL=2 value to use a legal contiguous caller span");
  }
  if (m4->assigned_register->contiguous_width != 4 ||
      !contains_register_span(caller_m4_spans, m4->assigned_register->occupied_register_names)) {
    return fail("expected LMUL=4 value to use a legal width-4 caller span");
  }
  if (m1->assigned_register->contiguous_width != 1 ||
      !contains_register_span(caller_m1_spans, m1->assigned_register->occupied_register_names)) {
    return fail("expected width-1 vector value to stay within the caller pool");
  }
  if (spans_overlap(m2->assigned_register->occupied_register_names,
                    m4->assigned_register->occupied_register_names) ||
      spans_overlap(m2->assigned_register->occupied_register_names,
                    m1->assigned_register->occupied_register_names) ||
      spans_overlap(m4->assigned_register->occupied_register_names,
                    m1->assigned_register->occupied_register_names)) {
    return fail("expected grouped vector assignments to avoid occupied units across widths");
  }

  return 0;
}

int check_vector_grouped_cross_call(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "vector_grouped_cross_call");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for vector_grouped_cross_call");
  }

  const auto* vcarry = find_regalloc_value(prepared, *regalloc, "carry.pre");
  const auto* local = find_regalloc_value(prepared, *regalloc, "post.local");
  if (vcarry == nullptr || local == nullptr) {
    return fail("expected vector grouped cross-call values in regalloc output");
  }
  if (!vcarry->crosses_call || vcarry->register_group_width != 2 ||
      !vcarry->assigned_register.has_value()) {
    return fail("expected call-crossing grouped vector value to keep its published grouped seed");
  }
  if (local->crosses_call || !local->assigned_register.has_value()) {
    return fail("expected non-crossing local vector value to receive a caller-pool assignment");
  }

  const auto callee_m2_spans = prepare::callee_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::Vector, 2);
  const auto caller_m1_spans = prepare::caller_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::Vector, 1);
  if (vcarry->assigned_register->contiguous_width != 2 ||
      !contains_register_span(callee_m2_spans, vcarry->assigned_register->occupied_register_names)) {
    return fail("expected call-crossing grouped vector value to use a legal callee span");
  }
  if (local->assigned_register->contiguous_width != 1 ||
      !contains_register_span(caller_m1_spans, local->assigned_register->occupied_register_names)) {
    return fail("expected non-crossing local vector value to stay in the caller pool");
  }
  if (spans_overlap(vcarry->assigned_register->occupied_register_names,
                    local->assigned_register->occupied_register_names)) {
    return fail("expected caller and callee grouped vector assignments to stay disjoint");
  }

  return 0;
}

int check_general_grouped_cross_call(const prepare::PreparedBirModule& prepared) {
  const auto* regalloc = find_regalloc_function(prepared, "general_grouped_cross_call");
  if (regalloc == nullptr) {
    return fail("expected regalloc output for general_grouped_cross_call");
  }

  const auto* gcarry = find_regalloc_value(prepared, *regalloc, "carry.pre");
  const auto* local = find_regalloc_value(prepared, *regalloc, "post.local");
  if (gcarry == nullptr || local == nullptr) {
    return fail("expected grouped general cross-call values in regalloc output");
  }
  if (!gcarry->crosses_call || gcarry->register_group_width != 2 ||
      !gcarry->assigned_register.has_value()) {
    return fail("expected call-crossing grouped general value to keep its grouped seed");
  }
  if (local->crosses_call || !local->assigned_register.has_value()) {
    return fail("expected non-crossing general local value to receive a caller-pool assignment");
  }

  const auto callee_m2_spans = prepare::callee_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::General, 2);
  const auto caller_m1_spans = prepare::caller_saved_register_spans(
      prepared.target_profile, prepare::PreparedRegisterClass::General, 1);
  if (gcarry->assigned_register->contiguous_width != 2 ||
      !contains_register_span(callee_m2_spans, gcarry->assigned_register->occupied_register_names)) {
    return fail("expected call-crossing grouped general value to use a legal callee span");
  }
  if (local->assigned_register->contiguous_width != 1 ||
      !contains_register_span(caller_m1_spans, local->assigned_register->occupied_register_names)) {
    return fail("expected non-crossing general local value to stay in the caller pool");
  }
  if (spans_overlap(gcarry->assigned_register->occupied_register_names,
                    local->assigned_register->occupied_register_names)) {
    return fail("expected grouped general callee spans to stay disjoint from caller assignments");
  }

  return 0;
}

}  // namespace

int main() {
  const auto liveness_prepared = prepare_phi_module();
  if (const int rc = check_phi_predecessor_edge_liveness(liveness_prepared); rc != 0) {
    return rc;
  }
  const auto id_authoritative_liveness_prepared = prepare_id_authoritative_phi_module();
  if (const int rc =
          check_id_authoritative_phi_liveness(id_authoritative_liveness_prepared);
      rc != 0) {
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

  const auto select_materialized_join_prepared = prepare_select_materialized_join_module_with_regalloc();
  if (const int rc =
          check_select_materialized_join_move_resolution(select_materialized_join_prepared);
      rc != 0) {
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
  const auto grouped_call_boundary_prepared = prepare_grouped_call_boundary_move_module_with_regalloc();
  if (const int rc = check_grouped_call_boundary_move_resolution(grouped_call_boundary_prepared);
      rc != 0) {
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
  if (const int rc = check_x86_consumer_surface_reads_same_module_variadic_global_byval_authority(
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

  const auto grouped_evicted_spill_prepared = prepare_grouped_evicted_spill_module_with_regalloc();
  if (const int rc = check_grouped_evicted_value_spill_ops(grouped_evicted_spill_prepared); rc != 0) {
    return rc;
  }
  const auto general_grouped_evicted_spill_prepared =
      prepare_general_grouped_evicted_spill_module_with_regalloc();
  if (const int rc =
          check_general_grouped_evicted_value_spill_ops(general_grouped_evicted_spill_prepared);
      rc != 0) {
    return rc;
  }
  const auto float_grouped_evicted_spill_prepared =
      prepare_float_grouped_evicted_spill_module_with_regalloc();
  if (const int rc =
          check_float_grouped_evicted_value_spill_ops(float_grouped_evicted_spill_prepared);
      rc != 0) {
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
  const auto indirect_call_preservation_prepared =
      prepare_indirect_call_preservation_module_with_regalloc();
  if (const int rc =
          check_indirect_call_preservation_publication(indirect_call_preservation_prepared);
      rc != 0) {
    return rc;
  }
  if (const int rc = check_x86_consumer_surface_reads_indirect_call_preservation_authority(
          indirect_call_preservation_prepared);
      rc != 0) {
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
  if (const int rc = check_scalar_grouped_span_candidate_legality(); rc != 0) {
    return rc;
  }
  const auto vector_grouped_prepared = prepare_vector_grouped_linear_scan_module_with_regalloc();
  if (const int rc = check_vector_grouped_linear_scan(vector_grouped_prepared); rc != 0) {
    return rc;
  }
  const auto vector_grouped_cross_call_prepared = prepare_vector_grouped_cross_call_module_with_regalloc();
  if (const int rc = check_vector_grouped_cross_call(vector_grouped_cross_call_prepared); rc != 0) {
    return rc;
  }
  const auto general_grouped_cross_call_prepared = prepare_general_grouped_cross_call_module_with_regalloc();
  if (const int rc = check_general_grouped_cross_call(general_grouped_cross_call_prepared); rc != 0) {
    return rc;
  }
  return 0;
}

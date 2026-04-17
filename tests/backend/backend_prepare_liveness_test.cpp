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
  if (phi->allocation_status != prepare::PreparedAllocationStatus::Unallocated ||
      !phi->spillable || phi->priority == 0 || phi->spill_weight <= 0.0) {
    return fail("expected phi.v regalloc seed to remain unallocated while publishing nonzero allocation priority");
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
  return 0;
}

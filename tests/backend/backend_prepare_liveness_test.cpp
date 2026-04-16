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

}  // namespace

int main() {
  const auto prepared = prepare_phi_module();
  if (const int rc = check_phi_predecessor_edge_liveness(prepared); rc != 0) {
    return rc;
  }
  return 0;
}

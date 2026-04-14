#include "src/backend/bir.hpp"
#include "src/backend/prepare/legalize.hpp"
#include "src/backend/target.hpp"

#include <cstdlib>
#include <iostream>
#include <variant>

namespace {

using c4c::backend::Target;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

const bir::Block* find_block(const bir::Function& function, const char* label) {
  for (const auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

bool is_immediate_i32(const bir::Value& value, std::int64_t expected) {
  return value.kind == bir::Value::Kind::Immediate && value.type == bir::TypeKind::I32 &&
         value.immediate == expected;
}

}  // namespace

int main() {
  bir::Module module;
  bir::Function function;
  function.name = "merge3";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "left",
      .false_label = "split",
  };

  bir::Block left;
  left.label = "left";
  left.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block split;
  split.label = "split";
  split.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(0),
  });
  split.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .true_label = "middle",
      .false_label = "right",
  };

  bir::Block middle;
  middle.label = "middle";
  middle.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{.label = "left", .value = bir::Value::immediate_i32(11)},
          bir::PhiIncoming{.label = "middle", .value = bir::Value::immediate_i32(22)},
          bir::PhiIncoming{.label = "right", .value = bir::Value::immediate_i32(33)},
      },
  });
  join.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(5),
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum")};

  function.blocks = {std::move(entry), std::move(left), std::move(split), std::move(middle),
                     std::move(right), std::move(join)};
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target = Target::Riscv64;

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_regalloc = false;
  prepare::run_legalize(prepared, options);

  if (prepared.module.functions.size() != 1) {
    return fail("expected exactly one function after legalize");
  }
  const auto& legalized = prepared.module.functions.front();
  if (!legalized.local_slots.empty()) {
    return fail("expected reducible phi tree to materialize without local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr) {
    return fail("missing join block after legalize");
  }
  if (join_block->insts.size() != 3) {
    return fail("expected nested selects plus trailing add in join block");
  }

  const auto* nested_select = std::get_if<bir::SelectInst>(&join_block->insts[0]);
  const auto* root_select = std::get_if<bir::SelectInst>(&join_block->insts[1]);
  const auto* add = std::get_if<bir::BinaryInst>(&join_block->insts[2]);
  if (nested_select == nullptr || root_select == nullptr || add == nullptr) {
    return fail("expected select/select/add sequence after phi materialization");
  }
  if (!is_immediate_i32(nested_select->true_value, 22) ||
      !is_immediate_i32(nested_select->false_value, 33)) {
    return fail("expected nested select to cover the split subtree incomings");
  }
  if (!is_immediate_i32(root_select->true_value, 11) ||
      root_select->false_value.kind != bir::Value::Kind::Named ||
      root_select->false_value.name != nested_select->result.name) {
    return fail("expected root select to combine left incoming with nested subtree result");
  }
  if (root_select->result.name != "merge") {
    return fail("expected root select to define the original phi result");
  }
  if (add->lhs.kind != bir::Value::Kind::Named || add->lhs.name != "merge") {
    return fail("expected trailing add to consume the materialized phi result");
  }

  for (const auto& inst : join_block->insts) {
    if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
        std::holds_alternative<bir::StoreLocalInst>(inst)) {
      return fail("unexpected fallback phi lowering remained in join block");
    }
  }

  return 0;
}

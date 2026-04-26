#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/x86/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

c4c::TargetProfile x86_target_profile() {
  return c4c::default_target_profile(c4c::TargetArch::X86_64);
}

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedControlFlowFunction* find_control_flow_function(
    prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto function_name_id =
      prepare::resolve_prepared_function_name_id(prepared.names, function_name);
  if (!function_name_id.has_value()) {
    return nullptr;
  }
  for (auto& function : prepared.control_flow.functions) {
    if (function.function_name == *function_name_id) {
      return &function;
    }
  }
  return nullptr;
}

prepare::PreparedBranchCondition* find_branch_condition(
    prepare::PreparedControlFlowFunction& control_flow,
    c4c::BlockLabelId block_label) {
  for (auto& condition : control_flow.branch_conditions) {
    if (condition.block_label == block_label) {
      return &condition;
    }
  }
  return nullptr;
}

prepare::PreparedBirModule legalize_x86_branch_module() {
  bir::Module module;
  module.target_triple = "x86_64-unknown-linux-gnu";

  const auto entry_id = module.names.block_labels.intern("entry");
  const auto then_id = module.names.block_labels.intern("then.block");
  const auto else_id = module.names.block_labels.intern("else.block");

  bir::Function function;
  function.name = "prepared_label_authority";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = entry_id;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond"),
      .true_label = "then.block",
      .false_label = "else.block",
      .true_label_id = then_id,
      .false_label_id = else_id,
  };

  bir::Block then_block;
  then_block.label = "then.block";
  then_block.label_id = then_id;
  then_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(11)};

  bir::Block else_block;
  else_block.label = "else.block";
  else_block.label_id = else_id;
  else_block.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(22)};

  function.blocks = {std::move(entry), std::move(then_block), std::move(else_block)};
  module.functions.push_back(std::move(function));

  prepare::PreparedBirModule prepared;
  prepared.module = std::move(module);
  prepared.target_profile = x86_target_profile();

  prepare::PrepareOptions options;
  options.run_stack_layout = false;
  options.run_liveness = false;
  options.run_out_of_ssa = false;
  options.run_regalloc = false;
  prepare::BirPreAlloc planner(std::move(prepared), options);
  planner.run_legalize();
  return std::move(planner.prepared());
}

void drift_raw_bir_label_spelling(prepare::PreparedBirModule& prepared) {
  auto& function = prepared.module.functions.front();
  function.blocks[0].label = "raw.entry.drift";
  function.blocks[0].terminator.true_label = "raw.true.drift";
  function.blocks[0].terminator.false_label = "raw.false.drift";
  function.blocks[1].label = "raw.then.drift";
  function.blocks[2].label = "raw.else.drift";
}

bool emit_accepts(prepare::PreparedBirModule prepared) {
  drift_raw_bir_label_spelling(prepared);
  const auto assembly = c4c::backend::x86::module::emit(prepared);
  return assembly.find("x86 backend contract-first stub") != std::string::npos;
}

bool emit_rejects_with_handoff_message(prepare::PreparedBirModule prepared) {
  drift_raw_bir_label_spelling(prepared);
  try {
    static_cast<void>(c4c::backend::x86::module::emit(prepared));
  } catch (const std::invalid_argument& error) {
    return std::string_view(error.what()).find("canonical prepared-module handoff") !=
           std::string_view::npos;
  }
  return false;
}

int check_valid_handoff_consumes_prepared_ids() {
  auto prepared = legalize_x86_branch_module();
  if (!emit_accepts(std::move(prepared))) {
    return fail("x86 handoff rejected valid prepared control-flow label ids");
  }
  return 0;
}

int check_missing_prepared_target_is_rejected() {
  auto prepared = legalize_x86_branch_module();
  auto* control_flow = find_control_flow_function(prepared, "prepared_label_authority");
  if (control_flow == nullptr || control_flow->blocks.empty()) {
    return fail("test fixture did not publish prepared control-flow blocks");
  }
  control_flow->blocks.front().true_label = c4c::kInvalidBlockLabel;

  if (!emit_rejects_with_handoff_message(std::move(prepared))) {
    return fail("x86 handoff accepted a missing prepared true-label id");
  }
  return 0;
}

int check_drifted_prepared_condition_target_is_rejected() {
  auto prepared = legalize_x86_branch_module();
  auto* control_flow = find_control_flow_function(prepared, "prepared_label_authority");
  const auto entry_label = prepare::resolve_prepared_block_label_id(prepared.names, "entry");
  if (control_flow == nullptr || !entry_label.has_value()) {
    return fail("test fixture did not publish the prepared entry block");
  }
  auto* condition = find_branch_condition(*control_flow, *entry_label);
  if (condition == nullptr) {
    return fail("test fixture did not publish the prepared branch condition");
  }

  condition->true_label = prepared.names.block_labels.intern("drifted.prepared.true");
  if (!emit_rejects_with_handoff_message(std::move(prepared))) {
    return fail("x86 handoff accepted a drifted prepared branch-condition true-label id");
  }
  return 0;
}

}  // namespace

int main() {
  if (int result = check_valid_handoff_consumes_prepared_ids(); result != 0) {
    return result;
  }
  if (int result = check_missing_prepared_target_is_rejected(); result != 0) {
    return result;
  }
  if (int result = check_drifted_prepared_condition_target_is_rejected(); result != 0) {
    return result;
  }
  return 0;
}

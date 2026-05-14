#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

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

prepare::PreparedBirModule legalize_block_only_control_flow_module() {
  bir::Module module;
  const c4c::BlockLabelId entry_id = module.names.block_labels.intern("entry");
  const c4c::BlockLabelId exit_id = module.names.block_labels.intern("exit");

  bir::Function function;
  function.name = "block_only_control_flow_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "entry";
  entry.label_id = entry_id;
  entry.terminator = bir::BranchTerminator{
      .target_label = "exit",
      .target_label_id = exit_id,
  };

  bir::Block exit;
  exit.label = "exit";
  exit.label_id = exit_id;
  exit.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(7)};

  function.blocks = {std::move(entry), std::move(exit)};
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
  return std::move(planner.prepared());
}

}  // namespace

int main() {
  const auto prepared = legalize_block_only_control_flow_module();
  const auto* control_flow =
      find_control_flow_function(prepared, "block_only_control_flow_prepare_contract");
  if (control_flow == nullptr) {
    return fail("expected block-only functions to keep prepared control-flow publication");
  }
  if (control_flow->blocks.size() != 2 || !control_flow->branch_conditions.empty() ||
      !control_flow->join_transfers.empty() || !control_flow->parallel_copy_bundles.empty()) {
    return fail("expected block-only control-flow publication without branch or join metadata");
  }

  const auto& entry_block = control_flow->blocks.front();
  const auto& exit_block = control_flow->blocks.back();
  if (prepare::prepared_block_label(prepared.names, entry_block.block_label) != "entry" ||
      entry_block.terminator_kind != bir::TerminatorKind::Branch ||
      prepare::prepared_block_label(prepared.names, entry_block.branch_target_label) != "exit") {
    return fail("expected block-only control-flow publication to preserve branch target metadata");
  }
  if (prepare::prepared_block_label(prepared.names, exit_block.block_label) != "exit" ||
      exit_block.terminator_kind != bir::TerminatorKind::Return) {
    return fail("expected block-only control-flow publication to preserve return block metadata");
  }

  return 0;
}

#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/backend/prealloc/prepared_printer.hpp"
#include "src/target_profile.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <type_traits>
#include <variant>

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

bool expect_contains(const std::string& text, std::string_view needle) {
  if (text.find(needle) != std::string::npos) {
    return true;
  }
  std::cerr << "missing dump fragment: " << needle << "\n";
  std::cerr << "--- dump ---\n" << text << "\n";
  return false;
}

bool contains_invariant(const prepare::PreparedBirModule& module,
                        prepare::PreparedBirInvariant invariant) {
  for (const auto& entry : module.invariants) {
    if (entry == invariant) {
      return true;
    }
  }
  return false;
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

const prepare::PreparedParallelCopyBundle* find_parallel_copy_bundle(
    const prepare::PreparedBirModule& prepared,
    const char* function_name,
    const char* predecessor_label,
    const char* successor_label) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return nullptr;
  }
  return prepare::find_prepared_parallel_copy_bundle(
      prepared.names, *control_flow, predecessor_label, successor_label);
}

const bir::Block* find_block(const bir::Function& function, const char* label) {
  for (const auto& block : function.blocks) {
    if (block.label == label) {
      return &block;
    }
  }
  return nullptr;
}

const bir::Block* find_block_by_id(const bir::Function& function, c4c::BlockLabelId label_id) {
  for (const auto& block : function.blocks) {
    if (block.label_id == label_id) {
      return &block;
    }
  }
  return nullptr;
}

bool is_immediate_i32(const bir::Value& value, std::int64_t expected) {
  return value.kind == bir::Value::Kind::Immediate && value.type == bir::TypeKind::I32 &&
         value.immediate == expected;
}

bool is_named_i32(const bir::Value& value, std::string_view expected_name) {
  return value.kind == bir::Value::Kind::Named && value.type == bir::TypeKind::I32 &&
         value.name == expected_name;
}

bool function_contains_i1(const bir::Function& function) {
  const auto value_is_i1 = [](const bir::Value& value) { return value.type == bir::TypeKind::I1; };
  const auto address_is_i1 = [&](const std::optional<bir::MemoryAddress>& address) {
    return address.has_value() && value_is_i1(address->base_value);
  };

  if (function.return_type == bir::TypeKind::I1) {
    return true;
  }
  for (const auto& param : function.params) {
    if (param.type == bir::TypeKind::I1) {
      return true;
    }
  }
  for (const auto& slot : function.local_slots) {
    if (slot.type == bir::TypeKind::I1) {
      return true;
    }
  }
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      const bool has_i1 = std::visit(
          [&](const auto& lowered) {
            using T = std::decay_t<decltype(lowered)>;
            if constexpr (std::is_same_v<T, bir::BinaryInst>) {
              return lowered.result.type == bir::TypeKind::I1 ||
                     lowered.operand_type == bir::TypeKind::I1 ||
                     value_is_i1(lowered.lhs) || value_is_i1(lowered.rhs);
            } else if constexpr (std::is_same_v<T, bir::SelectInst>) {
              return lowered.result.type == bir::TypeKind::I1 ||
                     lowered.compare_type == bir::TypeKind::I1 ||
                     value_is_i1(lowered.lhs) || value_is_i1(lowered.rhs) ||
                     value_is_i1(lowered.true_value) || value_is_i1(lowered.false_value);
            } else if constexpr (std::is_same_v<T, bir::CastInst>) {
              return lowered.result.type == bir::TypeKind::I1 ||
                     value_is_i1(lowered.operand);
            } else if constexpr (std::is_same_v<T, bir::PhiInst>) {
              if (lowered.result.type == bir::TypeKind::I1) {
                return true;
              }
              for (const auto& incoming : lowered.incomings) {
                if (value_is_i1(incoming.value)) {
                  return true;
                }
              }
              return false;
            } else if constexpr (std::is_same_v<T, bir::CallInst>) {
              if (lowered.return_type == bir::TypeKind::I1) {
                return true;
              }
              if (lowered.result.has_value() && lowered.result->type == bir::TypeKind::I1) {
                return true;
              }
              if (lowered.callee_value.has_value() && value_is_i1(*lowered.callee_value)) {
                return true;
              }
              for (const auto& arg : lowered.args) {
                if (value_is_i1(arg)) {
                  return true;
                }
              }
              for (const auto& arg_type : lowered.arg_types) {
                if (arg_type == bir::TypeKind::I1) {
                  return true;
                }
              }
              for (const auto& arg_abi : lowered.arg_abi) {
                if (arg_abi.type == bir::TypeKind::I1) {
                  return true;
                }
              }
              return lowered.result_abi.has_value() && lowered.result_abi->type == bir::TypeKind::I1;
            } else if constexpr (std::is_same_v<T, bir::LoadLocalInst> ||
                                 std::is_same_v<T, bir::LoadGlobalInst>) {
              return lowered.result.type == bir::TypeKind::I1 || address_is_i1(lowered.address);
            } else if constexpr (std::is_same_v<T, bir::StoreLocalInst> ||
                                 std::is_same_v<T, bir::StoreGlobalInst>) {
              return value_is_i1(lowered.value) || address_is_i1(lowered.address);
            } else {
              return false;
            }
          },
          inst);
      if (has_i1) {
        return true;
      }
    }
    if (block.terminator.value.has_value() && value_is_i1(*block.terminator.value)) {
      return true;
    }
    if (value_is_i1(block.terminator.condition)) {
      return true;
    }
  }
  return false;
}

bool function_contains_phi(const bir::Function& function) {
  for (const auto& block : function.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::PhiInst>(inst)) {
        return true;
      }
    }
  }
  return false;
}

int check_prepare_phi_invariant(const prepare::PreparedBirModule& prepared) {
  if (!contains_invariant(prepared, prepare::PreparedBirInvariant::NoPhiNodes)) {
    return fail("expected out_of_ssa to publish the no-phi-nodes invariant");
  }
  if (prepare::prepared_bir_invariant_name(prepare::PreparedBirInvariant::NoPhiNodes) !=
      "no_phi_nodes") {
    return fail("expected stable name for the no-phi-nodes invariant");
  }
  if (prepared.module.functions.size() != 1) {
    return fail("expected exactly one function when checking the phi legality invariant");
  }
  if (function_contains_phi(prepared.module.functions.front())) {
    return fail("expected out_of_ssa to remove phi nodes from prepared semantic BIR");
  }
  return 0;
}

int check_legalize_preserves_phi(const prepare::PreparedBirModule& prepared) {
  if (contains_invariant(prepared, prepare::PreparedBirInvariant::NoPhiNodes)) {
    return fail("expected legalize-only preparation to avoid publishing the no-phi-nodes invariant");
  }
  if (prepared.module.functions.size() != 1) {
    return fail("expected exactly one function when checking legalize-only phi preservation");
  }
  if (!function_contains_phi(prepared.module.functions.front())) {
    return fail("expected legalize-only preparation to preserve phi nodes in semantic BIR");
  }
  return 0;
}

int check_prepare_i1_invariant(const prepare::PreparedBirModule& prepared) {
  if (!contains_invariant(prepared, prepare::PreparedBirInvariant::NoTargetFacingI1)) {
    return fail("expected prepare legalize to publish the no-target-facing-i1 invariant");
  }
  if (prepare::prepared_bir_invariant_name(prepare::PreparedBirInvariant::NoTargetFacingI1) !=
      "no_target_facing_i1") {
    return fail("expected stable name for the no-target-facing-i1 invariant");
  }
  for (const auto& function : prepared.module.functions) {
    if (function_contains_i1(function)) {
      return fail("expected legalize to remove target-facing i1 values from prepared semantic BIR");
    }
  }
  if (prepared.module.functions.empty()) {
    return fail("expected at least one function when checking the i1 legality invariant");
  }
  return 0;
}

int check_prepared_control_flow_contract(const prepare::PreparedBirModule& prepared,
                                         const char* function_name,
                                         std::size_t expected_branch_conditions,
                                         prepare::PreparedJoinTransferKind expected_join_kind) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish prepared control-flow metadata for the function");
  }
  if (control_flow->branch_conditions.size() != expected_branch_conditions) {
    return fail("unexpected number of prepared branch-condition records");
  }
  if (control_flow->join_transfers.size() != 1) {
    return fail("expected exactly one prepared join-transfer record");
  }

  const auto& entry_condition = control_flow->branch_conditions.front();
  if (prepare::prepared_block_label(prepared.names, entry_condition.block_label) != "entry" ||
      !entry_condition.predicate.has_value() ||
      !entry_condition.compare_type.has_value() || !entry_condition.lhs.has_value() ||
      !entry_condition.rhs.has_value()) {
    return fail("expected entry branch-condition metadata to publish compare semantics");
  }
  if (*entry_condition.predicate != bir::BinaryOpcode::Eq ||
      *entry_condition.compare_type != bir::TypeKind::I32 ||
      !is_immediate_i32(*entry_condition.lhs, 0) || !is_immediate_i32(*entry_condition.rhs, 0) ||
      prepare::prepared_block_label(prepared.names, entry_condition.true_label) != "left" ||
      prepare::prepared_block_label(prepared.names, entry_condition.false_label) != "split") {
    return fail("expected entry branch-condition metadata to match the legalized compare branch");
  }
  if (entry_condition.condition_value.kind != bir::Value::Kind::Named ||
      entry_condition.condition_value.name != "cond0" ||
      entry_condition.condition_value.type != bir::TypeKind::I32) {
    return fail("expected entry branch-condition metadata to track the legalized condition value");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) != "join" ||
      join_transfer.result.name != "merge") {
    return fail("expected join-transfer metadata to name the legalized join result");
  }
  if (join_transfer.kind != expected_join_kind ||
      prepare::prepared_join_transfer_kind_name(join_transfer.kind) ==
          std::string_view("unknown")) {
    return fail("expected a stable prepared join-transfer kind for the join metadata");
  }
  if (join_transfer.incomings.size() != 3) {
    return fail("expected join-transfer metadata to preserve every predecessor incoming");
  }
  if (!is_immediate_i32(join_transfer.incomings[0].value, 11) ||
      !is_immediate_i32(join_transfer.incomings[1].value, 22) ||
      !is_immediate_i32(join_transfer.incomings[2].value, 33)) {
    return fail("expected join-transfer metadata to preserve the original incoming values");
  }
  if (expected_join_kind == prepare::PreparedJoinTransferKind::SelectMaterialization &&
      join_transfer.storage_name.has_value()) {
    return fail("expected select-materialized join metadata to avoid fallback slot ownership");
  }

  return 0;
}

int check_two_way_branch_join_control_flow_contract(const prepare::PreparedBirModule& prepared,
                                                    const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish prepared control-flow metadata for the joined branch lane");
  }
  if (control_flow->branch_conditions.size() != 1 || control_flow->join_transfers.size() != 1 ||
      control_flow->parallel_copy_bundles.size() != 2) {
    return fail(
        "expected the joined branch lane to publish one branch-condition, one join-transfer, and two edge bundles");
  }

  const auto& branch_condition = control_flow->branch_conditions.front();
  if (prepare::prepared_block_label(prepared.names, branch_condition.block_label) != "entry" ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() || !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value() ||
      *branch_condition.predicate != bir::BinaryOpcode::Eq ||
      *branch_condition.compare_type != bir::TypeKind::I32 ||
      !is_named_i32(*branch_condition.lhs, "p.x") ||
      !is_immediate_i32(*branch_condition.rhs, 0) ||
      prepare::prepared_block_label(prepared.names, branch_condition.true_label) != "is_zero" ||
      prepare::prepared_block_label(prepared.names, branch_condition.false_label) !=
          "is_nonzero" ||
      !is_named_i32(branch_condition.condition_value, "cond0")) {
    return fail("expected joined branch metadata to preserve the compare-against-zero branch contract");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.kind != prepare::PreparedJoinTransferKind::SelectMaterialization ||
      prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) != "join" ||
      !is_named_i32(join_transfer.result, "merge") ||
      join_transfer.incomings.size() != 2 || join_transfer.storage_name.has_value()) {
    return fail("expected joined branch metadata to publish a select-materialized join contract");
  }
  if (!join_transfer.source_branch_block_label.has_value() ||
      !join_transfer.source_true_transfer_index.has_value() ||
      !join_transfer.source_false_transfer_index.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value() ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_branch_block_label) !=
          "entry" ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_true_incoming_label) !=
          "is_zero" ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_false_incoming_label) !=
          "is_nonzero") {
    return fail("expected joined branch metadata to publish authoritative branch-owned join mapping");
  }
  if (join_transfer.incomings[0].label != "is_zero" ||
      !is_named_i32(join_transfer.incomings[0].value, "zero.adjusted") ||
      join_transfer.incomings[1].label != "is_nonzero" ||
      !is_named_i32(join_transfer.incomings[1].value, "nonzero.adjusted")) {
    return fail("expected joined branch metadata to preserve the predecessor-to-join incoming mapping");
  }
  const auto authoritative_true_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, branch_condition.true_label, join_transfer.join_block_label);
  const auto authoritative_false_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, branch_condition.false_label, join_transfer.join_block_label);
  if (!authoritative_true_lane.has_value() || !authoritative_false_lane.has_value() ||
      *authoritative_true_lane != *join_transfer.source_true_incoming_label ||
      *authoritative_false_lane != *join_transfer.source_false_incoming_label) {
    return fail("expected prepared control-flow blocks to reproduce the authoritative joined-branch incoming ownership");
  }

  const auto* zero_bundle =
      find_parallel_copy_bundle(prepared, function_name, "is_zero", "join");
  const auto* nonzero_bundle =
      find_parallel_copy_bundle(prepared, function_name, "is_nonzero", "join");
  if (zero_bundle == nullptr || nonzero_bundle == nullptr) {
    return fail("expected joined branch metadata to publish one bundle per predecessor edge");
  }
  if (zero_bundle->execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      nonzero_bundle->execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail(
        "expected select-materialized join bundles to stay predecessor-terminator-owned instead of publishing successor-entry ownership");
  }
  if (zero_bundle->moves.size() != 1 || zero_bundle->steps.size() != 1 ||
      nonzero_bundle->moves.size() != 1 || nonzero_bundle->steps.size() != 1) {
    return fail("expected joined branch bundles to preserve one move and one step per predecessor edge");
  }
  if (!is_named_i32(zero_bundle->moves.front().source_value, "zero.adjusted") ||
      !is_named_i32(zero_bundle->moves.front().destination_value, "merge") ||
      !is_named_i32(nonzero_bundle->moves.front().source_value, "nonzero.adjusted") ||
      !is_named_i32(nonzero_bundle->moves.front().destination_value, "merge")) {
    return fail("expected joined branch bundles to preserve the authoritative source-to-join mapping");
  }

  return 0;
}

int check_short_circuit_or_control_flow_contract(const prepare::PreparedBirModule& prepared,
                                                 const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish prepared control-flow metadata for the short-circuit lane");
  }
  if (control_flow->branch_conditions.size() != 3 || control_flow->join_transfers.size() != 1) {
    return fail("expected the short-circuit lane to publish three branch-condition records and one join-transfer");
  }

  const prepare::PreparedBranchCondition* entry_condition = nullptr;
  const prepare::PreparedBranchCondition* rhs_condition = nullptr;
  const prepare::PreparedBranchCondition* join_condition = nullptr;
  for (const auto& condition : control_flow->branch_conditions) {
    if (prepare::prepared_block_label(prepared.names, condition.block_label) == "entry") {
      entry_condition = &condition;
    } else if (prepare::prepared_block_label(prepared.names, condition.block_label) ==
               "logic.rhs.7") {
      rhs_condition = &condition;
    } else if (prepare::prepared_block_label(prepared.names, condition.block_label) ==
               "logic.end.10") {
      join_condition = &condition;
    }
  }
  if (entry_condition == nullptr || rhs_condition == nullptr || join_condition == nullptr) {
    return fail("expected the short-circuit lane to publish entry, rhs, and join branch-condition metadata");
  }

  if (!entry_condition->predicate.has_value() || !entry_condition->compare_type.has_value() ||
      !entry_condition->lhs.has_value() || !entry_condition->rhs.has_value() ||
      *entry_condition->predicate != bir::BinaryOpcode::Ne ||
      *entry_condition->compare_type != bir::TypeKind::I32 ||
      !is_named_i32(*entry_condition->lhs, "%t3") || !is_immediate_i32(*entry_condition->rhs, 3) ||
      prepare::prepared_block_label(prepared.names, entry_condition->true_label) !=
          "logic.skip.8" ||
      prepare::prepared_block_label(prepared.names, entry_condition->false_label) !=
          "logic.rhs.7" ||
      !is_named_i32(entry_condition->condition_value, "%t4")) {
    return fail("expected the short-circuit entry metadata to preserve the authoritative guard compare");
  }

  if (!rhs_condition->predicate.has_value() || !rhs_condition->compare_type.has_value() ||
      !rhs_condition->lhs.has_value() || !rhs_condition->rhs.has_value() ||
      *rhs_condition->predicate != bir::BinaryOpcode::Ne ||
      *rhs_condition->compare_type != bir::TypeKind::I32 ||
      !is_named_i32(*rhs_condition->lhs, "%t12") || !is_immediate_i32(*rhs_condition->rhs, 3) ||
      prepare::prepared_block_label(prepared.names, rhs_condition->true_label) != "block_1" ||
      prepare::prepared_block_label(prepared.names, rhs_condition->false_label) != "block_2" ||
      !is_named_i32(rhs_condition->condition_value, "%t13")) {
    return fail("expected the short-circuit rhs metadata to preserve the authoritative continuation compare");
  }

  if (!join_condition->predicate.has_value() || !join_condition->compare_type.has_value() ||
      !join_condition->lhs.has_value() || !join_condition->rhs.has_value() ||
      *join_condition->predicate != bir::BinaryOpcode::Ne ||
      *join_condition->compare_type != bir::TypeKind::I32 ||
      !is_named_i32(*join_condition->lhs, "%t17") || !is_immediate_i32(*join_condition->rhs, 0) ||
      prepare::prepared_block_label(prepared.names, join_condition->true_label) != "block_1" ||
      prepare::prepared_block_label(prepared.names, join_condition->false_label) != "block_2" ||
      !is_named_i32(join_condition->condition_value, "%t18")) {
    return fail("expected the short-circuit join metadata to preserve the joined-boolean branch contract");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.kind != prepare::PreparedJoinTransferKind::SelectMaterialization ||
      prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) !=
          "logic.end.10" ||
      !is_named_i32(join_transfer.result, "%t17") ||
      join_transfer.incomings.size() != 2 || join_transfer.storage_name.has_value() ||
      !join_transfer.source_branch_block_label.has_value() ||
      !join_transfer.source_true_incoming_label.has_value() ||
      !join_transfer.source_false_incoming_label.has_value()) {
    return fail("expected the short-circuit join metadata to publish a select-materialized join contract");
  }
  if (prepare::prepared_block_label(prepared.names, *join_transfer.source_branch_block_label) !=
          "entry" ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_true_incoming_label) !=
          "logic.skip.8" ||
      prepare::prepared_block_label(prepared.names, *join_transfer.source_false_incoming_label) !=
          "logic.rhs.end.9") {
    return fail("expected the short-circuit join metadata to map compare truth to the authoritative join incomings");
  }
  const auto authoritative_true_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, entry_condition->true_label, join_transfer.join_block_label);
  const auto authoritative_false_lane = prepare::find_prepared_linear_join_predecessor(
      *control_flow, entry_condition->false_label, join_transfer.join_block_label);
  if (!authoritative_true_lane.has_value() || !authoritative_false_lane.has_value() ||
      *authoritative_true_lane != *join_transfer.source_true_incoming_label ||
      *authoritative_false_lane != *join_transfer.source_false_incoming_label) {
    return fail("expected prepared control-flow blocks to reproduce short-circuit authoritative join ownership");
  }
  bool saw_rhs_incoming = false;
  bool saw_short_circuit_incoming = false;
  for (const auto& incoming : join_transfer.incomings) {
    saw_rhs_incoming =
        saw_rhs_incoming ||
        (incoming.label == "logic.rhs.end.9" && is_named_i32(incoming.value, "%t13"));
    saw_short_circuit_incoming =
        saw_short_circuit_incoming ||
        (incoming.label == "logic.skip.8" && is_immediate_i32(incoming.value, 1));
  }
  if (!saw_rhs_incoming || !saw_short_circuit_incoming) {
    return fail("expected the short-circuit join metadata to preserve rhs and short-circuit incoming ownership");
  }

  return 0;
}

int check_loop_countdown_control_flow_contract(const prepare::PreparedBirModule& prepared,
                                               const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish prepared control-flow metadata for the loop lane");
  }
  if (control_flow->branch_conditions.size() != 1 || control_flow->join_transfers.size() != 1) {
    return fail("expected the loop lane to publish one branch-condition and one join-transfer record");
  }

  const auto& branch_condition = control_flow->branch_conditions.front();
  if (prepare::prepared_block_label(prepared.names, branch_condition.block_label) != "loop" ||
      !branch_condition.predicate.has_value() ||
      !branch_condition.compare_type.has_value() || !branch_condition.lhs.has_value() ||
      !branch_condition.rhs.has_value() ||
      *branch_condition.predicate != bir::BinaryOpcode::Ne ||
      *branch_condition.compare_type != bir::TypeKind::I32 ||
      !is_named_i32(*branch_condition.lhs, "counter") ||
      !is_immediate_i32(*branch_condition.rhs, 0) ||
      prepare::prepared_block_label(prepared.names, branch_condition.true_label) != "body" ||
      prepare::prepared_block_label(prepared.names, branch_condition.false_label) != "exit" ||
      !is_named_i32(branch_condition.condition_value, "cmp0")) {
    return fail("expected the loop branch metadata to preserve the countdown header compare");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.kind != prepare::PreparedJoinTransferKind::LoopCarry ||
      prepare::prepared_join_transfer_kind_name(join_transfer.kind) != "loop_carry" ||
      prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) != "loop" ||
      !is_named_i32(join_transfer.result, "counter") || !join_transfer.storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *join_transfer.storage_name) != "counter.phi" ||
      join_transfer.incomings.size() != 2) {
    return fail("expected the loop join metadata to publish a loop-carry contract");
  }
  if (join_transfer.incomings[0].label != "entry" ||
      !is_immediate_i32(join_transfer.incomings[0].value, 3) ||
      join_transfer.incomings[1].label != "body" ||
      !is_named_i32(join_transfer.incomings[1].value, "next")) {
    return fail("expected the loop join metadata to preserve entry and backedge incoming ownership");
  }

  return 0;
}

int check_id_authoritative_loop_countdown_contract(
    const prepare::PreparedBirModule& prepared,
    const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected prepared control-flow metadata for id-authoritative loop lane");
  }
  if (control_flow->join_transfers.size() != 1 || control_flow->parallel_copy_bundles.size() != 2) {
    return fail("expected id-authoritative loop lane to publish one join transfer and two bundles");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.kind != prepare::PreparedJoinTransferKind::LoopCarry ||
      prepare::prepared_block_label(prepared.names, join_transfer.join_block_label) != "loop" ||
      join_transfer.edge_transfers.size() != 2) {
    return fail("expected id-authoritative loop join transfer to use canonical loop ids");
  }
  if (prepare::prepared_block_label(prepared.names,
                                    join_transfer.edge_transfers[0].predecessor_label) !=
          "entry" ||
      prepare::prepared_block_label(prepared.names,
                                    join_transfer.edge_transfers[1].predecessor_label) !=
          "body") {
    return fail("expected id-authoritative phi incomings to publish canonical predecessor ids");
  }

  const auto* entry_bundle = find_parallel_copy_bundle(prepared, function_name, "entry", "loop");
  const auto* body_bundle = find_parallel_copy_bundle(prepared, function_name, "body", "loop");
  if (entry_bundle == nullptr || body_bundle == nullptr) {
    return fail("expected id-authoritative loop bundles to be indexed by canonical ids");
  }

  if (prepared.module.functions.size() != 1) {
    return fail("expected one id-authoritative loop function");
  }
  const auto& function = prepared.module.functions.front();
  const c4c::BlockLabelId entry_id = prepared.module.names.block_labels.find("entry");
  const c4c::BlockLabelId body_id = prepared.module.names.block_labels.find("body");
  const auto* entry_block = find_block_by_id(function, entry_id);
  const auto* body_block = find_block_by_id(function, body_id);
  if (entry_block == nullptr || body_block == nullptr) {
    return fail("expected stale-spelled loop blocks to remain findable by canonical ids");
  }
  const auto entry_store = std::find_if(entry_block->insts.begin(),
                                        entry_block->insts.end(),
                                        [](const bir::Inst& inst) {
                                          return std::holds_alternative<bir::StoreLocalInst>(inst);
                                        });
  const auto body_store = std::find_if(body_block->insts.begin(),
                                       body_block->insts.end(),
                                       [](const bir::Inst& inst) {
                                         return std::holds_alternative<bir::StoreLocalInst>(inst);
                                       });
  if (entry_store == entry_block->insts.end() || body_store == body_block->insts.end()) {
    return fail("expected id-authoritative phi lowering to place stores in canonical predecessor blocks");
  }

  return 0;
}

int check_parallel_copy_cycle_contract(const prepare::PreparedBirModule& prepared,
                                       const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish prepared control-flow metadata for the parallel-copy loop lane");
  }
  if (control_flow->join_transfers.size() != 2) {
    return fail("expected the parallel-copy loop lane to publish one join-transfer per phi result");
  }
  if (control_flow->parallel_copy_bundles.size() != 2) {
    return fail("expected the parallel-copy loop lane to publish one bundle per predecessor edge");
  }

  const auto* entry_bundle = find_parallel_copy_bundle(prepared, function_name, "entry", "loop");
  const auto* body_bundle = find_parallel_copy_bundle(prepared, function_name, "body", "loop");
  if (entry_bundle == nullptr || body_bundle == nullptr) {
    return fail("expected the parallel-copy loop lane to publish entry/body bundle ownership");
  }
  if (entry_bundle->execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator ||
      body_bundle->execution_site !=
          prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail("expected loop bundles to publish predecessor-terminator execution ownership");
  }
  const auto entry_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*entry_bundle);
  const auto body_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*body_bundle);
  if (!entry_execution_block.has_value() || !body_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *entry_execution_block) != "entry" ||
      prepare::prepared_block_label(prepared.names, *body_execution_block) != "body") {
    return fail("expected loop bundles to publish direct execution-block authority");
  }
  if (entry_bundle->has_cycle || entry_bundle->moves.size() != 2 ||
      entry_bundle->steps.size() != 2) {
    return fail("expected the entry-to-loop bundle to stay acyclic while preserving both phi moves");
  }
  if (entry_bundle->steps[0].kind != prepare::PreparedParallelCopyStepKind::Move ||
      entry_bundle->steps[1].kind != prepare::PreparedParallelCopyStepKind::Move ||
      entry_bundle->steps[0].uses_cycle_temp_source ||
      entry_bundle->steps[1].uses_cycle_temp_source) {
    return fail("expected the entry-to-loop bundle to publish direct move-only resolution");
  }

  if (!body_bundle->has_cycle || body_bundle->moves.size() != 2 ||
      body_bundle->steps.size() != 3) {
    return fail("expected the backedge bundle to publish a cycle-breaking resolution plan");
  }
  if (body_bundle->steps[0].kind !=
          prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp ||
      body_bundle->steps[1].kind != prepare::PreparedParallelCopyStepKind::Move ||
      body_bundle->steps[2].kind != prepare::PreparedParallelCopyStepKind::Move) {
    return fail("expected the backedge bundle to begin with an explicit save-to-temp step");
  }
  if (body_bundle->steps[0].move_index != 0 || body_bundle->steps[1].move_index != 0 ||
      body_bundle->steps[1].uses_cycle_temp_source || body_bundle->steps[2].move_index != 1 ||
      !body_bundle->steps[2].uses_cycle_temp_source) {
    return fail("expected the backedge bundle to rotate the cycle through the published temporary source");
  }
  const auto* save_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 0);
  const auto* direct_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 1);
  const auto* temp_step_move = prepare::find_prepared_parallel_copy_move_for_step(*body_bundle, 2);
  if (save_step_move == nullptr || direct_step_move == nullptr || temp_step_move == nullptr) {
    return fail("expected the backedge bundle to publish a direct step-to-carrier lookup seam");
  }
  if (save_step_move->carrier_kind != prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      !save_step_move->storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *save_step_move->storage_name) != "a.phi" ||
      direct_step_move->carrier_kind != prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      !direct_step_move->storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *direct_step_move->storage_name) != "a.phi" ||
      temp_step_move->carrier_kind != prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      !temp_step_move->storage_name.has_value() ||
      prepare::prepared_slot_name(prepared.names, *temp_step_move->storage_name) != "b.phi") {
    return fail("expected the loop backedge steps to publish edge-store carrier authority directly");
  }
  if (!is_named_i32(save_step_move->source_value, "b") ||
      !is_named_i32(save_step_move->destination_value, "a") ||
      !is_named_i32(direct_step_move->source_value, "b") ||
      !is_named_i32(direct_step_move->destination_value, "a") ||
      !is_named_i32(temp_step_move->source_value, "a") ||
      !is_named_i32(temp_step_move->destination_value, "b")) {
    return fail("expected the backedge steps to preserve the phi swap sources and destinations");
  }

  return 0;
}

int check_critical_edge_parallel_copy_contract(const prepare::PreparedBirModule& prepared,
                                               const char* function_name) {
  const auto* control_flow = find_control_flow_function(prepared, function_name);
  if (control_flow == nullptr) {
    return fail("expected legalize to publish control-flow metadata for the critical-edge bundle lane");
  }
  if (control_flow->join_transfers.size() != 1 || control_flow->parallel_copy_bundles.size() != 2) {
    return fail("expected the critical-edge lane to publish one join transfer and two edge bundles");
  }

  const auto* critical_bundle = find_parallel_copy_bundle(prepared, function_name, "left", "join");
  const auto* linear_bundle = find_parallel_copy_bundle(prepared, function_name, "right", "join");
  if (critical_bundle == nullptr || linear_bundle == nullptr) {
    return fail("expected the critical-edge lane to publish left/right bundle ownership");
  }
  if (critical_bundle->execution_site != prepare::PreparedParallelCopyExecutionSite::CriticalEdge) {
    return fail("expected the left-to-join bundle to publish critical-edge execution ownership");
  }
  if (linear_bundle->execution_site !=
      prepare::PreparedParallelCopyExecutionSite::PredecessorTerminator) {
    return fail("expected the right-to-join bundle to stay predecessor-terminator executable");
  }
  const auto critical_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*critical_bundle);
  const auto linear_execution_block =
      prepare::published_prepared_parallel_copy_execution_block_label(*linear_bundle);
  const auto critical_execution_block_index =
      prepare::published_prepared_parallel_copy_execution_block_index(
          prepared.names, prepared.module.functions.front(), *critical_bundle);
  const auto linear_execution_block_index =
      prepare::published_prepared_parallel_copy_execution_block_index(
          prepared.names, prepared.module.functions.front(), *linear_bundle);
  if (critical_execution_block.has_value() || !linear_execution_block.has_value() ||
      prepare::prepared_block_label(prepared.names, *linear_execution_block) != "right") {
    return fail("expected the critical-edge lane to publish direct execution-block authority");
  }
  if (critical_execution_block_index.has_value() || !linear_execution_block_index.has_value() ||
      *linear_execution_block_index != 2) {
    return fail("expected execution-block index lookup to stay empty for critical edges and stable for linear edges");
  }
  if (critical_bundle->moves.size() != 1 || critical_bundle->steps.size() != 1 ||
      linear_bundle->moves.size() != 1 || linear_bundle->steps.size() != 1) {
    return fail("expected the critical-edge lane to preserve one move and one step per incoming edge");
  }
  if (!is_immediate_i32(critical_bundle->moves.front().source_value, 11) ||
      !is_named_i32(critical_bundle->moves.front().destination_value, "merge") ||
      !is_immediate_i32(linear_bundle->moves.front().source_value, 22) ||
      !is_named_i32(linear_bundle->moves.front().destination_value, "merge")) {
    return fail("expected the critical-edge lane to preserve the per-edge copy sources and destination");
  }

  return 0;
}

prepare::PreparedBirModule legalize_call_abi_module() {
  bir::Module module;
  module.globals.push_back(bir::Global{
      .name = "gflag",
      .type = bir::TypeKind::I1,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer = bir::Value::immediate_i1(true),
  });

  bir::Function callee;
  callee.name = "callee";
  callee.return_type = bir::TypeKind::I1;
  callee.return_size_bytes = 1;
  callee.return_align_bytes = 1;
  callee.params.push_back(bir::Param{
      .type = bir::TypeKind::I1,
      .name = "flag",
      .size_bytes = 1,
      .align_bytes = 1,
  });
  callee.is_declaration = true;

  bir::Function caller;
  caller.name = "caller";
  caller.return_type = bir::TypeKind::I1;
  caller.return_size_bytes = 1;
  caller.return_align_bytes = 1;
  caller.params.push_back(bir::Param{
      .type = bir::TypeKind::I1,
      .name = "flag",
      .size_bytes = 1,
      .align_bytes = 1,
  });
  caller.local_slots.push_back(bir::LocalSlot{
      .name = "flag.slot",
      .type = bir::TypeKind::I1,
      .size_bytes = 1,
      .align_bytes = 1,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::CallInst{
      .result = bir::Value::named(bir::TypeKind::I1, "call_result"),
      .callee = "callee",
      .args = {bir::Value::named(bir::TypeKind::I1, "flag")},
      .arg_types = {bir::TypeKind::I1},
      .arg_abi = {bir::CallArgAbiInfo{
          .type = bir::TypeKind::I1,
          .size_bytes = 1,
          .align_bytes = 1,
          .primary_class = bir::AbiValueClass::Integer,
          .passed_in_register = true,
      }},
      .return_type_name = "i1",
      .return_type = bir::TypeKind::I1,
      .result_abi = bir::CallResultAbiInfo{
          .type = bir::TypeKind::I1,
          .primary_class = bir::AbiValueClass::Integer,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I1, "call_result"),
  };
  caller.blocks.push_back(std::move(entry));

  module.functions.push_back(std::move(callee));
  module.functions.push_back(std::move(caller));

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

prepare::PreparedBirModule legalize_two_way_branch_join_module() {
  bir::Module module;

  bir::Function function;
  function.name = "branch_join_prepare_contract";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks = {
      std::move(entry),
      std::move(is_zero),
      std::move(is_nonzero),
      std::move(join),
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

prepare::PreparedBirModule legalize_two_way_branch_join_module_legalize_only() {
  bir::Module module;

  bir::Function function;
  function.name = "branch_join_prepare_contract_legalize_only";
  function.return_type = bir::TypeKind::I32;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I32,
      .name = "p.x",
      .size_bytes = 4,
      .align_bytes = 4,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond0"),
      .true_label = "is_zero",
      .false_label = "is_nonzero",
  };

  bir::Block is_zero;
  is_zero.label = "is_zero";
  is_zero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(5),
  });
  is_zero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block is_nonzero;
  is_nonzero.label = "is_nonzero";
  is_nonzero.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "p.x"),
      .rhs = bir::Value::immediate_i32(1),
  });
  is_nonzero.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{
              .label = "is_zero",
              .value = bir::Value::named(bir::TypeKind::I32, "zero.adjusted"),
          },
          bir::PhiIncoming{
              .label = "is_nonzero",
              .value = bir::Value::named(bir::TypeKind::I32, "nonzero.adjusted"),
          },
      },
  });
  join.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I32, "merge"),
  };

  function.blocks = {
      std::move(entry),
      std::move(is_zero),
      std::move(is_nonzero),
      std::move(join),
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
  return std::move(planner.prepared());
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

prepare::PreparedBirModule legalize_id_authoritative_loop_countdown_module() {
  bir::Module module;

  const c4c::BlockLabelId entry_id = module.names.block_labels.intern("entry");
  const c4c::BlockLabelId loop_id = module.names.block_labels.intern("loop");
  const c4c::BlockLabelId body_id = module.names.block_labels.intern("body");
  const c4c::BlockLabelId exit_id = module.names.block_labels.intern("exit");

  bir::Function function;
  function.name = "id_authoritative_loop_countdown_prepare_contract";
  function.return_type = bir::TypeKind::I32;

  bir::Block entry;
  entry.label = "stale.entry";
  entry.label_id = entry_id;
  entry.terminator = bir::BranchTerminator{
      .target_label = "stale.loop",
      .target_label_id = loop_id,
  };

  bir::Block loop;
  loop.label = "stale.loop";
  loop.label_id = loop_id;
  loop.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "counter"),
      .incomings = {
          bir::PhiIncoming{
              .label = "stale.entry",
              .value = bir::Value::immediate_i32(3),
              .label_id = entry_id,
          },
          bir::PhiIncoming{
              .label = "stale.body",
              .value = bir::Value::named(bir::TypeKind::I32, "next"),
              .label_id = body_id,
          },
      },
  });
  loop.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ne,
      .result = bir::Value::named(bir::TypeKind::I32, "cmp0"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(0),
  });
  loop.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cmp0"),
      .true_label = "stale.body",
      .false_label = "stale.exit",
      .true_label_id = body_id,
      .false_label_id = exit_id,
  };

  bir::Block body;
  body.label = "stale.body";
  body.label_id = body_id;
  body.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sub,
      .result = bir::Value::named(bir::TypeKind::I32, "next"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "counter"),
      .rhs = bir::Value::immediate_i32(1),
  });
  body.terminator = bir::BranchTerminator{
      .target_label = "stale.loop",
      .target_label_id = loop_id,
  };

  bir::Block exit;
  exit.label = "stale.exit";
  exit.label_id = exit_id;
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

prepare::PreparedBirModule legalize_critical_edge_parallel_copy_module() {
  bir::Module module;
  bir::Function function;
  function.name = "critical_edge_parallel_copy_prepare_contract";
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
      .false_label = "right",
  };

  bir::Block left;
  left.label = "left";
  left.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(1),
      .rhs = bir::Value::immediate_i32(1),
  });
  left.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond1"),
      .true_label = "join",
      .false_label = "exit",
  };

  bir::Block right;
  right.label = "right";
  right.terminator = bir::BranchTerminator{.target_label = "join"};

  bir::Block exit;
  exit.label = "exit";
  exit.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(-1)};

  bir::Block join;
  join.label = "join";
  join.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32, "merge"),
      .incomings = {
          bir::PhiIncoming{.label = "left", .value = bir::Value::immediate_i32(11)},
          bir::PhiIncoming{.label = "right", .value = bir::Value::immediate_i32(22)},
      },
  });
  join.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};

  function.blocks = {
      std::move(entry),
      std::move(left),
      std::move(right),
      std::move(exit),
      std::move(join),
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

prepare::PreparedBirModule legalize_memory_access_module() {
  bir::Module module;
  module.globals.push_back(bir::Global{
      .name = "gflag",
      .type = bir::TypeKind::I1,
      .size_bytes = 1,
      .align_bytes = 1,
      .initializer = bir::Value::immediate_i1(true),
  });

  bir::Function function;
  function.name = "memory_access";
  function.return_type = bir::TypeKind::I1;
  function.return_size_bytes = 1;
  function.return_align_bytes = 1;
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::I1,
      .name = "flag",
      .size_bytes = 1,
      .align_bytes = 1,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "local.addr",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.params.push_back(bir::Param{
      .type = bir::TypeKind::Ptr,
      .name = "global.addr",
      .size_bytes = 8,
      .align_bytes = 8,
  });
  function.local_slots.push_back(bir::LocalSlot{
      .name = "flag.slot",
      .type = bir::TypeKind::I1,
      .size_bytes = 1,
      .align_bytes = 1,
  });

  bir::Block entry;
  entry.label = "entry";
  entry.insts.push_back(bir::StoreLocalInst{
      .slot_name = "flag.slot",
      .value = bir::Value::named(bir::TypeKind::I1, "flag"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "local.addr"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::LoadLocalInst{
      .result = bir::Value::named(bir::TypeKind::I1, "local.load"),
      .slot_name = "flag.slot",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "local.addr"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::StoreGlobalInst{
      .global_name = "gflag",
      .value = bir::Value::named(bir::TypeKind::I1, "flag"),
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "global.addr"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.insts.push_back(bir::LoadGlobalInst{
      .result = bir::Value::named(bir::TypeKind::I1, "global.load"),
      .global_name = "gflag",
      .align_bytes = 1,
      .address = bir::MemoryAddress{
          .base_kind = bir::MemoryAddress::BaseKind::PointerValue,
          .base_value = bir::Value::named(bir::TypeKind::Ptr, "global.addr"),
          .size_bytes = 1,
          .align_bytes = 1,
      },
  });
  entry.terminator = bir::ReturnTerminator{
      .value = bir::Value::named(bir::TypeKind::I1, "global.load"),
  };
  function.blocks.push_back(std::move(entry));

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

prepare::PreparedBirModule legalize_merge3_module(bool add_trailing_use) {
  bir::Module module;
  bir::Function function;
  function.name = add_trailing_use ? "merge3_add" : "merge3_return";
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
  if (add_trailing_use) {
    join.insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32, "sum"),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
        .rhs = bir::Value::immediate_i32(5),
    });
    join.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum")};
  } else {
    join.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "merge")};
  }

  function.blocks = {std::move(entry), std::move(left), std::move(split), std::move(middle),
                     std::move(right), std::move(join)};
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

prepare::PreparedBirModule legalize_merge3_successor_use_module() {
  bir::Module module;
  bir::Function function;
  function.name = "merge3_successor_use";
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
  join.terminator = bir::BranchTerminator{.target_label = "after"};

  bir::Block after;
  after.label = "after";
  after.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(5),
  });
  after.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum")};

  function.blocks = {std::move(entry), std::move(left), std::move(split), std::move(middle),
                     std::move(right), std::move(join), std::move(after)};
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

prepare::PreparedBirModule legalize_merge3_forwarded_successor_use_module() {
  bir::Module module;
  bir::Function function;
  function.name = "merge3_forwarded_successor_use";
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
  join.terminator = bir::BranchTerminator{.target_label = "forward0"};

  bir::Block forward0;
  forward0.label = "forward0";
  forward0.terminator = bir::BranchTerminator{.target_label = "forward1"};

  bir::Block forward1;
  forward1.label = "forward1";
  forward1.terminator = bir::BranchTerminator{.target_label = "after"};

  bir::Block after;
  after.label = "after";
  after.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(5),
  });
  after.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum")};

  function.blocks = {std::move(entry), std::move(left),    std::move(split),
                     std::move(middle), std::move(right),   std::move(join),
                     std::move(forward0), std::move(forward1), std::move(after)};
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

prepare::PreparedBirModule legalize_merge3_conditional_successor_use_module() {
  bir::Module module;
  bir::Function function;
  function.name = "merge3_conditional_successor_use";
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
  join.terminator = bir::BranchTerminator{.target_label = "gate"};

  bir::Block gate;
  gate.label = "gate";
  gate.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Eq,
      .result = bir::Value::named(bir::TypeKind::I1, "cond2"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(0),
  });
  gate.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I1, "cond2"),
      .true_label = "after_true",
      .false_label = "after_false",
  };

  bir::Block after_true;
  after_true.label = "after_true";
  after_true.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "sum"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "merge"),
      .rhs = bir::Value::immediate_i32(5),
  });
  after_true.terminator = bir::ReturnTerminator{.value = bir::Value::named(bir::TypeKind::I32, "sum")};

  bir::Block after_false;
  after_false.label = "after_false";
  after_false.terminator = bir::ReturnTerminator{.value = bir::Value::immediate_i32(0)};

  function.blocks = {std::move(entry),  std::move(left),  std::move(split), std::move(middle),
                     std::move(right),  std::move(join),  std::move(gate),  std::move(after_true),
                     std::move(after_false)};
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

int check_materialized_join(const bir::Function& legalized, bool add_trailing_use) {
  if (!legalized.local_slots.empty()) {
    return fail("expected reducible phi tree to materialize without local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr) {
    return fail("missing join block after legalize");
  }

  const std::size_t expected_inst_count = add_trailing_use ? 3u : 2u;
  if (join_block->insts.size() != expected_inst_count) {
    return fail("expected nested selects and only the live trailing join instructions");
  }

  const auto* nested_select = std::get_if<bir::SelectInst>(&join_block->insts[0]);
  const auto* root_select = std::get_if<bir::SelectInst>(&join_block->insts[1]);
  if (nested_select == nullptr || root_select == nullptr) {
    return fail("expected join block to begin with nested select materialization");
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

  if (add_trailing_use) {
    const auto* add = std::get_if<bir::BinaryInst>(&join_block->insts[2]);
    if (add == nullptr) {
      return fail("expected trailing add after select materialization");
    }
    if (add->lhs.kind != bir::Value::Kind::Named || add->lhs.name != "merge") {
      return fail("expected trailing add to consume the materialized phi result");
    }
  } else if (!legalized.blocks.back().terminator.value.has_value() ||
             legalized.blocks.back().terminator.value->kind != bir::Value::Kind::Named ||
             legalized.blocks.back().terminator.value->name != "merge") {
    return fail("expected return terminator to consume the materialized phi result");
  }

  for (const auto& inst : join_block->insts) {
    if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
        std::holds_alternative<bir::StoreLocalInst>(inst)) {
      return fail("unexpected fallback phi lowering remained in join block");
    }
  }

  return 0;
}

int check_two_way_materialized_join(const bir::Function& legalized) {
  if (!legalized.local_slots.empty()) {
    return fail("expected joined compare-branch phi materialization to avoid fallback local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr || join_block->insts.size() != 3) {
    return fail("expected the joined compare-branch lane to materialize into two arithmetic defs plus one select");
  }

  const auto* true_adjust = std::get_if<bir::BinaryInst>(&join_block->insts[0]);
  const auto* false_adjust = std::get_if<bir::BinaryInst>(&join_block->insts[1]);
  const auto* select = std::get_if<bir::SelectInst>(&join_block->insts[2]);
  if (true_adjust == nullptr || false_adjust == nullptr || select == nullptr) {
    return fail("expected the joined compare-branch lane to end in a select-backed join");
  }
  if (true_adjust->result.name != "zero.adjusted" || false_adjust->result.name != "nonzero.adjusted" ||
      select->result.name != "merge" || !is_named_i32(select->true_value, "zero.adjusted") ||
      !is_named_i32(select->false_value, "nonzero.adjusted")) {
    return fail("expected the joined compare-branch lane to preserve the materialized join values");
  }
  if (join_block->terminator.kind != bir::TerminatorKind::Return ||
      !join_block->terminator.value.has_value() ||
      !is_named_i32(*join_block->terminator.value, "merge")) {
    return fail("expected the joined compare-branch lane to return the materialized join result");
  }

  for (const auto& block : legalized.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
          std::holds_alternative<bir::StoreLocalInst>(inst)) {
        return fail("unexpected fallback phi lowering remained in the joined compare-branch lane");
      }
    }
  }

  return 0;
}

int check_materialized_successor_join(const bir::Function& legalized) {
  if (!legalized.local_slots.empty()) {
    return fail("expected successor-consumed reducible phi tree to materialize without local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr) {
    return fail("missing join block after legalize for successor use case");
  }
  if (join_block->insts.size() != 2) {
    return fail("expected successor-use join to keep only nested selects");
  }
  const auto* nested_select = std::get_if<bir::SelectInst>(&join_block->insts[0]);
  const auto* root_select = std::get_if<bir::SelectInst>(&join_block->insts[1]);
  if (nested_select == nullptr || root_select == nullptr) {
    return fail("expected successor-use join block to begin with nested select materialization");
  }
  if (root_select->result.name != "merge") {
    return fail("expected successor-use root select to define the original phi result");
  }
  if (join_block->terminator.kind != bir::TerminatorKind::Branch ||
      join_block->terminator.target_label != "after") {
    return fail("expected successor-use join to preserve the branch to the consumer block");
  }

  const auto* after_block = find_block(legalized, "after");
  if (after_block == nullptr) {
    return fail("missing successor consumer block after legalize");
  }
  if (after_block->insts.size() != 1) {
    return fail("expected successor consumer block to keep exactly one add");
  }
  const auto* add = std::get_if<bir::BinaryInst>(&after_block->insts[0]);
  if (add == nullptr || add->lhs.kind != bir::Value::Kind::Named || add->lhs.name != "merge") {
    return fail("expected successor consumer block to use the materialized phi result");
  }

  for (const auto& block : legalized.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
          std::holds_alternative<bir::StoreLocalInst>(inst)) {
        return fail("unexpected fallback phi lowering remained in successor-use case");
      }
    }
  }

  return 0;
}

int check_materialized_forwarded_successor_join(const bir::Function& legalized) {
  if (!legalized.local_slots.empty()) {
    return fail("expected forwarded-consumer reducible phi tree to materialize without local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr) {
    return fail("missing join block after legalize for forwarded successor use case");
  }
  if (join_block->insts.size() != 2) {
    return fail("expected forwarded successor join to keep only nested selects");
  }
  const auto* nested_select = std::get_if<bir::SelectInst>(&join_block->insts[0]);
  const auto* root_select = std::get_if<bir::SelectInst>(&join_block->insts[1]);
  if (nested_select == nullptr || root_select == nullptr) {
    return fail("expected forwarded successor join block to begin with nested select materialization");
  }
  if (root_select->result.name != "merge") {
    return fail("expected forwarded successor root select to define the original phi result");
  }

  const auto* forward0_block = find_block(legalized, "forward0");
  const auto* forward1_block = find_block(legalized, "forward1");
  if (forward0_block == nullptr || forward1_block == nullptr) {
    return fail("missing forwarding chain blocks after legalize");
  }
  if (forward0_block->terminator.kind != bir::TerminatorKind::Branch ||
      forward0_block->terminator.target_label != "forward1" ||
      forward1_block->terminator.kind != bir::TerminatorKind::Branch ||
      forward1_block->terminator.target_label != "after") {
    return fail("expected forwarded successor chain to preserve linear branches");
  }

  const auto* after_block = find_block(legalized, "after");
  if (after_block == nullptr) {
    return fail("missing forwarded successor consumer block after legalize");
  }
  if (after_block->insts.size() != 1) {
    return fail("expected forwarded successor consumer block to keep exactly one add");
  }
  const auto* add = std::get_if<bir::BinaryInst>(&after_block->insts[0]);
  if (add == nullptr || add->lhs.kind != bir::Value::Kind::Named || add->lhs.name != "merge") {
    return fail("expected forwarded successor consumer block to use the materialized phi result");
  }

  for (const auto& block : legalized.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
          std::holds_alternative<bir::StoreLocalInst>(inst)) {
        return fail("unexpected fallback phi lowering remained in forwarded successor case");
      }
    }
  }

  return 0;
}

int check_materialized_conditional_successor_join(const bir::Function& legalized) {
  if (!legalized.local_slots.empty()) {
    return fail("expected conditional-successor reducible phi tree to materialize without local slots");
  }

  const auto* join_block = find_block(legalized, "join");
  if (join_block == nullptr) {
    return fail("missing join block after legalize for conditional successor use case");
  }
  if (join_block->insts.size() != 2) {
    return fail("expected conditional successor join to keep only nested selects");
  }
  const auto* nested_select = std::get_if<bir::SelectInst>(&join_block->insts[0]);
  const auto* root_select = std::get_if<bir::SelectInst>(&join_block->insts[1]);
  if (nested_select == nullptr || root_select == nullptr) {
    return fail("expected conditional successor join block to begin with nested select materialization");
  }
  if (root_select->result.name != "merge") {
    return fail("expected conditional successor root select to define the original phi result");
  }

  const auto* gate_block = find_block(legalized, "gate");
  if (gate_block == nullptr) {
    return fail("missing conditional successor gate block after legalize");
  }
  if (gate_block->insts.size() != 1 ||
      !std::holds_alternative<bir::BinaryInst>(gate_block->insts[0]) ||
      gate_block->terminator.kind != bir::TerminatorKind::CondBranch ||
      gate_block->terminator.true_label != "after_true" ||
      gate_block->terminator.false_label != "after_false") {
    return fail("expected conditional successor gate block to preserve its compare and branch");
  }

  const auto* after_true_block = find_block(legalized, "after_true");
  if (after_true_block == nullptr) {
    return fail("missing conditional successor consumer block after legalize");
  }
  if (after_true_block->insts.size() != 1) {
    return fail("expected conditional successor consumer block to keep exactly one add");
  }
  const auto* add = std::get_if<bir::BinaryInst>(&after_true_block->insts[0]);
  if (add == nullptr || add->lhs.kind != bir::Value::Kind::Named || add->lhs.name != "merge") {
    return fail("expected conditional successor consumer block to use the materialized phi result");
  }

  for (const auto& block : legalized.blocks) {
    for (const auto& inst : block.insts) {
      if (std::holds_alternative<bir::PhiInst>(inst) || std::holds_alternative<bir::LoadLocalInst>(inst) ||
          std::holds_alternative<bir::StoreLocalInst>(inst)) {
        return fail("unexpected fallback phi lowering remained in conditional successor case");
      }
    }
  }

  return 0;
}

int check_legalized_call_abi_metadata(const prepare::PreparedBirModule& prepared) {
  if (prepared.module.globals.size() != 1 || prepared.module.functions.size() != 2) {
    return fail("expected one global plus declaration and caller when checking legality metadata");
  }

  const auto& global = prepared.module.globals.front();
  if (global.type != bir::TypeKind::I32 || global.size_bytes != 4 || global.align_bytes != 4 ||
      !global.initializer.has_value() || global.initializer->type != bir::TypeKind::I32 ||
      global.initializer->immediate != 1) {
    return fail("expected legalize to promote global storage bookkeeping away from i1");
  }

  const auto& callee = prepared.module.functions[0];
  if (!callee.is_declaration || callee.return_type != bir::TypeKind::I32 || callee.params.size() != 1 ||
      callee.params[0].type != bir::TypeKind::I32) {
    return fail("expected legalize to promote declaration signatures away from i1");
  }
  if (callee.return_size_bytes != 4 || callee.return_align_bytes != 4 || callee.params[0].size_bytes != 4 ||
      callee.params[0].align_bytes != 4) {
    return fail("expected legalize to promote declaration signature bookkeeping away from i1");
  }

  const auto& caller = prepared.module.functions[1];
  if (caller.return_type != bir::TypeKind::I32 || caller.params.size() != 1 ||
      caller.params[0].type != bir::TypeKind::I32) {
    return fail("expected legalize to promote caller signatures away from i1");
  }
  if (caller.return_size_bytes != 4 || caller.return_align_bytes != 4 || caller.params[0].size_bytes != 4 ||
      caller.params[0].align_bytes != 4) {
    return fail("expected legalize to promote caller signature bookkeeping away from i1");
  }
  if (caller.local_slots.size() != 1 || caller.local_slots.front().type != bir::TypeKind::I32 ||
      caller.local_slots.front().size_bytes != 4 || caller.local_slots.front().align_bytes != 4) {
    return fail("expected legalize to promote local slot bookkeeping away from i1");
  }
  if (caller.blocks.size() != 1 || caller.blocks.front().insts.size() != 1) {
    return fail("expected single caller block with one legalized call");
  }

  const auto* call = std::get_if<bir::CallInst>(&caller.blocks.front().insts.front());
  if (call == nullptr) {
    return fail("expected caller block to keep a call instruction");
  }
  if (!call->result.has_value() || call->result->type != bir::TypeKind::I32) {
    return fail("expected legalize to promote call result value type away from i1");
  }
  if (call->args.size() != 1 || call->args.front().type != bir::TypeKind::I32) {
    return fail("expected legalize to promote call argument value types away from i1");
  }
  if (call->arg_types.size() != 1 || call->arg_types.front() != bir::TypeKind::I32) {
    return fail("expected legalize to promote call argument type metadata away from i1");
  }
  if (call->arg_abi.size() != 1 || call->arg_abi.front().type != bir::TypeKind::I32 ||
      call->arg_abi.front().size_bytes != 4 || call->arg_abi.front().align_bytes != 4) {
    return fail("expected legalize to promote call ABI metadata away from i1");
  }
  if (!call->result_abi.has_value() || call->result_abi->type != bir::TypeKind::I32) {
    return fail("expected legalize to promote call result ABI metadata away from i1");
  }
  if (call->return_type != bir::TypeKind::I32) {
    return fail("expected legalize to promote call return type metadata away from i1");
  }
  if (call->return_type_name != "i32") {
    return fail("expected legalize to promote call return type text away from i1");
  }
  if (!caller.blocks.front().terminator.value.has_value() ||
      caller.blocks.front().terminator.value->type != bir::TypeKind::I32) {
    return fail("expected legalize to promote call return terminator away from i1");
  }
  return 0;
}

int check_legalized_memory_access_metadata(const prepare::PreparedBirModule& prepared) {
  if (prepared.module.functions.size() != 1) {
    return fail("expected one function when checking legalized memory-access metadata");
  }
  const auto& function = prepared.module.functions.front();
  if (function.params.size() != 3 || function.params[0].type != bir::TypeKind::I32) {
    return fail("expected legalize to promote the stored flag param away from i1");
  }
  if (function.blocks.size() != 1 || function.blocks.front().insts.size() != 4) {
    return fail("expected one block with four legalized memory-access instructions");
  }

  const auto* store_local = std::get_if<bir::StoreLocalInst>(&function.blocks.front().insts[0]);
  const auto* load_local = std::get_if<bir::LoadLocalInst>(&function.blocks.front().insts[1]);
  const auto* store_global = std::get_if<bir::StoreGlobalInst>(&function.blocks.front().insts[2]);
  const auto* load_global = std::get_if<bir::LoadGlobalInst>(&function.blocks.front().insts[3]);
  if (store_local == nullptr || load_local == nullptr || store_global == nullptr || load_global == nullptr) {
    return fail("expected legalized memory-access instructions to retain their structure");
  }

  const auto address_has_promoted_bookkeeping = [](const std::optional<bir::MemoryAddress>& address,
                                                   std::string_view expected_base_name) {
    return address.has_value() &&
           address->base_kind == bir::MemoryAddress::BaseKind::PointerValue &&
           address->base_value.type == bir::TypeKind::Ptr && address->base_value.name == expected_base_name &&
           address->size_bytes == 4 && address->align_bytes == 4;
  };

  if (store_local->value.type != bir::TypeKind::I32 || store_local->align_bytes != 4 ||
      !address_has_promoted_bookkeeping(store_local->address, "local.addr")) {
    return fail("expected legalize to promote local store bookkeeping away from i1");
  }
  if (load_local->result.type != bir::TypeKind::I32 || load_local->align_bytes != 4 ||
      !address_has_promoted_bookkeeping(load_local->address, "local.addr")) {
    return fail("expected legalize to promote local load bookkeeping away from i1");
  }
  if (store_global->value.type != bir::TypeKind::I32 || store_global->align_bytes != 4 ||
      !address_has_promoted_bookkeeping(store_global->address, "global.addr")) {
    return fail("expected legalize to promote global store bookkeeping away from i1");
  }
  if (load_global->result.type != bir::TypeKind::I32 || load_global->align_bytes != 4 ||
      !address_has_promoted_bookkeeping(load_global->address, "global.addr")) {
    return fail("expected legalize to promote global load bookkeeping away from i1");
  }
  if (!function.blocks.front().terminator.value.has_value() ||
      function.blocks.front().terminator.value->type != bir::TypeKind::I32) {
    return fail("expected legalize to promote the memory-access return terminator away from i1");
  }
  return 0;
}

}  // namespace

int main() {
  const auto prepared_call_abi = legalize_call_abi_module();
  if (const int status = check_prepare_i1_invariant(prepared_call_abi); status != 0) {
    return status;
  }
  if (const int status = check_legalized_call_abi_metadata(prepared_call_abi); status != 0) {
    return status;
  }

  const auto prepared_memory_access = legalize_memory_access_module();
  if (const int status = check_prepare_i1_invariant(prepared_memory_access); status != 0) {
    return status;
  }
  if (const int status = check_legalized_memory_access_metadata(prepared_memory_access); status != 0) {
    return status;
  }

  const auto prepared_with_add = legalize_merge3_module(true);
  if (const int status = check_prepare_phi_invariant(prepared_with_add); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_with_add); status != 0) {
    return status;
  }
  if (const int status = check_prepared_control_flow_contract(
          prepared_with_add,
          "merge3_add",
          2,
          prepare::PreparedJoinTransferKind::SelectMaterialization);
      status != 0) {
    return status;
  }
  if (const int status = check_materialized_join(prepared_with_add.module.functions.front(), true); status != 0) {
    return status;
  }

  const auto prepared_return_only = legalize_merge3_module(false);
  if (const int status = check_prepare_phi_invariant(prepared_return_only); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_return_only); status != 0) {
    return status;
  }
  if (const int status = check_materialized_join(prepared_return_only.module.functions.front(), false); status != 0) {
    return status;
  }

  const auto prepared_successor_use = legalize_merge3_successor_use_module();
  if (const int status = check_prepare_phi_invariant(prepared_successor_use); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_successor_use); status != 0) {
    return status;
  }
  if (const int status = check_prepared_control_flow_contract(
          prepared_successor_use,
          "merge3_successor_use",
          2,
          prepare::PreparedJoinTransferKind::SelectMaterialization);
      status != 0) {
    return status;
  }
  if (const int status = check_materialized_successor_join(prepared_successor_use.module.functions.front());
      status != 0) {
    return status;
  }

  const auto prepared_forwarded_successor_use = legalize_merge3_forwarded_successor_use_module();
  if (const int status = check_prepare_phi_invariant(prepared_forwarded_successor_use); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_forwarded_successor_use); status != 0) {
    return status;
  }
  if (const int status =
          check_materialized_forwarded_successor_join(prepared_forwarded_successor_use.module.functions.front());
      status != 0) {
    return status;
  }

  const auto prepared_two_way_join = legalize_two_way_branch_join_module();
  if (const int status = check_prepare_phi_invariant(prepared_two_way_join); status != 0) {
    return status;
  }
  const auto prepared_two_way_join_legalize_only = legalize_two_way_branch_join_module_legalize_only();
  if (const int status = check_legalize_preserves_phi(prepared_two_way_join_legalize_only); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_two_way_join); status != 0) {
    return status;
  }
  if (const int status =
          check_two_way_branch_join_control_flow_contract(prepared_two_way_join,
                                                          "branch_join_prepare_contract");
      status != 0) {
    return status;
  }
  if (const int status = check_two_way_materialized_join(prepared_two_way_join.module.functions.front());
      status != 0) {
    return status;
  }

  const auto prepared_short_circuit_or = legalize_short_circuit_or_guard_module();
  if (const int status = check_prepare_phi_invariant(prepared_short_circuit_or); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_short_circuit_or); status != 0) {
    return status;
  }
  if (const int status =
          check_short_circuit_or_control_flow_contract(prepared_short_circuit_or,
                                                       "short_circuit_or_prepare_contract");
      status != 0) {
    return status;
  }

  const auto prepared_loop_countdown = legalize_loop_countdown_module();
  if (const int status = check_prepare_phi_invariant(prepared_loop_countdown); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_loop_countdown); status != 0) {
    return status;
  }
  if (const int status =
          check_loop_countdown_control_flow_contract(prepared_loop_countdown,
                                                    "loop_countdown_prepare_contract");
      status != 0) {
    return status;
  }

  const auto prepared_id_authoritative_loop = legalize_id_authoritative_loop_countdown_module();
  if (const int status = check_prepare_phi_invariant(prepared_id_authoritative_loop); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_id_authoritative_loop); status != 0) {
    return status;
  }
  if (const int status = check_id_authoritative_loop_countdown_contract(
          prepared_id_authoritative_loop,
          "id_authoritative_loop_countdown_prepare_contract");
      status != 0) {
    return status;
  }

  const auto prepared_parallel_copy_cycle = legalize_parallel_copy_cycle_module();
  if (const int status = check_prepare_phi_invariant(prepared_parallel_copy_cycle); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_parallel_copy_cycle); status != 0) {
    return status;
  }
  if (const int status =
          check_parallel_copy_cycle_contract(prepared_parallel_copy_cycle,
                                             "parallel_copy_prepare_contract");
      status != 0) {
    return status;
  }
  const std::string parallel_copy_dump = prepare::print(prepared_parallel_copy_cycle);
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy entry -> loop execution_site=predecessor_terminator execution_block=entry has_cycle=no resolution=acyclic moves=2 steps=2")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "parallel_copy body -> loop execution_site=predecessor_terminator execution_block=body has_cycle=yes resolution=cycle_break moves=2 steps=3")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "move[0] 1 -> a join_transfer_index=0 edge_transfer_index=0")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "move[1] 2 -> b join_transfer_index=1 edge_transfer_index=0")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "move[0] b -> a join_transfer_index=0 edge_transfer_index=1")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "move[1] a -> b join_transfer_index=1 edge_transfer_index=1")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[0] save_destination_to_temp move_index=0 save_destination=a blocked_source=b temp_source=cycle_temp(a) carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[1] move move_index=0 source=b destination=a carrier=edge_store_slot storage=a.phi uses_cycle_temp_source=no")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(parallel_copy_dump,
                       "step[2] move move_index=1 source=cycle_temp(a) destination=b carrier=edge_store_slot storage=b.phi uses_cycle_temp_source=yes")) {
    return EXIT_FAILURE;
  }

  const auto prepared_critical_edge = legalize_critical_edge_parallel_copy_module();
  if (const int status = check_prepare_phi_invariant(prepared_critical_edge); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_critical_edge); status != 0) {
    return status;
  }
  if (const int status = check_critical_edge_parallel_copy_contract(
          prepared_critical_edge, "critical_edge_parallel_copy_prepare_contract");
      status != 0) {
    return status;
  }
  const std::string critical_edge_dump = prepare::print(prepared_critical_edge);
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy left -> join execution_site=critical_edge execution_block=<none> has_cycle=no resolution=acyclic moves=1 steps=1")) {
    return EXIT_FAILURE;
  }
  if (!expect_contains(
          critical_edge_dump,
          "parallel_copy right -> join execution_site=predecessor_terminator execution_block=right has_cycle=no resolution=acyclic moves=1 steps=1")) {
    return EXIT_FAILURE;
  }

  const auto prepared_conditional_successor_use = legalize_merge3_conditional_successor_use_module();
  if (const int status = check_prepare_phi_invariant(prepared_conditional_successor_use); status != 0) {
    return status;
  }
  if (const int status = check_prepare_i1_invariant(prepared_conditional_successor_use); status != 0) {
    return status;
  }
  return check_materialized_conditional_successor_join(
      prepared_conditional_successor_use.module.functions.front());
}

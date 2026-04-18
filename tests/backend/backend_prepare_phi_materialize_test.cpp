#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

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
  for (const auto& function : prepared.control_flow.functions) {
    if (function.function_name == function_name) {
      return &function;
    }
  }
  return nullptr;
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
    return fail("expected prepare legalize to publish the no-phi-nodes invariant");
  }
  if (prepared.invariants.empty() ||
      prepare::prepared_bir_invariant_name(prepared.invariants.front()) != "no_phi_nodes") {
    return fail("expected stable name for the no-phi-nodes invariant");
  }
  if (prepared.module.functions.size() != 1) {
    return fail("expected exactly one function when checking the phi legality invariant");
  }
  if (function_contains_phi(prepared.module.functions.front())) {
    return fail("expected legalize to remove phi nodes from prepared semantic BIR");
  }
  return 0;
}

int check_prepare_i1_invariant(const prepare::PreparedBirModule& prepared) {
  if (!contains_invariant(prepared, prepare::PreparedBirInvariant::NoTargetFacingI1)) {
    return fail("expected prepare legalize to publish the no-target-facing-i1 invariant");
  }
  if (prepared.invariants.size() < 2 ||
      prepare::prepared_bir_invariant_name(prepared.invariants[1]) != "no_target_facing_i1") {
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
  if (entry_condition.block_label != "entry" || !entry_condition.predicate.has_value() ||
      !entry_condition.compare_type.has_value() || !entry_condition.lhs.has_value() ||
      !entry_condition.rhs.has_value()) {
    return fail("expected entry branch-condition metadata to publish compare semantics");
  }
  if (*entry_condition.predicate != bir::BinaryOpcode::Eq ||
      *entry_condition.compare_type != bir::TypeKind::I32 ||
      !is_immediate_i32(*entry_condition.lhs, 0) || !is_immediate_i32(*entry_condition.rhs, 0) ||
      entry_condition.true_label != "left" || entry_condition.false_label != "split") {
    return fail("expected entry branch-condition metadata to match the legalized compare branch");
  }
  if (entry_condition.condition_value.kind != bir::Value::Kind::Named ||
      entry_condition.condition_value.name != "cond0" ||
      entry_condition.condition_value.type != bir::TypeKind::I32) {
    return fail("expected entry branch-condition metadata to track the legalized condition value");
  }

  const auto& join_transfer = control_flow->join_transfers.front();
  if (join_transfer.join_block_label != "join" || join_transfer.result.name != "merge") {
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

#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/codegen.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/machine_printer.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/comparison.hpp"
#include "src/backend/prealloc/prepared_lookups.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <string>
#include <string_view>
#include <variant>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

void attach_prepared_function_lookups(
    aarch64_module::FunctionLoweringContext& function_context,
    const prepare::PreparedFunctionLookups& prepared_lookups) {
  function_context.prepared_lookups = &prepared_lookups;
  function_context.call_plan_lookups = &prepared_lookups.call_plans;
  function_context.address_materialization_lookups =
      &prepared_lookups.address_materializations;
  function_context.move_bundle_lookups = &prepared_lookups.move_bundles;
  function_context.value_home_lookups = &prepared_lookups.value_homes;
}

prepare::PreparedBirModule prepared_with_unconditional_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("branch.fn");
  const auto entry_label = prepared.names.block_labels.intern("branch.entry");
  const auto exit_label = prepared.names.block_labels.intern("branch.exit");
  const auto function_link_name = prepared.module.names.link_names.intern("branch.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("branch.entry");
  const auto bir_exit_label = prepared.module.names.block_labels.intern("branch.exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = exit_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = exit_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
  });

  bir::Block entry;
  entry.label = "branch.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::BranchTerminator{
      .target_label = "branch.exit",
      .target_label_id = bir_exit_label,
  };

  bir::Block exit;
  exit.label = "branch.exit";
  exit.label_id = bir_exit_label;
  exit.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "branch.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(exit);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_unconditional_branch_divergent_bir_label_ids() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("branch.split_ids");
  const auto entry_label = prepared.names.block_labels.intern("branch.entry");
  const auto exit_label = prepared.names.block_labels.intern("branch.exit");
  const auto function_link_name = prepared.module.names.link_names.intern("branch.split_ids");
  prepared.module.names.block_labels.intern("module.only");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("branch.entry");
  const auto bir_exit_label = prepared.module.names.block_labels.intern("branch.exit");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = exit_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = exit_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
  });

  bir::Block entry;
  entry.label = "branch.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::BranchTerminator{
      .target_label = "branch.exit",
      .target_label_id = bir_exit_label,
  };

  bir::Block exit;
  exit.label = "branch.exit";
  exit.label_id = bir_exit_label;
  exit.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "branch.split_ids";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(exit);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedRegisterPlacement caller_saved_gpr(std::size_t slot_index) {
  return prepare::PreparedRegisterPlacement{
      .bank = prepare::PreparedRegisterBank::Gpr,
      .pool = prepare::PreparedRegisterSlotPool::CallerSaved,
      .slot_index = slot_index,
      .contiguous_width = 1,
  };
}

prepare::PreparedBirModule prepared_with_fused_compare_conditional_branch(
    bool can_fuse_with_branch = true) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("cond.fn");
  const auto entry_label = prepared.names.block_labels.intern("cond.entry");
  const auto then_label = prepared.names.block_labels.intern("cond.then");
  const auto else_label = prepared.names.block_labels.intern("cond.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto lhs_name = prepared.names.value_names.intern("%lhs");
  const auto function_link_name = prepared.module.names.link_names.intern("cond.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("cond.entry");
  const auto bir_then_label = prepared.module.names.block_labels.intern("cond.then");
  const auto bir_else_label = prepared.module.names.block_labels.intern("cond.else");
  const auto condition = bir::Value::named(bir::TypeKind::I32, "%cond");
  const auto lhs = bir::Value::named(bir::TypeKind::I32, "%lhs");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Eq,
          .compare_type = bir::TypeKind::I32,
          .lhs = lhs,
          .rhs = bir::Value::immediate_i32(0),
          .can_fuse_with_branch = can_fuse_with_branch,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{9},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{10},
                  .function_name = function_name,
                  .value_name = lhs_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{10},
          .value_name = lhs_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_placement = caller_saved_gpr(0),
      }},
  });

  bir::Block entry;
  entry.label = "cond.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "cond.then",
      .false_label = "cond.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "cond.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_i32_sext_i64_fused_compare_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("sext.cond.fn");
  const auto entry_label = prepared.names.block_labels.intern("sext.cond.entry");
  const auto then_label = prepared.names.block_labels.intern("sext.cond.then");
  const auto else_label = prepared.names.block_labels.intern("sext.cond.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto source_name = prepared.names.value_names.intern("%src");
  prepared.names.value_names.intern("%wide");
  const auto function_link_name =
      prepared.module.names.link_names.intern("sext.cond.fn");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("sext.cond.entry");
  const auto bir_then_label =
      prepared.module.names.block_labels.intern("sext.cond.then");
  const auto bir_else_label =
      prepared.module.names.block_labels.intern("sext.cond.else");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto source = bir::Value::named(bir::TypeKind::I32, "%src");
  const auto widened = bir::Value::named(bir::TypeKind::I64, "%wide");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Eq,
          .compare_type = bir::TypeKind::I64,
          .lhs = widened,
          .rhs = bir::Value::immediate_i64(0),
          .can_fuse_with_branch = true,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{30},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{31},
                  .function_name = function_name,
                  .value_name = source_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w13"},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{31},
          .value_name = source_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .contiguous_width = 1,
          .register_name = std::string{"w13"},
          .occupied_register_names = {std::string{"w13"}},
      }},
  });

  bir::Block entry;
  entry.label = "sext.cond.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = source,
      .operand = bir::Value::immediate_i16(1),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = widened,
      .operand = source,
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "sext.cond.then",
      .false_label = "sext.cond.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "sext.cond.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_i32_sext_i64_constant_udiv_bound_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("sext.bound.fn");
  const auto entry_label = prepared.names.block_labels.intern("sext.bound.entry");
  const auto then_label = prepared.names.block_labels.intern("sext.bound.then");
  const auto else_label = prepared.names.block_labels.intern("sext.bound.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto bound_name = prepared.names.value_names.intern("%bound");
  prepared.names.value_names.intern("%wide");
  const auto function_link_name =
      prepared.module.names.link_names.intern("sext.bound.fn");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("sext.bound.entry");
  const auto bir_then_label =
      prepared.module.names.block_labels.intern("sext.bound.then");
  const auto bir_else_label =
      prepared.module.names.block_labels.intern("sext.bound.else");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto source = bir::Value::named(bir::TypeKind::I32, "%src");
  const auto bound = bir::Value::named(bir::TypeKind::I64, "%bound");
  const auto widened = bir::Value::named(bir::TypeKind::I64, "%wide");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Ult,
          .compare_type = bir::TypeKind::I64,
          .lhs = widened,
          .rhs = bound,
          .can_fuse_with_branch = true,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{40},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{41},
                  .function_name = function_name,
                  .value_name = source_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w13"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{42},
                  .function_name = function_name,
                  .value_name = bound_name,
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{41},
          .value_name = source_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .contiguous_width = 1,
          .register_name = std::string{"w13"},
          .occupied_register_names = {std::string{"w13"}},
      }},
  });

  bir::Block entry;
  entry.label = "sext.bound.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = source,
      .operand = bir::Value::immediate_i16(1),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::UDiv,
      .result = bound,
      .operand_type = bir::TypeKind::I64,
      .lhs = bir::Value::immediate_i64(504),
      .rhs = bir::Value::immediate_i64(56),
  });
  entry.insts.push_back(bir::CastInst{
      .opcode = bir::CastOpcode::SExt,
      .result = widened,
      .operand = source,
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Ult,
      .result = condition,
      .operand_type = bir::TypeKind::I64,
      .lhs = widened,
      .rhs = bound,
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "sext.bound.then",
      .false_label = "sext.bound.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "sext.bound.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_stack_homed_constant_binary_bound_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("sizeof.bound.fn");
  const auto entry_label = prepared.names.block_labels.intern("sizeof.bound.entry");
  const auto then_label = prepared.names.block_labels.intern("sizeof.bound.then");
  const auto else_label = prepared.names.block_labels.intern("sizeof.bound.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto index_name = prepared.names.value_names.intern("%idx");
  const auto bound_name = prepared.names.value_names.intern("%sizeof.bound");
  const auto function_link_name =
      prepared.module.names.link_names.intern("sizeof.bound.fn");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("sizeof.bound.entry");
  const auto bir_then_label =
      prepared.module.names.block_labels.intern("sizeof.bound.then");
  const auto bir_else_label =
      prepared.module.names.block_labels.intern("sizeof.bound.else");
  const auto condition = bir::Value::named(bir::TypeKind::I32, "%cond");
  const auto index = bir::Value::named(bir::TypeKind::I32, "%idx");
  const auto bound = bir::Value::named(bir::TypeKind::I32, "%sizeof.bound");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Slt,
          .compare_type = bir::TypeKind::I32,
          .lhs = index,
          .rhs = bound,
          .can_fuse_with_branch = true,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{70},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{71},
                  .function_name = function_name,
                  .value_name = index_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w13"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{72},
                  .function_name = function_name,
                  .value_name = bound_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{72},
                  .offset_bytes = std::size_t{32},
                  .size_bytes = std::size_t{4},
                  .align_bytes = std::size_t{4},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{71},
                  .value_name = index_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"w13"},
                  .occupied_register_names = {std::string{"w13"}},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{72},
                  .value_name = bound_name,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .slot_id = prepare::PreparedFrameSlotId{72},
                  .stack_offset_bytes = std::size_t{32},
              },
          },
  });

  bir::Block entry;
  entry.label = "sizeof.bound.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Mul,
      .result = bound,
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(16),
      .rhs = bir::Value::immediate_i32(4),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Slt,
      .result = condition,
      .operand_type = bir::TypeKind::I32,
      .lhs = index,
      .rhs = bound,
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "sizeof.bound.then",
      .false_label = "sizeof.bound.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "sizeof.bound.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_loop_header_fused_compare_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("loop.fn");
  const auto header_label = prepared.names.block_labels.intern("for.cond.1");
  const auto body_label = prepared.names.block_labels.intern("block_1");
  const auto exit_label = prepared.names.block_labels.intern("block_2");
  const auto condition_name = prepared.names.value_names.intern("%t1");
  const auto lhs_name = prepared.names.value_names.intern("%t0");
  const auto function_link_name = prepared.module.names.link_names.intern("loop.fn");
  prepared.module.names.block_labels.intern("module.only");
  const auto bir_header_label = prepared.module.names.block_labels.intern("for.cond.1");
  const auto bir_body_label = prepared.module.names.block_labels.intern("block_1");
  const auto bir_exit_label = prepared.module.names.block_labels.intern("block_2");
  const auto condition = bir::Value::named(bir::TypeKind::I32, "%t1");
  const auto lhs = bir::Value::named(bir::TypeKind::I32, "%t0");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = header_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = body_label,
          .false_label = exit_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = header_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Ne,
          .compare_type = bir::TypeKind::I32,
          .lhs = lhs,
          .rhs = bir::Value::immediate_i32(0),
          .can_fuse_with_branch = true,
          .true_label = body_label,
          .false_label = exit_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{11},
                  .function_name = function_name,
                  .value_name = condition_name,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{12},
                  .function_name = function_name,
                  .value_name = lhs_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{12},
          .value_name = lhs_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_placement = caller_saved_gpr(0),
      }},
  });

  bir::Block header;
  header.label = "for.cond.1";
  header.label_id = bir_header_label;
  header.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "block_1",
      .false_label = "block_2",
      .true_label_id = bir_body_label,
      .false_label_id = bir_exit_label,
  };

  bir::Function function;
  function.name = "loop.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(header);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_materialized_bool_conditional_branch() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("mb.fn");
  const auto entry_label = prepared.names.block_labels.intern("mb.entry");
  const auto then_label = prepared.names.block_labels.intern("mb.then");
  const auto else_label = prepared.names.block_labels.intern("mb.else");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto function_link_name = prepared.module.names.link_names.intern("mb.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("mb.entry");
  const auto bir_then_label = prepared.module.names.block_labels.intern("mb.then");
  const auto bir_else_label = prepared.module.names.block_labels.intern("mb.else");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = then_label,
                  .false_label = else_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = then_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = else_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
          .condition_value = condition,
          .can_fuse_with_branch = false,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes = {prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{9},
          .function_name = function_name,
          .value_name = condition_name,
      }},
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values = {prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{9},
          .value_name = condition_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .register_placement = caller_saved_gpr(0),
      }},
  });

  bir::Block entry;
  entry.label = "mb.entry";
  entry.label_id = bir_entry_label;
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "mb.then",
      .false_label = "mb.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Block then_block;
  then_block.label = "mb.then";
  then_block.label_id = bir_then_label;
  then_block.terminator = bir::ReturnTerminator{};

  bir::Block else_block;
  else_block.label = "mb.else";
  else_block.label_id = bir_else_label;
  else_block.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "mb.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(then_block);
  function.blocks.push_back(else_block);
  prepared.module.functions.push_back(function);
  return prepared;
}

prepare::PreparedBirModule prepared_with_materialized_compare_condition_clobber() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("mb.clobber.fn");
  const auto entry_label = prepared.names.block_labels.intern("mb.clobber.entry");
  const auto then_label = prepared.names.block_labels.intern("mb.clobber.then");
  const auto else_label = prepared.names.block_labels.intern("mb.clobber.else");
  const auto source_name = prepared.names.value_names.intern("%src");
  const auto advanced_name = prepared.names.value_names.intern("%advanced");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto clobber_name = prepared.names.value_names.intern("%clobber");
  const auto function_link_name =
      prepared.module.names.link_names.intern("mb.clobber.fn");
  const auto bir_entry_label =
      prepared.module.names.block_labels.intern("mb.clobber.entry");
  const auto bir_then_label =
      prepared.module.names.block_labels.intern("mb.clobber.then");
  const auto bir_else_label =
      prepared.module.names.block_labels.intern("mb.clobber.else");
  const auto source = bir::Value::named(bir::TypeKind::I32, "%src");
  const auto advanced = bir::Value::named(bir::TypeKind::I32, "%advanced");
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto clobber = bir::Value::named(bir::TypeKind::I32, "%clobber");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks = {prepare::PreparedControlFlowBlock{
          .block_label = entry_label,
          .terminator_kind = bir::TerminatorKind::CondBranch,
          .true_label = then_label,
          .false_label = else_label,
      }},
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::MaterializedBool,
          .condition_value = condition,
          .can_fuse_with_branch = false,
          .true_label = then_label,
          .false_label = else_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{50},
                  .function_name = function_name,
                  .value_name = source_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w13"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{51},
                  .function_name = function_name,
                  .value_name = advanced_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w9"},
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{52},
                  .function_name = function_name,
                  .value_name = condition_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{52},
                  .offset_bytes = 128,
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{53},
                  .function_name = function_name,
                  .value_name = clobber_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w9"},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{50},
                  .value_name = source_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"w13"},
                  .occupied_register_names = {std::string{"w13"}},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{51},
                  .value_name = advanced_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"w9"},
                  .occupied_register_names = {std::string{"w9"}},
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{52},
                  .value_name = condition_name,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .slot_id = prepare::PreparedFrameSlotId{52},
                  .stack_offset_bytes = 128,
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{53},
                  .value_name = clobber_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"w9"},
                  .occupied_register_names = {std::string{"w9"}},
              },
          },
  });

  bir::Block entry;
  entry.label = "mb.clobber.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = advanced,
      .operand_type = bir::TypeKind::I32,
      .lhs = source,
      .rhs = bir::Value::immediate_i32(16),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Sle,
      .result = condition,
      .operand_type = bir::TypeKind::I32,
      .lhs = advanced,
      .rhs = bir::Value::immediate_i32(0),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = clobber,
      .operand_type = bir::TypeKind::I32,
      .lhs = source,
      .rhs = bir::Value::immediate_i32(16),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "mb.clobber.then",
      .false_label = "mb.clobber.else",
      .true_label_id = bir_then_label,
      .false_label_id = bir_else_label,
  };

  bir::Function function;
  function.name = "mb.clobber.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  prepared.module.functions.push_back(function);
  return prepared;
}

void move_materialized_condition_home_to_register(
    prepare::PreparedBirModule& prepared,
    std::string register_name = "w21") {
  const auto condition_name = prepared.names.value_names.intern("%cond");
  for (auto& home : prepared.value_locations.functions.front().value_homes) {
    if (home.value_name == condition_name) {
      home.kind = prepare::PreparedValueHomeKind::Register;
      home.register_name = register_name;
      home.slot_id = prepare::PreparedFrameSlotId{};
      home.offset_bytes.reset();
      home.size_bytes.reset();
      home.align_bytes.reset();
    }
  }
  for (auto& storage : prepared.storage_plans.functions.front().values) {
    if (storage.value_name == condition_name) {
      storage.encoding = prepare::PreparedStorageEncodingKind::Register;
      storage.slot_id = prepare::PreparedFrameSlotId{};
      storage.stack_offset_bytes.reset();
      storage.register_name = register_name;
      storage.occupied_register_names = {register_name};
    }
  }
}

int expect_materialized_compare_condition_fallback(
    prepare::PreparedBirModule prepared,
    std::string_view failure_message) {
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto prepared_lookups =
      prepare::make_prepared_function_lookups(prepared, function_cf);
  attach_prepared_function_lookups(function_context, prepared_lookups);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 12);

  aarch64_module::MachineBlock machine_block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, machine_block, diagnostics);
  if (!result.visited_terminator ||
      machine_block.instructions.empty() ||
      !diagnostics.empty()) {
    return fail(std::string{failure_message} + ": dispatch failed");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(
          machine_block.instructions.back().target);
  bool saw_cbnz = false;
  bool saw_cmp = false;
  for (const auto& line : printed.instruction_lines) {
    saw_cbnz = saw_cbnz || line.find("cbnz ") == 0;
    saw_cmp = saw_cmp || line.find("cmp ") == 0;
  }
  if (!printed.ok ||
      printed.instruction_lines.empty() ||
      !saw_cbnz ||
      saw_cmp ||
      printed.instruction_lines.back().find("b ") != 0) {
    std::string actual = printed.ok ? std::string{} : std::string{"<unprintable>"};
    for (const auto& line : printed.instruction_lines) {
      if (!actual.empty()) {
        actual += " | ";
      }
      actual += line;
    }
    return fail(std::string{failure_message} +
                ": expected emitted-condition fallback, got " + actual);
  }
  return 0;
}

int expect_materialized_compare_condition_selected_fallback(
    prepare::PreparedBirModule prepared,
    std::string_view failure_message) {
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto prepared_lookups =
      prepare::make_prepared_function_lookups(prepared, function_cf);
  attach_prepared_function_lookups(function_context, prepared_lookups);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 12);

  aarch64_module::MachineBlock machine_block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, machine_block, diagnostics);
  if (!result.visited_terminator ||
      machine_block.instructions.empty() ||
      !diagnostics.empty()) {
    return fail(std::string{failure_message} + ": dispatch failed");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(
          machine_block.instructions.back().target);
  bool saw_cmp = false;
  bool saw_cbnz = false;
  bool saw_conditional_branch = false;
  for (const auto& line : printed.instruction_lines) {
    saw_cmp = saw_cmp || line.find("cmp ") == 0;
    saw_cbnz = saw_cbnz || line.find("cbnz ") == 0;
    saw_conditional_branch = saw_conditional_branch ||
                             line.find("b.le ") == 0 ||
                             line.find("b.lt ") == 0 ||
                             line.find("b.eq ") == 0 ||
                             line.find("b.ne ") == 0;
  }
  if (!printed.ok ||
      !saw_cmp ||
      saw_cbnz ||
      !saw_conditional_branch ||
      printed.instruction_lines.empty() ||
      printed.instruction_lines.back().find("b ") != 0) {
    std::string actual = printed.ok ? std::string{} : std::string{"<unprintable>"};
    for (const auto& line : printed.instruction_lines) {
      if (!actual.empty()) {
        actual += " | ";
      }
      actual += line;
    }
    return fail(std::string{failure_message} +
                ": expected selected compare fallback, got " + actual);
  }
  return 0;
}

prepare::PreparedBirModule prepared_with_late_conditional_successor_after_return() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("late.cond.fn");
  const auto entry_label = prepared.names.block_labels.intern("late.cond.entry");
  const auto return_label = prepared.names.block_labels.intern("late.cond.return");
  const auto body_label = prepared.names.block_labels.intern("late.cond.body");
  const auto join_label = prepared.names.block_labels.intern("late.cond.join");
  const auto selected_name = prepared.names.value_names.intern("%selected");
  const auto condition_name = prepared.names.value_names.intern("%cond");
  const auto function_link_name = prepared.module.names.link_names.intern("late.cond.fn");
  const auto bir_entry_label = prepared.module.names.block_labels.intern("late.cond.entry");
  const auto bir_return_label = prepared.module.names.block_labels.intern("late.cond.return");
  const auto bir_body_label = prepared.module.names.block_labels.intern("late.cond.body");
  const auto bir_join_label = prepared.module.names.block_labels.intern("late.cond.join");
  const auto selected = bir::Value::named(bir::TypeKind::I32, "%selected");
  const auto condition = bir::Value::named(bir::TypeKind::I32, "%cond");

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = body_label,
                  .false_label = return_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = return_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = body_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = join_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = join_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .branch_conditions = {prepare::PreparedBranchCondition{
          .function_name = function_name,
          .block_label = entry_label,
          .kind = prepare::PreparedBranchConditionKind::FusedCompare,
          .condition_value = condition,
          .predicate = bir::BinaryOpcode::Slt,
          .compare_type = bir::TypeKind::I32,
          .lhs = selected,
          .rhs = bir::Value::immediate_i32(7),
          .can_fuse_with_branch = true,
          .true_label = body_label,
          .false_label = return_label,
      }},
  });
  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{60},
                  .function_name = function_name,
                  .value_name = selected_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = prepare::PreparedFrameSlotId{60},
                  .offset_bytes = 16,
                  .size_bytes = 4,
                  .align_bytes = 4,
              },
              prepare::PreparedValueHome{
                  .value_id = prepare::PreparedValueId{61},
                  .function_name = function_name,
                  .value_name = condition_name,
                  .kind = prepare::PreparedValueHomeKind::Register,
                  .register_name = std::string{"w21"},
              },
          },
  });
  prepared.storage_plans.functions.push_back(prepare::PreparedStoragePlanFunction{
      .function_name = function_name,
      .values =
          {
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{60},
                  .value_name = selected_name,
                  .encoding = prepare::PreparedStorageEncodingKind::FrameSlot,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .slot_id = prepare::PreparedFrameSlotId{60},
                  .stack_offset_bytes = 16,
              },
              prepare::PreparedStoragePlanValue{
                  .value_id = prepare::PreparedValueId{61},
                  .value_name = condition_name,
                  .encoding = prepare::PreparedStorageEncodingKind::Register,
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .contiguous_width = 1,
                  .register_name = std::string{"w21"},
                  .occupied_register_names = {std::string{"w21"}},
              },
          },
  });

  bir::Block entry;
  entry.label = "late.cond.entry";
  entry.label_id = bir_entry_label;
  entry.insts.push_back(bir::SelectInst{
      .predicate = bir::BinaryOpcode::Eq,
      .result = selected,
      .compare_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(0),
      .rhs = bir::Value::immediate_i32(0),
      .true_value = bir::Value::immediate_i32(3),
      .false_value = bir::Value::immediate_i32(9),
  });
  entry.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Slt,
      .result = condition,
      .operand_type = bir::TypeKind::I32,
      .lhs = selected,
      .rhs = bir::Value::immediate_i32(7),
  });
  entry.terminator = bir::CondBranchTerminator{
      .condition = condition,
      .true_label = "late.cond.body",
      .false_label = "late.cond.return",
      .true_label_id = bir_body_label,
      .false_label_id = bir_return_label,
  };

  bir::Block early_return;
  early_return.label = "late.cond.return";
  early_return.label_id = bir_return_label;
  early_return.terminator = bir::ReturnTerminator{};

  bir::Block late_body;
  late_body.label = "late.cond.body";
  late_body.label_id = bir_body_label;
  late_body.terminator = bir::BranchTerminator{
      .target_label = "late.cond.join",
      .target_label_id = bir_join_label,
  };

  bir::Block join;
  join.label = "late.cond.join";
  join.label_id = bir_join_label;
  join.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "late.cond.fn";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(entry);
  function.blocks.push_back(early_return);
  function.blocks.push_back(late_body);
  function.blocks.push_back(join);
  prepared.module.functions.push_back(function);
  return prepared;
}

int direct_dispatch_lowers_unconditional_branch_to_selected_node() {
  auto prepared = prepared_with_unconditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 3);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected unconditional branch dispatch to emit one selected instruction");
  }
  if (block.successors.size() != 1 ||
      block.successors.front().target_label != function_cf.blocks[1].block_label ||
      block.successors.front().kind != mir::MachineBlockSuccessorKind::Unconditional ||
      !block.successors.front().origin.has_value() ||
      block.successors.front().origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors.front().origin->function_name != function_cf.function_name ||
      block.successors.front().origin->block_label != block_cf.block_label) {
    return fail("expected unconditional branch dispatch to record one typed successor");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason !=
          c4c::backend::mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.surface != aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::Branch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 3) {
    return fail("expected branch instruction target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || branch->conditional || branch->target_pair.has_value() ||
      !branch->condition_record.has_value() ||
      branch->condition_record->form != aarch64_codegen::BranchConditionForm::Unconditional ||
      branch->target.function_name != function_cf.function_name ||
      branch->target.block_label != function_cf.blocks[1].block_label ||
      branch->target.condition_value_id.has_value()) {
    return fail("expected branch payload to preserve prepared destination label identity");
  }
  return 0;
}

int direct_dispatch_lowers_unconditional_branch_with_divergent_bir_label_ids() {
  auto prepared = prepared_with_unconditional_branch_divergent_bir_label_ids();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected unconditional branch to canonicalize retained BIR target labels");
  }
  if (block.successors.size() != 1 ||
      block.successors.front().target_label != function_cf.blocks[1].block_label) {
    return fail("expected canonicalized unconditional branch to record prepared successor label");
  }
  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(
          &block.instructions.front().target.payload);
  if (branch == nullptr || branch->target.block_label != function_cf.blocks[1].block_label) {
    return fail("expected canonicalized unconditional branch payload to use prepared target");
  }
  return 0;
}

int direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node() {
  auto prepared = prepared_with_materialized_bool_conditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 4);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected materialized-bool conditional branch to emit one selected instruction");
  }
  if (block.successors.size() != 2 ||
      block.successors[0].target_label != block_cf.true_label ||
      block.successors[0].kind != mir::MachineBlockSuccessorKind::ConditionalTrue ||
      block.successors[1].target_label != block_cf.false_label ||
      block.successors[1].kind != mir::MachineBlockSuccessorKind::ConditionalFalse ||
      !block.successors[0].origin.has_value() ||
      !block.successors[1].origin.has_value() ||
      block.successors[0].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[1].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[0].origin->function_name != function_cf.function_name ||
      block.successors[1].origin->block_label != block_cf.block_label) {
    return fail("expected materialized-bool branch dispatch to record true/false successors");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason != mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected conditional branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::ConditionalBranch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 4) {
    return fail("expected conditional branch target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || !branch->conditional || !branch->target_pair.has_value() ||
      !branch->condition.has_value() || !branch->condition_record.has_value() ||
      branch->condition_record->form != aarch64_codegen::BranchConditionForm::MaterializedBool ||
      branch->target_pair->true_target.block_label != block_cf.true_label ||
      branch->target_pair->false_target.block_label != block_cf.false_label) {
    return fail("expected conditional branch payload to preserve prepared condition targets");
  }
  if (branch->condition->kind != aarch64_codegen::OperandKind::Register) {
    return fail("expected materialized-bool condition operand to be a typed register");
  }
  const auto* condition_register =
      std::get_if<aarch64_codegen::RegisterOperand>(&branch->condition->payload);
  if (condition_register == nullptr ||
      condition_register->role != aarch64_codegen::RegisterOperandRole::StoragePlan ||
      !condition_register->value_id.has_value() ||
      *condition_register->value_id != prepare::PreparedValueId{9} ||
      condition_register->value_name != prepared.names.value_names.intern("%cond") ||
      !condition_register->expected_view.has_value() ||
      *condition_register->expected_view != aarch64_abi::RegisterView::W) {
    return fail("expected materialized-bool condition to resolve from typed register authority");
  }
  return 0;
}

int materialized_compare_branch_does_not_reuse_clobbered_bool_register() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto prepared_lookups =
      prepare::make_prepared_function_lookups(prepared, function_cf);
  attach_prepared_function_lookups(function_context, prepared_lookups);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 12);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (!result.visited_terminator || block.instructions.size() < 2 ||
      block.successors.size() != 2 || !diagnostics.empty()) {
    return fail("expected clobbered materialized compare branch to lower with successors");
  }

  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(block.instructions.back().target);
  if (!printed.ok || printed.instruction_lines.size() != 6 ||
      printed.instruction_lines[0] != "mov w9, w13" ||
      printed.instruction_lines[1] != "mov w10, #16" ||
      printed.instruction_lines[2] != "add x9, x9, x10" ||
      printed.instruction_lines[3] != "cmp w9, #0" ||
      printed.instruction_lines[4] !=
          "b.le .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[5] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected materialized compare terminator to recompute compare before branch");
  }
  return 0;
}

int materialized_compare_branch_route7_provenance_matches_bir_identity() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  const auto& block = prepared.module.functions.front().blocks.front();
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto route7_index = bir::route7_build_comparison_condition_index(block);
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto source_producers =
      prepare::make_prepared_edge_publication_source_producer_lookups(
          prepared, function_cf);
  const auto prepared_producer =
      prepare::find_prepared_materialized_condition_producer(
          prepared.names,
          &source_producers,
          block_cf.block_label,
          &block,
          condition,
          block.insts.size());
  const auto route7_record =
      bir::route7_find_materialized_condition(route7_index,
                                              block,
                                              condition,
                                              block.insts.size());
  const auto route7_reference =
      bir::route_index_validate_materialized_condition_reference(
          bir::route_index_reference_facade(route7_index),
          block,
          condition,
          block.insts.size());
  const auto bir_identity =
      bir::find_materialized_condition_producer_identity(
          block, condition, block.insts.size());
  if (!route7_record ||
      !route7_reference ||
      route7_reference.comparison_record == nullptr ||
      route7_reference.comparison_record->lhs.status !=
          bir::Route7ComparisonStatus::Available ||
      route7_reference.comparison_record->rhs.status !=
          bir::Route7ComparisonStatus::Available ||
      !bir_identity.available ||
      route7_record.instruction_index != bir_identity.instruction_index ||
      route7_reference.comparison_record->binary != bir_identity.binary ||
      !prepared_producer.has_value() ||
      route7_reference.comparison_record->binary != prepared_producer->binary ||
      route7_reference.comparison_record->instruction_index !=
          prepared_producer->instruction_index ||
      prepared.names.value_names.find(
          route7_reference.comparison_record->condition_value.name) !=
          prepared_producer->condition_value_name ||
      prepared.names.value_names.find(bir_identity.condition_value_name) !=
          prepared_producer->condition_value_name ||
      bir_identity.binary == nullptr ||
      bir_identity.binary->opcode != bir::BinaryOpcode::Sle) {
    return fail("expected Route 7 materialized condition provenance to match prepared and BIR identity");
  }

  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto prepared_lookups =
      prepare::make_prepared_function_lookups(prepared, function_cf);
  attach_prepared_function_lookups(function_context, prepared_lookups);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 12);

  aarch64_module::MachineBlock machine_block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, machine_block, diagnostics);
  if (!result.visited_terminator ||
      machine_block.instructions.empty() ||
      !diagnostics.empty()) {
    return fail("expected selected materialized compare branch to lower from Route 7-compatible provenance");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(
          machine_block.instructions.back().target);
  if (!printed.ok ||
      printed.instruction_lines.size() != 6 ||
      printed.instruction_lines[0] != "mov w9, w13" ||
      printed.instruction_lines[1] != "mov w10, #16" ||
      printed.instruction_lines[2] != "add x9, x9, x10" ||
      printed.instruction_lines[3] != "cmp w9, #0" ||
      printed.instruction_lines[4] !=
          "b.le .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[5] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected selected materialized compare branch to lower from Route 7-compatible provenance");
  }
  const auto& instruction = machine_block.instructions.back();
  const auto* assembler =
      std::get_if<aarch64_codegen::AssemblerInstructionRecord>(
          &instruction.target.payload);
  if (assembler == nullptr ||
      !assembler->has_inline_asm_payload ||
      assembler->inline_asm_template.find("cmp w9, #0\nb.le ") == std::string::npos ||
      instruction.target.family != aarch64_codegen::InstructionFamily::Assembler ||
      instruction.target.surface !=
          aarch64_codegen::RecordSurfaceKind::MachineInstructionNode ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.instruction_index != block.insts.size() ||
      !instruction.origin.has_value() ||
      instruction.origin->reason != mir::MachineOriginReason::BirInstruction ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label ||
      instruction.origin->instruction_index != block.insts.size()) {
    return fail("expected selected materialized compare row to keep Route 7 provenance local to the assembler oracle");
  }
  return 0;
}

int materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  move_materialized_condition_home_to_register(prepared);
  auto& block = prepared.module.functions.front().blocks.front();
  block.insts.insert(block.insts.begin() + 2,
                     bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Sgt,
                         .result = bir::Value::named(bir::TypeKind::I1, "%cond"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::named(bir::TypeKind::I32, "%advanced"),
                         .rhs = bir::Value::immediate_i32(1),
                     });
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto route7_index = bir::route7_build_comparison_condition_index(block);
  const auto route7_reference =
      bir::route_index_validate_materialized_condition_reference(
          bir::route_index_reference_facade(route7_index),
          block,
          condition,
          block.insts.size());
  if (route7_reference ||
      route7_reference.status != bir::RouteIndexValidationStatus::DuplicateReference ||
      bir::find_materialized_condition_producer_identity(
          block, condition, block.insts.size()).available) {
    return fail("expected duplicate materialized condition provenance to be invalid");
  }

  return expect_materialized_compare_condition_fallback(
      std::move(prepared),
      "expected invalid Route 7 materialized condition to preserve emitted-condition fallback");
}

int materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  move_materialized_condition_home_to_register(prepared);
  auto& block = prepared.module.functions.front().blocks.front();
  block.insts.erase(block.insts.begin() + 1);

  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto route7_index = bir::route7_build_comparison_condition_index(block);
  const auto route7_record =
      bir::route7_find_materialized_condition(route7_index,
                                              block,
                                              condition,
                                              block.insts.size());
  if (route7_record ||
      (route7_record.status != bir::Route7ComparisonStatus::MissingBlock &&
       route7_record.status != bir::Route7ComparisonStatus::NoMatch)) {
    return fail("expected absent materialized condition provenance");
  }

  return expect_materialized_compare_condition_fallback(
      std::move(prepared),
      "expected absent Route 7 materialized condition to preserve emitted-condition fallback");
}

int materialized_compare_branch_condition_name_mismatch_uses_emitted_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  move_materialized_condition_home_to_register(prepared);
  const auto other_condition_name = prepared.names.value_names.intern("%other_cond");
  prepared.value_locations.functions.front().value_homes.push_back(
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{55},
          .function_name = prepared.control_flow.functions.front().function_name,
          .value_name = other_condition_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"w22"},
      });
  prepared.storage_plans.functions.front().values.push_back(
      prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{55},
          .value_name = other_condition_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .contiguous_width = 1,
          .register_name = std::string{"w22"},
          .occupied_register_names = {std::string{"w22"}},
      });
  auto& compare =
      std::get<bir::BinaryInst>(prepared.module.functions.front().blocks.front().insts[1]);
  compare.result = bir::Value::named(bir::TypeKind::I1, "%other_cond");

  return expect_materialized_compare_condition_fallback(
      std::move(prepared),
      "expected mismatched Route 7 materialized condition name to preserve emitted-condition fallback");
}

int materialized_compare_branch_invalid_route7_reference_rejected() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  auto& block = prepared.module.functions.front().blocks.front();
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  auto route7_index = bir::route7_build_comparison_condition_index(block);
  for (auto& record : route7_index.comparison_records) {
    if (record.condition_value.name == "%cond") {
      record.instruction = nullptr;
      break;
    }
  }

  const auto route7_reference =
      bir::route_index_validate_materialized_condition_reference(
          bir::route_index_reference_facade(route7_index),
          block,
          condition,
          block.insts.size());
  if (route7_reference ||
      route7_reference.status != bir::RouteIndexValidationStatus::StaleOwner) {
    return fail("expected invalid Route 7 materialized condition reference to reject stale producer index: status=" +
                std::to_string(static_cast<unsigned>(route7_reference.status)));
  }
  return 0;
}

int materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  const auto prepared_lookup_snapshot = prepared;
  const auto& stale_function_cf =
      prepared_lookup_snapshot.control_flow.functions.front();
  const auto stale_lookups =
      prepare::make_prepared_function_lookups(
          prepared_lookup_snapshot, stale_function_cf);

  auto& block = prepared.module.functions.front().blocks.front();
  auto& compare = std::get<bir::BinaryInst>(block.insts[1]);
  compare.opcode = bir::BinaryOpcode::Slt;
  const auto condition = bir::Value::named(bir::TypeKind::I1, "%cond");
  const auto route7_index = bir::route7_build_comparison_condition_index(block);
  const auto route7_record =
      bir::route7_find_materialized_condition(route7_index,
                                              block,
                                              condition,
                                              block.insts.size());
  const auto bir_identity =
      bir::find_materialized_condition_producer_identity(
          block, condition, block.insts.size());
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto stale_prepared_producer =
      prepare::find_prepared_materialized_condition_producer(
          prepared.names,
          &stale_lookups.edge_publication_source_producers,
          block_cf.block_label,
          &block,
          condition,
          block.insts.size());
  if (!route7_record ||
      !bir_identity.available ||
      bir_identity.binary != &compare ||
      stale_prepared_producer.has_value()) {
    return fail(
        "expected valid Route 7/BIR materialized condition with stale prepared producer mismatch");
  }

  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  attach_prepared_function_lookups(function_context, stale_lookups);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 12);

  aarch64_module::MachineBlock machine_block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, machine_block, diagnostics);
  if (!result.visited_terminator ||
      machine_block.instructions.empty() ||
      !diagnostics.empty()) {
    return fail("expected stale prepared producer mismatch to preserve BIR fallback");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(
          machine_block.instructions.back().target);
  bool saw_cmp = false;
  bool saw_conditional_branch = false;
  bool saw_cbnz = false;
  for (const auto& line : printed.instruction_lines) {
    saw_cmp = saw_cmp || line.find("cmp ") == 0;
    saw_conditional_branch = saw_conditional_branch || line.find("b.lt ") == 0;
    saw_cbnz = saw_cbnz || line.find("cbnz ") == 0;
  }
  if (!printed.ok ||
      !saw_cmp ||
      !saw_conditional_branch ||
      saw_cbnz) {
    return fail(
        "expected route/prepared mismatch to preserve selected BIR compare fallback");
  }
  return 0;
}

int materialized_compare_branch_lhs_provenance_mismatch_uses_emitted_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  move_materialized_condition_home_to_register(prepared);
  auto& block = prepared.module.functions.front().blocks.front();
  block.insts.insert(block.insts.begin() + 1,
                     bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Add,
                         .result = bir::Value::named(bir::TypeKind::I32, "%advanced"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::named(bir::TypeKind::I32, "%src"),
                         .rhs = bir::Value::immediate_i32(32),
                     });

  return expect_materialized_compare_condition_selected_fallback(
      std::move(prepared),
      "expected lhs provenance mismatch to preserve selected compare fallback");
}

int materialized_compare_branch_rhs_provenance_mismatch_uses_emitted_fallback() {
  auto prepared = prepared_with_materialized_compare_condition_clobber();
  move_materialized_condition_home_to_register(prepared);
  const auto rhs_name = prepared.names.value_names.intern("%rhs");
  prepared.value_locations.functions.front().value_homes.push_back(
      prepare::PreparedValueHome{
          .value_id = prepare::PreparedValueId{54},
          .function_name = prepared.control_flow.functions.front().function_name,
          .value_name = rhs_name,
          .kind = prepare::PreparedValueHomeKind::Register,
          .register_name = std::string{"w14"},
      });
  prepared.storage_plans.functions.front().values.push_back(
      prepare::PreparedStoragePlanValue{
          .value_id = prepare::PreparedValueId{54},
          .value_name = rhs_name,
          .encoding = prepare::PreparedStorageEncodingKind::Register,
          .bank = prepare::PreparedRegisterBank::Gpr,
          .contiguous_width = 1,
          .register_name = std::string{"w14"},
          .occupied_register_names = {std::string{"w14"}},
      });
  auto& block = prepared.module.functions.front().blocks.front();
  auto& compare = std::get<bir::BinaryInst>(block.insts[1]);
  compare.rhs = bir::Value::named(bir::TypeKind::I32, "%rhs");
  block.insts.insert(block.insts.begin() + 1,
                     bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Add,
                         .result = bir::Value::named(bir::TypeKind::I32, "%rhs"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::named(bir::TypeKind::I32, "%src"),
                         .rhs = bir::Value::immediate_i32(1),
                     });
  block.insts.insert(block.insts.begin() + 2,
                     bir::BinaryInst{
                         .opcode = bir::BinaryOpcode::Add,
                         .result = bir::Value::named(bir::TypeKind::I32, "%rhs"),
                         .operand_type = bir::TypeKind::I32,
                         .lhs = bir::Value::named(bir::TypeKind::I32, "%src"),
                         .rhs = bir::Value::immediate_i32(2),
                     });

  return expect_materialized_compare_condition_selected_fallback(
      std::move(prepared),
      "expected rhs provenance mismatch to preserve selected compare fallback");
}

int direct_dispatch_lowers_fusable_compare_branch_to_selected_node() {
  auto prepared = prepared_with_fused_compare_conditional_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 5);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected fusable fused-compare branch to emit one selected instruction");
  }
  if (block.successors.size() != 2 ||
      block.successors[0].target_label != block_cf.true_label ||
      block.successors[0].kind != mir::MachineBlockSuccessorKind::ConditionalTrue ||
      block.successors[1].target_label != block_cf.false_label ||
      block.successors[1].kind != mir::MachineBlockSuccessorKind::ConditionalFalse ||
      !block.successors[0].origin.has_value() ||
      !block.successors[1].origin.has_value() ||
      block.successors[0].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[1].origin->reason != mir::MachineOriginReason::BirTerminator ||
      block.successors[0].origin->function_name != function_cf.function_name ||
      block.successors[1].origin->function_name != function_cf.function_name ||
      block.successors[0].origin->block_label != block_cf.block_label ||
      block.successors[1].origin->block_label != block_cf.block_label) {
    return fail("expected fusable compare branch dispatch to record true/false successors");
  }

  const auto& instruction = block.instructions.front();
  if (!instruction.origin.has_value() ||
      instruction.origin->reason != mir::MachineOriginReason::BirTerminator ||
      instruction.origin->function_name != function_cf.function_name ||
      instruction.origin->block_label != block_cf.block_label) {
    return fail("expected compare branch origin to preserve source block identity");
  }
  if (instruction.target.family != aarch64_codegen::InstructionFamily::Branch ||
      instruction.target.opcode != aarch64_codegen::MachineOpcode::CompareBranch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected ||
      instruction.target.function_name != function_cf.function_name ||
      instruction.target.block_label != block_cf.block_label ||
      instruction.target.block_index != 5) {
    return fail("expected compare branch target to be canonical selected branch node");
  }

  const auto* branch =
      std::get_if<aarch64_codegen::BranchInstructionRecord>(&instruction.target.payload);
  if (branch == nullptr || !branch->conditional || !branch->target_pair.has_value() ||
      !branch->condition_record.has_value() || branch->condition.has_value() ||
      branch->target_pair->true_target.block_label != block_cf.true_label ||
      branch->target_pair->false_target.block_label != block_cf.false_label) {
    return fail("expected compare branch payload to preserve prepared branch targets");
  }

  const auto& condition = *branch->condition_record;
  if (condition.form != aarch64_codegen::BranchConditionForm::FusedCompare ||
      !condition.can_fuse_with_branch || !condition.predicate.has_value() ||
      !condition.compare_operands.has_value() ||
      condition.predicate->source_predicate != bir::BinaryOpcode::Eq ||
      condition.predicate->compare_type != bir::TypeKind::I32 ||
      condition.compare_operands->compare_type != bir::TypeKind::I32 ||
      condition.compare_operands->lhs.source_value !=
          bir::Value::named(bir::TypeKind::I32, "%lhs") ||
      condition.compare_operands->rhs.source_value != bir::Value::immediate_i32(0)) {
    return fail("expected compare branch condition record to preserve compare facts");
  }
  if (!condition.compare_branch_candidate.has_value() ||
      condition.compare_branch_candidate->kind !=
          aarch64_codegen::BranchCompareCandidateKind::FusedCompareAndBranch ||
      !condition.compare_branch_candidate->can_fuse_with_branch ||
      !condition.compare_branch_candidate->target_pair.has_value()) {
    return fail("expected compare branch condition to retain fusable candidate metadata");
  }
  if (instruction.target.operands.size() != 5 ||
      instruction.target.operands[0].kind != aarch64_codegen::OperandKind::BranchTarget ||
      instruction.target.operands[1].kind != aarch64_codegen::OperandKind::BranchTarget ||
      instruction.target.operands[2].kind != aarch64_codegen::OperandKind::PreparedValue ||
      instruction.target.operands[3].kind != aarch64_codegen::OperandKind::Register ||
      instruction.target.operands[4].kind != aarch64_codegen::OperandKind::Immediate) {
    return fail("expected compare branch node operands to carry targets and compare inputs");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(instruction.target);
  if (!printed.ok || printed.instruction_lines.size() != 3 ||
      printed.instruction_lines[0] != "cmp w13, #0" ||
      printed.instruction_lines[1] !=
          "b.eq .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[2] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected fusable compare branch to print cmp plus true/false branches");
  }
  return 0;
}

int direct_dispatch_lowers_i32_sext_i64_fused_compare_branch_with_legal_widths() {
  auto prepared = prepared_with_i32_sext_i64_fused_compare_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 9);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (!result.visited_terminator || result.emitted_instructions != 2 ||
      block.instructions.size() != 2 || !diagnostics.empty()) {
    std::string messages;
    for (const auto& diagnostic : diagnostics.entries) {
      messages += diagnostic.message;
      messages += "; ";
    }
    return fail("expected i32->i64 sext compare branch to lower after source publication: emitted=" +
                std::to_string(result.emitted_instructions) +
                " block=" + std::to_string(block.instructions.size()) +
                " diagnostics=" + std::to_string(diagnostics.entries.size()) + " " +
                messages);
  }

  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(block.instructions.back().target);
  if (!printed.ok || printed.instruction_lines.size() != 4 ||
      printed.instruction_lines[0] != "sxtw x9, w13" ||
      printed.instruction_lines[1] != "cmp x9, #0" ||
      printed.instruction_lines[2] !=
          "b.eq .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[3] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected fused i32->i64 sign-extension branch to use X destination and compare: " +
                (printed.ok ? printed.instruction_lines.empty()
                                  ? std::string{}
                                  : printed.instruction_lines.front()
                            : std::string{"<print failed>"}));
  }
  return 0;
}

int direct_dispatch_lowers_i32_sext_i64_constant_udiv_bound_branch() {
  auto prepared = prepared_with_i32_sext_i64_constant_udiv_bound_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 10);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (!result.visited_terminator || result.emitted_instructions != 2 ||
      block.instructions.size() != 2 || !diagnostics.empty()) {
    std::string messages;
    for (const auto& diagnostic : diagnostics.entries) {
      messages += diagnostic.message;
      messages += "; ";
    }
    return fail("expected i32->i64 sext compare branch to fold constant udiv bound: emitted=" +
                std::to_string(result.emitted_instructions) +
                " block=" + std::to_string(block.instructions.size()) +
                " diagnostics=" + std::to_string(diagnostics.entries.size()) + " " +
                messages);
  }

  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(block.instructions.back().target);
  if (!printed.ok || printed.instruction_lines.size() != 4 ||
      printed.instruction_lines[0] != "sxtw x9, w13" ||
      printed.instruction_lines[1] != "cmp x9, #9" ||
      printed.instruction_lines[2] !=
          "b.lo .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[3] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected fused i32->i64 branch to compare against folded constant bound");
  }
  return 0;
}

int direct_dispatch_lowers_stack_homed_constant_binary_rhs_as_compare_immediate() {
  auto prepared = prepared_with_stack_homed_constant_binary_bound_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 10);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (!result.visited_terminator || block.instructions.empty() ||
      !diagnostics.empty()) {
    std::string messages;
    for (const auto& diagnostic : diagnostics.entries) {
      messages += diagnostic.message;
      messages += "; ";
    }
    return fail("expected stack-homed compile-time RHS bound to lower as fused compare immediate: emitted=" +
                std::to_string(result.emitted_instructions) +
                " block=" + std::to_string(block.instructions.size()) +
                " diagnostics=" + std::to_string(diagnostics.entries.size()) + " " +
                messages);
  }

  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(block.instructions.back().target);
  if (!printed.ok || printed.instruction_lines.size() != 4 ||
      printed.instruction_lines[0] != "mov w9, w13" ||
      printed.instruction_lines[1] != "cmp w9, #64" ||
      printed.instruction_lines[2] !=
          "b.lt .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[3] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected fused branch to materialize named same-block constant RHS as compare immediate");
  }

  for (const auto& instruction : block.instructions) {
    const auto payload =
        aarch64_codegen::print_machine_instruction_line_payloads(instruction.target);
    if (!payload.ok) {
      continue;
    }
    for (const auto& line : payload.instruction_lines) {
      if (line.find("ldr ") != std::string::npos &&
          line.find("[sp, #32]") != std::string::npos) {
        return fail("fused branch read the unpublished RHS stack home instead of the folded constant");
      }
    }
  }
  return 0;
}

int direct_dispatch_lowers_loop_header_fused_compare_branch_with_divergent_bir_label_ids() {
  auto prepared = prepared_with_loop_header_fused_compare_branch();
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 7);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 1 || block.instructions.size() != 1 ||
      !diagnostics.empty()) {
    return fail("expected loop-header compare branch to canonicalize retained BIR targets");
  }
  if (block.successors.size() != 2 ||
      block.successors[0].target_label != block_cf.true_label ||
      block.successors[1].target_label != block_cf.false_label) {
    return fail("expected loop-header compare branch to record prepared true/false successors");
  }

  const auto& instruction = block.instructions.front();
  if (instruction.target.opcode != aarch64_codegen::MachineOpcode::CompareBranch ||
      instruction.target.selection.status !=
          aarch64_codegen::MachineNodeSelectionStatus::Selected) {
    return fail("expected loop-header terminator to select a fused compare branch");
  }
  const auto printed =
      aarch64_codegen::print_machine_instruction_line_payloads(instruction.target);
  if (!printed.ok || printed.instruction_lines.size() != 3 ||
      printed.instruction_lines[0] != "cmp w13, #0" ||
      printed.instruction_lines[1] !=
          "b.ne .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.true_label) ||
      printed.instruction_lines[2] !=
          "b .LBB" + std::to_string(function_cf.function_name) + "_" +
              std::to_string(block_cf.false_label)) {
    return fail("expected loop-header fused compare branch to print cmp plus exits");
  }
  return 0;
}

int module_build_keeps_branch_node_without_restoring_legacy_return_nodes() {
  auto prepared = prepared_with_unconditional_branch();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared branch module to build");
  }

  const auto& module = *result.module;
  if (module.mir.functions.size() != 1 || module.mir.functions.front().blocks.size() != 2 ||
      module.mir.functions.front().blocks[0].instructions.size() != 1 ||
      module.mir.functions.front().blocks[1].instructions.size() != 1 ||
      module.mir.functions.front().blocks[0].successors.size() != 1 ||
      module.mir.functions.front().blocks[1].successors.size() != 0) {
    return fail("expected module build to lower branch block and return block");
  }
  if (module.mir.functions.front().blocks[0].successors.front().target_label !=
      module.mir.functions.front().blocks[1].block_label) {
    return fail("expected module build to keep branch successor target label identity");
  }
  if (module.functions.front().machine_nodes.size() != 1 ||
      module.functions.front().machine_nodes.front().family !=
          aarch64_codegen::InstructionFamily::Branch ||
      module.compatibility.functions.front().machine_nodes.size() != 1) {
    return fail("expected compatibility nodes to include selected branch but not return");
  }
  return 0;
}

int non_fusable_compare_branch_control_stays_fail_closed() {
  auto prepared = prepared_with_fused_compare_conditional_branch(false);
  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& block_cf = function_cf.blocks.front();
  const auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  const auto block_context =
      aarch64_codegen::make_block_lowering_context(function_context, block_cf, 0);

  aarch64_module::MachineBlock block;
  aarch64_module::ModuleLoweringDiagnostics diagnostics;
  const auto result =
      aarch64_codegen::dispatch_prepared_block(block_context, block, diagnostics);

  if (result.visited_operations != 0 || !result.visited_terminator ||
      result.emitted_instructions != 0 || !block.instructions.empty() ||
      !block.successors.empty()) {
    return fail("expected non-fusable compare branch to remain unsupported for this slice");
  }
  if (diagnostics.entries.size() != 1 ||
      diagnostics.entries.front().kind !=
          aarch64_module::ModuleLoweringDiagnosticKind::UnsupportedTerminatorFamily ||
      diagnostics.entries.front().function_name != function_cf.function_name ||
      diagnostics.entries.front().block_label != block_cf.block_label) {
    return fail("expected non-fusable compare branch to fail closed with a typed diagnostic");
  }
  return 0;
}

int module_print_labels_late_conditional_successor_after_return() {
  auto prepared = prepared_with_late_conditional_successor_after_return();
  const auto result = aarch64_codegen::compile_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected late conditional successor fixture to build");
  }

  const auto& function_cf = prepared.control_flow.functions.front();
  const auto& entry_cf = function_cf.blocks[0];
  const auto& function = result.module->mir.functions.front();
  if (function.blocks.size() != 4 || function.blocks.front().successors.size() != 2 ||
      function.blocks.front().successors[0].target_label != entry_cf.true_label ||
      function.blocks.front().successors[1].target_label != entry_cf.false_label ||
      function.blocks.front().instructions.empty()) {
    return fail("expected entry conditional branch to keep true/false successors");
  }

  const auto printed =
      mir::print_machine_function(function, aarch64_codegen::MachineInstructionPrinter{});
  if (!printed.ok) {
    return fail("expected late conditional successor fixture to print: " +
                printed.diagnostic);
  }

  const std::string late_label = ".LBB" + std::to_string(function_cf.function_name) +
                                 "_" + std::to_string(entry_cf.true_label) + ":\n";
  const std::string true_branch =
      "b.lt .LBB" + std::to_string(function_cf.function_name) + "_" +
      std::to_string(entry_cf.true_label);
  const auto branch_pos = printed.assembly.find(true_branch);
  const auto return_pos = printed.assembly.find("ret");
  const auto late_label_pos = printed.assembly.find(late_label);
  if (branch_pos == std::string::npos || return_pos == std::string::npos ||
      late_label_pos == std::string::npos || !(return_pos < late_label_pos)) {
    return fail("expected conditional successor after return to stay branched and labeled: " +
                printed.assembly);
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = direct_dispatch_lowers_unconditional_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_unconditional_branch_with_divergent_bir_label_ids();
      status != 0) {
    return status;
  }
  if (const int status =
          module_build_keeps_branch_node_without_restoring_legacy_return_nodes();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_materialized_bool_conditional_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_does_not_reuse_clobbered_bool_register();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_route7_provenance_matches_bir_identity();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_duplicate_route7_provenance_uses_emitted_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_absent_route7_provenance_uses_emitted_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_condition_name_mismatch_uses_emitted_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_invalid_route7_reference_rejected();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_stale_prepared_lookup_uses_bir_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_lhs_provenance_mismatch_uses_emitted_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          materialized_compare_branch_rhs_provenance_mismatch_uses_emitted_fallback();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_fusable_compare_branch_to_selected_node();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_i32_sext_i64_fused_compare_branch_with_legal_widths();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_i32_sext_i64_constant_udiv_bound_branch();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_stack_homed_constant_binary_rhs_as_compare_immediate();
      status != 0) {
    return status;
  }
  if (const int status =
          direct_dispatch_lowers_loop_header_fused_compare_branch_with_divergent_bir_label_ids();
      status != 0) {
    return status;
  }
  if (const int status =
          module_print_labels_late_conditional_successor_after_return();
      status != 0) {
    return status;
  }
  if (const int status = non_fusable_compare_branch_control_stays_fail_closed();
      status != 0) {
    return status;
  }
  return 0;
}

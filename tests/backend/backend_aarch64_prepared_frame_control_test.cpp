#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/module/module.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace {

namespace aarch64_api = c4c::backend::aarch64::api;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int fail(std::string_view message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_frame_control_module() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("frame.control.fn");
  const auto entry_label = prepared.names.block_labels.intern("entry");
  const auto left_label = prepared.names.block_labels.intern("left");
  const auto right_label = prepared.names.block_labels.intern("right");
  const auto join_label = prepared.names.block_labels.intern("join");
  const auto local_slot_name = prepared.names.slot_names.intern("local.slot");
  const auto local_value_name = prepared.names.value_names.intern("local.value");
  const auto cond_name = prepared.names.value_names.intern("cond.value");
  const auto source_name = prepared.names.value_names.intern("source.value");
  const auto destination_name = prepared.names.value_names.intern("destination.value");
  const auto alloca_result_name = prepared.names.value_names.intern("dyn.ptr");
  const auto alloca_count_name = prepared.names.value_names.intern("dyn.count");
  const auto callee_symbol = prepared.names.link_names.intern("callee.identity");

  const auto function_link_name = prepared.module.names.link_names.intern("frame.control.fn");
  const auto entry_bir_label = prepared.module.names.block_labels.intern("entry");
  const auto left_bir_label = prepared.module.names.block_labels.intern("left");
  const auto right_bir_label = prepared.module.names.block_labels.intern("right");
  const auto join_bir_label = prepared.module.names.block_labels.intern("join");

  auto make_block = [](std::string label, c4c::BlockLabelId label_id) {
    bir::Block block;
    block.label = std::move(label);
    block.label_id = label_id;
    return block;
  };

  bir::Block entry = make_block("entry", entry_bir_label);
  entry.terminator = bir::CondBranchTerminator{
      .condition = bir::Value::named(bir::TypeKind::I32, "cond.value"),
      .true_label = "left",
      .false_label = "right",
  };
  bir::Block left = make_block("left", left_bir_label);
  left.terminator =
      bir::BranchTerminator{.target_label = "join", .target_label_id = join_bir_label};
  bir::Block right = make_block("right", right_bir_label);
  right.terminator =
      bir::BranchTerminator{.target_label = "join", .target_label_id = join_bir_label};
  bir::Block join = make_block("join", join_bir_label);
  join.terminator = bir::ReturnTerminator{};

  bir::Function function;
  function.name = "frame.control.raw";
  function.link_name_id = function_link_name;
  function.return_type = bir::TypeKind::Void;
  function.blocks.push_back(std::move(entry));
  function.blocks.push_back(std::move(left));
  function.blocks.push_back(std::move(right));
  function.blocks.push_back(std::move(join));
  prepared.module.functions.push_back(std::move(function));

  prepared.stack_layout.objects.push_back(prepare::PreparedStackObject{
      .object_id = 3,
      .function_name = function_name,
      .slot_name = local_slot_name,
      .value_name = local_value_name,
      .source_kind = "local",
      .type = bir::TypeKind::I64,
      .size_bytes = 8,
      .align_bytes = 8,
      .address_exposed = true,
      .requires_home_slot = true,
      .permanent_home_slot = true,
  });
  prepared.stack_layout.frame_slots.push_back(prepare::PreparedFrameSlot{
      .slot_id = 11,
      .object_id = 3,
      .function_name = function_name,
      .offset_bytes = 32,
      .size_bytes = 8,
      .align_bytes = 8,
      .fixed_location = true,
  });
  prepared.stack_layout.frame_size_bytes = 96;
  prepared.stack_layout.frame_alignment_bytes = 16;

  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {
              prepare::PreparedControlFlowBlock{
                  .block_label = entry_label,
                  .terminator_kind = bir::TerminatorKind::CondBranch,
                  .true_label = left_label,
                  .false_label = right_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = left_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = join_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = right_label,
                  .terminator_kind = bir::TerminatorKind::Branch,
                  .branch_target_label = join_label,
              },
              prepare::PreparedControlFlowBlock{
                  .block_label = join_label,
                  .terminator_kind = bir::TerminatorKind::Return,
              },
          },
      .branch_conditions =
          {
              prepare::PreparedBranchCondition{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .kind = prepare::PreparedBranchConditionKind::FusedCompare,
                  .condition_value = bir::Value::named(bir::TypeKind::I32, "cond.value"),
                  .predicate = bir::BinaryOpcode::Eq,
                  .compare_type = bir::TypeKind::I32,
                  .lhs = bir::Value::named(bir::TypeKind::I32, "source.value"),
                  .rhs = bir::Value::immediate_i32(0),
                  .can_fuse_with_branch = true,
                  .true_label = left_label,
                  .false_label = right_label,
              },
          },
      .parallel_copy_bundles =
          {
              prepare::PreparedParallelCopyBundle{
                  .predecessor_label = left_label,
                  .successor_label = join_label,
                  .execution_site = prepare::PreparedParallelCopyExecutionSite::SuccessorEntry,
                  .execution_block_label = join_label,
                  .moves =
                      {
                          prepare::PreparedParallelCopyMove{
                              .join_transfer_index = 0,
                              .edge_transfer_index = 1,
                              .source_value =
                                  bir::Value::named(bir::TypeKind::I32, "source.value"),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "destination.value"),
                              .carrier_kind =
                                  prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot,
                              .storage_name = local_slot_name,
                          },
                          prepare::PreparedParallelCopyMove{
                              .join_transfer_index = 0,
                              .edge_transfer_index = 2,
                              .source_value =
                                  bir::Value::named(bir::TypeKind::I32, "destination.value"),
                              .destination_value =
                                  bir::Value::named(bir::TypeKind::I32, "source.value"),
                              .carrier_kind =
                                  prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot,
                              .storage_name = local_slot_name,
                          },
                      },
                  .steps =
                      {
                          prepare::PreparedParallelCopyStep{
                              .kind = prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp,
                              .move_index = 0,
                          },
                          prepare::PreparedParallelCopyStep{
                              .kind = prepare::PreparedParallelCopyStepKind::Move,
                              .move_index = 1,
                              .uses_cycle_temp_source = true,
                          },
                      },
                  .has_cycle = true,
              },
          },
  });

  prepared.frame_plan.functions.push_back(prepare::PreparedFramePlanFunction{
      .function_name = function_name,
      .frame_size_bytes = 96,
      .frame_alignment_bytes = 16,
      .saved_callee_registers =
          {
              prepare::PreparedSavedRegister{
                  .bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = "x19",
                  .contiguous_width = 1,
                  .occupied_register_names = {"x19"},
                  .save_index = 2,
              },
          },
      .frame_slot_order = {11},
      .has_dynamic_stack = true,
      .uses_frame_pointer_for_fixed_slots = true,
  });
  prepared.dynamic_stack_plan.functions.push_back(prepare::PreparedDynamicStackPlanFunction{
      .function_name = function_name,
      .requires_stack_save_restore = true,
      .uses_frame_pointer_for_fixed_slots = true,
      .operations =
          {
              prepare::PreparedDynamicStackOp{
                  .function_name = function_name,
                  .block_label = entry_label,
                  .instruction_index = 2,
                  .kind = prepare::PreparedDynamicStackOpKind::DynamicAlloca,
                  .result_value_name = alloca_result_name,
                  .operand_value_name = alloca_count_name,
                  .allocation_type_text = "i64",
                  .element_size_bytes = 8,
                  .element_align_bytes = 8,
              },
          },
  });

  prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
      .function_name = function_name,
      .value_homes =
          {
              prepare::PreparedValueHome{
                  .value_id = 32,
                  .function_name = function_name,
                  .value_name = destination_name,
                  .kind = prepare::PreparedValueHomeKind::StackSlot,
                  .slot_id = 11,
                  .offset_bytes = std::size_t{32},
              },
          },
      .move_bundles =
          {
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BeforeCall,
                  .block_index = 0,
                  .instruction_index = 3,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = 21,
                              .to_value_id = 22,
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::Register,
                              .destination_abi_index = std::size_t{0},
                              .destination_register_name = std::string{"x0"},
                              .destination_contiguous_width = 1,
                              .destination_occupied_register_names = {"x0"},
                              .block_index = 0,
                              .instruction_index = 3,
                              .reason = "call argument",
                          },
                      },
                  .abi_bindings =
                      {
                          prepare::PreparedAbiBinding{
                              .destination_kind =
                                  prepare::PreparedMoveDestinationKind::CallArgumentAbi,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::StackSlot,
                              .destination_abi_index = std::size_t{1},
                              .destination_stack_offset_bytes = std::size_t{16},
                          },
                      },
              },
              prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BlockEntry,
                  .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .block_index = 3,
                  .source_parallel_copy_predecessor_label = left_label,
                  .source_parallel_copy_successor_label = join_label,
                  .moves =
                      {
                          prepare::PreparedMoveResolution{
                              .from_value_id = 31,
                              .to_value_id = 32,
                              .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::StackSlot,
                              .destination_stack_offset_bytes = std::size_t{32},
                              .block_index = 3,
                              .instruction_index = 0,
                              .source_parallel_copy_step_index = std::size_t{0},
                              .op_kind =
                                  prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp,
                              .authority_kind =
                                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                              .source_parallel_copy_predecessor_label = left_label,
                              .source_parallel_copy_successor_label = join_label,
                              .reason = "out-of-ssa",
                          },
                          prepare::PreparedMoveResolution{
                              .from_value_id = 32,
                              .to_value_id = 31,
                              .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                              .destination_storage_kind =
                                  prepare::PreparedMoveStorageKind::StackSlot,
                              .destination_stack_offset_bytes = std::size_t{32},
                              .block_index = 3,
                              .instruction_index = 0,
                              .uses_cycle_temp_source = true,
                              .source_parallel_copy_step_index = std::size_t{1},
                              .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                              .authority_kind =
                                  prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                              .source_parallel_copy_predecessor_label = left_label,
                              .source_parallel_copy_successor_label = join_label,
                              .reason = "out-of-ssa cycle",
                          },
                      },
              },
          },
  });

  prepared.regalloc.functions.push_back(prepare::PreparedRegallocFunction{
      .function_name = function_name,
      .move_resolution =
          {
              prepare::PreparedMoveResolution{
                  .from_value_id = 41,
                  .to_value_id = 42,
                  .destination_kind = prepare::PreparedMoveDestinationKind::FunctionReturnAbi,
                  .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                  .destination_register_name = std::string{"x0"},
                  .destination_contiguous_width = 1,
                  .destination_occupied_register_names = {"x0"},
                  .block_index = 3,
                  .instruction_index = 1,
                  .source_immediate_i32 = std::int64_t{7},
                  .reason = "return abi",
              },
          },
      .spill_reload_ops =
          {
              prepare::PreparedSpillReloadOp{
                  .value_id = 21,
                  .op_kind = prepare::PreparedSpillReloadOpKind::Spill,
                  .block_index = 0,
                  .instruction_index = 4,
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = std::string{"x20"},
                  .contiguous_width = 1,
                  .occupied_register_names = {"x20"},
                  .slot_id = 11,
                  .stack_offset_bytes = std::size_t{32},
              },
              prepare::PreparedSpillReloadOp{
                  .value_id = 21,
                  .op_kind = prepare::PreparedSpillReloadOpKind::Reload,
                  .block_index = 3,
                  .instruction_index = 0,
                  .register_bank = prepare::PreparedRegisterBank::Gpr,
                  .register_name = std::string{"x20"},
                  .contiguous_width = 1,
                  .occupied_register_names = {"x20"},
                  .slot_id = 11,
                  .stack_offset_bytes = std::size_t{32},
              },
          },
  });

  prepared.call_plans.functions.push_back(prepare::PreparedCallPlansFunction{
      .function_name = function_name,
      .calls =
          {
              prepare::PreparedCallPlan{
                  .block_index = 0,
                  .instruction_index = 3,
                  .wrapper_kind = prepare::PreparedCallWrapperKind::DirectExternFixedArity,
                  .is_indirect = false,
                  .direct_callee_name = std::string{"callee.identity"},
                  .arguments =
                      {
                          prepare::PreparedCallArgumentPlan{
                              .instruction_index = 3,
                              .arg_index = 0,
                              .value_bank = prepare::PreparedRegisterBank::Gpr,
                              .source_encoding = prepare::PreparedStorageEncodingKind::SymbolAddress,
                              .source_value_id = prepare::PreparedValueId{21},
                              .source_symbol_name_id = callee_symbol,
                              .destination_register_name = std::string{"x0"},
                              .destination_register_bank = prepare::PreparedRegisterBank::Gpr,
                          },
                      },
                  .result =
                      prepare::PreparedCallResultPlan{
                          .instruction_index = 3,
                          .value_bank = prepare::PreparedRegisterBank::Gpr,
                          .source_storage_kind = prepare::PreparedMoveStorageKind::Register,
                          .destination_storage_kind = prepare::PreparedMoveStorageKind::StackSlot,
                          .destination_value_id = prepare::PreparedValueId{22},
                          .source_register_name = std::string{"x0"},
                          .source_register_bank = prepare::PreparedRegisterBank::Gpr,
                          .destination_slot_id = 11,
                          .destination_stack_offset_bytes = std::size_t{32},
                      },
                  .preserved_values =
                      {
                          prepare::PreparedCallPreservedValue{
                              .value_id = 21,
                              .value_name = source_name,
                              .route = prepare::PreparedCallPreservationRoute::StackSlot,
                              .slot_id = 11,
                              .stack_offset_bytes = std::size_t{32},
                          },
                      },
                  .clobbered_registers =
                      {
                          prepare::PreparedClobberedRegister{
                              .bank = prepare::PreparedRegisterBank::Gpr,
                              .register_name = "x0",
                              .contiguous_width = 1,
                              .occupied_register_names = {"x0"},
                          },
                      },
              },
          },
  });

  (void)cond_name;
  (void)destination_name;
  return prepared;
}

int records_preserve_frame_control_call_and_move_identity() {
  auto prepared = prepared_frame_control_module();

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value() || !result.module.has_value()) {
    return fail("expected prepared frame/control module to build");
  }
  const auto& function = result.module->functions.front();

  if (function.frame.frame_size_bytes != 96 || function.frame.frame_alignment_bytes != 16 ||
      !function.frame.has_dynamic_stack || !function.frame.requires_stack_save_restore ||
      !function.frame.uses_frame_pointer_for_fixed_slots) {
    return fail("expected frame record to preserve prepared frame and dynamic-stack summary");
  }
  if (function.frame.frame_slot_order.size() != 1 || function.frame.frame_slot_order.front() != 11 ||
      function.frame.slots.size() != 1 || function.frame.slots.front().slot_label != "local.slot" ||
      function.frame.slots.front().value_label != "local.value" ||
      !function.frame.slots.front().offset_is_prepared_snapshot ||
      function.frame.slots.front().source_slot != &prepared.stack_layout.frame_slots.front() ||
      function.frame.slots.front().source_object != &prepared.stack_layout.objects.front()) {
    return fail("expected frame-slot record to preserve slot/object identity");
  }
  if (function.frame.callee_saves.size() != 1 ||
      function.frame.callee_saves.front().register_name != "x19" ||
      function.frame.callee_saves.front().save_index != 2) {
    return fail("expected callee-save record to preserve save register identity");
  }
  if (function.frame.dynamic_stack.size() != 1 ||
      function.frame.dynamic_stack.front().kind !=
          prepare::PreparedDynamicStackOpKind::DynamicAlloca ||
      function.frame.dynamic_stack.front().block_label_text != "entry" ||
      function.frame.dynamic_stack.front().result_label != "dyn.ptr" ||
      function.frame.dynamic_stack.front().operand_label != "dyn.count") {
    return fail("expected dynamic-stack record to preserve operation identities");
  }

  if (function.branches.size() != 1 ||
      function.branches.front().condition_kind != prepare::PreparedBranchConditionKind::FusedCompare ||
      function.branches.front().predicate != bir::BinaryOpcode::Eq ||
      !function.branches.front().can_fuse_with_branch ||
      function.branches.front().source_condition !=
          &prepared.control_flow.functions.front().branch_conditions.front()) {
    return fail("expected branch record to preserve prepared compare branch identity");
  }

  if (function.calls.size() != 1 || function.calls.front().direct_callee_name != "callee.identity" ||
      function.calls.front().arguments.size() != 1 || !function.calls.front().result.has_value() ||
      function.calls.front().preserved_values.size() != 1 ||
      function.calls.front().clobbered_registers.size() != 1) {
    return fail("expected call record to preserve direct call plan shape");
  }
  const auto& call = function.calls.front();
  if (call.source_call != &prepared.call_plans.functions.front().calls.front() ||
      call.arguments.front().source_symbol_label != "callee.identity" ||
      call.arguments.front().destination_register != "x0" ||
      call.result->destination_value_id != 22 || call.result->destination_slot_id != 11 ||
      call.preserved_values.front().value_label != "source.value" ||
      call.preserved_values.front().slot_id != 11) {
    return fail("expected call record to preserve structured call argument/result/preservation facts");
  }

  if (function.moves.size() != 4 || function.abi_bindings.size() != 1 ||
      function.spill_reloads.size() != 2 || function.parallel_copies.size() != 1) {
    return fail("expected move, ABI-binding, spill/reload, and parallel-copy records");
  }
  if (function.moves[0].phase != prepare::PreparedMovePhase::BeforeCall ||
      function.moves[0].destination_kind != prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      function.moves[0].destination_register != "x0" ||
      function.moves[0].destination_contiguous_width != 1 ||
      function.moves[0].destination_occupied_registers.size() != 1 ||
      function.moves[0].destination_occupied_registers.front() != "x0" ||
      function.moves[0].source_bundle !=
          &prepared.value_locations.functions.front().move_bundles.front()) {
    return fail("expected value-location move record to preserve call-argument move identity");
  }
  if (function.moves[1].authority_kind !=
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      function.moves[1].source_parallel_copy_step_index != 0 ||
      function.moves[1].destination_slot_id != 11 ||
      function.moves[1].op_kind !=
          prepare::PreparedMoveResolutionOpKind::SaveDestinationToTemp ||
      !function.moves[2].uses_cycle_temp_source ||
      function.moves[2].source_parallel_copy_step_index != 1 ||
      function.parallel_copies.front().execution_block_label !=
          prepared.names.block_labels.intern("join")) {
    return fail("expected parallel-copy move record to preserve out-of-SSA authority");
  }
  const auto& parallel_copy = function.parallel_copies.front();
  if (!parallel_copy.has_cycle || parallel_copy.moves.size() != 2 ||
      parallel_copy.steps.size() != 2 ||
      parallel_copy.moves.front().carrier_kind !=
          prepare::PreparedJoinTransferCarrierKind::EdgeStoreSlot ||
      parallel_copy.moves.front().storage_name !=
          parallel_copy.source_bundle->moves.front().storage_name ||
      parallel_copy.steps.front().kind !=
          prepare::PreparedParallelCopyStepKind::SaveDestinationToTemp ||
      parallel_copy.steps.front().source_move != &parallel_copy.source_bundle->moves.front() ||
      !parallel_copy.steps.front().has_target_move_record ||
      parallel_copy.steps.front().target_move_authority_kind !=
          prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy ||
      parallel_copy.steps.front().target_destination_slot_id != 11 ||
      !parallel_copy.steps.back().uses_cycle_temp_source ||
      parallel_copy.steps.back().source_move != &parallel_copy.source_bundle->moves.back() ||
      !parallel_copy.steps.back().has_target_move_record ||
      parallel_copy.steps.back().source_target_move != function.moves[2].source_move) {
    return fail("expected parallel-copy record to expose per-move and per-step target facts");
  }
  if (function.abi_bindings.front().destination_kind !=
          prepare::PreparedMoveDestinationKind::CallArgumentAbi ||
      function.abi_bindings.front().authority_kind != prepare::PreparedMoveAuthorityKind::None ||
      function.abi_bindings.front().destination_stack_offset_bytes != 16 ||
      !function.abi_bindings.front().destination_stack_offset_is_prepared_snapshot) {
    return fail("expected ABI binding record to preserve stack destination");
  }
  if (function.spill_reloads.front().op_kind != prepare::PreparedSpillReloadOpKind::Spill ||
      function.spill_reloads.back().op_kind != prepare::PreparedSpillReloadOpKind::Reload ||
      function.spill_reloads.front().slot_id != 11 ||
      function.spill_reloads.front().contiguous_width != 1 ||
      function.spill_reloads.front().occupied_registers.size() != 1 ||
      function.spill_reloads.front().occupied_registers.front() != "x20" ||
      !function.spill_reloads.front().stack_offset_is_prepared_snapshot) {
    return fail("expected spill/reload records to preserve frame-slot identity");
  }

  return 0;
}

}  // namespace

int main() {
  if (const int status = records_preserve_frame_control_call_and_move_identity(); status != 0) {
    return status;
  }
  return 0;
}

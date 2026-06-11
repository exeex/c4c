#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch_producers.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/mir/query.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <cstddef>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

namespace aarch64_codegen = c4c::backend::aarch64::codegen;
namespace aarch64_module = c4c::backend::aarch64::module;
namespace bir = c4c::backend::bir;
namespace mir = c4c::backend::mir;
namespace prepare = c4c::backend::prepare;

enum class CurrentJoinRouteShape {
  NormalPredecessor,
  MissingPredecessor,
  NoSource,
  MemorySource,
  AbsentRoute,
};

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

prepare::PreparedBirModule make_current_join_routing_prepared(
    CurrentJoinRouteShape route_shape,
    bool include_prepared_policy) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name =
      prepared.names.function_names.intern("dispatch.current.join.routing");
  const auto pred_label =
      prepared.names.block_labels.intern("dispatch.current.join.routing.pred");
  const auto join_label =
      prepared.names.block_labels.intern("dispatch.current.join.routing.join");
  const auto bir_pred_label =
      prepared.module.names.block_labels.intern("dispatch.current.join.routing.pred");
  const auto bir_join_label =
      prepared.module.names.block_labels.intern("dispatch.current.join.routing.join");
  const auto operand_name =
      prepared.names.value_names.intern("%current.join.routing.operand");
  const auto source_name =
      prepared.names.value_names.intern("%current.join.routing.source");
  const auto destination_name =
      prepared.names.value_names.intern("%current.join.routing.destination");
  const auto missing_source_name =
      prepared.names.value_names.intern("%current.join.routing.missing_source");
  const auto stack_source_name =
      prepared.names.value_names.intern("%current.join.routing.stack_source");
  const auto stack_destination_name =
      prepared.names.value_names.intern("%current.join.routing.stack_destination");

  const bool include_phi = route_shape != CurrentJoinRouteShape::AbsentRoute;
  const bool route_has_named_source =
      route_shape == CurrentJoinRouteShape::NormalPredecessor ||
      route_shape == CurrentJoinRouteShape::MissingPredecessor;
  const bool route_has_missing_source =
      route_shape == CurrentJoinRouteShape::NoSource;
  const bool route_has_memory_source =
      route_shape == CurrentJoinRouteShape::MemorySource;

  std::vector<bir::Inst> join_insts;
  if (include_phi) {
    bir::Value incoming_value =
        bir::Value::named(bir::TypeKind::I32, "%current.join.routing.source");
    if (route_has_missing_source) {
      incoming_value = bir::Value::named(bir::TypeKind::I32,
                                         "%current.join.routing.missing_source");
    } else if (route_has_memory_source) {
      incoming_value = bir::Value::named(bir::TypeKind::I32,
                                         "%current.join.routing.stack_source");
    }
    join_insts.push_back(bir::PhiInst{
        .result = bir::Value::named(bir::TypeKind::I32,
                                    "%current.join.routing.destination"),
        .incomings =
            {bir::PhiIncoming{
                .label = "dispatch.current.join.routing.pred",
                .value = incoming_value,
                .label_id = bir_pred_label,
            }},
    });
  }
  if (route_has_named_source || route_shape == CurrentJoinRouteShape::AbsentRoute) {
    join_insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result =
            bir::Value::named(bir::TypeKind::I32, "%current.join.routing.source"),
        .operand_type = bir::TypeKind::I32,
        .lhs =
            bir::Value::named(bir::TypeKind::I32, "%current.join.routing.operand"),
        .rhs = bir::Value::immediate_i32(1),
    });
  } else if (route_has_memory_source) {
    join_insts.push_back(bir::LoadLocalInst{
        .result = bir::Value::named(bir::TypeKind::I32,
                                    "%current.join.routing.stack_source"),
        .slot_name = "current.join.routing.stack",
        .slot_id = c4c::SlotNameId{901},
        .byte_offset = std::size_t{0},
        .align_bytes = std::size_t{4},
    });
  }
  join_insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result =
          bir::Value::named(bir::TypeKind::I32, "%current.join.routing.operand"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::immediate_i32(2),
      .rhs = bir::Value::immediate_i32(3),
  });

  if (route_has_missing_source) {
    join_insts.push_back(bir::BinaryInst{
        .opcode = bir::BinaryOpcode::Add,
        .result = bir::Value::named(bir::TypeKind::I32,
                                    "%current.join.routing.source"),
        .operand_type = bir::TypeKind::I32,
        .lhs = bir::Value::named(bir::TypeKind::I32,
                                 "%current.join.routing.operand"),
        .rhs = bir::Value::immediate_i32(4),
    });
  }

  bir::Function function{
      .name = "dispatch.current.join.routing",
      .return_type = bir::TypeKind::Void,
  };
  if (route_shape != CurrentJoinRouteShape::MissingPredecessor) {
    function.blocks.push_back(bir::Block{
        .label = "dispatch.current.join.routing.pred",
        .terminator =
            bir::Terminator{bir::BranchTerminator{
                .target_label = "dispatch.current.join.routing.join",
                .target_label_id = bir_join_label,
            }},
        .label_id = bir_pred_label,
    });
  }
  function.blocks.push_back(bir::Block{
      .label = "dispatch.current.join.routing.join",
      .insts = std::move(join_insts),
      .terminator = bir::Terminator{bir::ReturnTerminator{}},
      .label_id = bir_join_label,
  });
  prepared.module.functions.push_back(std::move(function));
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
      .blocks =
          {prepare::PreparedControlFlowBlock{
               .block_label = pred_label,
               .terminator_kind = bir::TerminatorKind::Branch,
               .branch_target_label = join_label,
           },
           prepare::PreparedControlFlowBlock{
               .block_label = join_label,
               .terminator_kind = bir::TerminatorKind::Return,
           }},
      .join_transfers =
          {prepare::PreparedJoinTransfer{
              .function_name = function_name,
              .join_block_label = join_label,
              .result = bir::Value::named(
                  bir::TypeKind::I32, "%current.join.routing.destination"),
              .edge_transfers =
                  {prepare::PreparedEdgeValueTransfer{
                      .predecessor_label = pred_label,
                      .successor_label = join_label,
                      .incoming_value = bir::Value::named(
                          bir::TypeKind::I32, "%current.join.routing.source"),
                      .destination_value = bir::Value::named(
                          bir::TypeKind::I32,
                          "%current.join.routing.destination"),
                  }},
          }},
  });

  if (!include_prepared_policy) {
    return prepared;
  }

  prepared.value_locations.functions.push_back(
      prepare::PreparedValueLocationFunction{
          .function_name = function_name,
          .value_homes =
              {prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{810},
                   .function_name = function_name,
                   .value_name = operand_name,
                   .kind = prepare::PreparedValueHomeKind::Register,
                   .register_name = std::string{"w12"},
               },
               prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{811},
                   .function_name = function_name,
                   .value_name = source_name,
                   .kind = prepare::PreparedValueHomeKind::Register,
                   .register_name = std::string{"w13"},
               },
               prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{812},
                   .function_name = function_name,
                   .value_name = destination_name,
                   .kind = prepare::PreparedValueHomeKind::Register,
                   .register_name = std::string{"w13"},
               },
               prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{813},
                   .function_name = function_name,
                   .value_name = missing_source_name,
                   .kind = prepare::PreparedValueHomeKind::Register,
                   .register_name = std::string{"w14"},
               },
               prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{814},
                   .function_name = function_name,
                   .value_name = stack_source_name,
                   .kind = prepare::PreparedValueHomeKind::StackSlot,
                   .slot_id = prepare::PreparedFrameSlotId{43},
                   .offset_bytes = std::size_t{32},
               },
               prepare::PreparedValueHome{
                   .value_id = prepare::PreparedValueId{815},
                   .function_name = function_name,
                   .value_name = stack_destination_name,
                   .kind = prepare::PreparedValueHomeKind::Register,
                   .register_name = std::string{"w15"},
               }},
          .move_bundles =
              {prepare::PreparedMoveBundle{
                  .function_name = function_name,
                  .phase = prepare::PreparedMovePhase::BlockEntry,
                  .authority_kind =
                      prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                  .block_index = 0,
                  .instruction_index = 0,
                  .source_parallel_copy_predecessor_label = pred_label,
                  .source_parallel_copy_successor_label = join_label,
                  .moves =
                      {prepare::PreparedMoveResolution{
                          .from_value_id = prepare::PreparedValueId{811},
                          .to_value_id = prepare::PreparedValueId{812},
                          .destination_kind =
                              prepare::PreparedMoveDestinationKind::Value,
                          .destination_storage_kind =
                              prepare::PreparedMoveStorageKind::Register,
                          .destination_register_name = std::string{"w13"},
                          .destination_contiguous_width = 1,
                          .destination_occupied_register_names = {"w13"},
                          .block_index = 0,
                          .instruction_index = 0,
                          .source_parallel_copy_step_index = std::size_t{0},
                          .op_kind =
                              prepare::PreparedMoveResolutionOpKind::Move,
                          .authority_kind =
                              prepare::PreparedMoveAuthorityKind::
                                  OutOfSsaParallelCopy,
                          .source_parallel_copy_predecessor_label = pred_label,
                          .source_parallel_copy_successor_label = join_label,
                          .reason = "test_current_block_join_query_routing",
                      },
                      prepare::PreparedMoveResolution{
                          .from_value_id = prepare::PreparedValueId{814},
                          .to_value_id = prepare::PreparedValueId{815},
                          .destination_kind =
                              prepare::PreparedMoveDestinationKind::Value,
                          .destination_storage_kind =
                              prepare::PreparedMoveStorageKind::Register,
                          .destination_register_name = std::string{"w15"},
                          .destination_contiguous_width = 1,
                          .destination_occupied_register_names = {"w15"},
                          .block_index = 0,
                          .instruction_index = 0,
                          .source_parallel_copy_step_index = std::size_t{1},
                          .op_kind =
                              prepare::PreparedMoveResolutionOpKind::Move,
                          .authority_kind =
                              prepare::PreparedMoveAuthorityKind::
                                  OutOfSsaParallelCopy,
                          .source_parallel_copy_predecessor_label = pred_label,
                          .source_parallel_copy_successor_label = join_label,
                          .reason = "test_current_block_join_query_routing_stack",
                      }},
              }},
      });
  return prepared;
}

int verify_current_join_routing(prepare::PreparedBirModule prepared,
                                bool expect_bir_available,
                                bool attach_prepared_policy,
                                std::vector<bool> expected_incoming,
                                std::vector<bool> expected_sources) {
  const auto& function_cf = prepared.control_flow.functions.front();
  auto prepared_lookups =
      prepare::make_prepared_function_lookups(prepared, function_cf);
  auto function_context = aarch64_codegen::make_function_lowering_context(
      prepared, prepared.target_profile, function_cf);
  if (attach_prepared_policy) {
    attach_prepared_function_lookups(function_context, prepared_lookups);
  }
  const auto join_context =
      aarch64_codegen::make_block_lowering_context(function_context,
                                                   function_cf.blocks[1],
                                                   1);
  const auto bir_identity = mir::find_bir_current_block_join_source_identity(
      mir::BirCurrentBlockJoinSourceRequest{
          .successor_block = join_context.bir_block,
          .successor_label_id = function_cf.blocks[1].block_label,
      });
  if ((bir_identity.status == mir::BirCurrentBlockJoinSourceStatus::Available) !=
      expect_bir_available) {
    return fail(expect_bir_available
                    ? "expected Route 5 current-block join identity"
                    : "expected absent Route 5 current-block join identity");
  }

  const auto routing =
      aarch64_codegen::build_current_block_join_prepared_query_routing(
          join_context);
  if (join_context.bir_block == nullptr ||
      join_context.bir_block->insts.size() != expected_incoming.size() ||
      join_context.bir_block->insts.size() != expected_sources.size()) {
    return fail("current-block join routing fixture has unexpected instruction count");
  }
  for (std::size_t instruction_index = 0;
       instruction_index < join_context.bir_block->insts.size();
       ++instruction_index) {
    const auto& inst = join_context.bir_block->insts[instruction_index];
    if (aarch64_codegen::current_block_join_prepared_query_incoming_expression(
            routing, join_context, instruction_index, inst) !=
        expected_incoming[instruction_index]) {
      return fail("current-block join incoming-expression routing bit mismatch");
    }
    if (aarch64_codegen::current_block_join_prepared_query_source(
            routing, join_context, instruction_index, inst) !=
        expected_sources[instruction_index]) {
      return fail("current-block join source routing bit mismatch");
    }
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(
              CurrentJoinRouteShape::NormalPredecessor, false),
          true, false, {false, true, true}, {false, true, false});
      status != 0) {
    return status;
  }
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(
              CurrentJoinRouteShape::NormalPredecessor, true),
          true, true, {false, true, true}, {false, true, false});
      status != 0) {
    return status;
  }
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(
              CurrentJoinRouteShape::MissingPredecessor, true),
          true, true, {false, true, true}, {false, true, false});
      status != 0) {
    return status;
  }
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(CurrentJoinRouteShape::NoSource,
                                             true),
          false, true, {false, true, true}, {false, false, true});
      status != 0) {
    return status;
  }
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(
              CurrentJoinRouteShape::MemorySource, true),
          true, true, {false, true, false}, {false, true, false});
      status != 0) {
    return status;
  }
  if (const int status = verify_current_join_routing(
          make_current_join_routing_prepared(CurrentJoinRouteShape::AbsentRoute,
                                             true),
          false, true, {true, true}, {true, false});
      status != 0) {
    return status;
  }
  return 0;
}

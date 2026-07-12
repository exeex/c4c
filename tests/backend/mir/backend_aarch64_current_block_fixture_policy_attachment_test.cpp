#include "src/backend/bir/bir.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch.hpp"
#include "src/backend/mir/aarch64/codegen/dispatch_producers.hpp"
#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <memory>

namespace codegen = c4c::backend::aarch64::codegen;
namespace bir = c4c::backend::bir;
namespace module = c4c::backend::aarch64::module;
namespace prepare = c4c::backend::prepare;

struct FixtureAxes {
  prepare::PreparedBirModule prepared;
  std::shared_ptr<const prepare::PreparedFunctionLookups> policy;
  module::FunctionLoweringContext function_context;
};

prepare::PreparedBirModule make_prepared(bool policy_present) {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  prepared.module.target_triple = prepared.target_profile.triple;

  const auto function_name = prepared.names.function_names.intern("fixture.policy");
  const auto pred_label = prepared.names.block_labels.intern("fixture.policy.pred");
  const auto join_label = prepared.names.block_labels.intern("fixture.policy.join");
  const auto source_name = prepared.names.value_names.intern("%fixture.source");
  const auto destination_name =
      prepared.names.value_names.intern("%fixture.destination");
  const auto operand_name = prepared.names.value_names.intern("%fixture.operand");
  const auto bir_pred_label =
      prepared.module.names.block_labels.intern("fixture.policy.pred");
  const auto bir_join_label =
      prepared.module.names.block_labels.intern("fixture.policy.join");

  bir::Function function{.name = "fixture.policy", .return_type = bir::TypeKind::Void};
  function.blocks.push_back(bir::Block{
      .label = "fixture.policy.pred",
      .terminator = bir::Terminator{bir::BranchTerminator{
          .target_label = "fixture.policy.join", .target_label_id = bir_join_label}},
      .label_id = bir_pred_label,
  });
  function.blocks.push_back(bir::Block{
      .label = "fixture.policy.join",
      .insts =
          {bir::PhiInst{
               .result = bir::Value::named(bir::TypeKind::I32,
                                           "%fixture.destination"),
               .incomings = {bir::PhiIncoming{
                   .label = "fixture.policy.pred",
                   .value = bir::Value::named(bir::TypeKind::I32,
                                              "%fixture.source"),
                   .label_id = bir_pred_label,
               }},
           },
           bir::BinaryInst{
               .opcode = bir::BinaryOpcode::Add,
               .result = bir::Value::named(bir::TypeKind::I32, "%fixture.source"),
               .operand_type = bir::TypeKind::I32,
               .lhs = bir::Value::named(bir::TypeKind::I32, "%fixture.operand"),
               .rhs = bir::Value::immediate_i32(1),
           },
           bir::BinaryInst{
               .opcode = bir::BinaryOpcode::Add,
               .result = bir::Value::named(bir::TypeKind::I32, "%fixture.operand"),
               .operand_type = bir::TypeKind::I32,
               .lhs = bir::Value::immediate_i32(2),
               .rhs = bir::Value::immediate_i32(3),
           }},
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
      .join_transfers = {prepare::PreparedJoinTransfer{
          .function_name = function_name,
          .join_block_label = join_label,
          .result = bir::Value::named(bir::TypeKind::I32, "%fixture.destination"),
          .edge_transfers = {prepare::PreparedEdgeValueTransfer{
              .predecessor_label = pred_label,
              .successor_label = join_label,
              .incoming_value =
                  bir::Value::named(bir::TypeKind::I32, "%fixture.source"),
              .destination_value =
                  bir::Value::named(bir::TypeKind::I32, "%fixture.destination"),
          }},
      }},
  });

  if (policy_present) {
    prepared.value_locations.functions.push_back(prepare::PreparedValueLocationFunction{
        .function_name = function_name,
        .value_homes =
            {prepare::PreparedValueHome{
                 .value_id = prepare::PreparedValueId{1},
                 .function_name = function_name,
                 .value_name = operand_name,
                 .kind = prepare::PreparedValueHomeKind::Register,
                 .register_name = std::string{"w10"},
             },
             prepare::PreparedValueHome{
                 .value_id = prepare::PreparedValueId{2},
                 .function_name = function_name,
                 .value_name = source_name,
                 .kind = prepare::PreparedValueHomeKind::Register,
                 .register_name = std::string{"w11"},
             },
             prepare::PreparedValueHome{
                 .value_id = prepare::PreparedValueId{3},
                 .function_name = function_name,
                 .value_name = destination_name,
                 .kind = prepare::PreparedValueHomeKind::Register,
                 .register_name = std::string{"w11"},
             }},
        .move_bundles = {prepare::PreparedMoveBundle{
            .function_name = function_name,
            .phase = prepare::PreparedMovePhase::BlockEntry,
            .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            .block_index = 0,
            .instruction_index = 0,
            .source_parallel_copy_predecessor_label = pred_label,
            .source_parallel_copy_successor_label = join_label,
            .moves = {prepare::PreparedMoveResolution{
                .from_value_id = prepare::PreparedValueId{2},
                .to_value_id = prepare::PreparedValueId{3},
                .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                .destination_register_name = std::string{"w11"},
                .destination_contiguous_width = 1,
                .destination_occupied_register_names = {"w11"},
                .block_index = 0,
                .instruction_index = 0,
                .source_parallel_copy_step_index = std::size_t{0},
                .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                .authority_kind =
                    prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
                .source_parallel_copy_predecessor_label = pred_label,
                .source_parallel_copy_successor_label = join_label,
                .reason = "fixture_policy_attachment_complete_edge_authority",
            }},
        }},
    });
  }
  return prepared;
}

FixtureAxes make_fixture(bool policy_present, bool attachment_present) {
  FixtureAxes fixture;
  fixture.prepared = make_prepared(policy_present);
  const auto& function = fixture.prepared.control_flow.functions.front();
  fixture.function_context = codegen::make_function_lowering_context(
      fixture.prepared, fixture.prepared.target_profile, function);
  fixture.function_context.prepared_lookups_owner.reset();
  fixture.function_context.prepared_lookups = nullptr;
  if (policy_present) {
    fixture.policy = std::make_shared<prepare::PreparedFunctionLookups>(
        prepare::make_prepared_function_lookups(fixture.prepared, function));
    if (attachment_present) {
      fixture.function_context.prepared_lookups_owner = fixture.policy;
      fixture.function_context.prepared_lookups = fixture.policy.get();
    } else {
      // Keep the independently prepared policy alive, but detach every lookup
      // path that could reconstruct policy from the prepared module.
      fixture.function_context.value_locations = nullptr;
      fixture.function_context.value_home_lookups = nullptr;
    }
  }
  return fixture;
}

bool source_is_authorized(FixtureAxes& fixture) {
  const auto& function = fixture.prepared.control_flow.functions.front();
  const auto block = codegen::make_block_lowering_context(
      fixture.function_context, function.blocks[1], 1);
  return codegen::current_block_join_prepared_query_source(
      block, block.bir_block->insts[1]);
}

int main() {
  auto supported = make_fixture(true, true);
  if (supported.policy == nullptr ||
      supported.function_context.prepared_lookups != supported.policy.get() ||
      !source_is_authorized(supported)) {
    return 1;
  }

  auto policy_absent = make_fixture(false, true);
  if (policy_absent.policy != nullptr ||
      policy_absent.function_context.prepared_lookups != nullptr ||
      source_is_authorized(policy_absent)) {
    return 2;
  }

  auto detached = make_fixture(true, false);
  if (detached.policy == nullptr ||
      detached.function_context.prepared_lookups != nullptr ||
      source_is_authorized(detached)) {
    return 3;
  }
  return 0;
}

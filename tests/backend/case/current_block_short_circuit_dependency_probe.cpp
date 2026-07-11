#include "src/backend/prealloc/prepared_lookups.hpp"

#include <algorithm>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

struct Fixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name = names.function_names.intern("short.probe");
  c4c::BlockLabelId predecessor = names.block_labels.intern("short.pred");
  c4c::BlockLabelId successor = names.block_labels.intern("short.join");
  c4c::ValueNameId selected_name = names.value_names.intern("%short.selected");
  c4c::ValueNameId add_name = names.value_names.intern("%rhs.add");
  c4c::ValueNameId leaf_name = names.value_names.intern("%rhs.leaf");
  c4c::ValueNameId destination_name = names.value_names.intern("%short.destination");
  prepare::PreparedControlFlowFunction control_flow;
  prepare::PreparedValueLocationFunction locations;
  bir::Block block;

  Fixture() {
    block.label = "short.join";
    block.label_id = successor;
    block.insts = {
        bir::PhiInst{
            .result = bir::Value::named(bir::TypeKind::I32,
                                        "%short.destination"),
            .incomings = {bir::PhiIncoming{
                .label = "short.pred",
                .value = bir::Value::named(bir::TypeKind::I32,
                                           "%short.selected"),
                .label_id = predecessor,
            }},
        },
        bir::SelectInst{
            .result = bir::Value::named(bir::TypeKind::I32, "%short.selected"),
            .compare_type = bir::TypeKind::I32,
            .lhs = bir::Value::immediate_i32(1),
            .rhs = bir::Value::immediate_i32(0),
            .true_value = bir::Value::named(bir::TypeKind::I32, "%rhs.add"),
            .false_value = bir::Value::immediate_i32(0),
        },
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I32, "%rhs.add"),
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::named(bir::TypeKind::I32, "%rhs.leaf"),
            .rhs = bir::Value::immediate_i32(1),
        },
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I32, "%rhs.leaf"),
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::immediate_i32(2),
            .rhs = bir::Value::immediate_i32(3),
        },
    };
    control_flow = prepare::PreparedControlFlowFunction{
        .function_name = function_name,
        .join_transfers = {prepare::PreparedJoinTransfer{
            .function_name = function_name,
            .join_block_label = successor,
            .result = bir::Value::named(bir::TypeKind::I32,
                                        "%short.destination"),
            .kind = prepare::PreparedJoinTransferKind::PhiEdge,
            .edge_transfers = {prepare::PreparedEdgeValueTransfer{
                .predecessor_label = predecessor,
                .successor_label = successor,
                .incoming_value = bir::Value::named(bir::TypeKind::I32,
                                                     "%short.selected"),
                .destination_value = bir::Value::named(
                    bir::TypeKind::I32, "%short.destination"),
            }},
        }},
        .parallel_copy_bundles = {prepare::PreparedParallelCopyBundle{
            .predecessor_label = predecessor,
            .successor_label = successor,
            .steps = {prepare::PreparedParallelCopyStep{
                .kind = prepare::PreparedParallelCopyStepKind::Move,
                .move_index = 0,
            }},
        }},
    };
    locations = prepare::PreparedValueLocationFunction{
        .function_name = function_name,
        .value_homes = {
            home(prepare::PreparedValueId{41}, selected_name, "w10"),
            home(prepare::PreparedValueId{42}, add_name, "w11"),
            home(prepare::PreparedValueId{43}, leaf_name, "w12"),
            home(prepare::PreparedValueId{31}, destination_name, "w10"),
        },
        .move_bundles = {prepare::PreparedMoveBundle{
            .function_name = function_name,
            .phase = prepare::PreparedMovePhase::BlockEntry,
            .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            .source_parallel_copy_predecessor_label = predecessor,
            .source_parallel_copy_successor_label = successor,
            .moves = {prepare::PreparedMoveResolution{
                .from_value_id = prepare::PreparedValueId{41},
                .to_value_id = prepare::PreparedValueId{31},
                .destination_kind = prepare::PreparedMoveDestinationKind::Value,
                .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
                .destination_register_name = std::string{"w10"},
                .source_parallel_copy_step_index = std::size_t{0},
                .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
                .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            }},
        }},
    };
  }

  prepare::PreparedValueHome home(prepare::PreparedValueId id,
                                  c4c::ValueNameId name,
                                  const char* reg) const {
    return {.value_id = id,
            .function_name = function_name,
            .value_name = name,
            .kind = prepare::PreparedValueHomeKind::Register,
            .register_name = std::string{reg}};
  }
};

prepare::PreparedCurrentBlockJoinParallelCopySourceFacts query(Fixture& fixture) {
  const auto homes = prepare::make_prepared_value_home_lookups(&fixture.locations);
  const auto publications = prepare::make_prepared_edge_publication_lookups(
      fixture.names, fixture.control_flow, &fixture.locations, &homes);
  return prepare::prepare_current_block_join_parallel_copy_source_facts({
      .names = &fixture.names,
      .value_locations = &fixture.locations,
      .value_home_lookups = &homes,
      .edge_publications = &publications,
      .control_flow = &fixture.control_flow,
      .join_source_evidence = {prepare::PreparedFactBoundaryEvidence{
          .status = prepare::PreparedFactBoundaryStatus::Available,
          .function_name = fixture.function_name,
          .block_label = fixture.successor,
          .value_name = fixture.selected_name,
          .instruction_index = 1,
      }},
      .block = &fixture.block,
      .successor_label = fixture.successor,
  });
}

bool authoritative(
    const prepare::PreparedCurrentBlockJoinParallelCopySourceFacts& facts,
    const Fixture& fixture,
    prepare::PreparedValueId value_id,
    c4c::ValueNameId value_name) {
  return static_cast<bool>(
      prepare::query_prepared_current_block_join_routing_consumption(
          facts.routing_facts,
          fixture.successor,
          value_id,
          value_name,
          prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression));
}

bool contains(const std::vector<c4c::ValueNameId>& names, c4c::ValueNameId name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

}  // namespace

int main() {
  Fixture fixture;
  const auto composed = query(fixture);
  if (!contains(composed.incoming_expression_value_names, fixture.selected_name) ||
      !contains(composed.incoming_expression_value_names, fixture.add_name) ||
      !contains(composed.incoming_expression_value_names, fixture.leaf_name) ||
      !authoritative(composed, fixture, prepare::PreparedValueId{41},
                     fixture.selected_name) ||
      !authoritative(composed, fixture, prepare::PreparedValueId{42},
                     fixture.add_name) ||
      !authoritative(composed, fixture, prepare::PreparedValueId{43},
                     fixture.leaf_name)) {
    return 1;
  }

  // The add dependency is not authoritative when its producer is absent.
  fixture.block.insts.erase(fixture.block.insts.begin() + 2);
  const auto missing_add = query(fixture);
  if (contains(missing_add.incoming_expression_value_names, fixture.add_name) ||
      contains(missing_add.incoming_expression_value_names, fixture.leaf_name) ||
      authoritative(missing_add, fixture, prepare::PreparedValueId{42},
                    fixture.add_name) ||
      authoritative(missing_add, fixture, prepare::PreparedValueId{43},
                    fixture.leaf_name)) {
    return 2;
  }

  Fixture mismatched_destination;
  auto& phi = std::get<bir::PhiInst>(mismatched_destination.block.insts.front());
  phi.result = bir::Value::named(bir::TypeKind::I32, "%other.destination");
  mismatched_destination.control_flow.join_transfers.front().result =
      bir::Value::named(bir::TypeKind::I32, "%other.transfer");
  const auto mismatched = query(mismatched_destination);
  if (authoritative(mismatched,
                    mismatched_destination,
                    prepare::PreparedValueId{42},
                    mismatched_destination.add_name)) {
    return 3;
  }

  Fixture incomplete_transfer;
  incomplete_transfer.block.insts.erase(incomplete_transfer.block.insts.begin());
  incomplete_transfer.control_flow.join_transfers.front().edge_transfers.clear();
  const auto incomplete = query(incomplete_transfer);
  if (authoritative(incomplete,
                    incomplete_transfer,
                    prepare::PreparedValueId{42},
                    incomplete_transfer.add_name)) {
    return 4;
  }
  return 0;
}

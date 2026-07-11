#include "src/backend/prealloc/prepared_lookups.hpp"

#include <algorithm>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

struct Fixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name;
  c4c::BlockLabelId predecessor_label;
  c4c::BlockLabelId successor_label;
  c4c::ValueNameId source_name;
  c4c::ValueNameId middle_name;
  c4c::ValueNameId leaf_name;
  c4c::ValueNameId destination_name;
  prepare::PreparedControlFlowFunction control_flow;
  prepare::PreparedValueLocationFunction locations;
  bir::Block block;

  Fixture()
      : function_name(names.function_names.intern("closure.probe")),
        predecessor_label(names.block_labels.intern("closure.pred")),
        successor_label(names.block_labels.intern("closure.join")),
        source_name(names.value_names.intern("%closure.source")),
        middle_name(names.value_names.intern("%closure.middle")),
        leaf_name(names.value_names.intern("%closure.leaf")),
        destination_name(names.value_names.intern("%closure.destination")) {
    block.label = "closure.join";
    block.label_id = successor_label;
    block.insts = {
        bir::PhiInst{
            .result = bir::Value::named(bir::TypeKind::I32,
                                        "%closure.destination"),
            .incomings = {bir::PhiIncoming{
                .label = "closure.pred",
                .value = bir::Value::named(bir::TypeKind::I32,
                                           "%closure.source"),
                .label_id = predecessor_label,
            }},
        },
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I32, "%closure.source"),
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::named(bir::TypeKind::I32, "%closure.middle"),
            .rhs = bir::Value::immediate_i32(1),
        },
        bir::CastInst{
            .opcode = bir::CastOpcode::Bitcast,
            .result = bir::Value::named(bir::TypeKind::I32, "%closure.middle"),
            .operand = bir::Value::named(bir::TypeKind::I32, "%closure.leaf"),
        },
        bir::BinaryInst{
            .opcode = bir::BinaryOpcode::Add,
            .result = bir::Value::named(bir::TypeKind::I32, "%closure.leaf"),
            .operand_type = bir::TypeKind::I32,
            .lhs = bir::Value::immediate_i32(2),
            .rhs = bir::Value::immediate_i32(3),
        },
    };
    control_flow = prepare::PreparedControlFlowFunction{
        .function_name = function_name,
        .join_transfers = {prepare::PreparedJoinTransfer{
            .function_name = function_name,
            .join_block_label = successor_label,
            .kind = prepare::PreparedJoinTransferKind::PhiEdge,
            .edge_transfers = {prepare::PreparedEdgeValueTransfer{
                .predecessor_label = predecessor_label,
                .successor_label = successor_label,
                .incoming_value = bir::Value::named(bir::TypeKind::I32,
                                                     "%closure.source"),
                .destination_value = bir::Value::named(
                    bir::TypeKind::I32, "%closure.destination"),
            }},
        }},
        .parallel_copy_bundles = {prepare::PreparedParallelCopyBundle{
            .predecessor_label = predecessor_label,
            .successor_label = successor_label,
            .steps = {prepare::PreparedParallelCopyStep{
                .kind = prepare::PreparedParallelCopyStepKind::Move,
                .move_index = 0,
            }},
        }},
    };
    locations = prepare::PreparedValueLocationFunction{
        .function_name = function_name,
        .value_homes = {
            prepare::PreparedValueHome{
                .value_id = prepare::PreparedValueId{41},
                .function_name = function_name,
                .value_name = source_name,
                .kind = prepare::PreparedValueHomeKind::Register,
                .register_name = std::string{"w10"},
            },
            prepare::PreparedValueHome{
                .value_id = prepare::PreparedValueId{42},
                .function_name = function_name,
                .value_name = middle_name,
                .kind = prepare::PreparedValueHomeKind::Register,
                .register_name = std::string{"w11"},
            },
            prepare::PreparedValueHome{
                .value_id = prepare::PreparedValueId{43},
                .function_name = function_name,
                .value_name = leaf_name,
                .kind = prepare::PreparedValueHomeKind::Register,
                .register_name = std::string{"w12"},
            },
            prepare::PreparedValueHome{
                .value_id = prepare::PreparedValueId{31},
                .function_name = function_name,
                .value_name = destination_name,
                .kind = prepare::PreparedValueHomeKind::Register,
                .register_name = std::string{"w10"},
            },
        },
        .move_bundles = {prepare::PreparedMoveBundle{
            .function_name = function_name,
            .phase = prepare::PreparedMovePhase::BlockEntry,
            .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
            .source_parallel_copy_predecessor_label = predecessor_label,
            .source_parallel_copy_successor_label = successor_label,
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
          .block_label = fixture.successor_label,
          .value_name = fixture.source_name,
          .instruction_index = 1,
      }},
      .block = &fixture.block,
      .successor_label = fixture.successor_label,
  });
}

bool contains_once(const std::vector<c4c::ValueNameId>& names,
                   c4c::ValueNameId name) {
  return std::count(names.begin(), names.end(), name) == 1;
}

}  // namespace

int main() {
  Fixture fixture;
  const auto transitive = query(fixture);
  if (!contains_once(transitive.incoming_expression_value_names,
                     fixture.source_name) ||
      !contains_once(transitive.incoming_expression_value_names,
                     fixture.middle_name) ||
      !contains_once(transitive.incoming_expression_value_names,
                     fixture.leaf_name)) {
    return 1;
  }

  // A cycle must terminate and must not duplicate closure members.
  fixture.block.insts[3] = bir::CastInst{
      .opcode = bir::CastOpcode::Bitcast,
      .result = bir::Value::named(bir::TypeKind::I32, "%closure.leaf"),
      .operand = bir::Value::named(bir::TypeKind::I32, "%closure.source"),
  };
  const auto cyclic = query(fixture);
  if (!contains_once(cyclic.incoming_expression_value_names,
                     fixture.source_name) ||
      !contains_once(cyclic.incoming_expression_value_names,
                     fixture.middle_name) ||
      !contains_once(cyclic.incoming_expression_value_names,
                     fixture.leaf_name)) {
    return 2;
  }

  // Missing producer evidence must fail closed rather than authorize a leaf
  // merely because a value-home record exists.
  fixture.block.insts.pop_back();
  const auto missing_edge = query(fixture);
  if (std::find(missing_edge.incoming_expression_value_names.begin(),
                missing_edge.incoming_expression_value_names.end(),
                fixture.leaf_name) !=
      missing_edge.incoming_expression_value_names.end()) {
    return 3;
  }

  // Conflicting producers for one result must fail closed rather than select
  // the first instruction encountered.
  fixture.block.insts.push_back(bir::BinaryInst{
      .opcode = bir::BinaryOpcode::Add,
      .result = bir::Value::named(bir::TypeKind::I32, "%closure.middle"),
      .operand_type = bir::TypeKind::I32,
      .lhs = bir::Value::named(bir::TypeKind::I32, "%closure.leaf"),
      .rhs = bir::Value::immediate_i32(9),
  });
  const auto conflicting = query(fixture);
  if (std::find(conflicting.incoming_expression_value_names.begin(),
                conflicting.incoming_expression_value_names.end(),
                fixture.middle_name) !=
      conflicting.incoming_expression_value_names.end()) {
    return 4;
  }
  return 0;
}

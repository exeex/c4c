#include "src/backend/prealloc/publication_plans.hpp"

#include <vector>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

using Role = prepare::PreparedCurrentBlockJoinRoutingRole;
using Status = prepare::PreparedFactBoundaryStatus;
using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;
using RoutingFact = prepare::PreparedCurrentBlockJoinRoutingFact;

struct Fixture {
  prepare::PreparedNameTables names;
  c4c::FunctionNameId function_name;
  c4c::BlockLabelId predecessor;
  c4c::BlockLabelId successor;
  c4c::ValueNameId source_name;
  c4c::ValueNameId destination_name;
  prepare::PreparedValueId source_id{11};
  prepare::PreparedValueId destination_id{12};
  prepare::PreparedValueLocationFunction locations;
  prepare::PreparedValueHomeLookups homes;
  prepare::PreparedEdgePublicationLookups publications;
  prepare::PreparedControlFlowFunction control_flow;
  bir::Block block;

  Fixture()
      : function_name(names.function_names.intern("phi.root.probe")),
        predecessor(names.block_labels.intern("phi.root.pred")),
        successor(names.block_labels.intern("phi.root.join")),
        source_name(names.value_names.intern("%phi.root.source")),
        destination_name(names.value_names.intern("%phi.root.destination")),
        locations{.function_name = function_name,
                  .value_homes = {
                      prepare::PreparedValueHome{
                          .value_id = source_id,
                          .function_name = function_name,
                          .value_name = source_name,
                          .kind = prepare::PreparedValueHomeKind::StackSlot,
                          .slot_id = prepare::PreparedFrameSlotId{1},
                          .offset_bytes = std::size_t{0},
                      },
                      prepare::PreparedValueHome{
                          .value_id = destination_id,
                          .function_name = function_name,
                          .value_name = destination_name,
                          .kind = prepare::PreparedValueHomeKind::Register,
                          .register_name = std::string{"w9"},
                      },
                  }},
        homes(prepare::make_prepared_value_home_lookups(&locations)),
        control_flow{.function_name = function_name},
        block{.label = "phi.root.join",
              .insts = {bir::PhiInst{
                  .result = bir::Value::named(bir::TypeKind::I32,
                                              "%phi.root.destination"),
                  .incomings = {bir::PhiIncoming{
                      .label = "phi.root.pred",
                      .value = bir::Value::named(bir::TypeKind::I32,
                                                "%phi.root.source"),
                      .label_id = predecessor,
                  }},
              }},
              .label_id = successor} {}

  prepare::PreparedCurrentBlockJoinParallelCopySourceFacts run() const {
    return prepare::prepare_current_block_join_parallel_copy_source_facts(
        prepare::PreparedCurrentBlockJoinParallelCopySourceQueryInputs{
            .names = &names,
            .value_locations = &locations,
            .value_home_lookups = &homes,
            .edge_publications = &publications,
            .control_flow = &control_flow,
            .block = &block,
            .successor_label = successor,
        });
  }
};

Status query(const std::vector<RoutingFact>& facts,
             c4c::BlockLabelId successor,
             prepare::PreparedValueId id,
             c4c::ValueNameId name,
             Role role) {
  return prepare::query_prepared_current_block_join_routing_consumption(
             facts, successor, id, name, role)
      .status;
}

}  // namespace

int main() {
  Fixture fixture;
  auto prepared = fixture.run();
  if (query(prepared.routing_facts, fixture.successor, fixture.source_id,
            fixture.source_name, Role::IncomingExpression) != Status::Available) {
    return 1;
  }

  auto missing_predecessor = fixture;
  std::get<bir::PhiInst>(missing_predecessor.block.insts.front())
      .incomings.front()
      .label = "phi.root.missing";
  if (query(missing_predecessor.run().routing_facts, fixture.successor,
            fixture.source_id, fixture.source_name,
            Role::IncomingExpression) != Status::Missing) {
    return 2;
  }

  auto duplicate = fixture;
  duplicate.block.insts.push_back(duplicate.block.insts.front());
  if (query(duplicate.run().routing_facts, fixture.successor, fixture.source_id,
            fixture.source_name, Role::IncomingExpression) !=
      Status::Ambiguous) {
    return 3;
  }

  auto parallel = fixture;
  const auto parallel_destination_name =
      parallel.names.value_names.intern("%phi.root.parallel_destination");
  const prepare::PreparedValueId parallel_destination_id{15};
  parallel.locations.value_homes.push_back(prepare::PreparedValueHome{
      .value_id = parallel_destination_id,
      .function_name = parallel.function_name,
      .value_name = parallel_destination_name,
      .kind = prepare::PreparedValueHomeKind::Register,
      .register_name = std::string{"w10"},
  });
  parallel.homes =
      prepare::make_prepared_value_home_lookups(&parallel.locations);
  parallel.block.insts.push_back(bir::PhiInst{
      .result = bir::Value::named(bir::TypeKind::I32,
                                  "%phi.root.parallel_destination"),
      .incomings = {bir::PhiIncoming{
          .label = "phi.root.pred",
          .value = bir::Value::named(bir::TypeKind::I32,
                                     "%phi.root.source"),
          .label_id = parallel.predecessor,
      }},
  });
  if (query(parallel.run().routing_facts, parallel.successor,
            parallel.source_id, parallel.source_name,
            Role::IncomingExpression) != Status::Ambiguous) {
    return 4;
  }

  const RoutingFact source_a{
      .status = Status::Available,
      .predecessor_label = fixture.predecessor,
      .successor_label = fixture.successor,
      .destination_value_id = fixture.destination_id,
      .destination_value_name = fixture.destination_name,
      .source_value_id = fixture.source_id,
      .source_value_name = fixture.source_name,
      .routed_value_id = fixture.destination_id,
      .routed_value_name = fixture.destination_name,
      .role = Role::Source,
      .publication_semantic_origin = Origin::BirPhi,
  };
  auto source_b = source_a;
  source_b.predecessor_label = fixture.names.block_labels.intern("phi.root.pred2");
  source_b.source_value_id = prepare::PreparedValueId{13};
  source_b.source_value_name = fixture.names.value_names.intern("%phi.root.source2");
  if (query({source_a, source_b}, fixture.successor, fixture.destination_id,
            fixture.destination_name, Role::Source) != Status::Available) {
    return 5;
  }
  auto different_destination = source_b;
  different_destination.destination_value_id = prepare::PreparedValueId{14};
  different_destination.destination_value_name =
      fixture.names.value_names.intern("%phi.root.destination2");
  auto different_origin = source_b;
  different_origin.publication_semantic_origin = Origin::PreparedJoinTransfer;
  if (query({source_a, different_destination}, fixture.successor,
            fixture.destination_id, fixture.destination_name, Role::Source) !=
          Status::Ambiguous ||
      query({source_a, different_origin}, fixture.successor,
            fixture.destination_id, fixture.destination_name, Role::Source) !=
          Status::Ambiguous ||
      query({source_a, source_a}, fixture.successor, fixture.destination_id,
            fixture.destination_name, Role::Source) != Status::Ambiguous) {
    return 6;
  }

  RoutingFact incomplete = source_a;
  incomplete.status = Status::Incomplete;
  incomplete.role = Role::IncomingExpression;
  incomplete.routed_value_id = fixture.source_id;
  incomplete.routed_value_name = fixture.source_name;
  if (query({incomplete}, fixture.successor, fixture.source_id,
            fixture.source_name, Role::IncomingExpression) !=
      Status::Incomplete) {
    return 7;
  }

  Fixture negative;
  negative.locations.move_bundles.push_back(prepare::PreparedMoveBundle{
      .function_name = negative.function_name,
      .phase = prepare::PreparedMovePhase::BlockEntry,
      .authority_kind = prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      .source_parallel_copy_predecessor_label = negative.predecessor,
      .source_parallel_copy_successor_label = negative.successor,
      .moves = {prepare::PreparedMoveResolution{
          .from_value_id = negative.source_id,
          .to_value_id = negative.destination_id,
          .destination_kind = prepare::PreparedMoveDestinationKind::Value,
          .destination_storage_kind = prepare::PreparedMoveStorageKind::Register,
          .destination_register_name = std::string{"w9"},
          .op_kind = prepare::PreparedMoveResolutionOpKind::Move,
          .authority_kind =
              prepare::PreparedMoveAuthorityKind::OutOfSsaParallelCopy,
      }},
  });
  negative.homes =
      prepare::make_prepared_value_home_lookups(&negative.locations);
  prepare::PreparedEdgePublication negative_publication{
      .status = prepare::PreparedEdgePublicationLookupStatus::Available,
      .predecessor_label = negative.predecessor,
      .successor_label = negative.successor,
      .destination_value = bir::Value::named(bir::TypeKind::I32,
                                             "%phi.root.destination"),
      .source_value = bir::Value::named(bir::TypeKind::I32,
                                        "%phi.root.source"),
      .destination_value_id = negative.destination_id,
      .destination_value_name = negative.destination_name,
      .source_value_id = negative.source_id,
      .source_value_name = negative.source_name,
      .source_value_kind = bir::Value::Kind::Named,
      .source_home = &negative.locations.value_homes[0],
      .source_home_kind = prepare::PreparedValueHomeKind::StackSlot,
      .destination_home = &negative.locations.value_homes[1],
      .destination_home_kind = prepare::PreparedValueHomeKind::Register,
  };
  negative.publications.publications_by_edge_destination.emplace(
      prepare::prepared_edge_publication_key(
          negative.predecessor, negative.successor, negative.destination_id),
      std::vector<const prepare::PreparedEdgePublication*>{
          &negative_publication});
  const auto negative_prepared = negative.run();
  if (query(negative_prepared.routing_facts,
            negative.successor,
            negative.source_id,
            negative.source_name,
            Role::IncomingExpression) != Status::Incomplete) {
    return 8;
  }
  return 0;
}

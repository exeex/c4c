#include "src/backend/prealloc/prepared_lookups.hpp"

#include <vector>

namespace prepare = c4c::backend::prepare;

namespace {

using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

prepare::PreparedCurrentBlockJoinRoutingFact fact(
    c4c::BlockLabelId predecessor = c4c::BlockLabelId{1}) {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = predecessor,
      .successor_label = c4c::BlockLabelId{3},
      .destination_value_id = prepare::PreparedValueId{10},
      .destination_value_name = c4c::ValueNameId{10},
      .source_value_id = prepare::PreparedValueId{20},
      .source_value_name = c4c::ValueNameId{20},
      .routed_value_id = prepare::PreparedValueId{20},
      .routed_value_name = c4c::ValueNameId{20},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

prepare::PreparedCurrentBlockJoinRoutingConsumption query(
    std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> facts) {
  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts = std::move(facts);
  return prepare::query_prepared_current_block_join_routing_consumption(
      owner, c4c::BlockLabelId{3}, prepare::PreparedValueId{20},
      c4c::ValueNameId{20},
      prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
}

}  // namespace

int main() {
  const auto one = query({fact()});
  if (!one || one.edge_fact_count != 1) {
    return 1;
  }

  // Distinct predecessors are independent applicable edges. They authorize
  // the routed value only when the complete destination/source/origin family
  // agrees.
  const auto predecessors = query({fact(c4c::BlockLabelId{1}),
                                   fact(c4c::BlockLabelId{2})});
  if (!predecessors || predecessors.edge_fact_count != 2) {
    return 2;
  }

  if (query({}).status != prepare::PreparedFactBoundaryStatus::Missing) {
    return 3;
  }
  auto missing = fact(c4c::BlockLabelId{2});
  missing.status = prepare::PreparedFactBoundaryStatus::Missing;
  if (query({fact(), missing}).status !=
      prepare::PreparedFactBoundaryStatus::Missing) {
    return 4;
  }

  auto destination_id = fact(c4c::BlockLabelId{2});
  destination_id.destination_value_id = prepare::PreparedValueId{11};
  auto destination_name = fact(c4c::BlockLabelId{2});
  destination_name.destination_value_name = c4c::ValueNameId{11};
  for (const auto& conflicting : {destination_id, destination_name}) {
    if (query({fact(), conflicting}).status !=
        prepare::PreparedFactBoundaryStatus::Ambiguous) {
      return 5;
    }
  }

  auto source_id = fact(c4c::BlockLabelId{2});
  source_id.source_value_id = prepare::PreparedValueId{21};
  auto source_name = fact(c4c::BlockLabelId{2});
  source_name.source_value_name = c4c::ValueNameId{21};
  for (const auto& conflicting : {source_id, source_name}) {
    if (query({fact(), conflicting}).status !=
        prepare::PreparedFactBoundaryStatus::Mismatched) {
      return 6;
    }
  }

  auto conflicting_origin = fact(c4c::BlockLabelId{2});
  conflicting_origin.publication_semantic_origin = Origin::BirPhi;
  if (query({fact(), conflicting_origin}).status !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 7;
  }

  // A same-predecessor parallel-copy fact for a different destination cannot
  // be accepted as a unique agreeing subset.
  auto parallel_destination = fact();
  parallel_destination.destination_value_id = prepare::PreparedValueId{12};
  parallel_destination.destination_value_name = c4c::ValueNameId{12};
  if (query({fact(), parallel_destination}).status !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 8;
  }

  if (query({fact(), fact()}).status !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 9;
  }

  auto wrong_successor = fact(c4c::BlockLabelId{2});
  wrong_successor.successor_label = c4c::BlockLabelId{4};
  if (query({fact(), wrong_successor}).status !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 10;
  }

  auto incomplete_destination = fact();
  incomplete_destination.destination_value_name = c4c::kInvalidValueName;
  auto incomplete_source = fact();
  incomplete_source.source_value_id.reset();
  auto incomplete_origin = fact();
  incomplete_origin.publication_semantic_origin = Origin::Unknown;
  for (const auto& incomplete :
       {incomplete_destination, incomplete_source, incomplete_origin}) {
    if (query({incomplete}).status !=
        prepare::PreparedFactBoundaryStatus::Mismatched) {
      return 11;
    }
  }

  auto mismatched = fact(c4c::BlockLabelId{2});
  mismatched.status = prepare::PreparedFactBoundaryStatus::Mismatched;
  auto incomplete = fact(c4c::BlockLabelId{3});
  incomplete.status = prepare::PreparedFactBoundaryStatus::Incomplete;
  const auto forward = query({missing, incomplete, mismatched}).status;
  const auto reverse = query({mismatched, incomplete, missing}).status;
  if (forward != prepare::PreparedFactBoundaryStatus::Mismatched ||
      reverse != forward) {
    return 12;
  }

  return 0;
}

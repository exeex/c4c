#include "src/backend/prealloc/prepared_lookups.hpp"

#include <vector>

namespace prepare = c4c::backend::prepare;

namespace {

using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

prepare::PreparedCurrentBlockJoinRoutingFact fact(
    c4c::BlockLabelId predecessor = c4c::BlockLabelId{1},
    prepare::PreparedValueId destination = prepare::PreparedValueId{10}) {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = predecessor,
      .successor_label = c4c::BlockLabelId{3},
      .destination_value_id = destination,
      .destination_value_name =
          c4c::ValueNameId{static_cast<c4c::ValueNameId>(destination)},
      .source_value_id = prepare::PreparedValueId{20},
      .source_value_name = c4c::ValueNameId{20},
      .routed_value_id = prepare::PreparedValueId{20},
      .routed_value_name = c4c::ValueNameId{20},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

prepare::PreparedFactBoundaryStatus query(
    std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> facts,
    std::size_t* edge_count = nullptr,
    prepare::PreparedCurrentBlockJoinRoutingRole role =
        prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
    prepare::PreparedValueId routed_value_id = prepare::PreparedValueId{20},
    c4c::ValueNameId routed_value_name = c4c::ValueNameId{20}) {
  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts = std::move(facts);
  const auto result = role ==
                              prepare::PreparedCurrentBlockJoinRoutingRole::Source
                          ? prepare::query_attached_prepared_current_block_join_source(
                                &owner, &owner, c4c::BlockLabelId{3},
                                routed_value_id, routed_value_name)
                          : prepare::
                                query_attached_prepared_current_block_join_incoming_expression(
                                    &owner, &owner, c4c::BlockLabelId{3},
                                    routed_value_id, routed_value_name);
  if (edge_count != nullptr) {
    *edge_count = result.edge_fact_count;
  }
  return result.status;
}

}  // namespace

int main() {
  std::size_t edge_count = 0;
  if (query({fact()}, &edge_count) !=
          prepare::PreparedFactBoundaryStatus::Available ||
      edge_count != 1) {
    return 1;
  }

  // Source authority is queried through the same attached owner seam.  Use a
  // distinct routed identity to represent a second result-producing shape.
  auto source = fact();
  source.role = prepare::PreparedCurrentBlockJoinRoutingRole::Source;
  source.routed_value_id = prepare::PreparedValueId{30};
  source.routed_value_name = c4c::ValueNameId{30};
  if (query({source}, &edge_count,
            prepare::PreparedCurrentBlockJoinRoutingRole::Source,
            prepare::PreparedValueId{30}, c4c::ValueNameId{30}) !=
          prepare::PreparedFactBoundaryStatus::Available ||
      edge_count != 1) {
    return 10;
  }

  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts = {fact()};
  prepare::PreparedFunctionLookups stale_owner = owner;
  const auto attached_query = [&](const prepare::PreparedFunctionLookups* expected,
                                  const prepare::PreparedFunctionLookups* attached,
                                  auto role) {
    return prepare::query_attached_prepared_current_block_join_routing_consumption(
               expected, attached, c4c::BlockLabelId{3},
               prepare::PreparedValueId{20}, c4c::ValueNameId{20}, role)
        .status;
  };
  if (attached_query(nullptr, nullptr,
                     prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
          prepare::PreparedFactBoundaryStatus::Missing ||
      attached_query(&owner, &stale_owner,
                     prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
          prepare::PreparedFactBoundaryStatus::Mismatched ||
      attached_query(
          &owner, &owner,
          static_cast<prepare::PreparedCurrentBlockJoinRoutingRole>(99)) !=
          prepare::PreparedFactBoundaryStatus::Unsupported) {
    return 11;
  }

  // A result-level family cannot select a unique destination subset merely
  // because each edge is independently complete.
  if (query({fact(c4c::BlockLabelId{1}, prepare::PreparedValueId{10}),
             fact(c4c::BlockLabelId{2}, prepare::PreparedValueId{11})},
            &edge_count) != prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 2;
  }

  if (query({}) != prepare::PreparedFactBoundaryStatus::Missing) {
    return 3;
  }

  auto missing = fact();
  missing.status = prepare::PreparedFactBoundaryStatus::Missing;
  if (query({fact(), missing}) != prepare::PreparedFactBoundaryStatus::Missing) {
    return 4;
  }

  if (query({fact(), fact()}) != prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 5;
  }

  // Unrelated routed values and roles do not participate in the applicable
  // edge family and therefore cannot veto otherwise complete authority.
  auto unrelated_value = fact(c4c::BlockLabelId{2}, prepare::PreparedValueId{11});
  unrelated_value.routed_value_id = prepare::PreparedValueId{21};
  unrelated_value.routed_value_name = c4c::ValueNameId{21};
  auto unrelated_role = fact(c4c::BlockLabelId{2}, prepare::PreparedValueId{11});
  unrelated_role.role = prepare::PreparedCurrentBlockJoinRoutingRole::Source;
  if (query({unrelated_value, fact(), unrelated_role}, &edge_count) !=
          prepare::PreparedFactBoundaryStatus::Available ||
      edge_count != 1) {
    return 9;
  }

  auto wrong_successor = fact(c4c::BlockLabelId{2}, prepare::PreparedValueId{11});
  wrong_successor.successor_label = c4c::BlockLabelId{4};
  if (query({fact(), wrong_successor}) !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 6;
  }

  auto incomplete_predecessor = fact();
  incomplete_predecessor.predecessor_label = c4c::kInvalidBlockLabel;
  auto incomplete_destination = fact();
  incomplete_destination.destination_value_id = 0;
  auto collapsed_source = fact();
  collapsed_source.source_value_id = prepare::PreparedValueId{21};
  auto missing_origin = fact();
  missing_origin.publication_semantic_origin = Origin::Unknown;
  for (const auto& incomplete : {incomplete_predecessor, incomplete_destination,
                                 collapsed_source, missing_origin}) {
    if (query({incomplete}) != prepare::PreparedFactBoundaryStatus::Mismatched) {
      return 7;
    }
  }

  auto disagreeing_origin = fact(c4c::BlockLabelId{2}, prepare::PreparedValueId{11});
  disagreeing_origin.publication_semantic_origin = Origin::BirPhi;
  if (query({fact(), disagreeing_origin}) !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 8;
  }
  return 0;
}

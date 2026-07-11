#include "src/backend/prealloc/prepared_lookups.hpp"

namespace prepare = c4c::backend::prepare;

namespace {

using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

prepare::PreparedCurrentBlockJoinRoutingFact fact(
    prepare::PreparedValueId id, c4c::ValueNameId name,
    prepare::PreparedCurrentBlockJoinRoutingRole role) {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{7},
      .successor_label = c4c::BlockLabelId{9},
      .destination_value_id = prepare::PreparedValueId{31},
      .destination_value_name = c4c::ValueNameId{31},
      .source_value_id = prepare::PreparedValueId{41},
      .source_value_name = c4c::ValueNameId{41},
      .routed_value_id = id,
      .routed_value_name = name,
      .role = role,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

}  // namespace

int main() {
  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts = {
      fact(prepare::PreparedValueId{41}, c4c::ValueNameId{41},
           prepare::PreparedCurrentBlockJoinRoutingRole::Source),
      fact(prepare::PreparedValueId{42}, c4c::ValueNameId{42},
           prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression),
  };

  const auto dependency =
      prepare::query_prepared_current_block_join_routing_consumption(
          owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{42},
          c4c::ValueNameId{42},
          prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  if (dependency.status != prepare::PreparedFactBoundaryStatus::Available ||
      dependency.edge_fact_count != 1) {
    return 1;
  }

  // Rejecting the enclosing direct root as an incoming expression must not
  // erase the independently keyed dependency authority.
  const auto root = prepare::query_prepared_current_block_join_routing_consumption(
      owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{41},
      c4c::ValueNameId{41},
      prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  if (root.status != prepare::PreparedFactBoundaryStatus::Mismatched || root) {
    return 2;
  }

  const auto wrong_key =
      prepare::query_prepared_current_block_join_routing_consumption(
          owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{42},
          c4c::ValueNameId{43},
          prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  if (wrong_key.status != prepare::PreparedFactBoundaryStatus::Mismatched ||
      wrong_key) {
    return 3;
  }
  return 0;
}

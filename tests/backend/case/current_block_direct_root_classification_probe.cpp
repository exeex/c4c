#include "src/backend/prealloc/prepared_lookups.hpp"

namespace prepare = c4c::backend::prepare;

namespace {

using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

prepare::PreparedCurrentBlockJoinRoutingFact direct_root_fact() {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{7},
      .successor_label = c4c::BlockLabelId{9},
      .destination_value_id = prepare::PreparedValueId{31},
      .destination_value_name = c4c::ValueNameId{31},
      .source_value_id = prepare::PreparedValueId{41},
      .source_value_name = c4c::ValueNameId{41},
      .routed_value_id = prepare::PreparedValueId{41},
      .routed_value_name = c4c::ValueNameId{41},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::Source,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

}  // namespace

int main() {
  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts = {direct_root_fact()};

  const auto direct = prepare::query_prepared_current_block_join_routing_consumption(
      owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{41},
      c4c::ValueNameId{41},
      prepare::PreparedCurrentBlockJoinRoutingRole::Source);
  if (direct.status != prepare::PreparedFactBoundaryStatus::Available ||
      direct.edge_fact_count != 1) {
    return 1;
  }

  // A direct publication root is not thereby an incoming-expression dependency.
  const auto dependency =
      prepare::query_prepared_current_block_join_routing_consumption(
          owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{41},
          c4c::ValueNameId{41},
          prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  if (dependency.status != prepare::PreparedFactBoundaryStatus::Mismatched ||
      dependency) {
    return 2;
  }
  return 0;
}

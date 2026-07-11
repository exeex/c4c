#include "src/backend/prealloc/prepared_lookups.hpp"

namespace prepare = c4c::backend::prepare;

namespace {

using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

prepare::PreparedCurrentBlockJoinRoutingFact fact() {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{7},
      .successor_label = c4c::BlockLabelId{9},
      .destination_value_id = prepare::PreparedValueId{31},
      .destination_value_name = c4c::ValueNameId{31},
      .source_value_id = prepare::PreparedValueId{41},
      .source_value_name = c4c::ValueNameId{41},
      .routed_value_id = prepare::PreparedValueId{43},
      .routed_value_name = c4c::ValueNameId{43},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

prepare::PreparedFactBoundaryStatus query(
    const prepare::PreparedFunctionLookups& owner, c4c::BlockLabelId successor,
    prepare::PreparedValueId value_id, c4c::ValueNameId value_name,
    prepare::PreparedCurrentBlockJoinRoutingRole role) {
  return prepare::query_prepared_current_block_join_routing_consumption(
             owner, successor, value_id, value_name, role)
      .status;
}

}  // namespace

int main() {
  prepare::PreparedFunctionLookups owner;
  owner.current_block_join_routing_facts.push_back(fact());
  if (query(owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{43},
            c4c::ValueNameId{43},
            prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
      prepare::PreparedFactBoundaryStatus::Available) {
    return 1;
  }
  if (query(owner, c4c::BlockLabelId{10}, prepare::PreparedValueId{43},
            c4c::ValueNameId{43},
            prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 2;
  }
  if (query(owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{44},
            c4c::ValueNameId{43},
            prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 3;
  }
  if (query(owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{43},
            c4c::ValueNameId{44},
            prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 4;
  }
  if (query(owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{43},
            c4c::ValueNameId{43},
            prepare::PreparedCurrentBlockJoinRoutingRole::Source) !=
      prepare::PreparedFactBoundaryStatus::Mismatched) {
    return 5;
  }
  owner.current_block_join_routing_facts.push_back(fact());
  if (query(owner, c4c::BlockLabelId{9}, prepare::PreparedValueId{43},
            c4c::ValueNameId{43},
            prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression) !=
      prepare::PreparedFactBoundaryStatus::Ambiguous) {
    return 6;
  }
  return 0;
}

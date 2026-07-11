#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/publication_plans.hpp"

#include <vector>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

prepare::PreparedCurrentBlockJoinRoutingFact authoritative_fact() {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{1},
      .successor_label = c4c::BlockLabelId{2},
      .destination_value_id = prepare::PreparedValueId{10},
      .destination_value_name = c4c::ValueNameId{10},
      .source_value_id = prepare::PreparedValueId{20},
      .source_value_name = c4c::ValueNameId{20},
      .routed_value_id = prepare::PreparedValueId{20},
      .routed_value_name = c4c::ValueNameId{20},
      .role = prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression,
      .publication_semantic_origin =
          prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
              PublicationSemanticOrigin::PreparedJoinTransfer,
  };
}

bool same_fact(const prepare::PreparedCurrentBlockJoinRoutingFact& lhs,
               const prepare::PreparedCurrentBlockJoinRoutingFact& rhs) {
  return lhs.status == rhs.status &&
         lhs.predecessor_label == rhs.predecessor_label &&
         lhs.successor_label == rhs.successor_label &&
         lhs.destination_value_id == rhs.destination_value_id &&
         lhs.destination_value_name == rhs.destination_value_name &&
         lhs.source_value_id == rhs.source_value_id &&
         lhs.source_value_name == rhs.source_value_name &&
         lhs.routed_value_id == rhs.routed_value_id &&
         lhs.routed_value_name == rhs.routed_value_name &&
         lhs.role == rhs.role &&
         lhs.publication_semantic_origin == rhs.publication_semantic_origin;
}

bir::Route5CurrentBlockJoinSourceRecord diagnostic(
    bir::Route5PublicationStatus status,
    c4c::ValueNameId source_name) {
  return {
      .available = status == bir::Route5PublicationStatus::Available,
      .status = status,
      .successor_label_id = c4c::BlockLabelId{2},
      .predecessor_label_id = c4c::BlockLabelId{1},
      .destination_value_name = "destination",
      .source_value_name = source_name == c4c::ValueNameId{20} ? "source"
                                                               : "conflict",
      .source_value_kind = bir::Value::Kind::Named,
  };
}

}  // namespace

int main() {
  const auto expected = authoritative_fact();
  std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> authoritative_facts{
      expected};
  const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact> no_facts;

  bir::Route5EdgeJoinSourceIndex missing;
  bir::Route5EdgeJoinSourceIndex unique;
  unique.join_records.push_back(
      diagnostic(bir::Route5PublicationStatus::NoMatch,
                 c4c::ValueNameId{20}));
  bir::Route5EdgeJoinSourceIndex agreeing;
  agreeing.join_records.push_back(
      diagnostic(bir::Route5PublicationStatus::Available,
                 c4c::ValueNameId{20}));
  bir::Route5EdgeJoinSourceIndex conflicting;
  conflicting.join_records.push_back(
      diagnostic(bir::Route5PublicationStatus::Available,
                 c4c::ValueNameId{20}));
  conflicting.join_records.push_back(
      diagnostic(bir::Route5PublicationStatus::Available,
                 c4c::ValueNameId{21}));

  for (const auto* route5_diagnostics :
       {&missing, &unique, &agreeing, &conflicting}) {
    // Route 5 is intentionally not an input to the authoritative query. Merely
    // observing every diagnostic shape must not seed an absent prepared fact.
    (void)route5_diagnostics;
    const auto absent =
        prepare::query_prepared_current_block_join_routing_consumption(
            no_facts, expected.successor_label, expected.routed_value_id,
            expected.routed_value_name, expected.role);
    if (absent.status != prepare::PreparedFactBoundaryStatus::Missing) {
      return 1;
    }

    // Nor may Route 5 replace, erase, or authorize the fixed prepared fact.
    const auto available =
        prepare::query_prepared_current_block_join_routing_consumption(
            authoritative_facts, expected.successor_label,
            expected.routed_value_id, expected.routed_value_name,
            expected.role);
    if (available.status != prepare::PreparedFactBoundaryStatus::Available ||
        available.edge_fact_count != 1 ||
        available.publication_semantic_origin !=
            expected.publication_semantic_origin ||
        authoritative_facts.size() != 1 ||
        !same_fact(authoritative_facts.front(), expected)) {
      return 2;
    }
  }
  return 0;
}

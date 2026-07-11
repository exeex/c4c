#include "src/backend/prealloc/publication_plans.hpp"

namespace prepare = c4c::backend::prepare;

namespace {

using PolicyState = prepare::PreparedCurrentBlockJoinPolicyState;
using Role = prepare::PreparedCurrentBlockJoinRoutingRole;
using Origin = prepare::PreparedCurrentBlockJoinParallelCopySourceFact::
    PublicationSemanticOrigin;

constexpr c4c::BlockLabelId kSuccessor{9};
constexpr prepare::PreparedValueId kRoutedValue{43};
constexpr c4c::ValueNameId kRoutedName{43};

prepare::PreparedCurrentBlockJoinRoutingFact authoritative_fact() {
  return {
      .status = prepare::PreparedFactBoundaryStatus::Available,
      .predecessor_label = c4c::BlockLabelId{7},
      .successor_label = kSuccessor,
      .destination_value_id = prepare::PreparedValueId{31},
      .destination_value_name = c4c::ValueNameId{31},
      .source_value_id = kRoutedValue,
      .source_value_name = kRoutedName,
      .routed_value_id = kRoutedValue,
      .routed_value_name = kRoutedName,
      .role = Role::IncomingExpression,
      .publication_semantic_origin = Origin::PreparedJoinTransfer,
  };
}

prepare::PreparedCurrentBlockJoinPolicyQueryResult query(
    const prepare::PreparedCurrentBlockJoinRoutingPolicy* policy) {
  return prepare::query_prepared_current_block_join_policy(
      policy, kSuccessor, kRoutedValue, kRoutedName,
      Role::IncomingExpression);
}

}  // namespace

int main() {
  const auto absent_policy = query(nullptr);
  if (absent_policy.state != PolicyState::AbsentPolicy ||
      absent_policy.consumption || absent_policy) {
    return 1;
  }

  const prepare::PreparedCurrentBlockJoinRoutingPolicy absent_owner;
  const auto no_owner = query(&absent_owner);
  if (no_owner.state != PolicyState::AbsentOwner || no_owner.consumption ||
      no_owner) {
    return 2;
  }

  prepare::PreparedCurrentBlockJoinRoutingPolicy zero_fact_owner;
  zero_fact_owner.owner_facts.emplace();
  const auto zero_facts = query(&zero_fact_owner);
  if (zero_facts.state != PolicyState::AttachedOwnerWithoutApplicableFacts ||
      zero_facts.consumption || zero_facts) {
    return 3;
  }

  prepare::PreparedCurrentBlockJoinRoutingPolicy nonapplicable_owner;
  nonapplicable_owner.owner_facts = {authoritative_fact()};
  nonapplicable_owner.owner_facts->front().successor_label =
      c4c::BlockLabelId{10};
  const auto nonapplicable = query(&nonapplicable_owner);
  if (nonapplicable.state !=
          PolicyState::AttachedOwnerWithoutApplicableFacts ||
      nonapplicable.consumption || nonapplicable) {
    return 4;
  }

  prepare::PreparedCurrentBlockJoinRoutingPolicy authoritative_owner;
  authoritative_owner.owner_facts = {authoritative_fact()};
  const auto authoritative = query(&authoritative_owner);
  if (authoritative.state != PolicyState::AuthoritativeFacts ||
      !authoritative.consumption ||
      authoritative.consumption.edge_fact_count != 1 || !authoritative) {
    return 5;
  }

  return 0;
}

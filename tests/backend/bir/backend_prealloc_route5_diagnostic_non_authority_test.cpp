#include "src/backend/bir/bir.hpp"
#include "src/backend/prealloc/publication_plans.hpp"

#include <vector>

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

int main() {
  const std::vector<prepare::PreparedCurrentBlockJoinRoutingFact>
      authoritative_facts;
  const auto query = [&] {
    return prepare::query_prepared_current_block_join_routing_consumption(
        authoritative_facts, c4c::BlockLabelId{1},
        prepare::PreparedValueId{2}, c4c::ValueNameId{3},
        prepare::PreparedCurrentBlockJoinRoutingRole::IncomingExpression);
  };

  bir::Route5EdgeJoinSourceIndex missing_diagnostic;
  bir::Route5EdgeJoinSourceIndex unique_diagnostic;
  unique_diagnostic.join_records.push_back(
      bir::Route5CurrentBlockJoinSourceRecord{});
  bir::Route5EdgeJoinSourceIndex conflicting_diagnostics;
  conflicting_diagnostics.join_records.resize(2);

  for (const auto* diagnostic : {&missing_diagnostic, &unique_diagnostic,
                                 &conflicting_diagnostics}) {
    (void)diagnostic;
    if (query().status != prepare::PreparedFactBoundaryStatus::Missing ||
        !authoritative_facts.empty()) {
      return 1;
    }
  }
  return 0;
}

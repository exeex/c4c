#include "src/backend/prealloc/publication_plans.hpp"

namespace bir = c4c::backend::bir;
namespace prepare = c4c::backend::prepare;

namespace {

bool consistent(const bir::Value& transfer_result,
                const bir::Value& edge_destination,
                const bir::Value& publication_destination) {
  prepare::PreparedJoinTransfer transfer{.result = transfer_result};
  prepare::PreparedEdgeValueTransfer edge{
      .destination_value = edge_destination,
  };
  prepare::PreparedEdgePublication publication{
      .destination_value = publication_destination,
      .join_transfer = &transfer,
      .edge_transfer = &edge,
  };
  return prepare::prepared_join_transfer_destination_consistent(publication);
}

}  // namespace

int main() {
  const auto destination = bir::Value::named(bir::TypeKind::I32, "%join");
  const auto other = bir::Value::named(bir::TypeKind::I32, "%other");

  if (!consistent(destination, destination, destination)) {
    return 1;
  }
  if (consistent(other, destination, destination)) {
    return 2;
  }
  if (consistent(destination, other, destination)) {
    return 3;
  }
  if (consistent(destination, destination, other)) {
    return 4;
  }

  prepare::PreparedEdgePublication missing_links{
      .destination_value = destination,
  };
  if (prepare::prepared_join_transfer_destination_consistent(missing_links)) {
    return 5;
  }
  return 0;
}

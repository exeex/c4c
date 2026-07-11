#include "src/backend/mir/aarch64/module/module.hpp"

#include <memory>

namespace module = c4c::backend::aarch64::module;
namespace prepare = c4c::backend::prepare;

int main() {
  module::FunctionLoweringContext owner;
  owner.prepared_lookups_owner =
      std::make_shared<prepare::PreparedFunctionLookups>();
  owner.prepared_lookups = owner.prepared_lookups_owner.get();

  auto copied = owner;
  owner = {};
  if (copied.prepared_lookups_owner == nullptr ||
      copied.prepared_lookups != copied.prepared_lookups_owner.get()) {
    return 1;
  }

  module::FunctionLoweringContext missing;
  if (missing.prepared_lookups_owner != nullptr ||
      missing.prepared_lookups != nullptr) {
    return 2;
  }
  return 0;
}

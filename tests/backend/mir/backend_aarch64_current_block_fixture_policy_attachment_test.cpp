#include "src/backend/mir/aarch64/module/module.hpp"

#include <memory>

namespace module = c4c::backend::aarch64::module;
namespace prepare = c4c::backend::prepare;

struct FixtureAxes {
  bool policy_present = false;
  module::FunctionLoweringContext context;
  std::shared_ptr<prepare::PreparedFunctionLookups> detached_owner;
};

FixtureAxes make_fixture(bool policy_present, bool attachment_present) {
  FixtureAxes fixture;
  fixture.policy_present = policy_present;
  fixture.detached_owner =
      std::make_shared<prepare::PreparedFunctionLookups>();
  if (attachment_present) {
    fixture.context.prepared_lookups_owner = fixture.detached_owner;
    fixture.context.prepared_lookups = fixture.detached_owner.get();
  }
  return fixture;
}

int main() {
  for (const bool policy_present : {false, true}) {
    for (const bool attachment_present : {false, true}) {
      const auto fixture = make_fixture(policy_present, attachment_present);
      if (fixture.policy_present != policy_present ||
          (fixture.context.prepared_lookups != nullptr) != attachment_present ||
          (fixture.context.prepared_lookups_owner != nullptr) !=
              attachment_present) {
        return 1;
      }
    }
  }
  return 0;
}

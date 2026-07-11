#include "src/backend/mir/aarch64/codegen/traversal.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <memory>

namespace codegen = c4c::backend::aarch64::codegen;
namespace module = c4c::backend::aarch64::module;
namespace prepare = c4c::backend::prepare;

int main() {
  prepare::PreparedBirModule prepared;
  prepared.target_profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  const auto function_name = prepared.names.function_names.intern("attachment.owner");
  prepared.control_flow.functions.push_back(prepare::PreparedControlFlowFunction{
      .function_name = function_name,
  });

  std::weak_ptr<const prepare::PreparedFunctionLookups> owner_lifetime;
  const auto copied = [&] {
    const auto owner = codegen::make_function_lowering_context(
        prepared, prepared.target_profile, prepared.control_flow.functions.front());
    if (owner.prepared_lookups_owner == nullptr ||
        owner.prepared_lookups != owner.prepared_lookups_owner.get()) {
      return module::FunctionLoweringContext{};
    }
    owner_lifetime = owner.prepared_lookups_owner;
    return owner;
  }();

  if (copied.prepared_lookups_owner == nullptr ||
      copied.prepared_lookups != copied.prepared_lookups_owner.get() ||
      owner_lifetime.expired()) {
    return 1;
  }

  module::FunctionLoweringContext missing;
  if (missing.prepared_lookups_owner != nullptr ||
      missing.prepared_lookups != nullptr) {
    return 2;
  }
  return 0;
}

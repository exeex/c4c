#pragma once

#include "../abi/abi.hpp"

#include <optional>

namespace c4c::backend::aarch64::module {

struct Module {
  const c4c::backend::prepare::PreparedBirModule* prepared = nullptr;
  c4c::TargetProfile target_profile{};
};

struct BuildResult {
  std::optional<Module> module;
  std::optional<c4c::backend::aarch64::abi::HandoffError> error;
};

[[nodiscard]] BuildResult build(const c4c::backend::prepare::PreparedBirModule& prepared);

}  // namespace c4c::backend::aarch64::module

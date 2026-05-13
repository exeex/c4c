#pragma once

#include "../../../backend.hpp"

#include <optional>
#include <string>

namespace c4c::backend::aarch64::abi {

enum class HandoffErrorKind {
  UnsupportedTargetArch,
  UnsupportedBackendAbi,
};

struct HandoffError {
  HandoffErrorKind kind = HandoffErrorKind::UnsupportedTargetArch;
  c4c::TargetProfile target_profile{};
  std::string message;
};

[[nodiscard]] c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module);
[[nodiscard]] bool is_aarch64_target(const c4c::TargetProfile& target_profile);
[[nodiscard]] bool is_aapcs64_abi(const c4c::TargetProfile& target_profile);
[[nodiscard]] std::optional<HandoffError> validate_prepared_module_handoff(
    const c4c::backend::prepare::PreparedBirModule& module);

}  // namespace c4c::backend::aarch64::abi

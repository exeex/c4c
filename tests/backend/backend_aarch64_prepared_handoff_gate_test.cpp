#include "src/backend/mir/aarch64/api/api.hpp"
#include "src/backend/mir/aarch64/abi/abi.hpp"
#include "src/backend/prealloc/prealloc.hpp"
#include "src/target_profile.hpp"

#include <iostream>
#include <optional>
#include <string_view>
#include <utility>

namespace {

namespace aarch64_abi = c4c::backend::aarch64::abi;
namespace aarch64_api = c4c::backend::aarch64::api;
namespace prepare = c4c::backend::prepare;

int fail(const char* message) {
  std::cerr << message << "\n";
  return 1;
}

prepare::PreparedBirModule prepared_for(c4c::TargetProfile target_profile) {
  prepare::PreparedBirModule prepared;
  prepared.module.target_triple = target_profile.triple;
  prepared.target_profile = std::move(target_profile);
  return prepared;
}

bool contains(std::string_view haystack, std::string_view needle) {
  return haystack.find(needle) != std::string_view::npos;
}

int accepted_aarch64_aapcs64_handoff_keeps_prepared_identity() {
  auto prepared = prepared_for(c4c::default_target_profile(c4c::TargetArch::Aarch64));

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.error.has_value()) {
    return fail("expected AArch64/AAPCS64 prepared module to pass handoff gate");
  }
  if (!result.module.has_value()) {
    return fail("expected accepted handoff to construct an AArch64 module skeleton");
  }
  if (result.module->prepared != &prepared) {
    return fail("expected accepted handoff to keep the prepared module as semantic input");
  }
  if (result.module->target_profile.arch != c4c::TargetArch::Aarch64 ||
      result.module->target_profile.backend_abi != c4c::BackendAbiKind::Aapcs64) {
    return fail("expected accepted handoff to publish resolved AArch64/AAPCS64 profile");
  }
  return 0;
}

int rejected_non_aarch64_target_does_not_build_module() {
  const auto prepared = prepared_for(c4c::default_target_profile(c4c::TargetArch::X86_64));

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.module.has_value()) {
    return fail("expected non-AArch64 prepared module to be rejected before module construction");
  }
  if (!result.error.has_value() ||
      result.error->kind != aarch64_abi::HandoffErrorKind::UnsupportedTargetArch) {
    return fail("expected non-AArch64 rejection to report target-arch handoff error");
  }
  if (!contains(result.error->message, "TargetArch::Aarch64") ||
      !contains(result.error->message, "arch=x86_64")) {
    return fail("expected non-AArch64 rejection message to name required and actual arch");
  }
  return 0;
}

int rejected_non_aapcs64_abi_does_not_build_module() {
  auto profile = c4c::default_target_profile(c4c::TargetArch::Aarch64);
  profile.backend_abi = c4c::BackendAbiKind::SysV_X86_64;
  const auto prepared = prepared_for(std::move(profile));

  const auto result = aarch64_api::build_prepared_module(prepared);
  if (result.module.has_value()) {
    return fail("expected non-AAPCS64 prepared module to be rejected before module construction");
  }
  if (!result.error.has_value() ||
      result.error->kind != aarch64_abi::HandoffErrorKind::UnsupportedBackendAbi) {
    return fail("expected wrong-ABI rejection to report backend-ABI handoff error");
  }
  if (!contains(result.error->message, "BackendAbiKind::Aapcs64") ||
      !contains(result.error->message, "abi=sysv_x86_64")) {
    return fail("expected wrong-ABI rejection message to name required and actual ABI");
  }
  return 0;
}

}  // namespace

int main() {
  if (const int status = accepted_aarch64_aapcs64_handoff_keeps_prepared_identity();
      status != 0) {
    return status;
  }
  if (const int status = rejected_non_aarch64_target_does_not_build_module(); status != 0) {
    return status;
  }
  if (const int status = rejected_non_aapcs64_abi_does_not_build_module(); status != 0) {
    return status;
  }
  return 0;
}

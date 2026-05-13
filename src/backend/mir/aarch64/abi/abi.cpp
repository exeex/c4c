#include "abi.hpp"

namespace c4c::backend::aarch64::abi {

namespace {

std::string explicit_target_triple(const c4c::backend::prepare::PreparedBirModule& module) {
  if (!module.target_profile.triple.empty()) {
    return module.target_profile.triple;
  }
  if (!module.module.target_triple.empty()) {
    return module.module.target_triple;
  }
  return c4c::llvm_target_triple(module.target_profile);
}

HandoffError make_error(HandoffErrorKind kind,
                        const c4c::TargetProfile& target_profile,
                        const char* requirement) {
  return HandoffError{
      .kind = kind,
      .target_profile = target_profile,
      .message = std::string("AArch64 prepared-module handoff requires ") + requirement +
                 ", got arch=" + c4c::target_arch_name(target_profile.arch) +
                 " abi=" + c4c::backend_abi_name(target_profile.backend_abi),
  };
}

}  // namespace

c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module) {
  if (module.target_profile.arch != c4c::TargetArch::Unknown) {
    return module.target_profile;
  }
  const std::string target_triple = explicit_target_triple(module);
  if (!target_triple.empty()) {
    return c4c::target_profile_from_triple(target_triple);
  }
  return module.target_profile;
}

bool is_aarch64_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::Aarch64;
}

bool is_aapcs64_abi(const c4c::TargetProfile& target_profile) {
  return target_profile.backend_abi == c4c::BackendAbiKind::Aapcs64;
}

std::optional<HandoffError> validate_prepared_module_handoff(
    const c4c::backend::prepare::PreparedBirModule& module) {
  const c4c::TargetProfile target_profile = resolve_target_profile(module);
  if (!is_aarch64_target(target_profile)) {
    return make_error(HandoffErrorKind::UnsupportedTargetArch, target_profile,
                      "TargetArch::Aarch64");
  }
  if (!is_aapcs64_abi(target_profile)) {
    return make_error(HandoffErrorKind::UnsupportedBackendAbi, target_profile,
                      "BackendAbiKind::Aapcs64");
  }
  return std::nullopt;
}

}  // namespace c4c::backend::aarch64::abi

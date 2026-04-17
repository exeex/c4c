#pragma once

#include "../target_profile.hpp"

#include <string_view>

namespace c4c::backend {

enum class Target {
  X86_64,
  I686,
  Aarch64,
  Riscv64,
};

Target target_from_triple(std::string_view target_triple);
Target target_from_profile(const c4c::TargetProfile& profile);
c4c::TargetProfile target_profile_from_backend_target(Target target,
                                                      std::string_view target_triple = {});
const char* target_name(Target target);

}  // namespace c4c::backend

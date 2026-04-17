#include "target.hpp"

#include "../target_profile.hpp"

namespace c4c::backend {

Target target_from_triple(std::string_view target_triple) {
  return target_from_profile(c4c::target_profile_from_triple(target_triple));
}

Target target_from_profile(const c4c::TargetProfile& profile) {
  switch (profile.arch) {
    case c4c::TargetArch::X86_64:
      return Target::X86_64;
    case c4c::TargetArch::I686:
      return Target::I686;
    case c4c::TargetArch::Aarch64:
      return Target::Aarch64;
    case c4c::TargetArch::Riscv64:
      return Target::Riscv64;
    case c4c::TargetArch::Unknown:
      break;
  }
  return Target::X86_64;
}

c4c::TargetProfile target_profile_from_backend_target(Target target,
                                                      std::string_view target_triple) {
  if (!target_triple.empty()) {
    const auto profile = c4c::target_profile_from_triple(target_triple);
    if (target_from_profile(profile) == target) {
      return profile;
    }
  }

  switch (target) {
    case Target::X86_64:
      return c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
    case Target::I686:
      return c4c::target_profile_from_triple("i386-unknown-linux-gnu");
    case Target::Aarch64:
      return c4c::target_profile_from_triple("aarch64-unknown-linux-gnu");
    case Target::Riscv64:
      return c4c::target_profile_from_triple("riscv64gc-unknown-linux-gnu");
  }
  return c4c::target_profile_from_triple("x86_64-unknown-linux-gnu");
}

const char* target_name(Target target) {
  switch (target) {
    case Target::X86_64:
      return "x86_64";
    case Target::I686:
      return "i686";
    case Target::Aarch64:
      return "aarch64";
    case Target::Riscv64:
      return "riscv64";
  }
  return "unknown";
}

}  // namespace c4c::backend

#include "target.hpp"

#include "../target_profile.hpp"

namespace c4c::backend {

Target target_from_triple(std::string_view target_triple) {
  switch (c4c::target_profile_from_triple(target_triple).arch) {
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

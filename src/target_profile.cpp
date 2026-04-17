#include "target_profile.hpp"

#include <stdexcept>

namespace c4c {

namespace {

bool triple_contains(std::string_view triple, std::string_view needle) {
  return !triple.empty() && triple.find(needle) != std::string_view::npos;
}

std::string_view arch_from_triple(std::string_view target_triple) {
  const std::size_t dash = target_triple.find('-');
  if (dash == std::string_view::npos) {
    return target_triple;
  }
  return target_triple.substr(0, dash);
}

TargetOs os_from_triple(std::string_view triple) {
  if (triple_contains(triple, "apple") || triple_contains(triple, "darwin")) {
    return TargetOs::Darwin;
  }
  if (triple_contains(triple, "linux")) {
    return TargetOs::Linux;
  }
  if (triple_contains(triple, "windows") || triple_contains(triple, "mingw") ||
      triple_contains(triple, "msvc")) {
    return TargetOs::Windows;
  }
  return TargetOs::Unknown;
}

BackendAbiKind backend_abi_from_triple(TargetArch arch, std::string_view triple) {
  switch (arch) {
    case TargetArch::X86_64:
      return BackendAbiKind::SysV_X86_64;
    case TargetArch::I686:
      return BackendAbiKind::SysV_I686;
    case TargetArch::Aarch64:
      return BackendAbiKind::Aapcs64;
    case TargetArch::Riscv64:
      if (triple_contains(triple, "lp64d") || triple_contains(triple, "riscv64gc")) {
        return BackendAbiKind::RiscvLp64D;
      }
      if (triple_contains(triple, "lp64f")) {
        return BackendAbiKind::RiscvLp64F;
      }
      if (triple_contains(triple, "lp64")) {
        return BackendAbiKind::RiscvLp64;
      }
      return BackendAbiKind::RiscvLp64;
    case TargetArch::Unknown:
      return BackendAbiKind::Unknown;
  }
  return BackendAbiKind::Unknown;
}

bool abi_has_float_arg_registers(BackendAbiKind abi) {
  switch (abi) {
    case BackendAbiKind::SysV_X86_64:
    case BackendAbiKind::Aapcs64:
    case BackendAbiKind::RiscvLp64F:
    case BackendAbiKind::RiscvLp64D:
      return true;
    case BackendAbiKind::SysV_I686:
    case BackendAbiKind::RiscvLp64:
    case BackendAbiKind::Unknown:
      return false;
  }
  return false;
}

}  // namespace

std::string default_host_target_triple() {
#if defined(__aarch64__) || defined(_M_ARM64)
#if defined(__APPLE__)
  return "aarch64-apple-darwin";
#elif defined(__linux__)
  return "aarch64-unknown-linux-gnu";
#else
  return "aarch64-unknown-unknown";
#endif
#elif defined(__x86_64__) || defined(_M_X64)
#if defined(__APPLE__)
  return "x86_64-apple-darwin";
#elif defined(__linux__)
  return "x86_64-unknown-linux-gnu";
#else
  return "x86_64-unknown-unknown";
#endif
#elif defined(__i386__) || defined(_M_IX86)
#if defined(__linux__)
  return "i386-unknown-linux-gnu";
#else
  return "i386-unknown-unknown";
#endif
#else
  return "unknown-unknown-unknown";
#endif
}

TargetProfile default_target_profile(TargetArch arch) {
  switch (arch) {
    case TargetArch::X86_64:
      return target_profile_from_triple("x86_64-unknown-linux-gnu");
    case TargetArch::I686:
      return target_profile_from_triple("i386-unknown-linux-gnu");
    case TargetArch::Aarch64:
      return target_profile_from_triple("aarch64-unknown-linux-gnu");
    case TargetArch::Riscv64:
      return target_profile_from_triple("riscv64gc-unknown-linux-gnu");
    case TargetArch::Unknown:
      break;
  }
  return target_profile_from_triple(default_host_target_triple());
}

TargetProfile target_profile_from_triple(std::string_view target_triple) {
  TargetProfile profile;
  profile.triple = std::string(target_triple);

  const std::string_view arch = arch_from_triple(target_triple);
  if (arch == "x86_64" || arch == "amd64") {
    profile.arch = TargetArch::X86_64;
  } else if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
    profile.arch = TargetArch::I686;
  } else if (arch == "aarch64" || arch == "arm64") {
    profile.arch = TargetArch::Aarch64;
  } else if (arch == "riscv64" || arch == "riscv64gc") {
    profile.arch = TargetArch::Riscv64;
  } else {
    throw std::invalid_argument("unsupported target triple: " + std::string(target_triple));
  }

  profile.os = os_from_triple(target_triple);
  profile.backend_abi = backend_abi_from_triple(profile.arch, target_triple);
  profile.has_float_arg_registers = abi_has_float_arg_registers(profile.backend_abi);
  profile.has_float_return_registers = abi_has_float_arg_registers(profile.backend_abi);
  return profile;
}

const char* target_arch_name(TargetArch arch) {
  switch (arch) {
    case TargetArch::Unknown:
      return "unknown";
    case TargetArch::X86_64:
      return "x86_64";
    case TargetArch::I686:
      return "i686";
    case TargetArch::Aarch64:
      return "aarch64";
    case TargetArch::Riscv64:
      return "riscv64";
  }
  return "unknown";
}

const char* target_os_name(TargetOs os) {
  switch (os) {
    case TargetOs::Unknown:
      return "unknown";
    case TargetOs::Linux:
      return "linux";
    case TargetOs::Darwin:
      return "darwin";
    case TargetOs::Windows:
      return "windows";
  }
  return "unknown";
}

const char* backend_abi_name(BackendAbiKind abi) {
  switch (abi) {
    case BackendAbiKind::Unknown:
      return "unknown";
    case BackendAbiKind::SysV_X86_64:
      return "sysv_x86_64";
    case BackendAbiKind::SysV_I686:
      return "sysv_i686";
    case BackendAbiKind::Aapcs64:
      return "aapcs64";
    case BackendAbiKind::RiscvLp64:
      return "riscv_lp64";
    case BackendAbiKind::RiscvLp64F:
      return "riscv_lp64f";
    case BackendAbiKind::RiscvLp64D:
      return "riscv_lp64d";
  }
  return "unknown";
}

}  // namespace c4c

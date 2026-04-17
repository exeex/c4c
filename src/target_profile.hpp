#pragma once

#include <string>
#include <string_view>

namespace c4c {

enum class TargetArch {
  Unknown,
  X86_64,
  I686,
  Aarch64,
  Riscv64,
};

enum class BackendAbiKind {
  Unknown,
  SysV_X86_64,
  SysV_I686,
  Aapcs64,
  RiscvLp64,
  RiscvLp64F,
  RiscvLp64D,
};

struct TargetProfile {
  std::string triple;
  TargetArch arch = TargetArch::Unknown;
  BackendAbiKind backend_abi = BackendAbiKind::Unknown;
  bool has_float_arg_registers = false;
  bool has_float_return_registers = false;
};

std::string default_host_target_triple();
TargetProfile default_target_profile(TargetArch arch);
TargetProfile target_profile_from_triple(std::string_view target_triple);
const char* target_arch_name(TargetArch arch);
const char* backend_abi_name(BackendAbiKind abi);

}  // namespace c4c

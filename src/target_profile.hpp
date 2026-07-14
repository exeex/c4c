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

enum class TargetOs {
  Unknown,
  Linux,
  Darwin,
  Windows,
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

enum class TargetRelocationModel {
  Static,
  Pic,
  Pie,
};

struct TargetProfile {
  std::string triple;
  TargetArch arch = TargetArch::Unknown;
  TargetOs os = TargetOs::Unknown;
  BackendAbiKind backend_abi = BackendAbiKind::Unknown;
  TargetRelocationModel relocation_model = TargetRelocationModel::Static;
  bool has_float_arg_registers = false;
  bool has_float_return_registers = false;
};

// C long is the one scalar integer width that differs among supported targets.
// Keep this policy structured so producer, verifier, and receiver share it.
constexpr unsigned long_width_bits(const TargetProfile& target_profile) {
  return target_profile.arch == TargetArch::I686 ? 32u : 64u;
}

std::string default_host_target_triple();
TargetProfile default_target_profile(TargetArch arch);
TargetProfile target_profile_from_triple(std::string_view target_triple);
std::string llvm_target_triple(const TargetProfile& target_profile);
const char* target_arch_name(TargetArch arch);
const char* target_os_name(TargetOs os);
const char* backend_abi_name(BackendAbiKind abi);
const char* target_relocation_model_name(TargetRelocationModel model);

}  // namespace c4c

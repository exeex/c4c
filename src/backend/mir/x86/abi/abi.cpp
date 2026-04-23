#include "abi.hpp"

namespace c4c::backend::x86::abi {

std::string resolve_target_triple(const c4c::backend::prepare::PreparedBirModule& module) {
  return module.module.target_triple.empty() ? c4c::default_host_target_triple()
                                             : module.module.target_triple;
}

c4c::TargetProfile resolve_target_profile(
    const c4c::backend::prepare::PreparedBirModule& module) {
  if (module.target_profile.arch != c4c::TargetArch::Unknown) {
    return module.target_profile;
  }
  return c4c::target_profile_from_triple(resolve_target_triple(module));
}

bool is_x86_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::X86_64 ||
         target_profile.arch == c4c::TargetArch::I686;
}

bool is_apple_darwin_target(std::string_view target_triple) {
  return target_triple.find("apple-darwin") != std::string_view::npos;
}

std::string narrow_i32_register_name(std::string_view wide_register) {
  if (wide_register == "rax") return "eax";
  if (wide_register == "rbx") return "ebx";
  if (wide_register == "rcx") return "ecx";
  if (wide_register == "rdx") return "edx";
  if (wide_register == "rdi") return "edi";
  if (wide_register == "rsi") return "esi";
  if (wide_register == "rbp") return "ebp";
  if (wide_register == "rsp") return "esp";
  if (wide_register == "r8") return "r8d";
  if (wide_register == "r9") return "r9d";
  if (wide_register == "r10") return "r10d";
  if (wide_register == "r11") return "r11d";
  if (wide_register == "r12") return "r12d";
  if (wide_register == "r13") return "r13d";
  if (wide_register == "r14") return "r14d";
  if (wide_register == "r15") return "r15d";
  return std::string(wide_register);
}

std::string render_asm_symbol_name(std::string_view target_triple,
                                   std::string_view logical_name) {
  if (is_apple_darwin_target(target_triple)) {
    return "_" + std::string(logical_name);
  }
  return std::string(logical_name);
}

std::string render_private_data_label(std::string_view target_triple,
                                      std::string_view pool_name) {
  std::string label(pool_name);
  if (!label.empty() && label.front() == '@') {
    label.erase(label.begin());
  }
  while (!label.empty() && label.front() == '.') {
    label.erase(label.begin());
  }
  if (is_apple_darwin_target(target_triple)) {
    return "L" + label;
  }
  return ".L." + label;
}

}  // namespace c4c::backend::x86::abi

#include "x86_target_abi.hpp"

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

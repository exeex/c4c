#include "x86_target_abi.hpp"

namespace c4c::backend::x86::abi {

bool is_x86_target(const c4c::TargetProfile& target_profile) {
  return target_profile.arch == c4c::TargetArch::X86_64 ||
         target_profile.arch == c4c::TargetArch::I686;
}

std::string render_asm_symbol_name(std::string_view target_triple,
                                   std::string_view logical_name) {
  if (target_triple.find("apple-darwin") != std::string_view::npos) {
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
  if (target_triple.find("apple-darwin") != std::string_view::npos) {
    return "L" + label;
  }
  return ".L." + label;
}

}  // namespace c4c::backend::x86::abi

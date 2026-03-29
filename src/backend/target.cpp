#include "target.hpp"

#include <stdexcept>
#include <string>

namespace c4c::backend {

namespace {

std::string_view arch_from_triple(std::string_view target_triple) {
  const size_t dash = target_triple.find('-');
  if (dash == std::string_view::npos) return target_triple;
  return target_triple.substr(0, dash);
}

}  // namespace

Target target_from_triple(std::string_view target_triple) {
  const std::string_view arch = arch_from_triple(target_triple);
  if (arch == "x86_64" || arch == "amd64") return Target::X86_64;
  if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
    return Target::I686;
  }
  if (arch == "aarch64" || arch == "arm64") return Target::Aarch64;
  if (arch == "riscv64" || arch == "riscv64gc") return Target::Riscv64;
  throw std::invalid_argument("unsupported backend target triple: " +
                              std::string(target_triple));
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

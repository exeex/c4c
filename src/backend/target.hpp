#pragma once

#include <string_view>

namespace c4c::backend {

enum class Target {
  X86_64,
  I686,
  Aarch64,
  Riscv64,
};

Target target_from_triple(std::string_view target_triple);
const char* target_name(Target target);

}  // namespace c4c::backend

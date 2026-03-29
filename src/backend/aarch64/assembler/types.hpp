#pragma once

#include <string>

namespace c4c::backend::aarch64::assembler {

struct Operand {
  std::string text;
};

struct AsmStatement {
  std::string text;
};

}  // namespace c4c::backend::aarch64::assembler

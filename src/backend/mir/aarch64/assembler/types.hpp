#pragma once

#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler {

struct Operand {
  std::string text;
};

enum class AsmStatementKind {
  Directive,
  Label,
  Instruction,
};

struct AsmStatement {
  AsmStatementKind kind = AsmStatementKind::Directive;
  std::string text;
  std::string op;
  std::vector<Operand> operands;
  std::string raw_operands;
};

}  // namespace c4c::backend::aarch64::assembler

#pragma once

#include <string>
#include <vector>

namespace c4c::backend::aarch64::assembler {

// Text parser compatibility payloads for external assembler input. These
// string-shaped records are not the future structured encoder/object contract
// for internal compile-through lowering from machine instruction nodes.
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

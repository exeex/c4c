#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace c4c::backend::x86::assembler {

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

std::vector<AsmStatement> parse_asm(const std::string& text);
std::string trim_asm(std::string_view text);

}  // namespace c4c::backend::x86::assembler

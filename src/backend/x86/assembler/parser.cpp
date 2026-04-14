#include "parser.hpp"

#include <cctype>
#include <cstdint>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace c4c::backend::x86::assembler {

std::string trim_asm(std::string_view text) {
  std::size_t begin = 0;
  while (begin < text.size() &&
         (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) {
    ++begin;
  }

  std::size_t end = text.size();
  while (end > begin &&
         (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) {
    --end;
  }

  return std::string(text.substr(begin, end - begin));
}

namespace {

std::string strip_comment(std::string_view line) {
  const auto hash = line.find('#');
  if (hash == std::string_view::npos) return std::string(line);
  return std::string(line.substr(0, hash));
}

bool is_identifier_start(unsigned char c) {
  return std::isalpha(c) || c == '_';
}

bool is_identifier_char(unsigned char c) {
  return std::isalnum(c) || c == '_';
}

bool is_identifier(std::string_view text) {
  if (text.empty()) return false;
  if (!is_identifier_start(static_cast<unsigned char>(text.front()))) return false;
  for (unsigned char c : text) {
    if (!is_identifier_char(c)) return false;
  }
  return true;
}

std::pair<std::string, std::string> split_opcode_and_operands(std::string_view line) {
  const auto split = line.find_first_of(" \t");
  if (split == std::string_view::npos) return {std::string(line), std::string()};
  return {std::string(line.substr(0, split)), trim_asm(line.substr(split + 1))};
}

std::vector<std::string> split_operands(std::string_view text) {
  std::vector<std::string> operands;
  std::string current;
  for (char c : text) {
    if (c == ',') {
      operands.push_back(trim_asm(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  if (!current.empty()) operands.push_back(trim_asm(current));
  return operands;
}

void validate_imm32(std::string_view text) {
  const auto trimmed = trim_asm(text);
  if (trimmed.empty()) {
    throw std::runtime_error("x86 immediate must not be empty");
  }

  std::size_t parsed = 0;
  std::int64_t value = 0;
  try {
    value = std::stoll(trimmed, &parsed, 0);
  } catch (const std::exception&) {
    throw std::runtime_error("unsupported x86 immediate for the Step 2 parser slice: " +
                             trimmed);
  }
  if (parsed != trimmed.size()) {
    throw std::runtime_error("unsupported x86 immediate for the Step 2 parser slice: " +
                             trimmed);
  }
  if (value < std::numeric_limits<std::int32_t>::min() ||
      value > std::numeric_limits<std::int32_t>::max()) {
    throw std::runtime_error("x86 immediate is outside the Step 2 imm32 range: " +
                             trimmed);
  }
}

AsmStatement parse_directive(std::string_view line) {
  AsmStatement statement;
  statement.kind = AsmStatementKind::Directive;
  statement.text = std::string(line);

  if (line == ".intel_syntax noprefix" || line == ".text") {
    statement.op = std::string(line);
    return statement;
  }

  const auto [directive, raw_operands] = split_opcode_and_operands(line);
  if ((directive == ".globl" || directive == ".global") && is_identifier(raw_operands)) {
    statement.op = directive;
    statement.raw_operands = raw_operands;
    statement.operands.push_back(Operand{raw_operands});
    return statement;
  }

  if (directive == ".type") {
    const auto operands = split_operands(raw_operands);
    if (operands.size() == 2 && is_identifier(operands[0]) && operands[1] == "@function") {
      statement.op = directive;
      statement.raw_operands = raw_operands;
      statement.operands.push_back(Operand{operands[0]});
      statement.operands.push_back(Operand{operands[1]});
      return statement;
    }
  }

  throw std::runtime_error("unsupported x86 directive for the Step 2 parser slice: " +
                           std::string(line));
}

AsmStatement parse_label(std::string_view line) {
  const auto label = trim_asm(line.substr(0, line.size() - 1));
  if (!is_identifier(label)) {
    throw std::runtime_error("unsupported x86 label for the Step 2 parser slice: " +
                             label);
  }

  AsmStatement statement;
  statement.kind = AsmStatementKind::Label;
  statement.text = label + ":";
  statement.op = label;
  return statement;
}

AsmStatement parse_instruction(std::string_view line) {
  const auto [opcode, raw_operands] = split_opcode_and_operands(line);

  AsmStatement statement;
  statement.kind = AsmStatementKind::Instruction;
  statement.text = std::string(line);
  statement.op = opcode;
  statement.raw_operands = raw_operands;

  if (opcode == "ret") {
    if (!raw_operands.empty()) {
      throw std::runtime_error("unsupported x86 ret operands for the Step 2 parser slice");
    }
    return statement;
  }

  if (opcode == "call") {
    if (!is_identifier(raw_operands)) {
      throw std::runtime_error(
          "unsupported x86 call target for the Step 2 parser slice: " +
          raw_operands);
    }
    statement.operands.push_back(Operand{raw_operands});
    return statement;
  }

  if (opcode != "mov") {
    throw std::runtime_error("unsupported x86 instruction for the Step 2 parser slice: " +
                             std::string(line));
  }

  const auto operands = split_operands(raw_operands);
  if (operands.size() != 2) {
    throw std::runtime_error("Step 2 x86 mov must have exactly two operands: " +
                             std::string(line));
  }

  if (operands[0] != "eax") {
    throw std::runtime_error(
        "Step 2 x86 mov only supports eax as the destination register");
  }
  validate_imm32(operands[1]);

  statement.operands.push_back(Operand{operands[0]});
  statement.operands.push_back(Operand{operands[1]});
  return statement;
}

}  // namespace

std::vector<AsmStatement> parse_asm(const std::string& text) {
  std::vector<AsmStatement> statements;
  std::stringstream stream(text);
  std::string line;

  bool seen_syntax = false;
  bool seen_text = false;
  bool seen_global = false;
  bool seen_type = false;
  bool seen_label = false;
  std::string global_symbol;
  std::vector<std::string> instruction_ops;

  while (std::getline(stream, line)) {
    const auto trimmed = trim_asm(strip_comment(line));
    if (trimmed.empty()) continue;

    AsmStatement statement;
    if (trimmed.back() == ':') {
      statement = parse_label(trimmed);
      if (!seen_global || seen_label) {
        throw std::runtime_error(
            "Step 2 x86 parser requires exactly one global function label");
      }
      if (statement.op != global_symbol) {
        throw std::runtime_error(
            "Step 2 x86 parser requires the function label to match the declared .globl symbol");
      }
      seen_label = true;
    } else if (trimmed.front() == '.') {
      statement = parse_directive(trimmed);

      if (trimmed == ".intel_syntax noprefix") {
        if (seen_syntax || !statements.empty()) {
          throw std::runtime_error(
              "Step 2 x86 parser requires .intel_syntax noprefix as the first statement");
        }
        seen_syntax = true;
      } else if (trimmed == ".text") {
        if (!seen_syntax || seen_text || seen_global || seen_label) {
          throw std::runtime_error(
              "Step 2 x86 parser requires .text after .intel_syntax noprefix");
        }
        seen_text = true;
      } else if (statement.op == ".type") {
        if (!seen_global || seen_type || seen_label || statement.operands.empty() ||
            statement.operands.front().text != global_symbol) {
          throw std::runtime_error(
              "Step 2 x86 parser only accepts an optional .type for the declared .globl symbol before the label");
        }
        seen_type = true;
      } else {
        if (!seen_text || seen_global || seen_label || statement.operands.empty()) {
          throw std::runtime_error(
              "Step 2 x86 parser requires exactly one .globl after .text");
        }
        seen_global = true;
        global_symbol = statement.operands.front().text;
      }
    } else {
      statement = parse_instruction(trimmed);
      if (!seen_label) {
        throw std::runtime_error(
            "Step 2 x86 parser only accepts instructions after the function label");
      }
      instruction_ops.push_back(statement.op);
    }

    statements.push_back(std::move(statement));
  }

  if (!seen_syntax || !seen_text || !seen_global || !seen_label) {
    throw std::runtime_error("incomplete Step 2 x86 assembly slice");
  }
  if (instruction_ops.size() != 2 ||
      (instruction_ops.front() != "mov" && instruction_ops.front() != "call") ||
      instruction_ops.back() != "ret") {
    throw std::runtime_error(
        "Step 2 x86 parser only supports the bounded mov eax, imm32; ret or call symbol; ret function shapes");
  }

  return statements;
}

}  // namespace c4c::backend::x86::assembler

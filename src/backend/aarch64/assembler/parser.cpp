#include "parser.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace c4c::backend::aarch64::assembler {

enum class SymbolKind {
  Function,
  Object,
  TlsObject,
  NoType,
};

struct SectionDirective {
  std::string name;
  std::optional<std::string> flags;
  std::optional<std::string> section_type;
};

struct SizeExpr {
  enum class Kind {
    Constant,
    CurrentMinusSymbol,
  } kind = Kind::Constant;
  std::uint64_t constant = 0;
  std::string symbol;
};

struct DataValue {
  enum class Kind {
    Integer,
    Symbol,
    SymbolOffset,
    SymbolDiff,
    SymbolDiffAddend,
    Expr,
  } kind = Kind::Integer;
  std::int64_t integer = 0;
  std::string a;
  std::string b;
  std::int64_t addend = 0;
  std::string expr;
};

struct AsmDirective {
  std::string text;
};

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

std::vector<Operand> parse_operands(const std::string& raw_operands) {
  std::vector<Operand> operands;
  std::size_t pos = 0;
  while (pos < raw_operands.size()) {
    const auto comma = raw_operands.find(',', pos);
    const auto token = raw_operands.substr(
        pos, comma == std::string::npos ? std::string::npos : comma - pos);
    const auto trimmed = trim_asm(token);
    if (!trimmed.empty()) {
      operands.push_back(Operand{trimmed});
    }
    if (comma == std::string::npos) {
      break;
    }
    pos = comma + 1;
  }
  return operands;
}

std::vector<AsmStatement> parse_asm(const std::string& text) {
  std::vector<AsmStatement> result;
  std::size_t pos = 0;
  while (pos <= text.size()) {
    const auto next = text.find('\n', pos);
    const auto line = text.substr(pos, next == std::string::npos ? std::string::npos
                                                                  : next - pos);
    const auto trimmed = trim_asm(line);
    if (!trimmed.empty()) {
      if (!trimmed.empty() && trimmed.back() == ':') {
        result.push_back(AsmStatement{
            .kind = AsmStatementKind::Label,
            .text = trimmed.substr(0, trimmed.size() - 1) + ":",
            .op = trimmed.substr(0, trimmed.size() - 1),
        });
      } else {
        const auto split = trimmed.find_first_of(" \t");
        const auto op = split == std::string::npos ? trimmed : trimmed.substr(0, split);
        const auto raw_operands =
            split == std::string::npos ? std::string() : trim_asm(trimmed.substr(split + 1));
        result.push_back(AsmStatement{
            .kind = !op.empty() && op.front() == '.' ? AsmStatementKind::Directive
                                                     : AsmStatementKind::Instruction,
            .text = trimmed,
            .op = op,
            .operands = parse_operands(raw_operands),
            .raw_operands = raw_operands,
        });
      }
    }
    if (next == std::string::npos) {
      break;
    }
    pos = next + 1;
  }
  return result;
}

}  // namespace c4c::backend::aarch64::assembler

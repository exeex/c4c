#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

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

struct Operand {
  std::string text;
};

struct AsmDirective {
  std::string text;
};

struct AsmStatement {
  std::string text;
};

std::vector<AsmStatement> parse_asm(const std::string& text) {
  std::vector<AsmStatement> result;
  result.push_back(AsmStatement{text});
  return result;
}

}  // namespace c4c::backend::aarch64::assembler

#ifndef TINYC2LL_FRONTEND_CXX_PP_COND_HPP
#define TINYC2LL_FRONTEND_CXX_PP_COND_HPP

#include <functional>
#include <string>

namespace tinyc2ll::frontend_cxx {

// Resolve `defined(NAME)`, `__has_builtin(X)`, etc. in a #if expression string.
// Replaces them with "0" or "1" literals.
std::string resolve_defined_and_intrinsics(
    const std::string& expr,
    const std::function<bool(const std::string&)>& is_defined_name);

struct ExprValue {
  unsigned long long bits = 0;
  bool is_unsigned = false;
};

ExprValue make_signed(long long v);
ExprValue make_unsigned(unsigned long long v);
long long as_signed(ExprValue v);
bool as_bool(ExprValue v);

// Evaluate a C preprocessor constant expression (after defined/intrinsic resolution
// and macro expansion). Sets *ok = false on parse failure.
class IfExprParser {
public:
  IfExprParser(const std::string& text,
               const std::function<bool(const std::string&)>& is_defined_name);

  ExprValue parse(bool* ok);

private:
  void skip_spaces();
  bool consume(const char* op);
  bool consume_single(char op, char disallow_if_next_same);

  ExprValue parse_conditional();
  ExprValue parse_logical_or();
  ExprValue parse_logical_and();
  ExprValue parse_bitor();
  ExprValue parse_bitxor();
  ExprValue parse_bitand();
  ExprValue parse_equality();
  ExprValue parse_relational();
  ExprValue parse_shift();
  ExprValue parse_additive();
  ExprValue parse_multiplicative();
  ExprValue parse_unary();
  ExprValue parse_primary();
  ExprValue parse_char_literal();
  ExprValue parse_int_literal();

  const std::string& text_;
  size_t pos_ = 0;
  bool ok_ = true;
  std::function<bool(const std::string&)> is_defined_name_;
};

}  // namespace tinyc2ll::frontend_cxx

#endif

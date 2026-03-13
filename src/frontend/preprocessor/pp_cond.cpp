#include "pp_cond.hpp"
#include "pp_text.hpp"

#include <cctype>
#include <cstdlib>
#include <limits>

namespace tinyc2ll::frontend_cxx {

namespace {

bool is_word_boundary(const std::string& s, size_t pos) {
  if (pos >= s.size()) return true;
  return !is_ident_continue(s[pos]);
}

std::string parse_identifier_token(const std::string& s, size_t* i) {
  if (*i >= s.size() || !is_ident_start(s[*i])) return std::string();
  size_t b = (*i)++;
  while (*i < s.size() && is_ident_continue(s[*i])) ++(*i);
  return s.substr(b, *i - b);
}

void skip_ws(const std::string& s, size_t* i) {
  while (*i < s.size() && std::isspace(static_cast<unsigned char>(s[*i]))) ++(*i);
}

}  // namespace

std::string resolve_defined_and_intrinsics(
    const std::string& expr,
    const std::function<bool(const std::string&)>& is_defined_name) {
  std::string out;
  out.reserve(expr.size() + 8);

  size_t i = 0;
  while (i < expr.size()) {
    if (is_ident_start(expr[i])) {
      size_t word_begin = i;
      std::string ident = parse_identifier_token(expr, &i);

      auto parse_intrinsic_arg = [&](bool* ok, std::string* arg_ident) {
        size_t j = i;
        skip_ws(expr, &j);
        if (j >= expr.size() || expr[j] != '(') {
          *ok = false;
          return;
        }
        ++j;
        skip_ws(expr, &j);
        *arg_ident = parse_identifier_token(expr, &j);
        skip_ws(expr, &j);
        if (j >= expr.size() || expr[j] != ')') {
          *ok = false;
          return;
        }
        ++j;
        i = j;
        *ok = true;
      };

      if (ident == "defined" && is_word_boundary(expr, i)) {
        size_t j = i;
        skip_ws(expr, &j);
        bool has_paren = (j < expr.size() && expr[j] == '(');
        std::string name;

        if (has_paren) {
          ++j;
          skip_ws(expr, &j);
          name = parse_identifier_token(expr, &j);
          skip_ws(expr, &j);
          if (j < expr.size() && expr[j] == ')') ++j;
          i = j;
        } else {
          name = parse_identifier_token(expr, &j);
          i = j;
        }
        out += (is_defined_name(name) ? "1" : "0");
        continue;
      }

      if ((ident == "__has_builtin" || ident == "__has_attribute" ||
           ident == "__has_feature" || ident == "__has_extension" ||
           ident == "__has_include" || ident == "__has_include_next") &&
          is_word_boundary(expr, i)) {
        bool ok = false;
        std::string arg;
        parse_intrinsic_arg(&ok, &arg);
        if (ok) {
          out += "0";
          continue;
        }
      }

      out.append(expr, word_begin, i - word_begin);
      continue;
    }

    out.push_back(expr[i++]);
  }

  return out;
}

ExprValue make_signed(long long v) {
  return ExprValue{static_cast<unsigned long long>(v), false};
}

ExprValue make_unsigned(unsigned long long v) {
  return ExprValue{v, true};
}

long long as_signed(ExprValue v) {
  return static_cast<long long>(v.bits);
}

bool as_bool(ExprValue v) {
  return v.bits != 0;
}

IfExprParser::IfExprParser(const std::string& text,
                           const std::function<bool(const std::string&)>& is_defined_name)
    : text_(text), is_defined_name_(is_defined_name) {}

ExprValue IfExprParser::parse(bool* ok) {
  ExprValue v = parse_conditional();
  skip_spaces();
  if (pos_ != text_.size()) ok_ = false;
  if (ok) *ok = ok_;
  return v;
}

void IfExprParser::skip_spaces() {
  while (pos_ < text_.size() &&
         std::isspace(static_cast<unsigned char>(text_[pos_]))) {
    ++pos_;
  }
}

bool IfExprParser::consume(const char* op) {
  skip_spaces();
  size_t i = pos_;
  size_t k = 0;
  while (op[k] != '\0') {
    if (i >= text_.size() || text_[i] != op[k]) return false;
    ++i;
    ++k;
  }
  pos_ = i;
  return true;
}

bool IfExprParser::consume_single(char op, char disallow_if_next_same) {
  skip_spaces();
  if (pos_ >= text_.size() || text_[pos_] != op) return false;
  if (pos_ + 1 < text_.size() && text_[pos_ + 1] == disallow_if_next_same) {
    return false;
  }
  ++pos_;
  return true;
}

ExprValue IfExprParser::parse_conditional() {
  ExprValue c = parse_logical_or();
  skip_spaces();
  if (!consume("?")) return c;
  ExprValue t = parse_conditional();
  if (!consume(":")) {
    ok_ = false;
    return make_signed(0);
  }
  ExprValue f = parse_conditional();
  return as_bool(c) ? t : f;
}

ExprValue IfExprParser::parse_logical_or() {
  ExprValue lhs = parse_logical_and();
  while (consume("||")) {
    ExprValue rhs = parse_logical_and();
    lhs = make_signed(as_bool(lhs) || as_bool(rhs));
  }
  return lhs;
}

ExprValue IfExprParser::parse_logical_and() {
  ExprValue lhs = parse_bitor();
  while (consume("&&")) {
    ExprValue rhs = parse_bitor();
    lhs = make_signed(as_bool(lhs) && as_bool(rhs));
  }
  return lhs;
}

ExprValue IfExprParser::parse_bitor() {
  ExprValue lhs = parse_bitxor();
  while (consume_single('|', '|')) {
    ExprValue rhs = parse_bitxor();
    if (lhs.is_unsigned || rhs.is_unsigned) {
      lhs = make_unsigned(lhs.bits | rhs.bits);
    } else {
      lhs = make_signed(as_signed(lhs) | as_signed(rhs));
    }
  }
  return lhs;
}

ExprValue IfExprParser::parse_bitxor() {
  ExprValue lhs = parse_bitand();
  while (consume("^")) {
    ExprValue rhs = parse_bitand();
    if (lhs.is_unsigned || rhs.is_unsigned) {
      lhs = make_unsigned(lhs.bits ^ rhs.bits);
    } else {
      lhs = make_signed(as_signed(lhs) ^ as_signed(rhs));
    }
  }
  return lhs;
}

ExprValue IfExprParser::parse_bitand() {
  ExprValue lhs = parse_equality();
  while (consume_single('&', '&')) {
    ExprValue rhs = parse_equality();
    if (lhs.is_unsigned || rhs.is_unsigned) {
      lhs = make_unsigned(lhs.bits & rhs.bits);
    } else {
      lhs = make_signed(as_signed(lhs) & as_signed(rhs));
    }
  }
  return lhs;
}

ExprValue IfExprParser::parse_equality() {
  ExprValue lhs = parse_relational();
  while (true) {
    if (consume("==")) {
      ExprValue rhs = parse_relational();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits == rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) == as_signed(rhs));
      }
    } else if (consume("!=")) {
      ExprValue rhs = parse_relational();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits != rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) != as_signed(rhs));
      }
    } else {
      return lhs;
    }
  }
}

ExprValue IfExprParser::parse_relational() {
  ExprValue lhs = parse_shift();
  while (true) {
    if (consume("<=")) {
      ExprValue rhs = parse_shift();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits <= rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) <= as_signed(rhs));
      }
    } else if (consume(">=")) {
      ExprValue rhs = parse_shift();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits >= rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) >= as_signed(rhs));
      }
    } else if (consume("<")) {
      ExprValue rhs = parse_shift();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits < rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) < as_signed(rhs));
      }
    } else if (consume(">")) {
      ExprValue rhs = parse_shift();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_signed(lhs.bits > rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) > as_signed(rhs));
      }
    } else {
      return lhs;
    }
  }
}

ExprValue IfExprParser::parse_shift() {
  ExprValue lhs = parse_additive();
  while (true) {
    if (consume("<<")) {
      ExprValue rhs = parse_additive();
      unsigned long long sh = rhs.bits & 63ULL;
      if (lhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits << sh);
      } else {
        lhs = make_signed(as_signed(lhs) << sh);
      }
    } else if (consume(">>")) {
      ExprValue rhs = parse_additive();
      unsigned long long sh = rhs.bits & 63ULL;
      if (lhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits >> sh);
      } else {
        lhs = make_signed(as_signed(lhs) >> sh);
      }
    } else {
      return lhs;
    }
  }
}

ExprValue IfExprParser::parse_additive() {
  ExprValue lhs = parse_multiplicative();
  while (true) {
    if (consume("+")) {
      ExprValue rhs = parse_multiplicative();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits + rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) + as_signed(rhs));
      }
    } else if (consume("-")) {
      ExprValue rhs = parse_multiplicative();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits - rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) - as_signed(rhs));
      }
    } else {
      return lhs;
    }
  }
}

ExprValue IfExprParser::parse_multiplicative() {
  ExprValue lhs = parse_unary();
  while (true) {
    if (consume("*")) {
      ExprValue rhs = parse_unary();
      if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits * rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) * as_signed(rhs));
      }
    } else if (consume("/")) {
      ExprValue rhs = parse_unary();
      if (rhs.bits == 0) {
        ok_ = false;
        lhs = make_signed(0);
      } else if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits / rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) / as_signed(rhs));
      }
    } else if (consume("%")) {
      ExprValue rhs = parse_unary();
      if (rhs.bits == 0) {
        ok_ = false;
        lhs = make_signed(0);
      } else if (lhs.is_unsigned || rhs.is_unsigned) {
        lhs = make_unsigned(lhs.bits % rhs.bits);
      } else {
        lhs = make_signed(as_signed(lhs) % as_signed(rhs));
      }
    } else {
      return lhs;
    }
  }
}

ExprValue IfExprParser::parse_unary() {
  if (consume("+")) return parse_unary();
  if (consume("-")) {
    ExprValue v = parse_unary();
    if (v.is_unsigned) return make_unsigned(0ULL - v.bits);
    return make_signed(-as_signed(v));
  }
  if (consume("!")) {
    ExprValue v = parse_unary();
    return make_signed(!as_bool(v));
  }
  if (consume("~")) {
    ExprValue v = parse_unary();
    if (v.is_unsigned) return make_unsigned(~v.bits);
    return make_signed(~as_signed(v));
  }
  return parse_primary();
}

ExprValue IfExprParser::parse_primary() {
  skip_spaces();
  if (pos_ >= text_.size()) {
    ok_ = false;
    return make_signed(0);
  }

  if (consume("(")) {
    ExprValue v = parse_conditional();
    if (!consume(")")) ok_ = false;
    return v;
  }

  if (is_ident_start(text_[pos_])) {
    size_t b = pos_;
    std::string ident;
    ident.push_back(text_[pos_++]);
    while (pos_ < text_.size() && is_ident_continue(text_[pos_]))
      ident.push_back(text_[pos_++]);

    if (ident == "defined") {
      skip_spaces();
      std::string name;
      if (consume("(")) {
        skip_spaces();
        while (pos_ < text_.size() && is_ident_continue(text_[pos_]))
          name.push_back(text_[pos_++]);
        skip_spaces();
        if (!consume(")")) ok_ = false;
      } else {
        while (pos_ < text_.size() && is_ident_continue(text_[pos_]))
          name.push_back(text_[pos_++]);
      }
      return make_signed(is_defined_name_(name) ? 1 : 0);
    }
    return make_signed(0);
  }

  if (text_[pos_] == '\'') {
    return parse_char_literal();
  }

  return parse_int_literal();
}

ExprValue IfExprParser::parse_char_literal() {
  skip_spaces();
  if (pos_ >= text_.size() || text_[pos_] != '\'') {
    ok_ = false;
    return make_signed(0);
  }
  ++pos_;
  if (pos_ >= text_.size()) {
    ok_ = false;
    return make_signed(0);
  }

  unsigned int ch = 0;
  if (text_[pos_] == '\\') {
    ++pos_;
    if (pos_ >= text_.size()) {
      ok_ = false;
      return make_signed(0);
    }
    char e = text_[pos_++];
    switch (e) {
      case 'n': ch = '\n'; break;
      case 'r': ch = '\r'; break;
      case 't': ch = '\t'; break;
      case '\\': ch = '\\'; break;
      case '\'': ch = '\''; break;
      case '0': ch = '\0'; break;
      default: ch = static_cast<unsigned char>(e); break;
    }
  } else {
    ch = static_cast<unsigned char>(text_[pos_++]);
  }

  if (pos_ >= text_.size() || text_[pos_] != '\'') {
    ok_ = false;
    return make_signed(0);
  }
  ++pos_;
  return make_signed(static_cast<long long>(ch));
}

ExprValue IfExprParser::parse_int_literal() {
  skip_spaces();
  size_t b = pos_;
  if (pos_ < text_.size() && (text_[pos_] == '0') &&
      (pos_ + 1 < text_.size()) &&
      (text_[pos_ + 1] == 'x' || text_[pos_ + 1] == 'X')) {
    pos_ += 2;
    while (pos_ < text_.size() &&
           std::isxdigit(static_cast<unsigned char>(text_[pos_]))) {
      ++pos_;
    }
  } else {
    while (pos_ < text_.size() &&
           std::isdigit(static_cast<unsigned char>(text_[pos_]))) {
      ++pos_;
    }
  }

  if (b == pos_) {
    ok_ = false;
    return make_signed(0);
  }

  size_t digits_end = pos_;
  bool is_unsigned = false;
  while (pos_ < text_.size()) {
    char c = text_[pos_];
    if (c == 'u' || c == 'U') {
      is_unsigned = true;
      ++pos_;
    } else if (c == 'l' || c == 'L') {
      ++pos_;
    } else {
      break;
    }
  }

  std::string token = text_.substr(b, digits_end - b);
  int base = 10;
  if (token.size() > 2 && token[0] == '0' &&
      (token[1] == 'x' || token[1] == 'X')) {
    base = 16;
  } else if (token.size() > 1 && token[0] == '0') {
    base = 8;
  }

  char* end = nullptr;
  unsigned long long v = std::strtoull(token.c_str(), &end, base);
  if (end == nullptr || *end != '\0') {
    ok_ = false;
    return make_signed(0);
  }

  if (!is_unsigned &&
      v > static_cast<unsigned long long>(std::numeric_limits<long long>::max())) {
    is_unsigned = true;
  }
  return is_unsigned ? make_unsigned(v) : make_signed(static_cast<long long>(v));
}

}  // namespace tinyc2ll::frontend_cxx

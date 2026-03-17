#include "pp_cond.hpp"
#include "pp_text.hpp"

#include <cctype>
#include <cstdlib>
#include <limits>

namespace c4c {

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
    const std::function<bool(const std::string&)>& is_defined_name,
    const HasIncludeCallback& has_include) {
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

      if ((ident == "__has_include" || ident == "__has_include_next") &&
          is_word_boundary(expr, i)) {
        // Parse __has_include(<path>) or __has_include("path")
        size_t j = i;
        skip_ws(expr, &j);
        if (j < expr.size() && expr[j] == '(') {
          ++j;
          skip_ws(expr, &j);
          // Collect everything until matching ')'
          size_t arg_start = j;
          int depth = 0;
          while (j < expr.size()) {
            if (expr[j] == '(') ++depth;
            else if (expr[j] == ')') {
              if (depth == 0) break;
              --depth;
            }
            ++j;
          }
          std::string arg = trim_copy(expr.substr(arg_start, j - arg_start));
          if (j < expr.size()) ++j;  // skip ')'
          i = j;

          if (has_include && !arg.empty()) {
            out += (has_include(arg) ? "1" : "0");
          } else {
            out += "0";
          }
          continue;
        }
      }

      if (ident == "__has_builtin" && is_word_boundary(expr, i)) {
        bool ok = false;
        std::string arg;
        parse_intrinsic_arg(&ok, &arg);
        if (ok) {
          // Common GCC/Clang builtins we support or that code commonly checks for.
          static const char* known_builtins[] = {
            "__builtin_expect", "__builtin_unreachable", "__builtin_trap",
            "__builtin_clz", "__builtin_clzl", "__builtin_clzll",
            "__builtin_ctz", "__builtin_ctzl", "__builtin_ctzll",
            "__builtin_popcount", "__builtin_popcountl", "__builtin_popcountll",
            "__builtin_bswap16", "__builtin_bswap32", "__builtin_bswap64",
            "__builtin_constant_p", "__builtin_types_compatible_p",
            "__builtin_choose_expr", "__builtin_offsetof",
            "__builtin_va_start", "__builtin_va_end", "__builtin_va_arg",
            "__builtin_va_copy", "__builtin_va_list",
            "__builtin_memcpy", "__builtin_memset", "__builtin_memmove",
            "__builtin_strlen", "__builtin_strcmp", "__builtin_strcpy",
            "__builtin_abs", "__builtin_labs", "__builtin_llabs",
            "__builtin_fabs", "__builtin_fabsf", "__builtin_fabsl",
            "__builtin_huge_val", "__builtin_huge_valf", "__builtin_inf",
            "__builtin_inff", "__builtin_nan", "__builtin_nanf",
            "__builtin_isnan", "__builtin_isinf", "__builtin_isfinite",
            "__builtin_add_overflow", "__builtin_sub_overflow", "__builtin_mul_overflow",
            nullptr
          };
          bool found = false;
          for (const char** p = known_builtins; *p; ++p) {
            if (arg == *p) { found = true; break; }
          }
          out += (found ? "1" : "0");
          continue;
        }
      }

      if (ident == "__has_attribute" && is_word_boundary(expr, i)) {
        bool ok = false;
        std::string arg;
        parse_intrinsic_arg(&ok, &arg);
        if (ok) {
          static const char* known_attrs[] = {
            "aligned", "packed", "unused", "used", "weak", "alias",
            "deprecated", "visibility", "format", "format_arg",
            "noinline", "always_inline", "noreturn", "nonnull",
            "warn_unused_result", "constructor", "destructor",
            "section", "cleanup", "cold", "hot", "pure", "const",
            "malloc", "sentinel", "may_alias", "transparent_union",
            "vector_size", "__vector_size__",
            nullptr
          };
          bool found = false;
          for (const char** p = known_attrs; *p; ++p) {
            if (arg == *p) { found = true; break; }
          }
          out += (found ? "1" : "0");
          continue;
        }
      }

      if ((ident == "__has_feature" || ident == "__has_extension") &&
          is_word_boundary(expr, i)) {
        bool ok = false;
        std::string arg;
        parse_intrinsic_arg(&ok, &arg);
        if (ok) {
          // We support a small set of C features/extensions.
          static const char* known_features[] = {
            "c_alignas", "c_alignof", "c_atomic", "c_generic_selections",
            "c_static_assert", "c_thread_local",
            nullptr
          };
          bool found = false;
          for (const char** p = known_features; *p; ++p) {
            if (arg == *p) { found = true; break; }
          }
          out += (found ? "1" : "0");
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
    // L'x', u'x', U'x' — prefixed character literals
    if ((ident == "L" || ident == "u" || ident == "U") &&
        pos_ < text_.size() && text_[pos_] == '\'') {
      return parse_char_literal();
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
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7': {
        // Octal escape: up to 3 digits
        ch = static_cast<unsigned int>(e - '0');
        for (int d = 0; d < 2 && pos_ < text_.size() &&
             text_[pos_] >= '0' && text_[pos_] <= '7'; ++d)
          ch = ch * 8 + static_cast<unsigned int>(text_[pos_++] - '0');
        break;
      }
      case 'x': {
        // Hex escape
        ch = 0;
        while (pos_ < text_.size() &&
               std::isxdigit(static_cast<unsigned char>(text_[pos_]))) {
          char hc = text_[pos_++];
          unsigned int hv = (hc >= '0' && hc <= '9') ? (hc - '0') :
                            (hc >= 'a' && hc <= 'f') ? (hc - 'a' + 10) :
                            (hc - 'A' + 10);
          ch = ch * 16 + hv;
        }
        break;
      }
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

}  // namespace c4c

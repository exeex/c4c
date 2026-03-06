#include "preprocessor.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>
#include <vector>

namespace tinyc2ll::frontend_cxx {

namespace {

std::string trim_copy(const std::string& s) {
  size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

bool is_ident_start(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

bool is_ident_continue(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// ---- Function-like macro expansion helpers ---------------------------------

// Collect balanced parenthesised argument list starting at (*pos) which
// must point at the character AFTER the opening '('. On return *pos points
// at the character after the closing ')'.  Respects string/char literals.
// Returns one string per argument (raw, un-expanded).
std::vector<std::string> collect_funclike_args(const std::string& line, size_t* pos) {
  std::vector<std::string> args;
  std::string cur;
  int depth = 0;
  bool in_str = false, in_chr = false;

  while (*pos < line.size()) {
    char c = line[*pos];
    if (!in_str && !in_chr) {
      if (c == '(') { ++depth; cur.push_back(c); ++(*pos); continue; }
      if (c == ')') {
        if (depth == 0) {
          args.push_back(cur);
          ++(*pos);
          return args;
        }
        --depth; cur.push_back(c); ++(*pos); continue;
      }
      if (c == ',' && depth == 0) {
        args.push_back(cur);
        cur.clear();
        ++(*pos);
        continue;
      }
    }
    if (!in_chr && c == '"') {
      // Track string start/end, handle escape sequences
      in_str = !in_str;
    } else if (!in_str && c == '\'') {
      in_chr = !in_chr;
    }
    if (in_str || in_chr) {
      // Inside string/char literal: copy verbatim including escapes
      cur.push_back(c);
      ++(*pos);
      if (c == '\\' && *pos < line.size()) {
        cur.push_back(line[*pos]);
        ++(*pos);
      }
      // Check for closing quote
      if (*pos < line.size()) {
        char q = line[*pos];
        if ((in_str && q == '"') || (in_chr && q == '\'')) {
          cur.push_back(q);
          ++(*pos);
          if (in_str) in_str = false;
          else in_chr = false;
        }
      }
      continue;
    }
    cur.push_back(c);
    ++(*pos);
  }
  // Unbalanced — return whatever we collected
  if (!cur.empty() || !args.empty()) args.push_back(cur);
  return args;
}

// Stringify a raw argument for the # operator (C11 6.10.3.2).
std::string stringify_arg(const std::string& raw) {
  std::string r = trim_copy(raw);
  std::string out;
  out.push_back('"');
  for (char c : r) {
    if (c == '\\' || c == '"') out.push_back('\\');
    out.push_back(c);
  }
  out.push_back('"');
  return out;
}

// Look up a named parameter in the macro definition.
// Returns the index (0-based), or -1 if not found.
int find_param_idx(const std::vector<std::string>& params, const std::string& name) {
  for (int i = 0; i < static_cast<int>(params.size()); ++i) {
    if (params[i] == name) return i;
  }
  return -1;
}

// Perform parameter substitution on the macro body, implementing:
//   #param  → stringify raw arg
//   a##b    → token paste (using raw args for ## operands)
//   param   → prescan-expanded arg
//   __VA_ARGS__ → variadic args (expanded)
std::string substitute_funclike_body(const std::string& body,
                                     const std::vector<std::string>& params,
                                     const std::vector<std::string>& raw_args,
                                     const std::vector<std::string>& exp_args,
                                     bool variadic,
                                     const std::string& va_raw,
                                     const std::string& va_exp);

std::string substitute_funclike_body(const std::string& body,
                                     const std::vector<std::string>& params,
                                     const std::vector<std::string>& raw_args,
                                     const std::vector<std::string>& exp_args,
                                     bool variadic,
                                     const std::string& va_raw,
                                     const std::string& va_exp) {
  std::string out;
  size_t i = 0;

  auto get_raw = [&](const std::string& name) -> std::string {
    if (variadic && name == "__VA_ARGS__") return va_raw;
    int idx = find_param_idx(params, name);
    return (idx >= 0 && idx < static_cast<int>(raw_args.size())) ? trim_copy(raw_args[idx]) : name;
  };
  auto get_exp = [&](const std::string& name) -> std::string {
    if (variadic && name == "__VA_ARGS__") return va_exp;
    int idx = find_param_idx(params, name);
    return (idx >= 0 && idx < static_cast<int>(exp_args.size())) ? exp_args[idx] : name;
  };

  while (i < body.size()) {
    // String literal in body — copy verbatim
    if (body[i] == '"') {
      out.push_back('"');
      ++i;
      while (i < body.size() && body[i] != '"') {
        if (body[i] == '\\') { out.push_back(body[i++]); }
        if (i < body.size()) out.push_back(body[i++]);
      }
      if (i < body.size()) { out.push_back('"'); ++i; }
      continue;
    }

    // # operator (stringify)
    if (body[i] == '#' && i + 1 < body.size() && body[i + 1] != '#') {
      size_t j = i + 1;
      while (j < body.size() && (body[j] == ' ' || body[j] == '\t')) ++j;
      if (j < body.size() && is_ident_start(body[j])) {
        size_t k = j + 1;
        while (k < body.size() && is_ident_continue(body[k])) ++k;
        std::string pname = body.substr(j, k - j);
        bool is_param = (find_param_idx(params, pname) >= 0) ||
                        (variadic && pname == "__VA_ARGS__");
        if (is_param) {
          out += stringify_arg(get_raw(pname));
          i = k;
          continue;
        }
      }
      out.push_back('#');
      ++i;
      continue;
    }

    // Identifier — check for ## paste or parameter substitution
    if (is_ident_start(body[i])) {
      size_t j = i + 1;
      while (j < body.size() && is_ident_continue(body[j])) ++j;
      std::string tok = body.substr(i, j - i);

      // Check: is this identifier a character/string literal prefix?
      // Tokens like L'x', u"s", U'x', u8"s" are single pp-tokens; the
      // prefix (L/u/U/u8) is NOT a macro-parameter reference.
      if (j < body.size() && (body[j] == '\'' || body[j] == '"')) {
        out += tok;  // emit prefix verbatim (not substituted)
        char delim = body[j];
        out.push_back(body[j++]);  // opening quote
        while (j < body.size() && body[j] != delim) {
          if (body[j] == '\\') { out.push_back(body[j++]); }
          if (j < body.size()) out.push_back(body[j++]);
        }
        if (j < body.size()) { out.push_back(body[j++]); }  // closing quote
        i = j;
        continue;
      }

      // Is there ## following this token?
      size_t k = j;
      while (k < body.size() && (body[k] == ' ' || body[k] == '\t')) ++k;
      if (k + 1 < body.size() && body[k] == '#' && body[k + 1] == '#') {
        // Use raw arg for ## left operand
        std::string left = get_raw(tok);
        // Do NOT trim trailing whitespace from out: whitespace before the ## expression
        // in the body (e.g. the space between 'A' and 's' in "struct S s##S") must be
        // preserved.  The look-ahead above already skipped whitespace between the
        // identifier and ##, so nothing extra needs removing here.
        out += trim_copy(left);
        // Skip ##
        k += 2;
        while (k < body.size() && (body[k] == ' ' || body[k] == '\t')) ++k;
        // Right operand: get next token raw form
        if (k < body.size() && is_ident_start(body[k])) {
          size_t m = k + 1;
          while (m < body.size() && is_ident_continue(body[m])) ++m;
          std::string rtok = body.substr(k, m - k);
          out += get_raw(rtok);
          i = m;
        } else {
          // non-ident right operand: copy one char
          if (k < body.size()) { out.push_back(body[k]); i = k + 1; }
          else i = k;
        }
        continue;
      }

      // Normal parameter substitution (use pre-scan expanded arg)
      out += get_exp(tok);
      i = j;
      continue;
    }

    // ## without left-operand identifier (rare — just emit ##)
    if (body[i] == '#' && i + 1 < body.size() && body[i + 1] == '#') {
      // Strip trailing spaces and skip ##
      while (!out.empty() && (out.back() == ' ' || out.back() == '\t')) out.pop_back();
      i += 2;
      while (i < body.size() && (body[i] == ' ' || body[i] == '\t')) ++i;
      // Right operand
      if (i < body.size() && is_ident_start(body[i])) {
        size_t j = i + 1;
        while (j < body.size() && is_ident_continue(body[j])) ++j;
        std::string rtok = body.substr(i, j - i);
        out += get_raw(rtok);
        i = j;
      } else if (i < body.size()) {
        out.push_back(body[i++]);
      }
      continue;
    }

    out.push_back(body[i++]);
  }

  return out;
}

// ----------------------------------------------------------------------------

std::pair<std::string, std::string> split_directive(const std::string& line) {
  size_t i = 0;
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
  size_t k0 = i;
  while (i < line.size() && is_ident_continue(line[i])) ++i;
  std::string key = line.substr(k0, i - k0);
  while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
  std::string rest = (i < line.size()) ? line.substr(i) : std::string();
  return {key, rest};
}

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
          // TODO(preprocessor): wire __has_include* to include search logic and
          // populate builtin/attribute feature tables.
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

struct ExprValue {
  unsigned long long bits = 0;
  bool is_unsigned = false;
};

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

class IfExprParser {
public:
  IfExprParser(const std::string& text,
               const std::function<bool(const std::string&)>& is_defined_name)
      : text_(text), is_defined_name_(is_defined_name) {}

  ExprValue parse(bool* ok) {
    ExprValue v = parse_conditional();
    skip_spaces();
    if (pos_ != text_.size()) ok_ = false;
    if (ok) *ok = ok_;
    return v;
  }

private:
  void skip_spaces() {
    while (pos_ < text_.size() &&
           std::isspace(static_cast<unsigned char>(text_[pos_]))) {
      ++pos_;
    }
  }

  bool consume(const char* op) {
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

  bool consume_single(char op, char disallow_if_next_same) {
    skip_spaces();
    if (pos_ >= text_.size() || text_[pos_] != op) return false;
    if (pos_ + 1 < text_.size() && text_[pos_ + 1] == disallow_if_next_same) {
      return false;
    }
    ++pos_;
    return true;
  }

  ExprValue parse_conditional() {
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

  ExprValue parse_logical_or() {
    ExprValue lhs = parse_logical_and();
    while (consume("||")) {
      ExprValue rhs = parse_logical_and();
      lhs = make_signed(as_bool(lhs) || as_bool(rhs));
    }
    return lhs;
  }

  ExprValue parse_logical_and() {
    ExprValue lhs = parse_bitor();
    while (consume("&&")) {
      ExprValue rhs = parse_bitor();
      lhs = make_signed(as_bool(lhs) && as_bool(rhs));
    }
    return lhs;
  }

  ExprValue parse_bitor() {
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

  ExprValue parse_bitxor() {
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

  ExprValue parse_bitand() {
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

  ExprValue parse_equality() {
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

  ExprValue parse_relational() {
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

  ExprValue parse_shift() {
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

  ExprValue parse_additive() {
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

  ExprValue parse_multiplicative() {
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

  ExprValue parse_unary() {
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

  ExprValue parse_primary() {
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
      std::string ident = parse_identifier_token(text_, &pos_);
      if (ident == "defined") {
        skip_spaces();
        std::string name;
        if (consume("(")) {
          skip_spaces();
          name = parse_identifier_token(text_, &pos_);
          skip_spaces();
          if (!consume(")")) ok_ = false;
        } else {
          name = parse_identifier_token(text_, &pos_);
        }
        return make_signed(is_defined_name_(name) ? 1 : 0);
      }
      // C preprocessor rule: remaining identifiers evaluate to 0.
      return make_signed(0);
    }

    if (text_[pos_] == '\'') {
      return parse_char_literal();
    }

    return parse_int_literal();
  }

  ExprValue parse_char_literal() {
    // Basic char literal parsing for #if constant expressions.
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

  ExprValue parse_int_literal() {
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

private:
  const std::string& text_;
  size_t pos_ = 0;
  bool ok_ = true;
  std::function<bool(const std::string&)> is_defined_name_;
};

}  // namespace

Preprocessor::Preprocessor() {
  // Minimal predefined macros for framework bring-up.
  macros_["__STDC__"] = MacroDef{"__STDC__", false, false, {}, "1"};
  macros_["__STDC_VERSION__"] =
      MacroDef{"__STDC_VERSION__", false, false, {}, "201710L"};
  // Type size macros (LP64 / ILP32 targets: int=4, long=8, pointer=8)
  macros_["__SIZEOF_INT__"]       = MacroDef{"__SIZEOF_INT__",       false, false, {}, "4"};
  macros_["__SIZEOF_SHORT__"]     = MacroDef{"__SIZEOF_SHORT__",     false, false, {}, "2"};
  macros_["__SIZEOF_LONG__"]      = MacroDef{"__SIZEOF_LONG__",      false, false, {}, "8"};
  macros_["__SIZEOF_LONG_LONG__"] = MacroDef{"__SIZEOF_LONG_LONG__", false, false, {}, "8"};
  macros_["__SIZEOF_POINTER__"]   = MacroDef{"__SIZEOF_POINTER__",   false, false, {}, "8"};
  macros_["__SIZEOF_FLOAT__"]     = MacroDef{"__SIZEOF_FLOAT__",     false, false, {}, "4"};
  macros_["__SIZEOF_DOUBLE__"]    = MacroDef{"__SIZEOF_DOUBLE__",    false, false, {}, "8"};
  macros_["__SIZEOF_SIZE_T__"]    = MacroDef{"__SIZEOF_SIZE_T__",    false, false, {}, "8"};
  // GCC built-in type macros (LP64 / arm64)
  macros_["__SIZE_TYPE__"]       = MacroDef{"__SIZE_TYPE__",       false, false, {}, "long unsigned int"};
  macros_["__PTRDIFF_TYPE__"]    = MacroDef{"__PTRDIFF_TYPE__",    false, false, {}, "long int"};
  macros_["__WCHAR_TYPE__"]      = MacroDef{"__WCHAR_TYPE__",      false, false, {}, "int"};
  macros_["__WINT_TYPE__"]       = MacroDef{"__WINT_TYPE__",       false, false, {}, "int"};
  macros_["__INTMAX_TYPE__"]     = MacroDef{"__INTMAX_TYPE__",     false, false, {}, "long int"};
  macros_["__UINTMAX_TYPE__"]    = MacroDef{"__UINTMAX_TYPE__",    false, false, {}, "long unsigned int"};
  macros_["__INTPTR_TYPE__"]     = MacroDef{"__INTPTR_TYPE__",     false, false, {}, "long int"};
  macros_["__UINTPTR_TYPE__"]    = MacroDef{"__UINTPTR_TYPE__",    false, false, {}, "long unsigned int"};
  // Integer limit macros (LP64: int=32-bit, long/long long=64-bit)
  macros_["__CHAR_BIT__"]        = MacroDef{"__CHAR_BIT__",        false, false, {}, "8"};
  macros_["__SCHAR_MAX__"]       = MacroDef{"__SCHAR_MAX__",       false, false, {}, "127"};
  macros_["__SHRT_MAX__"]        = MacroDef{"__SHRT_MAX__",        false, false, {}, "32767"};
  macros_["__INT_MAX__"]         = MacroDef{"__INT_MAX__",         false, false, {}, "2147483647"};
  macros_["__LONG_MAX__"]        = MacroDef{"__LONG_MAX__",        false, false, {}, "9223372036854775807L"};
  macros_["__LONG_LONG_MAX__"]   = MacroDef{"__LONG_LONG_MAX__",   false, false, {}, "9223372036854775807LL"};
  macros_["__INT8_MAX__"]        = MacroDef{"__INT8_MAX__",        false, false, {}, "127"};
  macros_["__INT16_MAX__"]       = MacroDef{"__INT16_MAX__",       false, false, {}, "32767"};
  macros_["__INT32_MAX__"]       = MacroDef{"__INT32_MAX__",       false, false, {}, "2147483647"};
  macros_["__INT64_MAX__"]       = MacroDef{"__INT64_MAX__",       false, false, {}, "9223372036854775807LL"};
  macros_["__UINT8_MAX__"]       = MacroDef{"__UINT8_MAX__",       false, false, {}, "255"};
  macros_["__UINT16_MAX__"]      = MacroDef{"__UINT16_MAX__",      false, false, {}, "65535"};
  macros_["__UINT32_MAX__"]      = MacroDef{"__UINT32_MAX__",      false, false, {}, "4294967295U"};
  macros_["__UINT64_MAX__"]      = MacroDef{"__UINT64_MAX__",      false, false, {}, "18446744073709551615ULL"};
  macros_["__INTMAX_MAX__"]      = MacroDef{"__INTMAX_MAX__",      false, false, {}, "9223372036854775807LL"};
  macros_["__UINTMAX_MAX__"]     = MacroDef{"__UINTMAX_MAX__",     false, false, {}, "18446744073709551615ULL"};
  macros_["__SIZE_MAX__"]        = MacroDef{"__SIZE_MAX__",        false, false, {}, "18446744073709551615ULL"};
  macros_["__INTPTR_MAX__"]      = MacroDef{"__INTPTR_MAX__",      false, false, {}, "9223372036854775807LL"};
  macros_["__UINTPTR_MAX__"]     = MacroDef{"__UINTPTR_MAX__",     false, false, {}, "18446744073709551615ULL"};
  macros_["__PTRDIFF_MAX__"]     = MacroDef{"__PTRDIFF_MAX__",     false, false, {}, "9223372036854775807LL"};
  macros_["__WCHAR_MAX__"]       = MacroDef{"__WCHAR_MAX__",       false, false, {}, "2147483647"};
}

std::string Preprocessor::preprocess_file(const std::string& path) {
  std::string source;
  if (!read_file(path, &source)) {
    throw std::runtime_error("failed to open file: " + path);
  }

  errors_.clear();
  warnings_.clear();
  cond_stack_.clear();
  needs_external_fallback_ = false;
  base_file_ = path;

  std::string internal = preprocess_text(source, path, 0);

  // Keep existing project behavior stable while TODO sections are unfinished.
  if (needs_external_fallback_) {
    std::string external = preprocess_external(path);
    if (!external.empty()) {
      // External preprocessor is authoritative; discard any errors/warnings
      // that the partial internal pass may have produced incorrectly.
      errors_.clear();
      warnings_.clear();
      return external;
    }
  }
  return internal;
}

std::string Preprocessor::preprocess_text(const std::string& source,
                                          const std::string& file,
                                          int include_depth) {
  if (include_depth > 200) {
    errors_.push_back(PreprocessorDiagnostic{file, 1, 1,
                                             "include depth exceeds limit (200)"});
    return "\n";
  }

  // Save and reset virtual source location for this translation unit / included file.
  int saved_voffset = virtual_line_offset_;
  std::string saved_vfile = virtual_file_;
  virtual_line_offset_ = 0;
  virtual_file_ = file;

  std::string p2 = join_continued_lines(source);
  std::string p3 = strip_comments(p2);

  std::string out;
  std::istringstream in(p3);
  std::string line;
  int line_no = 0;

  while (std::getline(in, line)) {
    ++line_no;

    // Keep __LINE__ and __FILE__ macros in sync with current virtual source position.
    // These are updated before processing the line so that both directive expressions
    // (e.g. #if __LINE__ == 1000) and non-directive text see the correct value.
    int vline = line_no + virtual_line_offset_;
    macros_["__LINE__"] = MacroDef{"__LINE__", false, false, {}, std::to_string(vline)};
    macros_["__FILE__"] = MacroDef{"__FILE__", false, false, {},
                                   "\"" + virtual_file_ + "\""};

    size_t i = 0;
    while (i < line.size() && (line[i] == ' ' || line[i] == '\t')) ++i;
    bool is_directive = (i < line.size() && line[i] == '#');

    if (is_directive) {
      process_directive(line.substr(i + 1), out, file, line_no, include_depth);
      continue;
    }

    if (is_active()) {
      out += expand_line(line);
      out.push_back('\n');
    } else {
      // Preserve line count in inactive conditional branches.
      out.push_back('\n');
    }
  }

  // Restore caller's virtual source location (important for include return).
  virtual_line_offset_ = saved_voffset;
  virtual_file_ = saved_vfile;

  return out;
}

std::string Preprocessor::join_continued_lines(const std::string& source) {
  std::string out;
  out.reserve(source.size());

  for (size_t i = 0; i < source.size(); ++i) {
    char c = source[i];
    if (c == '\\' && i + 1 < source.size() && source[i + 1] == '\n') {
      ++i;
      continue;
    }
    out.push_back(c);
  }
  return out;
}

std::string Preprocessor::strip_comments(const std::string& source) {
  std::string out;
  out.reserve(source.size());

  bool in_str = false;
  bool in_chr = false;

  for (size_t i = 0; i < source.size();) {
    char c = source[i];

    if (!in_str && !in_chr && c == '/' && i + 1 < source.size()) {
      char n = source[i + 1];
      if (n == '/') {
        i += 2;
        while (i < source.size() && source[i] != '\n') ++i;
        continue;
      }
      if (n == '*') {
        i += 2;
        while (i + 1 < source.size() && !(source[i] == '*' && source[i + 1] == '/')) {
          if (source[i] == '\n') out.push_back('\n');
          ++i;
        }
        if (i + 1 < source.size()) i += 2;
        out.push_back(' ');
        continue;
      }
    }

    out.push_back(c);

    if (!in_chr && c == '"' && (i == 0 || source[i - 1] != '\\')) {
      in_str = !in_str;
    } else if (!in_str && c == '\'' && (i == 0 || source[i - 1] != '\\')) {
      in_chr = !in_chr;
    }

    ++i;
  }

  return out;
}

void Preprocessor::process_directive(const std::string& raw_line, std::string& out,
                                     const std::string& current_file, int line_no,
                                     int include_depth) {
  auto [key, rest] = split_directive(raw_line);

  if (key.empty()) {
    out.push_back('\n');
    return;
  }

  if (key == "if") {
    handle_if(rest);
    out.push_back('\n');
    return;
  }
  if (key == "ifdef") {
    handle_ifdef(rest, false);
    out.push_back('\n');
    return;
  }
  if (key == "ifndef") {
    handle_ifdef(rest, true);
    out.push_back('\n');
    return;
  }
  if (key == "elif") {
    handle_elif(rest);
    out.push_back('\n');
    return;
  }
  if (key == "else") {
    handle_else();
    out.push_back('\n');
    return;
  }
  if (key == "endif") {
    handle_endif();
    out.push_back('\n');
    return;
  }

  if (!is_active()) {
    // In inactive branches only conditional directives affect state.
    out.push_back('\n');
    return;
  }

  if (key == "define") {
    handle_define(rest, current_file, line_no);
  } else if (key == "undef") {
    handle_undef(rest);
  } else if (key == "include") {
    out += handle_include(rest, current_file, include_depth, line_no);
    return;
  } else if (key == "error") {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             trim_copy(rest)});
  } else if (key == "warning") {
    warnings_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               trim_copy(rest)});
  } else if (key == "pragma") {
    // TODO(preprocessor): implement pragma handlers (once/pack/push_macro/pop_macro/...)
    needs_external_fallback_ = true;
  } else if (key == "line") {
    // C11 6.10.4: #line <digit-sequence> ["filename"]
    // Macro-expand the argument first (e.g. #line LINE where LINE=1000).
    std::string expanded = expand_line(trim_copy(rest));
    size_t i = 0;
    while (i < expanded.size() && std::isspace(static_cast<unsigned char>(expanded[i]))) ++i;
    size_t num_start = i;
    while (i < expanded.size() && std::isdigit(static_cast<unsigned char>(expanded[i]))) ++i;
    if (i == num_start) {
      errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                               "#line directive: missing line number"});
    } else {
      int new_lineno = std::stoi(expanded.substr(num_start, i - num_start));
      while (i < expanded.size() && std::isspace(static_cast<unsigned char>(expanded[i]))) ++i;
      std::string new_file = virtual_file_;
      if (i < expanded.size() && expanded[i] == '"') {
        size_t j = i + 1;
        while (j < expanded.size() && expanded[j] != '"') ++j;
        new_file = expanded.substr(i + 1, j - i - 1);
      }
      // Update virtual location: next physical line (line_no+1) maps to new_lineno.
      virtual_line_offset_ = new_lineno - (line_no + 1);
      virtual_file_ = new_file;
      // Emit a GCC-compatible line marker so downstream lexer can track source positions.
      out += "# " + std::to_string(new_lineno) + " \"" + new_file + "\"\n";
      return;  // line marker already has the newline
    }
  } else {
    // TODO(preprocessor): unknown directives should be handled more carefully
    // (diagnostic policy, extension points).
    needs_external_fallback_ = true;
  }

  out.push_back('\n');
}

void Preprocessor::handle_define(const std::string& args,
                                 const std::string& file,
                                 int line_no) {
  std::string s = args;
  size_t i = 0;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  if (i >= s.size() || !is_ident_start(s[i])) {
    errors_.push_back(PreprocessorDiagnostic{file, line_no, 1,
                                             "invalid #define: missing macro name"});
    return;
  }

  size_t n0 = i++;
  while (i < s.size() && is_ident_continue(s[i])) ++i;
  std::string name = s.substr(n0, i - n0);

  MacroDef def;
  def.name = name;

  // Function-like macro has no whitespace between name and '('.
  if (i < s.size() && s[i] == '(') {
    def.function_like = true;
    ++i;
    std::string param;
    bool seen_variadic = false;

    while (i < s.size()) {
      char c = s[i];
      if (c == ')') {
        if (!param.empty()) {
          std::string p = trim_copy(param);
          if (p == "...") {
            def.variadic = true;
            seen_variadic = true;
          } else {
            def.params.push_back(p);
          }
          param.clear();
        }
        ++i;
        break;
      }
      if (c == ',') {
        std::string p = trim_copy(param);
        if (!p.empty()) {
          if (p == "...") {
            def.variadic = true;
            seen_variadic = true;
          } else {
            def.params.push_back(p);
          }
        }
        param.clear();
        ++i;
        continue;
      }
      param.push_back(c);
      ++i;
    }

    (void)seen_variadic;  // variadic macros are now handled natively
  }

  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
  def.body = (i < s.size()) ? s.substr(i) : std::string();

  macros_[name] = std::move(def);
}

void Preprocessor::handle_undef(const std::string& args) {
  std::string name = trim_copy(args);
  if (name.empty()) return;
  macros_.erase(name);
}

void Preprocessor::handle_ifdef(const std::string& args, bool is_ifndef) {
  std::string name = trim_copy(args);
  bool defined = macros_.find(name) != macros_.end();
  bool take = is_ifndef ? !defined : defined;

  ConditionalFrame f;
  f.parent_active = is_active();
  f.this_active = f.parent_active && take;
  f.any_taken = f.this_active;
  cond_stack_.push_back(f);
}

void Preprocessor::handle_if(const std::string& args) {
  bool take = evaluate_if_expr(args);

  ConditionalFrame f;
  f.parent_active = is_active();
  f.this_active = f.parent_active && take;
  f.any_taken = f.this_active;
  cond_stack_.push_back(f);
}

void Preprocessor::handle_elif(const std::string& args) {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }

  ConditionalFrame& f = cond_stack_.back();
  if (f.saw_else) {
    needs_external_fallback_ = true;
    return;
  }

  bool take = false;
  if (!f.any_taken) {
    take = evaluate_if_expr(args);
  }
  f.this_active = f.parent_active && !f.any_taken && take;
  if (f.this_active) f.any_taken = true;
}

void Preprocessor::handle_else() {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }

  ConditionalFrame& f = cond_stack_.back();
  if (f.saw_else) {
    needs_external_fallback_ = true;
    return;
  }
  f.saw_else = true;

  f.this_active = f.parent_active && !f.any_taken;
  if (f.this_active) f.any_taken = true;
}

void Preprocessor::handle_endif() {
  if (cond_stack_.empty()) {
    needs_external_fallback_ = true;
    return;
  }
  cond_stack_.pop_back();
}

bool Preprocessor::evaluate_if_expr(const std::string& expr) {
  auto is_defined_name = [this](const std::string& name) {
    return macros_.find(name) != macros_.end();
  };

  // C11-ish pipeline:
  // 1) resolve defined()/__has_* intrinsics
  // 2) macro expansion
  // 3) resolve again in case expansion introduced intrinsics
  std::string r1 = resolve_defined_and_intrinsics(expr, is_defined_name);
  std::string expanded = expand_line(r1);
  std::string r2 = resolve_defined_and_intrinsics(expanded, is_defined_name);
  std::string s = trim_copy(r2);
  if (s.empty()) return false;

  bool ok = true;
  IfExprParser parser(s, is_defined_name);
  ExprValue v = parser.parse(&ok);
  if (!ok) {
    // Keep behavior compatible while parser coverage expands.
    needs_external_fallback_ = true;
    return false;
  }
  return as_bool(v);
}

bool Preprocessor::is_active() const {
  if (cond_stack_.empty()) return true;
  return cond_stack_.back().this_active;
}

// Single-pass recursive expansion with hideset support (C11 6.10.3.4 "blue paint").
// 'disabled' is the set of macro names that must NOT be expanded in this context.
// Each call handles one left-to-right pass; recursive calls handle nested expansions
// and rescan with the macro's own name added to the disabled set.
std::string Preprocessor::expand_text(const std::string& text,
                                      std::unordered_set<std::string> disabled) {
  std::string out;
  out.reserve(text.size() + 16);

  bool in_str = false;
  bool in_chr = false;

  for (size_t i = 0; i < text.size();) {
    char c = text[i];

    if (!in_str && !in_chr && is_ident_start(c)) {
      size_t j = i + 1;
      while (j < text.size() && is_ident_continue(text[j])) ++j;
      std::string ident = text.substr(i, j - i);

      auto it = macros_.find(ident);
      if (it != macros_.end() && disabled.find(ident) == disabled.end()) {
        const MacroDef& def = it->second;
        if (def.function_like) {
          // Function-like: only expand if immediately followed by '('.
          size_t k = j;
          while (k < text.size() && (text[k] == ' ' || text[k] == '\t')) ++k;
          if (k < text.size() && text[k] == '(') {
            ++k;  // skip '('
            std::vector<std::string> raw_args = collect_funclike_args(text, &k);
            // Normalise: F() with no params → empty args list.
            if (def.params.empty() && !def.variadic &&
                raw_args.size() == 1 && trim_copy(raw_args[0]).empty()) {
              raw_args.clear();
            }
            out += expand_funclike_call(def, raw_args, disabled);
            i = k;
          } else {
            // No '(' — not a macro invocation; leave as-is.
            out += ident;
            i = j;
          }
        } else {
          // Object-like: expand body with disabled ∪ {ident}.
          // Exception: L/u/U/u8 immediately followed by ' or " form a wide/unicode
          // character/string literal prefix — do NOT expand them as macros.
          if ((ident == "L" || ident == "u" || ident == "U" || ident == "u8") &&
              j < text.size() && (text[j] == '\'' || text[j] == '"')) {
            out += ident;
            i = j;
          } else {
            std::unordered_set<std::string> new_disabled = disabled;
            new_disabled.insert(ident);
            out += expand_text(def.body, std::move(new_disabled));
            i = j;
          }
        }
      } else {
        out += ident;
        i = j;
      }
      continue;
    }

    out.push_back(c);

    if (!in_chr && c == '"' && (i == 0 || text[i - 1] != '\\')) {
      in_str = !in_str;
    } else if (!in_str && c == '\'' && (i == 0 || text[i - 1] != '\\')) {
      in_chr = !in_chr;
    }

    ++i;
  }

  return out;
}

std::string Preprocessor::expand_funclike_call(const MacroDef& def,
                                               const std::vector<std::string>& raw_args,
                                               std::unordered_set<std::string> disabled) {
  // Prescan-expand each argument (C11 6.10.3.1 step 2).
  // Arguments use the caller's disabled set (not yet inside the macro body).
  std::vector<std::string> exp_args;
  exp_args.reserve(raw_args.size());
  for (const auto& a : raw_args) {
    exp_args.push_back(expand_text(trim_copy(a), disabled));
  }

  // Build variadic join (everything beyond named params).
  std::string va_raw, va_exp;
  if (def.variadic) {
    for (size_t k = def.params.size(); k < raw_args.size(); ++k) {
      if (k > def.params.size()) { va_raw += ","; va_exp += ","; }
      va_raw += raw_args[k];
      if (k < raw_args.size()) va_exp += expand_text(trim_copy(raw_args[k]), disabled);
    }
    // GNU extension: ", ## __VA_ARGS__" removal handled in substitute_funclike_body.
  }

  // Substitute body: handles #, ## and parameter replacement.
  std::string substituted = substitute_funclike_body(def.body, def.params,
                                                     raw_args, exp_args,
                                                     def.variadic, va_raw, va_exp);

  // Rescan result for further macro expansion (C11 6.10.3.4).
  // Add the macro's own name to the disabled set to prevent self-re-expansion.
  disabled.insert(def.name);
  return expand_text(substituted, std::move(disabled));
}

std::string Preprocessor::expand_line(const std::string& line) {
  return expand_text(line, {});
}

std::string Preprocessor::handle_include(const std::string& args,
                                         const std::string& current_file,
                                         int include_depth,
                                         int line_no) {
  std::string s = trim_copy(args);
  if (s.empty()) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "empty #include"});
    return "\n";
  }

  // TODO(preprocessor): support computed include and <...> search chain
  // (-I/-iquote/-isystem/-idirafter/default system paths).
  bool quoted = s.size() >= 2 && s.front() == '"' && s.back() == '"';
  if (!quoted) {
    needs_external_fallback_ = true;
    return "\n";
  }

  std::string rel = s.substr(1, s.size() - 2);
  std::string base_dir = dirname_of(current_file);
  std::string full = base_dir.empty() ? rel : (base_dir + "/" + rel);

  std::string child_src;
  if (!read_file(full, &child_src)) {
    errors_.push_back(PreprocessorDiagnostic{current_file, line_no, 1,
                                             "include file not found: " + rel});
    return "\n";
  }

  return preprocess_text(child_src, full, include_depth + 1);
}

std::string Preprocessor::dirname_of(const std::string& path) {
  size_t pos = path.find_last_of("/");
  if (pos == std::string::npos) return std::string();
  if (pos == 0) return "/";
  return path.substr(0, pos);
}

bool Preprocessor::read_file(const std::string& path, std::string* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  std::ostringstream buf;
  buf << in.rdbuf();
  *out = buf.str();
  return true;
}

std::string Preprocessor::shell_quote(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 2);
  out.push_back('\'');
  for (char c : s) {
    if (c == '\'') {
      out += "'\"'\"'";
    } else {
      out.push_back(c);
    }
  }
  out.push_back('\'');
  return out;
}

bool Preprocessor::run_capture(const std::string& cmd, std::string& out_text) {
  FILE* fp = popen(cmd.c_str(), "r");
  if (!fp) return false;
  out_text.clear();
  char buf[4096];
  while (fgets(buf, sizeof(buf), fp)) {
    out_text += buf;
  }
  int rc = pclose(fp);
  if (rc != 0) return false;
  return !out_text.empty();
}

std::string Preprocessor::preprocess_external(const std::string& path) const {
  const std::string qpath = shell_quote(path);
  std::vector<std::string> commands = {
      "ccc -E -P -x c " + qpath + " 2>/dev/null",
      "cc -E -P -x c " + qpath + " 2>/dev/null",
      "cpp -E -P " + qpath + " 2>/dev/null",
  };

  std::string result;
  for (const std::string& cmd : commands) {
    if (run_capture(cmd, result)) return result;
  }
  return std::string();
}

}  // namespace tinyc2ll::frontend_cxx

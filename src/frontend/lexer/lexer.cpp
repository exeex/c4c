#include "lexer.hpp"

#include <cctype>
#include <cstring>
#include <stdexcept>

#include "../../shared/text_ops.hpp"

namespace c4c {

Lexer::Lexer(std::string source, LexProfile profile)
    : source_(std::move(source)), lex_profile_(profile) {}

std::vector<Token> Lexer::scan_all() {
  std::vector<Token> out;
  while (true) {
    skip_whitespace_and_comments();
    if (has_pending_pragma_pack_) {
      out.push_back(std::move(pending_pragma_pack_));
      has_pending_pragma_pack_ = false;
      continue;
    }
    if (has_pending_pragma_weak_) {
      out.push_back(std::move(pending_pragma_weak_));
      has_pending_pragma_weak_ = false;
      continue;
    }
    if (has_pending_pragma_gcc_visibility_) {
      out.push_back(std::move(pending_pragma_gcc_visibility_));
      has_pending_pragma_gcc_visibility_ = false;
      continue;
    }
    if (has_pending_pragma_exec_) {
      out.push_back(std::move(pending_pragma_exec_));
      has_pending_pragma_exec_ = false;
      continue;
    }
    if (at_end()) {
      out.push_back(make_token(TokenKind::EndOfFile, "", line_, column_));
      break;
    }

    char c = peek();

    // Number literals: digits, or .digit (float starting with dot)
    if (std::isdigit(static_cast<unsigned char>(c)) ||
        (c == '.' && std::isdigit(static_cast<unsigned char>(peek_next())))) {
      out.push_back(scan_number());
      continue;
    }

    // Identifiers and keywords (including __xxx GCC forms and $-identifier extension)
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
      out.push_back(scan_identifier_or_keyword());
      continue;
    }

    if (c == '"') {
      out.push_back(scan_string());
      continue;
    }
    if (c == '\'') {
      out.push_back(scan_char());
      continue;
    }

    out.push_back(scan_punct());
  }
  return out;
}

// ── helpers ──────────────────────────────────────────────────────────────────

char Lexer::peek() const {
  return at_end() ? '\0' : source_[index_];
}

char Lexer::peek_next() const {
  if (index_ + 1 >= source_.size()) return '\0';
  return source_[index_ + 1];
}

// Peek 2 characters ahead (used for ... and <<= / >>=)
char Lexer::peek2() const {
  if (index_ + 2 >= source_.size()) return '\0';
  return source_[index_ + 2];
}

bool Lexer::at_end() const {
  return index_ >= source_.size();
}

char Lexer::advance() {
  if (at_end()) return '\0';
  char c = source_[index_++];
  if (c == '\n') {
    line_ += 1;
    column_ = 1;
  } else {
    column_ += 1;
  }
  return c;
}

Token Lexer::make_token(TokenKind kind, std::string_view lexeme, int line, int col) {
  const TextId text_id = intern_text(text_table_, lexeme);
  Token token{};
  token.kind = kind;
  token.line = line;
  token.column = col;
  token.text_id = text_id;
  token.file_id = current_file_id_;
  return token;
}

// ── whitespace / comment / GCC line-marker skipping ─────────────────────────
// GCC/clang preprocessor emits line markers:  # <num> "filename" [flags]
// We skip these entirely (they only affect debug info, not semantics).
// Pure-C backport note: same logic, no std::string needed in the skip path.

void Lexer::skip_whitespace_and_comments() {
  while (!at_end()) {
    char c = peek();

    // Plain whitespace
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f' || c == '\v') {
      advance();
      continue;
    }

    // Line comment //
    if (c == '/' && peek_next() == '/') {
      while (!at_end() && peek() != '\n') advance();
      continue;
    }

    // Block comment /* ... */
    if (c == '/' && peek_next() == '*') {
      advance(); advance();  // consume /*
      while (!at_end()) {
        if (peek() == '*' && peek_next() == '/') {
          advance(); advance();  // consume */
          break;
        }
        advance();
      }
      continue;
    }

    // GCC line-marker: '#' at column 1 (after whitespace consumption puts us
    // at a fresh position).  A line marker looks like:
    //   # <decimal-number> "filename" [flags]
    // We skip the entire line, EXCEPT for #pragma pack which produces a token.
    if (c == '#') {
      // Only treat as line marker if the '#' is at the logical start of a
      // source line (column == 1 after all leading whitespace).
      // We use column_ == 1 here since we just processed whitespace above.
      if (column_ == 1) {
        // Check if this is #pragma pack — peek ahead without consuming.
        static const char pragma_pack[] = "#pragma pack";
        size_t pp_len = sizeof(pragma_pack) - 1;
        if (index_ + pp_len <= source_.size() &&
            source_.compare(index_, pp_len, pragma_pack) == 0) {
          // Extract the arguments: everything between ( and ) on this line.
          int tok_line = line_;
          int tok_col = column_;
          // Skip past "#pragma pack"
          for (size_t j = 0; j < pp_len; ++j) advance();
          // Skip whitespace before '('
          while (!at_end() && peek() != '\n' && (peek() == ' ' || peek() == '\t')) advance();
          std::string args;
          if (!at_end() && peek() == '(') {
            advance();  // consume '('
            while (!at_end() && peek() != ')' && peek() != '\n') {
              char ch = advance();
              if (ch != ' ' && ch != '\t') args += ch;  // strip whitespace from args
            }
            if (!at_end() && peek() == ')') advance();  // consume ')'
          }
          // Skip rest of line
          while (!at_end() && peek() != '\n') advance();
          pending_pragma_pack_ = make_token(TokenKind::PragmaPack, args, tok_line, tok_col);
          has_pending_pragma_pack_ = true;
          return;  // exit skip loop to let scan_all() pick up the pending token
        }
        // Check if this is #pragma weak — peek ahead without consuming.
        static const char pragma_weak[] = "#pragma weak";
        size_t pw_len = sizeof(pragma_weak) - 1;
        if (index_ + pw_len <= source_.size() &&
            source_.compare(index_, pw_len, pragma_weak) == 0) {
          int tok_line = line_;
          int tok_col = column_;
          // Skip past "#pragma weak"
          for (size_t j = 0; j < pw_len; ++j) advance();
          // Skip whitespace before symbol name
          while (!at_end() && peek() != '\n' && (peek() == ' ' || peek() == '\t')) advance();
          // Collect the symbol name (identifier chars)
          std::string sym;
          while (!at_end() && peek() != '\n' && peek() != ' ' && peek() != '\t') {
            sym += advance();
          }
          // Skip rest of line
          while (!at_end() && peek() != '\n') advance();
          if (!sym.empty()) {
            pending_pragma_weak_ = make_token(TokenKind::PragmaWeak, sym, tok_line, tok_col);
            has_pending_pragma_weak_ = true;
            return;
          }
          continue;
        }
        // Check if this is #pragma GCC visibility push/pop.
        static const char pragma_gv[] = "#pragma GCC visibility";
        size_t gv_len = sizeof(pragma_gv) - 1;
        if (index_ + gv_len <= source_.size() &&
            source_.compare(index_, gv_len, pragma_gv) == 0) {
          int tok_line = line_;
          int tok_col = column_;
          for (size_t j = 0; j < gv_len; ++j) advance();
          // Skip whitespace
          while (!at_end() && peek() != '\n' && (peek() == ' ' || peek() == '\t')) advance();
          // Collect the directive: "push" or "pop"
          std::string directive;
          while (!at_end() && peek() != '\n' && peek() != '(' && peek() != ' ' && peek() != '\t') {
            directive += advance();
          }
          std::string lexeme;
          if (directive == "push") {
            // Expect push(visibility)
            while (!at_end() && peek() != '\n' && (peek() == ' ' || peek() == '\t')) advance();
            std::string vis;
            if (!at_end() && peek() == '(') {
              advance();  // consume '('
              while (!at_end() && peek() != ')' && peek() != '\n') {
                char ch = advance();
                if (ch != ' ' && ch != '\t') vis += ch;
              }
              if (!at_end() && peek() == ')') advance();
            }
            lexeme = "push," + vis;
          } else if (directive == "pop") {
            lexeme = "pop";
          }
          // Skip rest of line
          while (!at_end() && peek() != '\n') advance();
          if (!lexeme.empty()) {
            pending_pragma_gcc_visibility_ = make_token(TokenKind::PragmaGccVisibility, lexeme, tok_line, tok_col);
            has_pending_pragma_gcc_visibility_ = true;
            return;
          }
          continue;
        }
        // Check if this is #pragma c4 exec host/device.
        static const char pragma_exec[] = "#pragma c4 exec";
        size_t pe_len = sizeof(pragma_exec) - 1;
        if (index_ + pe_len <= source_.size() &&
            source_.compare(index_, pe_len, pragma_exec) == 0) {
          int tok_line = line_;
          int tok_col = column_;
          for (size_t j = 0; j < pe_len; ++j) advance();
          while (!at_end() && peek() != '\n' && (peek() == ' ' || peek() == '\t')) advance();
          std::string domain;
          while (!at_end() && peek() != '\n' && peek() != ' ' && peek() != '\t') {
            domain += advance();
          }
          while (!at_end() && peek() != '\n') advance();
          if (domain == "host" || domain == "device") {
            pending_pragma_exec_ = make_token(TokenKind::PragmaExec, domain, tok_line, tok_col);
            has_pending_pragma_exec_ = true;
            return;
          }
          continue;
        }
        if (consume_line_marker()) continue;
        // Skip to end of line
        while (!at_end() && peek() != '\n') advance();
        continue;
      }
      break;  // '#' mid-line → punctuation token
    }

    break;
  }
}

bool Lexer::consume_line_marker() {
  if (peek() != '#' || column_ != 1) return false;

  size_t p = index_ + 1;
  while (p < source_.size() && (source_[p] == ' ' || source_[p] == '\t')) ++p;
  if (p >= source_.size() || !std::isdigit(static_cast<unsigned char>(source_[p]))) {
    return false;
  }

  int new_line = 0;
  while (p < source_.size() && std::isdigit(static_cast<unsigned char>(source_[p]))) {
    new_line = new_line * 10 + (source_[p] - '0');
    ++p;
  }

  while (p < source_.size() && (source_[p] == ' ' || source_[p] == '\t')) ++p;
  if (p >= source_.size() || source_[p] != '"') return false;
  ++p;

  std::string new_file;
  while (p < source_.size() && source_[p] != '"') {
    if (source_[p] == '\\' && p + 1 < source_.size()) ++p;
    new_file.push_back(source_[p++]);
  }
  if (p >= source_.size() || source_[p] != '"') return false;

  while (!at_end() && peek() != '\n') advance();
  if (!at_end() && peek() == '\n') advance();

  current_file_id_ = intern_file_path(file_table_, new_file);
  line_ = new_line;
  column_ = 1;
  return true;
}

// ── identifier / keyword ─────────────────────────────────────────────────────

Token Lexer::scan_identifier_or_keyword() {
  int tok_line = line_;
  int tok_col  = column_;
  std::string text;
  while (!at_end()) {
    char c = peek();
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '$') {
      text.push_back(advance());
    } else {
      break;
    }
  }
  // Classify keyword vs identifier
  TokenKind kind = keyword_from_string(text, /*gnu_extensions=*/true, lex_profile_);

  // Handle wide/unicode string and char literal prefixes: L"..." u"..." U"..." u8"..."
  if (kind == TokenKind::Identifier &&
      (text == "L" || text == "u" || text == "U" || text == "u8") &&
      !at_end() && (peek() == '"' || peek() == '\'')) {
    return (peek() == '"') ? scan_string(std::move(text))
                           : scan_char(std::move(text));
  }

  return make_token(kind, std::move(text), tok_line, tok_col);
}

// ── number literals ──────────────────────────────────────────────────────────
// Scans the full raw lexeme (including prefix, digits, and suffix) so that
// the parser / IR builder can re-parse the value from the lexeme string.
//
// Handles:
//   decimal integers:  123, 123u, 123ul, 123ll, etc.
//   hex integers:      0x1A, 0X1Au, etc.
//   octal integers:    0755, 0755u
//   binary integers:   0b101, 0B101  (GCC extension)
//   decimal floats:    3.14, 3.14f, 3.14L, 1e10, 1.5e-3
//   hex floats:        0x1.8p+1, 0x1p10f  (C99)
//   leading-dot:       .5, .5f
//
// Returns IntLit or FloatLit depending on whether a float indicator was found.
//
// Pure-C backport note: replace std::string with a char buf[] + len pair.

Token Lexer::scan_number() {
  int tok_line = line_;
  int tok_col  = column_;
  std::string text;
  bool is_float = false;

  // Leading dot case (.5 .5f etc.)
  if (peek() == '.') {
    is_float = true;
    text.push_back(advance());  // consume '.'
    while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
      text.push_back(advance());
    }
    // Optional exponent: e/E [+-] digits
    if (!at_end() && (peek() == 'e' || peek() == 'E')) {
      is_float = true;
      text.push_back(advance());
      if (!at_end() && (peek() == '+' || peek() == '-')) {
        text.push_back(advance());
      }
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
    }
    // Float suffix
    scan_float_suffix(text, is_float);
    return make_token(TokenKind::FloatLit, std::move(text), tok_line, tok_col);
  }

  // Must start with a digit at this point
  // Check for hex (0x/0X), octal (0...), binary (0b/0B), or decimal
  if (peek() == '0') {
    text.push_back(advance());  // consume '0'
    char next = peek();

    if (next == 'x' || next == 'X') {
      // Hex literal
      text.push_back(advance());  // consume x/X
      while (!at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
      // Hex float? (. or p/P)
      if (!at_end() && peek() == '.') {
        is_float = true;
        text.push_back(advance());
        while (!at_end() && std::isxdigit(static_cast<unsigned char>(peek()))) {
          text.push_back(advance());
        }
      }
      if (!at_end() && (peek() == 'p' || peek() == 'P')) {
        is_float = true;
        text.push_back(advance());
        if (!at_end() && (peek() == '+' || peek() == '-')) {
          text.push_back(advance());
        }
        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
          text.push_back(advance());
        }
      }
    } else if (next == 'b' || next == 'B') {
      // Binary literal (GCC extension)
      text.push_back(advance());  // consume b/B
      while (!at_end() && (peek() == '0' || peek() == '1')) {
        text.push_back(advance());
      }
    } else {
      // Octal or plain 0 — read remaining digits
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
      // Could still be float if . or e/E follows
      if (!at_end() && peek() == '.') {
        is_float = true;
        text.push_back(advance());
        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
          text.push_back(advance());
        }
      }
      if (!at_end() && (peek() == 'e' || peek() == 'E')) {
        is_float = true;
        text.push_back(advance());
        if (!at_end() && (peek() == '+' || peek() == '-')) {
          text.push_back(advance());
        }
        while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
          text.push_back(advance());
        }
      }
    }
  } else {
    // Decimal
    while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
      text.push_back(advance());
    }
    if (!at_end() && peek() == '.') {
      is_float = true;
      text.push_back(advance());
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
    }
    if (!at_end() && (peek() == 'e' || peek() == 'E')) {
      is_float = true;
      text.push_back(advance());
      if (!at_end() && (peek() == '+' || peek() == '-')) {
        text.push_back(advance());
      }
      while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
        text.push_back(advance());
      }
    }
  }

  if (is_float) {
    scan_float_suffix(text, is_float);
    return make_token(TokenKind::FloatLit, std::move(text), tok_line, tok_col);
  }

  // Integer suffixes: u/U, l/L, ll/LL, ul/UL, ull/ULL etc. (any order)
  scan_int_suffix(text);
  return make_token(TokenKind::IntLit, std::move(text), tok_line, tok_col);
}

// Consume float suffix (f/F, l/L, f16/F16, f32/F32, f64/F64, f128/F128,
// bf16/BF16) and optional imaginary suffix (i/j/I/J) in any order.
// We currently lower the fixed-width suffixes to the closest existing frontend
// type category rather than distinct extended floating-point types.
void Lexer::scan_float_suffix(std::string &text, bool &is_float) {
  bool seen_sz  = false;  // f/F or l/L
  bool seen_ext = false;  // f16/f32/f64/f128/bf16
  bool seen_bf16 = false;
  bool seen_imag = false; // i/j/I/J
  for (int pass = 0; pass < 4 && !at_end(); ++pass) {
    char s = peek();
    const auto has_suffix = [&](const char* suffix) -> bool {
      const size_t n = std::strlen(suffix);
      if (index_ + n > source_.size()) return false;
      for (size_t i = 0; i < n; ++i) {
        if (source_[index_ + i] != suffix[i]) return false;
      }
      return true;
    };
    const auto consume_suffix = [&](const char* suffix) {
      for (const char* p = suffix; *p; ++p) text.push_back(advance());
      is_float = true;
    };
    if (!seen_ext && has_suffix("f128")) {
      seen_ext = true;
      consume_suffix("f128");
    } else if (!seen_ext && has_suffix("F128")) {
      seen_ext = true;
      consume_suffix("F128");
    } else if (!seen_ext && has_suffix("f16")) {
      seen_ext = true;
      consume_suffix("f16");
    } else if (!seen_ext && has_suffix("F16")) {
      seen_ext = true;
      consume_suffix("F16");
    } else if (!seen_ext && has_suffix("f32")) {
      seen_ext = true;
      consume_suffix("f32");
    } else if (!seen_ext && has_suffix("F32")) {
      seen_ext = true;
      consume_suffix("F32");
    } else if (!seen_ext && has_suffix("f64")) {
      seen_ext = true;
      consume_suffix("f64");
    } else if (!seen_ext && has_suffix("F64")) {
      seen_ext = true;
      consume_suffix("F64");
    }
    const bool has_bf16_suffix =
        index_ + 3 < source_.size() &&
        source_[index_] == 'b' &&
        source_[index_ + 1] == 'f' &&
        source_[index_ + 2] == '1' &&
        source_[index_ + 3] == '6';
    const bool has_BF16_suffix =
        index_ + 3 < source_.size() &&
        source_[index_] == 'B' &&
        source_[index_ + 1] == 'F' &&
        source_[index_ + 2] == '1' &&
        source_[index_ + 3] == '6';
    if (!seen_bf16 && has_bf16_suffix) {
      seen_bf16 = true;
      is_float = true;
      text.push_back(advance());
      text.push_back(advance());
      text.push_back(advance());
      text.push_back(advance());
    } else if (!seen_bf16 && has_BF16_suffix) {
      seen_bf16 = true;
      is_float = true;
      text.push_back(advance());
      text.push_back(advance());
      text.push_back(advance());
      text.push_back(advance());
    } else if ((s == 'f' || s == 'F' || s == 'l' || s == 'L') && !seen_sz) {
      seen_sz = true;
      is_float = true;
      text.push_back(advance());
    } else if ((s == 'i' || s == 'I' || s == 'j' || s == 'J') && !seen_imag) {
      seen_imag = true;
      text.push_back(advance());
    } else {
      break;
    }
  }
}

// Consume integer suffix: u/U, l/L, ll/LL and optional imaginary (i/j/I/J).
void Lexer::scan_int_suffix(std::string &text) {
  bool seen_u    = false;
  bool seen_l    = false;
  bool seen_imag = false;
  for (int pass = 0; pass < 4 && !at_end(); ++pass) {
    char s = peek();
    if ((s == 'u' || s == 'U') && !seen_u) {
      seen_u = true;
      text.push_back(advance());
    } else if ((s == 'l' || s == 'L') && !seen_l) {
      seen_l = true;
      text.push_back(advance());
      if (!at_end() && (peek() == 'l' || peek() == 'L'))
        text.push_back(advance());
    } else if ((s == 'i' || s == 'I' || s == 'j' || s == 'J') && !seen_imag) {
      seen_imag = true;
      text.push_back(advance());
    } else {
      break;
    }
  }
}

// ── string literal ───────────────────────────────────────────────────────────

Token Lexer::scan_string(std::string prefix) {
  int tok_line = line_;
  int tok_col  = column_;
  std::string text = std::move(prefix);
  text.push_back(advance());  // consume opening '"'
  while (!at_end()) {
    char c = advance();
    text.push_back(c);
    if (c == '\\' && !at_end()) {
      text.push_back(advance());  // consume escape sequence char
      continue;
    }
    if (c == '"') {
      return make_token(TokenKind::StrLit, std::move(text), tok_line, tok_col);
    }
  }
  return make_token(TokenKind::Invalid, std::move(text), tok_line, tok_col);
}

// ── char literal ─────────────────────────────────────────────────────────────

Token Lexer::scan_char(std::string prefix) {
  int tok_line = line_;
  int tok_col  = column_;
  std::string text = std::move(prefix);
  text.push_back(advance());  // consume opening '\''
  while (!at_end()) {
    char c = advance();
    text.push_back(c);
    if (c == '\\' && !at_end()) {
      text.push_back(advance());
      continue;
    }
    if (c == '\'') {
      return make_token(TokenKind::CharLit, std::move(text), tok_line, tok_col);
    }
  }
  return make_token(TokenKind::Invalid, std::move(text), tok_line, tok_col);
}

// ── punctuation / operators ──────────────────────────────────────────────────
// All multi-character operators are eagerly consumed here (maximal-munch).
// Pure-C backport note: same switch logic, no C++ needed.

Token Lexer::scan_punct() {
  int tok_line = line_;
  int tok_col  = column_;
  char c = advance();
  std::string text(1, c);

  switch (c) {
    case '(':  return make_token(TokenKind::LParen,   text, tok_line, tok_col);
    case ')':  return make_token(TokenKind::RParen,   text, tok_line, tok_col);
    case '{':  return make_token(TokenKind::LBrace,   text, tok_line, tok_col);
    case '}':  return make_token(TokenKind::RBrace,   text, tok_line, tok_col);
    case '[':  return make_token(TokenKind::LBracket, text, tok_line, tok_col);
    case ']':  return make_token(TokenKind::RBracket, text, tok_line, tok_col);
    case ';':  return make_token(TokenKind::Semi,     text, tok_line, tok_col);
    case ',':  return make_token(TokenKind::Comma,    text, tok_line, tok_col);
    case '~':  return make_token(TokenKind::Tilde,    text, tok_line, tok_col);
    case '?':  return make_token(TokenKind::Question, text, tok_line, tok_col);
    case ':':
      if (lex_profile_ == LexProfile::CppSubset && peek() == ':') {
        text.push_back(advance());
        return make_token(TokenKind::ColonColon, text, tok_line, tok_col);
      }
      return make_token(TokenKind::Colon, text, tok_line, tok_col);

    case '.':
      // Could be '...' (ellipsis)
      if (peek() == '.' && peek_next() == '.') {
        text.push_back(advance()); text.push_back(advance());
        return make_token(TokenKind::Ellipsis, text, tok_line, tok_col);
      }
      return make_token(TokenKind::Dot, text, tok_line, tok_col);

    case '+':
      if (peek() == '+') { text.push_back(advance()); return make_token(TokenKind::PlusPlus,    text, tok_line, tok_col); }
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::PlusAssign,  text, tok_line, tok_col); }
      return make_token(TokenKind::Plus, text, tok_line, tok_col);

    case '-':
      if (peek() == '-') { text.push_back(advance()); return make_token(TokenKind::MinusMinus,  text, tok_line, tok_col); }
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::MinusAssign, text, tok_line, tok_col); }
      if (peek() == '>') { text.push_back(advance()); return make_token(TokenKind::Arrow,       text, tok_line, tok_col); }
      return make_token(TokenKind::Minus, text, tok_line, tok_col);

    case '*':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::StarAssign,    text, tok_line, tok_col); }
      return make_token(TokenKind::Star, text, tok_line, tok_col);

    case '/':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::SlashAssign,   text, tok_line, tok_col); }
      return make_token(TokenKind::Slash, text, tok_line, tok_col);

    case '%':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::PercentAssign, text, tok_line, tok_col); }
      return make_token(TokenKind::Percent, text, tok_line, tok_col);

    case '&':
      if (peek() == '&') { text.push_back(advance()); return make_token(TokenKind::AmpAmp,    text, tok_line, tok_col); }
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::AmpAssign, text, tok_line, tok_col); }
      return make_token(TokenKind::Amp, text, tok_line, tok_col);

    case '|':
      if (peek() == '|') { text.push_back(advance()); return make_token(TokenKind::PipePipe,   text, tok_line, tok_col); }
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::PipeAssign, text, tok_line, tok_col); }
      return make_token(TokenKind::Pipe, text, tok_line, tok_col);

    case '^':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::CaretAssign, text, tok_line, tok_col); }
      return make_token(TokenKind::Caret, text, tok_line, tok_col);

    case '!':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::BangEqual, text, tok_line, tok_col); }
      return make_token(TokenKind::Bang, text, tok_line, tok_col);

    case '=':
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::EqualEqual, text, tok_line, tok_col); }
      return make_token(TokenKind::Assign, text, tok_line, tok_col);

    case '<':
      if (peek() == '<') {
        text.push_back(advance());
        if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::LessLessAssign, text, tok_line, tok_col); }
        return make_token(TokenKind::LessLess, text, tok_line, tok_col);
      }
      if (peek() == '=') {
        text.push_back(advance());
        if (peek() == '>') {
          text.push_back(advance());
          return make_token(TokenKind::Spaceship, text, tok_line, tok_col);
        }
        return make_token(TokenKind::LessEqual, text, tok_line, tok_col);
      }
      return make_token(TokenKind::Less, text, tok_line, tok_col);

    case '>':
      if (peek() == '>') {
        text.push_back(advance());
        if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::GreaterGreaterAssign, text, tok_line, tok_col); }
        return make_token(TokenKind::GreaterGreater, text, tok_line, tok_col);
      }
      if (peek() == '=') { text.push_back(advance()); return make_token(TokenKind::GreaterEqual, text, tok_line, tok_col); }
      return make_token(TokenKind::Greater, text, tok_line, tok_col);

    case '#':
      if (peek() == '#') { text.push_back(advance()); return make_token(TokenKind::HashHash, text, tok_line, tok_col); }
      return make_token(TokenKind::Hash, text, tok_line, tok_col);

    default:
      return make_token(TokenKind::Invalid, text, tok_line, tok_col);
  }
}

}  // namespace c4c

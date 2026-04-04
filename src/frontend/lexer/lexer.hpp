#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace c4c {

// C lexer: tokenizes preprocessed C source into a Token stream.
// Pure-C backport note: replace class with a Lexer struct + free functions.
class Lexer {
 public:
  explicit Lexer(std::string source,
                 LexProfile profile = LexProfile::C);
  std::vector<Token> scan_all();

 private:
  char peek() const;
  char peek_next() const;
  char peek2() const;
  bool at_end() const;
  char advance();
  void skip_whitespace_and_comments();
  bool consume_line_marker();

  Token make_token(TokenKind kind, std::string lexeme, int line, int col) const;
  Token scan_identifier_or_keyword();
  Token scan_number();
  Token scan_string();
  Token scan_char();
  Token scan_punct();

  void scan_float_suffix(std::string &text, bool &is_float);
  void scan_int_suffix(std::string &text);

  std::string source_;
  LexProfile  lex_profile_;
  std::size_t index_ = 0;
  int line_   = 1;
  int column_ = 1;
  std::string current_file_;

  // Pending pragma tokens detected during whitespace/comment skipping.
  bool has_pending_pragma_pack_ = false;
  Token pending_pragma_pack_{};
  bool has_pending_pragma_weak_ = false;
  Token pending_pragma_weak_{};
  bool has_pending_pragma_gcc_visibility_ = false;
  Token pending_pragma_gcc_visibility_{};
  bool has_pending_pragma_exec_ = false;
  Token pending_pragma_exec_{};
};

}  // namespace c4c

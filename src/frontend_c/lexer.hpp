#pragma once

#include <string>
#include <vector>

#include "token.hpp"

namespace tinyc2ll::frontend_cxx {

class Lexer {
 public:
  explicit Lexer(std::string source);
  std::vector<Token> scan_all();

 private:
  char peek() const;
  char peek_next() const;
  bool at_end() const;
  char advance();
  void skip_whitespace_and_comments();
  Token make_token(TokenKind kind, std::string lexeme, int line, int column) const;
  Token scan_identifier_or_keyword();
  Token scan_number();
  Token scan_string();
  Token scan_char();
  Token scan_punct();

  std::string source_;
  std::size_t index_ = 0;
  int line_ = 1;
  int column_ = 1;
};

}  // namespace tinyc2ll::frontend_cxx


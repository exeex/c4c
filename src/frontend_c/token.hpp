#pragma once

#include <string>

namespace tinyc2ll::frontend_cxx {

enum class TokenKind {
  Identifier,
  Number,
  String,
  Char,
  Punct,
  EndOfFile,
  Invalid,
};

struct Token {
  TokenKind kind;
  std::string lexeme;
  int line;
  int column;
};

const char *token_kind_name(TokenKind kind);

}  // namespace tinyc2ll::frontend_cxx


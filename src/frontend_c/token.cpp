#include "token.hpp"

namespace tinyc2ll::frontend_cxx {

const char *token_kind_name(TokenKind kind) {
  switch (kind) {
    case TokenKind::Identifier:
      return "IDENT";
    case TokenKind::Number:
      return "NUM";
    case TokenKind::String:
      return "STR";
    case TokenKind::Char:
      return "CHAR";
    case TokenKind::Punct:
      return "PUNCT";
    case TokenKind::EndOfFile:
      return "EOF";
    case TokenKind::Invalid:
      return "INVALID";
  }
  return "UNKNOWN";
}

}  // namespace tinyc2ll::frontend_cxx


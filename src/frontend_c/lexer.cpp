#include "lexer.hpp"

#include <cctype>

namespace tinyc2ll::frontend_cxx {

Lexer::Lexer(std::string source) : source_(std::move(source)) {}

std::vector<Token> Lexer::scan_all() {
  std::vector<Token> out;
  while (true) {
    skip_whitespace_and_comments();
    if (at_end()) {
      out.push_back(make_token(TokenKind::EndOfFile, "", line_, column_));
      break;
    }

    char c = peek();
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
      out.push_back(scan_identifier_or_keyword());
      continue;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
      out.push_back(scan_number());
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

char Lexer::peek() const {
  return at_end() ? '\0' : source_[index_];
}

char Lexer::peek_next() const {
  if (index_ + 1 >= source_.size()) {
    return '\0';
  }
  return source_[index_ + 1];
}

bool Lexer::at_end() const {
  return index_ >= source_.size();
}

char Lexer::advance() {
  if (at_end()) {
    return '\0';
  }
  char c = source_[index_++];
  if (c == '\n') {
    line_ += 1;
    column_ = 1;
  } else {
    column_ += 1;
  }
  return c;
}

void Lexer::skip_whitespace_and_comments() {
  while (!at_end()) {
    char c = peek();
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      advance();
      continue;
    }
    if (c == '/' && peek_next() == '/') {
      while (!at_end() && peek() != '\n') {
        advance();
      }
      continue;
    }
    if (c == '/' && peek_next() == '*') {
      advance();
      advance();
      while (!at_end()) {
        if (peek() == '*' && peek_next() == '/') {
          advance();
          advance();
          break;
        }
        advance();
      }
      continue;
    }
    break;
  }
}

Token Lexer::make_token(TokenKind kind, std::string lexeme, int line, int column) const {
  return Token{kind, std::move(lexeme), line, column};
}

Token Lexer::scan_identifier_or_keyword() {
  int tok_line = line_;
  int tok_col = column_;
  std::string text;
  while (!at_end()) {
    char c = peek();
    if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
      text.push_back(advance());
      continue;
    }
    break;
  }
  return make_token(TokenKind::Identifier, std::move(text), tok_line, tok_col);
}

Token Lexer::scan_number() {
  int tok_line = line_;
  int tok_col = column_;
  std::string text;
  while (!at_end() && std::isdigit(static_cast<unsigned char>(peek()))) {
    text.push_back(advance());
  }
  return make_token(TokenKind::Number, std::move(text), tok_line, tok_col);
}

Token Lexer::scan_string() {
  int tok_line = line_;
  int tok_col = column_;
  std::string text;
  text.push_back(advance());
  while (!at_end()) {
    char c = advance();
    text.push_back(c);
    if (c == '\\' && !at_end()) {
      text.push_back(advance());
      continue;
    }
    if (c == '"') {
      return make_token(TokenKind::String, std::move(text), tok_line, tok_col);
    }
  }
  return make_token(TokenKind::Invalid, std::move(text), tok_line, tok_col);
}

Token Lexer::scan_char() {
  int tok_line = line_;
  int tok_col = column_;
  std::string text;
  text.push_back(advance());
  while (!at_end()) {
    char c = advance();
    text.push_back(c);
    if (c == '\\' && !at_end()) {
      text.push_back(advance());
      continue;
    }
    if (c == '\'') {
      return make_token(TokenKind::Char, std::move(text), tok_line, tok_col);
    }
  }
  return make_token(TokenKind::Invalid, std::move(text), tok_line, tok_col);
}

Token Lexer::scan_punct() {
  int tok_line = line_;
  int tok_col = column_;
  std::string text;
  text.push_back(advance());
  return make_token(TokenKind::Punct, std::move(text), tok_line, tok_col);
}

}  // namespace tinyc2ll::frontend_cxx


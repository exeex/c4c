#pragma once

#include <string>
#include <string_view>
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
  TextTable& text_table() { return text_table_; }
  const TextTable& text_table() const { return text_table_; }
  FileTable& file_table() { return file_table_; }
  const FileTable& file_table() const { return file_table_; }

 private:
  char peek() const;
  char peek_next() const;
  char peek2() const;
  bool at_end() const;
  char advance();
  void skip_whitespace_and_comments();
  bool consume_line_marker();

  Token make_token(TokenKind kind, std::string_view lexeme, int line, int col);
  Token scan_identifier_or_keyword();
  Token scan_number();
  Token scan_string(std::string prefix = {});
  Token scan_char(std::string prefix = {});
  Token scan_punct();

  void scan_float_suffix(std::string &text, bool &is_float);
  void scan_int_suffix(std::string &text);

  std::string source_;
  LexProfile  lex_profile_;
  TextTable text_table_;
  FileTable file_table_;
  std::size_t index_ = 0;
  int line_   = 1;
  int column_ = 1;
  FileId current_file_id_ = kInvalidFile;

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

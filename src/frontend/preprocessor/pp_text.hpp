#ifndef TINYC2LL_FRONTEND_CXX_PP_TEXT_HPP
#define TINYC2LL_FRONTEND_CXX_PP_TEXT_HPP

#include <string>

namespace tinyc2ll::frontend_cxx {

// Lexical utility: trim leading/trailing whitespace.
std::string trim_copy(const std::string& s);

// Lexical utility: identifier character classification.
bool is_ident_start(char c);
bool is_ident_continue(char c);

// Translation phase 2: join backslash-newline continuations.
std::string join_continued_lines(const std::string& source);

// Translation phase 3: strip C-style comments (// and /* */).
std::string strip_comments(const std::string& source);

}  // namespace tinyc2ll::frontend_cxx

#endif

#include "pp_text.hpp"

#include <cctype>

namespace c4c {

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

std::string join_continued_lines(const std::string& source) {
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

std::string strip_comments(const std::string& source) {
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

}  // namespace c4c

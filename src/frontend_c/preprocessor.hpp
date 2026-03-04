#ifndef TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP
#define TINYC2LL_FRONTEND_CXX_PREPROCESSOR_HPP

#include <string>

namespace tinyc2ll::frontend_cxx {

class Preprocessor {
public:
  // Preprocess C source file into plain text for lexer/parser.
  //
  // Backend priority:
  // 1) ref/claudes-c-compiler ccc (if prebuilt and available in PATH as "ccc")
  // 2) system "cc -E -P"
  // 3) system "cpp -E -P"
  // 4) fallback: raw file content (best-effort)
  std::string preprocess_file(const std::string& path) const;
};

}  // namespace tinyc2ll::frontend_cxx

#endif

#ifndef TINYC2LL_FRONTEND_CXX_PP_PRAGMA_HPP
#define TINYC2LL_FRONTEND_CXX_PP_PRAGMA_HPP

#include <string>

namespace tinyc2ll::frontend_cxx {

enum class PragmaResult {
  Handled,
  Ignored,
  Unhandled,
};

// Dispatch a #pragma directive. Returns how it was handled.
// |args| is the text after "#pragma " (trimmed).
// |file| and |line_no| are for diagnostics.
PragmaResult dispatch_pragma(const std::string& args,
                             const std::string& file, int line_no);

}  // namespace tinyc2ll::frontend_cxx

#endif

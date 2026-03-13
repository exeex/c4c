#include "pp_pragma.hpp"
#include "pp_text.hpp"

namespace tinyc2ll::frontend_cxx {

PragmaResult dispatch_pragma(const std::string& args,
                             const std::string& /*file*/, int /*line_no*/) {
  std::string s = trim_copy(args);
  if (s.empty()) return PragmaResult::Ignored;

  // TODO(preprocessor): implement pragma handlers:
  //   once, pack, push_macro, pop_macro, weak,
  //   redefine_extname, GCC visibility push/pop
  (void)s;
  return PragmaResult::Unhandled;
}

}  // namespace tinyc2ll::frontend_cxx

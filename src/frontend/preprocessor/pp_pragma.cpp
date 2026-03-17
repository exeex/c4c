#include "pp_pragma.hpp"
#include "pp_text.hpp"

#include <cctype>

namespace c4c {

PragmaResult dispatch_pragma(const std::string& args,
                             const std::string& /*file*/, int /*line_no*/) {
  std::string s = trim_copy(args);
  if (s.empty()) return PragmaResult::Ignored;

  // Extract the first token to identify the pragma kind.
  size_t i = 0;
  while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i])) && s[i] != '(')
    ++i;
  std::string kind = s.substr(0, i);

  // "once" is handled by the caller (Preprocessor::process_directive).

  // Pragmas that are safe to ignore in a compiler frontend:
  if (kind == "push_macro" || kind == "pop_macro" ||
      kind == "redefine_extname" || kind == "comment" ||
      kind == "message" || kind == "warning" || kind == "error" ||
      kind == "poison" || kind == "system_header" || kind == "clang" ||
      kind == "GCC") {
    return PragmaResult::Ignored;
  }

  // Unknown pragmas — ignore rather than triggering fallback.
  // Real compilers typically just warn on unknown pragmas.
  return PragmaResult::Ignored;
}

}  // namespace c4c

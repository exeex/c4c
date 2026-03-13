#ifndef TINYC2LL_FRONTEND_CXX_PP_MACRO_EXPAND_HPP
#define TINYC2LL_FRONTEND_CXX_PP_MACRO_EXPAND_HPP

#include <string>
#include <utility>
#include <vector>

namespace tinyc2ll::frontend_cxx {

// Collect balanced parenthesised argument list for function-like macro expansion.
// *pos must point at the character AFTER the opening '('. On return *pos points
// at the character after the closing ')'.
std::vector<std::string> collect_funclike_args(const std::string& line, size_t* pos);

// Stringify a raw argument for the # operator (C11 6.10.3.2).
std::string stringify_arg(const std::string& raw);

// Look up a named parameter in the macro parameter list.
// Returns 0-based index, or -1 if not found.
int find_param_idx(const std::vector<std::string>& params, const std::string& name);

// Perform parameter substitution on a function-like macro body.
std::string substitute_funclike_body(const std::string& body,
                                     const std::vector<std::string>& params,
                                     const std::vector<std::string>& raw_args,
                                     const std::vector<std::string>& exp_args,
                                     bool variadic,
                                     const std::string& va_raw,
                                     const std::string& va_exp);

// Split a directive line (text after '#') into (keyword, rest).
std::pair<std::string, std::string> split_directive(const std::string& line);

}  // namespace tinyc2ll::frontend_cxx

#endif

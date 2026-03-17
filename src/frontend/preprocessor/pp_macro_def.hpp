#ifndef TINYC2LL_FRONTEND_CXX_PP_MACRO_DEF_HPP
#define TINYC2LL_FRONTEND_CXX_PP_MACRO_DEF_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace c4c {

struct MacroDef {
  std::string name;
  bool function_like = false;
  bool variadic = false;
  std::vector<std::string> params;
  std::string body;
  // GNU named variadic: e.g., #define F(fmt, args...) — va_name = "args"
  // Empty for standard variadic (... / __VA_ARGS__).
  std::string va_name;
};

using MacroTable = std::unordered_map<std::string, MacroDef>;

}  // namespace c4c

#endif

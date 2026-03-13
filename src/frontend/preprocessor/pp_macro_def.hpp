#ifndef TINYC2LL_FRONTEND_CXX_PP_MACRO_DEF_HPP
#define TINYC2LL_FRONTEND_CXX_PP_MACRO_DEF_HPP

#include <string>
#include <unordered_map>
#include <vector>

namespace tinyc2ll::frontend_cxx {

struct MacroDef {
  std::string name;
  bool function_like = false;
  bool variadic = false;
  std::vector<std::string> params;
  std::string body;
};

using MacroTable = std::unordered_map<std::string, MacroDef>;

}  // namespace tinyc2ll::frontend_cxx

#endif

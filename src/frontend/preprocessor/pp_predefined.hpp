#ifndef TINYC2LL_FRONTEND_CXX_PP_PREDEFINED_HPP
#define TINYC2LL_FRONTEND_CXX_PP_PREDEFINED_HPP

#include "pp_macro_def.hpp"

namespace c4c {

// Populate |table| with predefined macros for the default target (LP64 aarch64).
void init_predefined_macros(MacroTable& table, const std::string& target_triple = "");

}  // namespace c4c

#endif

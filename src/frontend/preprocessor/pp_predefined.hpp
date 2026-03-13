#ifndef TINYC2LL_FRONTEND_CXX_PP_PREDEFINED_HPP
#define TINYC2LL_FRONTEND_CXX_PP_PREDEFINED_HPP

#include "pp_macro_def.hpp"

namespace tinyc2ll::frontend_cxx {

// Populate |table| with predefined macros for the default target (LP64 aarch64).
void init_predefined_macros(MacroTable& table);

}  // namespace tinyc2ll::frontend_cxx

#endif

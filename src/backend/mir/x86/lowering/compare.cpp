#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view compare_summary() {
  return "comparison lowering owns compare/setcc/jcc policy after contract recovery";
}

}  // namespace c4c::backend::x86::lowering

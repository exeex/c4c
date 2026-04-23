#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view call_summary() {
  return "call lowering owns argument/result ABI shaping; behavior recovery is still pending";
}

}  // namespace c4c::backend::x86::lowering

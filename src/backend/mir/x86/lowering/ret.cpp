#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view ret_summary() {
  return "return lowering owns ABI result publication and terminal epilogue shaping";
}

}  // namespace c4c::backend::x86::lowering

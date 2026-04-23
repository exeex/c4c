#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view float_summary() {
  return "float lowering owns xmm/scalar-fp emission policy once behavior recovery starts";
}

}  // namespace c4c::backend::x86::lowering

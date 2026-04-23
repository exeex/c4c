#include "lowering.hpp"

namespace c4c::backend::x86::lowering {

std::string_view atomics_summary() {
  return "atomics/intrinsics lowering is deferred behind the contract-first x86 backend pass";
}

}  // namespace c4c::backend::x86::lowering

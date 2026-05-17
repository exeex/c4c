#include "peephole.hpp"

#include <utility>

namespace c4c::backend::aarch64::codegen {

std::string apply_deferred_peephole_boundary(std::string assembly) {
  // Own the current no-live-pass decision; archived text-first rewrites stay
  // deferred until a structured or explicit printer-output cleanup policy lands.
  return std::move(assembly);
}

}  // namespace c4c::backend::aarch64::codegen

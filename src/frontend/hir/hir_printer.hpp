#pragma once

#include <string>

#include "hir_ir.hpp"

namespace c4c::hir {

// Return a human-readable dump of the entire HIR module.
// Suitable for --dump-hir debug output.
std::string format_hir(const Module& module);

}  // namespace c4c::hir

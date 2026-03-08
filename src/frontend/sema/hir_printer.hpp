#pragma once

#include <string>

#include "ir.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

// Return a human-readable dump of the entire HIR module.
// Suitable for --dump-hir debug output.
std::string format_hir(const Module& module);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir

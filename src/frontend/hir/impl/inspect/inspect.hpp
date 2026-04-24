#pragma once

// Private HIR inspection index.
//
// Implementation/debug internals that need the full HIR dump surface can use
// this boundary. App-facing callers should include hir_printer.hpp explicitly;
// the core public facade, hir.hpp, intentionally exposes only build_hir(...)
// and format_summary(...).

#include "../../hir_printer.hpp"

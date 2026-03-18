#pragma once

#include <string>

#include "ast.hpp"
#include "canonical_symbol.hpp"
#include "hir_printer.hpp"
#include "ir.hpp"

namespace c4c::hir {

Module build_hir(const Node* program_root,
                 const sema::ResolvedTypeTable* resolved_types = nullptr);
std::string format_summary(const Module& module);

// Compatibility alias while call sites transition to the new naming.
inline Module lower_ast_to_hir(
    const Node* program_root,
    const sema::ResolvedTypeTable* resolved_types = nullptr) {
  return build_hir(program_root, resolved_types);
}

}  // namespace c4c::hir

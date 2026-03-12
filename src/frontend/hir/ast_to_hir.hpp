#pragma once

#include <string>

#include "ast.hpp"
#include "hir_printer.hpp"
#include "ir.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

Module lower_ast_to_hir(const Node* program_root);
std::string format_summary(const Module& module);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir

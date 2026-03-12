#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "ir.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

std::vector<long long> decode_string_literal_values(const char* sval, bool wide);
std::string bytes_from_string_literal(const StringLiteral& sl);
std::optional<long long> eval_int_const_expr(
    const Node* n,
    const std::unordered_map<std::string, long long>& enum_consts);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir

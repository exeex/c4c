#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ir.hpp"

namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir {

// ── Constant-evaluation value model ──────────────────────────────────────────

enum class ConstValueKind {
  Integer,
  Boolean,
  NullPtr,
  Unknown,
};

struct ConstValue {
  ConstValueKind kind = ConstValueKind::Unknown;
  long long int_val = 0;

  static ConstValue make_int(long long v) { return {ConstValueKind::Integer, v}; }
  static ConstValue make_bool(bool v) { return {ConstValueKind::Boolean, v ? 1LL : 0LL}; }
  static ConstValue make_null() { return {ConstValueKind::NullPtr, 0}; }
  static ConstValue unknown() { return {ConstValueKind::Unknown, 0}; }

  bool is_known() const { return kind != ConstValueKind::Unknown; }
  long long as_int() const { return int_val; }
};

struct ConstEvalResult {
  std::optional<ConstValue> value;

  bool ok() const { return value.has_value() && value->is_known(); }
  long long as_int() const { return value->as_int(); }
  std::optional<long long> as_optional_int() const {
    if (ok()) return value->as_int();
    return std::nullopt;
  }

  static ConstEvalResult success(ConstValue v) { return {v}; }
  static ConstEvalResult failure() { return {std::nullopt}; }
};

// ── Constant-evaluation environment ──────────────────────────────────────────

struct ConstEvalEnv {
  const std::unordered_map<std::string, long long>* enum_consts = nullptr;
  const std::unordered_map<std::string, long long>* named_consts = nullptr;
  const std::unordered_map<std::string, long long>* local_consts = nullptr;

  std::optional<long long> lookup(const std::string& name) const {
    if (enum_consts) {
      auto it = enum_consts->find(name);
      if (it != enum_consts->end()) return it->second;
    }
    if (local_consts) {
      auto it = local_consts->find(name);
      if (it != local_consts->end()) return it->second;
    }
    if (named_consts) {
      auto it = named_consts->find(name);
      if (it != named_consts->end()) return it->second;
    }
    return std::nullopt;
  }
};

// ── Unified constant-expression evaluation API ───────────────────────────────

ConstEvalResult evaluate_constant_expr(const Node* n, const ConstEvalEnv& env);

// ── Legacy API (thin wrapper) ────────────────────────────────────────────────

std::optional<long long> eval_int_const_expr(
    const Node* n,
    const std::unordered_map<std::string, long long>& enum_consts,
    const std::unordered_map<std::string, long long>* named_consts = nullptr);

// ── String literal helpers ───────────────────────────────────────────────────

std::vector<long long> decode_string_literal_values(const char* sval, bool wide);
std::string bytes_from_string_literal(const StringLiteral& sl);

}  // namespace tinyc2ll::frontend_cxx::sema_ir::phase2::hir

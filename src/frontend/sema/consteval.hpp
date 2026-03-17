#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ir.hpp"

namespace c4c::hir {

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
  std::string error;  // non-empty on failure: describes why evaluation failed

  bool ok() const { return value.has_value() && value->is_known(); }
  long long as_int() const { return value->as_int(); }
  std::optional<long long> as_optional_int() const {
    if (ok()) return value->as_int();
    return std::nullopt;
  }

  static ConstEvalResult success(ConstValue v) { return {v, {}}; }
  static ConstEvalResult failure(std::string msg = {}) { return {std::nullopt, std::move(msg)}; }
};

// ── Constant-evaluation environment ──────────────────────────────────────────

using ConstMap = std::unordered_map<std::string, long long>;

// Map from template parameter name to concrete TypeSpec (for template-substituted evaluation).
using TypeBindings = std::unordered_map<std::string, TypeSpec>;

struct ConstEvalEnv {
  // Flat maps (used by ast_to_hir.cpp where scoping is managed externally).
  const ConstMap* enum_consts = nullptr;
  const ConstMap* named_consts = nullptr;
  const ConstMap* local_consts = nullptr;

  // Scoped maps (used by validate.cpp where block scoping uses vector-of-maps).
  // Searched innermost (back) to outermost (front).
  const std::vector<ConstMap>* enum_scopes = nullptr;
  const std::vector<ConstMap>* local_const_scopes = nullptr;

  // Template type substitution map (template param name → concrete type).
  const TypeBindings* type_bindings = nullptr;

  // Non-type template parameter bindings (NTTP name → constant value).
  const std::unordered_map<std::string, long long>* nttp_bindings = nullptr;

  std::optional<long long> lookup(const std::string& name) const {
    // 1. Scoped enum constants (innermost first).
    if (enum_scopes) {
      for (auto it = enum_scopes->rbegin(); it != enum_scopes->rend(); ++it) {
        auto sit = it->find(name);
        if (sit != it->end()) return sit->second;
      }
    }
    // 2. Flat enum constants.
    if (enum_consts) {
      auto it = enum_consts->find(name);
      if (it != enum_consts->end()) return it->second;
    }
    // 3. Scoped local constants (innermost first).
    if (local_const_scopes) {
      for (auto it = local_const_scopes->rbegin(); it != local_const_scopes->rend(); ++it) {
        auto sit = it->find(name);
        if (sit != it->end()) return sit->second;
      }
    }
    // 4. Flat local constants.
    if (local_consts) {
      auto it = local_consts->find(name);
      if (it != local_consts->end()) return it->second;
    }
    // 5. Named constants (global const/constexpr bindings).
    if (named_consts) {
      auto it = named_consts->find(name);
      if (it != named_consts->end()) return it->second;
    }
    // 6. Non-type template parameter bindings.
    if (nttp_bindings) {
      auto it = nttp_bindings->find(name);
      if (it != nttp_bindings->end()) return it->second;
    }
    return std::nullopt;
  }
};

// ── Unified constant-expression evaluation API ───────────────────────────────

ConstEvalResult evaluate_constant_expr(const Node* n, const ConstEvalEnv& env);

// ── Consteval function-body interpreter ──────────────────────────────────────

// Evaluate a consteval function call at compile time.
// `func_def` must be an NK_FUNCTION node with is_consteval=true.
// `args` are the constant values for each parameter.
// `consteval_fns` maps function names to their NK_FUNCTION AST nodes,
// allowing recursive/chained consteval calls.
ConstEvalResult evaluate_consteval_call(
    const Node* func_def,
    const std::vector<ConstValue>& args,
    const ConstEvalEnv& env,
    const std::unordered_map<std::string, const Node*>& consteval_fns,
    int depth = 0);

// ── String literal helpers ───────────────────────────────────────────────────

std::vector<long long> decode_string_literal_values(const char* sval, bool wide);
std::string bytes_from_string_literal(const StringLiteral& sl);

}  // namespace c4c::hir

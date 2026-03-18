#pragma once

#include "ir.hpp"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace c4c::hir {

// ── Template instantiation types ────────────────────────────────────────────

/// Origin of a template seed — tracks how the instantiation was discovered.
enum class TemplateSeedOrigin {
  DirectCall,
  EnclosingTemplateExpansion,
  DeducedCall,
  ConstevalSeed,
  ConstevalEnclosingExpansion,
};

/// A discovered template application candidate (todo list item).
struct TemplateSeedWorkItem {
  std::string template_name;
  TypeBindings bindings;
  NttpBindings nttp_bindings;
  std::string mangled_name;
  SpecializationKey spec_key;
  TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall;
};

/// A realized template instantiation (authoritative for lowering).
struct TemplateInstance {
  TypeBindings bindings;
  NttpBindings nttp_bindings;  // non-type template param → constant value
  std::string mangled_name;
  SpecializationKey spec_key;  // stable identity for dedup/caching
};

// ── Template mangling utilities ─────────────────────────────────────────────

/// Produce a deterministic type suffix for name mangling.
inline std::string type_suffix_for_mangling(const TypeSpec& ts) {
  if (ts.ptr_level > 0) return "p" + std::to_string(ts.ptr_level);
  switch (ts.base) {
    case TB_BOOL: return "b";
    case TB_CHAR: case TB_SCHAR: return "c";
    case TB_UCHAR: return "uc";
    case TB_SHORT: return "s";
    case TB_USHORT: return "us";
    case TB_INT: return "i";
    case TB_UINT: return "ui";
    case TB_LONG: return "l";
    case TB_ULONG: return "ul";
    case TB_LONGLONG: return "ll";
    case TB_ULONGLONG: return "ull";
    case TB_FLOAT: return "f";
    case TB_DOUBLE: return "d";
    case TB_LONGDOUBLE: return "ld";
    case TB_INT128: return "i128";
    case TB_UINT128: return "u128";
    case TB_VOID: return "v";
    default:
      if (ts.tag) return std::string("T") + ts.tag;
      return "unknown";
  }
}

/// Check whether all type bindings are concrete (no unresolved typedefs).
inline bool bindings_are_concrete(const TypeBindings& bindings) {
  for (const auto& [param, ts] : bindings) {
    if (ts.base == TB_TYPEDEF) return false;
  }
  return true;
}

/// Build a mangled name from base name and template bindings.
inline std::string mangle_template_name(const std::string& base,
                                        const TypeBindings& bindings,
                                        const NttpBindings& nttp_bindings = {}) {
  if (bindings.empty() && nttp_bindings.empty()) return base;
  // Collect all param names for deterministic ordering.
  std::vector<std::string> all_params;
  for (const auto& [k, v] : bindings) all_params.push_back(k);
  for (const auto& [k, v] : nttp_bindings) all_params.push_back(k);
  std::sort(all_params.begin(), all_params.end());
  std::string result = base;
  for (const auto& param : all_params) {
    result += "_";
    auto nttp_it = nttp_bindings.find(param);
    if (nttp_it != nttp_bindings.end()) {
      // NTTP: encode value (use 'n' prefix for negatives).
      if (nttp_it->second < 0)
        result += "n" + std::to_string(-nttp_it->second);
      else
        result += std::to_string(nttp_it->second);
    } else {
      auto it = bindings.find(param);
      if (it != bindings.end()) result += type_suffix_for_mangling(it->second);
    }
  }
  return result;
}

// ── InstantiationRegistry ───────────────────────────────────────────────────
//
// Centralized bookkeeping for template instantiation discovery and
// realization.  Both AST-side seed collection and HIR-side deferred
// instantiation go through this registry so that dedup, specialization
// lookup, and instance recording share a single code path.
//
// Ownership split:
//   seed_work   – discovered template application candidates (todo list)
//   instances   – realized instantiations (authoritative for lowering)
//   specs       – explicit specialization AST nodes keyed by mangled name

class InstantiationRegistry {
 public:
  // ── Queries ──────────────────────────────────────────────────────────

  /// O(1) dedup check: has this (fn_name, mangled) pair been realized?
  bool has_instance(const std::string& fn_name,
                    const std::string& mangled) const {
    return instance_keys_.count(fn_name + "::" + mangled) > 0;
  }

  /// O(1) dedup check: has this (fn_name, mangled) pair been recorded as a seed?
  bool has_seed(const std::string& fn_name,
                const std::string& mangled) const {
    return seed_keys_.count(fn_name + "::" + mangled) > 0;
  }

  /// O(1) dedup check: has this pair been recorded as either seed or instance?
  bool has_seed_or_instance(const std::string& fn_name,
                            const std::string& mangled) const {
    std::string key = fn_name + "::" + mangled;
    return seed_keys_.count(key) > 0 || instance_keys_.count(key) > 0;
  }

  /// Look up realized instances for a template function (may be empty).
  const std::vector<TemplateInstance>* find_instances(
      const std::string& fn_name) const {
    auto it = instances_.find(fn_name);
    if (it == instances_.end()) return nullptr;
    return &it->second;
  }

  /// Look up explicit specialization AST node by mangled name.
  const Node* find_specialization(const std::string& mangled) const {
    auto it = specs_.find(mangled);
    if (it == specs_.end()) return nullptr;
    return it->second;
  }

  /// All realized instances (for iteration, e.g., metadata population).
  const std::unordered_map<std::string, std::vector<TemplateInstance>>&
  all_instances() const { return instances_; }

  /// All seed work items (for debugging / realize_seeds()).
  const std::unordered_map<std::string, std::vector<TemplateSeedWorkItem>>&
  all_seeds() const { return seed_work_; }

  // ── Mutations ────────────────────────────────────────────────────────

  /// Register an explicit specialization.
  void register_specialization(const std::string& mangled,
                               const Node* spec_node) {
    specs_[mangled] = spec_node;
  }

  /// Record a template instantiation seed.  Returns the mangled name,
  /// or "" if bindings are not concrete.  Instances are created
  /// exclusively by realize_seeds().
  std::string record_seed(
      const std::string& fn_name, TypeBindings bindings,
      NttpBindings nttp_bindings,
      const std::vector<std::string>& param_order,
      TemplateSeedOrigin origin = TemplateSeedOrigin::DirectCall) {
    if (!bindings_are_concrete(bindings)) return "";
    std::string mangled = mangle_template_name(fn_name, bindings,
                                                nttp_bindings);
    std::string key = fn_name + "::" + mangled;
    if (seed_keys_.count(key) > 0) return mangled;  // already recorded
    SpecializationKey sk = nttp_bindings.empty()
        ? make_specialization_key(fn_name, param_order, bindings)
        : make_specialization_key(fn_name, param_order, bindings,
                                  nttp_bindings);
    seed_work_[fn_name].push_back(
        TemplateSeedWorkItem{fn_name, bindings, nttp_bindings, mangled,
                             sk, origin});
    seed_keys_.insert(std::move(key));
    return mangled;
  }

  /// Realize all seeds that have not yet been converted to instances.
  /// Returns the number of newly realized instances.
  size_t realize_seeds() {
    size_t realized = 0;
    for (const auto& [fn_name, seeds] : seed_work_) {
      for (const auto& seed : seeds) {
        std::string key = fn_name + "::" + seed.mangled_name;
        if (instance_keys_.count(key) > 0) continue;  // already realized
        instances_[fn_name].push_back(
            {seed.bindings, seed.nttp_bindings, seed.mangled_name,
             seed.spec_key});
        instance_keys_.insert(std::move(key));
        ++realized;
      }
    }
    return realized;
  }

  /// Compute total instance count across all template functions.
  size_t total_instance_count() const {
    size_t n = 0;
    for (const auto& [k, v] : instances_) n += v.size();
    return n;
  }

  /// Compute total seed count across all template functions.
  size_t total_seed_count() const {
    size_t n = 0;
    for (const auto& [k, v] : seed_work_) n += v.size();
    return n;
  }

  /// Verify that every seed has a corresponding realized instance and
  /// vice versa.  Returns true when seeds and instances are in perfect
  /// parity.
  bool verify_parity() const {
    // Every seed must be realized.
    for (const auto& [fn_name, seeds] : seed_work_) {
      for (const auto& s : seeds) {
        if (instance_keys_.count(fn_name + "::" + s.mangled_name) == 0)
          return false;
      }
    }
    // Every instance must have a seed.
    for (const auto& [fn_name, insts] : instances_) {
      for (const auto& inst : insts) {
        if (seed_keys_.count(fn_name + "::" + inst.mangled_name) == 0)
          return false;
      }
    }
    return total_seed_count() == total_instance_count();
  }

  /// Dump seed/instance parity report to the given stream.
  void dump_parity(FILE* out) const {
    size_t seeds = total_seed_count();
    size_t insts = total_instance_count();
    std::fprintf(out, "[InstantiationRegistry] seeds=%zu instances=%zu",
                 seeds, insts);
    if (seeds == insts)
      std::fprintf(out, " (parity OK)\n");
    else
      std::fprintf(out, " (MISMATCH)\n");

    // Report per-function detail.
    std::set<std::string> all_fns;
    for (const auto& [k, _] : seed_work_) all_fns.insert(k);
    for (const auto& [k, _] : instances_) all_fns.insert(k);
    for (const auto& fn : all_fns) {
      auto sit = seed_work_.find(fn);
      auto iit = instances_.find(fn);
      size_t s = sit != seed_work_.end() ? sit->second.size() : 0;
      size_t i = iit != instances_.end() ? iit->second.size() : 0;
      const char* status = (s == i) ? "ok" : "MISMATCH";
      std::fprintf(out, "  %-40s seeds=%-3zu instances=%-3zu %s\n",
                   fn.c_str(), s, i, status);

      // List unrealized seeds.
      if (sit != seed_work_.end()) {
        for (const auto& seed : sit->second) {
          bool realized = instance_keys_.count(fn + "::" + seed.mangled_name) > 0;
          if (!realized)
            std::fprintf(out, "    UNREALIZED seed: %s\n",
                         seed.mangled_name.c_str());
        }
      }
      // List orphan instances (instance without seed).
      if (iit != instances_.end()) {
        for (const auto& inst : iit->second) {
          bool has_s = seed_keys_.count(fn + "::" + inst.mangled_name) > 0;
          if (!has_s)
            std::fprintf(out, "    ORPHAN instance: %s\n",
                         inst.mangled_name.c_str());
        }
      }
    }
  }

 private:
  // Seed / todo list — discovered template application candidates.
  std::unordered_map<std::string, std::vector<TemplateSeedWorkItem>>
      seed_work_;
  // Realized instances — authoritative for lowering.
  std::unordered_map<std::string, std::vector<TemplateInstance>>
      instances_;
  // O(1) dedup set for realized instances: "fn_name::mangled_name".
  std::unordered_set<std::string> instance_keys_;
  // O(1) dedup set for seeds: "fn_name::mangled_name".
  std::unordered_set<std::string> seed_keys_;
  // Explicit specializations: mangled_name → AST node.
  std::unordered_map<std::string, const Node*> specs_;
};

// ── CompileTimeState ─────────────────────────────────────────────────────
//
// Engine-owned state that travels through the HIR pipeline.
// Created during initial HIR construction (build_initial_hir), then
// passed to the compile-time engine (run_compile_time_engine) for
// iterative reduction.  This is the intended single source of truth
// for compile-time bookkeeping.

struct CompileTimeState {
  InstantiationRegistry registry;

  // ── Template / consteval definition registries ──────────────────────
  //
  // These maps give the compile-time engine direct visibility into which
  // template and consteval functions exist, without having to probe the
  // opaque Lowerer callback.  The Lowerer populates them during initial
  // HIR construction; the engine reads them during the fixpoint loop.

  /// Register a template function definition (AST node pointer).
  void register_template_def(const std::string& name, const Node* node) {
    template_fn_defs_[name] = node;
  }

  /// Register a consteval function definition (AST node pointer).
  void register_consteval_def(const std::string& name, const Node* node) {
    consteval_fn_defs_[name] = node;
  }

  /// Check whether a template function definition is known.
  bool has_template_def(const std::string& name) const {
    return template_fn_defs_.count(name) > 0;
  }

  /// Check whether a consteval function definition is known.
  bool has_consteval_def(const std::string& name) const {
    return consteval_fn_defs_.count(name) > 0;
  }

  /// Check whether a registered template definition is marked consteval.
  /// Returns true only if the definition exists AND has is_consteval set.
  bool is_consteval_template(const std::string& name) const {
    auto it = template_fn_defs_.find(name);
    if (it == template_fn_defs_.end()) return false;
    return it->second && it->second->is_consteval;
  }

  /// Look up a template function definition by name (nullptr if unknown).
  const Node* find_template_def(const std::string& name) const {
    auto it = template_fn_defs_.find(name);
    return it != template_fn_defs_.end() ? it->second : nullptr;
  }

  /// Look up a consteval function definition by name (nullptr if unknown).
  const Node* find_consteval_def(const std::string& name) const {
    auto it = consteval_fn_defs_.find(name);
    return it != consteval_fn_defs_.end() ? it->second : nullptr;
  }

  /// Const reference to the internal consteval function definition map.
  /// Useful for passing to evaluate_consteval_call() which expects a map ref.
  const std::unordered_map<std::string, const Node*>& consteval_fn_defs() const {
    return consteval_fn_defs_;
  }

  /// Number of registered template function definitions.
  size_t template_def_count() const { return template_fn_defs_.size(); }

  /// Number of registered consteval function definitions.
  size_t consteval_def_count() const { return consteval_fn_defs_.size(); }

  /// Iterate over all registered template function definitions.
  template<typename Fn>
  void for_each_template_def(Fn&& fn) const {
    for (const auto& [name, node] : template_fn_defs_)
      fn(name, node);
  }

  /// Record a deferred template instance discovered during the engine's
  /// fixpoint loop.  Updates the registry (seed + realize) and returns a
  /// HirTemplateInstantiation suitable for appending to module metadata.
  HirTemplateInstantiation record_deferred_instance(
      const std::string& source_template,
      const TypeBindings& bindings,
      const NttpBindings& nttp_bindings,
      const std::string& mangled_name,
      const std::vector<std::string>& template_params) {
    registry.record_seed(source_template, bindings, nttp_bindings,
                         template_params,
                         TemplateSeedOrigin::EnclosingTemplateExpansion);
    registry.realize_seeds();
    HirTemplateInstantiation hi;
    hi.mangled_name = mangled_name;
    hi.bindings = bindings;
    hi.nttp_bindings = nttp_bindings;
    hi.spec_key = nttp_bindings.empty()
        ? make_specialization_key(source_template, template_params, bindings)
        : make_specialization_key(source_template, template_params, bindings,
                                  nttp_bindings);
    return hi;
  }

  /// Convert all realized instances for a given template into
  /// HirTemplateInstantiation metadata objects.  Used to populate
  /// module.template_defs during initial HIR construction.
  std::vector<HirTemplateInstantiation> instances_to_hir_metadata(
      const std::string& fn_name,
      const std::vector<std::string>& template_params) const {
    std::vector<HirTemplateInstantiation> result;
    auto* inst_list = registry.find_instances(fn_name);
    if (!inst_list) return result;
    result.reserve(inst_list->size());
    for (const auto& inst : *inst_list) {
      HirTemplateInstantiation hi;
      hi.mangled_name = inst.mangled_name;
      hi.bindings = inst.bindings;
      hi.nttp_bindings = inst.nttp_bindings;
      hi.spec_key = inst.nttp_bindings.empty()
          ? make_specialization_key(fn_name, template_params, inst.bindings)
          : make_specialization_key(fn_name, template_params, inst.bindings,
                                    inst.nttp_bindings);
      result.push_back(std::move(hi));
    }
    return result;
  }

  // ── Constant environment (for consteval evaluation) ──────────────────
  //
  // The Lowerer populates these maps during initial HIR construction
  // by calling register_enum_const() / register_const_int_binding().
  // The engine uses them to build a ConstEvalEnv when evaluating
  // PendingConstevalExpr nodes directly, without going through the
  // Lowerer callback.
  //
  // NOTE: The Lowerer also keeps its own working copies of these maps
  // because it needs mutable save/restore semantics for block-scoped
  // enum definitions (see NK_BLOCK handling in lower_stmt_node).
  // The CompileTimeState copies accumulate the final global state and
  // are used exclusively during the engine normalization phase.

  /// Register an enum constant value.
  void register_enum_const(const std::string& name, long long value) {
    enum_consts_[name] = value;
  }

  /// Register a global const-integer binding.
  void register_const_int_binding(const std::string& name, long long value) {
    const_int_bindings_[name] = value;
  }

  /// Evaluate a consteval function call using engine-owned state.
  /// Evaluates a consteval function call using engine-owned state,
  /// operating on definitions and constant environment owned by
  /// CompileTimeState rather than Lowerer internals.
  std::optional<long long> evaluate_consteval(
      const std::string& fn_name,
      const std::vector<long long>& const_args,
      const TypeBindings& bindings,
      const NttpBindings& nttp_bindings) const;

  /// Dump debug visibility for seed work vs realized instances.
  void dump(FILE* out) const {
    std::fprintf(out, "[CompileTimeState] template_defs=%zu consteval_defs=%zu\n",
                 template_fn_defs_.size(), consteval_fn_defs_.size());
    std::fprintf(out, "[CompileTimeState] enum_consts=%zu const_int_bindings=%zu\n",
                 enum_consts_.size(), const_int_bindings_.size());
    std::fprintf(out, "[CompileTimeState] registry parity:\n");
    registry.dump_parity(out);
  }

 private:
  // Template function definitions indexed by name (AST node pointers).
  std::unordered_map<std::string, const Node*> template_fn_defs_;
  // Consteval function definitions indexed by name (AST node pointers).
  std::unordered_map<std::string, const Node*> consteval_fn_defs_;
  // Enum constant values (name → value).
  std::unordered_map<std::string, long long> enum_consts_;
  // Global const-integer bindings (name → value).
  std::unordered_map<std::string, long long> const_int_bindings_;
};

/// A diagnostic for a single irreducible compile-time node.
struct CompileTimeDiagnostic {
  enum Kind { UnresolvedTemplate, UnreducedConsteval };
  Kind kind;
  std::string description;  // human-readable description of the irreducible node
};

/// Result of a single compile-time reduction iteration.
struct CompileTimeEngineStats {
  size_t templates_instantiated = 0;  // template calls with resolved target functions
  size_t templates_pending = 0;       // template calls whose target is missing
  size_t templates_deferred = 0;      // template instantiations created by this pass
  size_t consteval_reduced = 0;       // consteval calls successfully reduced to constants
  size_t consteval_pending = 0;       // consteval calls whose result is missing or invalid
  size_t consteval_deferred = 0;      // consteval reductions unlocked by deferred template instantiation
  size_t iterations = 0;              // total fixpoint iterations performed
  bool converged = false;             // true if no new work was found
  // Definition registries (populated when ct_state is provided).
  size_t template_defs_known = 0;      // template function definitions registered
  size_t consteval_defs_known = 0;     // consteval function definitions registered
  // Registry parity (populated when ct_state is provided).
  size_t registry_seeds = 0;           // total seeds in registry after engine run
  size_t registry_instances = 0;       // total realized instances in registry after engine run
  bool registry_parity = true;         // true if seeds == instances (all realized)
  std::vector<CompileTimeDiagnostic> diagnostics;  // details on irreducible nodes
};

/// Callback for deferred template instantiation.
///
/// Called by the compile-time reduction pass when it discovers a template
/// call whose target function has not been lowered yet.
///
/// Parameters:
///   template_name  - original template name (e.g. "add")
///   bindings       - resolved type bindings (e.g. {T: int})
///   mangled_name   - target mangled name (e.g. "add_i")
///
/// Returns true if the function was successfully lowered and added to the module.
using DeferredInstantiateFn = std::function<bool(
    const std::string& template_name,
    const TypeBindings& bindings,
    const NttpBindings& nttp_bindings,
    const std::string& mangled_name)>;

/// Run the HIR compile-time reduction loop.
///
/// This pass iterates:
///   1. template instantiation for newly-ready applications
///   2. consteval reduction for newly-ready immediate calls
///   3. pending consteval evaluation for deferred consteval nodes
/// until convergence or the iteration limit is reached.
///
/// When instantiate_fn is provided, the pass will invoke it to lower template
/// functions that were not instantiated during initial AST-to-HIR lowering
/// (e.g., nested template calls like add<T>() inside twice<T>()).
/// Consteval evaluation uses ct_state->evaluate_consteval() directly.
/// When callbacks are null, the pass only verifies existing state.
CompileTimeEngineStats run_compile_time_engine(
    Module& module,
    std::shared_ptr<CompileTimeState> ct_state = nullptr,
    DeferredInstantiateFn instantiate_fn = nullptr);

/// Format pass statistics for debug output (used by --dump-hir).
std::string format_compile_time_stats(const CompileTimeEngineStats& stats);

/// Result of materialization.
struct MaterializationStats {
  size_t materialized = 0;      // functions marked for emission
  size_t non_materialized = 0;  // functions kept for compile-time only
};

/// Mark functions in the module for materialization.
///
/// Current policy: all concrete functions are materialized.  This pass
/// makes the materialization decision explicit and separable from codegen,
/// so future policies (lazy emission, JIT deferral) can override it.
MaterializationStats materialize_ready_functions(Module& module);

/// Format materialization stats for debug output.
std::string format_materialization_stats(const MaterializationStats& stats);

}  // namespace c4c::hir

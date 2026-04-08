#pragma once

#include "../shared/llvm_helpers.hpp"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace c4c::codegen::lir {

struct LirModule;

/// Standalone constant-initializer emitter for global variables.
/// Owns all const-eval logic previously in HirEmitter; depends only on
/// the HIR Module (read-only) and LirModule (for string interning).
class ConstInitEmitter {
 public:
  ConstInitEmitter(const c4c::hir::Module& mod, LirModule& module);

  /// Emit the LLVM constant initializer text for a global variable.
  std::string emit_const_init(const TypeSpec& ts, const c4c::hir::GlobalInit& init);

  /// Emit per-field initializer strings for a struct (used for flexible-array structs).
  std::vector<std::string> emit_const_struct_fields(
      const TypeSpec& ts,
      const c4c::hir::HirStructDef& sd,
      const c4c::hir::GlobalInit& init,
      std::vector<TypeSpec>* out_field_types = nullptr);

 private:
  const c4c::hir::Module& mod_;
  LirModule& module_;

  // ── Expr lookup ─────────────────────────────────────────────────────────
  const c4c::hir::Expr& get_expr(c4c::hir::ExprId id) const;

  // ── Field type reconstruction ───────────────────────────────────────────
  TypeSpec field_decl_type(const c4c::hir::HirStructField& f) const;

  // ── Global object lookup ────────────────────────────────────────────────
  const c4c::hir::GlobalVar* select_global_object(const std::string& name) const;
  const c4c::hir::GlobalVar* select_global_object(c4c::hir::GlobalId id) const;

  // ── Constant evaluation ─────────────────────────────────────────────────
  std::optional<long long> try_const_eval_int(c4c::hir::ExprId id);
  std::optional<double> try_const_eval_float(c4c::hir::ExprId id);
  std::optional<std::pair<long long, long long>> try_const_eval_complex_int(c4c::hir::ExprId id);
  std::optional<std::pair<double, double>> try_const_eval_complex_fp(c4c::hir::ExprId id);
  std::string emit_const_int_like(long long value, const TypeSpec& expected_ts);
  std::string emit_const_scalar_expr(c4c::hir::ExprId id, const TypeSpec& expected_ts);

  // ── String/byte helpers ─────────────────────────────────────────────────
  std::string intern_str(const std::string& raw_bytes);
  static bool is_char_like(TypeBase b);
  static TypeSpec drop_one_array_dim(TypeSpec ts);
  static std::string bytes_from_string_literal(const c4c::hir::StringLiteral& sl);
  static std::vector<long long> decode_wide_string_values(const std::string& raw);
  static std::string escape_llvm_c_bytes(const std::string& raw_bytes);

  // ── Aggregate init helpers (moved from nested ConstInitEmitter) ─────────
  c4c::hir::GlobalInit child_init_of(const c4c::hir::InitListItem& item) const;
  std::string emit_const_vector(const TypeSpec& ts, const c4c::hir::GlobalInit& init);
  std::string emit_const_array(const TypeSpec& ts, const c4c::hir::GlobalInit& init);
  std::optional<std::string> try_emit_ptr_from_char_init(const c4c::hir::GlobalInit& init);
  TypeSpec resolve_flexible_array_field_ts(const c4c::hir::HirStructField& f,
                                           const c4c::hir::InitListItem* item,
                                           const c4c::hir::GlobalInit& init);
  std::optional<size_t> find_union_field_index(const c4c::hir::HirStructDef& union_sd,
                                                const c4c::hir::InitListItem& item) const;
  std::optional<long long> find_array_index(const c4c::hir::InitListItem& item,
                                             long long next_idx, long long bound) const;
  bool is_explicitly_mapped_item(const c4c::hir::InitListItem& item) const;
  bool is_explicitly_mapped_list(const c4c::hir::InitList& list) const;
  bool is_indexed_list(const c4c::hir::InitList& list) const;
  std::optional<size_t> find_struct_field_index(const c4c::hir::HirStructDef& sd,
                                                 const c4c::hir::InitListItem& item,
                                                 size_t next_idx) const;
  std::optional<std::pair<size_t, c4c::hir::GlobalInit>> try_select_canonical_union_field_init(
      const c4c::hir::HirStructDef& union_sd, const c4c::hir::GlobalInit& union_init) const;
  std::vector<std::string> emit_const_struct_fields_impl(const TypeSpec& ts,
                                                          const c4c::hir::HirStructDef& sd,
                                                          const c4c::hir::GlobalInit& init,
                                                          std::vector<TypeSpec>* out_field_types);
  std::string emit_const_struct(const TypeSpec& ts, const c4c::hir::GlobalInit& init);
  std::string emit_const_union(const TypeSpec& ts, const c4c::hir::HirStructDef& sd,
                                const c4c::hir::GlobalInit& init);
  std::string format_array_literal(const TypeSpec& elem_ts,
                                    const std::vector<std::string>& elems) const;
  std::string format_struct_literal(const c4c::hir::HirStructDef& sd,
                                     const std::vector<std::string>& field_vals) const;
  long long deduce_array_size_from_init(const c4c::hir::GlobalInit& init) const;
};

}  // namespace c4c::codegen::lir

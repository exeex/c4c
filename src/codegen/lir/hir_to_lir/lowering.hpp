#pragma once

#include "../../shared/llvm_helpers.hpp"
#include "../call_args.hpp"
#include "../../shared/fn_lowering_ctx.hpp"
#include "../../shared/struct_name_table.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Private implementation index for `src/codegen/lir/hir_to_lir/`.
//
// Agents working on parent-level HIR-to-LIR lowering should start here for the
// shared lowering context, module orchestration helpers, constant initializer
// support, and the common StmtEmitter surface. Keep call-only and expression-
// only helper details in `call/call.hpp` and `expr/expr.hpp`; this header should
// name those subdomains without becoming their declaration dump.

namespace c4c::codegen::llvm_backend {
struct Amd64VarargInfo;
}

namespace c4c::codegen::lir {

struct LirModule;
struct LirStructDecl;

using namespace c4c;
using namespace c4c::hir;
using namespace c4c::codegen::llvm_helpers;

// Import shared FnCtx / BlockMeta from codegen::shared into this namespace.
using c4c::codegen::FnCtx;
using c4c::codegen::BlockMeta;

// ── Shared lowering context ────────────────────────────────────────────────

struct BitfieldAccess {
  int bit_width = -1;
  int bit_offset = 0;
  int storage_unit_bits = 0;
  bool is_signed = false;
  bool is_bitfield() const { return bit_width >= 0; }
};

// ── Struct field lookup ───────────────────────────────────────────────────

struct FieldStep {
  std::string tag;
  int llvm_idx = 0;
  bool is_union = false;
  // Bitfield metadata (bit_width >= 0 means this is a bitfield access)
  int bit_width = -1;
  int bit_offset = 0;
  int storage_unit_bits = 0;
  bool bf_is_signed = false;
};

struct MemberFieldAccess {
  TypeSpec base_ts{};
  std::string tag;
  std::vector<FieldStep> chain;
  TypeSpec field_ts{};
  BitfieldAccess bf{};
  bool field_found = false;

  bool has_tag() const { return !tag.empty(); }
};

struct AssignableLValue {
  std::string ptr;
  TypeSpec pointee_ts{};
  BitfieldAccess bf{};

  bool is_bitfield() const { return bf.is_bitfield(); }
};

struct LoadedAssignableValue {
  std::string value;
  TypeSpec value_ts{};
};

#define C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES
#include "call/call.hpp"
#undef C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES

// ── Parent module orchestration ─────────────────────────────────────────────

/// Deduplicate globals: prefer entries with explicit initializers; among
/// equals, prefer later entries (last-wins for extern/tentative semantics).
/// Returns indices into mod.globals of the "best" entry per name, in original
/// order.
std::vector<size_t> dedup_globals(const Module& mod);

/// Deduplicate functions: prefer definitions (non-empty blocks) over
/// declarations. Skips non-materialized functions. Returns indices into
/// mod.functions of the "best" entry per name, in original order.
std::vector<size_t> dedup_functions(const Module& mod);

/// Build LLVM struct/union type declaration strings from the HIR module's
/// struct definitions. When a LIR module is supplied, also records the same
/// layout into the structured declaration mirror.
std::vector<std::string> build_type_decls(const Module& mod,
                                          LirModule* lir_module = nullptr);

/// Build the LLVM IR signature text for a function (define/declare line).
/// Ownership of signature construction belongs to hir_to_lir, not StmtEmitter.
std::string build_fn_signature(const Module& mod, const Function& fn);

/// Compute the HIR block iteration order for a function: entry block first,
/// then remaining blocks in their original order.
std::vector<const Block*> build_block_order(const Function& fn);

/// Find parameter indices that are modified (assigned to, incremented, or
/// address-taken) anywhere in the function body.
std::unordered_set<uint32_t> find_modified_params(const Module& mod, const Function& fn);

/// Returns true if the function has any VLA (variable-length array) locals.
bool fn_has_vla_locals(const Function& fn);

/// Hoist alloca instructions for all locals and spilled parameters.
void hoist_allocas(FnCtx& ctx, const Module& mod, const Function& fn);

/// Initialize a FnCtx for the given function.
FnCtx init_fn_ctx(const Module& mod, const Function& fn);

/// Map a HIR BlockId to its LLVM IR label string.
std::string block_lbl(BlockId id);

/// Create a new LIR block with the given label and make it current in ctx.
void emit_lbl(FnCtx& ctx, const std::string& lbl);

// ── Constant initializer lowering ──────────────────────────────────────────

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
  const c4c::hir::GlobalVar* select_global_object(const c4c::hir::DeclRef& ref) const;

  // ── Constant evaluation ─────────────────────────────────────────────────
  std::optional<long long> try_const_eval_int(c4c::hir::ExprId id);
  std::optional<double> try_const_eval_float(c4c::hir::ExprId id);
  std::optional<std::pair<long long, long long>> try_const_eval_complex_int(c4c::hir::ExprId id);
  std::optional<std::pair<double, double>> try_const_eval_complex_fp(c4c::hir::ExprId id);
  std::string emit_const_int_like(long long value, const TypeSpec& expected_ts);
  std::string emit_const_scalar_expr(c4c::hir::ExprId id, const TypeSpec& expected_ts);
  std::optional<std::string> try_emit_global_address_expr(c4c::hir::ExprId id);

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

namespace stmt_emitter_detail {

// Common helpers shared by parent lowering plus statement, lvalue, expression,
// and call subdomains. More specialized call or expression helpers belong in
// their subdomain indexes.

inline constexpr int kAmd64GpAreaBytes = 48;
inline constexpr int kAmd64FpAreaBytes = 176;

bool amd64_registers_available(const llvm_backend::Amd64VarargInfo& layout,
                               const Amd64CallArgState& state);
void amd64_track_usage(const llvm_backend::Amd64VarargInfo& layout,
                       Amd64CallArgState& state);
void amd64_account_type_if_needed(const hir::Module& mod, const TypeSpec& ts,
                                  Amd64CallArgState* state);
bool amd64_fixed_aggregate_byval(const hir::Module& mod, const TypeSpec& ts);

bool set_terminator_if_open(FnCtx& ctx, lir::LirTerminator terminator);

void open_lbl(FnCtx& ctx, const std::string& lbl);
void emit_condbr_and_open_lbl(FnCtx& ctx, const std::string& cond,
                              const std::string& true_label,
                              const std::string& false_label,
                              const std::string& open_label);
void emit_condbr_and_open_sibling_lbl(FnCtx& ctx, const std::string& cond,
                                      const std::string& true_label,
                                      const std::string& false_label,
                                      const std::string& sibling_label);
void emit_condbr_and_fallthrough_lbl(FnCtx& ctx, const std::string& cond,
                                     const std::string& true_label,
                                     const std::string& false_label);

TypeSpec sig_return_type(const FnPtrSig& sig);
TypeSpec sig_param_type(const FnPtrSig& sig, size_t i);
bool sig_param_is_va_list_value(const FnPtrSig& sig, size_t i);
size_t sig_param_count(const FnPtrSig& sig);
bool sig_is_variadic(const FnPtrSig& sig);
bool sig_has_void_param_list(const FnPtrSig& sig);
bool sig_has_meaningful_prototype(const FnPtrSig& sig);
std::string llvm_fn_type_suffix_str(const hir::Module& mod, const FnPtrSig& sig);
std::string llvm_fn_type_suffix_str(const hir::Module& mod, const Function& fn);
int llvm_struct_field_slot(const hir::Module& mod, const HirStructDef& sd,
                           int target_llvm_idx);
int llvm_struct_field_slot(const HirStructDef& sd, int target_llvm_idx);
int llvm_struct_base_slot(const hir::Module& mod, const HirStructDef& sd,
                          size_t base_index);
int llvm_struct_field_slot_by_name(const HirStructDef& sd, const std::string& field_name);

struct StructuredLayoutLookup {
  const HirStructDef* legacy_decl = nullptr;
  const LirStructDecl* structured_decl = nullptr;
  StructNameId structured_name_id = kInvalidStructName;
  bool structured_lookup_attempted = false;
  bool structured_parity_checked = false;
  bool structured_parity_matches = false;
};

StructuredLayoutLookup lookup_structured_layout(const Module& mod,
                                                const LirModule* lir_module,
                                                const TypeSpec& ts);

struct Aarch64HomogeneousFpAggregateInfo {
  std::string elem_ty;
  int elem_count = 0;
  int elem_size = 0;
  int aggregate_size = 0;
  int aggregate_align = 0;
};

std::optional<Aarch64HomogeneousFpAggregateInfo> classify_aarch64_hfa(
    const Module& mod, const TypeSpec& ts);
int round_up_to(int value, int align);
int object_align_bytes(const Module& mod, const LirModule* lir_module,
                       const TypeSpec& ts);
int object_align_bytes(const Module& mod, const TypeSpec& ts);

}  // namespace stmt_emitter_detail

// ── Statement/lvalue core and subdomain entry points ───────────────────────

class StmtEmitter {
 public:
  explicit StmtEmitter(const Module& m);

  /// Set the working LirModule by reference.  The caller (hir_to_lir::lower)
  /// owns the module throughout; the emitter writes into it during per-item
  /// lowering.
  void set_module(lir::LirModule& module);

  /// Lower a single HIR function into the working LirModule.
  /// Emit a single HIR statement into the FnCtx.  Exposed so that
  /// hir_to_lir::lower() can drive block iteration directly.
  void emit_stmt(FnCtx& ctx, const Stmt& stmt);

 private:
  const Module& mod_;
  lir::LirModule* module_ = nullptr;  // Non-owning ref to caller's LirModule
  mutable std::unordered_map<uint32_t, FnPtrSig> inferred_direct_fn_sigs_;

  /// Create a new LIR block with the given label and make it current in ctx.
  void emit_lbl(FnCtx& ctx, const std::string& lbl);

  /// Map a HIR BlockId to its LLVM IR label string.
  static std::string block_lbl(BlockId id);

  // ── Instruction helpers ───────────────────────────────────────────────────

  // Push a typed LIR instruction (non-terminator) into the current block.
  void emit_lir_op(FnCtx& ctx, lir::LirInst op);
  void emit_term_br(FnCtx& ctx, const std::string& target_label);
  void emit_term_condbr(FnCtx& ctx, const std::string& cond,
                        const std::string& true_label, const std::string& false_label);
  void emit_term_ret(FnCtx& ctx, const std::string& type_str,
                     const std::optional<std::string>& value_str);
  void emit_term_switch(FnCtx& ctx, const std::string& sel_name,
                        const std::string& sel_type, const std::string& default_label,
                        std::vector<std::pair<long long, std::string>> cases);
  void emit_term_unreachable(FnCtx& ctx);
  void emit_br_and_open_lbl(FnCtx& ctx, const std::string& branch_label,
                            const std::string& open_label);
  void emit_fallthrough_lbl(FnCtx& ctx, const std::string& lbl);
  std::string fresh_tmp(FnCtx& ctx);
  void record_extern_call_decl(const std::string& name, const std::string& ret_ty,
                               LinkNameId link_name_id = kInvalidLinkName);
  std::string fresh_lbl(FnCtx& ctx, const std::string& pfx);


  // ── String constant pool ──────────────────────────────────────────────────
  std::string intern_str(const std::string& raw_bytes);

  // NOTE: preamble (type decls) generation moved to lir::build_type_decls().


  // ── Globals ───────────────────────────────────────────────────────────────
  static bool is_char_like(TypeBase b);
  static TypeSpec drop_one_array_dim(TypeSpec ts);
  static TypeSpec drop_one_indexed_element_type(TypeSpec ts);
  static TypeSpec resolve_indexed_gep_pointee_type(TypeSpec ts);
  static std::string bytes_from_string_literal(const StringLiteral& sl);

  // Decode a wide string literal (L"...") into wchar_t (i32) values with null terminator
  static std::vector<long long> decode_wide_string_values(const std::string& raw);
  static std::string escape_llvm_c_bytes(const std::string& raw_bytes);
  TypeSpec field_decl_type(const HirStructField& f) const;

  // Format an integer-like value as an LLVM constant (handles ptr/float targets).
  std::string emit_const_int_like(long long value, const TypeSpec& expected_ts);

 private:

  const GlobalVar* select_global_object(const std::string& name) const;
  const GlobalVar* select_global_object(GlobalId id) const;
  const GlobalVar* select_global_object(const DeclRef& ref) const;


  // ── Expr lookup ───────────────────────────────────────────────────────────
  const Expr& get_expr(ExprId id) const;

  // ── Coerce ────────────────────────────────────────────────────────────────
  std::string coerce(FnCtx& ctx, const std::string& val,
                     const TypeSpec& from_ts, const TypeSpec& to_ts);


  // ── to_bool: convert any value to i1 ─────────────────────────────────────
  std::string to_bool(FnCtx& ctx, const std::string& val, const TypeSpec& ts);


  // Recursively find field_name in struct/union identified by tag.
  // On success, appends steps to chain (outermost first).
  // Returns true and sets out_field_ts.
  bool find_field_chain(const std::string& tag, const std::string& field_name,
                        std::vector<FieldStep>& chain, TypeSpec& out_field_ts);
  bool resolve_field_access(const std::string& tag, const std::string& field_name,
                            std::vector<FieldStep>& chain, TypeSpec& out_field_ts,
                            BitfieldAccess* out_bf = nullptr);
  bool find_field_chain_by_member_symbol_id(const std::string& tag, MemberSymbolId member_symbol_id,
                                            std::vector<FieldStep>& chain,
                                            TypeSpec& out_field_ts);
  bool resolve_field_access_by_member_symbol_id(const std::string& tag,
                                                MemberSymbolId member_symbol_id,
                                                std::vector<FieldStep>& chain,
                                                TypeSpec& out_field_ts,
                                                BitfieldAccess* out_bf = nullptr);

  // Emit GEP chain for struct member access.
  // base_ptr: LLVM value (ptr to struct), chain: resolved field access path.
  // Returns LLVM value (ptr to field).
  std::string emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                              const std::vector<FieldStep>& chain);


  // ── Bitfield load/store helpers ─────────────────────────────────────────────

  // Determine the promoted LLVM type for a bitfield.
  // C integer promotion: if bit_width fits in int (<=31 unsigned, <=32 signed),
  // promote to i32. Otherwise keep the storage unit type (e.g. i64).
  static int bitfield_promoted_bits(const BitfieldAccess& bf);
  static TypeBase bitfield_promoted_base(int bit_width, bool is_signed, int storage_unit_bits);
  static TypeSpec bitfield_promoted_ts(const BitfieldAccess& bf);
  // Load a bitfield value from a storage unit pointer.
  // Returns a value of the promoted type (i32 for <=32-bit fields, i64 for wider).
  std::string emit_bitfield_load(FnCtx& ctx, const std::string& unit_ptr,
                                  const BitfieldAccess& bf);
  // Store a value into a bitfield within a storage unit.
  void emit_bitfield_store(FnCtx& ctx, const std::string& unit_ptr,
                            const BitfieldAccess& bf,
                            const std::string& new_val, const TypeSpec& val_ts);

  // ── Lvalue emission ───────────────────────────────────────────────────────
  std::string emit_lval(FnCtx& ctx, ExprId id, TypeSpec& pointee_ts);
  std::string emit_va_list_obj_ptr(FnCtx& ctx, ExprId id, TypeSpec& ts);
  std::string emit_lval_dispatch(FnCtx& ctx, const Expr& e, TypeSpec& pts);
  TypeSpec resolve_member_base_type(FnCtx& ctx, ExprId base_id, bool is_arrow);
  MemberFieldAccess resolve_member_field_access(FnCtx& ctx, const MemberExpr& m);
  std::string emit_member_base_ptr(FnCtx& ctx, const MemberExpr& m, TypeSpec& base_ts);
  std::string emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                BitfieldAccess* out_bf = nullptr);
  AssignableLValue emit_assignable_lval(FnCtx& ctx, ExprId id);
  LoadedAssignableValue emit_load_assignable_value(FnCtx& ctx, const AssignableLValue& lhs);
  std::string emit_store_assignable_value(FnCtx& ctx, const AssignableLValue& lhs,
                                          const std::string& value,
                                          const TypeSpec& value_ts,
                                          bool reload_after_store);
  std::string emit_assignable_incdec_value(FnCtx& ctx, const AssignableLValue& lhs,
                                           bool increment, bool return_updated_value);
  std::string emit_set_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                    const std::string& rhs, const TypeSpec& rhs_ts);
  std::string emit_compound_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                         AssignOp op, const std::string& rhs,
                                         const TypeSpec& rhs_ts);
  static TypeSpec resolve_compound_assign_op_type(BinaryOp op, const TypeSpec& lhs_ts,
                                                  const TypeSpec& rhs_ts);
  std::string emit_nonptr_compound_assign_value(FnCtx& ctx, const AssignableLValue& lhs,
                                                const LoadedAssignableValue& loaded,
                                                BinaryOp op, const char* instr,
                                                const std::string& rhs,
                                                const TypeSpec& rhs_ts);
  std::string indexed_gep_elem_ty(const TypeSpec& base_ts);
  std::string emit_indexed_gep(FnCtx& ctx, const std::string& base_ptr,
                               const TypeSpec& base_ts, const std::string& idx);
  std::string emit_rval_from_access_expr(FnCtx& ctx, const Expr& e,
                                         const std::string& ptr,
                                         const TypeSpec& access_ts,
                                         bool decay_from_array_object);
  std::string emit_rval_from_access_ptr(FnCtx& ctx, const std::string& ptr,
                                        const TypeSpec& access_ts, const TypeSpec& load_ts,
                                        bool decay_from_array_object);
  // ── Call subdomain ───────────────────────────────────────────────────────
  //
  // Member declarations are indexed from `hir_to_lir/call/call.hpp` so call
  // lowering agents have one private entry point for args, target, builtin,
  // generic call emission, and va_arg helpers.
#define C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS
#include "call/call.hpp"
#undef C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS

  // ── Expression subdomain entry points ────────────────────────────────────
  //
  // Member declarations are indexed from `hir_to_lir/expr/expr.hpp` so
  // expression-lowering agents have one private entry point for type
  // resolution, rval coordination, binary helpers, and miscellaneous payloads.
#define C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS
#include "expr/expr.hpp"
#undef C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS

  // ── Statement emission ────────────────────────────────────────────────────

  void emit_stmt_impl(FnCtx& ctx, const LocalDecl& d);
  void emit_stmt_impl(FnCtx& ctx, const ExprStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const InlineAsmStmt& s);
  void emit_non_control_flow_stmt(FnCtx& ctx, const LocalDecl& d);
  void emit_non_control_flow_stmt(FnCtx& ctx, const ExprStmt& s);
  void emit_non_control_flow_stmt(FnCtx& ctx, const InlineAsmStmt& s);

  void emit_control_flow_stmt(FnCtx& ctx, const ReturnStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const IfStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const WhileStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const ForStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const DoWhileStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const SwitchStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const GotoStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const IndirBrStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const LabelStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const BreakStmt& s);
  void emit_control_flow_stmt(FnCtx& ctx, const ContinueStmt& s);
  void emit_switch_label_stmt(FnCtx&, const CaseStmt&);
  void emit_switch_label_stmt(FnCtx&, const CaseRangeStmt&);
  void emit_switch_label_stmt(FnCtx&, const DefaultStmt&);

  void emit_stmt_impl(FnCtx& ctx, const ReturnStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const IfStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const WhileStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const ForStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const DoWhileStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const SwitchStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const GotoStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const IndirBrStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const LabelStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const BreakStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const ContinueStmt& s);
  void emit_stmt_impl(FnCtx&, const CaseStmt&);
  void emit_stmt_impl(FnCtx&, const CaseRangeStmt&);
  void emit_stmt_impl(FnCtx&, const DefaultStmt&);

};



}  // namespace c4c::codegen::lir

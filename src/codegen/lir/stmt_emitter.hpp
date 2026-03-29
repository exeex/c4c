#pragma once

#include "../shared/llvm_helpers.hpp"
#include "ir.hpp"
#include "../shared/fn_lowering_ctx.hpp"

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
#include <utility>
#include <vector>

namespace c4c::codegen::llvm_backend {
struct Amd64VarargInfo;
}

namespace c4c::codegen::lir {

using namespace c4c;
using namespace c4c::hir;
using namespace c4c::codegen::llvm_helpers;

// Import shared FnCtx / BlockMeta from codegen::shared into this namespace.
using c4c::codegen::FnCtx;
using c4c::codegen::BlockMeta;

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
  const char* tag = nullptr;
  std::vector<FieldStep> chain;
  TypeSpec field_ts{};
  BitfieldAccess bf{};
  bool field_found = false;

  bool has_tag() const { return tag && tag[0]; }
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

struct CallTargetInfo {
  BuiltinId builtin_id = BuiltinId::Unknown;
  const BuiltinInfo* builtin = nullptr;
  std::string fn_name;
  TypeSpec callee_ts{};
  std::string callee_val;
  TypeSpec ret_spec{};
  std::string ret_ty;
  const Function* target_fn = nullptr;
  const FnPtrSig* callee_fn_ptr_sig = nullptr;
  std::string callee_type_suffix;
  bool builtin_special = false;
};

struct PreparedCallArg {
  std::vector<std::string> texts;
  bool skip = false;
};

struct PreparedBuiltinIntArg {
  std::string value;
  std::string llvm_ty;
  bool is_i64 = false;
};

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
  template<typename T>
  void emit_lir_op(FnCtx& ctx, T&& op) {
    // Skip dead code after a terminator has been placed in this block.
    if (!std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator))
      return;
    ctx.cur_block().insts.push_back(std::forward<T>(op));
    ctx.last_term = false;
  }
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
  void record_extern_call_decl(const std::string& name, const std::string& ret_ty);
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
  struct Amd64VaListPtrs {
    std::string gp_offset_ptr;
    std::string fp_offset_ptr;
    std::string overflow_ptr_ptr;
    std::string reg_save_area_ptr;
  };
  Amd64VaListPtrs load_amd64_va_list_ptrs(FnCtx& ctx, const std::string& ap_ptr);
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
  bool callee_needs_va_list_by_value_copy(const CallTargetInfo& call_target,
                                          size_t arg_index) const;
  void apply_default_arg_promotion(FnCtx& ctx, std::string& arg, TypeSpec& out_ts,
                                   const TypeSpec& in_ts);
  PreparedCallArg prepare_call_arg(FnCtx& ctx, const CallExpr& call,
                                   const CallTargetInfo& call_target, size_t arg_index);
  PreparedCallArg prepare_amd64_variadic_aggregate_arg(
      FnCtx& ctx, const TypeSpec& arg_ts, const std::string& obj_ptr,
      int payload_sz);
  std::string prepare_call_args(FnCtx& ctx, const CallExpr& call,
                                const CallTargetInfo& call_target);
  void emit_void_call(FnCtx& ctx, const CallTargetInfo& call_target,
                      const std::string& args_str);
  std::string emit_call_with_result(FnCtx& ctx, const CallTargetInfo& call_target,
                                    const std::string& args_str);
  PreparedBuiltinIntArg prepare_builtin_int_arg(FnCtx& ctx, ExprId arg_id,
                                                BuiltinId builtin_id);
  std::string narrow_builtin_int_result(FnCtx& ctx, const PreparedBuiltinIntArg& arg,
                                        const std::string& value);
  std::string emit_builtin_ffs_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_ctz_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_clz_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_popcount_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_parity_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_clrsb_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  void promote_builtin_fp_predicate_arg(FnCtx& ctx, std::string& value, TypeSpec& value_ts);
  std::string emit_builtin_fp_predicate_result(FnCtx& ctx, const char* predicate,
                                               const std::string& fp_ty,
                                               const std::string& lhs,
                                               const std::string& rhs);
  std::string emit_builtin_fp_compare_call(FnCtx& ctx, const CallExpr& call,
                                           BuiltinId builtin_id);
  std::string emit_builtin_isunordered_call(FnCtx& ctx, const CallExpr& call);
  std::string emit_builtin_isnan_call(FnCtx& ctx, ExprId arg_id);
  std::string emit_builtin_isinf_call(FnCtx& ctx, ExprId arg_id);
  std::string emit_builtin_isfinite_call(FnCtx& ctx, ExprId arg_id);
  void promote_builtin_fp_math_arg(FnCtx& ctx, std::string& value, TypeSpec& value_ts,
                                   BuiltinId builtin_id);
  std::string emit_builtin_copysign_call(FnCtx& ctx, const CallExpr& call,
                                         BuiltinId builtin_id);
  std::string emit_builtin_fabs_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_builtin_conj_call(FnCtx& ctx, ExprId arg_id);
  void promote_builtin_signbit_arg(FnCtx& ctx, std::string& value, TypeSpec& value_ts,
                                   BuiltinId builtin_id);
  std::string emit_builtin_signbit_call(FnCtx& ctx, ExprId arg_id, BuiltinId builtin_id);
  std::string emit_post_builtin_call(FnCtx& ctx, const CallExpr& call,
                                     const CallTargetInfo& call_target);
  std::string emit_amd64_va_arg(FnCtx& ctx, const TypeSpec& res_ts,
                                const std::string& res_ty, const std::string& ap_ptr);
  std::string emit_amd64_va_arg_from_registers(
      FnCtx& ctx, const TypeSpec& res_ts, const std::string& res_ty,
      const c4c::codegen::llvm_backend::Amd64VarargInfo& layout,
      const Amd64VaListPtrs& access, const std::string& gp_offset,
      const std::string& fp_offset);
  std::string emit_amd64_va_arg_from_overflow(FnCtx& ctx, const TypeSpec& res_ts,
                                              const std::string& res_ty,
                                              const Amd64VaListPtrs& access,
                                              int size_bytes);

  // ── Rvalue emission ───────────────────────────────────────────────────────

  // Recursively resolve the C type of an expression from HIR structure.
  // The AST doesn't annotate types on NK_BINOP/NK_VAR nodes, so we infer.
  TypeSpec resolve_expr_type(FnCtx& ctx, ExprId id);
  TypeSpec resolve_expr_type(FnCtx& ctx, const Expr& e);
  const FnPtrSig* resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e);
  CallTargetInfo resolve_call_target_info(FnCtx& ctx, const CallExpr& call, const Expr& e);
  TypeSpec resolve_payload_type(FnCtx& ctx, const DeclRef& r);
  TypeSpec resolve_payload_type(FnCtx& ctx, const BinaryExpr& b);
  TypeSpec resolve_payload_type(FnCtx& ctx, const UnaryExpr& u);
  TypeSpec resolve_payload_type(FnCtx& ctx, const AssignExpr& a);
  TypeSpec resolve_payload_type(FnCtx& ctx, const CastExpr& c);
  TypeSpec resolve_payload_type(FnCtx& ctx, const CallExpr& c);
  TypeSpec resolve_payload_type(FnCtx&, const IntLiteral&);
  TypeSpec resolve_payload_type(FnCtx&, const FloatLiteral&);
  TypeSpec resolve_payload_type(FnCtx&, const CharLiteral&);
  TypeSpec resolve_payload_type(FnCtx&, const StringLiteral&);
  TypeSpec resolve_payload_type(FnCtx& ctx, const MemberExpr& m);
  TypeSpec resolve_payload_type(FnCtx& ctx, const IndexExpr& idx);
  TypeSpec resolve_payload_type(FnCtx& ctx, const TernaryExpr& t);
  TypeSpec resolve_payload_type(FnCtx& ctx, const VaArgExpr& v);
  TypeSpec resolve_payload_type(FnCtx& ctx, const SizeofExpr& s);
  TypeSpec resolve_payload_type(FnCtx& ctx, const SizeofTypeExpr& s);
  TypeSpec resolve_payload_type(FnCtx& ctx, const LabelAddrExpr& la);
  TypeSpec resolve_payload_type(FnCtx& ctx, const PendingConstevalExpr& p);
  template <typename T>
  TypeSpec resolve_payload_type(FnCtx&, const T&);
  std::string emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts);
  std::string emit_rval_expr(FnCtx& ctx, const Expr& e);

  // ── Default payload (unimplemented) ─────────────────────────────────────
  template <typename T>
  std::string emit_rval_payload(FnCtx&, const T&, const Expr&);

  // ── Literal payloads ─────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e);
  std::string emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e);
  std::string emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&);
  std::string emit_complex_binary_arith(FnCtx& ctx,
                                        BinaryOp op,
                                        const std::string& lv,
                                        const TypeSpec& lts,
                                        const std::string& rv,
                                        const TypeSpec& rts,
                                        const TypeSpec& res_spec);
  std::string emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& /*e*/);

  // ── DeclRef ───────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e);

  // ── Unary ─────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e);

  // ── Binary ────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const BinaryExpr& b, const Expr& e);
  std::string emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e);

  // ── Assign ────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& /*e*/);

  // ── Cast ─────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr& /*e*/);

  // ── Call ─────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e);

  // ── VaArg ────────────────────────────────────────────────────────────────
  std::string emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr, int slot_bytes);
  std::string emit_aarch64_vaarg_fp_src_ptr(
      FnCtx& ctx, const std::string& ap_ptr, int reg_slot_bytes, int stack_slot_bytes,
      int stack_align_bytes);
  std::string emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e);

  // ── Ternary ───────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e);

  // ── Sizeof ────────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr&);
  std::string emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr&);

  // ── LabelAddrExpr (GCC &&label extension) ────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr&);

  // ── PendingConstevalExpr ────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const PendingConstevalExpr& p, const Expr& e);

  // ── IndexExpr (rval: load through computed ptr) ──────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const IndexExpr&, const Expr& e);

  // ── MemberExpr ────────────────────────────────────────────────────────────
  std::string emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e);

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

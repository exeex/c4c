#ifndef C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_INDEX_HPP
#define C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_INDEX_HPP

#if !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES) && \
    !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS)
#include "../lowering.hpp"
#endif

// Private implementation index for `src/codegen/lir/hir_to_lir/call/`.
//
// Agents working on call lowering should start here for the call-target model,
// argument preparation state, builtin-call helpers, direct call emission, and
// va_arg helper surface. This remains the single private call index: do not add
// args.hpp, builtin.hpp, target.hpp, or per-ABI headers unless the semantic
// directory itself is split.

#endif  // C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_INDEX_HPP

#if defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES) && \
    !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES_INCLUDED)
#define C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES_INCLUDED

// Call target and argument preparation model. This section is included from
// `lowering.hpp` at namespace scope; the standalone call index includes
// `lowering.hpp` instead.
struct CallTargetInfo {
  BuiltinId builtin_id = BuiltinId::Unknown;
  const BuiltinInfo* builtin = nullptr;
  std::string fn_name;
  TypeSpec callee_ts{};
  std::string callee_val;
  LinkNameId callee_link_name_id = kInvalidLinkName;
  TypeSpec ret_spec{};
  std::string ret_ty;
  const Function* target_fn = nullptr;
  const FnPtrSig* callee_fn_ptr_sig = nullptr;
  std::string callee_type_suffix;
  bool builtin_special = false;
};

struct PreparedCallArg {
  std::vector<OwnedLirTypedCallArg> args;
  bool skip = false;
};

struct Amd64CallArgState {
  int gp_bytes = 0;
  int sse_bytes = 0;
};

struct PreparedBuiltinIntArg {
  std::string value;
  std::string llvm_ty;
  bool is_i64 = false;
};

#endif  // C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_TYPES_INCLUDED

#if defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS) && \
    !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS_INCLUDED)
#define C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS_INCLUDED

// Call argument preparation and generic call emission.
bool callee_needs_va_list_by_value_copy(const CallTargetInfo& call_target,
                                        size_t arg_index) const;
void apply_default_arg_promotion(FnCtx& ctx, std::string& arg, TypeSpec& out_ts,
                                 const TypeSpec& in_ts);
PreparedCallArg prepare_call_arg(FnCtx& ctx, const CallExpr& call,
                                 const CallTargetInfo& call_target, size_t arg_index,
                                 Amd64CallArgState* amd64_state);
PreparedCallArg prepare_amd64_variadic_aggregate_arg(
    FnCtx& ctx, const TypeSpec& arg_ts, const std::string& obj_ptr,
    int payload_sz, Amd64CallArgState* amd64_state);
std::vector<OwnedLirTypedCallArg> prepare_call_args(FnCtx& ctx, const CallExpr& call,
                                                    const CallTargetInfo& call_target);
void emit_void_call(FnCtx& ctx, const CallTargetInfo& call_target,
                    const std::vector<OwnedLirTypedCallArg>& args);
std::string emit_call_with_result(FnCtx& ctx, const CallTargetInfo& call_target,
                                  const std::vector<OwnedLirTypedCallArg>& args);
CallTargetInfo resolve_call_target_info(FnCtx& ctx, const CallExpr& call, const Expr& e);

// Builtin-call lowering helpers.
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

// Call and va_arg expression payload entry points.
std::string emit_rval_payload(FnCtx& ctx, const CallExpr& call, const Expr& e);

struct Amd64VaListPtrs {
  std::string gp_offset_ptr;
  std::string fp_offset_ptr;
  std::string overflow_ptr_ptr;
  std::string reg_save_area_ptr;
};

Amd64VaListPtrs load_amd64_va_list_ptrs(FnCtx& ctx, const std::string& ap_ptr);
std::string emit_aarch64_vaarg_gp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                          int slot_bytes);
std::string emit_aarch64_vaarg_fp_src_ptr(FnCtx& ctx, const std::string& ap_ptr,
                                          int reg_slot_bytes, int stack_slot_bytes,
                                          int stack_align_bytes);
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
std::string emit_rval_payload(FnCtx& ctx, const VaArgExpr& v, const Expr& e);

#endif  // C4C_CODEGEN_LIR_HIR_TO_LIR_CALL_MEMBERS_INCLUDED

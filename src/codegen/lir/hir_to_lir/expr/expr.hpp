#ifndef C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_INDEX_HPP
#define C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_INDEX_HPP

#if !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS)
#include "../lowering.hpp"
#endif

// Private implementation index for `src/codegen/lir/hir_to_lir/expr/`.
//
// Agents working on expression lowering should start here for expression type
// resolution, rval coordination, binary helpers, and the miscellaneous payload
// emitters. This remains the single private expression index: do not add
// binary.hpp, misc.hpp, or one header per expression implementation file.

#endif  // C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_INDEX_HPP

#if defined(C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS) && \
    !defined(C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS_INCLUDED)
#define C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS_INCLUDED

// Expression type resolution and callee-signature inference.
TypeSpec resolve_expr_type(FnCtx& ctx, ExprId id);
TypeSpec resolve_expr_type(FnCtx& ctx, const Expr& e);
const Function* find_local_target_function(LinkNameId link_name_id,
                                           std::string_view fallback_name) const;
const FnPtrSig* resolve_callee_fn_ptr_sig(FnCtx& ctx, const Expr& callee_e);
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

// Rvalue expression coordination.
std::string emit_rval_id(FnCtx& ctx, ExprId id, TypeSpec& out_ts);
std::string emit_rval_expr(FnCtx& ctx, const Expr& e);

// Default payload fallback and literal payloads.
template <typename T>
std::string emit_rval_payload(FnCtx&, const T&, const Expr&);
std::string emit_rval_payload(FnCtx&, const IntLiteral& x, const Expr& e);
std::string emit_rval_payload(FnCtx&, const FloatLiteral& x, const Expr& e);
std::string emit_rval_payload(FnCtx&, const CharLiteral& x, const Expr&);
std::string emit_rval_payload(FnCtx& ctx, const StringLiteral& sl, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const DeclRef& r, const Expr& e);

// Binary expression helpers.
std::string emit_complex_binary_arith(FnCtx& ctx, BinaryOp op,
                                      const std::string& lv,
                                      const TypeSpec& lts,
                                      const std::string& rv,
                                      const TypeSpec& rts,
                                      const TypeSpec& res_spec);
std::string emit_rval_payload(FnCtx& ctx, const BinaryExpr& b, const Expr& e);
std::string emit_logical(FnCtx& ctx, const BinaryExpr& b, const Expr& e);

// Miscellaneous expression payloads.
std::string emit_rval_payload(FnCtx& ctx, const UnaryExpr& u, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const AssignExpr& a, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const CastExpr& c, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const TernaryExpr& t, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const SizeofExpr& s, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const SizeofTypeExpr& s, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const LabelAddrExpr& la, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const PendingConstevalExpr& p, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const IndexExpr& idx, const Expr& e);
std::string emit_rval_payload(FnCtx& ctx, const MemberExpr& m, const Expr& e);

#endif  // C4C_CODEGEN_LIR_HIR_TO_LIR_EXPR_MEMBERS_INCLUDED

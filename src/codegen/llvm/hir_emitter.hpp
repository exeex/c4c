#pragma once

#include "llvm_codegen.hpp"
#include "hir_to_llvm_helpers.hpp"
#include "../lir/ir.hpp"

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

namespace c4c::codegen::llvm_backend {

using namespace c4c;
using namespace c4c::hir;
using namespace c4c::codegen::llvm_backend::detail;

struct BlockMeta {
  std::optional<std::string> break_label;
  std::optional<std::string> continue_label;
};

struct BitfieldAccess {
  int bit_width = -1;
  int bit_offset = 0;
  int storage_unit_bits = 0;
  bool is_signed = false;
  bool is_bitfield() const { return bit_width >= 0; }
};

// ── Per-function state ────────────────────────────────────────────────────

struct FnCtx {
  const Function* fn = nullptr;
  int tmp_idx = 0;
  bool last_term = false;
  // local_id.value → alloca slot (e.g. "%lv.x")
  std::unordered_map<uint32_t, std::string> local_slots;
  // local_id.value → C TypeSpec
  std::unordered_map<uint32_t, TypeSpec> local_types;
  // local_id.value → true if this local is a VLA allocated dynamically at runtime
  std::unordered_map<uint32_t, bool> local_is_vla;
  // param_index → SSA name (e.g. "%p.x")
  std::unordered_map<uint32_t, std::string> param_slots;
  // Structured LIR blocks (replaces body_lines).
  std::vector<lir::LirBlock> lir_blocks;
  size_t current_block_idx = 0;
  lir::LirBlock& cur_block() { return lir_blocks[current_block_idx]; }
  // Hoisted alloca instructions (rendered before entry block body).
  std::vector<lir::LirInst> alloca_insts;
  // legacy per-block metadata (kept for compatibility; mostly unused now)
  std::unordered_map<uint32_t, BlockMeta> block_meta;
  // body_block -> continue branch target label
  std::unordered_map<uint32_t, std::string> continue_redirect;
  // user label name → LLVM label
  std::unordered_map<std::string, std::string> user_labels;
  // local_id.value / param_index / global_id.value → fn-ptr signature metadata.
  std::unordered_map<uint32_t, FnPtrSig> local_fn_ptr_sigs;
  std::unordered_map<uint32_t, FnPtrSig> param_fn_ptr_sigs;
  std::unordered_map<uint32_t, FnPtrSig> global_fn_ptr_sigs;
  // Per-function stacksave pointer for VLA scope rewinds.
  std::optional<std::string> vla_stack_save_ptr;
  // Block currently being emitted (for backward-goto detection).
  uint32_t current_block_id = 0;
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

class HirEmitter {
 public:
  // ── Specialization metadata for cross-TU serialization ──────────────────
  struct SpecEntry { std::string spec_key; std::string template_origin; std::string mangled_name; };

  explicit HirEmitter(const Module& m);
  std::string emit();

  /// Lower the HIR module to a LirModule without printing.
  /// This is the structured lowering result that can be consumed by lir::print_llvm()
  /// or any future backend.
  lir::LirModule lower_to_lir();

  /// Adopt/release the working LirModule.  The caller (hir_to_lir::lower)
  /// builds the module shell, then hands it to the emitter for per-item
  /// lowering, and finally takes it back for finalization.
  void adopt_module(lir::LirModule module);
  lir::LirModule release_module();

  /// Lower all globals whose indices are given.
  void lower_globals(const std::vector<size_t>& global_indices);

  /// Lower a single HIR function into the working LirModule.
  /// If signature_text is non-empty, it is used as the pre-built LLVM IR
  /// signature line; otherwise the emitter builds it internally (legacy path).
  /// If block_order is non-empty, it specifies the HIR block iteration order
  /// (entry first, rest in order); otherwise the emitter computes it internally.
  void lower_single_function(const hir::Function& fn,
                             const std::string& signature_text = {},
                             const std::vector<const hir::Block*>& block_order = {});

  // ── Post-lowering accessors for module-level finalization ──────────────
  // These expose accumulated state so that hir_to_lir::lower() can own
  // the decision of what goes into the final LirModule.
  bool needs_va_start()     const { return need_llvm_va_start_; }
  bool needs_va_end()       const { return need_llvm_va_end_; }
  bool needs_va_copy()      const { return need_llvm_va_copy_; }
  bool needs_memcpy()       const { return need_llvm_memcpy_; }
  bool needs_stacksave()    const { return need_llvm_stacksave_; }
  bool needs_stackrestore() const { return need_llvm_stackrestore_; }
  bool needs_abs()          const { return need_llvm_abs_; }

  const std::unordered_map<std::string, std::string>& extern_call_decls() const {
    return extern_call_decls_;
  }
  const std::vector<SpecEntry>& spec_entries() const { return spec_entries_; }


 private:
  const Module& mod_;
  lir::LirModule module_;  // Structured intermediate representation
  std::unordered_map<std::string, std::string> extern_call_decls_;  // name -> ret llvm type
  std::unordered_map<std::string, std::string> str_pool_;
  int str_idx_ = 0;
  bool need_llvm_va_start_ = false;
  bool need_llvm_va_end_ = false;
  bool need_llvm_va_copy_ = false;
  bool need_llvm_memcpy_ = false;
  bool need_llvm_stacksave_ = false;
  bool need_llvm_stackrestore_ = false;
  bool need_llvm_abs_ = false;
  mutable std::unordered_map<uint32_t, FnPtrSig> inferred_ret_fn_ptr_sigs_;
  mutable std::unordered_map<uint32_t, FnPtrSig> inferred_direct_fn_sigs_;

  std::vector<SpecEntry> spec_entries_;

  // ── Instruction helpers ───────────────────────────────────────────────────

  void emit_instr(FnCtx& ctx, const std::string& line);
  // Push a typed LIR instruction (non-terminator) into the current block.
  template<typename T>
  void emit_lir_op(FnCtx& ctx, T&& op) {
    // Skip dead code after a terminator has been placed in this block.
    if (!std::holds_alternative<lir::LirUnreachable>(ctx.cur_block().terminator))
      return;
    ctx.cur_block().insts.push_back(std::forward<T>(op));
    ctx.last_term = false;
  }
  void emit_lbl(FnCtx& ctx, const std::string& lbl);
  void emit_term(FnCtx& ctx, const std::string& line);
  std::string fresh_tmp(FnCtx& ctx);
  void record_extern_call_decl(const std::string& name, const std::string& ret_ty);
  std::string fresh_lbl(FnCtx& ctx, const std::string& pfx);
  static std::string block_lbl(BlockId id);


  // ── String constant pool ──────────────────────────────────────────────────
  std::string intern_str(const std::string& raw_bytes);

  // NOTE: preamble (type decls) generation moved to lir::build_type_decls().


  // ── Globals ───────────────────────────────────────────────────────────────
  static bool is_char_like(TypeBase b);
  static TypeSpec drop_one_array_dim(TypeSpec ts);
  static std::string bytes_from_string_literal(const StringLiteral& sl);

  // Decode a wide string literal (L"...") into wchar_t (i32) values with null terminator
  static std::vector<long long> decode_wide_string_values(const std::string& raw);
  static std::string escape_llvm_c_bytes(const std::string& raw_bytes);
  TypeSpec field_decl_type(const HirStructField& f) const;

  // Recursively constant-evaluate an expression to an integer (returns nullopt if not possible).
  std::optional<long long> try_const_eval_int(ExprId id);
  std::optional<double> try_const_eval_float(ExprId id);
  std::optional<std::pair<long long, long long>> try_const_eval_complex_int(ExprId id);
  std::optional<std::pair<double, double>> try_const_eval_complex_fp(ExprId id);
  std::string emit_const_int_like(long long value, const TypeSpec& expected_ts);
  std::string emit_const_scalar_expr(ExprId id, const TypeSpec& expected_ts);

 private:
  class ConstInitEmitter;

  std::vector<std::string> emit_const_struct_fields(const TypeSpec& ts,
                                                    const HirStructDef& sd,
                                                    const GlobalInit& init,
                                                    std::vector<TypeSpec>* out_field_types = nullptr);
  std::string emit_const_init(const TypeSpec& ts, const GlobalInit& init);
  const GlobalVar* select_global_object(const std::string& name) const;
  const GlobalVar* select_global_object(GlobalId id) const;
  void emit_global(const GlobalVar& gv);


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

  // Emit GEP chain for struct member access.
  // base_ptr: LLVM value (ptr to struct), tag: struct type tag, field_name: C field name.
  // Returns LLVM value (ptr to field), sets out_field_ts.
  std::string emit_member_gep(FnCtx& ctx, const std::string& base_ptr,
                               const std::string& tag, const std::string& field_name,
                               TypeSpec& out_field_ts,
                               BitfieldAccess* out_bf = nullptr);


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
  std::string emit_member_lval(FnCtx& ctx, const MemberExpr& m, TypeSpec& out_pts,
                                BitfieldAccess* out_bf = nullptr);

  // ── Rvalue emission ───────────────────────────────────────────────────────

  // Recursively resolve the C type of an expression from HIR structure.
  // The AST doesn't annotate types on NK_BINOP/NK_VAR nodes, so we infer.
  TypeSpec resolve_expr_type(FnCtx& ctx, ExprId id);
  TypeSpec resolve_expr_type(FnCtx& ctx, const Expr& e);
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

  void emit_stmt(FnCtx& ctx, const Stmt& stmt);
  void emit_stmt_impl(FnCtx& ctx, const LocalDecl& d);
  void emit_stmt_impl(FnCtx& ctx, const ExprStmt& s);
  void emit_stmt_impl(FnCtx& ctx, const InlineAsmStmt& s);
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

  // ── Function emission ─────────────────────────────────────────────────────

  // Returns set of param indices that are used as lvalues (modified or address-taken).
  std::unordered_set<uint32_t> find_modified_params(const Function& fn);
  static bool fn_has_vla_locals(const Function& fn);
  void hoist_allocas(FnCtx& ctx, const Function& fn);
  void emit_function(const Function& fn, const std::string& pre_sig = {},
                     const std::vector<const Block*>& block_order = {});
};



}  // namespace tinyc2ll::codegen::llvm_backend

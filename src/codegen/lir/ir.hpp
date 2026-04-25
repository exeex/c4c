#pragma once

// Public LIR package index.
//
// External codegen and backend users should include this header for the LIR
// data model plus printer/verifier entry points. `operands.hpp` and
// `types.hpp` remain top-level model subheaders because this index uses their
// typed wrappers throughout the instruction definitions.

// ── LIR: Low-level IR between HIR and LLVM text emission ────────────────────
//
// This header defines the minimal LIR data model for the HIR→LIR→Printer
// refactor (Milestone D).  The goal is to make the lowering boundary explicit
// and structured, rather than having HirEmitter build LLVM IR strings directly.
//
// Design principles:
//   1. LIR is target-neutral (no LLVM textual syntax here).
//   2. LIR values use explicit IDs; the printer maps them to %tN / @g names.
//   3. Every block has an explicit terminator.
//   4. C type info (TypeSpec) is preserved for later backends.
//   5. This first skeleton is intentionally narrow — only enough to represent
//      what hir_emitter.cpp already lowers.

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "operands.hpp"
#include "types.hpp"
#include "ast.hpp"  // TypeSpec, TypeBase
#include "../../shared/struct_name_table.hpp"
#include "../../shared/text_id_table.hpp"
#include "../../target_profile.hpp"

namespace c4c::codegen::lir {

// ── Value / Block / Slot IDs ─────────────────────────────────────────────────

struct LirValueId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirValueId invalid() { return LirValueId{kInvalid}; }
};

struct LirBlockId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirBlockId invalid() { return LirBlockId{kInvalid}; }
};

struct LirStackSlotId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirStackSlotId invalid() { return LirStackSlotId{kInvalid}; }
};

struct LirGlobalId {
  uint32_t value = 0;
  static constexpr uint32_t kInvalid = std::numeric_limits<uint32_t>::max();
  [[nodiscard]] constexpr bool valid() const { return value != kInvalid; }
  [[nodiscard]] static constexpr LirGlobalId invalid() { return LirGlobalId{kInvalid}; }
};

// ── Instructions (non-terminator) ────────────────────────────────────────────
//
// Each instruction produces zero or one result value (identified by LirValueId).
// These are intentionally minimal stubs for Stage 0; concrete fields will be
// populated in Stage 1 when string sinks are replaced.

struct LirConstInt {
  LirValueId result{};
  TypeSpec type{};
  long long value = 0;
};

struct LirConstFloat {
  LirValueId result{};
  TypeSpec type{};
  double value = 0.0;
};

struct LirLoad {
  LirValueId result{};
  TypeSpec type{};
  LirValueId ptr{};
};

struct LirStore {
  LirValueId ptr{};
  LirValueId val{};
  TypeSpec type{};
};

struct LirBinary {
  LirValueId result{};
  TypeSpec type{};
  int op = 0;  // will map to BinaryOp or LLVM-level opcode
  LirValueId lhs{};
  LirValueId rhs{};
};

struct LirCast {
  LirValueId result{};
  TypeSpec from_type{};
  TypeSpec to_type{};
  LirValueId operand{};
};

struct LirCmp {
  LirValueId result{};
  int predicate = 0;
  LirValueId lhs{};
  LirValueId rhs{};
};

struct LirCall {
  LirValueId result{};
  TypeSpec return_type{};
  std::string callee_name;
  LirValueId callee_ptr{};  // for indirect calls
  std::vector<LirValueId> args;
};

struct LirGep {
  LirValueId result{};
  TypeSpec base_type{};
  LirValueId base_ptr{};
  std::vector<LirValueId> indices;
};

struct LirSelect {
  LirValueId result{};
  TypeSpec type{};
  LirValueId cond{};
  LirValueId true_val{};
  LirValueId false_val{};
};

struct LirIntrinsic {
  LirValueId result{};
  std::string name;
  std::vector<LirValueId> args;
};

// ── Typed intrinsic operations (Stage 3) ────────────────────────────────────
// Typed intrinsic call operations.
// Operands use string SSA names (matching current emitter convention).

struct LirMemcpyOp {
  LirOperand dst;         // ptr operand
  LirOperand src;         // ptr operand
  LirOperand size;        // i64 operand
  bool is_volatile = false;
};

struct LirVaStartOp {
  LirOperand ap_ptr;      // ptr operand
};

struct LirVaEndOp {
  LirOperand ap_ptr;      // ptr operand
};

struct LirVaCopyOp {
  LirOperand dst_ptr;     // ptr operand
  LirOperand src_ptr;     // ptr operand
};

struct LirStackSaveOp {
  LirOperand result;      // SSA name for saved stack pointer
};

struct LirStackRestoreOp {
  LirOperand saved_ptr;   // SSA name of saved stack pointer
};

struct LirAbsOp {
  LirOperand result;      // SSA name for result
  LirOperand arg;         // SSA name for input
  LirTypeRef int_type;    // e.g. "i32" or "i64"
};

struct LirInlineAsm {
  LirValueId result{};
  std::string asm_string;
  std::string constraints;
  std::vector<LirValueId> operands;
};


struct LirIndirectBrOp {
  LirOperand addr;                    // SSA name of ptr to branch to
  std::vector<std::string> targets;   // label names (e.g. "%ulbl_foo")
};

struct LirExtractValueOp {
  LirOperand result;      // SSA name for result
  LirTypeRef agg_type;    // aggregate type string (e.g. "{ double, double }")
  LirOperand agg;         // SSA name of aggregate value
  int index = 0;          // field index
};

struct LirInsertValueOp {
  LirOperand result;      // SSA name for result
  LirTypeRef agg_type;    // aggregate type string
  LirOperand agg;         // SSA name of aggregate value (or "undef")
  LirTypeRef elem_type;   // element type string
  LirOperand elem;        // SSA name of element to insert
  int index = 0;          // field index
};

struct LirLoadOp {
  LirOperand result;      // SSA name for result (e.g. "%t5")
  LirTypeRef type_str;    // LLVM type string (e.g. "i32", "ptr")
  LirOperand ptr;         // SSA name of pointer operand
};

struct LirStoreOp {
  LirTypeRef type_str;    // LLVM type string (e.g. "i32", "ptr")
  LirOperand val;         // SSA name of value (or "zeroinitializer")
  LirOperand ptr;         // SSA name of pointer operand
};

struct LirMemsetOp {
  LirOperand dst;         // destination pointer
  LirOperand byte_val;    // i8-compatible byte value
  LirOperand size;        // i64 byte count
  bool is_volatile = false;
};

// Cast opcode for LirCastOp
enum class LirCastKind : uint8_t {
  Trunc,
  ZExt,
  SExt,
  FPTrunc,
  FPExt,
  FPToSI,
  FPToUI,
  SIToFP,
  UIToFP,
  PtrToInt,
  IntToPtr,
  Bitcast,
};

struct LirCastOp {
  LirOperand result;      // SSA name for result
  LirCastKind kind{};     // cast opcode
  LirTypeRef from_type;   // LLVM type string of source
  LirOperand operand;     // SSA name of source operand
  LirTypeRef to_type;     // LLVM type string of destination
};

struct LirGepOp {
  LirOperand result;          // SSA name for result
  LirTypeRef element_type;    // LLVM type string (e.g. "i8", "[5 x i8]", "%struct.foo")
  LirOperand ptr;             // SSA name of pointer operand
  bool inbounds = false;      // getelementptr inbounds
  std::vector<std::string> indices;  // each entry is "type value" (e.g. "i32 0", "i64 5")
};

// Typed call instruction.
// Covers both direct calls, indirect calls, and intrinsic calls.
struct LirCallOp {
  LirOperand result;               // SSA name for result (empty for void calls)
  LirTypeRef return_type;          // LLVM return type string (e.g. "i32", "void", "{ i32, i1 }")
  LirOperand callee;               // callee value (e.g. "@foo", "%ptr", "@llvm.fabs.f64")
  LinkNameId direct_callee_link_name_id = kInvalidLinkName;
  std::string callee_type_suffix;  // optional fn ptr type suffix (empty for direct calls)
  std::string args_str;            // pre-formatted argument string (e.g. "i32 %t1, i32 %t2")
  std::vector<LirTypeRef> arg_type_refs;  // Mirrors argument type fragments when available
};

// Typed binary arithmetic/bitwise/unary operation.
// Covers: add, sub, mul, sdiv, udiv, srem, urem, fadd, fsub, fmul, fdiv, frem,
//         and, or, xor, shl, lshr, ashr, fneg.
struct LirBinOp {
  LirOperand result;          // SSA name for result
  LirBinaryOpcodeRef opcode;  // LLVM opcode string (e.g. "add", "fadd", "xor", "fneg")
  LirTypeRef type_str;        // LLVM type string (e.g. "i32", "double", "<4 x i32>")
  LirOperand lhs;             // SSA name or literal for left operand
  LirOperand rhs;             // SSA name or literal for right operand (empty for unary fneg)
};

// Typed comparison operation (icmp/fcmp).
struct LirCmpOp {
  LirOperand result;              // SSA name for result
  bool is_float = false;          // true = fcmp, false = icmp
  LirCmpPredicateRef predicate;   // predicate string (e.g. "eq", "ne", "slt", "oeq", "uno")
  LirTypeRef type_str;            // LLVM type string of operands
  LirOperand lhs;                 // SSA name or literal for left operand
  LirOperand rhs;                 // SSA name or literal for right operand
};

// Typed PHI node.
struct LirPhiOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM type string
  std::vector<std::pair<std::string, std::string>> incoming;  // (value, label) pairs
};

// Typed select instruction.
struct LirSelectOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM type string for true/false operands
  LirOperand cond;        // SSA name of i1 condition
  LirOperand true_val;    // SSA name or literal for true branch
  LirOperand false_val;   // SSA name or literal for false branch
};

// Typed vector insert/extract/shuffle ops.
struct LirInsertElementOp {
  LirOperand result;      // SSA name for result
  LirTypeRef vec_type;    // vector type string (e.g. "<4 x i32>")
  LirOperand vec;         // SSA name of vector (or "poison")
  LirTypeRef elem_type;   // element type string
  LirOperand elem;        // SSA name of element value
  LirOperand index;       // index value (e.g. "0", "%idx")
};

struct LirExtractElementOp {
  LirOperand result;
  LirTypeRef vec_type;
  LirOperand vec;
  LirTypeRef index_type;
  LirOperand index;
};

struct LirShuffleVectorOp {
  LirOperand result;
  LirTypeRef vec_type;
  LirOperand vec1;
  LirOperand vec2;
  LirTypeRef mask_type;
  LirOperand mask;
};

// Typed va_arg instruction.
struct LirVaArgOp {
  LirOperand result;      // SSA name for result
  LirOperand ap_ptr;      // SSA name of va_list pointer
  LirTypeRef type_str;    // result type string
};

// Typed inline (non-hoisted) alloca instruction.
// Covers dynamic allocas (__builtin_alloca, alloca(), VLA) and temporary allocas
// emitted inline in the current block (va_list copy, variadic aggregate, vaarg struct).
struct LirAllocaOp {
  LirOperand result;      // SSA name for result
  LirTypeRef type_str;    // LLVM element type string (e.g. "i8", "i64", "%struct.foo")
  LirOperand count;       // dynamic count operand (empty for simple allocas)
  int align = 0;          // alignment in bytes (0 = unspecified)
};

// Typed inline asm call instruction.
struct LirInlineAsmOp {
  LirOperand result;          // SSA name for result (empty for void asm)
  LirTypeRef ret_type;        // LLVM return type string (e.g. "void", "i32")
  std::string asm_text;       // assembly template string
  std::string constraints;    // constraint string
  bool side_effects = false;  // sideeffect flag
  std::string args_str;       // pre-formatted argument string
};

using LirInst = std::variant<
    LirConstInt,
    LirConstFloat,
    LirLoad,
    LirStore,
    LirBinary,
    LirCast,
    LirCmp,
    LirCall,
    LirGep,
    LirSelect,
    LirIntrinsic,
    LirInlineAsm,
    LirMemcpyOp,
    LirVaStartOp,
    LirVaEndOp,
    LirVaCopyOp,
    LirStackSaveOp,
    LirStackRestoreOp,
    LirAbsOp,
    LirIndirectBrOp,
    LirExtractValueOp,
    LirInsertValueOp,
    LirLoadOp,
    LirStoreOp,
    LirMemsetOp,
    LirCastOp,
    LirGepOp,
    LirCallOp,
    LirBinOp,
    LirCmpOp,
    LirPhiOp,
    LirSelectOp,
    LirInsertElementOp,
    LirExtractElementOp,
    LirShuffleVectorOp,
    LirVaArgOp,
    LirAllocaOp,
    LirInlineAsmOp
>;

// ── Terminators ──────────────────────────────────────────────────────────────

struct LirBr {
  std::string target_label;  // e.g. "block_3"
};

struct LirCondBr {
  std::string cond_name;     // e.g. "%t5"
  std::string true_label;
  std::string false_label;
};

struct LirRet {
  std::optional<std::string> value_str;  // e.g. "%t7", "null", "0"
  std::string type_str;                  // e.g. "i32", "ptr", "void"
};

struct LirSwitch {
  std::string selector_name;   // e.g. "%t3"
  std::string selector_type;   // e.g. "i32"
  std::string default_label;
  std::vector<std::pair<long long, std::string>> cases;  // {value, label}
};

struct LirIndirectBr {
  LirValueId addr{};
  std::vector<LirBlockId> targets;
};

struct LirUnreachable {};

using LirTerminator = std::variant<
    LirBr,
    LirCondBr,
    LirRet,
    LirSwitch,
    LirIndirectBr,
    LirUnreachable
>;

// ── Block ────────────────────────────────────────────────────────────────────

struct LirBlock {
  LirBlockId id{};
  std::string label;  // display label (e.g. ".LBB0")
  std::vector<LirInst> insts;
  LirTerminator terminator{LirUnreachable{}};
};

// ── Stack object (alloca) ────────────────────────────────────────────────────

struct LirStackObject {
  LirStackSlotId id{};
  std::string name;   // e.g. "lv.x"
  TypeSpec type{};
  int align = 0;
  bool is_vla = false;
};

// ── String constant ──────────────────────────────────────────────────────────

struct LirStringConst {
  std::string pool_name;   // e.g. "@.str.0"
  std::string raw_bytes;
  int byte_length = 0;
};

// ── External declaration ─────────────────────────────────────────────────────

struct LirExternDecl {
  std::string name;
  std::string return_type_str;  // LLVM return type (temporary; will be TypeSpec later)
  LirTypeRef return_type;
  LinkNameId link_name_id = kInvalidLinkName;
};

// ── Function ─────────────────────────────────────────────────────────────────

struct LirFunction {
  std::string name;
  LinkNameId link_name_id = kInvalidLinkName;
  bool is_internal = false;
  bool can_elide_if_unreferenced = false;
  bool is_declaration = false;  // true for declarations (no body)
  TypeSpec return_type{};
  std::vector<std::pair<std::string, TypeSpec>> params;  // name, type
  std::optional<LirTypeRef> signature_return_type_ref;
  std::vector<LirTypeRef> signature_param_type_refs;
  std::vector<LirBlock> blocks;
  std::vector<LirStackObject> stack_objects;
  LirBlockId entry{};

  // Pre-formatted signature text (define/declare line + template comments).
  // Used by the printer; will be replaced with structured fields in Stage 2+.
  std::string signature_text;

  // Hoisted alloca instructions, rendered before entry block body instructions.
  std::vector<LirInst> alloca_insts;

  // ID generation for values and blocks within this function.
  uint32_t next_value_id = 0;
  uint32_t next_block_id = 0;

  [[nodiscard]] LirValueId alloc_value() { return LirValueId{next_value_id++}; }
  [[nodiscard]] LirBlockId alloc_block() { return LirBlockId{next_block_id++}; }
};

// ── Global variable ──────────────────────────────────────────────────────────

struct LirGlobal {
  LirGlobalId id{};
  std::string name;        // Unquoted C name (printer quotes it)
  LinkNameId link_name_id = kInvalidLinkName;
  TypeSpec type{};
  bool is_internal = false;
  bool is_const = false;

  // Structured fields — lowering fills these, printer assembles LLVM text.
  std::string linkage_vis;  // e.g. "internal ", "external ", "weak ", "extern_weak ", ""
  std::string qualifier;    // "constant " or "global "
  std::string llvm_type;    // Pre-computed LLVM type string
  std::optional<LirTypeRef> llvm_type_ref;  // Structured mirror when type identity is exact
  std::string init_text;    // LLVM constant init text (empty for extern decls)
  int align_bytes = 0;      // 0 = no align suffix
  bool is_extern_decl = false;  // extern: no init, type only
};

// ── Structured struct declaration mirror ────────────────────────────────────

struct LirStructField {
  LirTypeRef type;
};

struct LirStructDecl {
  StructNameId name_id = kInvalidStructName;
  std::vector<LirStructField> fields;
  bool is_packed = false;
  bool is_opaque = false;
};

// ── Specialization metadata entry ────────────────────────────────────────────

struct LirSpecEntry {
  std::string spec_key;
  std::string template_origin;
  std::string mangled_name;
  LinkNameId mangled_link_name_id = kInvalidLinkName;
};

// ── Module ───────────────────────────────────────────────────────────────────

struct LirModule {
  struct ExternDeclInfo {
    std::string name;
    std::string return_type_str;
    LirTypeRef return_type;
    LinkNameId link_name_id = kInvalidLinkName;
  };

  c4c::TargetProfile target_profile{};
  std::string data_layout;
  std::shared_ptr<c4c::TextTable> link_name_texts;
  c4c::LinkNameTable link_names;
  c4c::StructNameTable struct_names;

  std::vector<LirGlobal> globals;
  std::vector<LirFunction> functions;
  std::vector<LirStringConst> string_pool;
  std::vector<LirExternDecl> extern_decls;

  // Extern call declarations dedup first by semantic link-visible identity,
  // then by raw fallback spelling only when no LinkNameId exists yet.
  std::unordered_map<LinkNameId, ExternDeclInfo> extern_decl_link_name_map;
  std::unordered_map<std::string, ExternDeclInfo> extern_decl_name_map;

  [[nodiscard]] LirTypeRef extern_return_type_ref(const std::string& ret_ty) const {
    const StructNameId struct_name_id = struct_names.find(ret_ty);
    if (struct_name_id != kInvalidStructName && find_struct_decl(struct_name_id)) {
      return LirTypeRef::struct_type(ret_ty, struct_name_id);
    }
    return LirTypeRef(ret_ty);
  }

  void merge_extern_decl_info(ExternDeclInfo& info, const std::string& name,
                              const std::string& ret_ty,
                              const LirTypeRef& ret_type,
                              LinkNameId link_name_id) {
    if (info.name.empty()) info.name = name;
    if (info.return_type_str == "void" && ret_ty != "void") {
      info.return_type_str = ret_ty;
      info.return_type = ret_type;
    } else if (info.return_type_str == ret_ty &&
               !info.return_type.has_struct_name_id() &&
               ret_type.has_struct_name_id()) {
      info.return_type = ret_type;
    }
    if (info.link_name_id == kInvalidLinkName &&
        link_name_id != kInvalidLinkName) {
      info.link_name_id = link_name_id;
    }
  }

  /// Record an extern function call declaration. Prefer semantic dedup by
  /// LinkNameId and fall back to raw-name dedup only when no semantic id
  /// exists. Upgrades void returns to concrete types when a non-void call is
  /// seen.
  void record_extern_decl(const std::string& name, const std::string& ret_ty,
                          LinkNameId link_name_id = kInvalidLinkName) {
    const LirTypeRef ret_type = extern_return_type_ref(ret_ty);
    if (link_name_id != kInvalidLinkName) {
      auto it = extern_decl_link_name_map.find(link_name_id);
      if (it == extern_decl_link_name_map.end()) {
        auto by_name = extern_decl_name_map.find(name);
        if (by_name != extern_decl_name_map.end()) {
          ExternDeclInfo info = std::move(by_name->second);
          extern_decl_name_map.erase(by_name);
          merge_extern_decl_info(info, name, ret_ty, ret_type, link_name_id);
          extern_decl_link_name_map.emplace(link_name_id, std::move(info));
          return;
        }
        extern_decl_link_name_map.emplace(
            link_name_id, ExternDeclInfo{name, ret_ty, ret_type, link_name_id});
        return;
      }
      merge_extern_decl_info(it->second, name, ret_ty, ret_type, link_name_id);
      return;
    }

    auto it = extern_decl_name_map.find(name);
    if (it == extern_decl_name_map.end()) {
      extern_decl_name_map.emplace(
          name, ExternDeclInfo{name, ret_ty, ret_type, link_name_id});
      return;
    }
    merge_extern_decl_info(it->second, name, ret_ty, ret_type, link_name_id);
  }

  // Type declarations (struct definitions) needed by the output.
  // For now, these are stored as pre-formatted text lines (LLVM syntax).
  // Stage 1+ will replace with structured type defs.
  std::vector<std::string> type_decls;

  // Structured mirror for struct declarations. This is intentionally inert
  // until HIR lowering starts populating it and the printer is taught to use it.
  std::vector<LirStructDecl> struct_decls;
  std::unordered_map<StructNameId, std::size_t> struct_decl_index;

  LirStructDecl* find_struct_decl(StructNameId name_id) {
    const auto it = struct_decl_index.find(name_id);
    if (it == struct_decl_index.end()) return nullptr;
    return &struct_decls[it->second];
  }

  const LirStructDecl* find_struct_decl(StructNameId name_id) const {
    const auto it = struct_decl_index.find(name_id);
    if (it == struct_decl_index.end()) return nullptr;
    return &struct_decls[it->second];
  }

  LirStructDecl& record_struct_decl(LirStructDecl decl) {
    const StructNameId name_id = decl.name_id;
    if (name_id != kInvalidStructName) {
      LirStructDecl* existing = find_struct_decl(name_id);
      if (existing) {
        *existing = std::move(decl);
        return *existing;
      }
    }
    const std::size_t index = struct_decls.size();
    struct_decls.push_back(std::move(decl));
    if (name_id != kInvalidStructName) {
      struct_decl_index.emplace(name_id, index);
    }
    return struct_decls.back();
  }

  // Intrinsic requirement flags — set directly by lowering (emit_stmt).
  bool need_va_start = false;
  bool need_va_end = false;
  bool need_va_copy = false;
  bool need_memcpy = false;
  bool need_memset = false;
  bool need_stacksave = false;
  bool need_stackrestore = false;
  bool need_abs = false;
  bool need_ptrmask = false;
  bool prefer_semantic_va_ops = false;

  // Specialization metadata for cross-TU serialization.
  std::vector<LirSpecEntry> spec_entries;

  // ── String constant pool ──────────────────────────────────────────────────
  // Dedup map: raw bytes → pool name (e.g. "@.str0").
  std::unordered_map<std::string, std::string> str_pool_map;
  int str_pool_idx = 0;

  /// Intern a string literal.  Returns the pool name (e.g. "@.str3").
  /// Deduplicates by raw byte content and appends to string_pool on first use.
  std::string intern_str(const std::string& raw_bytes) {
    auto it = str_pool_map.find(raw_bytes);
    if (it != str_pool_map.end()) return it->second;
    const std::string name = "@.str" + std::to_string(str_pool_idx++);
    str_pool_map[raw_bytes] = name;
    const size_t len = raw_bytes.size() + 1;
    std::string esc;
    for (unsigned char c : raw_bytes) {
      if (c == '"')       { esc += "\\22"; }
      else if (c == '\\') { esc += "\\5C"; }
      else if (c == '\n') { esc += "\\0A"; }
      else if (c == '\r') { esc += "\\0D"; }
      else if (c == '\t') { esc += "\\09"; }
      else if (c < 32 || c >= 127) {
        char buf[8]; std::snprintf(buf, sizeof(buf), "\\%02X", c); esc += buf;
      } else {
        esc += static_cast<char>(c);
      }
    }
    LirStringConst sc;
    sc.pool_name = name;
    sc.raw_bytes = esc;
    sc.byte_length = static_cast<int>(len);
    string_pool.push_back(std::move(sc));
    return name;
  }
};

// LIR verification and rendering live on the public IR surface. Keep these
// declarations here so agents can use `ir.hpp` as the LIR index entry.
enum class LirVerifyErrorKind {
  Malformed,
};

class LirVerifyError : public std::invalid_argument {
 public:
  LirVerifyError(LirVerifyErrorKind kind, const std::string& message)
      : std::invalid_argument(message), kind_(kind) {}

  [[nodiscard]] LirVerifyErrorKind kind() const noexcept { return kind_; }

 private:
  LirVerifyErrorKind kind_;
};

const std::string& require_operand_kind(
    const LirOperand& operand,
    std::string_view field,
    std::initializer_list<LirOperandKind> allowed_kinds,
    bool allow_empty = false);

const std::string& require_type_ref(const LirTypeRef& type,
                                    std::string_view field,
                                    bool allow_void = false);

std::string_view render_binary_opcode(const LirBinaryOpcodeRef& opcode,
                                      std::string_view field);

std::string_view render_cmp_predicate(const LirCmpPredicateRef& predicate,
                                      std::string_view field);

std::string render_struct_decl_llvm(const LirModule& mod,
                                    const LirStructDecl& decl);

void verify_module(const LirModule& mod);

std::string print_llvm(const LirModule& mod);

}  // namespace c4c::codegen::lir

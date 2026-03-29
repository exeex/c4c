#pragma once

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

#include <cstdint>
#include <cstdio>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "ast.hpp"  // TypeSpec, TypeBase

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
  std::string dst;        // ptr operand
  std::string src;        // ptr operand
  std::string size;       // i64 operand
  bool is_volatile = false;
};

struct LirVaStartOp {
  std::string ap_ptr;     // ptr operand
};

struct LirVaEndOp {
  std::string ap_ptr;     // ptr operand
};

struct LirVaCopyOp {
  std::string dst_ptr;    // ptr operand
  std::string src_ptr;    // ptr operand
};

struct LirStackSaveOp {
  std::string result;     // SSA name for saved stack pointer
};

struct LirStackRestoreOp {
  std::string saved_ptr;  // SSA name of saved stack pointer
};

struct LirAbsOp {
  std::string result;     // SSA name for result
  std::string arg;        // SSA name for input
  std::string int_type;   // e.g. "i32" or "i64"
};

struct LirInlineAsm {
  LirValueId result{};
  std::string asm_string;
  std::string constraints;
  std::vector<LirValueId> operands;
};


struct LirIndirectBrOp {
  std::string addr;                   // SSA name of ptr to branch to
  std::vector<std::string> targets;   // label names (e.g. "%ulbl_foo")
};

struct LirExtractValueOp {
  std::string result;     // SSA name for result
  std::string agg_type;   // aggregate type string (e.g. "{ double, double }")
  std::string agg;        // SSA name of aggregate value
  int index = 0;          // field index
};

struct LirInsertValueOp {
  std::string result;     // SSA name for result
  std::string agg_type;   // aggregate type string
  std::string agg;        // SSA name of aggregate value (or "undef")
  std::string elem_type;  // element type string
  std::string elem;       // SSA name of element to insert
  int index = 0;          // field index
};

struct LirLoadOp {
  std::string result;     // SSA name for result (e.g. "%t5")
  std::string type_str;   // LLVM type string (e.g. "i32", "ptr")
  std::string ptr;        // SSA name of pointer operand
};

struct LirStoreOp {
  std::string type_str;   // LLVM type string (e.g. "i32", "ptr")
  std::string val;        // SSA name of value (or "zeroinitializer")
  std::string ptr;        // SSA name of pointer operand
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
  std::string result;     // SSA name for result
  LirCastKind kind{};     // cast opcode
  std::string from_type;  // LLVM type string of source
  std::string operand;    // SSA name of source operand
  std::string to_type;    // LLVM type string of destination
};

struct LirGepOp {
  std::string result;         // SSA name for result
  std::string element_type;   // LLVM type string (e.g. "i8", "[5 x i8]", "%struct.foo")
  std::string ptr;            // SSA name of pointer operand
  bool inbounds = false;      // getelementptr inbounds
  std::vector<std::string> indices;  // each entry is "type value" (e.g. "i32 0", "i64 5")
};

// Typed call instruction.
// Covers both direct calls, indirect calls, and intrinsic calls.
struct LirCallOp {
  std::string result;              // SSA name for result (empty for void calls)
  std::string return_type;         // LLVM return type string (e.g. "i32", "void", "{ i32, i1 }")
  std::string callee;              // callee value (e.g. "@foo", "%ptr", "@llvm.fabs.f64")
  std::string callee_type_suffix;  // optional fn ptr type suffix (empty for direct calls)
  std::string args_str;            // pre-formatted argument string (e.g. "i32 %t1, i32 %t2")
};

// Typed binary arithmetic/bitwise/unary operation.
// Covers: add, sub, mul, sdiv, udiv, srem, urem, fadd, fsub, fmul, fdiv, frem,
//         and, or, xor, shl, lshr, ashr, fneg.
struct LirBinOp {
  std::string result;     // SSA name for result
  std::string opcode;     // LLVM opcode string (e.g. "add", "fadd", "xor", "fneg")
  std::string type_str;   // LLVM type string (e.g. "i32", "double", "<4 x i32>")
  std::string lhs;        // SSA name or literal for left operand
  std::string rhs;        // SSA name or literal for right operand (empty for unary fneg)
};

// Typed comparison operation (icmp/fcmp).
struct LirCmpOp {
  std::string result;     // SSA name for result
  bool is_float = false;  // true = fcmp, false = icmp
  std::string predicate;  // predicate string (e.g. "eq", "ne", "slt", "oeq", "uno")
  std::string type_str;   // LLVM type string of operands
  std::string lhs;        // SSA name or literal for left operand
  std::string rhs;        // SSA name or literal for right operand
};

// Typed PHI node.
struct LirPhiOp {
  std::string result;     // SSA name for result
  std::string type_str;   // LLVM type string
  std::vector<std::pair<std::string, std::string>> incoming;  // (value, label) pairs
};

// Typed select instruction.
struct LirSelectOp {
  std::string result;     // SSA name for result
  std::string type_str;   // LLVM type string for true/false operands
  std::string cond;       // SSA name of i1 condition
  std::string true_val;   // SSA name or literal for true branch
  std::string false_val;  // SSA name or literal for false branch
};

// Typed vector insert/extract/shuffle ops.
struct LirInsertElementOp {
  std::string result;     // SSA name for result
  std::string vec_type;   // vector type string (e.g. "<4 x i32>")
  std::string vec;        // SSA name of vector (or "poison")
  std::string elem_type;  // element type string
  std::string elem;       // SSA name of element value
  std::string index;      // index value (e.g. "0", "%idx")
};

struct LirExtractElementOp {
  std::string result;
  std::string vec_type;
  std::string vec;
  std::string index_type;
  std::string index;
};

struct LirShuffleVectorOp {
  std::string result;
  std::string vec_type;
  std::string vec1;
  std::string vec2;
  std::string mask_type;
  std::string mask;
};

// Typed va_arg instruction.
struct LirVaArgOp {
  std::string result;     // SSA name for result
  std::string ap_ptr;     // SSA name of va_list pointer
  std::string type_str;   // result type string
};

// Typed inline (non-hoisted) alloca instruction.
// Covers dynamic allocas (__builtin_alloca, alloca(), VLA) and temporary allocas
// emitted inline in the current block (va_list copy, variadic aggregate, vaarg struct).
struct LirAllocaOp {
  std::string result;     // SSA name for result
  std::string type_str;   // LLVM element type string (e.g. "i8", "i64", "%struct.foo")
  std::string count;      // dynamic count operand (empty for simple allocas)
  int align = 0;          // alignment in bytes (0 = unspecified)
};

// Typed inline asm call instruction.
struct LirInlineAsmOp {
  std::string result;         // SSA name for result (empty for void asm)
  std::string ret_type;       // LLVM return type string (e.g. "void", "i32")
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
};

// ── Function ─────────────────────────────────────────────────────────────────

struct LirFunction {
  std::string name;
  bool is_internal = false;
  bool can_elide_if_unreferenced = false;
  bool is_declaration = false;  // true for declarations (no body)
  TypeSpec return_type{};
  std::vector<std::pair<std::string, TypeSpec>> params;  // name, type
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
  TypeSpec type{};
  bool is_internal = false;
  bool is_const = false;

  // Structured fields — lowering fills these, printer assembles LLVM text.
  std::string linkage_vis;  // e.g. "internal ", "external ", "weak ", "extern_weak ", ""
  std::string qualifier;    // "constant " or "global "
  std::string llvm_type;    // Pre-computed LLVM type string
  std::string init_text;    // LLVM constant init text (empty for extern decls)
  int align_bytes = 0;      // 0 = no align suffix
  bool is_extern_decl = false;  // extern: no init, type only
};

// ── Specialization metadata entry ────────────────────────────────────────────

struct LirSpecEntry {
  std::string spec_key;
  std::string template_origin;
  std::string mangled_name;
};

// ── Module ───────────────────────────────────────────────────────────────────

struct LirModule {
  std::string target_triple;
  std::string data_layout;

  std::vector<LirGlobal> globals;
  std::vector<LirFunction> functions;
  std::vector<LirStringConst> string_pool;
  std::vector<LirExternDecl> extern_decls;

  // Dedup map for extern call declarations (name → return type string).
  // Lowering writes here via record_extern_decl(); finalize converts to extern_decls vector.
  std::unordered_map<std::string, std::string> extern_decl_map;

  /// Record an extern function call declaration.  Deduplicates by name and
  /// upgrades void returns to concrete types when a non-void call is seen.
  void record_extern_decl(const std::string& name, const std::string& ret_ty) {
    auto it = extern_decl_map.find(name);
    if (it == extern_decl_map.end()) {
      extern_decl_map.emplace(name, ret_ty);
      return;
    }
    if (it->second == "void" && ret_ty != "void") it->second = ret_ty;
  }

  // Type declarations (struct definitions) needed by the output.
  // For now, these are stored as pre-formatted text lines (LLVM syntax).
  // Stage 1+ will replace with structured type defs.
  std::vector<std::string> type_decls;

  // Intrinsic requirement flags — set directly by lowering (emit_stmt).
  bool need_va_start = false;
  bool need_va_end = false;
  bool need_va_copy = false;
  bool need_memcpy = false;
  bool need_stacksave = false;
  bool need_stackrestore = false;
  bool need_abs = false;
  bool need_ptrmask = false;

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

}  // namespace c4c::codegen::lir

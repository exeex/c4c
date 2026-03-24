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
#include <limits>
#include <optional>
#include <string>
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

struct LirAlloca {
  LirValueId result{};
  TypeSpec type{};
  std::optional<LirValueId> count;  // for VLA
  std::string name;                 // SSA slot name (e.g. "%lv.foo")
  int align = 0;                    // alignment in bytes (0 = unspecified)
};

struct LirIntrinsic {
  LirValueId result{};
  std::string name;
  std::vector<LirValueId> args;
};

// ── Typed intrinsic operations (Stage 3) ────────────────────────────────────
// These replace LirRawLine for well-known intrinsic calls.
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

struct LirBitfieldExtract {
  std::string result;       // final promoted SSA name
  std::string unit_ptr;     // ptr to storage unit
  int bit_width = 0;
  int bit_offset = 0;
  int storage_unit_bits = 0;
  bool is_signed = false;
  int promoted_bits = 0;    // target integer width after C promotion
};

struct LirBitfieldInsert {
  std::string unit_ptr;     // ptr to storage unit
  std::string new_val;      // already coerced to storage unit type
  std::string scratch;      // base name for intermediate SSA values
  int bit_width = 0;
  int bit_offset = 0;
  int storage_unit_bits = 0;
};

struct LirHoistedStore {
  std::string ptr;       // destination slot SSA name (e.g. "%lv.param.foo")
  std::string val;       // source SSA name (e.g. "%p.foo"); empty when zeroinit=true
  TypeSpec type{};       // type of stored value
  bool zeroinit = false; // if true, store zeroinitializer instead of val
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

// Catch-all for instructions not yet migrated to typed LIR ops.
// Contains the raw LLVM IR line produced by the legacy emitter.
// This allows incremental migration: Stage 1+ will shrink usage of this type.
struct LirRawLine {
  std::string line;
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
    LirAlloca,
    LirIntrinsic,
    LirInlineAsm,
    LirBitfieldExtract,
    LirBitfieldInsert,
    LirMemcpyOp,
    LirVaStartOp,
    LirVaEndOp,
    LirVaCopyOp,
    LirStackSaveOp,
    LirStackRestoreOp,
    LirAbsOp,
    LirHoistedStore,
    LirIndirectBrOp,
    LirExtractValueOp,
    LirInsertValueOp,
    LirLoadOp,
    LirStoreOp,
    LirCastOp,
    LirGepOp,
    LirRawLine
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

// Catch-all for terminators not yet migrated to typed LIR variants.
// Contains the raw LLVM IR line produced by the legacy emitter.
struct LirRawTerminator {
  std::string line;
};

using LirTerminator = std::variant<
    LirBr,
    LirCondBr,
    LirRet,
    LirSwitch,
    LirIndirectBr,
    LirUnreachable,
    LirRawTerminator
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
  std::string name;
  TypeSpec type{};
  bool is_internal = false;
  bool is_const = false;
  std::string init_text;  // LLVM constant init text (temporary; will be structured later)
  // Pre-formatted complete LLVM IR line for this global (temporary; Stage 2+ will
  // replace with structured emission from the fields above).
  std::string raw_line;
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

  // Type declarations (struct definitions) needed by the output.
  // For now, these are stored as pre-formatted text lines (LLVM syntax).
  // Stage 1+ will replace with structured type defs.
  std::vector<std::string> type_decls;

  // Intrinsic requirement flags (mirrors HirEmitter flags).
  bool need_va_start = false;
  bool need_va_end = false;
  bool need_va_copy = false;
  bool need_memcpy = false;
  bool need_stacksave = false;
  bool need_stackrestore = false;
  bool need_abs = false;

  // Specialization metadata for cross-TU serialization.
  std::vector<LirSpecEntry> spec_entries;
};

}  // namespace c4c::codegen::lir

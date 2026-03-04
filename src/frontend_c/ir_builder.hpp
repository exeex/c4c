#pragma once

// Phase-1 LLVM IR text emitter for the tiny-c2ll C++ frontend.
//
// Takes the NK_PROGRAM node from Parser::parse() and emits valid LLVM IR text.
//
// Design:
//  - String-based ("phase-1") backend: output is a std::string of LLVM IR lines.
//  - Two-list pattern per function: alloca_lines_ (hoisted to entry block) +
//    body_lines_ (sequential code).  Concatenated at the end of emit_function().
//  - Global state (struct_defs_, func_sigs_, enum_consts_) is collected in a
//    pre-pass over the NK_PROGRAM children before any function is emitted.
//  - User goto labels are pre-scanned per function so forward gotos work.
//
// Pure-C backport note:
//  - Replace class → struct + free functions.
//  - Replace std::unordered_map with hash-table or sorted char*[] structs.
//  - Replace std::string with a StrBuf (dynamic char array).
//  - Replace std::vector<std::string> with StrVec.

#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"

namespace tinyc2ll::frontend_cxx {

class IRBuilder {
 public:
  // ── Nested structs ──────────────────────────────────────────────────────────

  struct StructInfo {
    std::vector<std::string> field_names;
    std::vector<TypeSpec>    field_types;
    // -1 = not an array field; >= 0 = array field size
    std::vector<long long>   field_array_sizes;
    // true = synthetic anonymous sub-struct/union field (for field promotion)
    std::vector<bool>        field_is_anon;
    bool                     is_union;
  };

  struct FuncSig {
    TypeSpec              ret_type;
    std::vector<TypeSpec> param_types;
    bool                  variadic;
  };

  struct SwitchInfo {
    std::unordered_map<long long, std::string> case_labels;
    std::string default_label;
    std::string end_label;
  };

  // ── Global state (persists across functions) ────────────────────────────────

  std::unordered_map<std::string, StructInfo> struct_defs_;
  std::unordered_map<std::string, FuncSig>    func_sigs_;
  std::unordered_map<std::string, TypeSpec>   global_types_;
  std::unordered_map<std::string, bool>       global_is_extern_;
  std::unordered_map<std::string, long long>  enum_consts_;
  // Raw string content → "@.strN" global name
  std::unordered_map<std::string, std::string> string_consts_;
  int string_idx_;

  // Functions called but not declared: name → "declare ..." line.
  // Emitted before function bodies in the final output.
  std::unordered_map<std::string, std::string> auto_declared_fns_;

  // Functions explicitly declared (no body) — used to prevent duplicate declares.
  std::unordered_set<std::string> declared_fns_;
  // Functions that have a body (definition) in this module — declarations for
  // these are suppressed since LLVM accepts define without a preceding declare.
  std::unordered_set<std::string> has_body_;

  // Emitted preamble lines (struct type defs, extern decls, global var defs,
  // string constant globals).  Emitted before function bodies.
  std::vector<std::string> preamble_;

  // Collected function body texts (one per function definition).
  std::vector<std::string> fn_sections_;

  // ── Per-function state (reset at the start of each emit_function call) ─────

  // Alloca instructions hoisted to the function entry block.
  std::vector<std::string> alloca_lines_;
  // Sequential IR instructions for the function body.
  std::vector<std::string> body_lines_;

  // varname → C TypeSpec (for load/store type annotation)
  std::unordered_map<std::string, TypeSpec> local_types_;
  // varname → LLVM slot name (e.g. "%x")
  std::unordered_map<std::string, std::string> local_slots_;

  int  tmp_idx_;          // counter for %tN temporaries
  int  label_idx_;        // counter for labeled basic blocks
  bool last_was_terminator_;  // suppress dead-code after ret/br

  TypeSpec    current_fn_ret_;      // return type of the current function
  std::string current_fn_ret_llty_; // LLVM type string for the return

  // (cond_label, end_label) for innermost loop (break/continue targets)
  std::vector<std::pair<std::string, std::string>> loop_stack_;
  // break_label for innermost breakable scope (loop or switch)
  std::vector<std::string> break_stack_;

  // C label name → LLVM basic-block label (pre-scanned before function body)
  std::unordered_map<std::string, std::string> user_labels_;

  // Switch stack for case/default dispatch
  std::vector<SwitchInfo> switch_stack_;

  // ── Constructor ─────────────────────────────────────────────────────────────
  IRBuilder();

  // ── Entry point ─────────────────────────────────────────────────────────────

  // Emit LLVM IR for the entire program.  Returns the complete IR as a string.
  std::string emit_program(Node* prog);

  // ── Type helpers ────────────────────────────────────────────────────────────

  std::string llvm_ty(const TypeSpec& ts);
  std::string llvm_ty_base(TypeBase base);
  int sizeof_ty(const TypeSpec& ts);

  // ── Expression type inference ───────────────────────────────────────────────

  // Infer the C TypeSpec of an expression (used for load/store annotations).
  TypeSpec expr_type(Node* n);
  std::string expr_llty(Node* n) { return llvm_ty(expr_type(n)); }

  // ── Temp / label allocation ─────────────────────────────────────────────────
  std::string fresh_tmp()   { return "%t" + std::to_string(tmp_idx_++); }
  std::string fresh_label(const std::string& pfx) {
    return pfx + std::to_string(label_idx_++);
  }

  // ── Code emission ───────────────────────────────────────────────────────────

  // Emit a regular instruction (prefixed with two spaces).
  void emit(const std::string& line);
  // Emit a basic-block label line ("label:").
  void emit_label(const std::string& label);
  // Emit a terminator (br/ret); suppressed if already terminated.
  void emit_terminator(const std::string& line);

  // ── Hoisting ────────────────────────────────────────────────────────────────

  // Walk a subtree, find all NK_DECL local variable nodes, and emit their
  // alloca instructions into alloca_lines_.
  void hoist_allocas(Node* node);

  // Pre-scan a function body for all NK_LABEL nodes and populate user_labels_.
  void prescan_labels(Node* node);

  // ── String helpers ──────────────────────────────────────────────────────────

  // Intern a C string literal and return its global constant name ("@.strN").
  std::string get_string_global(const std::string& content);

  // Convert a double to LLVM IR 64-bit hex fp constant ("0xXXXXXXXXXXXXXXXX").
  static std::string fp_to_hex(double v);

  // ── Anonymous field helpers ──────────────────────────────────────────────────

  // Recursively look up a field name in a struct (searching anonymous sub-fields).
  // Returns the field index if found, or -1 if not found.
  // If found in a nested anonymous field, anon_path is populated with the chain
  // of (anon_field_idx, sub_tag) pairs from outer to inner.
  struct FieldPath {
    // Sequence of (field_index_in_parent, StructInfo*) steps before the final field.
    // Empty = direct field.
    std::vector<int>         anon_indices; // indices of anonymous sub-struct fields
    int                      final_idx;    // field index in the innermost struct
    std::string              final_tag;    // tag of innermost struct
  };
  // Returns true if found; fills path.
  bool find_field(const std::string& tag, const char* fname, FieldPath& path);
  // Emit a GEP chain for a FieldPath and return the resulting ptr.
  std::string emit_field_gep(const std::string& struct_ptr,
                              const std::string& tag, const FieldPath& path);
  // Get the TypeSpec of a field found via find_field.
  TypeSpec field_type_from_path(const std::string& tag, const FieldPath& path);

  // ── Codegen ─────────────────────────────────────────────────────────────────

  // Returns the LLVM value name for the expression result.
  std::string codegen_expr(Node* n);
  // Returns the LLVM ptr name for the lvalue.
  std::string codegen_lval(Node* n);

  // Convert value v (of C type ts) to i1 (for conditions).
  std::string to_bool(const std::string& val, const TypeSpec& ts);

  // Coerce value v from one LLVM type to another.
  std::string coerce(const std::string& val, const TypeSpec& from_ts,
                     const std::string& to_llty);

  // Emit one statement.
  void emit_stmt(Node* n);

  // ── Top-level emitters ──────────────────────────────────────────────────────

  void emit_struct_def(Node* sd);   // collect into struct_defs_ + preamble
  void emit_global(Node* gv);       // emit global variable declaration
  void emit_function(Node* fn);     // emit full function definition or declaration

  // ── Global initializer helpers ──────────────────────────────────────────────

  // Emit an LLVM constant expression for a global initializer.
  // Returns the value portion (e.g. "42", "[3 x i32] [i32 1, i32 2, i32 3]").
  std::string global_const(TypeSpec ts, Node* init);
};

}  // namespace tinyc2ll::frontend_cxx

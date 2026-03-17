#pragma once

// Recursive-descent parser for the tiny-c2ll C++ frontend.
//
// Entry point: Parser::parse() returns a NK_PROGRAM Node* containing all
// top-level declarations (functions, global variables, struct/enum defs).
//
// Design:
//  - All AST nodes and strings are allocated from the supplied Arena.
//  - typedef names are tracked in a std::set<std::string> for disambiguation.
//  - Anonymous struct/union/enum definitions are collected and prepended to
//    the program item list so the IR builder sees types before use sites.
//
// Pure-C backport note: replace class with struct + free functions.
// Replace std::set with a sorted char*[] searched by strcmp.

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "source_profile.hpp"
#include "token.hpp"

namespace c4c {

class Parser {
 public:
  // All members public (required by project coding constraints).
  explicit Parser(std::vector<Token> tokens, Arena& arena,
                  SourceProfile source_profile = SourceProfile::C);

  // Parse the entire token stream and return a NK_PROGRAM node.
  Node* parse();

  // ── public state ──────────────────────────────────────────────────────────
  std::vector<Token> tokens_;
  int                pos_;
  Arena&             arena_;
  SourceProfile      source_profile_;
  std::set<std::string> typedefs_;  // known typedef names
  // Typedef names declared in the current translation unit (not pre-seeded).
  std::set<std::string> user_typedefs_;
  // Maps typedef name → resolved TypeSpec (populated when registering typedefs)
  // so subsequent uses of the typedef name resolve to the actual struct/base type.
  std::unordered_map<std::string, TypeSpec> typedef_types_;
  // Phase C: fn_ptr parameter info for typedef'd function pointer types.
  // Stores (fn_ptr_params, n_fn_ptr_params, fn_ptr_variadic) for typedefs
  // whose TypeSpec has is_fn_ptr=true, so the info can be propagated to
  // declaration nodes that use the typedef.
  struct FnPtrTypedefInfo {
    Node** params = nullptr;
    int n_params = 0;
    bool variadic = false;
  };
  std::unordered_map<std::string, FnPtrTypedefInfo> typedef_fn_ptr_info_;
  // Last resolved typedef name from parse_base_type() (for fn_ptr propagation).
  std::string last_resolved_typedef_;
  std::vector<Node*>    struct_defs_;  // collected struct/enum defs (prepended)
  int                anon_counter_;  // counter for anonymous tag names
  // Struct/union tags that already have a full definition (with body).
  // Used to detect block-scoped redefinitions and generate unique tags.
  std::set<std::string> defined_struct_tags_;
  // Enum constants: name → value (populated as enums are parsed).
  // Used to evaluate enum initializers that reference previously-defined constants.
  std::unordered_map<std::string, long long> enum_consts_;
  // Variable name → TypeSpec (populated as variables are declared).
  // Used to resolve typeof(variable) in type expressions.
  std::unordered_map<std::string, TypeSpec> var_types_;
  // Struct/union tag → NK_STRUCT_DEF node (populated when parsing struct bodies).
  // Used by eval_const_int to compute __builtin_offsetof at parse time.
  std::unordered_map<std::string, Node*> struct_tag_def_map_;
  // True if parse() encountered any recoverable parse error.
  bool had_error_;
  // True while parsing a file-scope declaration in parse_top_level().
  bool parsing_top_level_context_;
  // True while parsing an explicit template specialization (template<>).
  bool parsing_explicit_specialization_ = false;
  // Template struct definitions: maps struct tag → NK_STRUCT_DEF node with
  // n_template_params > 0.  Used to instantiate template structs at usage sites.
  std::unordered_map<std::string, Node*> template_struct_defs_;
  // Already-instantiated template struct mangled names (avoid double instantiation).
  std::set<std::string> instantiated_template_structs_;
  // Last enum definition node produced by parse_base_type(), if any.
  // Used so declaration-only enum statements (`enum { ... };`) can be retained.
  Node* last_enum_def_;

  // #pragma pack state: current packing alignment (0 = default/no packing).
  int pack_alignment_ = 0;
  std::vector<int> pack_stack_;  // for #pragma pack(push/pop)

  // #pragma GCC visibility state: 0=default, 1=hidden, 2=protected.
  uint8_t visibility_ = 0;
  std::vector<uint8_t> visibility_stack_;  // for push/pop

  // ── pragma helpers ────────────────────────────────────────────────────────
  void handle_pragma_pack(const std::string& args);
  void handle_pragma_gcc_visibility(const std::string& args);

  // ── token cursor helpers ──────────────────────────────────────────────────
  const Token& cur() const;              // current token
  const Token& peek(int offset = 0) const; // peek at pos_+offset (0=current)
  const Token& consume();                // consume and return current token
  bool at_end() const;
  bool check(TokenKind k) const;         // is current token kind k?
  bool check2(TokenKind k) const;        // is next token kind k?
  bool match(TokenKind k);               // consume if check(k)
  void expect(TokenKind k);              // consume or throw
  void skip_until(TokenKind k);          // skip tokens until k (consume k)

  // ── type parsing helpers ──────────────────────────────────────────────────
  bool is_type_start() const;            // can current token start a type?
  bool is_typedef_name(const std::string& s) const;
  bool is_cpp_mode() const {
    return source_profile_ == SourceProfile::CppSubset ||
           source_profile_ == SourceProfile::C4;
  }

  // Parse a complete type specifier (base type + qualifiers).
  // ptr_level and array_size are NOT parsed here (those are in the declarator).
  TypeSpec parse_base_type();

  // Parse declarator suffix: pointer levels, array, etc.
  // Modifies ts in place.  Returns the declared name (into out_name).
  void parse_declarator(TypeSpec& ts, const char** out_name,
                        Node*** out_fn_ptr_params = nullptr,
                        int* out_n_fn_ptr_params = nullptr,
                        bool* out_fn_ptr_variadic = nullptr,
                        Node*** out_ret_fn_ptr_params = nullptr,
                        int* out_n_ret_fn_ptr_params = nullptr,
                        bool* out_ret_fn_ptr_variadic = nullptr);

  // Parse a full type-name (type + declarator with no name): e.g. for sizeof.
  TypeSpec parse_type_name();

  // Skip __attribute__((...)) sequences (zero or more).
  void skip_attributes();

  // Consume __attribute__((...)) sequences, recording supported attributes
  // into the type when provided (currently aligned/vector_size).
  void parse_attributes(TypeSpec* ts);

  // Skip __asm__("...") or asm("...") sequences.
  void skip_asm();

  // Skip a balanced paren group (consuming the closing paren).
  void skip_paren_group();

  // Skip a balanced brace group (consuming the closing brace).
  void skip_brace_group();

  // Parse struct/union body { fields... } or just a tag reference.
  // Returns a NK_STRUCT_DEF node; appends to struct_defs_ if new.
  Node* parse_struct_or_union(bool is_union);

  // Parse enum body { variants... } or just a tag reference.
  // Returns a NK_ENUM_DEF node; appends to struct_defs_ if new.
  Node* parse_enum();

  // ── parameter / field parsing ──────────────────────────────────────────────
  // Parse a single parameter declaration.  Returns NK_DECL node.
  Node* parse_param();

  // ── expression parsing (Pratt) ─────────────────────────────────────────────
  Node* parse_expr();            // comma-level
  Node* parse_assign_expr();     // assignment / ternary
  Node* parse_ternary();         // ternary ?:
  Node* parse_binary(int min_prec); // binary ops (precedence climbing)
  Node* parse_unary();           // prefix unary
  Node* parse_postfix(Node* base); // postfix ops (left-recursive)
  Node* parse_primary();         // primary expression

  // Operator precedence helper.
  static int bin_prec(TokenKind k);

  // ── statement parsing ──────────────────────────────────────────────────────
  Node* parse_stmt();
  Node* parse_block();           // { ... }

  // ── initializer parsing ───────────────────────────────────────────────────
  Node* parse_initializer();     // expr or { list }
  Node* parse_init_list();       // { item, item, ... }

  // ── declaration parsing ───────────────────────────────────────────────────
  // Parse a local declaration (inside function body).  May return a NK_BLOCK
  // if there are multiple declarators.
  Node* parse_local_decl();

  // ── top-level parsing ─────────────────────────────────────────────────────
  // Parse one top-level item. Returns nullptr on EOF.
  Node* parse_top_level();

  // ── node constructors ──────────────────────────────────────────────────────
  Node* make_node(NodeKind k, int line);
  Node* make_int_lit(long long v, int line);
  Node* make_float_lit(double v, const char* raw, int line);
  Node* make_str_lit(const char* raw, int line);
  Node* make_var(const char* name, int line);
  Node* make_binop(const char* op, Node* l, Node* r, int line);
  Node* make_unary(const char* op, Node* operand, int line);
  Node* make_assign(const char* op, Node* lhs, Node* rhs, int line);
  Node* make_block(Node** stmts, int n, int line);
};

}  // namespace c4c

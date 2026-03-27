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
  struct NamespaceContext {
    int id = 0;
    int parent_id = -1;
    bool is_anonymous = false;
    const char* display_name = nullptr;
    const char* canonical_name = nullptr;
  };

  struct QualifiedNameRef {
    bool is_global_qualified = false;
    std::vector<std::string> qualifier_segments;
    std::string base_name;
  };

  struct TemplateArgParseResult {
    bool is_value = false;
    TypeSpec type{};
    long long value = 0;
    const char* nttp_name = nullptr;
    Node* expr = nullptr;
  };

  enum class TypenameTemplateParamKind {
    TypeParameter,
    TypedNonTypeParameter,
  };

  // Template-scope stack: tracks active template parameter visibility.
  // Each frame records the parameters introduced by one template<...> clause
  // and the semantic context (enclosing class, member template, or free function).
  enum class TemplateScopeKind {
    EnclosingClass,      // template<...> struct/class body
    MemberTemplate,      // template<...> member inside a class template
    FreeFunctionTemplate // template<...> free function
  };

  struct TemplateScopeParam {
    const char* name = nullptr;
    bool is_nttp = false;
  };

  struct TemplateScopeFrame {
    TemplateScopeKind kind = TemplateScopeKind::FreeFunctionTemplate;
    std::vector<TemplateScopeParam> params;
    std::string owner_struct_tag;  // set when kind == EnclosingClass
  };

  // All members public (required by project coding constraints).
  explicit Parser(std::vector<Token> tokens, Arena& arena,
                  SourceProfile source_profile = SourceProfile::C,
                  const std::string& source_file = "<input>");

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
  // Global const/constexpr integer bindings visible to parser-time constant folding.
  std::unordered_map<std::string, long long> const_int_bindings_;
  // Variable name → TypeSpec (populated as variables are declared).
  // Used to resolve typeof(variable) in type expressions.
  std::unordered_map<std::string, TypeSpec> var_types_;
  // Qualified function names (populated as functions are declared/defined).
  // Used by lookup_value_in_context for namespace-aware function lookup.
  std::set<std::string> known_fn_names_;
  // Struct/union tag → NK_STRUCT_DEF node (populated when parsing struct bodies).
  // Used by eval_const_int to compute __builtin_offsetof at parse time.
  std::unordered_map<std::string, Node*> struct_tag_def_map_;
  // Source file name for diagnostic messages.
  std::string source_file_;
  // True if parse() encountered any recoverable parse error.
  bool had_error_;
  int parse_error_count_ = 0;
  int max_parse_errors_ = 20;
  int max_no_progress_steps_ = 8;
  // True while parsing a file-scope declaration in parse_top_level().
  bool parsing_top_level_context_;
  // True while parsing an explicit template specialization (template<>).
  bool parsing_explicit_specialization_ = false;
  // Template struct definitions: maps struct tag → NK_STRUCT_DEF node with
  // n_template_params > 0.  Used to instantiate template structs at usage sites.
  std::unordered_map<std::string, Node*> template_struct_defs_;
  // Template struct specialization patterns keyed by primary template name.
  std::unordered_map<std::string, std::vector<Node*>> template_struct_specializations_;
  // Already-instantiated template structs keyed by semantic identity
  // (primary family + canonical args), not by mangled print name.
  std::set<std::string> instantiated_template_struct_keys_;
  // Deferred NTTP default expression tokens, keyed by "template_tag:param_idx".
  // Used for complex defaults like `arithmetic<T>::value` that can only be
  // evaluated once template type arguments are known.
  std::unordered_map<std::string, std::vector<Token>> nttp_default_expr_tokens_;
  // Alias template metadata: template<...> using Name = AliasedType;
  // Stores the alias template's parameter info and the aliased TypeSpec so
  // that applying Name<args> can rebuild the aliased template struct with
  // substituted args instead of losing the alias template param mapping.
  struct AliasTemplateInfo {
    std::vector<const char*> param_names;
    std::vector<bool> param_is_nttp;
    TypeSpec aliased_type;  // TypeSpec from parse_type_name() (has tpl_struct_origin/arg_refs)
  };
  std::unordered_map<std::string, AliasTemplateInfo> alias_template_info_;
  // Set by the using-alias handler to let the template wrapper detect that
  // a using type alias was defined during `parse_top_level()`.
  std::string last_using_alias_name_;
  // Last enum definition node produced by parse_base_type(), if any.
  // Used so declaration-only enum statements (`enum { ... };`) can be retained.
  Node* last_enum_def_;

  // Struct member typedef scoped names: "StructTag::TypeName" → TypeSpec.
  // Populated when parsing typedef inside struct bodies.
  std::unordered_map<std::string, TypeSpec> struct_typedefs_;
  // Tag of the struct currently being parsed (empty if not in struct body).
  std::string current_struct_tag_;
  // Template-scope stack: tracks active template parameter visibility.
  std::vector<TemplateScopeFrame> template_scope_stack_;
  // Transitional flattened path kept only as a compatibility bridge.
  std::string current_namespace_;
  std::vector<NamespaceContext> namespace_contexts_;
  std::vector<int> namespace_stack_;
  std::unordered_map<std::string, int> named_namespace_contexts_;
  std::unordered_map<int, std::vector<int>> anonymous_namespace_children_;
  // Unqualified visible aliases introduced by using-declarations per namespace context.
  std::unordered_map<int, std::unordered_map<std::string, std::string>> using_value_aliases_;
  std::unordered_map<int, std::vector<int>> using_namespace_contexts_;

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
  const char* diag_file_at(int token_index) const;
  // Template angle-bracket helpers: parser-owned handling for >-prefixed tokens.
  bool check_template_close() const;     // is current > / >> / >= / >>=?
  bool parse_greater_than_in_template_list(bool consume_last_token = true);
  bool match_template_close();           // consume one template-close >
  void expect_template_close();          // match_template_close or throw
  void skip_until(TokenKind k);          // skip tokens until k (consume k)

  // ── type parsing helpers ──────────────────────────────────────────────────
  bool is_type_start() const;            // can current token start a type?
  bool is_typedef_name(const std::string& s) const;
  bool is_cpp_mode() const {
    return source_profile_ == SourceProfile::CppSubset ||
           source_profile_ == SourceProfile::C4;
  }
  void refresh_current_namespace_bridge();
  int current_namespace_context_id() const;
  int ensure_named_namespace_context(int parent_id, const std::string& name);
  int create_anonymous_namespace_context(int parent_id);
  void push_namespace_context(int context_id);
  void pop_namespace_context();
  std::string canonical_name_in_context(int context_id, const std::string& name) const;
  int resolve_namespace_context(const QualifiedNameRef& name) const;
  int resolve_namespace_name(const QualifiedNameRef& name) const;
  bool lookup_value_in_context(int context_id, const std::string& name,
                               std::string* resolved) const;
  bool lookup_type_in_context(int context_id, const std::string& name,
                              std::string* resolved) const;
  std::string qualify_name(const std::string& name) const;
  const char* qualify_name_arena(const char* name);
  std::string resolve_visible_value_name(const std::string& name) const;
  std::string resolve_visible_type_name(const std::string& name) const;
  bool peek_qualified_name(QualifiedNameRef* out, bool allow_global = true) const;
  QualifiedNameRef parse_qualified_name(bool allow_global = true);
  void apply_qualified_name(Node* node, const QualifiedNameRef& qn,
                            const char* resolved_name = nullptr);
  void apply_decl_namespace(Node* node, int context_id, const char* unqualified_name);

  // Parse a complete type specifier (base type + qualifiers).
  // ptr_level and array_size are NOT parsed here (those are in the declarator).
  TypeSpec parse_base_type();

  // Parse declarator suffix: pointer levels, array, etc.
  // Modifies ts in place.  Returns the declared name (into out_name).
  void parse_declarator(TypeSpec& ts, const char** out_name,
                        Node*** out_fn_ptr_params = nullptr,
                        int* out_n_fn_ptr_params = nullptr,
                        bool* out_fn_ptr_variadic = nullptr,
                        bool* out_is_parameter_pack = nullptr,
                        Node*** out_ret_fn_ptr_params = nullptr,
                        int* out_n_ret_fn_ptr_params = nullptr,
                        bool* out_ret_fn_ptr_variadic = nullptr);

  // Parse a full type-name (type + declarator with no name): e.g. for sizeof.
  TypeSpec parse_type_name();

  // Skip __attribute__((...)) sequences (zero or more).
  void skip_attributes();
  // Skip noexcept / noexcept(expr) / throw() exception specifications.
  void skip_exception_spec();

  // Consume __attribute__((...)) sequences, recording supported attributes
  // into the type when provided (currently aligned/vector_size).
  void parse_attributes(TypeSpec* ts);

  // Skip __asm__("...") or asm("...") sequences.
  void skip_asm();

  // Skip a balanced paren group (consuming the closing paren).
  void skip_paren_group();

  // Evaluate a deferred NTTP default expression for a template parameter.
  // Returns true if evaluation succeeded and writes the result to *out.
  bool eval_deferred_nttp_default(
      const std::string& tpl_name, int param_idx,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings,
      long long* out);
  bool eval_deferred_nttp_expr_tokens(
      const std::string& tpl_name,
      const std::vector<Token>& toks,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings,
      long long* out);
  Node* find_template_struct_primary(const std::string& name) const;
  const std::vector<Node*>* find_template_struct_specializations(
      const Node* primary_tpl) const;
  void register_template_struct_primary(const std::string& name, Node* node);
  void register_template_struct_specialization(const char* primary_name, Node* node);
  bool try_parse_template_type_arg(TemplateArgParseResult* out_arg);
  bool try_parse_template_non_type_arg(TemplateArgParseResult* out_arg);
  bool capture_template_arg_expr(int expr_start, TemplateArgParseResult* out_arg);
  bool is_clearly_value_template_arg(const Node* primary_tpl, int arg_idx) const;
  bool parse_template_argument_list(std::vector<TemplateArgParseResult>* out_args,
                                    const Node* primary_tpl = nullptr);
  bool consume_qualified_type_spelling(bool allow_global,
                                       bool consume_final_template_args,
                                       std::string* out_name = nullptr,
                                       QualifiedNameRef* out_qn = nullptr);
  bool consume_template_args_followed_by_scope();
  bool consume_member_pointer_owner_prefix();
  bool try_parse_declarator_member_pointer_prefix(TypeSpec& ts);
  void apply_declarator_pointer_token(TypeSpec& ts, TokenKind pointer_tok,
                                      bool preserve_array_base);
  bool parse_dependent_typename_specifier(std::string* out_name = nullptr);
  bool parse_operator_declarator_name(std::string* out_name);
  bool parse_qualified_declarator_name(std::string* out_name);
  bool is_grouped_declarator_start() const;
  bool try_parse_grouped_declarator(TypeSpec& ts, const char** out_name,
                                    std::vector<long long>* out_dims);
  void parse_normal_declarator_tail(TypeSpec& ts, const char** out_name,
                                    std::vector<long long>* out_dims);
  void parse_declarator_parameter_list(std::vector<Node*>* out_params,
                                       bool* out_variadic);
  void parse_parenthesized_function_pointer_suffix(
      TypeSpec& ts, bool is_nested_fn_ptr,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic,
      Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
      bool* out_ret_fn_ptr_variadic);
  void store_declarator_function_pointer_params(
      Node*** out_params, int* out_n_params, bool* out_variadic,
      const std::vector<Node*>& params, bool variadic);
  long long parse_one_declarator_array_dim(TypeSpec& ts);
  void parse_declarator_array_suffixes(TypeSpec& ts,
                                       std::vector<long long>* out_dims);
  void apply_declarator_array_dims(TypeSpec& ts,
                                   const std::vector<long long>& decl_dims);
  TypenameTemplateParamKind classify_typename_template_parameter() const;
  // Template-scope stack helpers.
  void push_template_scope(TemplateScopeKind kind,
                           const std::vector<TemplateScopeParam>& params);
  void pop_template_scope();
  // Check if a name is a type parameter in any active template scope frame.
  // Walks the stack from innermost to outermost.
  bool is_template_scope_type_param(const std::string& name) const;

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
  Node* parse_sizeof_pack_expr(int ln); // sizeof...(pack)
  Node* parse_new_expr(int ln, bool global_qualified); // C++ new expression

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

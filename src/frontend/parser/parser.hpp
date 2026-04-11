#pragma once

#include <chrono>
#include <cstdint>

// Parser entry/index surface.
//
// Role:
// - owns the recursive-descent parser state and top-level entry point
// - declares the cross-translation-unit parser helper families
// - acts as the navigation index for parser_*.cpp implementation files
//
// Implementation map:
// - parser_core.cpp: token cursor, diagnostics, namespace/visibility plumbing
// - parser_types_base.cpp: type-specifier parsing and enum parsing
// - parser_types_declarator.cpp: declarator parsing and qualified type spelling
// - parser_types_struct.cpp: record/struct parsing and in-record dispatch
// - parser_types_template.cpp: template argument parsing and template metadata
// - parser_expressions.cpp: expression parsing
// - parser_statements.cpp: statement parsing
// - parser_declarations.cpp: declaration and translation-unit entry points
// - parser_support.cpp: AST builders and shared parser helpers
//
// Design constraints:
//  - All AST nodes and strings are allocated from the supplied Arena.
//  - typedef names are tracked in a std::set<std::string> for disambiguation.
//  - Anonymous struct/union/enum definitions are collected and prepended to
//    the program item list so the IR builder sees types before use sites.
//
// Pure-C backport note: replace class with struct + free functions.
// Replace std::set with a sorted char*[] searched by strcmp.

#include <functional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "source_profile.hpp"
#include "token.hpp"
#include "../string_id_table.hpp"

namespace c4c {

class Parser {
 public:
  // ── parser-side model types ───────────────────────────────────────────────
  using SymbolId = uint32_t;

  static constexpr SymbolId kInvalidSymbol = 0;

  // Identifier-atom identity layer. Symbols reuse TextTable storage rather
  // than maintaining a second owning copy of the spelling.
  struct SymbolTable {
    explicit SymbolTable(TextTable* texts = nullptr) : texts_(texts) {}

    void attach_text_table(TextTable* texts) { texts_ = texts; }

    SymbolId find_identifier(TextId text_id) const {
      if (!texts_ || text_id == kInvalidText) return kInvalidSymbol;
      const auto it = symbol_ids_.id_by_key_.find(text_id);
      return it == symbol_ids_.id_by_key_.end() ? kInvalidSymbol : it->second;
    }

    SymbolId find_identifier(std::string_view text) const {
      if (!texts_ || text.empty()) return kInvalidSymbol;
      const auto text_it = texts_->id_by_key_.find(std::string(text));
      if (text_it == texts_->id_by_key_.end()) return kInvalidSymbol;
      return find_identifier(text_it->second);
    }

    SymbolId intern_identifier(TextId text_id) {
      if (!texts_ || text_id == kInvalidText) return kInvalidSymbol;
      return symbol_ids_.intern(text_id);
    }

    SymbolId intern_identifier(std::string_view text) {
      if (!texts_) return kInvalidSymbol;
      return intern_identifier(texts_->intern(text));
    }

    TextId text_id(SymbolId id) const {
      const TextId* stored = symbol_ids_.lookup(id);
      return stored ? *stored : kInvalidText;
    }

    std::string_view spelling(SymbolId id) const {
      if (!texts_) return {};
      return texts_->lookup(text_id(id));
    }

    size_t size() const { return symbol_ids_.size(); }

    TextTable* texts_ = nullptr;
    KeyIdTable<SymbolId, kInvalidSymbol, TextId> symbol_ids_;
  };

  // Parser-local semantic tables keyed by identifier atoms. Scope and binding
  // stay outside this struct; this only centralizes atom-keyed parser facts.
  struct ParserNameTables {
    SymbolId find_identifier(TextId text_id) const {
      return symbols ? symbols->find_identifier(text_id) : kInvalidSymbol;
    }

    SymbolId find_identifier(std::string_view text) const {
      return symbols ? symbols->find_identifier(text) : kInvalidSymbol;
    }

    SymbolId intern_identifier(TextId text_id) {
      return symbols ? symbols->intern_identifier(text_id) : kInvalidSymbol;
    }

    SymbolId intern_identifier(std::string_view text) {
      return symbols ? symbols->intern_identifier(text) : kInvalidSymbol;
    }

    bool is_typedef(SymbolId id) const {
      return id != kInvalidSymbol && typedefs.count(id) != 0;
    }

    bool has_typedef_type(SymbolId id) const {
      return id != kInvalidSymbol && typedef_types.count(id) != 0;
    }

    const TypeSpec* lookup_typedef_type(SymbolId id) const {
      const auto it = typedef_types.find(id);
      return it == typedef_types.end() ? nullptr : &it->second;
    }

    SymbolTable* symbols = nullptr;
    std::unordered_set<SymbolId> typedefs;
    std::unordered_set<SymbolId> user_typedefs;
    std::unordered_map<SymbolId, TypeSpec> typedef_types;
    std::unordered_map<SymbolId, TypeSpec> var_types;
  };

  using ParserSymbolTables = ParserNameTables;

  // Namespace tree node used by C++ qualified-name lookup and using-directive
  // visibility tracking.
  struct NamespaceContext {
    int id = 0;
    int parent_id = -1;
    bool is_anonymous = false;
    const char* display_name = nullptr;
    const char* canonical_name = nullptr;
  };

  // Spelled qualified name as seen in the token stream.
  struct QualifiedNameRef {
    bool is_global_qualified = false;
    std::vector<std::string> qualifier_segments;
    std::string base_name;

    bool is_unqualified_atom() const {
      return !is_global_qualified && qualifier_segments.empty();
    }

    std::string spelled(bool include_global_prefix = false) const {
      std::string name;
      if (include_global_prefix && is_global_qualified) name = "::";
      for (size_t i = 0; i < qualifier_segments.size(); ++i) {
        if (!name.empty() && name != "::") name += "::";
        name += qualifier_segments[i];
      }
      if (!name.empty() && name != "::") name += "::";
      name += base_name;
      return name;
    }
  };

  // Result container for one parsed template argument after type-vs-value
  // disambiguation has been resolved.
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

  struct RecordBodyState {
    std::vector<Node*> fields;
    std::vector<Node*> methods;
    std::vector<const char*> member_typedef_names;
    std::vector<TypeSpec> member_typedef_types;
  };

  enum class RecordMemberRecoveryResult {
    Failed,
    SyncedAtSemicolon,
    StoppedAtNextMember,
    StoppedAtRBrace,
  };

  struct ParseContextFrame {
    std::string function_name;
    int token_index = -1;
  };

  struct ParseFailure {
    bool active = false;
    bool committed = true;
    int token_index = -1;
    TokenKind token_kind = TokenKind::EndOfFile;
    int line = 1;
    int column = 1;
    std::string function_name;
    std::string expected;
    std::string got;
    std::string detail;
    std::vector<std::string> stack_trace;
  };

  struct ParseDebugEvent {
    std::string kind;
    std::string function_name;
    std::string detail;
    int token_index = -1;
    int line = 1;
    int column = 1;
  };

  enum ParseDebugChannel : unsigned {
    ParseDebugNone = 0,
    ParseDebugGeneral = 1u << 0,
    ParseDebugTentative = 1u << 1,
    ParseDebugInjected = 1u << 2,
    ParseDebugAll = ParseDebugGeneral | ParseDebugTentative | ParseDebugInjected,
  };

  struct ParseContextGuard {
    Parser* parser = nullptr;
    ParseContextGuard(Parser* parser, const char* function_name);
    ~ParseContextGuard();
  };

  // ── tentative parse snapshot / guard ─────────────────────────────────────
  enum class TentativeParseMode {
    Heavy,
    Lite,
  };

  // Cheap speculative parser state used by lite tentative parsing. This is
  // the rollback surface that remains safe for syntax-only probes.
  struct ParserLiteSnapshot {
    int pos;
    std::string last_resolved_typedef;
    int template_arg_expr_depth = 0;
    size_t token_mutation_count = 0;
  };

  struct ParserSnapshot {
    ParserLiteSnapshot lite;
    ParserSymbolTables symbol_tables;
    std::set<std::string> non_atom_typedefs;
    std::set<std::string> non_atom_user_typedefs;
    std::unordered_map<std::string, TypeSpec> non_atom_typedef_types;
    std::unordered_map<std::string, TypeSpec> non_atom_var_types;
  };

  struct TentativeParseStats {
    int heavy_enter = 0;
    int heavy_commit = 0;
    int heavy_rollback = 0;
    int lite_enter = 0;
    int lite_commit = 0;
    int lite_rollback = 0;
  };

  // RAII guard that saves parser state on construction and restores it on
  // destruction unless commit() has been called.
  struct TentativeParseGuard {
    Parser& parser;
    ParserSnapshot snapshot;
    int start_pos = -1;
    bool committed = false;

    explicit TentativeParseGuard(Parser& p)
        : parser(p), snapshot(p.save_state()), start_pos(snapshot.lite.pos) {
      parser.note_tentative_parse_event(TentativeParseMode::Heavy,
                                        "tentative_enter", start_pos,
                                        start_pos);
    }

    ~TentativeParseGuard() {
      if (!committed) {
        parser.note_tentative_parse_event(TentativeParseMode::Heavy,
                                          "tentative_rollback", start_pos,
                                          parser.pos_);
        parser.restore_state(snapshot);
      }
    }

    void commit() {
      if (committed) return;
      parser.note_tentative_parse_event(TentativeParseMode::Heavy,
                                        "tentative_commit", start_pos,
                                        parser.pos_);
      committed = true;
    }
  };

  struct TentativeParseGuardLite {
    Parser& parser;
    ParserLiteSnapshot snapshot;
    int start_pos = -1;
    bool committed = false;

    explicit TentativeParseGuardLite(Parser& p)
        : parser(p), snapshot(p.save_lite_state()), start_pos(snapshot.pos) {
      parser.note_tentative_parse_event(TentativeParseMode::Lite,
                                        "tentative_enter", start_pos,
                                        start_pos);
    }

    ~TentativeParseGuardLite() {
      if (!committed) {
        parser.note_tentative_parse_event(TentativeParseMode::Lite,
                                          "tentative_rollback", start_pos,
                                          parser.pos_);
        parser.restore_lite_state(snapshot);
      }
    }

    void commit() {
      if (committed) return;
      parser.note_tentative_parse_event(TentativeParseMode::Lite,
                                        "tentative_commit", start_pos,
                                        parser.pos_);
      committed = true;
    }
  };

  struct LocalVarBindingSuppressionGuard {
    Parser& parser;
    bool old = false;

    explicit LocalVarBindingSuppressionGuard(Parser& p)
        : parser(p), old(p.suppress_local_var_bindings_) {
      parser.suppress_local_var_bindings_ = true;
    }

    ~LocalVarBindingSuppressionGuard() {
      parser.suppress_local_var_bindings_ = old;
    }
  };

  struct RecordTemplatePreludeGuard {
    Parser* parser = nullptr;
    std::vector<std::string> injected_type_params;
    bool pushed_template_scope = false;

    explicit RecordTemplatePreludeGuard(Parser* p) : parser(p) {}

    ~RecordTemplatePreludeGuard() {
      if (!parser) return;
      if (pushed_template_scope && !parser->template_scope_stack_.empty()) {
        parser->template_scope_stack_.pop_back();
      }
      for (const std::string& name : injected_type_params) {
        parser->unregister_typedef_binding(name);
      }
    }
  };

  struct TemplateDeclarationPreludeGuard {
    Parser* parser = nullptr;
    std::vector<std::string> injected_type_params;
    bool pushed_template_scope = false;

    explicit TemplateDeclarationPreludeGuard(Parser* p) : parser(p) {}

    ~TemplateDeclarationPreludeGuard() {
      if (!parser) return;
      if (pushed_template_scope && !parser->template_scope_stack_.empty()) {
        parser->template_scope_stack_.pop_back();
      }
      for (const std::string& name : injected_type_params) {
        parser->unregister_typedef_binding(name);
      }
    }
  };

  ParserLiteSnapshot save_lite_state() const;
  void restore_lite_state(const ParserLiteSnapshot& snap);
  ParserSnapshot save_state() const;
  void restore_state(const ParserSnapshot& snap);
  ParserSymbolTables& parser_symbol_tables();
  const ParserSymbolTables& parser_symbol_tables() const;

  // ── public parser entry points ────────────────────────────────────────────
  // All members public (required by project coding constraints).
  explicit Parser(std::vector<Token> tokens, Arena& arena,
                  SourceProfile source_profile = SourceProfile::C,
                  const std::string& source_file = "<input>");

  // Parse the entire token stream and return a NK_PROGRAM node.
  Node* parse();

  // ── core parser state ─────────────────────────────────────────────────────
  // Token stream + cursor are the parser's single source of truth.
  std::vector<Token> tokens_;
  struct TokenMutation {
    int pos = -1;
    Token token;
  };
  std::vector<TokenMutation> token_mutations_;
  int pos_;
  Arena& arena_;
  SourceProfile source_profile_;
  std::string source_file_;  // source path used by diagnostics
  TextTable parser_texts_;
  std::unordered_map<TextId, TextId> parser_text_ids_;
  SymbolTable parser_symbols_{&parser_texts_};
  ParserNameTables parser_name_tables_{&parser_symbols_};

  // ── name / type knowledge accumulated during parsing ─────────────────────
  // Declared concept names visible to the parser. Kept separate from typedef
  // tracking so concept-ids do not get mistaken for type names.
  std::set<std::string> concept_names_;
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

  // Enum constants: name → value (populated as enums are parsed).
  // Used to evaluate enum initializers that reference previously-defined constants.
  std::unordered_map<std::string, long long> enum_consts_;
  // Global const/constexpr integer bindings visible to parser-time constant folding.
  std::unordered_map<std::string, long long> const_int_bindings_;
  bool suppress_local_var_bindings_ = false;
  // Qualified function names (populated as functions are declared/defined).
  // Used by lookup_value_in_context for namespace-aware function lookup.
  std::set<std::string> known_fn_names_;
  // String-keyed fallback storage for composed or synthesized names that are
  // not eligible for source-atom SymbolId identity.
  std::set<std::string> non_atom_typedefs_;
  std::set<std::string> non_atom_user_typedefs_;
  std::unordered_map<std::string, TypeSpec> non_atom_typedef_types_;
  std::unordered_map<std::string, TypeSpec> non_atom_var_types_;
  // Struct member typedef scoped names: "StructTag::TypeName" → TypeSpec.
  // Populated when parsing typedef inside struct bodies.
  std::unordered_map<std::string, TypeSpec> struct_typedefs_;

  // ── record / enum definition caches ──────────────────────────────────────
  // Collected struct/enum defs are prepended to the final program node.
  std::vector<Node*> struct_defs_;
  int anon_counter_;  // counter for anonymous tag names
  // Struct/union tags that already have a full definition (with body).
  // Used to detect block-scoped redefinitions and generate unique tags.
  std::set<std::string> defined_struct_tags_;
  // Struct/union tag → NK_STRUCT_DEF node (populated when parsing struct bodies).
  // Used by eval_const_int to compute __builtin_offsetof at parse time.
  std::unordered_map<std::string, Node*> struct_tag_def_map_;
  // Last enum definition node produced by parse_base_type(), if any.
  // Used so declaration-only enum statements (`enum { ... };`) can be retained.
  Node* last_enum_def_;

  // ── template metadata and active template scopes ─────────────────────────
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
    std::vector<bool> param_is_pack;
    std::vector<bool> param_has_default;
    std::vector<TypeSpec> param_default_types;
    std::vector<long long> param_default_values;
    TypeSpec aliased_type;  // TypeSpec from parse_type_name() (has tpl_struct_origin/arg_refs)
  };
  std::unordered_map<std::string, AliasTemplateInfo> alias_template_info_;
  // Template-scope stack: tracks active template parameter visibility.
  std::vector<TemplateScopeFrame> template_scope_stack_;
  // Set by the using-alias handler to let the template wrapper detect that
  // a using type alias was defined during `parse_top_level()`.
  std::string last_using_alias_name_;

  // ── active parse context ──────────────────────────────────────────────────
  // Tag of the struct currently being parsed (empty if not in struct body).
  std::string current_struct_tag_;
  // True while parsing a file-scope declaration in parse_top_level().
  bool parsing_top_level_context_;
  // True while parsing an explicit template specialization (template<>).
  bool parsing_explicit_specialization_ = false;
  // Nesting depth for expression parses that are constrained by template
  // argument delimiters, so expression parsing does not consume enclosing
  // template-close tokens as operators.
  int template_arg_expr_depth_ = 0;

  // ── namespace / using-directive visibility state ─────────────────────────
  // Transitional flattened path kept only as a compatibility bridge.
  std::string current_namespace_;
  std::vector<NamespaceContext> namespace_contexts_;
  std::vector<int> namespace_stack_;
  std::unordered_map<std::string, int> named_namespace_contexts_;
  std::unordered_map<int, std::vector<int>> anonymous_namespace_children_;
  // Unqualified visible aliases introduced by using-declarations per namespace context.
  std::unordered_map<int, std::unordered_map<std::string, std::string>> using_value_aliases_;
  std::unordered_map<int, std::vector<int>> using_namespace_contexts_;

  // ── diagnostic and recovery state ────────────────────────────────────────
  // True if parse() encountered any recoverable parse error.
  bool had_error_;
  int parse_error_count_ = 0;
  int max_parse_errors_ = 20;
  int max_no_progress_steps_ = 8;
  unsigned parser_debug_channels_ = ParseDebugNone;
  int max_parse_debug_events_ = 256;
  int parse_debug_progress_interval_ms_ = 1000;
  std::vector<ParseContextFrame> parse_context_stack_;
  std::vector<ParseDebugEvent> parse_debug_events_;
  ParseFailure best_parse_failure_;
  int best_parse_stack_token_index_ = -1;
  std::vector<std::string> best_parse_stack_trace_;
  TentativeParseStats tentative_parse_stats_;
  std::chrono::steady_clock::time_point parse_debug_started_at_{};
  std::chrono::steady_clock::time_point parse_debug_last_progress_at_{};

  // ── pragma state ─────────────────────────────────────────────────────────
  // #pragma pack state: current packing alignment (0 = default/no packing).
  int pack_alignment_ = 0;
  std::vector<int> pack_stack_;  // for #pragma pack(push/pop)

  // #pragma GCC visibility state: 0=default, 1=hidden, 2=protected.
  uint8_t visibility_ = 0;
  std::vector<uint8_t> visibility_stack_;  // for push/pop
  ExecutionDomain execution_domain_ = ExecutionDomain::Host;

  // ── pragma helpers ────────────────────────────────────────────────────────
  void handle_pragma_pack(const std::string& args);
  void handle_pragma_gcc_visibility(const std::string& args);
  void handle_pragma_exec(const std::string& args);

  // ── parser diagnostics / debug tracing ───────────────────────────────────
  void set_parser_debug(bool enabled);
  void set_parser_debug_channels(unsigned channels);
  bool parser_debug_enabled() const;
  bool parse_debug_event_visible(const char* kind) const;
  void clear_parse_debug_state();
  void reset_parse_debug_progress();
  void push_parse_context(const char* function_name);
  void pop_parse_context();
  void note_parse_debug_event(const char* kind, const char* detail = nullptr);
  void note_parse_debug_event_for(const char* kind,
                                  const char* function_name,
                                  const char* detail = nullptr);
  void maybe_emit_parse_debug_progress();
  void note_tentative_parse_event(TentativeParseMode mode,
                                  const char* kind,
                                  int start_pos,
                                  int end_pos);
  void note_parse_failure(const char* expected,
                          const char* detail = nullptr,
                          bool committed = true);
  void note_parse_failure_message(const char* detail,
                                  bool committed = true);
  std::vector<std::string> best_debug_summary_stack() const;
  std::string format_best_parse_failure() const;
  void dump_parse_debug_trace() const;
  std::string format_tentative_parse_detail(TentativeParseMode mode,
                                            const char* kind,
                                            int start_pos,
                                            int end_pos) const;
  std::string format_parse_failure_token_window(const ParseFailure& failure) const;

  // ── token cursor / shared token utilities ────────────────────────────────
  const Token& cur() const;              // current token
  const Token& peek(int offset = 0) const; // peek at pos_+offset (0=current)
  const Token& consume();                // consume and return current token
  bool at_end() const;
  bool check(TokenKind k) const;         // is current token kind k?
  bool peek_next_is(TokenKind k) const;  // is next token kind k?
  bool match(TokenKind k);               // consume if check(k)
  void expect(TokenKind k);              // consume or throw
  const char* diag_file_at(int token_index) const;
  // Template angle-bracket helpers: parser-owned handling for >-prefixed tokens.
  bool check_template_close() const;     // is current > / >> / >= / >>=?
  bool parse_greater_than_in_template_list(bool consume_last_token = true);
  bool match_template_close();           // consume one template-close >
  void expect_template_close();          // match_template_close or throw
  void skip_until(TokenKind k);          // skip tokens until k (consume k)
  bool try_parse_operator_function_id(std::string& out_name);

  // ── parser mode / identifier classification ──────────────────────────────
  bool is_type_start() const;            // can current token start a type?
  bool can_start_parameter_type() const;
  bool looks_like_unresolved_identifier_type_head(int pos) const;
  TextId parser_text_id_for_token(TextId token_text_id,
                                  std::string_view fallback = {}) {
    if (token_text_id != kInvalidText) {
      const auto it = parser_text_ids_.find(token_text_id);
      if (it != parser_text_ids_.end()) return it->second;
    }
    if (fallback.empty()) return kInvalidText;
    const TextId parser_text_id = parser_texts_.intern(fallback);
    if (token_text_id != kInvalidText) {
      parser_text_ids_.emplace(token_text_id, parser_text_id);
    }
    return parser_text_id;
  }
  SymbolId symbol_id_for_token_text(TextId token_text_id,
                                    std::string_view fallback = {}) {
    return parser_name_tables_.intern_identifier(
        parser_text_id_for_token(token_text_id, fallback));
  }
  SymbolId symbol_id_for_token(const Token& token);
  std::string_view symbol_spelling(SymbolId id) const {
    return parser_symbols_.spelling(id);
  }
  bool has_typedef_name(const std::string& name) const;
  bool has_typedef_type(const std::string& name) const;
  const TypeSpec* find_typedef_type(const std::string& name) const;
  bool has_visible_typedef_type(const std::string& name) const;
  const TypeSpec* find_visible_typedef_type(const std::string& name) const;
  TypeSpec resolve_typedef_type_chain(TypeSpec ts) const;
  TypeSpec resolve_struct_like_typedef_type(TypeSpec ts) const;
  bool are_types_compatible(const TypeSpec& lhs, const TypeSpec& rhs) const;
  bool resolves_to_record_ctor_type(TypeSpec ts) const;
  bool is_user_typedef_name(const std::string& name) const;
  bool has_conflicting_user_typedef_binding(const std::string& name,
                                            const TypeSpec& type) const;
  void register_typedef_name(const std::string& name, bool is_user_typedef);
  void register_typedef_binding(const std::string& name, const TypeSpec& type,
                                bool is_user_typedef);
  void unregister_typedef_binding(const std::string& name);
  void register_synthesized_typedef_binding(const std::string& name);
  void register_tag_type_binding(const std::string& name, TypeBase base,
                                 const char* tag,
                                 TypeBase enum_underlying_base = TB_VOID);
  void cache_typedef_type(const std::string& name, const TypeSpec& type);
  void register_struct_member_typedef_binding(const std::string& scoped_name,
                                              const TypeSpec& type);
  bool has_var_type(const std::string& name) const;
  const TypeSpec* find_var_type(const std::string& name) const;
  const TypeSpec* find_visible_var_type(const std::string& name) const;
  void register_var_type_binding(const std::string& name, const TypeSpec& type);
  bool has_known_fn_name(const std::string& name) const;
  void register_known_fn_name(const std::string& name);
  bool is_typedef_name(const std::string& s) const;
  bool is_cpp_mode() const {
    return source_profile_ == SourceProfile::CppSubset ||
           source_profile_ == SourceProfile::C4;
  }

  // ── namespace resolution / qualified-name plumbing ───────────────────────
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
  bool lookup_concept_in_context(int context_id, const std::string& name,
                                 std::string* resolved) const;
  std::string qualify_name(const std::string& name) const;
  const char* qualify_name_arena(const char* name);
  std::string resolve_visible_value_name(const std::string& name) const;
  std::string resolve_visible_type_name(const std::string& name) const;
  std::string resolve_visible_concept_name(const std::string& name) const;
  bool is_concept_name(const std::string& name) const;
  bool peek_qualified_name(QualifiedNameRef* out, bool allow_global = true) const;
  bool consume_template_parameter_type_start(bool allow_typename_keyword = true);
  QualifiedNameRef parse_qualified_name(bool allow_global = true);
  void apply_qualified_name(Node* node, const QualifiedNameRef& qn,
                            const char* resolved_name = nullptr);
  void apply_decl_namespace(Node* node, int context_id, const char* unqualified_name);

  // ── generic skipping / attribute helpers ─────────────────────────────────
  void skip_attributes();
  void skip_exception_spec();
  void parse_attributes(TypeSpec* ts);
  void skip_asm();
  void skip_paren_group();
  void skip_brace_group();

  // ── template argument parsing / template instantiation helpers ───────────
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
  const Node* select_template_struct_pattern_for_args(
      const std::vector<TemplateArgParseResult>& args,
      const Node* primary_tpl,
      const std::vector<Node*>* specializations,
      std::vector<std::pair<std::string, TypeSpec>>* out_type_bindings,
      std::vector<std::pair<std::string, long long>>* out_nttp_bindings) const;
  void register_template_struct_primary(const std::string& name, Node* node);
  void register_template_struct_specialization(const char* primary_name, Node* node);
  bool parse_next_template_argument(std::vector<TemplateArgParseResult>* out_args,
                                    const Node* primary_tpl, int arg_idx,
                                    bool* out_has_more,
                                    const std::vector<bool>* explicit_param_is_nttp = nullptr);
  bool try_parse_template_type_arg(TemplateArgParseResult* out_arg);
  bool try_parse_template_non_type_expr(int expr_start,
                                        TemplateArgParseResult* out_arg);
  bool try_parse_template_non_type_arg(TemplateArgParseResult* out_arg);
  bool capture_template_arg_expr(int expr_start, TemplateArgParseResult* out_arg);
  bool is_clearly_value_template_arg(const Node* primary_tpl, int arg_idx,
                                     const std::vector<bool>* explicit_param_is_nttp = nullptr) const;
  bool parse_template_argument_list(std::vector<TemplateArgParseResult>* out_args,
                                    const Node* primary_tpl = nullptr,
                                    const std::vector<bool>* explicit_param_is_nttp = nullptr);
  bool ensure_template_struct_instantiated_from_args(
      const std::string& template_name,
      const Node* primary_tpl,
      const std::vector<TemplateArgParseResult>& args,
      int line,
      std::string* out_mangled,
      const char* debug_reason = nullptr,
      TypeSpec* out_resolved = nullptr);
  std::string build_template_struct_mangled_name(
      const std::string& template_name,
      const Node* primary_tpl,
      const Node* selected_tpl,
      const std::vector<TemplateArgParseResult>& args) const;
  bool decode_type_ref_text(const std::string& text, TypeSpec* out);
  TypenameTemplateParamKind classify_typename_template_parameter() const;
  void push_template_scope(TemplateScopeKind kind,
                           const std::vector<TemplateScopeParam>& params);
  void pop_template_scope();
  // Check if a name is a type parameter in any active template scope frame.
  // Walks the stack from innermost to outermost.
  bool is_template_scope_type_param(const std::string& name) const;

  // ── type spelling / type-specifier parsing ────────────────────────────────
  bool consume_qualified_type_spelling_with_typename(
      bool require_typename, bool allow_global,
      bool consume_final_template_args,
      std::string* out_name = nullptr,
      QualifiedNameRef* out_qn = nullptr);
  bool consume_qualified_type_spelling(bool allow_global,
                                       bool consume_final_template_args,
                                       std::string* out_name = nullptr,
                                       QualifiedNameRef* out_qn = nullptr);
  bool consume_template_args_before_scope();
  bool consume_member_pointer_owner_prefix();
  bool try_parse_declarator_member_pointer_prefix(TypeSpec& ts);
  void apply_declarator_pointer_token(TypeSpec& ts, TokenKind pointer_tok,
                                      bool preserve_array_base);
  bool parse_dependent_typename_specifier(std::string* out_name = nullptr);
  bool try_parse_cpp_scoped_base_type(bool already_have_base, TypeSpec* out_ts);
  bool try_parse_qualified_base_type(TypeSpec* out_ts);

  // Parse a complete type specifier (base type + qualifiers).
  // ptr_level and array_size are NOT parsed here (those are in the declarator).
  TypeSpec parse_base_type();
  // Parse a full type-name (type + declarator with no name): e.g. for sizeof.
  TypeSpec parse_type_name();

  // ── declarator parsing ────────────────────────────────────────────────────
  // Modifies ts in place. Returns the declared name via out_name.
  // Function-pointer metadata is optionally surfaced through the out_* params.
  void parse_declarator(TypeSpec& ts, const char** out_name,
                        Node*** out_fn_ptr_params = nullptr,
                        int* out_n_fn_ptr_params = nullptr,
                        bool* out_fn_ptr_variadic = nullptr,
                        bool* out_is_parameter_pack = nullptr,
                        Node*** out_ret_fn_ptr_params = nullptr,
                        int* out_n_ret_fn_ptr_params = nullptr,
                        bool* out_ret_fn_ptr_variadic = nullptr);
  bool parse_operator_declarator_name(std::string* out_name);
  bool parse_qualified_declarator_name(std::string* out_name);
  bool is_grouped_declarator_start() const;
  bool is_parenthesized_pointer_declarator_start();
  void parse_pointer_ref_qualifiers(TypeSpec& ts, TokenKind pointer_tok,
                                    bool preserve_array_base,
                                    bool consume_pointer_token = true);
  void consume_declarator_post_pointer_qualifiers();
  void parse_declarator_prefix(TypeSpec& ts, bool* out_is_parameter_pack);
  bool try_parse_grouped_declarator(TypeSpec& ts, const char** out_name,
                                    std::vector<long long>* out_dims);
  void parse_normal_declarator_tail(TypeSpec& ts, const char** out_name,
                                    std::vector<long long>* out_dims);
  void parse_non_parenthesized_declarator_suffixes(
      TypeSpec& ts, const char** out_name, std::vector<long long>* out_dims);
  void parse_declarator_parameter_list(std::vector<Node*>* out_params,
                                       bool* out_variadic);
  void parse_top_level_parameter_list(
      std::vector<Node*>* out_params,
      std::vector<const char*>* out_knr_param_names,
      bool* out_variadic);
  void parse_parenthesized_function_pointer_suffix(
      TypeSpec& ts, bool is_nested_fn_ptr,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic,
      Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
      bool* out_ret_fn_ptr_variadic);
  void parse_parenthesized_pointer_declarator_prefix(TypeSpec& ts);
  void skip_parenthesized_pointer_declarator_array_chunks();
  bool parse_parenthesized_pointer_declarator_name(const char** out_name);
  bool try_parse_nested_parenthesized_pointer_declarator(
      TypeSpec& ts, const char** out_name,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic);
  bool parse_parenthesized_pointer_declarator_inner(
      TypeSpec& ts, const char** out_name,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic);
  void finalize_parenthesized_pointer_declarator(
      TypeSpec& ts, bool is_nested_fn_ptr, std::vector<long long>* decl_dims,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic,
      Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
      bool* out_ret_fn_ptr_variadic);
  void parse_parenthesized_pointer_declarator(
      TypeSpec& ts, const char** out_name,
      Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
      bool* out_fn_ptr_variadic,
      Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
      bool* out_ret_fn_ptr_variadic);
  void parse_non_parenthesized_declarator(TypeSpec& ts, const char** out_name);
  void parse_non_parenthesized_declarator_tail(
      TypeSpec& ts, const char** out_name,
      bool decay_plain_function_suffix);
  void parse_plain_function_declarator_suffix(TypeSpec& ts,
                                              bool decay_to_function_pointer);
  void store_declarator_function_pointer_params(
      Node*** out_params, int* out_n_params, bool* out_variadic,
      const std::vector<Node*>& params, bool variadic);
  long long parse_one_declarator_array_dim(TypeSpec& ts);
  void parse_declarator_array_suffixes(TypeSpec& ts,
                                       std::vector<long long>* out_dims);
  void apply_declarator_array_dims(TypeSpec& ts,
                                   const std::vector<long long>& decl_dims);

  // ── record parsing (struct / union) ──────────────────────────────────────
  // This family handles both outer record definitions and in-record member
  // dispatch, including recovery in class/struct bodies.
  bool try_parse_record_using_member(
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types);
  bool try_parse_record_typedef_member(
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types);
  bool try_parse_nested_record_member(
      std::vector<Node*>* fields,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_enum_member(
      std::vector<Node*>* fields,
      const std::function<void(const char*)>& check_dup_field);
  bool is_record_special_member_name(
      const std::string& lex, const std::string& struct_source_name) const;
  bool try_parse_record_constructor_member(
      const std::string& struct_source_name,
      std::vector<Node*>* methods);
  bool try_parse_record_destructor_member(
      const std::string& struct_source_name,
      std::vector<Node*>* methods);
  bool try_parse_record_method_or_field_member(
      std::vector<Node*>* fields,
      std::vector<Node*>* methods,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_type_like_member_dispatch(
      std::vector<Node*>* fields,
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_member_dispatch(
      const std::string& struct_source_name,
      std::vector<Node*>* fields,
      std::vector<Node*>* methods,
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_special_member_dispatch(
      const std::string& struct_source_name,
      std::vector<Node*>* methods);
  bool try_parse_record_member_with_template_prelude(
      const std::string& struct_source_name,
      std::vector<Node*>* fields,
      std::vector<Node*>* methods,
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types,
      const std::function<void(const char*)>& check_dup_field);
  bool begin_record_member_parse();
  bool try_parse_record_member_prelude(std::vector<Node*>* methods);
  bool try_parse_record_member(
      const std::string& struct_source_name,
      std::vector<Node*>* fields,
      std::vector<Node*>* methods,
      std::vector<const char*>* member_typedef_names,
      std::vector<TypeSpec>* member_typedef_types,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_body_member(
      const std::string& struct_source_name,
      RecordBodyState* body_state,
      const std::function<void(const char*)>& check_dup_field);
  bool try_parse_record_access_label();
  bool try_skip_record_friend_member();
  bool try_skip_record_static_assert_member(std::vector<Node*>* methods);
  RecordMemberRecoveryResult recover_record_member_parse_error(int member_start_pos);
  void parse_record_template_member_prelude(
      std::vector<std::string>* injected_type_params,
      bool* pushed_template_scope);
  void parse_decl_attrs_for_record(int line, TypeSpec* attr_ts);
  void skip_record_base_specifier_tail();
  bool try_parse_record_base_specifier(TypeSpec* base_ts);
  void parse_record_base_clause(std::vector<TypeSpec>* base_types);
  void parse_record_definition_prelude(
      int line,
      TypeSpec* attr_ts,
      const char** tag,
      const char** template_origin_name,
      std::vector<TemplateArgParseResult>* specialization_args,
      std::vector<TypeSpec>* base_types);
  Node* parse_record_tag_setup(int line,
                               bool is_union,
                               const char** tag,
                               const char* template_origin_name,
                               const TypeSpec& attr_ts,
                               const std::vector<TemplateArgParseResult>& specialization_args);
  Node* build_record_definition_node(
      int line,
      bool is_union,
      const char* tag,
      const char* template_origin_name,
      const TypeSpec& attr_ts,
      const std::vector<TemplateArgParseResult>& specialization_args,
      const std::vector<TypeSpec>& base_types);
  Node* parse_record_definition_after_tag_setup(
      int line,
      bool is_union,
      const char* tag,
      const char* template_origin_name,
      const TypeSpec& attr_ts,
      const std::vector<TemplateArgParseResult>& specialization_args,
      const std::vector<TypeSpec>& base_types);
  void begin_record_body_context(const char* tag,
                                 const char* template_origin_name,
                                 std::string* saved_struct_tag,
                                 std::string* struct_source_name);
  void parse_record_body(
      const std::string& struct_source_name,
      RecordBodyState* body_state);
  void parse_record_body_with_context(
      const char* tag,
      const char* template_origin_name,
      RecordBodyState* body_state);
  void finish_record_body_context(const std::string& saved_struct_tag);
  void apply_record_trailing_type_attributes(Node* sd);
  void store_record_body_members(
      Node* sd,
      const RecordBodyState& body_state);
  void register_record_definition(Node* sd,
                                  bool is_union,
                                  const char* source_tag);
  void finalize_record_definition(
      Node* sd,
      bool is_union,
      const char* source_tag,
      const RecordBodyState& body_state);
  void parse_record_definition_body(Node* sd,
                                    bool is_union,
                                    const char* source_tag,
                                    const char* tag,
                                    const char* template_origin_name);
  Node* parse_struct_or_union(bool is_union);

  // ── enum parsing ──────────────────────────────────────────────────────────
  Node* parse_enum();

  // ── parameter parsing ─────────────────────────────────────────────────────
  Node* parse_param();

  // ── expression parsing (Pratt) ────────────────────────────────────────────
  Node* parse_expr();            // comma-level
  Node* parse_assign_expr();     // assignment / ternary
  Node* parse_ternary();         // ternary ?:
  Node* parse_binary(int min_prec); // binary ops (precedence climbing)
  Node* parse_unary();           // prefix unary
  Node* parse_postfix(Node* base); // postfix ops (left-recursive)
  Node* parse_primary();         // primary expression
  Node* parse_lambda_expr(int ln); // minimal C++ lambda expression
  Node* parse_sizeof_pack_expr(int ln); // sizeof...(pack)
  Node* parse_new_expr(int ln, bool global_qualified); // C++ new expression

  // Operator precedence helper.
  static int bin_prec(TokenKind k);

  // ── statement parsing ─────────────────────────────────────────────────────
  Node* parse_stmt();
  Node* parse_block();           // { ... }
  Node* parse_static_assert_declaration();

  // ── initializer parsing ──────────────────────────────────────────────────
  Node* parse_initializer();     // expr or { list }
  Node* parse_init_list();       // { item, item, ... }

  // ── declaration and translation-unit entry points ────────────────────────
  // Parse a local declaration (inside function body). May return a NK_BLOCK
  // when the declaration expands into multiple declarators.
  Node* parse_local_decl();
  // Parse one top-level item. Returns nullptr on EOF.
  Node* parse_top_level();

  // ── AST node builders ─────────────────────────────────────────────────────
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

bool eval_enum_expr(Node* n, const std::unordered_map<std::string, long long>& consts,
                    long long* out);
bool is_dependent_enum_expr(Node* n,
                            const std::unordered_map<std::string, long long>& consts);
TypeBase effective_scalar_base(const TypeSpec& ts);
long long sizeof_base(TypeBase b);
long long align_base(TypeBase b, int ptr_level);
long long sizeof_type_spec(const TypeSpec& ts);
long long alignof_type_spec(const TypeSpec& ts);
bool eval_const_int(Node* n, long long* out,
                    const std::unordered_map<std::string, Node*>* struct_map = nullptr,
                    const std::unordered_map<std::string, long long>* named_consts = nullptr);

bool is_qualifier(TokenKind k);
bool is_storage_class(TokenKind k);
bool is_type_kw(TokenKind k);

bool lexeme_is_imaginary(const char* s);
long long parse_int_lexeme(const char* s);
double parse_float_lexeme(const char* s);

TypeSpec resolve_typedef_chain(TypeSpec ts,
                               const std::unordered_map<std::string, TypeSpec>& tmap);
bool types_compatible_p(TypeSpec a, TypeSpec b,
                        const std::unordered_map<std::string, TypeSpec>& tmap);

}  // namespace c4c

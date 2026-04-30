#pragma once

#include <chrono>
#include <cstdint>

// Parser entry/index surface.
//
// Role:
// - owns the recursive-descent parser state and top-level entry point
// - declares the cross-translation-unit parser helper families
// - acts as the public facade while impl/parser_impl.hpp indexes private
//   implementation files
//
// Implementation map:
// - impl/core.cpp: token cursor, diagnostics, namespace/visibility plumbing
// - impl/types/base.cpp: type-specifier parsing and enum parsing
// - impl/types/declarator.cpp: declarator parsing and qualified type spelling
// - impl/types/struct.cpp: record/struct parsing and in-record dispatch
// - impl/types/template.cpp: template argument parsing and template metadata
// - impl/expressions.cpp: expression parsing
// - impl/statements.cpp: statement parsing
// - impl/declarations.cpp: declaration and translation-unit entry points
// - impl/support.cpp: AST builders and shared parser helpers
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
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "arena.hpp"
#include "ast.hpp"
#include "parser_types.hpp"
#include "shared/qualified_name_table.hpp"
#include "source_profile.hpp"
#include "token.hpp"

namespace c4c {

struct ParserImpl;
struct ParserCoreInputState;
struct ParserSharedLookupState;
struct ParserBindingState;
struct ParserDefinitionState;
struct ParserTemplateState;
struct ParserLexicalScopeState;
struct ParserActiveContextState;
struct ParserNamespaceState;
struct ParserDiagnosticState;
struct ParserPragmaState;
struct ParserLiteSnapshot;
struct ParserSnapshot;
struct ParserTentativeParseStats;
struct ParserParseContextGuard;
struct ParserTentativeParseGuard;
struct ParserTentativeParseGuardLite;
struct ParserLocalVarBindingSuppressionGuard;
struct ParserRecordTemplatePreludeGuard;
struct ParserTemplateDeclarationPreludeGuard;

class Parser {
 public:
  // ── public facade: parser-side table / identity model ────────────────────
  using SymbolId = ParserSymbolId;
  static constexpr SymbolId kInvalidSymbol = kInvalidParserSymbol;
  using SymbolTable = ParserSymbolTable;
  using ParserSymbolTables = c4c::ParserNameTables;
  using FnPtrTypedefInfo = ParserFnPtrTypedefInfo;

  // ── public facade: parser-side structural model ──────────────────────────
  // Namespace tree node used by C++ qualified-name lookup and using-directive
  // visibility tracking.
  using NamespaceContext = ParserNamespaceContext;

  // Spelled qualified name as seen in the token stream.
  using QualifiedNameRef = ParserQualifiedNameRef;

  // Result container for one parsed template argument after type-vs-value
  // disambiguation has been resolved.
  using TemplateArgParseResult = ParserTemplateArgParseResult;

  enum class TypenameTemplateParamKind {
    TypeParameter,
    TypedNonTypeParameter,
  };

  enum class VisibleNameKind {
    Type,
    Value,
    Concept,
  };

  enum class VisibleNameSource {
    Local,
    Namespace,
    UsingAlias,
    ImportedNamespace,
    AnonymousNamespace,
    Fallback,
  };

  struct VisibleNameResult {
    bool found = false;
    VisibleNameKind kind = VisibleNameKind::Type;
    QualifiedNameKey key;
    TextId base_text_id = kInvalidText;
    int context_id = -1;
    VisibleNameSource source = VisibleNameSource::Fallback;
    std::string compatibility_spelling;

    explicit operator bool() const { return found; }
  };

  // Template-scope stack: tracks active template parameter visibility.
  // Each frame records the parameters introduced by one template<...> clause
  // and the semantic context (enclosing class, member template, or free function).
  using TemplateScopeKind = ParserTemplateScopeKind;
  using TemplateScopeParam = ParserTemplateScopeParam;
  using TemplateScopeFrame = ParserTemplateScopeFrame;
  using InjectedTemplateParam = ParserInjectedTemplateParam;
  using LexicalScopeState = ParserLexicalScopeState;
  using RecordBodyState = ParserRecordBodyState;
  using RecordMemberRecoveryResult = ParserRecordMemberRecoveryResult;
  using ParseContextFrame = ParserParseContextFrame;
  using ParseFailure = ParserParseFailure;
  using ParseDebugEvent = ParserParseDebugEvent;
  using ParseContextGuard = ParserParseContextGuard;

  enum ParseDebugChannel : unsigned {
    ParseDebugNone = 0,
    ParseDebugGeneral = 1u << 0,
    ParseDebugTentative = 1u << 1,
    ParseDebugInjected = 1u << 2,
    ParseDebugAll = ParseDebugGeneral | ParseDebugTentative | ParseDebugInjected,
  };

  // ── parser-local state snapshots / guard facades ─────────────────────────
  using TentativeParseMode = ParserTentativeParseMode;
  using TentativeTextRefKind = ParserTentativeTextRefKind;
  using TentativeTextRef = ParserTentativeTextRef;
  using ParserLiteSnapshot = c4c::ParserLiteSnapshot;
  using ParserSnapshot = c4c::ParserSnapshot;
  using TentativeParseStats = ParserTentativeParseStats;
  using TentativeParseGuard = ParserTentativeParseGuard;
  using TentativeParseGuardLite = ParserTentativeParseGuardLite;
  using LocalVarBindingSuppressionGuard =
      ParserLocalVarBindingSuppressionGuard;
  using RecordTemplatePreludeGuard = ParserRecordTemplatePreludeGuard;
  using TemplateDeclarationPreludeGuard =
      ParserTemplateDeclarationPreludeGuard;

  ParserLiteSnapshot save_lite_state() const;
  void restore_lite_state(const ParserLiteSnapshot& snap);
  ParserSnapshot save_state() const;
  void restore_state(const ParserSnapshot& snap);
  ParserSymbolTables& parser_symbol_tables();
  const ParserSymbolTables& parser_symbol_tables() const;

  // ── public parse entry / facade lifecycle ────────────────────────────────
  // All members public (required by project coding constraints).
  explicit Parser(std::vector<Token> tokens, Arena& arena,
                  TextTable* token_texts,
                  FileTable* token_files,
                  SourceProfile source_profile = SourceProfile::C,
                  const std::string& source_file = "<input>");
  ~Parser();
  Parser(const Parser&) = delete;
  Parser& operator=(const Parser&) = delete;
  Parser(Parser&&) = delete;
  Parser& operator=(Parser&&) = delete;

  // Parse the entire token stream and return a NK_PROGRAM node.
  Node* parse();

  // ── parser-local state references: opaque implementation ownership ───────
  std::unique_ptr<ParserImpl> impl_;

  // ── parser-local state references: core token cursor and arena ────────────
  using TokenMutation = ParserTokenMutation;
  ParserCoreInputState& core_input_state_;
  std::vector<Token>& tokens_;
  int& pos_;
  Arena& arena_;

  // ── parser-local state references: shared lookup tables ──────────────────
  ParserSharedLookupState& shared_lookup_state_;

  // ── parser-local state references: name / binding tables ─────────────────
  ParserBindingState& binding_state_;

  // ── parser-local state references: record / enum definition tables ───────
  ParserDefinitionState& definition_state_;

  // ── parser-local state references: template metadata and active scopes ───
  ParserTemplateState& template_state_;

  // ── parser-local state references: lexical binding scopes ────────────────
  ParserLexicalScopeState& lexical_scope_state_;

  // ── parser-local state references: active parse context ──────────────────
  ParserActiveContextState& active_context_state_;

  // ── parser-local state references: namespace / using-directive tables ────
  ParserNamespaceState& namespace_state_;

  // ── parser-local state references: diagnostic and recovery state ─────────
  ParserDiagnosticState& diagnostic_state_;

  // ── parser-local state references: pragma state ──────────────────────────
  ParserPragmaState& pragma_state_;

  // ── diagnostics / debug hooks: parser tracing and failure reporting ──────
  void set_parser_debug(bool enabled);
  void set_parser_debug_channels(unsigned channels);
  bool had_parse_error() const;
  bool parser_debug_enabled() const;
  bool parse_debug_event_visible(const char* kind) const;
  void clear_parse_debug_state();
  void reset_parse_debug_progress();

  // Lexical parse-context stack used for debug traces and failure stacks.
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

  // ── lookup / binding helpers: token cursor and shared token utilities ────
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

  // ── lookup / binding helpers: parser mode and identifier classification ──
  bool is_type_start() const;            // can current token start a type?
  bool can_start_parameter_type() const;
  int classify_visible_value_or_type_head(int pos, int* after_pos = nullptr);
  int classify_visible_value_or_type_starter(int pos, int* after_pos = nullptr);
  bool looks_like_unresolved_identifier_type_head(int pos) const;
  bool looks_like_unresolved_parenthesized_parameter_type_head(int pos) const;
  TextId parser_text_id_for_token(TextId token_text_id,
                                  std::string_view fallback = {}) const;
  TextId find_parser_text_id(std::string_view text) const;
  std::string_view parser_text(TextId text_id,
                               std::string_view fallback = {}) const;
  void clear_current_struct_tag();
  void set_current_struct_tag(std::string_view tag);
  std::string_view current_struct_tag_text() const;
  void clear_last_resolved_typedef();
  void set_last_resolved_typedef(std::string_view name);
  void clear_last_using_alias_name();
  void set_last_using_alias_name(const QualifiedNameKey& key);
  std::string_view last_using_alias_name_text() const;
  const FnPtrTypedefInfo* find_typedef_fn_ptr_info(TextId text_id) const;
  const FnPtrTypedefInfo* find_current_typedef_fn_ptr_info() const;
  std::string_view token_spelling(const Token& token) const;
  void set_parser_owned_spelling(Token& token, std::string_view spelling);
  Token make_injected_token(const Token& seed, TokenKind kind,
                            std::string_view spelling);
  int current_token_index() const;
  int token_count() const;
  TokenKind token_kind(int index) const;
  bool token_kind_at(int index, TokenKind kind) const;
  bool parse_injected_base_type(std::vector<Token> tokens,
                                const char* debug_reason,
                                TypeSpec* out_resolved = nullptr);
  bool has_defined_struct_tag(std::string_view tag) const;
  bool eval_const_int_with_parser_tables(Node* n, long long* out) const;

  // Diagnostics / debug / testing hooks: white-box parser state probes.
  void replace_token_stream_for_testing(std::vector<Token> tokens, int pos = 0);
  int token_cursor_for_testing() const;
  const Token& token_at_for_testing(int index) const;
  bool parser_symbols_use_text_table_for_testing(const TextTable* texts) const;
  size_t parser_symbol_count_for_testing() const;
  void register_concept_name_for_testing(TextId name_text_id);
  void register_struct_definition_for_testing(std::string tag, Node* definition);
  void register_using_value_alias_for_testing(
      int context_id, TextId alias_text_id, const QualifiedNameKey& target_key,
      std::string compatibility_name);
  bool replace_using_value_alias_compatibility_name_for_testing(
      int context_id, TextId alias_text_id, std::string compatibility_name);
  void register_alias_template_info_for_testing(
      const QualifiedNameKey& key, const ParserAliasTemplateInfo& info);
  bool has_last_using_alias_name_text_id_for_testing() const;
  void replace_last_using_alias_name_fallback_for_testing(std::string name);

  // Lookup / binding helpers: parser symbol identity and typedef tables.
  std::string_view last_resolved_typedef_text() const;
  SymbolId symbol_id_for_token_text(TextId token_text_id,
                                    std::string_view fallback = {});
  SymbolId symbol_id_for_token(const Token& token);
  void populate_qualified_name_symbol_ids(QualifiedNameRef* name);
  std::string_view symbol_spelling(SymbolId id) const;
  bool has_typedef_name(std::string_view name) const;
  bool is_typedef_name(TextId name_text_id, std::string_view name) const;
  bool has_typedef_type(std::string_view name) const;
  const TypeSpec* find_typedef_type(TextId name_text_id,
                                    std::string_view fallback_name) const;
  const TypeSpec* find_typedef_type(std::string_view name) const;
  const TypeSpec* find_typedef_type(const QualifiedNameKey& key,
                                    std::string_view fallback_name) const;
  bool has_structured_typedef_type(const QualifiedNameKey& key) const;
  const TypeSpec* find_structured_typedef_type(
      const QualifiedNameKey& key) const;
  const ParserAliasTemplateInfo* find_alias_template_info(
      const QualifiedNameKey& key) const;
  bool has_visible_typedef_type(TextId name_text_id, std::string_view name) const;
  // String-only visible typedef helpers are compatibility bridges for tag/ref
  // strings. Parser-owned token paths should pass TextId identity above.
  bool has_visible_typedef_type(std::string_view name) const;
  const TypeSpec* find_visible_typedef_type(TextId name_text_id,
                                            std::string_view name) const;
  // String-only visible typedef lookup delegates to the TextId-aware path after
  // a parser text lookup; use it only when no originating TextId is available.
  const TypeSpec* find_visible_typedef_type(std::string_view name) const;
  void push_local_binding_scope();
  bool pop_local_binding_scope();
  bool has_local_binding_scope() const;
  void bind_local_typedef(TextId name_text_id, const TypeSpec& type,
                          bool is_user_typedef = false);
  void bind_local_value(TextId name_text_id, const TypeSpec& type);
  const TypeSpec* find_local_visible_typedef_type(TextId name_text_id) const;
  const TypeSpec* find_local_visible_var_type(TextId name_text_id) const;
  bool has_local_visible_user_typedef(TextId name_text_id) const;
  TypeSpec resolve_typedef_type_chain(TypeSpec ts) const;
  TypeSpec resolve_struct_like_typedef_type(TypeSpec ts) const;
  bool are_types_compatible(const TypeSpec& lhs, const TypeSpec& rhs) const;
  bool resolves_to_record_ctor_type(TypeSpec ts) const;
  bool is_user_typedef_name(const std::string& name) const;
  bool has_conflicting_user_typedef_binding(const std::string& name,
                                            const TypeSpec& type) const;
  void register_typedef_name(TextId name_text_id, std::string_view name,
                             bool is_user_typedef);
  void register_typedef_name(const std::string& name, bool is_user_typedef);
  void register_typedef_binding(TextId name_text_id, std::string_view name,
                                const TypeSpec& type, bool is_user_typedef);
  void register_typedef_binding(const std::string& name, const TypeSpec& type,
                                bool is_user_typedef);
  void unregister_typedef_binding(TextId name_text_id,
                                  std::string_view fallback_name);
  void unregister_typedef_binding(const std::string& name);
  void register_synthesized_typedef_binding(TextId name_text_id,
                                            std::string_view name);
  void register_synthesized_typedef_binding(const std::string& name);
  void register_tag_type_binding(const std::string& name, TypeBase base,
                                 const char* tag,
                                 TypeBase enum_underlying_base = TB_VOID);
  void cache_typedef_type(const std::string& name, const TypeSpec& type);
  void register_struct_member_typedef_binding(std::string_view owner_name,
                                              std::string_view member_name,
                                              const TypeSpec& type);
  void register_struct_member_typedef_binding(const std::string& scoped_name,
                                              const TypeSpec& type);
  void register_structured_typedef_binding_in_context(
      int context_id, TextId name_text_id, std::string_view fallback_name,
      const TypeSpec& type);
  bool has_var_type(const std::string& name) const;
  const TypeSpec* find_var_type(TextId name_text_id,
                                std::string_view fallback_name) const;
  const TypeSpec* find_var_type(const std::string& name) const;
  const TypeSpec* find_var_type(const QualifiedNameKey& key,
                                std::string_view fallback_name) const;
  bool has_structured_var_type(const QualifiedNameKey& key) const;
  const TypeSpec* find_structured_var_type(
      const QualifiedNameKey& key) const;
  const TypeSpec* find_visible_var_type(TextId name_text_id,
                                        std::string_view name) const;
  // String-only visible value lookup is a compatibility bridge for projection
  // strings. Token-backed semantic callers should use the TextId overload.
  const TypeSpec* find_visible_var_type(const std::string& name) const;
  void register_var_type_binding(TextId name_text_id, std::string_view name,
                                 const TypeSpec& type);
  void register_var_type_binding(const std::string& name, const TypeSpec& type);
  void register_structured_var_type_binding(const QualifiedNameKey& key,
                                            const TypeSpec& type);
  void register_structured_var_type_binding_in_context(
      int context_id, TextId name_text_id, std::string_view fallback_name,
      const TypeSpec& type);
  // Public string-facing compatibility bridge; parser-owned callers should use
  // the QualifiedNameKey/TextId overloads when that identity is available.
  bool has_known_fn_name(const std::string& name) const;
  bool has_known_fn_name(const QualifiedNameKey& key) const;
  bool has_known_fn_name_compatibility_fallback(
      std::string_view rendered_name) const;
  // Public string-facing compatibility bridge; parser-owned callers should use
  // the QualifiedNameKey/TextId overloads when that identity is available.
  void register_known_fn_name(const std::string& name);
  void register_known_fn_name(const QualifiedNameKey& key);
  void register_known_fn_name_compatibility_fallback(
      std::string_view rendered_name);
  bool register_known_fn_name_in_context(int context_id, TextId name_text_id,
                                         std::string_view fallback_name);
  bool has_structured_concept_name(const QualifiedNameKey& key) const;
  void register_concept_name_in_context(int context_id, TextId name_text_id,
                                        std::string_view fallback_name);
  bool is_typedef_name(std::string_view s) const;
  bool is_cpp_mode() const;

  // ── lookup / binding helpers: namespace resolution and qualified names ───
  void refresh_current_namespace_bridge();
  int current_namespace_context_id() const;
  int ensure_named_namespace_context(int parent_id, TextId text_id,
                                     const std::string& name);
  int create_anonymous_namespace_context(int parent_id);
  int find_named_namespace_child(int parent_id, TextId text_id) const;

  // Namespace scope stack: pushes the active lookup frame for nested scopes.
  void push_namespace_context(int context_id);
  void pop_namespace_context();
  std::string canonical_name_in_context(int context_id, const std::string& name) const;
  // Compatibility/debug bridge for legacy string-keyed namespace consumers.
  std::string compatibility_namespace_name_in_context(
      int context_id, TextId name_text_id, std::string_view fallback_name) const;
  QualifiedNameKey known_fn_name_key(int context_id, TextId name_text_id,
                                     std::string_view name) const;
  QualifiedNameKey intern_semantic_name_key(std::string_view name);
  QualifiedNameKey known_fn_name_key_in_context(
      int context_id, TextId name_text_id,
      std::string_view fallback_name) const;
  QualifiedNameKey current_record_member_name_key(
      TextId member_text_id, std::string_view fallback_member_name) const;
  QualifiedNameKey struct_typedef_key_in_context(
      int context_id, TextId name_text_id,
      std::string_view fallback_name) const;
  QualifiedNameKey alias_template_key_in_context(
      int context_id, TextId name_text_id,
      std::string_view fallback_name) const;
  const ParserAliasTemplateInfo* find_alias_template_info_in_context(
      int context_id, TextId name_text_id,
      std::string_view fallback_name) const;
  QualifiedNameKey qualified_name_key(const QualifiedNameRef& name);
  std::string bridge_name_in_context(int context_id, TextId name_text_id,
                                     std::string_view fallback_name) const;
  std::string qualified_name_text(const QualifiedNameRef& name,
                                  bool include_global_prefix = true) const;
  int resolve_namespace_context(const QualifiedNameRef& name) const;
  int resolve_namespace_name(const QualifiedNameRef& name) const;
  VisibleNameResult resolve_qualified_value(
      const QualifiedNameRef& name) const;
  std::string resolve_qualified_value_name(const QualifiedNameRef& name) const;
  VisibleNameResult resolve_qualified_type(
      const QualifiedNameRef& name) const;
  std::string resolve_qualified_type_name(const QualifiedNameRef& name) const;
  bool lookup_using_value_alias(int context_id, TextId name_text_id,
                                std::string_view fallback_name,
                                VisibleNameResult* resolved) const;
  // String output overloads project structured lookup results to spelling.
  // They must not be used as independent semantic authority.
  bool lookup_using_value_alias(int context_id, TextId name_text_id,
                                std::string_view fallback_name,
                                std::string* resolved) const;
  bool lookup_value_in_context(int context_id, TextId name_text_id,
                               std::string_view name,
                               VisibleNameResult* resolved) const;
  // Projection bridge over the VisibleNameResult overload.
  bool lookup_value_in_context(int context_id, TextId name_text_id,
                               std::string_view name,
                               std::string* resolved) const;
  bool lookup_type_in_context(int context_id, TextId name_text_id,
                              std::string_view name,
                              VisibleNameResult* resolved) const;
  // Projection bridge over the VisibleNameResult overload.
  bool lookup_type_in_context(int context_id, TextId name_text_id,
                              std::string_view name,
                              std::string* resolved) const;
  bool lookup_concept_in_context(int context_id, TextId name_text_id,
                                 std::string_view name,
                                 VisibleNameResult* resolved) const;
  // Projection bridge over the VisibleNameResult overload.
  bool lookup_concept_in_context(int context_id, TextId name_text_id,
                                 std::string_view name,
                                 std::string* resolved) const;
  std::string qualify_name(TextId name_text_id, std::string_view name) const;
  std::string qualify_name(const std::string& name) const;
  const char* qualify_name_arena(TextId name_text_id, const char* name);
  const char* qualify_name_arena(const char* name);
  VisibleNameResult resolve_visible_value(TextId name_text_id,
                                          std::string_view name) const;
  // String-only visible-name resolution is a compatibility entry point for
  // callers that only have spelling; token/semantic callers should pass TextId.
  VisibleNameResult resolve_visible_value(std::string_view name) const;
  std::string resolve_visible_value_name(TextId name_text_id,
                                         std::string_view name) const;
  // Final spelling projection over resolve_visible_value(...).
  std::string resolve_visible_value_name(const std::string& name) const;
  VisibleNameResult resolve_visible_type(TextId name_text_id,
                                         std::string_view name) const;
  // String-only visible type resolution is compatibility/fallback only; prefer
  // the TextId overload where parser-owned identity exists.
  VisibleNameResult resolve_visible_type(std::string_view name) const;
  std::string visible_name_spelling(
      const VisibleNameResult& result) const;
  std::string resolve_visible_type_name(TextId name_text_id,
                                        std::string_view name) const;
  // Final spelling projection over resolve_visible_type(...).
  std::string resolve_visible_type_name(std::string_view name) const;
  VisibleNameResult resolve_visible_concept(TextId name_text_id,
                                            std::string_view name) const;
  // String-only visible concept resolution is compatibility/fallback only.
  VisibleNameResult resolve_visible_concept(std::string_view name) const;
  std::string resolve_visible_concept_name(TextId name_text_id,
                                           std::string_view name) const;
  // Final spelling projection over resolve_visible_concept(...).
  std::string resolve_visible_concept_name(const std::string& name) const;
  bool is_concept_name(const std::string& name) const;
  bool peek_qualified_name(QualifiedNameRef* out, bool allow_global = true) const;
  bool consume_template_parameter_type_start(bool allow_typename_keyword = true);
  QualifiedNameRef parse_qualified_name(bool allow_global = true);
  void apply_qualified_name(Node* node, const QualifiedNameRef& qn,
                            const char* resolved_name = nullptr);
  void apply_decl_namespace(Node* node, int context_id, const char* unqualified_name);

  // ── AST handoff helpers: generic skipping and attributes ─────────────────
  void skip_attributes();
  void skip_exception_spec();
  void parse_attributes(TypeSpec* ts);
  void skip_asm();
  void skip_paren_group();
  void skip_brace_group();

  // ── lookup / binding helpers: template arguments and instantiation ───────
  // Evaluate a deferred NTTP default expression for a template parameter.
  // Returns true if evaluation succeeded and writes the result to *out.
  bool eval_deferred_nttp_default(
      const QualifiedNameKey& template_key,
      std::string_view rendered_template_name,
      int param_idx,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings,
      long long* out);
  bool eval_deferred_nttp_default(
      const std::string& tpl_name, int param_idx,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings,
      long long* out);
  void cache_nttp_default_expr_tokens(
      const QualifiedNameKey& template_key,
      std::string_view legacy_template_name,
      int param_idx,
      std::vector<Token> toks);
  bool eval_deferred_nttp_expr_tokens(
      const std::string& tpl_name,
      const std::vector<Token>& toks,
      const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
      const std::vector<std::pair<std::string, long long>>& nttp_bindings,
      long long* out);
  bool has_template_struct_primary(int context_id, TextId name_text_id,
                                   std::string_view fallback_name) const;
  bool has_template_struct_primary(std::string_view name) const;
  bool has_template_struct_primary(const QualifiedNameRef& name) const;
  Node* find_template_struct_primary(int context_id, TextId name_text_id,
                                     std::string_view fallback_name) const;
  Node* find_template_struct_primary(const std::string& name) const;
  Node* find_template_struct_primary(const QualifiedNameRef& name) const;
  const std::vector<Node*>* find_template_struct_specializations(
      int context_id, TextId name_text_id, std::string_view fallback_name,
      const Node* primary_tpl = nullptr) const;
  const std::vector<Node*>* find_template_struct_specializations(
      const Node* primary_tpl) const;
  const std::vector<Node*>* find_template_struct_specializations(
      const QualifiedNameRef& name, const Node* primary_tpl = nullptr) const;
  const Node* select_template_struct_pattern_for_args(
      const std::vector<TemplateArgParseResult>& args,
      const Node* primary_tpl,
      const std::vector<Node*>* specializations,
      std::vector<std::pair<std::string, TypeSpec>>* out_type_bindings,
      std::vector<std::pair<std::string, long long>>* out_nttp_bindings) const;
  void register_template_struct_primary(int context_id, TextId name_text_id,
                                        std::string_view fallback_name,
                                        Node* node);
  void register_template_struct_primary(const std::string& name, Node* node);
  void register_template_struct_specialization(
      int context_id, TextId primary_name_text_id,
      std::string_view primary_name, Node* node);
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

  // Template scope stack: tracks active template parameter visibility.
  void push_template_scope(TemplateScopeKind kind,
                           const std::vector<TemplateScopeParam>& params);
  void pop_template_scope();
  // Check if a name is a type parameter in any active template scope frame.
  // Walks the stack from innermost to outermost.
  bool is_template_scope_type_param(TextId name_text_id,
                                    std::string_view name) const;
  bool is_template_scope_type_param(std::string_view name) const;

  // ── AST handoff producers: type spelling and type-specifier parsing ──────
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
  bool parse_dependent_typename_specifier(std::string* out_name = nullptr);

  // Parse a complete type specifier (base type + qualifiers).
  // ptr_level and array_size are NOT parsed here (those are in the declarator).
  TypeSpec parse_base_type();
  // Parse a full type-name (type + declarator with no name): e.g. for sizeof.
  TypeSpec parse_type_name();

  // ── AST handoff producers: declarator parsing ────────────────────────────
  // Modifies ts in place. Returns the declared name via out_name.
  // Function-pointer metadata is optionally surfaced through the out_* params.
  void parse_declarator(TypeSpec& ts, const char** out_name,
                        Node*** out_fn_ptr_params = nullptr,
                        int* out_n_fn_ptr_params = nullptr,
                        bool* out_fn_ptr_variadic = nullptr,
                        bool* out_is_parameter_pack = nullptr,
                        Node*** out_ret_fn_ptr_params = nullptr,
                        int* out_n_ret_fn_ptr_params = nullptr,
                        bool* out_ret_fn_ptr_variadic = nullptr,
                        TextId* out_name_text_id = nullptr);
  bool parse_operator_declarator_name(std::string* out_name);

  // ── AST handoff builders: AST node construction ──────────────────────────
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

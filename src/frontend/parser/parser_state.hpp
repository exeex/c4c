#pragma once

#include <chrono>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "token.hpp"
#include "source_profile.hpp"
#include "../../shared/local_name_table.hpp"
#include "../../shared/text_id_table.hpp"

namespace c4c {

class Parser;
class Arena;

using ParserSymbolId = uint32_t;
constexpr ParserSymbolId kInvalidParserSymbol = 0;

struct ParserSymbolTable {
  explicit ParserSymbolTable(TextTable* texts = nullptr) : texts_(texts) {}

  void attach_text_table(TextTable* texts) { texts_ = texts; }

  ParserSymbolId find_identifier(TextId text_id) const {
    if (!texts_ || text_id == kInvalidText) return kInvalidParserSymbol;
    return symbol_ids_.find(text_id);
  }

  ParserSymbolId find_identifier(std::string_view text) const {
    if (!texts_ || text.empty()) return kInvalidParserSymbol;
    return find_identifier(texts_->find(text));
  }

  ParserSymbolId intern_identifier(TextId text_id) {
    if (!texts_ || text_id == kInvalidText) return kInvalidParserSymbol;
    return symbol_ids_.intern(text_id);
  }

  ParserSymbolId intern_identifier(std::string_view text) {
    if (!texts_) return kInvalidParserSymbol;
    return intern_identifier(texts_->intern(text));
  }

  TextId text_id(ParserSymbolId id) const {
    const TextId* stored = symbol_ids_.lookup(id);
    return stored ? *stored : kInvalidText;
  }

  std::string_view spelling(ParserSymbolId id) const {
    if (!texts_) return {};
    return texts_->lookup(text_id(id));
  }

  size_t size() const { return symbol_ids_.size(); }

  TextTable* texts_ = nullptr;
  KeyIdTable<ParserSymbolId, kInvalidParserSymbol, TextId> symbol_ids_;
};

struct ParserNameTables {
  ParserSymbolId find_identifier(TextId text_id) const {
    return symbols ? symbols->find_identifier(text_id) : kInvalidParserSymbol;
  }

  ParserSymbolId find_identifier(std::string_view text) const {
    return symbols ? symbols->find_identifier(text) : kInvalidParserSymbol;
  }

  ParserSymbolId intern_identifier(TextId text_id) {
    return symbols ? symbols->intern_identifier(text_id) : kInvalidParserSymbol;
  }

  ParserSymbolId intern_identifier(std::string_view text) {
    return symbols ? symbols->intern_identifier(text) : kInvalidParserSymbol;
  }

  bool is_typedef(ParserSymbolId id) const {
    return id != kInvalidParserSymbol && typedefs.count(id) != 0;
  }

  bool has_typedef_type(ParserSymbolId id) const {
    return id != kInvalidParserSymbol && typedef_types.count(id) != 0;
  }

  const TypeSpec* lookup_typedef_type(ParserSymbolId id) const {
    const auto it = typedef_types.find(id);
    return it == typedef_types.end() ? nullptr : &it->second;
  }

  ParserSymbolTable* symbols = nullptr;
  std::unordered_set<ParserSymbolId> typedefs;
  std::unordered_set<ParserSymbolId> user_typedefs;
  std::unordered_map<ParserSymbolId, TypeSpec> typedef_types;
  std::unordered_map<ParserSymbolId, TypeSpec> var_types;
};

struct ParserTokenMutation {
  int pos = -1;
  Token token;
};

struct ParserFnPtrTypedefInfo {
  Node** params = nullptr;
  int n_params = 0;
  bool variadic = false;
};

struct ParserNamespaceContext {
  int id = 0;
  int parent_id = -1;
  bool is_anonymous = false;
  TextId text_id = kInvalidText;
  const char* display_name = nullptr;
  const char* canonical_name = nullptr;
};

struct ParserQualifiedNameRef {
  bool is_global_qualified = false;
  std::vector<std::string> qualifier_segments;
  std::vector<TextId> qualifier_text_ids;
  std::vector<ParserSymbolId> qualifier_symbol_ids;
  std::string base_name;
  TextId base_text_id = kInvalidText;
  ParserSymbolId base_symbol_id = kInvalidParserSymbol;

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

struct ParserTemplateArgParseResult {
  bool is_value = false;
  TypeSpec type{};
  long long value = 0;
  const char* nttp_name = nullptr;
  Node* expr = nullptr;
};

struct ParserCoreInputState {
  std::vector<Token> tokens;
  std::vector<ParserTokenMutation> token_mutations;
  int pos = 0;
  Arena& arena;
  SourceProfile source_profile = SourceProfile::C;
  std::string source_file;

  ParserCoreInputState(std::vector<Token> tokens_in, Arena& arena_in,
                       SourceProfile source_profile_in,
                       std::string source_file_in)
      : tokens(std::move(tokens_in)),
        arena(arena_in),
        source_profile(source_profile_in),
        source_file(std::move(source_file_in)) {}
};

struct ParserSharedLookupState {
  TextTable* token_texts = nullptr;
  FileTable* token_files = nullptr;
  ParserSymbolTable parser_symbols;
  ParserNameTables parser_name_tables;

  ParserSharedLookupState(TextTable* token_texts_in = nullptr,
                          FileTable* token_files_in = nullptr)
      : token_texts(token_texts_in),
        token_files(token_files_in),
        parser_symbols(token_texts_in) {
    parser_name_tables.symbols = &parser_symbols;
  }

  void attach_text_table(TextTable* texts) {
    token_texts = texts;
    parser_symbols.attach_text_table(texts);
  }

  void sync_symbol_tables() { parser_name_tables.symbols = &parser_symbols; }
};

enum class ParserTemplateScopeKind {
  EnclosingClass,
  MemberTemplate,
  FreeFunctionTemplate,
};

struct ParserTemplateScopeParam {
  TextId name_text_id = kInvalidText;
  const char* name = nullptr;
  bool is_nttp = false;
};

struct ParserTemplateScopeFrame {
  ParserTemplateScopeKind kind = ParserTemplateScopeKind::FreeFunctionTemplate;
  std::vector<ParserTemplateScopeParam> params;
  std::string owner_struct_tag;
};

struct ParserRecordBodyState {
  std::vector<Node*> fields;
  std::vector<Node*> methods;
  std::vector<const char*> member_typedef_names;
  std::vector<TypeSpec> member_typedef_types;
};

enum class ParserRecordMemberRecoveryResult {
  Failed,
  SyncedAtSemicolon,
  StoppedAtNextMember,
  StoppedAtRBrace,
};

struct ParserParseContextFrame {
  std::string function_name;
  int token_index = -1;
};

struct ParserParseFailure {
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

struct ParserParseDebugEvent {
  std::string kind;
  std::string function_name;
  std::string detail;
  int token_index = -1;
  int line = 1;
  int column = 1;
};

enum class ParserTentativeParseMode {
  Heavy,
  Lite,
};

enum class ParserTentativeTextRefKind {
  None,
  TextId,
};

struct ParserTentativeTextRef {
  ParserTentativeTextRefKind kind = ParserTentativeTextRefKind::None;
  TextId text_id = kInvalidText;
};

struct ParserAliasTemplateInfo {
  std::vector<const char*> param_names;
  std::vector<bool> param_is_nttp;
  std::vector<bool> param_is_pack;
  std::vector<bool> param_has_default;
  std::vector<TypeSpec> param_default_types;
  std::vector<long long> param_default_values;
  TypeSpec aliased_type;
};

using ParserEnumConstTable = std::unordered_map<TextId, long long>;
using ParserConstIntBindingTable = std::unordered_map<TextId, long long>;

struct ParserBindingState {
  std::unordered_set<TextId> concept_name_text_ids;
  std::set<std::string> concept_names;
  std::unordered_map<TextId, ParserFnPtrTypedefInfo> typedef_fn_ptr_info;
  ParserEnumConstTable enum_consts;
  ParserConstIntBindingTable const_int_bindings;
  std::set<std::string> known_fn_names;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
  std::unordered_map<std::string, TypeSpec> struct_typedefs;
};

struct ParserDefinitionState {
  std::vector<Node*> struct_defs;
  int anon_counter = 0;
  std::set<std::string> defined_struct_tags;
  std::unordered_map<std::string, Node*> struct_tag_def_map;
  Node* last_enum_def = nullptr;
};

// Template-definition and template-scope state.
// - template_struct_* tables track primary templates and specializations.
// - template_scope_stack is the push/pop surface for active template params.
struct ParserTemplateState {
  std::unordered_map<std::string, Node*> template_struct_defs;
  std::unordered_map<std::string, std::vector<Node*>>
      template_struct_specializations;
  std::set<std::string> instantiated_template_struct_keys;
  std::unordered_map<std::string, std::vector<Token>>
      nttp_default_expr_tokens;
  std::unordered_map<std::string, ParserAliasTemplateInfo> alias_template_info;
  std::vector<ParserTemplateScopeFrame> template_scope_stack;
};

// Explicit parser-local lexical binding scopes. These stay separate from
// namespace traversal state so block-local visibility can migrate to
// TextId-native lookup without changing qualified lookup behavior.
struct ParserLexicalScopeState {
  using LocalTypeTable = LocalNameTable<LocalNameKey, TypeSpec>;

  void push_scope() {
    visible_typedef_types.push_scope();
    visible_value_types.push_scope();
  }

  bool pop_scope() {
    const bool popped_types = visible_typedef_types.pop_scope();
    const bool popped_values = visible_value_types.pop_scope();
    return popped_types || popped_values;
  }

  [[nodiscard]] bool empty() const { return visible_typedef_types.empty(); }

  [[nodiscard]] size_t scope_depth() const {
    return visible_typedef_types.scope_depth();
  }

  LocalTypeTable visible_typedef_types;
  LocalTypeTable visible_value_types;
};

// Transient parser context that moves with lexical/state transitions.
// These fields are restored by snapshots or cleared by the matching pop path.
struct ParserActiveContextState {
  std::string last_using_alias_name;
  TextId last_using_alias_name_text_id = kInvalidText;
  std::string last_resolved_typedef;
  TextId last_resolved_typedef_text_id = kInvalidText;
  std::string current_struct_tag;
  TextId current_struct_tag_text_id = kInvalidText;
  bool parsing_top_level_context = false;
  bool parsing_explicit_specialization = false;
  int template_arg_expr_depth = 0;
  bool suppress_local_var_bindings = false;
};

// Namespace resolution stack and cached lookup scopes.
// namespace_stack is the push/pop chain for nested namespace contexts.
struct ParserNamespaceState {
  std::string current_namespace;
  std::vector<ParserNamespaceContext> namespace_contexts;
  std::vector<int> namespace_stack;
  std::unordered_map<int, std::unordered_map<TextId, int>>
      named_namespace_children;
  std::unordered_map<int, std::vector<int>> anonymous_namespace_children;
  std::unordered_map<int, std::unordered_map<TextId, std::string>>
      using_value_aliases;
  std::unordered_map<int, std::vector<int>> using_namespace_contexts;
};

// Lite tentative snapshot: cursor plus the minimum rollback-visible context.
struct ParserLiteSnapshot {
  int pos = 0;
  ParserTentativeTextRef last_resolved_typedef;
  int template_arg_expr_depth = 0;
  size_t token_mutation_count = 0;
};

// Full tentative snapshot: lite snapshot plus bindings that heavy rollback restores.
struct ParserSnapshot {
  ParserLiteSnapshot lite;
  ParserNameTables symbol_tables;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
  ParserLexicalScopeState lexical_scope_state;
};

// Heavy/lite tentative parse accounting.
struct ParserTentativeParseStats {
  int heavy_enter = 0;
  int heavy_commit = 0;
  int heavy_rollback = 0;
  int lite_enter = 0;
  int lite_commit = 0;
  int lite_rollback = 0;
};

// Diagnostic trace stack and tentative-parse counters.
struct ParserDiagnosticState {
  bool had_error = false;
  int parse_error_count = 0;
  int max_parse_errors = 20;
  int max_no_progress_steps = 8;
  unsigned parser_debug_channels = 0;
  int max_parse_debug_events = 256;
  int parse_debug_progress_interval_ms = 1000;
  std::vector<ParserParseContextFrame> parse_context_stack;
  std::vector<ParserParseDebugEvent> parse_debug_events;
  ParserParseFailure best_parse_failure;
  int best_parse_stack_token_index = -1;
  std::vector<std::string> best_parse_stack_trace;
  ParserTentativeParseStats tentative_parse_stats;
  std::chrono::steady_clock::time_point parse_debug_started_at{};
  std::chrono::steady_clock::time_point parse_debug_last_progress_at{};
};

// Nested pragma state: each stack mirrors one push/pop pragma scope.
struct ParserPragmaState {
  int pack_alignment = 0;
  std::vector<int> pack_stack;
  uint8_t visibility = 0;
  std::vector<uint8_t> visibility_stack;
  ExecutionDomain execution_domain = ExecutionDomain::Host;
};

// Debug parse-context guard: push on construction, pop on destruction.
struct ParserParseContextGuard {
  Parser* parser = nullptr;
  ParserParseContextGuard(Parser* parser, const char* function_name);
  ~ParserParseContextGuard();
};

// Heavy tentative parse guard: save full state, restore on rollback.
struct ParserTentativeParseGuard {
  Parser& parser;
  ParserSnapshot snapshot;
  int start_pos = -1;
  bool committed = false;

  explicit ParserTentativeParseGuard(Parser& p);
  ~ParserTentativeParseGuard();
  void commit();
};

// Lite tentative parse guard: save cursor-level state only.
struct ParserTentativeParseGuardLite {
  Parser& parser;
  ParserLiteSnapshot snapshot;
  int start_pos = -1;
  bool committed = false;

  explicit ParserTentativeParseGuardLite(Parser& p);
  ~ParserTentativeParseGuardLite();
  void commit();
};

// Temporarily suppress local-variable binding while a nested scope is active.
struct ParserLocalVarBindingSuppressionGuard {
  Parser& parser;
  bool old = false;

  explicit ParserLocalVarBindingSuppressionGuard(Parser& p);
  ~ParserLocalVarBindingSuppressionGuard();
};

// Record prelude guard: inject template params, then unwind on scope exit.
struct ParserRecordTemplatePreludeGuard {
  Parser* parser = nullptr;
  std::vector<std::string> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserRecordTemplatePreludeGuard(Parser* p);
  ~ParserRecordTemplatePreludeGuard();
};

// Template-declaration prelude guard: same push/pop shape as record preludes.
struct ParserTemplateDeclarationPreludeGuard {
  Parser* parser = nullptr;
  std::vector<std::string> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserTemplateDeclarationPreludeGuard(Parser* p);
  ~ParserTemplateDeclarationPreludeGuard();
};

}  // namespace c4c

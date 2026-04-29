#pragma once

#include <chrono>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../parser_types.hpp"
#include "../../lexer/token.hpp"
#include "../../source_profile.hpp"
#include "../../../shared/local_name_table.hpp"
#include "../../../shared/qualified_name_table.hpp"
#include "../../../shared/text_id_table.hpp"

namespace c4c {

class Parser;
class Arena;

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
  NamePathTable parser_name_paths;

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

struct ParserBindingState {
  std::unordered_set<TextId> concept_name_text_ids;
  std::unordered_set<QualifiedNameKey, QualifiedNameKeyHash>
      concept_qualified_names;
  std::unordered_map<TextId, ParserFnPtrTypedefInfo> typedef_fn_ptr_info;
  ParserEnumConstTable enum_consts;
  ParserConstIntBindingTable const_int_bindings;
  std::unordered_set<QualifiedNameKey, QualifiedNameKeyHash> known_fn_names;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
  std::unordered_map<QualifiedNameKey, TypeSpec, QualifiedNameKeyHash>
      struct_typedefs;
  std::unordered_map<QualifiedNameKey, TypeSpec, QualifiedNameKeyHash>
      value_bindings;
};

struct ParserDefinitionState {
  std::vector<Node*> struct_defs;
  int anon_counter = 0;
  std::set<std::string> defined_struct_tags;
  // Rendered-tag compatibility mirror. Semantic record lookup should prefer
  // TypeSpec::record_def via resolve_record_type_spec() before using this map.
  std::unordered_map<std::string, Node*> struct_tag_def_map;
  Node* last_enum_def = nullptr;
};

// Template-definition and template-scope state.
// - template_struct_* tables track primary templates and specializations.
// - template_scope_stack is the push/pop surface for active template params.
struct ParserTemplateState {
  struct NttpDefaultExprKey {
    QualifiedNameKey template_key;
    int param_idx = -1;

    [[nodiscard]] bool operator==(const NttpDefaultExprKey& other) const {
      return template_key == other.template_key &&
             param_idx == other.param_idx;
    }
  };

  struct NttpDefaultExprKeyHash {
    [[nodiscard]] size_t operator()(const NttpDefaultExprKey& key) const {
      return static_cast<size_t>(hash_id_words(
          kIdHashSeed, static_cast<uint32_t>(key.template_key.context_id),
          key.template_key.qualifier_path_id, key.template_key.base_text_id,
          static_cast<uint32_t>(key.template_key.is_global_qualified),
          static_cast<uint32_t>(key.param_idx)));
    }
  };

  struct TemplateInstantiationKey {
    struct Argument {
      bool is_value = false;
      std::string canonical_key;

      [[nodiscard]] bool operator==(const Argument& other) const {
        return is_value == other.is_value &&
               canonical_key == other.canonical_key;
      }
    };

    QualifiedNameKey template_key;
    std::vector<Argument> arguments;

    [[nodiscard]] bool operator==(
        const TemplateInstantiationKey& other) const {
      return template_key == other.template_key &&
             arguments == other.arguments;
    }
  };

  struct TemplateInstantiationKeyHash {
    [[nodiscard]] size_t operator()(
        const TemplateInstantiationKey& key) const {
      const uint64_t name_hash = hash_id_words(
          kIdHashSeed, static_cast<uint32_t>(key.template_key.context_id),
          key.template_key.qualifier_path_id, key.template_key.base_text_id,
          static_cast<uint32_t>(key.template_key.is_global_qualified));
      size_t args_hash = 0;
      for (const auto& arg : key.arguments) {
        const size_t arg_hash =
            std::hash<std::string>{}(arg.canonical_key) ^
            (static_cast<size_t>(arg.is_value) + 0x9e3779b9U +
             (args_hash << 6U) + (args_hash >> 2U));
        args_hash ^= arg_hash + 0x9e3779b9U + (args_hash << 6U) +
                     (args_hash >> 2U);
      }
      return static_cast<size_t>(name_hash) ^ (args_hash + 0x9e3779b9U +
                                               (static_cast<size_t>(name_hash) << 6U) +
                                               (static_cast<size_t>(name_hash) >> 2U));
    }
  };

  std::unordered_map<QualifiedNameKey, Node*, QualifiedNameKeyHash>
      template_struct_defs_by_key;
  std::unordered_map<QualifiedNameKey, std::vector<Node*>, QualifiedNameKeyHash>
      template_struct_specializations_by_key;
  std::unordered_map<std::string, Node*> template_struct_defs;
  std::unordered_map<std::string, std::vector<Node*>>
      template_struct_specializations;
  std::unordered_set<TemplateInstantiationKey, TemplateInstantiationKeyHash>
      instantiated_template_struct_keys_by_key;
  std::set<std::string> instantiated_template_struct_keys;
  size_t template_struct_instantiation_key_mismatch_count = 0;
  std::unordered_map<NttpDefaultExprKey, std::vector<Token>,
                     NttpDefaultExprKeyHash>
      nttp_default_expr_tokens_by_key;
  std::unordered_map<std::string, std::vector<Token>>
      nttp_default_expr_tokens;
  size_t nttp_default_expr_cache_mismatch_count = 0;
  std::unordered_map<QualifiedNameKey, ParserAliasTemplateInfo,
                     QualifiedNameKeyHash>
      alias_template_info;
  std::vector<ParserTemplateScopeFrame> template_scope_stack;
};

// Explicit parser-local lexical binding scopes. These stay separate from
// namespace traversal state so block-local visibility can migrate to
// TextId-native lookup without changing qualified lookup behavior.
struct ParserLexicalScopeState {
  using LocalTypeTable = LocalNameTable<LocalNameKey, TypeSpec>;
  using LocalUserTypedefTable = LocalNameTable<LocalNameKey, bool>;

  void push_scope() {
    visible_typedef_types.push_scope();
    visible_value_types.push_scope();
    visible_user_typedefs.push_scope();
  }

  bool pop_scope() {
    const bool popped_types = visible_typedef_types.pop_scope();
    const bool popped_values = visible_value_types.pop_scope();
    const bool popped_user_typedefs = visible_user_typedefs.pop_scope();
    return popped_types || popped_values || popped_user_typedefs;
  }

  [[nodiscard]] bool empty() const { return visible_typedef_types.empty(); }

  [[nodiscard]] size_t scope_depth() const {
    return visible_typedef_types.scope_depth();
  }

  LocalTypeTable visible_typedef_types;
  LocalTypeTable visible_value_types;
  LocalUserTypedefTable visible_user_typedefs;
};

// Transient parser context that moves with lexical/state transitions.
// These fields are restored by snapshots or cleared by the matching pop path.
struct ParserActiveContextState {
  QualifiedNameKey last_using_alias_key;
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
  struct UsingValueAlias {
    QualifiedNameKey target_key;
    std::string compatibility_name;

    UsingValueAlias() = default;
    explicit UsingValueAlias(std::string_view name)
        : compatibility_name(name) {}
    UsingValueAlias(QualifiedNameKey key, std::string name)
        : target_key(key), compatibility_name(std::move(name)) {}

    UsingValueAlias& operator=(std::string_view name) {
      target_key = {};
      compatibility_name.assign(name);
      return *this;
    }
  };

  std::string current_namespace;
  std::vector<ParserNamespaceContext> namespace_contexts;
  std::vector<int> namespace_stack;
  std::unordered_map<int, std::unordered_map<TextId, int>>
      named_namespace_children;
  std::unordered_map<int, std::vector<int>> anonymous_namespace_children;
  std::unordered_map<int, std::unordered_map<TextId, UsingValueAlias>>
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
  std::unordered_map<QualifiedNameKey, TypeSpec, QualifiedNameKeyHash>
      value_bindings;
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
  std::vector<ParserInjectedTemplateParam> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserRecordTemplatePreludeGuard(Parser* p);
  ~ParserRecordTemplatePreludeGuard();
};

// Template-declaration prelude guard: same push/pop shape as record preludes.
struct ParserTemplateDeclarationPreludeGuard {
  Parser* parser = nullptr;
  std::vector<ParserInjectedTemplateParam> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserTemplateDeclarationPreludeGuard(Parser* p);
  ~ParserTemplateDeclarationPreludeGuard();
};

}  // namespace c4c

#pragma once

#include <chrono>
#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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
  std::unordered_map<TextId, TypeSpec> var_types_by_text_id;
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
  // TypeSpec::record_def via resolve_record_type_spec(); callers that still
  // use this mirror must opt into the explicit compatibility API.
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
      enum class PayloadKind {
        Type,
        TypeCompatibilityText,
        ValueExpression,
        ValueTokens,
        LegacyExpressionText,
        NumericValue,
      };

      struct TypeComponent {
        enum class Kind {
          Base,
          NameIdentity,
          Qualifier,
          Declarator,
          ArrayDim,
          Vector,
          TemplateOrigin,
          TemplateArgTypeBegin,
          TemplateArgEnd,
          TemplateArgValue,
          DeferredMemberType,
          CompatibilityText,
        };

        Kind kind = Kind::Base;
        int index = -1;
        int base = 0;
        int enum_underlying_base = 0;
        TextId text_id = kInvalidText;
        TextId owner_text_id = kInvalidText;
        TextId member_text_id = kInvalidText;
        int namespace_context_id = -1;
        int owner_namespace_context_id = -1;
        int template_param_index = -1;
        bool is_global_qualified = false;
        int qualifier_path_id = -1;
        std::uintptr_t record_identity = 0;
        int ptr_level = 0;
        bool is_lvalue_ref = false;
        bool is_rvalue_ref = false;
        bool is_const = false;
        bool is_volatile = false;
        bool is_fn_ptr = false;
        bool is_packed = false;
        bool is_noinline = false;
        bool is_always_inline = false;
        int align_bytes = 0;
        int array_rank = 0;
        long long value = 0;
        long long value2 = 0;
        long long value3 = 0;
        std::string compatibility_text;

        [[nodiscard]] bool operator==(const TypeComponent& other) const {
          return kind == other.kind && index == other.index &&
                 base == other.base &&
                 enum_underlying_base == other.enum_underlying_base &&
                 text_id == other.text_id &&
                 owner_text_id == other.owner_text_id &&
                 member_text_id == other.member_text_id &&
                 namespace_context_id == other.namespace_context_id &&
                 owner_namespace_context_id ==
                     other.owner_namespace_context_id &&
                 template_param_index == other.template_param_index &&
                 is_global_qualified == other.is_global_qualified &&
                 qualifier_path_id == other.qualifier_path_id &&
                 record_identity == other.record_identity &&
                 ptr_level == other.ptr_level &&
                 is_lvalue_ref == other.is_lvalue_ref &&
                 is_rvalue_ref == other.is_rvalue_ref &&
                 is_const == other.is_const &&
                 is_volatile == other.is_volatile &&
                 is_fn_ptr == other.is_fn_ptr &&
                 is_packed == other.is_packed &&
                 is_noinline == other.is_noinline &&
                 is_always_inline == other.is_always_inline &&
                 align_bytes == other.align_bytes &&
                 array_rank == other.array_rank && value == other.value &&
                 value2 == other.value2 && value3 == other.value3 &&
                 compatibility_text == other.compatibility_text;
        }
      };

      struct ValueExpressionNode {
        enum class Role {
          Root,
          Left,
          Right,
          Cond,
          Then,
          Else,
        };

        Role role = Role::Root;
        int depth = 0;
        int parent_index = -1;
        int node_kind = 0;
        std::string op;
        long long int_value = 0;
        TextId variable_text_id = kInvalidText;
        bool is_global_qualified = false;
        std::vector<TextId> qualifier_text_ids;

        [[nodiscard]] bool operator==(
            const ValueExpressionNode& other) const {
          return role == other.role && depth == other.depth &&
                 parent_index == other.parent_index &&
                 node_kind == other.node_kind && op == other.op &&
                 int_value == other.int_value &&
                 variable_text_id == other.variable_text_id &&
                 is_global_qualified == other.is_global_qualified &&
                 qualifier_text_ids == other.qualifier_text_ids;
        }
      };

      struct CapturedToken {
        int kind = 0;
        TextId text_id = kInvalidText;

        [[nodiscard]] bool operator==(const CapturedToken& other) const {
          return kind == other.kind && text_id == other.text_id;
        }
      };

      bool is_value = false;
      PayloadKind payload_kind = PayloadKind::Type;
      std::vector<TypeComponent> type_components;
      std::string compatibility_type_text;
      std::vector<ValueExpressionNode> value_expression_nodes;
      std::vector<CapturedToken> captured_tokens;
      std::string legacy_expression_text;
      long long numeric_value = 0;

      [[nodiscard]] static Argument type(std::vector<TypeComponent> components) {
        Argument arg;
        arg.is_value = false;
        arg.payload_kind = PayloadKind::Type;
        arg.type_components = std::move(components);
        return arg;
      }

      [[nodiscard]] static Argument type_base(TypeBase base_value) {
        TypeComponent base;
        base.kind = TypeComponent::Kind::Base;
        base.base = static_cast<int>(base_value);
        TypeComponent name;
        name.kind = TypeComponent::Kind::NameIdentity;
        TypeComponent declarator;
        declarator.kind = TypeComponent::Kind::Declarator;
        TypeComponent array_shape;
        array_shape.kind = TypeComponent::Kind::ArrayDim;
        array_shape.index = -1;
        array_shape.value = -1;
        array_shape.value2 = -1;
        return type({base, name, declarator, array_shape});
      }

      [[nodiscard]] static Argument type_compatibility_text(
          std::string text) {
        Argument arg;
        arg.is_value = false;
        arg.payload_kind = PayloadKind::TypeCompatibilityText;
        arg.compatibility_type_text = std::move(text);
        return arg;
      }

      [[nodiscard]] static Argument value_expression(
          std::vector<ValueExpressionNode> nodes) {
        Argument arg;
        arg.is_value = true;
        arg.payload_kind = PayloadKind::ValueExpression;
        arg.value_expression_nodes = std::move(nodes);
        return arg;
      }

      [[nodiscard]] static Argument value_tokens(
          std::vector<CapturedToken> tokens) {
        Argument arg;
        arg.is_value = true;
        arg.payload_kind = PayloadKind::ValueTokens;
        arg.captured_tokens = std::move(tokens);
        return arg;
      }

      [[nodiscard]] static Argument legacy_expression(std::string text) {
        Argument arg;
        arg.is_value = true;
        arg.payload_kind = PayloadKind::LegacyExpressionText;
        arg.legacy_expression_text = std::move(text);
        return arg;
      }

      [[nodiscard]] static Argument numeric(long long value) {
        Argument arg;
        arg.is_value = true;
        arg.payload_kind = PayloadKind::NumericValue;
        arg.numeric_value = value;
        return arg;
      }

      [[nodiscard]] bool operator==(const Argument& other) const {
        if (is_value != other.is_value ||
            payload_kind != other.payload_kind) {
          return false;
        }
        switch (payload_kind) {
          case PayloadKind::Type:
            return type_components == other.type_components;
          case PayloadKind::TypeCompatibilityText:
            return compatibility_type_text == other.compatibility_type_text;
          case PayloadKind::ValueExpression:
            return value_expression_nodes == other.value_expression_nodes;
          case PayloadKind::ValueTokens:
            return captured_tokens == other.captured_tokens;
          case PayloadKind::LegacyExpressionText:
            return legacy_expression_text == other.legacy_expression_text;
          case PayloadKind::NumericValue:
            return numeric_value == other.numeric_value;
        }
        return false;
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
        const size_t arg_hash = hash_argument(arg);
        args_hash ^= arg_hash + 0x9e3779b9U + (args_hash << 6U) +
                     (args_hash >> 2U);
      }
      return static_cast<size_t>(name_hash) ^ (args_hash + 0x9e3779b9U +
                                               (static_cast<size_t>(name_hash) << 6U) +
                                               (static_cast<size_t>(name_hash) >> 2U));
    }

   private:
    [[nodiscard]] static size_t mix(size_t seed, size_t value) {
      return seed ^ (value + 0x9e3779b9U + (seed << 6U) + (seed >> 2U));
    }

    [[nodiscard]] static size_t hash_expr_node(
        const TemplateInstantiationKey::Argument::ValueExpressionNode& node) {
      size_t out = std::hash<int>{}(static_cast<int>(node.role));
      out = mix(out, std::hash<int>{}(node.depth));
      out = mix(out, std::hash<int>{}(node.parent_index));
      out = mix(out, std::hash<int>{}(node.node_kind));
      out = mix(out, std::hash<std::string>{}(node.op));
      out = mix(out, std::hash<long long>{}(node.int_value));
      out = mix(out, std::hash<int>{}(node.variable_text_id));
      out = mix(out, std::hash<bool>{}(node.is_global_qualified));
      for (const TextId text_id : node.qualifier_text_ids) {
        out = mix(out, std::hash<int>{}(text_id));
      }
      return out;
    }

    [[nodiscard]] static size_t hash_type_component(
        const TemplateInstantiationKey::Argument::TypeComponent& component) {
      size_t out = std::hash<int>{}(static_cast<int>(component.kind));
      out = mix(out, std::hash<int>{}(component.index));
      out = mix(out, std::hash<int>{}(component.base));
      out = mix(out, std::hash<int>{}(component.enum_underlying_base));
      out = mix(out, std::hash<int>{}(component.text_id));
      out = mix(out, std::hash<int>{}(component.owner_text_id));
      out = mix(out, std::hash<int>{}(component.member_text_id));
      out = mix(out, std::hash<int>{}(component.namespace_context_id));
      out = mix(out,
                std::hash<int>{}(component.owner_namespace_context_id));
      out = mix(out, std::hash<int>{}(component.template_param_index));
      out = mix(out, std::hash<bool>{}(component.is_global_qualified));
      out = mix(out, std::hash<int>{}(component.qualifier_path_id));
      out = mix(out, std::hash<std::uintptr_t>{}(component.record_identity));
      out = mix(out, std::hash<int>{}(component.ptr_level));
      out = mix(out, std::hash<bool>{}(component.is_lvalue_ref));
      out = mix(out, std::hash<bool>{}(component.is_rvalue_ref));
      out = mix(out, std::hash<bool>{}(component.is_const));
      out = mix(out, std::hash<bool>{}(component.is_volatile));
      out = mix(out, std::hash<bool>{}(component.is_fn_ptr));
      out = mix(out, std::hash<bool>{}(component.is_packed));
      out = mix(out, std::hash<bool>{}(component.is_noinline));
      out = mix(out, std::hash<bool>{}(component.is_always_inline));
      out = mix(out, std::hash<int>{}(component.align_bytes));
      out = mix(out, std::hash<int>{}(component.array_rank));
      out = mix(out, std::hash<long long>{}(component.value));
      out = mix(out, std::hash<long long>{}(component.value2));
      out = mix(out, std::hash<long long>{}(component.value3));
      out = mix(out, std::hash<std::string>{}(component.compatibility_text));
      return out;
    }

    [[nodiscard]] static size_t hash_argument(
        const TemplateInstantiationKey::Argument& arg) {
      using Argument = TemplateInstantiationKey::Argument;
      size_t out = std::hash<bool>{}(arg.is_value);
      out = mix(out, std::hash<int>{}(static_cast<int>(arg.payload_kind)));
      switch (arg.payload_kind) {
        case Argument::PayloadKind::Type:
          for (const auto& component : arg.type_components) {
            out = mix(out, hash_type_component(component));
          }
          return out;
        case Argument::PayloadKind::TypeCompatibilityText:
          return mix(out, std::hash<std::string>{}(
                              arg.compatibility_type_text));
        case Argument::PayloadKind::ValueExpression:
          for (const auto& node : arg.value_expression_nodes) {
            out = mix(out, hash_expr_node(node));
          }
          return out;
        case Argument::PayloadKind::ValueTokens:
          for (const auto& token : arg.captured_tokens) {
            out = mix(out, std::hash<int>{}(token.kind));
            out = mix(out, std::hash<int>{}(token.text_id));
          }
          return out;
        case Argument::PayloadKind::LegacyExpressionText:
          return mix(out,
                     std::hash<std::string>{}(arg.legacy_expression_text));
        case Argument::PayloadKind::NumericValue:
          return mix(out, std::hash<long long>{}(arg.numeric_value));
      }
      return out;
    }
  };

  struct TemplateInstantiationMemberTypedefKey {
    TemplateInstantiationKey concrete_owner;
    TextId member_text_id = kInvalidText;

    [[nodiscard]] bool operator==(
        const TemplateInstantiationMemberTypedefKey& other) const {
      return concrete_owner == other.concrete_owner &&
             member_text_id == other.member_text_id;
    }
  };

  struct TemplateInstantiationMemberTypedefKeyHash {
    [[nodiscard]] size_t operator()(
        const TemplateInstantiationMemberTypedefKey& key) const {
      const size_t owner_hash = TemplateInstantiationKeyHash{}(
          key.concrete_owner);
      const size_t member_hash =
          static_cast<size_t>(hash_id_words(
              kIdHashSeed, static_cast<uint32_t>(key.member_text_id)));
      return owner_hash ^ (member_hash + 0x9e3779b9U +
                           (owner_hash << 6U) + (owner_hash >> 2U));
    }
  };

  struct DependentRecordMemberTypedefKey {
    QualifiedNameKey owner_key;
    TextId member_text_id = kInvalidText;

    [[nodiscard]] bool operator==(
        const DependentRecordMemberTypedefKey& other) const {
      return owner_key == other.owner_key &&
             member_text_id == other.member_text_id;
    }
  };

  struct DependentRecordMemberTypedefKeyHash {
    [[nodiscard]] size_t operator()(
        const DependentRecordMemberTypedefKey& key) const {
      const size_t owner_hash = QualifiedNameKeyHash{}(key.owner_key);
      const size_t member_hash =
          static_cast<size_t>(hash_id_words(
              kIdHashSeed, static_cast<uint32_t>(key.member_text_id)));
      return owner_hash ^ (member_hash + 0x9e3779b9U +
                           (owner_hash << 6U) + (owner_hash >> 2U));
    }
  };

  std::unordered_map<QualifiedNameKey, Node*, QualifiedNameKeyHash>
      template_struct_defs_by_key;
  std::unordered_map<QualifiedNameKey, std::vector<Node*>, QualifiedNameKeyHash>
      template_struct_specializations_by_key;
  std::unordered_map<QualifiedNameKey, Node*, QualifiedNameKeyHash>
      template_global_defs_by_key;
  std::unordered_set<TemplateInstantiationKey, TemplateInstantiationKeyHash>
      instantiated_template_struct_keys_by_key;
  std::unordered_map<TemplateInstantiationMemberTypedefKey, TypeSpec,
                     TemplateInstantiationMemberTypedefKeyHash>
      template_instantiation_member_typedefs_by_key;
  std::unordered_map<DependentRecordMemberTypedefKey, TypeSpec,
                     DependentRecordMemberTypedefKeyHash>
      dependent_record_member_typedefs_by_owner;
  std::unordered_map<QualifiedNameKey, ParserAliasTemplateMemberTypedefInfo,
                     QualifiedNameKeyHash>
      record_member_typedef_infos_by_key;
  std::unordered_map<NttpDefaultExprKey, std::vector<Token>,
                     NttpDefaultExprKeyHash>
      nttp_default_expr_tokens_by_key;
  // Rendered-name compatibility mirror for TextId-less/final-spelling NTTP
  // default lookups. Semantic cache lookup should prefer the
  // NttpDefaultExprKey table above.
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
  ParserAliasTemplateMemberTypedefInfo last_using_alias_member_typedef;
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
  std::unordered_map<TextId, TypeSpec> var_types_by_text_id;
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

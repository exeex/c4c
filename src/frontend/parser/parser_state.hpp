#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "token.hpp"
#include "../../shared/text_id_table.hpp"

namespace c4c {

class Parser;

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

struct ParserFnPtrTypedefInfo {
  Node** params = nullptr;
  int n_params = 0;
  bool variadic = false;
};

struct ParserNamespaceContext {
  int id = 0;
  int parent_id = -1;
  bool is_anonymous = false;
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

enum class ParserTemplateScopeKind {
  EnclosingClass,
  MemberTemplate,
  FreeFunctionTemplate,
};

struct ParserTemplateScopeParam {
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

struct ParserLiteSnapshot {
  int pos = 0;
  ParserTentativeTextRef last_resolved_typedef;
  int template_arg_expr_depth = 0;
  size_t token_mutation_count = 0;
};

struct ParserSnapshot {
  ParserLiteSnapshot lite;
  ParserNameTables symbol_tables;
  std::unordered_set<TextId> non_atom_typedefs;
  std::unordered_set<TextId> non_atom_user_typedefs;
  std::unordered_map<TextId, TypeSpec> non_atom_typedef_types;
  std::unordered_map<TextId, TypeSpec> non_atom_var_types;
};

struct ParserTentativeParseStats {
  int heavy_enter = 0;
  int heavy_commit = 0;
  int heavy_rollback = 0;
  int lite_enter = 0;
  int lite_commit = 0;
  int lite_rollback = 0;
};

struct ParserParseContextGuard {
  Parser* parser = nullptr;
  ParserParseContextGuard(Parser* parser, const char* function_name);
  ~ParserParseContextGuard();
};

struct ParserTentativeParseGuard {
  Parser& parser;
  ParserSnapshot snapshot;
  int start_pos = -1;
  bool committed = false;

  explicit ParserTentativeParseGuard(Parser& p);
  ~ParserTentativeParseGuard();
  void commit();
};

struct ParserTentativeParseGuardLite {
  Parser& parser;
  ParserLiteSnapshot snapshot;
  int start_pos = -1;
  bool committed = false;

  explicit ParserTentativeParseGuardLite(Parser& p);
  ~ParserTentativeParseGuardLite();
  void commit();
};

struct ParserLocalVarBindingSuppressionGuard {
  Parser& parser;
  bool old = false;

  explicit ParserLocalVarBindingSuppressionGuard(Parser& p);
  ~ParserLocalVarBindingSuppressionGuard();
};

struct ParserRecordTemplatePreludeGuard {
  Parser* parser = nullptr;
  std::vector<std::string> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserRecordTemplatePreludeGuard(Parser* p);
  ~ParserRecordTemplatePreludeGuard();
};

struct ParserTemplateDeclarationPreludeGuard {
  Parser* parser = nullptr;
  std::vector<std::string> injected_type_params;
  bool pushed_template_scope = false;

  explicit ParserTemplateDeclarationPreludeGuard(Parser* p);
  ~ParserTemplateDeclarationPreludeGuard();
};

struct ParserTokenMutation {
  int pos = -1;
  Token token;
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

}  // namespace c4c

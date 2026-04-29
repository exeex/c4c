#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ast.hpp"
#include "token.hpp"

namespace c4c {

class Arena;
class Parser;

using ParserSymbolId = uint32_t;
constexpr ParserSymbolId kInvalidParserSymbol = 0;

// Boundary role: parser-local state/table for parser-owned identifier ids.
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

// Boundary role: parser-local state/table for active parser name bindings.
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

// Boundary role: parser-local state carrier for tentative token rewriting.
struct ParserTokenMutation {
  int pos = -1;
  Token token;
};

// Boundary role: cross-stage durable payload for function-pointer typedefs.
struct ParserFnPtrTypedefInfo {
  Node** params = nullptr;
  int n_params = 0;
  bool variadic = false;
};

// Boundary role: parser-local state/table entry for namespace context lookup.
struct ParserNamespaceContext {
  int id = 0;
  int parent_id = -1;
  bool is_anonymous = false;
  TextId text_id = kInvalidText;
  const char* display_name = nullptr;
  const char* canonical_name = nullptr;
};

// Boundary role: parse-time carrier and AST projection source for names.
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

// Boundary role: parse-time carrier and AST projection source for template args.
struct ParserTemplateArgParseResult {
  bool is_value = false;
  TypeSpec type{};
  long long value = 0;
  const char* nttp_name = nullptr;
  Node* expr = nullptr;
};

// Boundary role: parser-local state tag for template scope tracking.
enum class ParserTemplateScopeKind {
  EnclosingClass,
  MemberTemplate,
  FreeFunctionTemplate,
};

// Boundary role: parser-local state and compatibility spelling holder.
struct ParserTemplateScopeParam {
  TextId name_text_id = kInvalidText;
  const char* name = nullptr;
  bool is_nttp = false;
};

// Boundary role: parser-local state and compatibility spelling holder.
struct ParserInjectedTemplateParam {
  TextId name_text_id = kInvalidText;
  const char* name = nullptr;
};

// Boundary role: parser-local state/table frame for active template scopes.
struct ParserTemplateScopeFrame {
  ParserTemplateScopeKind kind = ParserTemplateScopeKind::FreeFunctionTemplate;
  std::vector<ParserTemplateScopeParam> params;
  std::string owner_struct_tag;
};

// Boundary role: AST projection source collected while parsing record bodies.
struct ParserRecordBodyState {
  std::vector<Node*> fields;
  std::vector<Node*> methods;
  std::vector<const char*> member_typedef_names;
  std::vector<TypeSpec> member_typedef_types;
};

// Boundary role: diagnostics/debug carrier for record-member recovery.
enum class ParserRecordMemberRecoveryResult {
  Failed,
  SyncedAtSemicolon,
  StoppedAtNextMember,
  StoppedAtRBrace,
};

// Boundary role: diagnostics/debug carrier for parse stack reporting.
struct ParserParseContextFrame {
  std::string function_name;
  int token_index = -1;
};

// Boundary role: diagnostics/debug carrier for best parse-failure reporting.
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

// Boundary role: diagnostics/debug carrier for parser trace events.
struct ParserParseDebugEvent {
  std::string kind;
  std::string function_name;
  std::string detail;
  int token_index = -1;
  int line = 1;
  int column = 1;
};

// Boundary role: diagnostics/debug carrier for tentative-parse tracing.
enum class ParserTentativeParseMode {
  Heavy,
  Lite,
};

// Boundary role: parser-local state tag for tentative text references.
enum class ParserTentativeTextRefKind {
  None,
  TextId,
};

// Boundary role: parse-time carrier for tentative typedef resolution.
struct ParserTentativeTextRef {
  ParserTentativeTextRefKind kind = ParserTentativeTextRefKind::None;
  TextId text_id = kInvalidText;
};

// Boundary role: cross-stage durable payload and compatibility spelling holder.
struct ParserAliasTemplateInfo {
  std::vector<const char*> param_names;
  std::vector<TextId> param_name_text_ids;
  std::vector<bool> param_is_nttp;
  std::vector<bool> param_is_pack;
  std::vector<bool> param_has_default;
  std::vector<TypeSpec> param_default_types;
  std::vector<long long> param_default_values;
  TypeSpec aliased_type;
};

// Boundary role: parser-local state/table for enum constant bindings.
using ParserEnumConstTable = std::unordered_map<TextId, long long>;
// Boundary role: parser-local state/table for constant integer bindings.
using ParserConstIntBindingTable = std::unordered_map<TextId, long long>;

}  // namespace c4c

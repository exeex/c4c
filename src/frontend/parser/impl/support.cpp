#include "parser_impl.hpp"

#include <cstring>
#include <cstdlib>
#include <utility>

namespace c4c {

template <typename T>
auto support_legacy_typespec_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
    return ts.tag;
}

const char* support_legacy_typespec_tag_if_present(const TypeSpec&, long) {
    return nullptr;
}

static bool typespec_has_unresolved_typedef_identity(const TypeSpec& ts) {
    if (ts.base != TB_TYPEDEF) return false;
    if (ts.template_param_text_id != kInvalidText ||
        ts.tag_text_id != kInvalidText) {
        return true;
    }
    return support_legacy_typespec_tag_if_present(ts, 0) != nullptr;
}

// ── ParserSnapshot save / restore ────────────────────────────────────────────

Parser::ParserSymbolTables& Parser::parser_symbol_tables() {
    return shared_lookup_state_.parser_name_tables;
}

const Parser::ParserSymbolTables& Parser::parser_symbol_tables() const {
    return shared_lookup_state_.parser_name_tables;
}

TextId Parser::parser_text_id_for_token(TextId token_text_id,
                                        std::string_view fallback) const {
    if (token_text_id != kInvalidText) return token_text_id;
    if (fallback.empty()) return kInvalidText;
    return shared_lookup_state_.token_texts
               ? shared_lookup_state_.token_texts->intern(fallback)
               : kInvalidText;
}

TextId Parser::find_parser_text_id(std::string_view text) const {
    if (text.empty() || !shared_lookup_state_.token_texts) return kInvalidText;
    return shared_lookup_state_.token_texts->find(text);
}

std::string_view Parser::parser_text(TextId text_id,
                                     std::string_view fallback) const {
    if (shared_lookup_state_.token_texts && text_id != kInvalidText) {
        return shared_lookup_state_.token_texts->lookup(text_id);
    }
    return fallback;
}

void Parser::clear_current_struct_tag() {
    active_context_state_.current_struct_tag.clear();
    active_context_state_.current_struct_tag_text_id = kInvalidText;
}

void Parser::set_current_struct_tag(TextId tag_text_id,
                                    std::string_view display_name) {
    active_context_state_.current_struct_tag =
        std::string(parser_text(tag_text_id, display_name));
    active_context_state_.current_struct_tag_text_id = tag_text_id;
}

std::string_view Parser::current_struct_tag_text() const {
    return parser_text(active_context_state_.current_struct_tag_text_id,
                       active_context_state_.current_struct_tag);
}

void Parser::clear_last_resolved_typedef() {
    active_context_state_.last_resolved_typedef.clear();
    active_context_state_.last_resolved_typedef_text_id = kInvalidText;
}

void Parser::set_last_resolved_typedef(TextId name_text_id,
                                       std::string_view display_name) {
    if (name_text_id == kInvalidText) {
        clear_last_resolved_typedef();
        return;
    }
    active_context_state_.last_resolved_typedef =
        std::string(parser_text(name_text_id, display_name));
    active_context_state_.last_resolved_typedef_text_id = name_text_id;
}

void Parser::clear_last_using_alias_name() {
    active_context_state_.last_using_alias_key = {};
    active_context_state_.last_using_alias_member_typedef = {};
    active_context_state_.last_using_alias_name.clear();
    active_context_state_.last_using_alias_name_text_id = kInvalidText;
}

void Parser::set_last_using_alias_name(const QualifiedNameKey& key) {
    active_context_state_.last_using_alias_key = key;
    active_context_state_.last_using_alias_name.clear();
    active_context_state_.last_using_alias_name_text_id = key.base_text_id;
}

std::string_view Parser::last_using_alias_name_text() const {
    return parser_text(active_context_state_.last_using_alias_name_text_id,
                       active_context_state_.last_using_alias_name);
}

const Parser::FnPtrTypedefInfo* Parser::find_typedef_fn_ptr_info(
    TextId text_id) const {
    if (text_id == kInvalidText) return nullptr;
    const auto it = binding_state_.typedef_fn_ptr_info.find(text_id);
    return it == binding_state_.typedef_fn_ptr_info.end() ? nullptr
                                                          : &it->second;
}

const Parser::FnPtrTypedefInfo* Parser::find_current_typedef_fn_ptr_info()
    const {
    return find_typedef_fn_ptr_info(
        active_context_state_.last_resolved_typedef_text_id);
}

int Parser::current_token_index() const {
    return pos_;
}

int Parser::token_count() const {
    return static_cast<int>(tokens_.size());
}

TokenKind Parser::token_kind(int index) const {
    if (index < 0 || index >= static_cast<int>(tokens_.size())) {
        return TokenKind::EndOfFile;
    }
    return tokens_[index].kind;
}

bool Parser::token_kind_at(int index, TokenKind kind) const {
    return index >= 0 && index < static_cast<int>(tokens_.size()) &&
           tokens_[index].kind == kind;
}

bool Parser::parse_injected_base_type(std::vector<Token> tokens,
                                      const char* debug_reason,
                                      TypeSpec* out_resolved) {
    const int saved_pos = pos_;
    if (tokens.empty() || tokens.back().kind != TokenKind::EndOfFile) {
        const Token seed = tokens.empty() ? Token{} : tokens.back();
        tokens.push_back(make_injected_token(seed, TokenKind::EndOfFile, ""));
    }
    const std::string injected_detail =
        std::string("reason=") +
        (debug_reason ? debug_reason : "template_instantiation") +
        " saved_pos=" + std::to_string(saved_pos) +
        " token_count=" + std::to_string(tokens.size());
    auto saved_tokens = std::move(tokens_);
    tokens_ = std::move(tokens);
    pos_ = 0;
    note_parse_debug_event_for("injected_parse_begin",
                               "parse_base_type",
                               injected_detail.c_str());
    bool ok = false;
    try {
        TypeSpec resolved = parse_base_type();
        if (out_resolved) *out_resolved = resolved;
        ok = true;
    } catch (...) {
        ok = false;
    }
    note_parse_debug_event_for("injected_parse_end",
                               "parse_base_type",
                               injected_detail.c_str());
    tokens_ = std::move(saved_tokens);
    pos_ = saved_pos;
    return ok;
}

bool Parser::has_defined_struct_tag(std::string_view tag) const {
    if (tag.empty()) return false;
    return definition_state_.defined_struct_tags.count(std::string(tag)) > 0;
}

bool Parser::eval_const_int_with_parser_tables(Node* n, long long* out) const {
    return eval_const_int(n, out, &definition_state_.struct_tag_def_map,
                          &binding_state_.const_int_bindings);
}

void Parser::replace_token_stream_for_testing(std::vector<Token> tokens, int pos) {
    tokens_ = std::move(tokens);
    pos_ = pos;
}

int Parser::token_cursor_for_testing() const {
    return pos_;
}

const Token& Parser::token_at_for_testing(int index) const {
    return tokens_.at(static_cast<size_t>(index));
}

bool Parser::parser_symbols_use_text_table_for_testing(
    const TextTable* texts) const {
    return shared_lookup_state_.parser_symbols.texts_ == texts;
}

size_t Parser::parser_symbol_count_for_testing() const {
    return shared_lookup_state_.parser_symbols.size();
}

void Parser::register_concept_name_for_testing(TextId name_text_id) {
    binding_state_.concept_name_text_ids.insert(name_text_id);
}

void Parser::register_struct_definition_for_testing(std::string tag,
                                                    Node* definition) {
    definition_state_.struct_tag_def_map[std::move(tag)] = definition;
}

void Parser::register_using_value_alias_for_testing(
    int context_id, TextId alias_text_id, const QualifiedNameKey& target_key) {
    namespace_state_.using_value_aliases[context_id][alias_text_id] = {
        target_key};
}

void Parser::register_alias_template_info_for_testing(
    const QualifiedNameKey& key, const ParserAliasTemplateInfo& info) {
    template_state_.alias_template_info[key] = info;
}

bool Parser::has_last_using_alias_name_text_id_for_testing() const {
    return active_context_state_.last_using_alias_name_text_id != kInvalidText;
}

void Parser::replace_last_using_alias_name_fallback_for_testing(
    std::string name) {
    active_context_state_.last_using_alias_name = std::move(name);
}

std::string_view Parser::last_resolved_typedef_text() const {
    return parser_text(active_context_state_.last_resolved_typedef_text_id,
                       active_context_state_.last_resolved_typedef);
}

Parser::SymbolId Parser::symbol_id_for_token_text(TextId token_text_id,
                                                  std::string_view fallback) {
    return shared_lookup_state_.parser_name_tables.intern_identifier(
        parser_text_id_for_token(token_text_id, fallback));
}

std::string_view Parser::symbol_spelling(SymbolId id) const {
    return shared_lookup_state_.parser_symbols.spelling(id);
}

bool Parser::is_cpp_mode() const {
    return core_input_state_.source_profile == SourceProfile::CppSubset ||
           core_input_state_.source_profile == SourceProfile::C4;
}

Parser::ParserLiteSnapshot Parser::save_lite_state() const {
    ParserLiteSnapshot snap;
    snap.pos = pos_;
    if (active_context_state_.last_resolved_typedef_text_id != kInvalidText) {
        snap.last_resolved_typedef.kind = TentativeTextRefKind::TextId;
        snap.last_resolved_typedef.text_id =
            active_context_state_.last_resolved_typedef_text_id;
    }
    snap.template_arg_expr_depth = active_context_state_.template_arg_expr_depth;
    snap.token_mutation_count = core_input_state_.token_mutations.size();
    return snap;
}

void Parser::restore_lite_state(const ParserLiteSnapshot& snap) {
    pos_ = snap.pos;
    clear_last_resolved_typedef();
    if (snap.last_resolved_typedef.kind == TentativeTextRefKind::TextId &&
        snap.last_resolved_typedef.text_id != kInvalidText &&
        shared_lookup_state_.token_texts) {
        set_last_resolved_typedef(snap.last_resolved_typedef.text_id);
    }
    active_context_state_.template_arg_expr_depth =
        snap.template_arg_expr_depth;
    while (core_input_state_.token_mutations.size() > snap.token_mutation_count) {
        const TokenMutation& mutation =
            core_input_state_.token_mutations.back();
        tokens_[mutation.pos] = mutation.token;
        core_input_state_.token_mutations.pop_back();
    }
}

Parser::ParserSnapshot Parser::save_state() const {
    ParserSnapshot snap;
    snap.lite = save_lite_state();
#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
    snap.symbol_tables = parser_symbol_tables();
    snap.non_atom_typedefs = binding_state_.non_atom_typedefs;
    snap.non_atom_user_typedefs = binding_state_.non_atom_user_typedefs;
    snap.non_atom_typedef_types = binding_state_.non_atom_typedef_types;
    snap.var_types_by_text_id = binding_state_.var_types_by_text_id;
    snap.value_bindings = binding_state_.value_bindings;
    snap.lexical_scope_state = lexical_scope_state_;
#endif
    return snap;
}

void Parser::restore_state(const ParserSnapshot& snap) {
    restore_lite_state(snap.lite);
#if ENABLE_HEAVY_TENTATIVE_SNAPSHOT
    parser_symbol_tables() = snap.symbol_tables;
    shared_lookup_state_.parser_name_tables.symbols =
        &shared_lookup_state_.parser_symbols;
    binding_state_.non_atom_typedefs = snap.non_atom_typedefs;
    binding_state_.non_atom_user_typedefs = snap.non_atom_user_typedefs;
    binding_state_.non_atom_typedef_types = snap.non_atom_typedef_types;
    binding_state_.var_types_by_text_id = snap.var_types_by_text_id;
    binding_state_.value_bindings = snap.value_bindings;
    lexical_scope_state_ = snap.lexical_scope_state;
#endif
}


static bool lookup_enum_const_value(const Node* n,
                                    const ParserEnumConstTable& consts,
                                    long long* out) {
    if (!n || !out) return false;
    if (n->unqualified_text_id != kInvalidText) {
        const auto it = consts.find(n->unqualified_text_id);
        if (it != consts.end()) {
            *out = it->second;
            return true;
        }
        return false;
    }
    if (n->name) {
        // Compatibility fallback for AST nodes that still only carry spelling.
        // Enum initializer identifiers should normally arrive with unqualified_text_id.
    }
    return false;
}

bool eval_enum_expr(Node* n, const ParserEnumConstTable& consts, long long* out) {
    if (!n || !out) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) { *out = n->ival; return true; }
    if (n->kind == NK_VAR && n->name) {
        return lookup_enum_const_value(n, consts, out);
    }
    if (n->kind == NK_CAST && n->left) return eval_enum_expr(n->left, consts, out);
    if (n->kind == NK_SIZEOF_TYPE) {
        if (typespec_has_unresolved_typedef_identity(n->type)) {
            return false;  // dependent or unresolved type
        }
        *out = sizeof_type_spec(n->type);
        return true;
    }
    if (n->kind == NK_UNARY && n->op && n->left) {
        long long v = 0;
        if (!eval_enum_expr(n->left, consts, &v)) return false;
        if (strcmp(n->op, "-") == 0) { *out = -v; return true; }
        if (strcmp(n->op, "+") == 0) { *out = v;  return true; }
        return false;
    }
    if (n->kind == NK_BINOP && n->op && n->left && n->right) {
        long long l = 0, r = 0;
        if (!eval_enum_expr(n->left, consts, &l)) return false;
        if (!eval_enum_expr(n->right, consts, &r)) return false;
        if (strcmp(n->op, "+") == 0) { *out = l + r; return true; }
        if (strcmp(n->op, "-") == 0) { *out = l - r; return true; }
        if (strcmp(n->op, "*") == 0) { *out = l * r; return true; }
        if (strcmp(n->op, "&") == 0) { *out = l & r; return true; }
        if (strcmp(n->op, "|") == 0) { *out = l | r; return true; }
        if (strcmp(n->op, "^") == 0) { *out = l ^ r; return true; }
        if (strcmp(n->op, "<<") == 0) { *out = l << r; return true; }
        if (strcmp(n->op, ">>") == 0) { *out = l >> r; return true; }
        if (strcmp(n->op, "/") == 0 && r != 0) { *out = l / r; return true; }
        if (strcmp(n->op, "%") == 0 && r != 0) { *out = l % r; return true; }
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        long long c = 0;
        if (!eval_enum_expr(n->cond, consts, &c)) return false;
        return eval_enum_expr(c ? n->then_ : n->else_, consts, out);
    }
    return false;
}

bool is_dependent_enum_expr(Node* n,
                            const ParserEnumConstTable& consts) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) return false;
    if (n->kind == NK_VAR && n->name) {
        long long ignored = 0;
        if (lookup_enum_const_value(n, consts, &ignored)) return false;
        return true;
    }
    if (n->kind == NK_SIZEOF_TYPE) {
        return typespec_has_unresolved_typedef_identity(n->type);
    }
    if (n->kind == NK_SIZEOF_EXPR) {
        return true;
    }
    if (n->kind == NK_SIZEOF_PACK) {
        return true;
    }
    if (n->kind == NK_CAST && n->left) return is_dependent_enum_expr(n->left, consts);
    if (n->kind == NK_UNARY && n->left) return is_dependent_enum_expr(n->left, consts);
    if (n->kind == NK_BINOP && n->left && n->right) {
        return is_dependent_enum_expr(n->left, consts) ||
               is_dependent_enum_expr(n->right, consts);
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        return is_dependent_enum_expr(n->cond, consts) ||
               is_dependent_enum_expr(n->then_, consts) ||
               is_dependent_enum_expr(n->else_, consts);
    }
    return false;
}

TypeBase effective_scalar_base(const TypeSpec& ts) {
    if (ts.base == TB_ENUM && ts.enum_underlying_base != TB_VOID)
        return ts.enum_underlying_base;
    return ts.base;
}

// ── constant expression evaluator ─────────────────────────────────────────────
// Evaluate a simple AST subtree as an integer constant (for array sizes).
// Returns true and sets *out on success; false if the expression is dynamic.
long long sizeof_base(TypeBase b) {
    switch (b) {
        case TB_VOID: return 1;
        case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
        case TB_SHORT: case TB_USHORT: return 2;
        case TB_INT: case TB_UINT: case TB_FLOAT: return 4;
        case TB_LONG: case TB_ULONG: case TB_LONGLONG: case TB_ULONGLONG:
        case TB_DOUBLE: return 8;
        case TB_LONGDOUBLE: return 16;
        case TB_INT128: case TB_UINT128: return 16;
        case TB_COMPLEX_CHAR:
        case TB_COMPLEX_SCHAR:
        case TB_COMPLEX_UCHAR:
            return 2;
        case TB_COMPLEX_SHORT:
        case TB_COMPLEX_USHORT:
            return 4;
        case TB_COMPLEX_INT:
        case TB_COMPLEX_UINT:
        case TB_COMPLEX_FLOAT:
            return 8;
        case TB_COMPLEX_LONG:
        case TB_COMPLEX_ULONG:
        case TB_COMPLEX_LONGLONG:
        case TB_COMPLEX_ULONGLONG:
        case TB_COMPLEX_DOUBLE:
            return 16;
        case TB_COMPLEX_LONGDOUBLE:
            return 32;
        default: return 4;
    }
}

long long sizeof_type_spec(const TypeSpec& ts) {
    if (ts.ptr_level > 0 || ts.is_fn_ptr) return 8;
    return sizeof_base(effective_scalar_base(ts));
}

long long align_base(TypeBase b, int ptr_level) {
    if (ptr_level > 0) return 8;
    switch (b) {
        case TB_BOOL: case TB_CHAR: case TB_SCHAR: case TB_UCHAR: return 1;
        case TB_SHORT: case TB_USHORT: return 2;
        case TB_INT: case TB_UINT: case TB_FLOAT: case TB_ENUM: return 4;
        case TB_LONG: case TB_ULONG:
        case TB_LONGLONG: case TB_ULONGLONG:
        case TB_DOUBLE:
            return 8;
        case TB_LONGDOUBLE:
        case TB_INT128: case TB_UINT128:
            return 16;
        case TB_COMPLEX_CHAR:
        case TB_COMPLEX_SCHAR:
        case TB_COMPLEX_UCHAR:
            return 1;
        case TB_COMPLEX_SHORT:
        case TB_COMPLEX_USHORT:
            return 2;
        case TB_COMPLEX_INT:
        case TB_COMPLEX_UINT:
        case TB_COMPLEX_FLOAT:
            return 4;
        case TB_COMPLEX_LONG:
        case TB_COMPLEX_ULONG:
        case TB_COMPLEX_LONGLONG:
        case TB_COMPLEX_ULONGLONG:
        case TB_COMPLEX_DOUBLE:
            return 8;
        case TB_COMPLEX_LONGDOUBLE:
            return 16;
        default: return 8;
    }
}

long long alignof_type_spec(const TypeSpec& ts) {
    return align_base(effective_scalar_base(ts), ts.ptr_level);
}

bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    const std::unordered_map<TextId, long long>* structured_named_consts);
bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    const std::unordered_map<std::string, long long>* compatibility_named_consts);

// Resolve record layout identity. TypeSpec::record_def is authoritative; the
// rendered tag map is a final-spelling compatibility fallback for tag-only
// TypeSpecs that have not carried typed record identity through the parser.
Node* resolve_record_type_spec(
    const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map) {
    if (ts.record_def && ts.record_def->kind == NK_STRUCT_DEF) {
        if (ts.record_def->n_fields < 0 && compatibility_tag_map) {
            auto matches_record_identity = [&](const Node* candidate) {
                if (!candidate || candidate->kind != NK_STRUCT_DEF ||
                    candidate->n_fields < 0) {
                    return false;
                }
                if (ts.record_def->unqualified_text_id != kInvalidText &&
                    candidate->unqualified_text_id ==
                        ts.record_def->unqualified_text_id) {
                    return true;
                }
                if (ts.record_def->name && candidate->name &&
                    std::strcmp(ts.record_def->name, candidate->name) == 0) {
                    return true;
                }
                if (ts.record_def->unqualified_name &&
                    candidate->unqualified_name &&
                    std::strcmp(ts.record_def->unqualified_name,
                                candidate->unqualified_name) == 0) {
                    return true;
                }
                return false;
            };
            for (const auto& entry : *compatibility_tag_map) {
                if (matches_record_identity(entry.second)) return entry.second;
            }
        }
        return ts.record_def;
    }
    if (!compatibility_tag_map) return nullptr;
    if (ts.tag_text_id != kInvalidText) {
        for (const auto& entry : *compatibility_tag_map) {
            Node* sd = entry.second;
            if (sd && sd->kind == NK_STRUCT_DEF &&
                sd->unqualified_text_id == ts.tag_text_id) {
                return sd;
            }
        }
        return nullptr;
    }
    const char* tag = support_legacy_typespec_tag_if_present(ts, 0);
    if (!tag) return nullptr;
    const auto it = compatibility_tag_map->find(tag);
    if (it == compatibility_tag_map->end()) return nullptr;
    Node* sd = it->second;
    return sd && sd->kind == NK_STRUCT_DEF ? sd : nullptr;
}

static long long struct_align(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map);
static long long struct_sizeof(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map);

// Compute the ABI alignment of a field type.
static long long field_align(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map) {
    long long natural = 0;
    if (ts.ptr_level > 0) return 8;
    if (ts.is_vector && ts.vector_bytes > 0) {
        natural = ts.vector_bytes > 16 ? 16 : ts.vector_bytes;
    } else if (ts.base == TB_STRUCT || ts.base == TB_UNION) {
        natural = struct_align(ts, compatibility_tag_map);
    } else {
        natural = alignof_type_spec(ts);
    }
    if (ts.align_bytes > 0 && ts.align_bytes > natural) return ts.align_bytes;
    return natural;
}

// Compute the alignment of a struct/union (max alignment of its fields).
static long long struct_align(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map) {
    Node* sd = resolve_record_type_spec(ts, compatibility_tag_map);
    if (!sd) return 8;
    long long max_align = 1;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long a = field_align(f->type, compatibility_tag_map);
        if (a > max_align) max_align = a;
    }
    if (sd->struct_align > max_align) max_align = sd->struct_align;
    return max_align;
}

// Compute sizeof a struct (with alignment padding).
static long long struct_sizeof(const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map) {
    Node* sd = resolve_record_type_spec(ts, compatibility_tag_map);
    if (!sd) return 8;
    long long offset = 0, max_align = 1;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long a = field_align(f->type, compatibility_tag_map);
        if (a > max_align) max_align = a;
        offset = (offset + a - 1) / a * a;
        long long sz;
        if (f->type.ptr_level > 0) sz = 8;
        else if (f->type.is_vector && f->type.vector_bytes > 0)
            sz = f->type.vector_bytes;
        else if (f->type.base == TB_STRUCT || f->type.base == TB_UNION)
            sz = struct_sizeof(f->type, compatibility_tag_map);
        else sz = sizeof_type_spec(f->type);
        if (f->type.array_rank > 0)
            for (int d = 0; d < f->type.array_rank; d++)
                if (f->type.array_dims[d] > 0) sz *= f->type.array_dims[d];
        offset += sz;
    }
    return (offset + max_align - 1) / max_align * max_align;
}

// Helper: compute offsetof(type, field_name) using typed record identity first
// and rendered tag lookup only as a compatibility fallback.
static bool compute_offsetof(const TypeSpec& ts, const char* field_name,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    long long* out) {
    if (!field_name) return false;
    Node* sd = resolve_record_type_spec(ts, compatibility_tag_map);
    if (!sd) return false;
    std::string path(field_name);
    std::string head = path;
    std::string tail;
    size_t dot = path.find('.');
    if (dot != std::string::npos) {
        head = path.substr(0, dot);
        tail = path.substr(dot + 1);
    }

    long long offset = 0;
    for (int i = 0; i < sd->n_fields; i++) {
        Node* f = sd->fields[i];
        if (!f) continue;
        long long align = field_align(f->type, compatibility_tag_map);
        // Align offset
        offset = (offset + align - 1) / align * align;
        if (f->name && head == f->name) {
            if (tail.empty()) {
                *out = offset;
                return true;
            }
            if (f->type.base == TB_STRUCT || f->type.base == TB_UNION) {
                long long inner = 0;
                if (!compute_offsetof(f->type, tail.c_str(),
                                      compatibility_tag_map, &inner)) return false;
                *out = offset + inner;
                return true;
            }
            return false;
        }
        // Advance by field size
        long long sz;
        if (f->type.ptr_level > 0) sz = 8;
        else if (f->type.base == TB_STRUCT || f->type.base == TB_UNION)
            sz = struct_sizeof(f->type, compatibility_tag_map);
        else sz = sizeof_type_spec(f->type);
        if (f->type.array_rank > 0) {
            for (int d = 0; d < f->type.array_rank; d++)
                if (f->type.array_dims[d] > 0) sz *= f->type.array_dims[d];
        }
        offset += sz;
    }
    return false;
}

static bool lookup_unqualified_text_id_const_int_binding(
    const Node* n,
    const std::unordered_map<TextId, long long>* structured_named_consts,
    long long* out) {
    if (!n || !structured_named_consts || !out) return false;
    if (n->is_global_qualified || n->n_qualifier_segments != 0 ||
        n->unqualified_text_id == kInvalidText) {
        return false;
    }
    const auto it = structured_named_consts->find(n->unqualified_text_id);
    if (it == structured_named_consts->end()) return false;
    *out = it->second;
    return true;
}

// Primary parser-owned const-int surface: named constants are keyed by TextId.
bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    const std::unordered_map<TextId, long long>* structured_named_consts) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) {
        *out = n->ival;
        return true;
    }
    if (n->kind == NK_VAR && n->name) {
        return lookup_unqualified_text_id_const_int_binding(
            n, structured_named_consts, out);
    }
    if (n->kind == NK_CAST && n->left) {
        return eval_const_int(n->left, out, compatibility_tag_map,
                              structured_named_consts);
    }
    if (n->kind == NK_OFFSETOF) {
        if (n->type.base == TB_STRUCT || n->type.base == TB_UNION) {
            return compute_offsetof(n->type, n->name, compatibility_tag_map,
                                    out);
        }
        return false;
    }
    if (n->kind == NK_ALIGNOF_TYPE) {
        *out = field_align(n->type, compatibility_tag_map);
        return true;
    }
    if (n->kind == NK_SIZEOF_TYPE) {
        long long sz = sizeof_type_spec(n->type);
        if (n->type.base == TB_STRUCT || n->type.base == TB_UNION) {
            sz = struct_sizeof(n->type, compatibility_tag_map);
        }
        if (n->type.array_rank > 0) {
            for (int i = 0; i < n->type.array_rank; ++i)
                if (n->type.array_dims[i] > 0) sz *= n->type.array_dims[i];
        }
        *out = sz;
        return true;
    }
    if (n->kind == NK_SIZEOF_EXPR) {
        return false;
    }
    if (n->kind == NK_SIZEOF_PACK) {
        return false;
    }
    if (n->kind == NK_UNARY && n->op && n->left) {
        long long v;
        if (!eval_const_int(n->left, &v, compatibility_tag_map,
                            structured_named_consts)) return false;
        if (strcmp(n->op, "-") == 0) { *out = -v; return true; }
        if (strcmp(n->op, "+") == 0) { *out = v; return true; }
        if (strcmp(n->op, "~") == 0) { *out = ~v; return true; }
        if (strcmp(n->op, "!") == 0) { *out = !v; return true; }
    }
    if (n->kind == NK_BINOP && n->op) {
        long long l, r;
        if (!eval_const_int(n->left, &l, compatibility_tag_map,
                            structured_named_consts)) return false;
        if (!eval_const_int(n->right, &r, compatibility_tag_map,
                            structured_named_consts)) return false;
        if (strcmp(n->op, "+")  == 0) { *out = l + r; return true; }
        if (strcmp(n->op, "-")  == 0) { *out = l - r; return true; }
        if (strcmp(n->op, "*")  == 0) { *out = l * r; return true; }
        if (strcmp(n->op, "/")  == 0 && r != 0) { *out = l / r; return true; }
        if (strcmp(n->op, "%")  == 0 && r != 0) { *out = l % r; return true; }
        if (strcmp(n->op, "<<") == 0) { *out = l << r; return true; }
        if (strcmp(n->op, ">>") == 0) { *out = l >> r; return true; }
        if (strcmp(n->op, "&")  == 0) { *out = l & r;  return true; }
        if (strcmp(n->op, "|")  == 0) { *out = l | r;  return true; }
        if (strcmp(n->op, "^")  == 0) { *out = l ^ r;  return true; }
        if (strcmp(n->op, "<")  == 0) { *out = l < r;  return true; }
        if (strcmp(n->op, "<=") == 0) { *out = l <= r; return true; }
        if (strcmp(n->op, ">")  == 0) { *out = l > r;  return true; }
        if (strcmp(n->op, ">=") == 0) { *out = l >= r; return true; }
        if (strcmp(n->op, "==") == 0) { *out = l == r; return true; }
        if (strcmp(n->op, "!=") == 0) { *out = l != r; return true; }
        if (strcmp(n->op, "&&") == 0) { *out = (l && r); return true; }
        if (strcmp(n->op, "||") == 0) { *out = (l || r); return true; }
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        long long c;
        if (!eval_const_int(n->cond, &c, compatibility_tag_map,
                            structured_named_consts)) return false;
        return eval_const_int(c ? n->then_ : n->else_, out,
                              compatibility_tag_map, structured_named_consts);
    }
    return false;
}

// Compatibility bridge for callers that only have rendered names. Keep this
// path behavior-identical until those non-parser surfaces gain structured IDs.
bool eval_const_int(Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    const std::unordered_map<std::string, long long>* compatibility_named_consts) {
    if (!n) return false;
    if (n->kind == NK_INT_LIT || n->kind == NK_CHAR_LIT) {
        *out = n->ival;
        return true;
    }
    if (n->kind == NK_VAR && n->name) {
        if (compatibility_named_consts) {
            auto it = compatibility_named_consts->find(n->name);
            if (it != compatibility_named_consts->end()) {
                *out = it->second;
                return true;
            }
        }
        return false;
    }
    if (n->kind == NK_CAST && n->left) {
        return eval_const_int(n->left, out, compatibility_tag_map,
                              compatibility_named_consts);
    }
    if (n->kind == NK_OFFSETOF) {
        if (n->type.base == TB_STRUCT || n->type.base == TB_UNION) {
            return compute_offsetof(n->type, n->name, compatibility_tag_map,
                                    out);
        }
        return false;
    }
    if (n->kind == NK_ALIGNOF_TYPE) {
        *out = field_align(n->type, compatibility_tag_map);
        return true;
    }
    if (n->kind == NK_SIZEOF_TYPE) {
        // sizeof(type): use LP64 sizes
        long long sz = sizeof_type_spec(n->type);
        if (n->type.base == TB_STRUCT || n->type.base == TB_UNION) {
            sz = struct_sizeof(n->type, compatibility_tag_map);
        }
        if (n->type.array_rank > 0) {
            for (int i = 0; i < n->type.array_rank; ++i)
                if (n->type.array_dims[i] > 0) sz *= n->type.array_dims[i];
        }
        *out = sz;
        return true;
    }
    if (n->kind == NK_SIZEOF_EXPR) {
        // sizeof(expr): approximate from the expression type if it's a literal
        // For non-literals, return a conservative estimate
        return false;  // complex: skip for now
    }
    if (n->kind == NK_SIZEOF_PACK) {
        return false;
    }
    if (n->kind == NK_UNARY && n->op && n->left) {
        long long v;
        if (!eval_const_int(n->left, &v, compatibility_tag_map,
                            compatibility_named_consts)) return false;
        if (strcmp(n->op, "-") == 0) { *out = -v; return true; }
        if (strcmp(n->op, "+") == 0) { *out = v; return true; }
        if (strcmp(n->op, "~") == 0) { *out = ~v; return true; }
        if (strcmp(n->op, "!") == 0) { *out = !v; return true; }
    }
    if (n->kind == NK_BINOP && n->op) {
        long long l, r;
        if (!eval_const_int(n->left, &l, compatibility_tag_map,
                            compatibility_named_consts)) return false;
        if (!eval_const_int(n->right, &r, compatibility_tag_map,
                            compatibility_named_consts)) return false;
        if (strcmp(n->op, "+")  == 0) { *out = l + r; return true; }
        if (strcmp(n->op, "-")  == 0) { *out = l - r; return true; }
        if (strcmp(n->op, "*")  == 0) { *out = l * r; return true; }
        if (strcmp(n->op, "/")  == 0 && r != 0) { *out = l / r; return true; }
        if (strcmp(n->op, "%")  == 0 && r != 0) { *out = l % r; return true; }
        if (strcmp(n->op, "<<") == 0) { *out = l << r; return true; }
        if (strcmp(n->op, ">>") == 0) { *out = l >> r; return true; }
        if (strcmp(n->op, "&")  == 0) { *out = l & r;  return true; }
        if (strcmp(n->op, "|")  == 0) { *out = l | r;  return true; }
        if (strcmp(n->op, "^")  == 0) { *out = l ^ r;  return true; }
        if (strcmp(n->op, "<")  == 0) { *out = l < r;  return true; }
        if (strcmp(n->op, "<=") == 0) { *out = l <= r; return true; }
        if (strcmp(n->op, ">")  == 0) { *out = l > r;  return true; }
        if (strcmp(n->op, ">=") == 0) { *out = l >= r; return true; }
        if (strcmp(n->op, "==") == 0) { *out = l == r; return true; }
        if (strcmp(n->op, "!=") == 0) { *out = l != r; return true; }
        if (strcmp(n->op, "&&") == 0) { *out = (l && r); return true; }
        if (strcmp(n->op, "||") == 0) { *out = (l || r); return true; }
    }
    if (n->kind == NK_TERNARY && n->cond && n->then_ && n->else_) {
        long long c;
        if (!eval_const_int(n->cond, &c, compatibility_tag_map,
                            compatibility_named_consts)) return false;
        return eval_const_int(c ? n->then_ : n->else_, out,
                              compatibility_tag_map,
                              compatibility_named_consts);
    }
    return false;
}

// ── helpers ───────────────────────────────────────────────────────────────────

bool is_qualifier(TokenKind k) {
    return k == TokenKind::KwConst    || k == TokenKind::KwVolatile ||
           k == TokenKind::KwRestrict || k == TokenKind::KwAtomic   ||
           k == TokenKind::KwInline;
}

bool is_storage_class(TokenKind k) {
    return k == TokenKind::KwStatic   || k == TokenKind::KwExtern ||
           k == TokenKind::KwRegister || k == TokenKind::KwAuto   ||
           k == TokenKind::KwTypedef  || k == TokenKind::KwInline ||
           k == TokenKind::KwExtension || k == TokenKind::KwNoreturn ||
           k == TokenKind::KwThreadLocal || k == TokenKind::KwMutable;
}

bool is_type_kw(TokenKind k) {
    switch (k) {
        case TokenKind::KwVoid:
        case TokenKind::KwChar:
        case TokenKind::KwShort:
        case TokenKind::KwInt:
        case TokenKind::KwLong:
        case TokenKind::KwFloat:
        case TokenKind::KwDouble:
        case TokenKind::KwSigned:
        case TokenKind::KwUnsigned:
        case TokenKind::KwBool:
        case TokenKind::KwStruct:
        case TokenKind::KwClass:
        case TokenKind::KwUnion:
        case TokenKind::KwEnum:
        case TokenKind::KwDecltype:
        case TokenKind::KwInt128:
        case TokenKind::KwUInt128:
        case TokenKind::KwBuiltin:   // __builtin_va_list
        case TokenKind::KwAutoType:  // __auto_type
        case TokenKind::KwTypeof:
        case TokenKind::KwTypeofUnqual:
        case TokenKind::KwComplex:
            return true;
        default:
            return false;
    }
}

// parse an integer literal from a lexeme string
// Returns true if a numeric literal lexeme has an imaginary suffix (i/j/I/J).
bool lexeme_is_imaginary(const char* s) {
    size_t len = strlen(s);
    for (size_t k = 0; k < len; k++) {
        char c = s[k];
        if (c == 'i' || c == 'I' || c == 'j' || c == 'J') return true;
    }
    return false;
}

long long parse_int_lexeme(const char* s) {
    // strip u/U, l/L and imaginary i/j/I/J suffixes (any order)
    char buf[64];
    int  len = (int)strlen(s);
    int  end = len;
    while (end > 0) {
        char cc = s[end - 1];
        if (cc == 'u' || cc == 'U' || cc == 'l' || cc == 'L' ||
            cc == 'i' || cc == 'I' || cc == 'j' || cc == 'J') {
            --end;
        } else {
            break;
        }
    }
    if (end >= (int)sizeof(buf)) end = (int)sizeof(buf) - 1;
    strncpy(buf, s, (size_t)end);
    buf[end] = '\0';

    if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
        return (long long)strtoull(buf, nullptr, 16);
    } else if (buf[0] == '0' && (buf[1] == 'b' || buf[1] == 'B')) {
        return (long long)strtoull(buf + 2, nullptr, 2);
    } else if (buf[0] == '0' && buf[1] != '\0') {
        return (long long)strtoull(buf, nullptr, 8);
    } else {
        return (long long)strtoull(buf, nullptr, 10);
    }
}

double parse_float_lexeme(const char* s) {
    // strip f/F, l/L, f16/F16, f32/F32, f64/F64, f128/F128, bf16/BF16,
    // and imaginary i/j/I/J suffixes
    char buf[64];
    size_t len = strlen(s);
    if (len >= sizeof(buf)) len = sizeof(buf) - 1;
    strncpy(buf, s, len);
    buf[len] = '\0';
    bool is_float_suffix = false;  // 'f'/'F' suffix → single-precision
    size_t end = len;
    while (end > 0) {
        if (end >= 4 &&
            ((buf[end - 4] == 'f' && buf[end - 3] == '1' &&
              buf[end - 2] == '2' && buf[end - 1] == '8') ||
             (buf[end - 4] == 'F' && buf[end - 3] == '1' &&
              buf[end - 2] == '2' && buf[end - 1] == '8') ||
             (buf[end - 4] == 'b' && buf[end - 3] == 'f' &&
              buf[end - 2] == '1' && buf[end - 1] == '6') ||
             (buf[end - 4] == 'B' && buf[end - 3] == 'F' &&
              buf[end - 2] == '1' && buf[end - 1] == '6'))) {
            is_float_suffix = true;
            end -= 4;
            continue;
        }
        if (end >= 3 &&
            ((buf[end - 3] == 'f' && buf[end - 2] == '1' &&
              buf[end - 1] == '6') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '1' &&
              buf[end - 1] == '6') ||
             (buf[end - 3] == 'f' && buf[end - 2] == '3' &&
              buf[end - 1] == '2') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '3' &&
              buf[end - 1] == '2') ||
             (buf[end - 3] == 'f' && buf[end - 2] == '6' &&
              buf[end - 1] == '4') ||
             (buf[end - 3] == 'F' && buf[end - 2] == '6' &&
              buf[end - 1] == '4'))) {
            is_float_suffix = true;
            end -= 3;
            continue;
        }
        char c = buf[end - 1];
        if (c == 'f' || c == 'F') { is_float_suffix = true; --end; }
        else if (c == 'l' || c == 'L' ||
                 c == 'i' || c == 'I' || c == 'j' || c == 'J') { --end; }
        else { break; }
    }
    buf[end] = '\0';
    // For float (F suffix) literals, round to single-precision first so that fval
    // stores the exact float-rounded value when promoted to double.
    if (is_float_suffix) return (double)(float)strtod(buf, nullptr);
    return strtod(buf, nullptr);
}

TypeSpec resolve_typedef_chain(TypeSpec ts,
                                      const std::unordered_map<std::string, TypeSpec>& tmap) {
    for (int depth = 0; depth < 16; ++depth) {
        if (ts.base != TB_TYPEDEF || ts.ptr_level > 0 || ts.array_rank > 0) break;
        const char* tag = support_legacy_typespec_tag_if_present(ts, 0);
        if (!tag) break;
        auto it = tmap.find(tag);
        if (it == tmap.end()) break;
        bool c = ts.is_const, v = ts.is_volatile;
        ts = it->second;
        ts.is_const   |= c;
        ts.is_volatile |= v;
    }
    return ts;
}

static bool typespec_has_complete_qualifier_text_ids(const TypeSpec& ts) {
    if (ts.n_qualifier_segments <= 0) return false;
    if (!ts.qualifier_text_ids) return false;
    for (int i = 0; i < ts.n_qualifier_segments; ++i) {
        if (ts.qualifier_text_ids[i] == kInvalidText) return false;
    }
    return true;
}

static bool typespec_has_complete_type_name_text_identity(const TypeSpec& ts) {
    if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
        return false;
    }
    if (ts.n_qualifier_segments < 0) return false;
    if (ts.n_qualifier_segments == 0) return true;
    return typespec_has_complete_qualifier_text_ids(ts);
}

static bool same_typespec_type_name_text_identity(const TypeSpec& a,
                                                  const TypeSpec& b) {
    if (a.namespace_context_id != b.namespace_context_id ||
        a.tag_text_id != b.tag_text_id ||
        a.is_global_qualified != b.is_global_qualified ||
        a.n_qualifier_segments != b.n_qualifier_segments) {
        return false;
    }
    for (int i = 0; i < a.n_qualifier_segments; ++i) {
        if (a.qualifier_text_ids[i] != b.qualifier_text_ids[i]) return false;
    }
    return true;
}

static bool same_nominal_typespec_identity(const TypeSpec& a,
                                           const TypeSpec& b) {
    if (a.record_def && b.record_def) return a.record_def == b.record_def;

    const bool a_has_text_identity =
        typespec_has_complete_type_name_text_identity(a);
    const bool b_has_text_identity =
        typespec_has_complete_type_name_text_identity(b);
    if (a_has_text_identity || b_has_text_identity) {
        return a_has_text_identity && b_has_text_identity &&
               same_typespec_type_name_text_identity(a, b);
    }

    if (a.record_def || b.record_def) return false;

    const char* a_tag = support_legacy_typespec_tag_if_present(a, 0);
    const char* b_tag = support_legacy_typespec_tag_if_present(b, 0);
    return a_tag && b_tag && strcmp(a_tag, b_tag) == 0;
}

// Returns true if type a and type b are compatible per GCC __builtin_types_compatible_p rules.
bool types_compatible_p(TypeSpec a, TypeSpec b,
                                const std::unordered_map<std::string, TypeSpec>& tmap) {
    a = resolve_typedef_chain(a, tmap);
    b = resolve_typedef_chain(b, tmap);
    // Strip top-level cv-qualifiers (they don't affect compatibility at the value level).
    // But for pointer types, qualifiers on the pointed-to type DO matter (char* vs const char*).
    if (a.ptr_level == 0 && a.array_rank == 0) { a.is_const = false; a.is_volatile = false; }
    if (b.ptr_level == 0 && b.array_rank == 0) { b.is_const = false; b.is_volatile = false; }
    if (a.is_vector != b.is_vector) return false;
    if (a.is_vector &&
        (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes))
        return false;
    if (a.base != b.base) return false;
    if (a.ptr_level != b.ptr_level) return false;
    if (a.is_const != b.is_const || a.is_volatile != b.is_volatile) return false;
    if (a.array_rank != b.array_rank) return false;
    for (int i = 0; i < a.array_rank; ++i) {
        long long da = a.array_dims[i], db = b.array_dims[i];
        // -2 means unsized ([]) — compatible with any same-rank dimension
        if (da != db && da != -2 && db != -2) return false;
    }
    // Struct/union/enum: same tag required
    if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
        if (!same_nominal_typespec_identity(a, b)) return false;
    }
    return true;
}

}  // namespace c4c

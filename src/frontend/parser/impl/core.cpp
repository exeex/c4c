#include "parser_impl.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace c4c {

namespace {

std::vector<const std::string*> collapse_adjacent_stack_frames(
    const std::vector<std::string>& stack_trace) {
    std::vector<const std::string*> compact;
    compact.reserve(stack_trace.size());
    const std::string* previous = nullptr;
    for (const std::string& fn : stack_trace) {
        if (previous && *previous == fn) continue;
        compact.push_back(&fn);
        previous = &fn;
    }
    return compact;
}

bool should_replace_best_parse_failure(const Parser::ParseFailure& current,
                                       const Parser::ParseFailure& candidate) {
    if (!current.active) return true;
    if (candidate.token_index != current.token_index) {
        const bool wrapper_unwind_prefix =
            candidate.token_index > current.token_index &&
            candidate.token_index <= current.token_index + 1 &&
            candidate.committed == current.committed &&
            candidate.stack_trace.size() < current.stack_trace.size() &&
            !candidate.stack_trace.empty() &&
            std::equal(candidate.stack_trace.begin(), candidate.stack_trace.end(),
                       current.stack_trace.begin());
        if (wrapper_unwind_prefix) return false;
        return candidate.token_index > current.token_index;
    }
    if (candidate.committed != current.committed) {
        return candidate.committed && !current.committed;
    }
    if (candidate.stack_trace.size() != current.stack_trace.size()) {
        return candidate.stack_trace.size() > current.stack_trace.size();
    }
    if (candidate.column != current.column) {
        return candidate.column > current.column;
    }
    return false;
}

bool is_top_level_wrapper_failure(const std::string& function_name) {
    return function_name == "parse_top_level" ||
           function_name == "parse_top_level_parameter_list";
}

bool is_qualified_type_trace_wrapper(const std::string& function_name) {
    return function_name == "parse_next_template_argument" ||
           function_name == "consume_qualified_type_spelling" ||
           function_name == "consume_qualified_type_spelling_with_typename" ||
           function_name == "parse_dependent_typename_specifier";
}

bool is_qualified_type_trace_leaf(const std::string& function_name) {
    return function_name == "try_parse_cpp_scoped_base_type" ||
           function_name == "try_parse_qualified_base_type";
}

bool is_qualified_type_trace_anchor(const std::string& function_name) {
    return is_qualified_type_trace_leaf(function_name) ||
           function_name == "try_parse_template_type_arg";
}

bool is_qualified_type_trace_frame(const std::string& function_name) {
    return is_qualified_type_trace_anchor(function_name) ||
           is_qualified_type_trace_wrapper(function_name);
}

bool is_summary_only_parse_helper(const std::string& function_name) {
    return function_name == "consume_qualified_type_spelling" ||
           function_name == "consume_qualified_type_spelling_with_typename";
}

bool is_param_qualified_type_probe_frame(const std::string& function_name) {
    return function_name == "parse_param" ||
           function_name == "try_parse_cpp_scoped_base_type" ||
           function_name == "try_parse_qualified_base_type" ||
           is_summary_only_parse_helper(function_name);
}

bool stack_contains_qualified_type_trace(
    const std::vector<std::string>& stack_trace) {
    return std::any_of(stack_trace.begin(), stack_trace.end(),
                       [](const std::string& function_name) {
                           return is_qualified_type_trace_anchor(function_name);
                       });
}

bool current_stack_is_prefix_of_best(
    const std::vector<Parser::ParseContextFrame>& current_stack,
    const std::vector<std::string>& best_stack_trace) {
    if (current_stack.size() >= best_stack_trace.size()) return false;
    for (size_t i = 0; i < current_stack.size(); ++i) {
        if (current_stack[i].function_name != best_stack_trace[i]) return false;
    }
    return true;
}

std::vector<std::string> normalize_summary_stack(
    const std::vector<std::string>& stack_trace) {
    std::vector<std::string> normalized;
    normalized.reserve(stack_trace.size());
    for (size_t i = 0; i < stack_trace.size();) {
        if (i + 1 < stack_trace.size() &&
            stack_trace[i] == "parse_next_template_argument" &&
            stack_trace[i + 1] == "try_parse_template_type_arg") {
            normalized.push_back(stack_trace[i]);
            normalized.push_back(stack_trace[i + 1]);
            i += 2;
            while (i + 1 < stack_trace.size() &&
                   stack_trace[i] == "parse_next_template_argument" &&
                   stack_trace[i + 1] == "try_parse_template_type_arg") {
                i += 2;
            }
            continue;
        }
        normalized.push_back(stack_trace[i]);
        ++i;
    }
    return normalized;
}

bool is_top_level_qualified_probe_merge_target(
    const std::string& function_name) {
    return function_name == "parse_next_template_argument" ||
           function_name == "parse_top_level_parameter_list";
}

bool is_identifier_start(char ch) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalpha(uch) || ch == '_';
}

bool is_identifier_continue(char ch) {
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '_';
}

bool uses_symbol_identity(std::string_view name) {
    if (name.empty() || !is_identifier_start(name.front())) return false;
    for (size_t i = 1; i < name.size(); ++i) {
        if (!is_identifier_continue(name[i])) return false;
    }
    return true;
}

bool should_track_local_binding(const Parser& parser,
                                TextId name_text_id,
                                std::string_view name) {
    return name_text_id != kInvalidText &&
           parser.lexical_scope_state_.scope_depth() > 0 &&
           uses_symbol_identity(name);
}

bool can_probe_local_binding(TextId name_text_id, std::string_view name) {
    return name_text_id != kInvalidText && uses_symbol_identity(name) &&
           name.find("::") == std::string_view::npos;
}

bool is_record_projection_type(const Parser& parser, const TypeSpec& type) {
    if (type.base != TB_STRUCT && type.base != TB_UNION) return false;
    if (parser.active_context_state_.current_struct_tag.empty()) return true;
    if (type.record_def) return type.record_def->n_fields >= 0;
    const Node* record =
        resolve_record_type_spec(type, &parser.definition_state_.struct_tag_def_map);
    return record && record->n_fields >= 0;
}

bool spelling_matches_current_record(std::string_view spelling,
                                     std::string_view current_record,
                                     std::string_view qualified_current) {
    if (spelling.empty() || current_record.empty()) return false;
    if (spelling == current_record || spelling == qualified_current) return true;
    const size_t sep = qualified_current.rfind("::");
    return sep != std::string_view::npos &&
           spelling == qualified_current.substr(sep + 2);
}

std::string current_record_namespace_sibling(std::string_view current_record,
                                             std::string_view name) {
    if (name.empty() || name.find("::") != std::string_view::npos) return {};
    const size_t sep = current_record.rfind("::");
    if (sep == std::string_view::npos) return {};
    std::string sibling(current_record.substr(0, sep + 2));
    sibling.append(name);
    return sibling;
}

QualifiedNameKey qualified_key_in_context(const Parser& parser, int context_id,
                                          TextId name_text_id,
                                          bool create_missing_path);

std::string render_structured_name(const Parser& parser,
                                   const QualifiedNameKey& key) {
    if (key.base_text_id == kInvalidText) return {};
    const TextTable* texts = parser.shared_lookup_state_.token_texts;
    if (!texts) return {};
    return render_qualified_name(key, parser.shared_lookup_state_.parser_name_paths,
                                 *texts);
}

std::string render_value_binding_name(const Parser& parser,
                                      const QualifiedNameKey& key) {
    if (key.base_text_id == kInvalidText) return {};
    if (key.is_global_qualified && key.qualifier_path_id == kInvalidNamePath) {
        return std::string(
            parser.parser_text(key.base_text_id, std::string_view{}));
    }
    return render_structured_name(parser, key);
}

QualifiedNameKey find_qualified_name_key(const Parser& parser,
                                         const Parser::QualifiedNameRef& name) {
    QualifiedNameKey key;
    key.context_id = 0;
    key.is_global_qualified = name.is_global_qualified;
    key.base_text_id =
        name.base_text_id != kInvalidText
            ? name.base_text_id
            : parser.find_parser_text_id(name.base_name);
    if (key.base_text_id == kInvalidText) return key;

    if (name.qualifier_segments.empty() && name.qualifier_text_ids.empty()) {
        return key;
    }

    std::vector<TextId> qualifier_text_ids;
    qualifier_text_ids.reserve(
        std::max(name.qualifier_text_ids.size(),
                 name.qualifier_segments.size()));
    for (size_t i = 0; i < name.qualifier_segments.size(); ++i) {
        TextId text_id = i < name.qualifier_text_ids.size()
                             ? name.qualifier_text_ids[i]
                             : kInvalidText;
        if (text_id == kInvalidText) {
            text_id = parser.find_parser_text_id(name.qualifier_segments[i]);
        }
        if (text_id == kInvalidText) return {};
        qualifier_text_ids.push_back(text_id);
    }
    for (size_t i = name.qualifier_segments.size();
         i < name.qualifier_text_ids.size(); ++i) {
        if (name.qualifier_text_ids[i] == kInvalidText) return {};
        qualifier_text_ids.push_back(name.qualifier_text_ids[i]);
    }

    if (!qualifier_text_ids.empty()) {
        key.qualifier_path_id =
            parser.shared_lookup_state_.parser_name_paths.find(
                qualifier_text_ids);
        if (key.qualifier_path_id == kInvalidNamePath) return {};
    }
    return key;
}

std::string render_lookup_name_in_context(const Parser& parser, int context_id,
                                          TextId name_text_id,
                                          std::string_view fallback_name) {
    const QualifiedNameKey key = qualified_key_in_context(
        parser, context_id, name_text_id, true);
    if (const std::string structured = render_value_binding_name(parser, key);
        !structured.empty()) {
        return structured;
    }

    std::string name(parser.parser_text(name_text_id, fallback_name));
    if (name.empty() || context_id <= 0) return name;

    std::vector<int> ancestry;
    for (int walk = context_id; walk > 0;
         walk = parser.namespace_state_.namespace_contexts[walk].parent_id) {
        ancestry.push_back(walk);
    }

    std::string qualified;
    for (auto it = ancestry.rbegin(); it != ancestry.rend(); ++it) {
        const Parser::NamespaceContext& ctx =
            parser.namespace_state_.namespace_contexts[*it];
        if (ctx.is_anonymous) {
            if (ctx.canonical_name && ctx.canonical_name[0]) {
                qualified.assign(ctx.canonical_name);
            }
            continue;
        }

        const std::string_view segment =
            parser.parser_text(ctx.text_id,
                               ctx.display_name ? ctx.display_name : "");
        if (segment.empty()) continue;
        if (!qualified.empty()) qualified += "::";
        qualified += segment;
    }

    if (qualified.empty()) return name;
    qualified += "::";
    qualified += name;
    return qualified;
}

bool is_unqualified_lookup_name(std::string_view name) {
    return !name.empty() && name.find("::") == std::string_view::npos;
}

QualifiedNameKey find_compatibility_key_from_rendered_qualified_spelling(
    const Parser& parser, TextId name_text_id, std::string_view name);
QualifiedNameKey intern_compatibility_key_from_rendered_qualified_spelling(
    Parser& parser, TextId name_text_id, std::string_view name);

QualifiedNameKey qualified_key_in_context(const Parser& parser, int context_id,
                                          TextId name_text_id,
                                          bool create_missing_path) {
    QualifiedNameKey key;
    const std::string_view name = parser.parser_text(name_text_id, {});
    if (name.find("::") != std::string_view::npos) {
        return create_missing_path
                   ? intern_compatibility_key_from_rendered_qualified_spelling(
                         const_cast<Parser&>(parser), name_text_id, name)
                   : find_compatibility_key_from_rendered_qualified_spelling(
                         parser, name_text_id, name);
    }

    key.context_id = 0;
    key.base_text_id = name_text_id;
    if (key.base_text_id == kInvalidText) return key;
    if (context_id <= 0) return key;

    std::vector<int> ancestry;
    for (int walk = context_id; walk > 0;
         walk = parser.namespace_state_.namespace_contexts[walk].parent_id) {
        ancestry.push_back(walk);
    }

    std::vector<TextId> qualifier_text_ids;
    for (auto it = ancestry.rbegin(); it != ancestry.rend(); ++it) {
        const Parser::NamespaceContext& ctx =
            parser.namespace_state_.namespace_contexts[*it];
        if (ctx.is_anonymous) {
            qualifier_text_ids.clear();
            const std::string_view canonical =
                ctx.canonical_name ? std::string_view(ctx.canonical_name)
                                   : std::string_view{};
            size_t segment_start = 0;
            while (segment_start < canonical.size()) {
                const size_t sep = canonical.find("::", segment_start);
                const std::string_view segment =
                    canonical.substr(segment_start, sep - segment_start);
                if (!segment.empty()) {
                    qualifier_text_ids.push_back(
                        parser.parser_text_id_for_token(kInvalidText, segment));
                }
                if (sep == std::string_view::npos) break;
                segment_start = sep + 2;
            }
            continue;
        }
        if (ctx.text_id != kInvalidText) qualifier_text_ids.push_back(ctx.text_id);
    }

    if (qualifier_text_ids.empty()) return key;

    key.qualifier_path_id =
        create_missing_path
            ? const_cast<NamePathTable&>(
                  parser.shared_lookup_state_.parser_name_paths)
                  .intern(qualifier_text_ids)
            : parser.shared_lookup_state_.parser_name_paths.find(
                  qualifier_text_ids);
    if (key.qualifier_path_id != kInvalidNamePath) return key;

    return key;
}

QualifiedNameKey find_compatibility_key_from_rendered_qualified_spelling(
    const Parser& parser, TextId name_text_id, std::string_view name) {
    QualifiedNameKey key;
    key.context_id = 0;
    if (name.empty()) return key;

    size_t start = 0;
    if (name.rfind("::", 0) == 0) {
        key.is_global_qualified = true;
        start = 2;
    }

    std::vector<TextId> qualifier_text_ids;
    size_t segment_start = start;
    while (segment_start < name.size()) {
        const size_t sep = name.find("::", segment_start);
        if (sep == std::string_view::npos) break;
        const std::string_view segment = name.substr(segment_start, sep - segment_start);
        if (!segment.empty()) {
            qualifier_text_ids.push_back(
                parser.parser_text_id_for_token(kInvalidText, segment));
        }
        segment_start = sep + 2;
    }

    const std::string_view base = name.substr(segment_start);
    key.base_text_id = name_text_id != kInvalidText &&
                               !name.empty() &&
                               name.find("::") == std::string_view::npos
                           ? name_text_id
                           : parser.parser_text_id_for_token(kInvalidText, base);
    if (key.base_text_id == kInvalidText && !base.empty()) {
        key.base_text_id = parser.find_parser_text_id(base);
    }

    if (!qualifier_text_ids.empty()) {
        key.qualifier_path_id =
            parser.shared_lookup_state_.parser_name_paths.find(
                qualifier_text_ids);
    }
    return key;
}

QualifiedNameKey intern_compatibility_key_from_rendered_qualified_spelling(
    Parser& parser, TextId name_text_id, std::string_view name) {
    QualifiedNameKey key =
        find_compatibility_key_from_rendered_qualified_spelling(
            parser, name_text_id, name);
    if (key.qualifier_path_id != kInvalidNamePath) return key;

    std::vector<TextId> qualifier_text_ids;
    size_t segment_start = name.rfind("::", 0) == 0 ? 2 : 0;
    while (segment_start < name.size()) {
        const size_t sep = name.find("::", segment_start);
        if (sep == std::string_view::npos) break;
        const std::string_view segment =
            name.substr(segment_start, sep - segment_start);
        if (!segment.empty()) {
            qualifier_text_ids.push_back(
                parser.parser_text_id_for_token(kInvalidText, segment));
        }
        segment_start = sep + 2;
    }
    if (!qualifier_text_ids.empty()) {
        key.qualifier_path_id =
            parser.shared_lookup_state_.parser_name_paths.intern(
                qualifier_text_ids);
    }
    return key;
}

std::vector<std::string> merge_leading_top_level_qualified_probe(
    const std::vector<Parser::ParseDebugEvent>& parse_debug_events,
    const std::vector<std::string>& summary_stack) {
    if (summary_stack.size() < 2 || summary_stack.front() != "parse_top_level" ||
        !is_top_level_qualified_probe_merge_target(summary_stack[1])) {
        return summary_stack;
    }

    std::vector<std::string> leading_probe_frames;
    bool inside_top_level = false;
    const std::string& merge_anchor = summary_stack[1];
    for (const Parser::ParseDebugEvent& event : parse_debug_events) {
        if (event.kind != "enter") continue;
        if (event.function_name == "parse_top_level") {
            inside_top_level = true;
            continue;
        }
        if (!inside_top_level) continue;
        if (event.function_name == merge_anchor) break;
        if (!is_qualified_type_trace_frame(event.function_name)) continue;
        if (leading_probe_frames.empty() ||
            leading_probe_frames.back() != event.function_name) {
            leading_probe_frames.push_back(event.function_name);
        }
    }

    if (leading_probe_frames.empty()) return summary_stack;
    if (summary_stack.size() > leading_probe_frames.size() &&
        std::equal(leading_probe_frames.begin(), leading_probe_frames.end(),
                   summary_stack.begin() + 1)) {
        return summary_stack;
    }

    std::vector<std::string> merged;
    merged.reserve(summary_stack.size() + leading_probe_frames.size());
    merged.push_back(summary_stack.front());
    merged.insert(merged.end(), leading_probe_frames.begin(),
                  leading_probe_frames.end());
    merged.insert(merged.end(), summary_stack.begin() + 1, summary_stack.end());
    return merged;
}

std::string quote_debug_lexeme(const std::string& lexeme) {
    std::string escaped;
    escaped.reserve(lexeme.size());
    for (char ch : lexeme) {
        if (ch == '\\' || ch == '\'') escaped.push_back('\\');
        escaped.push_back(ch);
    }
    return escaped;
}

std::string format_debug_token_entry(const Token& token,
                                     std::string_view spelling,
                                     int index,
                                     bool highlight) {
    std::ostringstream oss;
    if (highlight) oss << ">>";
    oss << "[" << index << "] " << token_kind_name(token.kind)
        << " '" << quote_debug_lexeme(std::string(spelling)) << "'";
    return oss.str();
}

const std::string* select_best_parse_summary_leaf(
    const std::vector<std::string>& summary_stack,
    const Parser::ParseFailure& failure) {
    if (!failure.function_name.empty()) {
        for (size_t i = summary_stack.size(); i > 0; --i) {
            if (summary_stack[i - 1] != failure.function_name) continue;
            const bool trailing_param_probe_only =
                failure.function_name == "parse_top_level_parameter_list" &&
                std::all_of(summary_stack.begin() + i, summary_stack.end(),
                            [](const std::string& function_name) {
                                return is_param_qualified_type_probe_frame(
                                    function_name);
                            });
            if (trailing_param_probe_only) {
                return &summary_stack[i - 1];
            }
            const bool trailing_probe_only =
                std::all_of(summary_stack.begin() + i, summary_stack.end(),
                            [](const std::string& function_name) {
                                return is_qualified_type_trace_frame(function_name);
                            });
            if (trailing_probe_only &&
                failure.function_name != "parse_top_level") {
                return &summary_stack[i - 1];
            }
            break;
        }
    }

    for (auto it = summary_stack.rbegin(); it != summary_stack.rend(); ++it) {
        if (!is_summary_only_parse_helper(*it)) return &*it;
    }
    if (!summary_stack.empty()) return &summary_stack.back();
    return nullptr;
}

std::string build_canonical_namespace_name(const char* parent_name,
                                           const std::string& name) {
    std::string canonical = parent_name ? parent_name : "";
    if (!canonical.empty()) canonical += "::";
    canonical += name;
    return canonical;
}

template <typename LookupFn>
std::string resolve_visible_name_from_namespace_stack(
    const std::vector<int>& namespace_stack,
    const std::string& name,
    LookupFn&& lookup) {
    std::string resolved;
    for (int i = static_cast<int>(namespace_stack.size()) - 1; i >= 0; --i) {
        if (lookup(namespace_stack[i], &resolved)) return resolved;
    }
    return name;
}

template <typename FollowFn>
int resolve_namespace_id_from_stack(const std::vector<int>& namespace_stack,
                                    bool is_global_qualified,
                                    FollowFn&& follow) {
    if (is_global_qualified) return follow(0);
    for (int i = static_cast<int>(namespace_stack.size()) - 1; i >= 0; --i) {
        int resolved = follow(namespace_stack[i]);
        if (resolved >= 0) return resolved;
    }
    return follow(0);
}

}  // namespace

Parser::SymbolId Parser::symbol_id_for_token(const Token& token) {
    if (token.kind != TokenKind::Identifier) return kInvalidSymbol;
    return symbol_id_for_token_text(token.text_id);
}

std::string_view Parser::token_spelling(const Token& token) const {
    if (token.kind == TokenKind::EndOfFile) return {};
    if (shared_lookup_state_.token_texts && token.text_id != kInvalidText) {
        return shared_lookup_state_.token_texts->lookup(token.text_id);
    }
    throw std::runtime_error("token spelling requested without valid text_id");
}

void Parser::set_parser_owned_spelling(Token& token, std::string_view spelling) {
    if (!shared_lookup_state_.token_texts) {
        throw std::runtime_error("parser-owned token spelling requested without text table");
    }
    token.text_id = shared_lookup_state_.token_texts->intern(spelling);
}

Token Parser::make_injected_token(const Token& seed, TokenKind kind,
                                  std::string_view spelling) {
    Token token = seed;
    token.kind = kind;
    set_parser_owned_spelling(token, spelling);
    return token;
}

void Parser::populate_qualified_name_symbol_ids(QualifiedNameRef* name) {
    if (!name) return;
    name->qualifier_text_ids.clear();
    name->qualifier_text_ids.reserve(name->qualifier_segments.size());
    name->qualifier_symbol_ids.clear();
    name->qualifier_symbol_ids.reserve(name->qualifier_segments.size());
    for (const std::string& segment : name->qualifier_segments) {
        const TextId text_id = parser_text_id_for_token(kInvalidText, segment);
        name->qualifier_text_ids.push_back(text_id);
        name->qualifier_symbol_ids.push_back(
            shared_lookup_state_.parser_name_tables.intern_identifier(text_id));
    }
    name->base_text_id = parser_text_id_for_token(kInvalidText, name->base_name);
    name->base_symbol_id =
        name->base_name.empty()
            ? kInvalidSymbol
            : shared_lookup_state_.parser_name_tables.intern_identifier(
                  name->base_text_id);
}

bool Parser::has_typedef_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (!uses_symbol_identity(name)) {
        return name_text_id != kInvalidText &&
               binding_state_.non_atom_typedefs.count(name_text_id) > 0;
    }
    return shared_lookup_state_.parser_name_tables.is_typedef(
        shared_lookup_state_.parser_name_tables.find_identifier(name_text_id));
}

bool Parser::has_typedef_type(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (name.find("::") != std::string_view::npos) {
        const QualifiedNameKey key =
            find_compatibility_key_from_rendered_qualified_spelling(
                *this, name_text_id, name);
        if (find_typedef_type(key)) return true;
    }
    if (!uses_symbol_identity(name)) {
        return name_text_id != kInvalidText &&
               binding_state_.non_atom_typedef_types.count(name_text_id) > 0;
    }
    return shared_lookup_state_.parser_name_tables.has_typedef_type(
        shared_lookup_state_.parser_name_tables.find_identifier(name_text_id));
}

const TypeSpec* Parser::find_typedef_type(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (name.empty()) return nullptr;
    if (name.find("::") != std::string_view::npos) {
        const QualifiedNameKey key =
            find_compatibility_key_from_rendered_qualified_spelling(
                *this, name_text_id, name);
        if (const TypeSpec* structured = find_typedef_type(key)) {
            return structured;
        }
    }
    if (!uses_symbol_identity(name)) {
        if (name_text_id == kInvalidText) return nullptr;
        const auto it = binding_state_.non_atom_typedef_types.find(name_text_id);
        return it == binding_state_.non_atom_typedef_types.end() ? nullptr
                                                                 : &it->second;
    }
    return shared_lookup_state_.parser_name_tables.lookup_typedef_type(
        shared_lookup_state_.parser_name_tables.find_identifier(name_text_id));
}

const TypeSpec* Parser::find_typedef_type(const QualifiedNameKey& key) const {
    if (const TypeSpec* structured = find_structured_typedef_type(key)) {
        return structured;
    }
    if (const TypeSpec* dependent_member =
            find_dependent_record_member_typedef_type(key)) {
        return dependent_member;
    }
    return nullptr;
}

bool Parser::has_structured_typedef_type(const QualifiedNameKey& key) const {
    return find_structured_typedef_type(key) != nullptr;
}

const TypeSpec* Parser::find_structured_typedef_type(
    const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    const auto it = binding_state_.struct_typedefs.find(key);
    return it == binding_state_.struct_typedefs.end() ? nullptr : &it->second;
}

const ParserAliasTemplateInfo* Parser::find_alias_template_info(
    const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    const auto it = template_state_.alias_template_info.find(key);
    return it == template_state_.alias_template_info.end() ? nullptr
                                                           : &it->second;
}

void Parser::push_local_binding_scope() {
    lexical_scope_state_.push_scope();
}

bool Parser::pop_local_binding_scope() {
    return lexical_scope_state_.pop_scope();
}

bool Parser::has_local_binding_scope() const {
    return lexical_scope_state_.scope_depth() > 0;
}

void Parser::bind_local_typedef(TextId name_text_id, const TypeSpec& type,
                                bool is_user_typedef) {
    if (name_text_id == kInvalidText || !has_local_binding_scope()) return;
    lexical_scope_state_.visible_typedef_types.bind_in_current_scope(
        LocalNameKey{name_text_id}, type);
    if (is_user_typedef) {
        lexical_scope_state_.visible_user_typedefs.bind_in_current_scope(
            LocalNameKey{name_text_id}, true);
    }
}

void Parser::bind_local_value(TextId name_text_id, const TypeSpec& type) {
    if (name_text_id == kInvalidText || !has_local_binding_scope()) return;
    lexical_scope_state_.visible_value_types.bind_in_current_scope(
        LocalNameKey{name_text_id}, type);
}

const TypeSpec* Parser::find_local_visible_typedef_type(TextId name_text_id) const {
    if (name_text_id == kInvalidText) return nullptr;
    if (auto found = lexical_scope_state_.visible_typedef_types
                         .find_nearest_visible_text_ids(&name_text_id, 1);
        found) {
        return found.value;
    }
    return nullptr;
}

const TypeSpec* Parser::find_local_visible_var_type(TextId name_text_id) const {
    if (name_text_id == kInvalidText) return nullptr;
    if (auto found = lexical_scope_state_.visible_value_types
                         .find_nearest_visible_text_ids(&name_text_id, 1);
        found) {
        return found.value;
    }
    return nullptr;
}

bool Parser::has_local_visible_user_typedef(TextId name_text_id) const {
    if (name_text_id == kInvalidText) return false;
    return lexical_scope_state_.visible_user_typedefs
        .find_nearest_visible_text_ids(&name_text_id, 1)
        .found();
}

bool Parser::has_visible_typedef_type(TextId name_text_id) const {
    if (const TypeSpec* type = find_visible_typedef_type(name_text_id)) {
        (void)type;
        return true;
    }
    return false;
}

const TypeSpec* Parser::find_visible_typedef_type(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (can_probe_local_binding(name_text_id, name)) {
        if (const TypeSpec* type = find_local_visible_typedef_type(name_text_id)) {
            return type;
        }
    }
    if (const TypeSpec* type = find_typedef_type(name_text_id)) return type;
    const VisibleNameResult resolved_result =
        resolve_visible_type(name_text_id);
    if (!resolved_result) return nullptr;
    if (const TypeSpec* type = find_structured_typedef_type(resolved_result.key)) {
        return type;
    }
    const TextId resolved_text_id = resolved_result.base_text_id;
    const std::string_view resolved_name = parser_text(resolved_text_id, {});
    if (can_probe_local_binding(resolved_text_id, resolved_name)) {
        if (const TypeSpec* type =
                find_local_visible_typedef_type(resolved_text_id)) {
            return type;
        }
    }
    if (resolved_text_id == kInvalidText || resolved_text_id == name_text_id) {
        return nullptr;
    }
    return find_typedef_type(resolved_text_id);
}

bool typespec_has_complete_qualifier_text_ids(const TypeSpec& ts) {
    if (ts.n_qualifier_segments <= 0) return false;
    if (!ts.qualifier_text_ids) return false;
    for (int i = 0; i < ts.n_qualifier_segments; ++i) {
        if (ts.qualifier_text_ids[i] == kInvalidText) return false;
    }
    return true;
}

bool typespec_has_complete_type_name_text_identity(const TypeSpec& ts) {
    if (ts.namespace_context_id < 0 || ts.tag_text_id == kInvalidText) {
        return false;
    }
    if (ts.n_qualifier_segments < 0) return false;
    if (ts.n_qualifier_segments == 0) return true;
    return typespec_has_complete_qualifier_text_ids(ts);
}

bool same_typespec_type_name_text_identity(const TypeSpec& lhs,
                                           const TypeSpec& rhs) {
    if (lhs.namespace_context_id != rhs.namespace_context_id ||
        lhs.tag_text_id != rhs.tag_text_id ||
        lhs.is_global_qualified != rhs.is_global_qualified ||
        lhs.n_qualifier_segments != rhs.n_qualifier_segments) {
        return false;
    }
    for (int i = 0; i < lhs.n_qualifier_segments; ++i) {
        if (lhs.qualifier_text_ids[i] != rhs.qualifier_text_ids[i]) return false;
    }
    return true;
}

template <typename T>
auto typespec_legacy_nominal_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
    return ts.tag;
}

const char* typespec_legacy_nominal_tag_if_present(const TypeSpec&, long) {
    return nullptr;
}

template <typename T>
auto typespec_legacy_typedef_tag_if_present(const T& ts, int)
    -> decltype(ts.tag, static_cast<const char*>(nullptr)) {
    return ts.tag;
}

const char* typespec_legacy_typedef_tag_if_present(const TypeSpec&, long) {
    return nullptr;
}

template <typename T>
auto set_typespec_legacy_rendered_tag_if_present(T& ts, const char* tag, int)
    -> decltype(ts.tag = tag, void()) {
    ts.tag = tag;
}

void set_typespec_legacy_rendered_tag_if_present(TypeSpec&, const char*, long) {}

template <typename T>
auto set_typespec_legacy_rendered_tag_if_present(T& ts, std::string_view tag,
                                                 Arena& arena, int)
    -> decltype(ts.tag = static_cast<const char*>(nullptr), void()) {
    ts.tag = arena.strdup(std::string(tag).c_str());
}

void set_typespec_legacy_rendered_tag_if_present(TypeSpec&, std::string_view,
                                                 Arena&, long) {}

bool typespec_has_nominal_identity_carrier(const TypeSpec& ts) {
    return ts.record_def || ts.tag_text_id != kInvalidText ||
           ts.template_param_text_id != kInvalidText ||
           ts.deferred_member_type_text_id != kInvalidText;
}

bool typespec_has_typedef_lookup_identity_carrier(const TypeSpec& ts) {
    return ts.tag_text_id != kInvalidText || ts.namespace_context_id >= 0 ||
           ts.is_global_qualified || ts.n_qualifier_segments > 0;
}

bool same_legacy_tag_only_nominal_typespec_identity(const TypeSpec& lhs,
                                                    const TypeSpec& rhs) {
    if (typespec_has_nominal_identity_carrier(lhs) ||
        typespec_has_nominal_identity_carrier(rhs)) {
        return false;
    }
    const char* lhs_tag = typespec_legacy_nominal_tag_if_present(lhs, 0);
    const char* rhs_tag = typespec_legacy_nominal_tag_if_present(rhs, 0);
    return lhs_tag && rhs_tag && std::strcmp(lhs_tag, rhs_tag) == 0;
}

bool same_nominal_typespec_identity(const TypeSpec& lhs, const TypeSpec& rhs) {
    if (lhs.record_def && rhs.record_def) {
        return lhs.record_def == rhs.record_def;
    }

    const bool lhs_has_text_identity =
        typespec_has_complete_type_name_text_identity(lhs);
    const bool rhs_has_text_identity =
        typespec_has_complete_type_name_text_identity(rhs);
    if (lhs_has_text_identity || rhs_has_text_identity) {
        return lhs_has_text_identity && rhs_has_text_identity &&
               same_typespec_type_name_text_identity(lhs, rhs);
    }

    if (lhs.record_def || rhs.record_def) return false;

    if (lhs.tag_text_id != kInvalidText || rhs.tag_text_id != kInvalidText) {
        return lhs.tag_text_id != kInvalidText &&
               rhs.tag_text_id != kInvalidText &&
               lhs.tag_text_id == rhs.tag_text_id &&
               lhs.n_qualifier_segments == rhs.n_qualifier_segments &&
               lhs.namespace_context_id == rhs.namespace_context_id;
    }

    return same_legacy_tag_only_nominal_typespec_identity(lhs, rhs);
}

const TypeSpec* find_typedef_type_by_typespec_metadata(const Parser& parser,
                                                       const TypeSpec& ts) {
    if (ts.tag_text_id == kInvalidText) return nullptr;

    if (typespec_has_complete_qualifier_text_ids(ts)) {
        std::vector<TextId> qualifier_text_ids(
            ts.qualifier_text_ids,
            ts.qualifier_text_ids + ts.n_qualifier_segments);
        QualifiedNameKey key;
        key.context_id = 0;
        key.is_global_qualified = ts.is_global_qualified;
        key.base_text_id = ts.tag_text_id;
        key.qualifier_path_id =
            parser.shared_lookup_state_.parser_name_paths.find(
                qualifier_text_ids);
        if (key.qualifier_path_id != kInvalidNamePath) {
            if (const TypeSpec* type = parser.find_typedef_type(key)) {
                return type;
            }
        }
        return nullptr;
    }

    if (ts.namespace_context_id >= 0) {
        if (const TypeSpec* type = parser.find_typedef_type(
                parser.struct_typedef_key_in_context(ts.namespace_context_id,
                                                     ts.tag_text_id))) {
            return type;
        }
    }

    return parser.find_visible_typedef_type(ts.tag_text_id);
}

bool typespec_resolves_to_structured_record_ctor_type(const Parser& parser,
                                                      const TypeSpec& ts) {
    if (resolve_record_type_spec(ts, nullptr)) return true;

    if (ts.tag_text_id == kInvalidText) return false;

    if (const TypeSpec* type = find_typedef_type_by_typespec_metadata(parser, ts)) {
        TypeSpec resolved = parser.resolve_struct_like_typedef_type(*type);
        if (resolve_record_type_spec(resolved, nullptr)) return true;
        if (resolved.base == TB_STRUCT || resolved.base == TB_UNION) return true;
    }

    const int context_id = ts.namespace_context_id >= 0
                               ? ts.namespace_context_id
                               : parser.current_namespace_context_id();
    if (parser.has_template_struct_primary(context_id, ts.tag_text_id)) {
        return true;
    }
    if (context_id != 0 &&
        parser.has_template_struct_primary(0, ts.tag_text_id)) {
        return true;
    }

    for (const auto& [rendered_tag, record] :
         parser.definition_state_.struct_tag_def_map) {
        (void)rendered_tag;
        if (!record || record->kind != NK_STRUCT_DEF ||
            record->unqualified_text_id != ts.tag_text_id) {
            continue;
        }
        if (ts.namespace_context_id >= 0 &&
            record->namespace_context_id != ts.namespace_context_id) {
            continue;
        }
        return true;
    }
    return false;
}

TypeSpec Parser::resolve_typedef_type_chain(TypeSpec ts) const {
    auto find_chain_typedef = [&](const TypeSpec& type) -> const TypeSpec* {
        if (const TypeSpec* structured =
                find_typedef_type_by_typespec_metadata(*this, type)) {
            return structured;
        }
        if (typespec_has_typedef_lookup_identity_carrier(type)) return nullptr;
        const char* tag = typespec_legacy_typedef_tag_if_present(type, 0);
        if (!tag || !tag[0]) return nullptr;
        const std::string_view name(tag);
        const TextId name_text_id = find_parser_text_id(name);
        return name.find("::") == std::string_view::npos
                   ? find_visible_typedef_type(name_text_id)
                   : find_typedef_type(name_text_id);
    };
    for (int depth = 0; depth < 16; ++depth) {
        if (ts.base != TB_TYPEDEF || ts.ptr_level > 0 || ts.array_rank > 0) {
            break;
        }
        if (!typespec_has_typedef_lookup_identity_carrier(ts) &&
            !typespec_legacy_typedef_tag_if_present(ts, 0)) {
            break;
        }
        const TypeSpec* next = find_chain_typedef(ts);
        if (!next) break;
        const bool is_const = ts.is_const;
        const bool is_volatile = ts.is_volatile;
        ts = *next;
        ts.is_const |= is_const;
        ts.is_volatile |= is_volatile;
    }
    return ts;
}

TypeSpec Parser::resolve_struct_like_typedef_type(TypeSpec ts) const {
    auto find_chain_typedef = [&](const TypeSpec& type) -> const TypeSpec* {
        if (const TypeSpec* structured =
                find_typedef_type_by_typespec_metadata(*this, type)) {
            return structured;
        }
        if (typespec_has_typedef_lookup_identity_carrier(type)) return nullptr;
        const char* tag = typespec_legacy_typedef_tag_if_present(type, 0);
        if (!tag || !tag[0]) return nullptr;
        const std::string_view name(tag);
        const TextId name_text_id = find_parser_text_id(name);
        return name.find("::") == std::string_view::npos
                   ? find_visible_typedef_type(name_text_id)
                   : find_typedef_type(name_text_id);
    };
    ts = resolve_typedef_type_chain(ts);
    if (ts.base == TB_TYPEDEF &&
        (typespec_has_typedef_lookup_identity_carrier(ts) ||
         typespec_legacy_typedef_tag_if_present(ts, 0))) {
        if (const TypeSpec* typedef_type = find_chain_typedef(ts)) {
            ts = *typedef_type;
        }
    }
    return ts;
}

bool Parser::are_types_compatible(const TypeSpec& lhs,
                                  const TypeSpec& rhs) const {
    TypeSpec a = resolve_typedef_type_chain(lhs);
    TypeSpec b = resolve_typedef_type_chain(rhs);
    if (a.ptr_level == 0 && a.array_rank == 0) {
        a.is_const = false;
        a.is_volatile = false;
    }
    if (b.ptr_level == 0 && b.array_rank == 0) {
        b.is_const = false;
        b.is_volatile = false;
    }
    if (a.is_vector != b.is_vector) return false;
    if (a.is_vector &&
        (a.vector_lanes != b.vector_lanes || a.vector_bytes != b.vector_bytes)) {
        return false;
    }
    if (a.base != b.base) return false;
    if (a.ptr_level != b.ptr_level) return false;
    if (a.is_const != b.is_const || a.is_volatile != b.is_volatile) {
        return false;
    }
    if (a.array_rank != b.array_rank) return false;
    for (int i = 0; i < a.array_rank; ++i) {
        const long long lhs_dim = a.array_dims[i];
        const long long rhs_dim = b.array_dims[i];
        if (lhs_dim != rhs_dim && lhs_dim != -2 && rhs_dim != -2) return false;
    }
    if (a.base == TB_STRUCT || a.base == TB_UNION || a.base == TB_ENUM) {
        if (!same_nominal_typespec_identity(a, b)) return false;
    }
    return true;
}

bool Parser::resolves_to_record_ctor_type(TypeSpec ts) const {
    ts = resolve_struct_like_typedef_type(ts);
    if (typespec_resolves_to_structured_record_ctor_type(*this, ts)) {
        return true;
    }
    if (ts.base == TB_STRUCT || ts.base == TB_UNION) return true;
    if (ts.base != TB_TYPEDEF) return false;
    if (typespec_has_typedef_lookup_identity_carrier(ts)) return false;
    const char* tag = typespec_legacy_typedef_tag_if_present(ts, 0);
    if (!tag || !tag[0]) return false;
    // Compatibility fallback for TextId-less or tag-only paths that have not
    // carried structured record identity through the parser yet.
    return definition_state_.defined_struct_tags.count(tag) > 0 ||
           definition_state_.struct_tag_def_map.count(tag) > 0 ||
           has_template_struct_primary(
               current_namespace_context_id(),
               parser_text_id_for_token(kInvalidText, tag));
}

bool Parser::is_user_typedef_name(TextId name_text_id) const {
    if (has_local_visible_user_typedef(name_text_id)) return true;
    const std::string_view name = parser_text(name_text_id, {});
    if (!uses_symbol_identity(name)) {
        return name_text_id != kInvalidText &&
               binding_state_.non_atom_user_typedefs.count(name_text_id) > 0;
    }
    const SymbolId id =
        shared_lookup_state_.parser_name_tables.find_identifier(name_text_id);
    return id != kInvalidSymbol &&
           shared_lookup_state_.parser_name_tables.user_typedefs.count(id) > 0;
}

bool Parser::has_conflicting_user_typedef_binding(TextId name_text_id,
                                                  const TypeSpec& type) const {
    const TypeSpec* existing_typedef = find_visible_typedef_type(name_text_id);
    return is_user_typedef_name(name_text_id) && existing_typedef &&
           !are_types_compatible(*existing_typedef, type);
}

void Parser::register_typedef_name(TextId name_text_id, bool is_user_typedef) {
    const std::string_view resolved_name = parser_text(name_text_id, {});
    if (!uses_symbol_identity(resolved_name)) {
        if (name_text_id == kInvalidText) return;
        binding_state_.non_atom_typedefs.insert(name_text_id);
        if (is_user_typedef) binding_state_.non_atom_user_typedefs.insert(name_text_id);
        return;
    }
    const SymbolId id =
        shared_lookup_state_.parser_name_tables.intern_identifier(name_text_id);
    if (id == kInvalidSymbol) return;
    shared_lookup_state_.parser_name_tables.typedefs.insert(id);
    if (is_user_typedef) shared_lookup_state_.parser_name_tables.user_typedefs.insert(id);
}

void Parser::register_typedef_binding(TextId name_text_id,
                                      const TypeSpec& type,
                                      bool is_user_typedef) {
    const std::string_view resolved_name = parser_text(name_text_id, {});
    const TextId binding_name_id = name_text_id;
    if (should_track_local_binding(*this, binding_name_id, resolved_name)) {
        bind_local_typedef(binding_name_id, type, is_user_typedef);
        return;
    }
    if (!uses_symbol_identity(resolved_name)) {
        if (binding_name_id == kInvalidText) return;
        binding_state_.non_atom_typedefs.insert(binding_name_id);
        if (is_user_typedef) {
            binding_state_.non_atom_user_typedefs.insert(binding_name_id);
        }
        binding_state_.non_atom_typedef_types[binding_name_id] = type;
        return;
    }
    const SymbolId id =
        shared_lookup_state_.parser_name_tables.intern_identifier(binding_name_id);
    if (id == kInvalidSymbol) return;
    shared_lookup_state_.parser_name_tables.typedefs.insert(id);
    if (is_user_typedef) shared_lookup_state_.parser_name_tables.user_typedefs.insert(id);
    shared_lookup_state_.parser_name_tables.typedef_types[id] = type;
}

void Parser::unregister_typedef_binding(TextId name_text_id) {
    const std::string_view name = parser_text(name_text_id, {});
    if (!uses_symbol_identity(name)) {
        if (name_text_id == kInvalidText) return;
        binding_state_.non_atom_typedefs.erase(name_text_id);
        binding_state_.non_atom_user_typedefs.erase(name_text_id);
        binding_state_.non_atom_typedef_types.erase(name_text_id);
        return;
    }
    const SymbolId id = shared_lookup_state_.parser_name_tables.find_identifier(name_text_id);
    if (id == kInvalidSymbol) return;
    shared_lookup_state_.parser_name_tables.typedefs.erase(id);
    shared_lookup_state_.parser_name_tables.user_typedefs.erase(id);
    shared_lookup_state_.parser_name_tables.typedef_types.erase(id);
}

void Parser::register_synthesized_typedef_binding(TextId name_text_id) {
    const std::string_view resolved_name = parser_text(name_text_id, {});
    TypeSpec synthesized_ts{};
    synthesized_ts.array_size = -1;
    synthesized_ts.inner_rank = -1;
    synthesized_ts.base = TB_TYPEDEF;
    synthesized_ts.tag_text_id = name_text_id;
    set_typespec_legacy_rendered_tag_if_present(synthesized_ts, resolved_name,
                                                arena_, 0);
    register_typedef_binding(name_text_id, synthesized_ts, false);
}

void Parser::register_tag_type_binding(TextId name_text_id,
                                       TypeBase base,
                                       const char* tag,
                                       TextId tag_text_id,
                                       TypeBase enum_underlying_base) {
    if (name_text_id == kInvalidText || !tag || !tag[0]) return;

    TypeSpec tagged_ts{};
    tagged_ts.array_size = -1;
    tagged_ts.array_rank = 0;
    tagged_ts.base = base;
    tagged_ts.tag_text_id =
        tag_text_id != kInvalidText ? tag_text_id
                                    : parser_text_id_for_token(kInvalidText, tag);
    set_typespec_legacy_rendered_tag_if_present(tagged_ts, tag, 0);
    if (base == TB_ENUM) {
        tagged_ts.inner_rank = -1;
        for (int i = 0; i < 8; ++i) tagged_ts.array_dims[i] = -1;
        tagged_ts.enum_underlying_base = enum_underlying_base;
    }
    register_typedef_binding(name_text_id, tagged_ts, false);
}

void Parser::cache_typedef_type(const std::string& name, const TypeSpec& type) {
    const TextId local_name_id = parser_text_id_for_token(kInvalidText, name);
    if (should_track_local_binding(*this, local_name_id, name)) {
        bind_local_typedef(local_name_id, type);
        return;
    }
    if (!uses_symbol_identity(name)) {
        const TextId id = parser_text_id_for_token(kInvalidText, name);
        if (id == kInvalidText) return;
        binding_state_.non_atom_typedef_types[id] = type;
        return;
    }
    const SymbolId id =
        shared_lookup_state_.parser_name_tables.intern_identifier(name);
    if (id == kInvalidSymbol) return;
    shared_lookup_state_.parser_name_tables.typedef_types[id] = type;
}

void Parser::register_template_instantiation_member_typedef_binding(
    const ParserTemplateState::TemplateInstantiationKey& concrete_owner,
    TextId member_text_id, const TypeSpec& type) {
    if (concrete_owner.template_key.base_text_id == kInvalidText ||
        member_text_id == kInvalidText) {
        return;
    }
    ParserTemplateState::TemplateInstantiationMemberTypedefKey key;
    key.concrete_owner = concrete_owner;
    key.member_text_id = member_text_id;
    template_state_.template_instantiation_member_typedefs_by_key[key] = type;
}

void Parser::register_dependent_record_member_typedef_binding(
    const QualifiedNameKey& owner_key,
    TextId member_text_id,
    const TypeSpec& type) {
    if (owner_key.base_text_id == kInvalidText ||
        member_text_id == kInvalidText) {
        return;
    }
    ParserTemplateState::DependentRecordMemberTypedefKey key;
    key.owner_key = owner_key;
    key.member_text_id = member_text_id;
    template_state_.dependent_record_member_typedefs_by_owner[key] = type;
}

void Parser::register_record_member_typedef_info(
    const QualifiedNameKey& key,
    const ParserAliasTemplateMemberTypedefInfo& info) {
    if (key.base_text_id == kInvalidText || !info.valid) return;
    template_state_.record_member_typedef_infos_by_key[key] = info;
}

void Parser::register_structured_typedef_binding(
    const QualifiedNameKey& key, const TypeSpec& type) {
    if (key.base_text_id == kInvalidText) return;
    binding_state_.struct_typedefs[key] = type;
}

void Parser::register_structured_typedef_binding_in_context(
    int context_id, TextId name_text_id, const TypeSpec& type) {
    const QualifiedNameKey key = qualified_key_in_context(
        *this, context_id, name_text_id, true);
    register_structured_typedef_binding(key, type);
}

const TypeSpec* Parser::find_var_type(TextId name_text_id) const {
    if (name_text_id == kInvalidText) return nullptr;
    const auto text_it = binding_state_.var_types_by_text_id.find(name_text_id);
    if (text_it != binding_state_.var_types_by_text_id.end()) {
        return &text_it->second;
    }
    const SymbolId id =
        shared_lookup_state_.parser_name_tables.find_identifier(name_text_id);
    if (id == kInvalidSymbol) return nullptr;
    const auto it = shared_lookup_state_.parser_name_tables.var_types.find(id);
    if (it == shared_lookup_state_.parser_name_tables.var_types.end()) return nullptr;
    return &it->second;
}

const TypeSpec* Parser::find_var_type(const QualifiedNameKey& key) const {
    if (const TypeSpec* structured = find_structured_var_type(key)) {
        return structured;
    }
    return nullptr;
}

bool Parser::has_structured_var_type(const QualifiedNameKey& key) const {
    return find_structured_var_type(key) != nullptr;
}

const TypeSpec* Parser::find_structured_var_type(
    const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    const auto it = binding_state_.value_bindings.find(key);
    return it == binding_state_.value_bindings.end() ? nullptr : &it->second;
}

const TypeSpec* Parser::find_visible_var_type(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (can_probe_local_binding(name_text_id, name)) {
        if (const TypeSpec* type = find_local_visible_var_type(name_text_id)) {
            return type;
        }
    }
    if (const TypeSpec* type = find_var_type(name_text_id)) return type;
    const VisibleNameResult resolved_result =
        resolve_visible_value(name_text_id);
    if (!resolved_result) return nullptr;
    if (const TypeSpec* type = find_structured_var_type(resolved_result.key)) {
        return type;
    }
    const TextId resolved_text_id = resolved_result.base_text_id;
    const std::string_view resolved_name = parser_text(resolved_text_id, {});
    if (can_probe_local_binding(resolved_text_id, resolved_name)) {
        if (const TypeSpec* type = find_local_visible_var_type(resolved_text_id)) {
            return type;
        }
    }
    if (resolved_text_id == kInvalidText || resolved_text_id == name_text_id) {
        return nullptr;
    }
    return find_var_type(resolved_text_id);
}

void Parser::register_var_type_binding(TextId name_text_id,
                                       const TypeSpec& type) {
    const std::string_view resolved_name = parser_text(name_text_id, {});
    const TextId local_name_id = name_text_id;
    if (should_track_local_binding(*this, local_name_id, resolved_name)) {
        bind_local_value(local_name_id, type);
        return;
    }
    const QualifiedNameKey key =
        resolved_name.find("::") == std::string_view::npos
            ? qualified_key_in_context(*this, 0, local_name_id, true)
            : intern_compatibility_key_from_rendered_qualified_spelling(
                  *this, local_name_id, resolved_name);
    if (key.base_text_id != kInvalidText) {
        binding_state_.value_bindings[key] = type;
    }
    if (local_name_id != kInvalidText) {
        binding_state_.var_types_by_text_id[local_name_id] = type;
    }
}

void Parser::register_structured_var_type_binding(const QualifiedNameKey& key,
                                                  const TypeSpec& type) {
    if (key.base_text_id == kInvalidText) return;
    binding_state_.value_bindings[key] = type;
}

void Parser::register_structured_var_type_binding_in_context(
    int context_id, TextId name_text_id, const TypeSpec& type) {
    const QualifiedNameKey key = qualified_key_in_context(
        *this, context_id, name_text_id, true);
    register_structured_var_type_binding(key, type);
}

QualifiedNameKey Parser::known_fn_name_key(int context_id,
                                           TextId name_text_id) const {
    return qualified_key_in_context(*this, context_id, name_text_id, false);
}

QualifiedNameKey Parser::intern_semantic_name_key(TextId name_text_id) {
    return qualified_key_in_context(*this, 0, name_text_id, true);
}

QualifiedNameKey Parser::known_fn_name_key_in_context(
    int context_id, TextId name_text_id) const {
    return qualified_key_in_context(*this, context_id, name_text_id, false);
}

QualifiedNameKey Parser::current_record_member_name_key(
    TextId member_text_id) const {
    QualifiedNameKey key;
    key.context_id = 0;
    if (!is_cpp_mode() || active_context_state_.current_struct_tag.empty()) {
        return key;
    }

    const TextId base_text_id = member_text_id;
    if (base_text_id == kInvalidText) return key;

    const std::string_view record_name = current_struct_tag_text();
    if (record_name.empty()) return key;

    std::vector<TextId> qualifier_text_ids;
    size_t segment_start = 0;
    while (segment_start < record_name.size()) {
        const size_t sep = record_name.find("::", segment_start);
        const std::string_view segment =
            sep == std::string_view::npos
                ? record_name.substr(segment_start)
                : record_name.substr(segment_start, sep - segment_start);
        if (segment.empty()) return {};
        TextId segment_text_id = kInvalidText;
        if (sep == std::string_view::npos &&
            segment_start == 0 &&
            active_context_state_.current_struct_tag_text_id !=
                kInvalidText) {
            segment_text_id = active_context_state_.current_struct_tag_text_id;
        } else {
            segment_text_id = find_parser_text_id(segment);
        }
        if (segment_text_id == kInvalidText) return {};
        qualifier_text_ids.push_back(segment_text_id);
        if (sep == std::string_view::npos) break;
        segment_start = sep + 2;
    }
    if (qualifier_text_ids.empty()) return key;

    key.qualifier_path_id =
        shared_lookup_state_.parser_name_paths.find(qualifier_text_ids);
    if (key.qualifier_path_id == kInvalidNamePath) return {};
    key.base_text_id = base_text_id;
    return key;
}

QualifiedNameKey Parser::record_member_typedef_key_in_context(
    int context_id, TextId record_text_id, TextId member_text_id) {
    QualifiedNameKey key;
    if (record_text_id == kInvalidText || member_text_id == kInvalidText)
        return key;

    const QualifiedNameKey record_key =
        qualified_key_in_context(*this, context_id, record_text_id, true);
    if (record_key.base_text_id == kInvalidText) return key;

    key.context_id = 0;
    key.is_global_qualified = record_key.is_global_qualified;
    key.qualifier_path_id = shared_lookup_state_.parser_name_paths.append(
        record_key.qualifier_path_id, record_key.base_text_id);
    key.base_text_id = member_text_id;
    return key;
}

QualifiedNameKey Parser::record_member_typedef_owner_key_from_member_key(
    const QualifiedNameKey& key) const {
    QualifiedNameKey owner_key;
    if (key.base_text_id == kInvalidText ||
        key.qualifier_path_id == kInvalidNamePath) {
        return owner_key;
    }

    const NamePathTable::View path =
        shared_lookup_state_.parser_name_paths.lookup(key.qualifier_path_id);
    if (path.empty()) return owner_key;

    owner_key.context_id = key.context_id;
    owner_key.is_global_qualified = key.is_global_qualified;
    owner_key.base_text_id = path[path.size - 1];
    if (owner_key.base_text_id == kInvalidText) return {};

    if (path.size > 1) {
        owner_key.qualifier_path_id =
            shared_lookup_state_.parser_name_paths.find(path.data,
                                                        path.size - 1);
        if (owner_key.qualifier_path_id == kInvalidNamePath) return {};
    }
    return owner_key;
}

const TypeSpec* Parser::find_dependent_record_member_typedef_type(
    const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    const QualifiedNameKey owner_key =
        record_member_typedef_owner_key_from_member_key(key);
    return find_dependent_record_member_typedef_type(owner_key,
                                                     key.base_text_id);
}

const TypeSpec* Parser::find_dependent_record_member_typedef_type(
    const QualifiedNameKey& owner_key,
    TextId member_text_id) const {
    if (owner_key.base_text_id == kInvalidText ||
        member_text_id == kInvalidText) {
        return nullptr;
    }
    ParserTemplateState::DependentRecordMemberTypedefKey key;
    key.owner_key = owner_key;
    key.member_text_id = member_text_id;
    const auto it =
        template_state_.dependent_record_member_typedefs_by_owner.find(key);
    return it ==
                   template_state_.dependent_record_member_typedefs_by_owner
                       .end()
               ? nullptr
               : &it->second;
}

const ParserAliasTemplateMemberTypedefInfo*
Parser::find_record_member_typedef_info(const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    const auto it = template_state_.record_member_typedef_infos_by_key.find(key);
    return it == template_state_.record_member_typedef_infos_by_key.end()
               ? nullptr
               : &it->second;
}

const TypeSpec* Parser::find_template_instantiation_member_typedef_type(
    const ParserTemplateState::TemplateInstantiationKey& concrete_owner,
    TextId member_text_id) const {
    if (concrete_owner.template_key.base_text_id == kInvalidText ||
        member_text_id == kInvalidText) {
        return nullptr;
    }
    ParserTemplateState::TemplateInstantiationMemberTypedefKey key;
    key.concrete_owner = concrete_owner;
    key.member_text_id = member_text_id;
    const auto it =
        template_state_.template_instantiation_member_typedefs_by_key.find(key);
    return it == template_state_.template_instantiation_member_typedefs_by_key.end()
               ? nullptr
               : &it->second;
}

QualifiedNameKey Parser::struct_typedef_key_in_context(
    int context_id, TextId name_text_id) const {
    return qualified_key_in_context(*this, context_id, name_text_id, false);
}

QualifiedNameKey Parser::alias_template_key_in_context(
    int context_id, TextId name_text_id) const {
    return qualified_key_in_context(*this, context_id, name_text_id, true);
}

QualifiedNameKey Parser::qualified_name_key(const QualifiedNameRef& name) {
    QualifiedNameKey key;
    key.context_id = 0;
    key.is_global_qualified = name.is_global_qualified;
    key.base_text_id = name.base_text_id != kInvalidText
                           ? name.base_text_id
                           : parser_text_id_for_token(kInvalidText, name.base_name);
    if (key.base_text_id == kInvalidText) return key;

    if (!name.qualifier_text_ids.empty()) {
        key.qualifier_path_id =
            shared_lookup_state_.parser_name_paths.intern(name.qualifier_text_ids);
    }
    return key;
}

bool Parser::has_known_fn_name(const QualifiedNameKey& key) const {
    return binding_state_.known_fn_names.count(key) > 0;
}

void Parser::register_known_fn_name(const QualifiedNameKey& key) {
    binding_state_.known_fn_names.insert(key);
}

bool Parser::register_known_fn_name_in_context(int context_id,
                                               TextId name_text_id) {
    const std::string_view text_name =
        name_text_id != kInvalidText ? parser_text(name_text_id, {}) : "";
    if (!text_name.empty() &&
        text_name.find("::") == std::string_view::npos) {
        const QualifiedNameKey key = qualified_key_in_context(
            *this, context_id, name_text_id, true);
        if (key.base_text_id != kInvalidText) {
            register_known_fn_name(key);
            return true;
        }
    }

    const QualifiedNameKey structured_key = qualified_key_in_context(
        *this, context_id, name_text_id, true);
    if (structured_key.base_text_id != kInvalidText) {
        register_known_fn_name(structured_key);
        if (!text_name.empty() &&
            text_name.find("::") != std::string_view::npos) {
            const QualifiedNameKey compatibility_key =
                intern_compatibility_key_from_rendered_qualified_spelling(
                    *this, name_text_id, text_name);
            if (compatibility_key.base_text_id != kInvalidText) {
                register_known_fn_name(compatibility_key);
            }
        }
        return true;
    }
    return false;
}

bool Parser::has_structured_concept_name(const QualifiedNameKey& key) const {
    return key.base_text_id != kInvalidText &&
           binding_state_.concept_qualified_names.count(key) > 0;
}

const ParserAliasTemplateInfo* Parser::find_alias_template_info_in_context(
    int context_id, TextId name_text_id) const {
    if (context_id < 0) return nullptr;

    const QualifiedNameKey key = alias_template_key_in_context(
        context_id, name_text_id);
    if (const ParserAliasTemplateInfo* structured =
            find_alias_template_info(key)) {
        return structured;
    }
    return nullptr;
}

void Parser::register_concept_name_in_context(int context_id,
                                              TextId name_text_id) {
    const QualifiedNameKey key = qualified_key_in_context(
        *this, context_id, name_text_id, true);
    if (key.base_text_id == kInvalidText) return;
    binding_state_.concept_qualified_names.insert(key);
}

ParserParseContextGuard::ParserParseContextGuard(
    Parser* parser_in, const char* function_name)
    : parser(parser_in) {
    if (parser) parser->push_parse_context(function_name);
}

ParserParseContextGuard::~ParserParseContextGuard() {
    if (parser) parser->pop_parse_context();
}

ParserTentativeParseGuard::ParserTentativeParseGuard(Parser& p)
    : parser(p), snapshot(p.save_state()), start_pos(snapshot.lite.pos) {
    parser.note_tentative_parse_event(ParserTentativeParseMode::Heavy,
                                      "tentative_enter", start_pos,
                                      start_pos);
}

ParserTentativeParseGuard::~ParserTentativeParseGuard() {
    if (!committed) {
        parser.note_tentative_parse_event(ParserTentativeParseMode::Heavy,
                                      "tentative_rollback", start_pos,
                                          parser.core_input_state_.pos);
        parser.restore_state(snapshot);
    }
}

void ParserTentativeParseGuard::commit() {
    if (committed) return;
    parser.note_tentative_parse_event(ParserTentativeParseMode::Heavy,
                                      "tentative_commit", start_pos,
                                      parser.core_input_state_.pos);
    committed = true;
}

ParserTentativeParseGuardLite::ParserTentativeParseGuardLite(Parser& p)
    : parser(p), snapshot(p.save_lite_state()), start_pos(snapshot.pos) {
    parser.note_tentative_parse_event(ParserTentativeParseMode::Lite,
                                      "tentative_enter", start_pos,
                                      start_pos);
}

ParserTentativeParseGuardLite::~ParserTentativeParseGuardLite() {
    if (!committed) {
        parser.note_tentative_parse_event(ParserTentativeParseMode::Lite,
                                          "tentative_rollback", start_pos,
                                          parser.core_input_state_.pos);
        parser.restore_lite_state(snapshot);
    }
}

void ParserTentativeParseGuardLite::commit() {
    if (committed) return;
    parser.note_tentative_parse_event(ParserTentativeParseMode::Lite,
                                      "tentative_commit", start_pos,
                                      parser.core_input_state_.pos);
    committed = true;
}

ParserLocalVarBindingSuppressionGuard::ParserLocalVarBindingSuppressionGuard(
    Parser& p)
    : parser(p), old(p.active_context_state_.suppress_local_var_bindings) {
    parser.active_context_state_.suppress_local_var_bindings = true;
}

ParserLocalVarBindingSuppressionGuard::
    ~ParserLocalVarBindingSuppressionGuard() {
    parser.active_context_state_.suppress_local_var_bindings = old;
}

ParserRecordTemplatePreludeGuard::ParserRecordTemplatePreludeGuard(Parser* p)
    : parser(p) {}

ParserRecordTemplatePreludeGuard::~ParserRecordTemplatePreludeGuard() {
    if (!parser) return;
    if (pushed_template_scope &&
        !parser->template_state_.template_scope_stack.empty()) {
        parser->template_state_.template_scope_stack.pop_back();
    }
    for (const ParserInjectedTemplateParam& param : injected_type_params) {
        if (!param.registered_typedef) continue;
        parser->unregister_typedef_binding(param.name_text_id);
    }
}

ParserTemplateDeclarationPreludeGuard::ParserTemplateDeclarationPreludeGuard(
    Parser* p)
    : parser(p) {}

ParserTemplateDeclarationPreludeGuard::
    ~ParserTemplateDeclarationPreludeGuard() {
    if (!parser) return;
    if (pushed_template_scope &&
        !parser->template_state_.template_scope_stack.empty()) {
        parser->template_state_.template_scope_stack.pop_back();
    }
    for (const ParserInjectedTemplateParam& param : injected_type_params) {
        if (!param.registered_typedef) continue;
        parser->unregister_typedef_binding(param.name_text_id);
    }
}

Parser::Parser(std::vector<Token> tokens, Arena& arena,
               TextTable* token_texts,
               FileTable* token_files,
               SourceProfile source_profile,
               const std::string& source_file)
    : impl_(std::make_unique<ParserImpl>(std::move(tokens), arena, token_texts,
                                         token_files, source_profile,
                                         source_file)),
      core_input_state_(impl_->core_input_state),
      tokens_(core_input_state_.tokens),
      pos_(core_input_state_.pos),
      arena_(core_input_state_.arena),
      shared_lookup_state_(impl_->shared_lookup_state),
      binding_state_(impl_->binding_state),
      definition_state_(impl_->definition_state),
      template_state_(impl_->template_state),
      lexical_scope_state_(impl_->lexical_scope_state),
      active_context_state_(impl_->active_context_state),
      namespace_state_(impl_->namespace_state),
      diagnostic_state_(impl_->diagnostic_state),
      pragma_state_(impl_->pragma_state) {
    namespace_state_.namespace_contexts.push_back(
        NamespaceContext{0, -1, false, kInvalidText, arena_.strdup(""),
                         arena_.strdup("")});
    namespace_state_.namespace_stack.push_back(0);
    // Pre-seed well-known typedef names from standard / system headers
    // so the parser can disambiguate type-name vs identifier.
    static const char* seed[] = {
        "size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
        "uint_least8_t","uint_least16_t","uint_least32_t","uint_least64_t",
        "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t",
        "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t",
        "intmax_t", "uintmax_t",
        "off_t", "pid_t", "uid_t", "gid_t",
        "time_t", "clock_t", "suseconds_t",
        "FILE", "DIR", "fpos_t",
        "va_list", "__va_list", "__va_list_tag",
        "wchar_t", "wint_t", "wctype_t",
        "bool",
        "NULL",
        "__builtin_va_list", "__gnuc_va_list",
        "jmp_buf", "sigjmp_buf",
        "pthread_t", "pthread_mutex_t", "pthread_cond_t",
        "socklen_t", "in_addr_t", "in_port_t",
        "mode_t", "ino_t", "dev_t", "nlink_t", "blksize_t", "blkcnt_t",
        "div_t", "ldiv_t", "lldiv_t",
        "__int128_t", "__uint128_t",
        "__Float32x4_t", "__Float64x2_t",
        "__SVFloat32_t", "__SVFloat64_t", "__SVBool_t",
        "OSStatus", "OSErr",
        "__true_type", "__false_type",
        nullptr
    };
    for (int i = 0; seed[i]; ++i) {
        register_typedef_name(parser_text_id_for_token(kInvalidText, seed[i]),
                              false);
    }

    // Seed grouped typedef type storage for well-known names so they resolve
    // to correct LLVM types.
    // instead of the TB_TYPEDEF fallback (which emits i32).
    {
        TypeSpec va_ts{};
        va_ts.array_size = -1;
        va_ts.array_rank = 0;
        va_ts.is_ptr_to_array = false;
        va_ts.base = TB_VA_LIST;
        cache_typedef_type("va_list", va_ts);
        cache_typedef_type("__va_list", va_ts);
        cache_typedef_type("__builtin_va_list", va_ts);
        cache_typedef_type("__gnuc_va_list", va_ts);

        // size_t / uintptr_t etc. → u64 on 64-bit platforms
        TypeSpec u64_ts{};
        u64_ts.array_size = -1;
        u64_ts.array_rank = 0;
        u64_ts.is_ptr_to_array = false;
        u64_ts.base = TB_ULONGLONG;  // 64-bit unsigned
        cache_typedef_type("size_t", u64_ts);
        cache_typedef_type("uintptr_t", u64_ts);
        cache_typedef_type("uintmax_t", u64_ts);
        cache_typedef_type("uint64_t", u64_ts);
        cache_typedef_type("uint_least64_t", u64_ts);
        cache_typedef_type("uint_fast64_t", u64_ts);

        TypeSpec i64_ts{};
        i64_ts.array_size = -1;
        i64_ts.array_rank = 0;
        i64_ts.is_ptr_to_array = false;
        i64_ts.base = TB_LONGLONG;
        cache_typedef_type("ssize_t", i64_ts);
        cache_typedef_type("ptrdiff_t", i64_ts);
        cache_typedef_type("intptr_t", i64_ts);
        cache_typedef_type("intmax_t", i64_ts);
        cache_typedef_type("int64_t", i64_ts);
        cache_typedef_type("int_least64_t", i64_ts);
        cache_typedef_type("int_fast64_t", i64_ts);
        cache_typedef_type("off_t", i64_ts);

        TypeSpec u32_ts{};
        u32_ts.array_size = -1;
        u32_ts.array_rank = 0;
        u32_ts.is_ptr_to_array = false;
        u32_ts.base = TB_UINT;
        cache_typedef_type("uint32_t", u32_ts);
        cache_typedef_type("uint_least32_t", u32_ts);
        cache_typedef_type("uint_fast32_t", u32_ts);

        TypeSpec i32_ts{};
        i32_ts.array_size = -1;
        i32_ts.array_rank = 0;
        i32_ts.is_ptr_to_array = false;
        i32_ts.base = TB_INT;
        cache_typedef_type("int32_t", i32_ts);
        cache_typedef_type("int_least32_t", i32_ts);
        cache_typedef_type("int_fast32_t", i32_ts);

        TypeSpec u16_ts{};
        u16_ts.array_size = -1;
        u16_ts.array_rank = 0;
        u16_ts.is_ptr_to_array = false;
        u16_ts.base = TB_USHORT;
        cache_typedef_type("uint16_t", u16_ts);
        cache_typedef_type("uint_least16_t", u16_ts);
        cache_typedef_type("uint_fast16_t", u16_ts);

        TypeSpec i16_ts{};
        i16_ts.array_size = -1;
        i16_ts.array_rank = 0;
        i16_ts.is_ptr_to_array = false;
        i16_ts.base = TB_SHORT;
        cache_typedef_type("int16_t", i16_ts);
        cache_typedef_type("int_least16_t", i16_ts);
        cache_typedef_type("int_fast16_t", i16_ts);

        TypeSpec u8_ts{};
        u8_ts.array_size = -1;
        u8_ts.array_rank = 0;
        u8_ts.is_ptr_to_array = false;
        u8_ts.base = TB_UCHAR;
        cache_typedef_type("uint8_t", u8_ts);
        cache_typedef_type("uint_least8_t", u8_ts);
        cache_typedef_type("uint_fast8_t", u8_ts);

        TypeSpec i8_ts{};
        i8_ts.array_size = -1;
        i8_ts.array_rank = 0;
        i8_ts.is_ptr_to_array = false;
        i8_ts.base = TB_SCHAR;
        cache_typedef_type("int8_t", i8_ts);
        cache_typedef_type("int_least8_t", i8_ts);
        cache_typedef_type("int_fast8_t", i8_ts);

        // wchar_t on macOS/ARM64 is i32
        TypeSpec wchar_ts{};
        wchar_ts.array_size = -1;
        wchar_ts.array_rank = 0;
        wchar_ts.is_ptr_to_array = false;
        wchar_ts.base = TB_INT;
        cache_typedef_type("wchar_t", wchar_ts);
        cache_typedef_type("wint_t", wchar_ts);

        TypeSpec true_ts{};
        true_ts.array_size = -1;
        true_ts.array_rank = 0;
        true_ts.is_ptr_to_array = false;
        true_ts.base = TB_STRUCT;
        true_ts.tag_text_id = parser_text_id_for_token(kInvalidText, "__true_type");
        set_typespec_legacy_rendered_tag_if_present(true_ts, "__true_type", 0);
        cache_typedef_type("__true_type", true_ts);
        cache_typedef_type("std::__true_type", true_ts);
        cache_typedef_type("std::__8::__true_type", true_ts);

        TypeSpec false_ts{};
        false_ts.array_size = -1;
        false_ts.array_rank = 0;
        false_ts.is_ptr_to_array = false;
        false_ts.base = TB_STRUCT;
        false_ts.tag_text_id = parser_text_id_for_token(kInvalidText, "__false_type");
        set_typespec_legacy_rendered_tag_if_present(false_ts, "__false_type", 0);
        cache_typedef_type("__false_type", false_ts);
        cache_typedef_type("std::__false_type", false_ts);
        cache_typedef_type("std::__8::__false_type", false_ts);
    }
    refresh_current_namespace_bridge();
}

Parser::~Parser() = default;

// ── pragma helpers ────────────────────────────────────────────────────────────

void handle_pragma_pack(Parser& parser, const std::string& args) {
    // #pragma pack(N)        — set pack alignment to N
    // #pragma pack()         — reset to default (0)
    // #pragma pack(push)     — push current alignment
    // #pragma pack(push,N)   — push current alignment, set to N
    // #pragma pack(pop)      — pop previous alignment
    // #pragma pack(pop,N)    — pop and set to N
    // The lexeme has whitespace stripped and contains just the args, e.g. "1", "push,2", "pop", ""

    if (args.empty()) {
        parser.pragma_state_.pack_alignment = 0;
        return;
    }

    if (args.substr(0, 4) == "push") {
        parser.pragma_state_.pack_stack.push_back(
            parser.pragma_state_.pack_alignment);
        if (args.size() > 4 && args[4] == ',') {
            parser.pragma_state_.pack_alignment = std::stoi(args.substr(5));
        }
    } else if (args.substr(0, 3) == "pop") {
        if (!parser.pragma_state_.pack_stack.empty()) {
            parser.pragma_state_.pack_alignment =
                parser.pragma_state_.pack_stack.back();
            parser.pragma_state_.pack_stack.pop_back();
        } else {
            parser.pragma_state_.pack_alignment = 0;
        }
        if (args.size() > 3 && args[3] == ',') {
            parser.pragma_state_.pack_alignment = std::stoi(args.substr(4));
        }
    } else {
        // Simple numeric value
        parser.pragma_state_.pack_alignment = std::stoi(args);
    }
}

void handle_pragma_exec(Parser& parser, const std::string& args) {
    if (args == "host") {
        parser.pragma_state_.execution_domain = ExecutionDomain::Host;
    } else if (args == "device") {
        parser.pragma_state_.execution_domain = ExecutionDomain::Device;
    }
}

void Parser::set_parser_debug(bool enabled) {
    diagnostic_state_.parser_debug_channels =
        enabled ? ParseDebugAll : ParseDebugNone;
}

void Parser::set_parser_debug_channels(unsigned channels) {
    diagnostic_state_.parser_debug_channels = channels;
}

bool Parser::had_parse_error() const {
    return diagnostic_state_.had_error;
}

bool Parser::parser_debug_enabled() const {
    return diagnostic_state_.parser_debug_channels != ParseDebugNone;
}

bool Parser::parse_debug_event_visible(const char* kind) const {
    if (!parser_debug_enabled()) return false;
    if (!kind)
        return (diagnostic_state_.parser_debug_channels & ParseDebugGeneral) !=
               0;
    if (std::strncmp(kind, "tentative_", 10) == 0) {
        return (diagnostic_state_.parser_debug_channels &
                ParseDebugTentative) != 0;
    }
    if (std::strncmp(kind, "injected_parse_", 15) == 0) {
        return (diagnostic_state_.parser_debug_channels &
                ParseDebugInjected) != 0;
    }
    return (diagnostic_state_.parser_debug_channels & ParseDebugGeneral) != 0;
}

void Parser::clear_parse_debug_state() {
    diagnostic_state_.parse_context_stack.clear();
    diagnostic_state_.parse_debug_events.clear();
    diagnostic_state_.best_parse_failure = ParseFailure{};
    diagnostic_state_.best_parse_stack_token_index = -1;
    diagnostic_state_.best_parse_stack_trace.clear();
}

void Parser::reset_parse_debug_progress() {
    diagnostic_state_.parse_debug_started_at = {};
    diagnostic_state_.parse_debug_last_progress_at = {};
}

void Parser::push_parse_context(const char* function_name) {
    ParseContextFrame frame;
    frame.function_name = function_name ? function_name : "";
    frame.token_index = core_input_state_.pos;
    diagnostic_state_.parse_context_stack.push_back(std::move(frame));
    note_parse_debug_event("enter");
}

void Parser::pop_parse_context() {
    note_parse_debug_event("leave");
    if (!diagnostic_state_.parse_context_stack.empty()) {
        diagnostic_state_.parse_context_stack.pop_back();
    }
}

void Parser::note_parse_debug_event(const char* kind, const char* detail) {
    note_parse_debug_event_for(kind, nullptr, detail);
}

void Parser::note_parse_debug_event_for(const char* kind,
                                        const char* function_name,
                                        const char* detail) {
    if (!parser_debug_enabled()) return;

    const auto now = std::chrono::steady_clock::now();
    if (diagnostic_state_.parse_debug_started_at ==
        std::chrono::steady_clock::time_point{}) {
        diagnostic_state_.parse_debug_started_at = now;
    }
    if (diagnostic_state_.parse_debug_last_progress_at ==
        std::chrono::steady_clock::time_point{}) {
        diagnostic_state_.parse_debug_last_progress_at = now;
    }

    ParseDebugEvent event;
    event.kind = kind ? kind : "";
    event.detail = detail ? detail : "";
    event.token_index = !at_end()
                            ? core_input_state_.pos
                            : (core_input_state_.pos > 0
                                   ? core_input_state_.pos - 1
                                   : -1);
    event.line = !at_end()
                     ? cur().line
                     : (core_input_state_.pos > 0
                            ? core_input_state_.tokens[core_input_state_.pos - 1].line
                            : 1);
    event.column = !at_end() ? cur().column : 1;
    if (function_name && function_name[0]) {
        event.function_name = function_name;
    } else if (!diagnostic_state_.parse_context_stack.empty()) {
        event.function_name =
            diagnostic_state_.parse_context_stack.back().function_name;
    }
    diagnostic_state_.parse_debug_events.push_back(std::move(event));
    if (static_cast<int>(diagnostic_state_.parse_debug_events.size()) >
        diagnostic_state_.max_parse_debug_events) {
        diagnostic_state_.parse_debug_events.erase(
            diagnostic_state_.parse_debug_events.begin());
    }
    if (kind &&
        (std::strncmp(kind, "tentative_", 10) == 0 ||
         std::strncmp(kind, "injected_parse_", 15) == 0)) {
        return;
    }
    if (!diagnostic_state_.parse_context_stack.empty()) {
        const int token_index =
            diagnostic_state_.parse_debug_events.back().token_index;
        const int token_delta =
            token_index - diagnostic_state_.best_parse_stack_token_index;
        const std::string current_function =
            diagnostic_state_.parse_context_stack.back().function_name;
        const bool preserving_wrapper_prefix_snapshot =
            token_delta > 0 &&
            current_stack_is_prefix_of_best(
                diagnostic_state_.parse_context_stack,
                diagnostic_state_.best_parse_stack_trace);
        const bool preserving_qualified_type_probe_snapshot =
            token_delta > 0 &&
            (is_top_level_wrapper_failure(current_function) ||
             is_qualified_type_trace_wrapper(current_function)) &&
            stack_contains_qualified_type_trace(
                diagnostic_state_.best_parse_stack_trace);
        const bool replace_snapshot =
            diagnostic_state_.best_parse_stack_token_index < 0 ||
            (!(preserving_wrapper_prefix_snapshot ||
               preserving_qualified_type_probe_snapshot) &&
             token_delta > 1) ||
            (token_delta > 0 &&
             diagnostic_state_.parse_context_stack.size() >=
                 diagnostic_state_.best_parse_stack_trace.size()) ||
            (token_delta == 0 &&
             diagnostic_state_.parse_context_stack.size() >
                 diagnostic_state_.best_parse_stack_trace.size());
        if (replace_snapshot) {
            diagnostic_state_.best_parse_stack_token_index = token_index;
            diagnostic_state_.best_parse_stack_trace.clear();
            diagnostic_state_.best_parse_stack_trace.reserve(
                diagnostic_state_.parse_context_stack.size());
            for (const ParseContextFrame& frame :
                 diagnostic_state_.parse_context_stack) {
                diagnostic_state_.best_parse_stack_trace.push_back(
                    frame.function_name);
            }
        }
    }

    maybe_emit_parse_debug_progress();
}

void Parser::maybe_emit_parse_debug_progress() {
    if (!parser_debug_enabled()) return;
    if (diagnostic_state_.parse_debug_events.empty()) return;

    const auto now = std::chrono::steady_clock::now();
    const auto interval =
        std::chrono::milliseconds(
            diagnostic_state_.parse_debug_progress_interval_ms);
    if (now - diagnostic_state_.parse_debug_last_progress_at < interval) return;

    const ParseDebugEvent& event = diagnostic_state_.parse_debug_events.back();
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now - diagnostic_state_.parse_debug_started_at)
            .count();
    fprintf(stderr,
            "[pdebug] progress elapsed_ms=%lld token_index=%d line=%d col=%d",
            static_cast<long long>(elapsed_ms), event.token_index, event.line,
            event.column);
    if (event.token_index >= 0 &&
        event.token_index < static_cast<int>(core_input_state_.tokens.size()) &&
        shared_lookup_state_.token_files &&
        core_input_state_.tokens[event.token_index].file_id != kInvalidFile) {
        fprintf(stderr, " file=%s",
                std::string(shared_lookup_state_.token_files->lookup(
                                core_input_state_.tokens[event.token_index].file_id))
                    .c_str());
    }
    if (!event.function_name.empty()) {
        fprintf(stderr, " fn=%s", event.function_name.c_str());
    }
    if (event.token_index >= 0 &&
        event.token_index < static_cast<int>(core_input_state_.tokens.size())) {
        fprintf(stderr, " token_kind=%s token='%s'",
                token_kind_name(core_input_state_.tokens[event.token_index].kind),
                std::string(token_spelling(
                    core_input_state_.tokens[event.token_index])).c_str());
    }
    fprintf(stderr, "\n");
    fflush(stderr);

    diagnostic_state_.parse_debug_last_progress_at = now;
}

void Parser::note_tentative_parse_event(TentativeParseMode mode,
                                        const char* kind,
                                        int start_pos,
                                        int end_pos) {
    int* counter = nullptr;
    switch (mode) {
        case TentativeParseMode::Heavy:
            if (std::strcmp(kind, "tentative_enter") == 0)
                counter = &diagnostic_state_.tentative_parse_stats.heavy_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                counter = &diagnostic_state_.tentative_parse_stats.heavy_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                counter =
                    &diagnostic_state_.tentative_parse_stats.heavy_rollback;
            break;
        case TentativeParseMode::Lite:
            if (std::strcmp(kind, "tentative_enter") == 0)
                counter = &diagnostic_state_.tentative_parse_stats.lite_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                counter = &diagnostic_state_.tentative_parse_stats.lite_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                counter =
                    &diagnostic_state_.tentative_parse_stats.lite_rollback;
            break;
    }
    if (counter) ++(*counter);

    const std::string detail =
        format_tentative_parse_detail(mode, kind, start_pos, end_pos);
    note_parse_debug_event(kind, detail.c_str());
}

void Parser::note_parse_failure(const char* expected,
                                const char* detail,
                                bool committed) {
    ParseFailure failure;
    failure.active = true;
    failure.committed = committed;
    failure.token_index = !at_end()
                              ? core_input_state_.pos
                              : (core_input_state_.pos > 0
                                     ? core_input_state_.pos - 1
                                     : -1);
    failure.token_kind = !at_end() ? cur().kind : TokenKind::EndOfFile;
    failure.line = !at_end()
                       ? cur().line
                       : (core_input_state_.pos > 0
                              ? core_input_state_.tokens[core_input_state_.pos - 1].line
                              : 1);
    failure.column = !at_end() ? cur().column : 1;
    failure.expected = expected ? expected : "";
    failure.got = at_end() ? "<eof>" : token_spelling(cur());
    failure.detail = detail ? detail : "";
    if (!diagnostic_state_.parse_context_stack.empty()) {
        failure.function_name =
            diagnostic_state_.parse_context_stack.back().function_name;
    }
    for (const ParseContextFrame& frame : diagnostic_state_.parse_context_stack) {
        failure.stack_trace.push_back(frame.function_name);
    }

    if (should_replace_best_parse_failure(diagnostic_state_.best_parse_failure,
                                          failure)) {
        diagnostic_state_.best_parse_failure = std::move(failure);
    }

    note_parse_debug_event(committed ? "fail" : "soft_fail", detail);
}

void Parser::note_parse_failure_message(const char* detail, bool committed) {
    note_parse_failure(nullptr, detail, committed);
}

std::vector<std::string> Parser::best_debug_summary_stack() const {
    const bool top_level_wrapper_failure =
        is_top_level_wrapper_failure(
            diagnostic_state_.best_parse_failure.function_name);
    std::vector<std::string> summary_stack;
    if (diagnostic_state_.best_parse_stack_trace.size() >
            diagnostic_state_.best_parse_failure.stack_trace.size() &&
        (diagnostic_state_.best_parse_stack_token_index >=
             diagnostic_state_.best_parse_failure.token_index ||
         top_level_wrapper_failure)) {
        std::vector<std::string> candidate =
            diagnostic_state_.best_parse_stack_trace;
        if (diagnostic_state_.best_parse_failure.function_name ==
                "parse_top_level" &&
            !candidate.empty() &&
            is_qualified_type_trace_leaf(candidate.back())) {
            candidate.pop_back();
        }
        size_t common_prefix = 0;
        while (common_prefix < candidate.size() &&
               common_prefix <
                   diagnostic_state_.best_parse_failure.stack_trace.size() &&
               candidate[common_prefix] ==
                   diagnostic_state_.best_parse_failure
                       .stack_trace[common_prefix]) {
            ++common_prefix;
        }
        if (top_level_wrapper_failure && common_prefix > 0 &&
            common_prefix < candidate.size() &&
            diagnostic_state_.best_parse_failure.function_name ==
                "parse_top_level_parameter_list") {
            std::vector<std::string> merged = candidate;
            merged.insert(merged.end(),
                          diagnostic_state_.best_parse_failure.stack_trace
                                  .begin() +
                              common_prefix,
                          diagnostic_state_.best_parse_failure.stack_trace.end());
            summary_stack = normalize_summary_stack(merged);
            return merge_leading_top_level_qualified_probe(
                diagnostic_state_.parse_debug_events, summary_stack);
        }
        if (!diagnostic_state_.best_parse_failure.function_name.empty() &&
            std::find(candidate.begin(),
                      candidate.end(),
                      diagnostic_state_.best_parse_failure.function_name) !=
                candidate.end()) {
            summary_stack = normalize_summary_stack(candidate);
            return merge_leading_top_level_qualified_probe(
                diagnostic_state_.parse_debug_events, summary_stack);
        }
        if (diagnostic_state_.best_parse_failure.function_name ==
                "parse_top_level" &&
            !candidate.empty()) {
            summary_stack = normalize_summary_stack(candidate);
            return merge_leading_top_level_qualified_probe(
                diagnostic_state_.parse_debug_events, summary_stack);
        }
    }
    summary_stack = normalize_summary_stack(
        diagnostic_state_.best_parse_failure.stack_trace);
    return merge_leading_top_level_qualified_probe(
        diagnostic_state_.parse_debug_events, summary_stack);
}

std::string Parser::format_best_parse_failure() const {
    if (!diagnostic_state_.best_parse_failure.active) return {};

    std::ostringstream oss;
    const std::vector<std::string> summary_stack = best_debug_summary_stack();
    const std::string* summary_leaf =
        select_best_parse_summary_leaf(summary_stack,
                                       diagnostic_state_.best_parse_failure);
    if (summary_leaf && !summary_leaf->empty()) {
        oss << "parse_fn=" << *summary_leaf;
    } else if (!diagnostic_state_.best_parse_failure.function_name.empty()) {
        oss << "parse_fn=" << diagnostic_state_.best_parse_failure.function_name;
    }
    if (diagnostic_state_.best_parse_failure.committed) {
        if (oss.tellp() > 0) oss << " ";
        oss << "phase=committed";
    }
    if (!diagnostic_state_.best_parse_failure.expected.empty()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "expected=" << diagnostic_state_.best_parse_failure.expected;
        oss << " got='" << diagnostic_state_.best_parse_failure.got << "'";
    } else {
        if (oss.tellp() > 0) oss << " ";
        oss << "got='" << diagnostic_state_.best_parse_failure.got << "'";
    }
    if (!diagnostic_state_.best_parse_failure.detail.empty()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "detail=\"" << diagnostic_state_.best_parse_failure.detail
            << "\"";
    }
    if (parser_debug_enabled()) {
        if (oss.tellp() > 0) oss << " ";
        oss << "token_index=" << diagnostic_state_.best_parse_failure.token_index
            << " token_kind="
            << token_kind_name(diagnostic_state_.best_parse_failure.token_kind)
            << " token_window=\""
            << format_parse_failure_token_window(
                   diagnostic_state_.best_parse_failure)
            << "\"";
    }
    return oss.str();
}

std::string Parser::format_parse_failure_token_window(
    const ParseFailure& failure) const {
    if (core_input_state_.tokens.empty()) return {};

    const int center =
        failure.token_index >= 0
            ? std::min(failure.token_index,
                       static_cast<int>(core_input_state_.tokens.size()) - 1)
            : static_cast<int>(core_input_state_.tokens.size()) - 1;
    const int start = std::max(0, center - 2);
    const int end =
        std::min(static_cast<int>(core_input_state_.tokens.size()) - 1, center + 2);

    std::ostringstream oss;
    for (int i = start; i <= end; ++i) {
        if (i > start) oss << " | ";
        oss << format_debug_token_entry(core_input_state_.tokens[i],
                                        token_spelling(core_input_state_.tokens[i]),
                                        i, i == center);
    }
    return oss.str();
}

std::string Parser::format_tentative_parse_detail(TentativeParseMode mode,
                                                  const char* kind,
                                                  int start_pos,
                                                  int end_pos) const {
    std::ostringstream oss;
    oss << "mode="
        << (mode == TentativeParseMode::Heavy ? "heavy" : "lite")
        << " start=" << start_pos
        << " end=" << end_pos;
    if (kind && kind[0]) {
        int count = 0;
        if (mode == TentativeParseMode::Heavy) {
            if (std::strcmp(kind, "tentative_enter") == 0)
                count = diagnostic_state_.tentative_parse_stats.heavy_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                count = diagnostic_state_.tentative_parse_stats.heavy_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                count = diagnostic_state_.tentative_parse_stats.heavy_rollback;
        } else {
            if (std::strcmp(kind, "tentative_enter") == 0)
                count = diagnostic_state_.tentative_parse_stats.lite_enter;
            else if (std::strcmp(kind, "tentative_commit") == 0)
                count = diagnostic_state_.tentative_parse_stats.lite_commit;
            else if (std::strcmp(kind, "tentative_rollback") == 0)
                count = diagnostic_state_.tentative_parse_stats.lite_rollback;
        }
        if (count > 0) oss << " count=" << count;
    }
    return oss.str();
}

void Parser::dump_parse_debug_trace() const {
    if (!parser_debug_enabled() || diagnostic_state_.parse_debug_events.empty()) {
        return;
    }
    bool has_visible_event = false;
    for (const ParseDebugEvent& event : diagnostic_state_.parse_debug_events) {
        if (parse_debug_event_visible(event.kind.c_str())) {
            has_visible_event = true;
            break;
        }
    }
    if (!has_visible_event) return;
    fprintf(stderr, "%s:%d:%d: note: parser debug trace follows\n",
            diagnostic_state_.best_parse_failure.active
                ? diag_file_at(diagnostic_state_.best_parse_failure.token_index)
                                       : core_input_state_.source_file.c_str(),
            diagnostic_state_.best_parse_failure.active
                ? diagnostic_state_.best_parse_failure.line
                : 1,
            diagnostic_state_.best_parse_failure.active
                ? diagnostic_state_.best_parse_failure.column
                : 1);
    for (const ParseDebugEvent& event : diagnostic_state_.parse_debug_events) {
        if (!parse_debug_event_visible(event.kind.c_str())) continue;
        fprintf(stderr, "[pdebug] kind=%s",
                event.kind.c_str());
        if (!event.function_name.empty())
            fprintf(stderr, " fn=%s", event.function_name.c_str());
        fprintf(stderr, " line=%d col=%d", event.line, event.column);
        if (!event.detail.empty()) fprintf(stderr, " detail=\"%s\"", event.detail.c_str());
        fprintf(stderr, "\n");
    }
    const std::vector<std::string> summary_stack = best_debug_summary_stack();
    if (diagnostic_state_.best_parse_failure.active && !summary_stack.empty()) {
        const std::vector<const std::string*> compact_stack =
            collapse_adjacent_stack_frames(summary_stack);
        fprintf(stderr, "[pdebug] stack:");
        for (const std::string* fn : compact_stack) {
            fprintf(stderr, " -> %s", fn->c_str());
        }
        fprintf(stderr, "\n");
    }
}

const char* Parser::diag_file_at(int token_index) const {
    if (token_index >= 0 &&
        token_index < static_cast<int>(core_input_state_.tokens.size()) &&
        shared_lookup_state_.token_files &&
        core_input_state_.tokens[token_index].file_id != kInvalidFile) {
        const std::string file = std::string(
            shared_lookup_state_.token_files->lookup(
                core_input_state_.tokens[token_index].file_id));
        if (!file.empty()) return arena_.strdup(file);
    }
    return core_input_state_.source_file.c_str();
}

void handle_pragma_gcc_visibility(Parser& parser, const std::string& args) {
    // Lexeme format: "push,<visibility>" or "pop"
    if (args == "pop") {
        if (!parser.pragma_state_.visibility_stack.empty()) {
            parser.pragma_state_.visibility =
                parser.pragma_state_.visibility_stack.back();
            parser.pragma_state_.visibility_stack.pop_back();
        } else {
            parser.pragma_state_.visibility = 0;  // default
        }
    } else if (args.substr(0, 5) == "push,") {
        parser.pragma_state_.visibility_stack.push_back(
            parser.pragma_state_.visibility);
        const std::string vis = args.substr(5);
        if (vis == "hidden") parser.pragma_state_.visibility = 1;
        else if (vis == "protected") parser.pragma_state_.visibility = 2;
        else parser.pragma_state_.visibility = 0;  // "default" or unknown → default
    }
}

// ── token cursor helpers ──────────────────────────────────────────────────────

const Token& Parser::cur() const {
    return core_input_state_.tokens[core_input_state_.pos];
}

const Token& Parser::peek(int offset) const {
    int idx = core_input_state_.pos + offset;
    if (idx < 0 || idx >= (int)core_input_state_.tokens.size()) {
        static Token eof = [] {
            Token token{};
            token.kind = TokenKind::EndOfFile;
            return token;
        }();
        return eof;
    }
    return core_input_state_.tokens[idx];
}

const Token& Parser::consume() {
    const Token& t = core_input_state_.tokens[core_input_state_.pos];
    if (core_input_state_.pos + 1 < (int)core_input_state_.tokens.size())
        ++core_input_state_.pos;
    return t;
}

bool Parser::at_end() const {
    if (core_input_state_.pos < 0 ||
        core_input_state_.pos >= static_cast<int>(core_input_state_.tokens.size()))
        return true;
    return core_input_state_.tokens[core_input_state_.pos].kind ==
           TokenKind::EndOfFile;
}

bool Parser::check(TokenKind k) const {
    if (core_input_state_.pos < 0 ||
        core_input_state_.pos >= static_cast<int>(core_input_state_.tokens.size()))
        return false;
    return core_input_state_.tokens[core_input_state_.pos].kind == k;
}

bool Parser::peek_next_is(TokenKind k) const {
    int idx = core_input_state_.pos + 1;
    if (idx >= (int)core_input_state_.tokens.size()) return false;
    return core_input_state_.tokens[idx].kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { consume(); return true; }
    return false;
}

std::string Parser::qualify_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (name.empty()) return {};
    const int context_id = current_namespace_context_id();
    if (context_id <= 0) return std::string(name);
    if (name.find("::") != std::string::npos) return std::string(name);
    return render_name_in_context(context_id, name_text_id);
}

const char* Parser::qualify_name_arena(TextId name_text_id) {
    std::string qualified = qualify_name(name_text_id);
    if (qualified.empty()) return nullptr;
    return arena_.strdup(qualified.c_str());
}

Parser::VisibleNameResult Parser::resolve_visible_value(
    TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (can_probe_local_binding(name_text_id, name) &&
        find_local_visible_var_type(name_text_id)) {
        VisibleNameResult result;
        result.found = true;
        result.kind = VisibleNameKind::Value;
        result.base_text_id = name_text_id;
        result.context_id = current_namespace_context_id();
        result.source = VisibleNameSource::Local;
        result.compatibility_spelling = std::string(name);
        return result;
    }
    VisibleNameResult result;
    for (int i = static_cast<int>(namespace_state_.namespace_stack.size()) - 1;
         i >= 0; --i) {
        const int context_id = namespace_state_.namespace_stack[i];
        VisibleNameResult alias_result;
        if (lookup_using_value_alias(context_id, name_text_id, &alias_result)) {
            return alias_result;
        }
        if (lookup_value_in_context(context_id, name_text_id, &result)) {
            return result;
        }
    }
    return {};
}

std::string Parser::resolve_visible_value_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    const VisibleNameResult result = resolve_visible_value(name_text_id);
    if (result) {
        const std::string spelling = visible_name_spelling(result);
        if (!spelling.empty()) return spelling;
    }
    return std::string(name);
}

Parser::VisibleNameResult Parser::resolve_visible_type(
    TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (is_cpp_mode() && !active_context_state_.current_struct_tag.empty()) {
        const std::string_view current_record = current_struct_tag_text();
        const std::string qualified_current =
            current_record.find("::") != std::string_view::npos
                ? std::string(current_record)
                : render_name_in_context(
                      current_namespace_context_id(),
                      active_context_state_.current_struct_tag_text_id);
        if (spelling_matches_current_record(name, current_record,
                                            qualified_current)) {
            VisibleNameResult result;
            result.found = true;
            result.kind = VisibleNameKind::Type;
            result.base_text_id =
                active_context_state_.current_struct_tag_text_id;
            result.context_id = current_namespace_context_id();
            result.source = VisibleNameSource::Local;
            result.compatibility_spelling = qualified_current;
            return result;
        }
        const std::string sibling =
            current_record_namespace_sibling(qualified_current, name);
        if (!sibling.empty()) {
            const TextId sibling_text_id =
                parser_text_id_for_token(kInvalidText, sibling);
            const QualifiedNameKey sibling_key =
                struct_typedef_key_in_context(0, sibling_text_id);
            if (const TypeSpec* sibling_type =
                    find_typedef_type(sibling_text_id);
                sibling_type &&
                (sibling_type->base == TB_STRUCT ||
                 sibling_type->base == TB_UNION) &&
                !is_record_projection_type(*this, *sibling_type)) {
                VisibleNameResult result;
                result.found = true;
                result.kind = VisibleNameKind::Type;
                result.key = sibling_key;
                result.base_text_id = name_text_id;
                result.context_id = current_namespace_context_id();
                result.source = VisibleNameSource::Fallback;
                result.compatibility_spelling = sibling;
                return result;
            }
        }
    }
    if (can_probe_local_binding(name_text_id, name) &&
        find_local_visible_typedef_type(name_text_id)) {
        VisibleNameResult result;
        result.found = true;
        result.kind = VisibleNameKind::Type;
        result.base_text_id = name_text_id;
        result.context_id = current_namespace_context_id();
        result.source = VisibleNameSource::Local;
        result.compatibility_spelling = std::string(name);
        return result;
    }
    VisibleNameResult result;
    for (int i = static_cast<int>(namespace_state_.namespace_stack.size()) - 1;
         i >= 0; --i) {
        const int context_id = namespace_state_.namespace_stack[i];
        VisibleNameResult resolved_alias_result;
        if (lookup_using_value_alias(context_id, name_text_id,
                                     &resolved_alias_result)) {
            const QualifiedNameKey& target_key = resolved_alias_result.key;
            const TextId target_text_id = target_key.base_text_id;
            if (target_text_id != kInvalidText &&
                (find_structured_typedef_type(target_key) ||
                 find_alias_template_info(target_key) ||
                 find_local_visible_typedef_type(target_text_id) ||
                 find_typedef_type(target_text_id))) {
                result = resolved_alias_result;
                result.kind = VisibleNameKind::Type;
                result.base_text_id = target_text_id;
                result.context_id = target_key.context_id;
                result.source = VisibleNameSource::UsingAlias;
                return result;
            }
        }
        if (lookup_type_in_context(context_id, name_text_id, &result)) {
            return result;
        }
    }
    return {};
}

std::string Parser::visible_name_spelling(
    const VisibleNameResult& result) const {
    if (!result) return {};
    if (!result.compatibility_spelling.empty()) {
        return result.compatibility_spelling;
    }
    if (const std::string structured = render_structured_name(*this, result.key);
        !structured.empty()) {
        return structured;
    }
    if (result.context_id >= 0 && result.base_text_id != kInvalidText) {
        return render_name_in_context(result.context_id, result.base_text_id);
    }
    return {};
}

std::string Parser::resolve_visible_type_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    const VisibleNameResult result = resolve_visible_type(name_text_id);
    if (result) {
        const std::string spelling = visible_name_spelling(result);
        if (!spelling.empty()) return spelling;
    }
    return std::string(name);
}

Parser::VisibleNameResult Parser::resolve_visible_concept(
    TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (name_text_id != kInvalidText && is_unqualified_lookup_name(name) &&
        binding_state_.concept_name_text_ids.count(name_text_id) > 0) {
        VisibleNameResult result;
        result.found = true;
        result.kind = VisibleNameKind::Concept;
        result.base_text_id = name_text_id;
        result.context_id = current_namespace_context_id();
        result.source = VisibleNameSource::Local;
        result.compatibility_spelling = std::string(name);
        return result;
    }
    VisibleNameResult result;
    for (int i = static_cast<int>(namespace_state_.namespace_stack.size()) - 1;
         i >= 0; --i) {
        if (lookup_concept_in_context(namespace_state_.namespace_stack[i],
                                      name_text_id, &result)) {
            return result;
        }
    }
    return {};
}

std::string Parser::resolve_visible_concept_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    const VisibleNameResult result = resolve_visible_concept(name_text_id);
    if (result) {
        const std::string spelling = visible_name_spelling(result);
        if (!spelling.empty()) return spelling;
    }
    return std::string(name);
}

bool Parser::is_concept_name(TextId name_text_id) const {
    const std::string_view name = parser_text(name_text_id, {});
    if (name.empty()) return false;
    if (name.find("::") != std::string::npos &&
        has_structured_concept_name(known_fn_name_key(0, name_text_id))) {
        return true;
    }
    if (name_text_id != kInvalidText && is_unqualified_lookup_name(name) &&
        binding_state_.concept_name_text_ids.count(name_text_id) > 0) {
        return true;
    }
    return static_cast<bool>(resolve_visible_concept(name_text_id));
}

void Parser::refresh_current_namespace_bridge() {
    namespace_state_.current_namespace.clear();
    for (size_t i = 1; i < namespace_state_.namespace_stack.size(); ++i) {
        const NamespaceContext& ctx =
            namespace_state_.namespace_contexts[namespace_state_.namespace_stack[i]];
        if (!ctx.canonical_name || !ctx.canonical_name[0]) continue;
        namespace_state_.current_namespace = ctx.canonical_name;
    }
}

int Parser::current_namespace_context_id() const {
    if (namespace_state_.namespace_stack.empty()) return 0;
    return namespace_state_.namespace_stack.back();
}

int Parser::find_named_namespace_child(int parent_id, TextId text_id) const {
    if (text_id == kInvalidText) return -1;
    auto parent_it = namespace_state_.named_namespace_children.find(parent_id);
    if (parent_it == namespace_state_.named_namespace_children.end()) return -1;
    auto child_it = parent_it->second.find(text_id);
    if (child_it == parent_it->second.end()) return -1;
    return child_it->second;
}

int Parser::ensure_named_namespace_context(int parent_id, TextId text_id) {
    if (text_id == kInvalidText) return -1;
    const int existing = find_named_namespace_child(parent_id, text_id);
    if (existing >= 0) return existing;

    const std::string name(parser_text(text_id, {}));
    if (name.empty()) return -1;
    const NamespaceContext& parent = namespace_state_.namespace_contexts[parent_id];
    std::string canonical =
        build_canonical_namespace_name(parent.canonical_name, name);

    const int id = static_cast<int>(namespace_state_.namespace_contexts.size());
    namespace_state_.namespace_contexts.push_back(
        NamespaceContext{id, parent_id, false, text_id, arena_.strdup(name.c_str()),
                         arena_.strdup(canonical.c_str())});
    namespace_state_.named_namespace_children[parent_id][text_id] = id;
    return id;
}

int Parser::create_anonymous_namespace_context(int parent_id) {
    const NamespaceContext& parent = namespace_state_.namespace_contexts[parent_id];
    std::string canonical = build_canonical_namespace_name(
        parent.canonical_name,
        "__anon_ns_" + std::to_string(definition_state_.anon_counter++));

    const int id = static_cast<int>(namespace_state_.namespace_contexts.size());
    namespace_state_.namespace_contexts.push_back(
        NamespaceContext{id, parent_id, true, kInvalidText, arena_.strdup(""),
                         arena_.strdup(canonical.c_str())});
    namespace_state_.anonymous_namespace_children[parent_id].push_back(id);
    return id;
}

void Parser::push_namespace_context(int context_id) {
    namespace_state_.namespace_stack.push_back(context_id);
    refresh_current_namespace_bridge();
}

void Parser::pop_namespace_context() {
    if (namespace_state_.namespace_stack.size() > 1) {
        namespace_state_.namespace_stack.pop_back();
    }
    refresh_current_namespace_bridge();
}

std::string Parser::render_name_in_context(int context_id,
                                           TextId name_text_id) const {
    return render_lookup_name_in_context(*this, context_id, name_text_id,
                                         {});
}

int Parser::resolve_namespace_context(const QualifiedNameRef& name) const {
    auto follow_path = [&](int start_id) -> int {
        int context_id = start_id;
        for (size_t i = 0; i < name.qualifier_segments.size(); ++i) {
            const TextId segment_text_id =
                i < name.qualifier_text_ids.size()
                    ? name.qualifier_text_ids[i]
                    : kInvalidText;
            if (segment_text_id == kInvalidText) return -1;
            context_id = find_named_namespace_child(context_id, segment_text_id);
            if (context_id < 0) return -1;
        }
        return context_id;
    };

    return resolve_namespace_id_from_stack(namespace_state_.namespace_stack,
                                           name.is_global_qualified,
                                           follow_path);
}

int Parser::resolve_namespace_name(const QualifiedNameRef& name) const {
    auto follow_name = [&](int start_id) -> int {
        int context_id = start_id;
        for (size_t i = 0; i < name.qualifier_segments.size(); ++i) {
            const TextId segment_text_id =
                i < name.qualifier_text_ids.size()
                    ? name.qualifier_text_ids[i]
                    : kInvalidText;
            if (segment_text_id == kInvalidText) return -1;
            context_id = find_named_namespace_child(context_id, segment_text_id);
            if (context_id < 0) return -1;
        }
        const TextId base_text_id =
            name.base_text_id != kInvalidText ? name.base_text_id
                                              : kInvalidText;
        if (base_text_id == kInvalidText) return -1;
        return find_named_namespace_child(context_id, base_text_id);
    };

    return resolve_namespace_id_from_stack(namespace_state_.namespace_stack,
                                           name.is_global_qualified,
                                           follow_name);
}

std::string Parser::qualified_name_text(const QualifiedNameRef& name,
                                        bool include_global_prefix) const {
    std::string qualified;
    if (include_global_prefix && name.is_global_qualified) qualified = "::";
    for (size_t i = 0; i < name.qualifier_segments.size(); ++i) {
        if (!qualified.empty() && qualified != "::") qualified += "::";
        const TextId segment_text_id =
            i < name.qualifier_text_ids.size() ? name.qualifier_text_ids[i]
                                               : kInvalidText;
        qualified += parser_text(segment_text_id, name.qualifier_segments[i]);
    }
    if (!qualified.empty() && qualified != "::") qualified += "::";
    qualified += parser_text(name.base_text_id, name.base_name);
    return qualified;
}

Parser::VisibleNameResult Parser::resolve_qualified_value(
    const QualifiedNameRef& name) const {
    if (name.qualifier_segments.empty()) {
        if (!name.is_global_qualified) {
            return resolve_visible_value(name.base_text_id);
        }
        VisibleNameResult result;
        if (lookup_value_in_context(0, name.base_text_id, &result)) {
            return result;
        }
        return {};
    }

    const int context_id = resolve_namespace_context(name);
    if (context_id < 0) return {};

    VisibleNameResult result;
    VisibleNameResult alias_result;
    if (lookup_using_value_alias(context_id, name.base_text_id,
                                 &alias_result)) {
        return alias_result;
    }

    if (lookup_value_in_context(context_id, name.base_text_id, &result)) {
        return result;
    }
    return {};
}

Parser::VisibleNameResult Parser::resolve_qualified_type(
    const QualifiedNameRef& name) const {
    if (name.qualifier_segments.empty()) {
        if (!name.is_global_qualified) {
            return resolve_visible_type(name.base_text_id);
        }
        VisibleNameResult result;
        if (lookup_type_in_context(0, name.base_text_id, &result)) {
            return result;
        }
        return {};
    }

    const QualifiedNameKey direct_key = find_qualified_name_key(*this, name);
    if (find_typedef_type(direct_key)) {
        VisibleNameResult result;
        result.found = true;
        result.kind = VisibleNameKind::Type;
        result.key = direct_key;
        result.base_text_id = direct_key.base_text_id;
        result.context_id = direct_key.context_id;
        result.source = VisibleNameSource::Namespace;
        result.compatibility_spelling =
            render_structured_name(*this, direct_key);
        if (result.compatibility_spelling.empty()) {
            result.compatibility_spelling =
                qualified_name_text(name, /*include_global_prefix=*/true);
        }
        return result;
    }

    const int context_id = resolve_namespace_context(name);
    if (context_id < 0) return {};
    VisibleNameResult result;
    if (lookup_type_in_context(context_id, name.base_text_id, &result)) {
        return result;
    }
    return {};
}

std::string Parser::resolve_qualified_type_name(
    const QualifiedNameRef& name) const {
    const VisibleNameResult result = resolve_qualified_type(name);
    if (result) {
        return visible_name_spelling(result);
    }
    return {};
}

bool Parser::lookup_using_value_alias(int context_id, TextId name_text_id,
                                      VisibleNameResult* resolved) const {
    if (!resolved) return false;
    if (name_text_id == kInvalidText) return false;

    const auto alias_it = namespace_state_.using_value_aliases.find(context_id);
    if (alias_it == namespace_state_.using_value_aliases.end()) return false;
    const auto value_it = alias_it->second.find(name_text_id);
    if (value_it == alias_it->second.end()) return false;
    const ParserNamespaceState::UsingValueAlias& alias = value_it->second;
    if (alias.target_key.base_text_id == kInvalidText) return false;
    const std::string spelling =
        render_value_binding_name(*this, alias.target_key);

    resolved->found = true;
    resolved->kind = VisibleNameKind::Value;
    resolved->key = alias.target_key;
    resolved->base_text_id = alias.target_key.base_text_id;
    resolved->context_id = alias.target_key.context_id;
    resolved->source = VisibleNameSource::UsingAlias;
    resolved->compatibility_spelling = spelling;

    if (has_known_fn_name(alias.target_key)) return true;
    if (find_structured_var_type(alias.target_key)) return true;
    return false;
}

bool Parser::lookup_value_in_context(int context_id, TextId name_text_id,
                                     VisibleNameResult* resolved) const {
    if (!resolved) return false;
    if (name_text_id == kInvalidText) return false;
    const QualifiedNameKey candidate_key =
        known_fn_name_key_in_context(context_id, name_text_id);
    if (has_known_fn_name(candidate_key)) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Value;
        resolved->key = candidate_key;
        resolved->base_text_id = candidate_key.base_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Namespace;
        resolved->compatibility_spelling =
            render_structured_name(*this, candidate_key);
        return true;
    }

    if (find_structured_var_type(candidate_key)) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Value;
        resolved->key = candidate_key;
        resolved->base_text_id = candidate_key.base_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Namespace;
        resolved->compatibility_spelling =
            render_value_binding_name(*this, candidate_key);
        if (resolved->compatibility_spelling.empty()) {
            resolved->compatibility_spelling =
                render_name_in_context(context_id, name_text_id);
        }
        return true;
    }

    if (context_id == 0) {
        const QualifiedNameKey global_key =
            known_fn_name_key_in_context(0, name_text_id);
        if (has_known_fn_name(global_key)) {
            resolved->found = true;
            resolved->kind = VisibleNameKind::Value;
            resolved->key = global_key;
            resolved->base_text_id = global_key.base_text_id;
            resolved->context_id = context_id;
            resolved->source = VisibleNameSource::Namespace;
            resolved->compatibility_spelling =
                render_structured_name(*this, global_key);
            return true;
        }
    }

    auto anon_it = namespace_state_.anonymous_namespace_children.find(context_id);
    if (anon_it != namespace_state_.anonymous_namespace_children.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_value_in_context(anon_id, name_text_id, resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::AnonymousNamespace;
                }
                return true;
            }
        }
    }

    auto using_it = namespace_state_.using_namespace_contexts.find(context_id);
    if (using_it != namespace_state_.using_namespace_contexts.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_value_in_context(imported_id, name_text_id, resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::ImportedNamespace;
                }
                return true;
            }
        }
    }

    const QualifiedNameKey lookup_key = qualified_key_in_context(
        *this, context_id, name_text_id, true);
    if (find_structured_var_type(lookup_key)) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Value;
        resolved->key = lookup_key;
        resolved->base_text_id = lookup_key.base_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Fallback;
        resolved->compatibility_spelling =
            render_value_binding_name(*this, lookup_key);
        if (resolved->compatibility_spelling.empty()) {
            resolved->compatibility_spelling =
                render_name_in_context(context_id, name_text_id);
        }
        return true;
    }
    return false;
}

bool Parser::lookup_type_in_context(int context_id, TextId name_text_id,
                                    VisibleNameResult* resolved) const {
    if (!resolved) return false;
    if (name_text_id == kInvalidText) return false;
    const QualifiedNameKey candidate_key =
        struct_typedef_key_in_context(context_id, name_text_id);
    if (find_typedef_type(candidate_key)) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Type;
        resolved->key = candidate_key;
        resolved->base_text_id = candidate_key.base_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Namespace;
        resolved->compatibility_spelling = render_structured_name(*this, candidate_key);
        if (resolved->compatibility_spelling.empty()) {
            resolved->compatibility_spelling =
                render_name_in_context(context_id, name_text_id);
        }
        return true;
    }
    const std::string_view name = parser_text(name_text_id, {});
    if (context_id == 0 && !name.empty() &&
        name.find("::") == std::string_view::npos) {
        if (find_local_visible_typedef_type(name_text_id)) {
            resolved->found = true;
            resolved->kind = VisibleNameKind::Type;
            resolved->base_text_id = name_text_id;
            resolved->context_id = context_id;
            resolved->source = VisibleNameSource::Local;
            resolved->compatibility_spelling =
                std::string(parser_text(name_text_id, name));
            return true;
        }
    }

    auto anon_it = namespace_state_.anonymous_namespace_children.find(context_id);
    if (anon_it != namespace_state_.anonymous_namespace_children.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_type_in_context(anon_id, name_text_id, resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::AnonymousNamespace;
                }
                return true;
            }
        }
    }

    auto using_it = namespace_state_.using_namespace_contexts.find(context_id);
    if (using_it != namespace_state_.using_namespace_contexts.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_type_in_context(imported_id, name_text_id, resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::ImportedNamespace;
                }
                return true;
            }
        }
    }
    return false;
}

bool Parser::lookup_concept_in_context(int context_id, TextId name_text_id,
                                       VisibleNameResult* resolved) const {
    if (!resolved) return false;
    if (name_text_id == kInvalidText) return false;
    const std::string_view name = parser_text(name_text_id, {});
    if (context_id == 0 && name_text_id != kInvalidText &&
        is_unqualified_lookup_name(name) &&
        binding_state_.concept_name_text_ids.count(name_text_id) > 0) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Concept;
        resolved->base_text_id = name_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Local;
        resolved->compatibility_spelling = std::string(name);
        return true;
    }
    const QualifiedNameKey candidate_key =
        known_fn_name_key_in_context(context_id, name_text_id);
    if (has_structured_concept_name(candidate_key)) {
        resolved->found = true;
        resolved->kind = VisibleNameKind::Concept;
        resolved->key = candidate_key;
        resolved->base_text_id = candidate_key.base_text_id;
        resolved->context_id = context_id;
        resolved->source = VisibleNameSource::Namespace;
        resolved->compatibility_spelling =
            render_structured_name(*this, candidate_key);
        if (resolved->compatibility_spelling.empty()) {
            resolved->compatibility_spelling =
                render_name_in_context(context_id, name_text_id);
        }
        return true;
    }

    auto anon_it = namespace_state_.anonymous_namespace_children.find(context_id);
    if (anon_it != namespace_state_.anonymous_namespace_children.end()) {
        for (int anon_id : anon_it->second) {
            if (lookup_concept_in_context(anon_id, name_text_id, resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::AnonymousNamespace;
                }
                return true;
            }
        }
    }

    auto using_it = namespace_state_.using_namespace_contexts.find(context_id);
    if (using_it != namespace_state_.using_namespace_contexts.end()) {
        for (int imported_id : using_it->second) {
            if (lookup_concept_in_context(imported_id, name_text_id,
                                          resolved)) {
                if (resolved->source == VisibleNameSource::Namespace) {
                    resolved->source = VisibleNameSource::ImportedNamespace;
                }
                return true;
            }
        }
    }
    return false;
}

bool Parser::peek_qualified_name(QualifiedNameRef* out, bool allow_global) const {
    if (!out) return false;
    QualifiedNameRef result;
    const auto& tokens = core_input_state_.tokens;
    int p = core_input_state_.pos;
    if (allow_global && p < static_cast<int>(tokens.size()) &&
        tokens[p].kind == TokenKind::ColonColon) {
        result.is_global_qualified = true;
        ++p;
    }
    if (p >= static_cast<int>(tokens.size()) || tokens[p].kind != TokenKind::Identifier) {
        return false;
    }
    result.base_name = std::string(token_spelling(tokens[p]));
    result.base_text_id = parser_text_id_for_token(tokens[p].text_id, result.base_name);
    ++p;
    while (p + 1 < static_cast<int>(tokens.size()) &&
           tokens[p].kind == TokenKind::ColonColon &&
           tokens[p + 1].kind == TokenKind::Identifier) {
        result.qualifier_segments.push_back(result.base_name);
        result.qualifier_text_ids.push_back(result.base_text_id);
        result.base_name = std::string(token_spelling(tokens[p + 1]));
        result.base_text_id =
            parser_text_id_for_token(tokens[p + 1].text_id, result.base_name);
        p += 2;
    }
    *out = std::move(result);
    return true;
}

Parser::QualifiedNameRef Parser::parse_qualified_name(bool allow_global) {
    QualifiedNameRef result;
    if (!peek_qualified_name(&result, allow_global)) {
        throw std::runtime_error("expected qualified name");
    }
    if (result.is_global_qualified) expect(TokenKind::ColonColon);
    expect(TokenKind::Identifier);
    for (size_t i = 0; i < result.qualifier_segments.size(); ++i) {
        expect(TokenKind::ColonColon);
        expect(TokenKind::Identifier);
    }
    populate_qualified_name_symbol_ids(&result);
    return result;
}

void Parser::apply_qualified_name(Node* node, const QualifiedNameRef& qn,
                                  const char* resolved_name) {
    if (!node) return;
    node->is_global_qualified = qn.is_global_qualified;
    node->unqualified_text_id = qn.base_text_id;
    node->unqualified_name =
        arena_.strdup(std::string(shared_lookup_state_.token_texts &&
                                          qn.base_text_id != kInvalidText
                                      ? shared_lookup_state_.token_texts->lookup(
                                            qn.base_text_id)
                                      : std::string_view(qn.base_name))
                          .c_str());
    node->n_qualifier_segments = static_cast<int>(qn.qualifier_segments.size());
    if (node->n_qualifier_segments > 0) {
        node->qualifier_segments = arena_.alloc_array<const char*>(node->n_qualifier_segments);
        node->qualifier_text_ids = arena_.alloc_array<TextId>(node->n_qualifier_segments);
        for (int i = 0; i < node->n_qualifier_segments; ++i) {
            const std::string_view segment =
                shared_lookup_state_.token_texts &&
                        i < static_cast<int>(qn.qualifier_text_ids.size()) &&
                        qn.qualifier_text_ids[i] != kInvalidText
                    ? shared_lookup_state_.token_texts->lookup(
                          qn.qualifier_text_ids[i])
                    : std::string_view(qn.qualifier_segments[i]);
            node->qualifier_segments[i] =
                arena_.strdup(std::string(segment).c_str());
            node->qualifier_text_ids[i] =
                i < static_cast<int>(qn.qualifier_text_ids.size())
                    ? qn.qualifier_text_ids[i]
                    : parser_text_id_for_token(kInvalidText, segment);
        }
    }
    if (resolved_name && resolved_name[0]) {
        node->namespace_context_id = resolve_namespace_context(qn);
        if (node->namespace_context_id < 0 && qn.qualifier_segments.size() > 0) {
            QualifiedNameRef owner_namespace = qn;
            owner_namespace.qualifier_segments.pop_back();
            if (!owner_namespace.qualifier_text_ids.empty()) {
                owner_namespace.qualifier_text_ids.pop_back();
            }
            node->namespace_context_id = resolve_namespace_context(owner_namespace);
        }
    }
}

void Parser::apply_using_value_alias_target(
    Node* node, const VisibleNameResult& resolved) {
    if (!node || resolved.source != VisibleNameSource::UsingAlias) return;
    if (resolved.key.base_text_id == kInvalidText ||
        resolved.key.context_id < 0) {
        return;
    }
    node->using_value_alias_target_text_id = resolved.key.base_text_id;
    node->using_value_alias_target_namespace_context_id =
        resolved.key.context_id;
    node->using_value_alias_target_is_global_qualified =
        resolved.key.is_global_qualified;
    const NamePathTable::View qualifier_path =
        shared_lookup_state_.parser_name_paths.lookup(
            resolved.key.qualifier_path_id);
    node->n_using_value_alias_target_qualifier_segments =
        static_cast<int>(qualifier_path.size);
    if (node->n_using_value_alias_target_qualifier_segments > 0) {
        node->using_value_alias_target_qualifier_text_ids =
            arena_.alloc_array<TextId>(
                node->n_using_value_alias_target_qualifier_segments);
        for (int i = 0;
             i < node->n_using_value_alias_target_qualifier_segments; ++i) {
            node->using_value_alias_target_qualifier_text_ids[i] =
                qualifier_path[static_cast<std::size_t>(i)];
        }
    }
}

void Parser::apply_decl_namespace(Node* node, int context_id, const char* unqualified_name) {
    if (!node) return;
    node->namespace_context_id = context_id;
    node->unqualified_name = unqualified_name;
    node->unqualified_text_id =
        parser_text_id_for_token(kInvalidText, unqualified_name ? unqualified_name : "");

    std::vector<const char*> segments;
    for (int walk = context_id; walk > 0;
         walk = namespace_state_.namespace_contexts[walk].parent_id) {
        const NamespaceContext& ctx = namespace_state_.namespace_contexts[walk];
        if (ctx.is_anonymous || !ctx.display_name || !ctx.display_name[0]) continue;
        segments.push_back(ctx.display_name);
    }
    node->n_qualifier_segments = static_cast<int>(segments.size());
    if (node->n_qualifier_segments > 0) {
        node->qualifier_segments = arena_.alloc_array<const char*>(node->n_qualifier_segments);
        node->qualifier_text_ids = arena_.alloc_array<TextId>(node->n_qualifier_segments);
        for (int i = 0; i < node->n_qualifier_segments; ++i) {
            node->qualifier_segments[i] = segments[node->n_qualifier_segments - 1 - i];
            node->qualifier_text_ids[i] = parser_text_id_for_token(
                kInvalidText, node->qualifier_segments[i] ? node->qualifier_segments[i] : "");
        }
    }
}

void Parser::expect(TokenKind k) {
    if (!check(k)) {
        note_parse_failure(token_kind_name(k));
        std::ostringstream msg;
        msg << "expected " << token_kind_name(k) << " but got '"
            << token_spelling(cur()) << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
    consume();
}

bool Parser::check_template_close() const {
    return check(TokenKind::Greater) ||
           check(TokenKind::GreaterGreater) ||
           check(TokenKind::GreaterEqual) ||
           check(TokenKind::GreaterGreaterAssign);
}

bool Parser::parse_greater_than_in_template_list(bool consume_last_token) {
    if (check(TokenKind::Greater)) {
        if (consume_last_token) consume();
        return true;
    }

    if (check(TokenKind::GreaterGreater)) {
        // Consume one > and leave the second > in the token stream.
        core_input_state_.token_mutations.push_back(
            {core_input_state_.pos, core_input_state_.tokens[core_input_state_.pos]});
        core_input_state_.tokens[core_input_state_.pos].kind = TokenKind::Greater;
        set_parser_owned_spelling(core_input_state_.tokens[core_input_state_.pos], ">");
        return true;
    }

    if (check(TokenKind::GreaterEqual)) {
        // Consume one > and leave = in the token stream.
        core_input_state_.token_mutations.push_back(
            {core_input_state_.pos, core_input_state_.tokens[core_input_state_.pos]});
        core_input_state_.tokens[core_input_state_.pos].kind = TokenKind::Assign;
        set_parser_owned_spelling(core_input_state_.tokens[core_input_state_.pos], "=");
        return true;
    }

    if (check(TokenKind::GreaterGreaterAssign)) {
        // Consume one > and leave >= in the token stream.
        core_input_state_.token_mutations.push_back(
            {core_input_state_.pos, core_input_state_.tokens[core_input_state_.pos]});
        core_input_state_.tokens[core_input_state_.pos].kind = TokenKind::GreaterEqual;
        set_parser_owned_spelling(core_input_state_.tokens[core_input_state_.pos], ">=");
        return true;
    }

    return false;
}

bool Parser::match_template_close() {
    return parse_greater_than_in_template_list(true);
}

void Parser::expect_template_close() {
    if (!match_template_close()) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(TokenKind::Greater) << " but got '"
            << token_spelling(cur()) << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
}

void Parser::skip_until(TokenKind k) {
    while (!at_end() && !check(k)) consume();
    if (!at_end()) consume();  // consume the terminator
}

Node* Parser::parse() {
    std::vector<Node*> items;
    diagnostic_state_.had_error = false;
    diagnostic_state_.parse_error_count = 0;
    int no_progress_steps = 0;
    reset_parse_debug_progress();

    while (!at_end()) {
        Node* item = nullptr;
        int loop_start_pos = core_input_state_.pos;
        clear_parse_debug_state();
        try {
            item = parse_top_level(*this);
        } catch (const std::exception& e) {
            // Parse error: emit stable diagnostic and try to recover.
            int err_idx = diagnostic_state_.best_parse_failure.active
                              ? diagnostic_state_.best_parse_failure.token_index
                              : (!at_end()
                                     ? core_input_state_.pos
                                     : (core_input_state_.pos > 0
                                            ? core_input_state_.pos - 1
                                            : -1));
            int err_line = diagnostic_state_.best_parse_failure.active
                               ? diagnostic_state_.best_parse_failure.line
                               : ((!at_end())
                                      ? cur().line
                                      : (core_input_state_.pos > 0
                                             ? core_input_state_.tokens[core_input_state_.pos - 1].line
                                             : 1));
            int err_col  = diagnostic_state_.best_parse_failure.active
                               ? diagnostic_state_.best_parse_failure.column
                               : ((!at_end()) ? cur().column : 1);
            std::string diag = format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: %s\n",
                    diag_file_at(err_idx), err_line, err_col,
                    diag.empty() ? e.what() : diag.c_str());
            dump_parse_debug_trace();
            diagnostic_state_.had_error = true;
            ++diagnostic_state_.parse_error_count;
            if (diagnostic_state_.parse_error_count >=
                diagnostic_state_.max_parse_errors) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        diag_file_at(err_idx), err_line, err_col);
                break;
            }
            if (core_input_state_.pos == loop_start_pos && !at_end()) consume();
            while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                consume();
            }
            if (!at_end()) consume();
            no_progress_steps = 0;
            continue;
        }
        if (core_input_state_.pos == loop_start_pos && !at_end()) {
            diagnostic_state_.had_error = true;
            ++no_progress_steps;
            note_parse_failure_message("unexpected token with no parse progress");
            std::string diag = format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: unexpected token '%s'\n",
                    diag_file_at(core_input_state_.pos), cur().line, cur().column,
                    std::string(token_spelling(cur())).c_str());
            if (!diag.empty()) {
                fprintf(stderr, "%s:%d:%d: note: %s\n",
                        diag_file_at(core_input_state_.pos), cur().line, cur().column,
                        diag.c_str());
            }
            dump_parse_debug_trace();
            consume();
            ++diagnostic_state_.parse_error_count;
            if (no_progress_steps >= diagnostic_state_.max_no_progress_steps) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        diag_file_at(!at_end() ? core_input_state_.pos
                                               : static_cast<int>(core_input_state_.tokens.size()) - 1),
                        !at_end() ? cur().line : core_input_state_.tokens.back().line,
                        !at_end() ? cur().column : 1);
                break;
            }
            continue;
        }
        no_progress_steps = 0;
        if (item) items.push_back(item);
    }

    // Prepend collected struct/enum definitions (so IR builder sees them first)
    std::vector<Node*> all;
    for (Node* sd : definition_state_.struct_defs) all.push_back(sd);
    for (Node* it : items)        all.push_back(it);

    Node* prog = make_node(NK_PROGRAM, 0);
    prog->n_children = (int)all.size();
    if (prog->n_children > 0) {
        prog->children = arena_.alloc_array<Node*>(prog->n_children);
        for (int i = 0; i < prog->n_children; ++i) prog->children[i] = all[i];
    }
    return prog;
}

// ── node constructors ─────────────────────────────────────────────────────────

Node* Parser::make_node(NodeKind k, int line) {
    Node* n = arena_.alloc_array<Node>(1);
    std::memset(n, 0, sizeof(Node));
    n->kind = k;
    n->line = line;
    n->column = 1;
    n->file = arena_.strdup(core_input_state_.source_file);
    int loc_index = -1;
    if (core_input_state_.pos >= 0 &&
        core_input_state_.pos < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[core_input_state_.pos].line == line) {
        loc_index = core_input_state_.pos;
    } else if (core_input_state_.pos > 0 &&
               core_input_state_.pos - 1 < static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[core_input_state_.pos - 1].line == line) {
        loc_index = core_input_state_.pos - 1;
    } else if (core_input_state_.pos >= 0 &&
               core_input_state_.pos < static_cast<int>(core_input_state_.tokens.size())) {
        loc_index = core_input_state_.pos;
    } else if (!core_input_state_.tokens.empty()) {
        loc_index = static_cast<int>(core_input_state_.tokens.size()) - 1;
    }
    if (loc_index >= 0 &&
        loc_index < static_cast<int>(core_input_state_.tokens.size())) {
        n->column = core_input_state_.tokens[loc_index].column;
        if (shared_lookup_state_.token_files &&
            core_input_state_.tokens[loc_index].file_id != kInvalidFile) {
            const std::string file = std::string(
                shared_lookup_state_.token_files->lookup(
                    core_input_state_.tokens[loc_index].file_id));
            if (!file.empty()) n->file = arena_.strdup(file);
        }
    }
    n->ival = -1;  // -1 = not a bitfield (for struct field declarations)
    n->builtin_id = BuiltinId::Unknown;
    n->source_language = is_cpp_mode() ? SourceLanguage::Cxx : SourceLanguage::C;
    n->using_value_alias_target_text_id = kInvalidText;
    n->using_value_alias_target_namespace_context_id = -1;
    n->type.array_size = -1;
    n->type.array_rank = 0;
    for (int i = 0; i < 8; ++i) n->type.array_dims[i] = -1;
    n->type.inner_rank = -1;
    n->type.is_ptr_to_array = false;
    n->execution_domain = ExecutionDomain::Host;
    return n;
}

Node* Parser::make_int_lit(long long v, int line) {
    Node* n = make_node(NK_INT_LIT, line);
    n->ival = v;
    return n;
}

Node* Parser::make_float_lit(double v, const char* raw, int line) {
    Node* n = make_node(NK_FLOAT_LIT, line);
    n->fval = v;
    n->sval = raw;
    return n;
}

Node* Parser::make_str_lit(const char* raw, int line) {
    Node* n = make_node(NK_STR_LIT, line);
    n->sval = raw;
    return n;
}

const char* consume_adjacent_string_literal(Parser& parser) {
    if (!parser.check(TokenKind::StrLit)) return nullptr;

    std::string combined = std::string(parser.token_spelling(parser.cur()));
    parser.consume();
    while (parser.check(TokenKind::StrLit)) {
        if (!combined.empty() && combined.back() == '"') combined.pop_back();
        const std::string next = std::string(parser.token_spelling(parser.cur()));
        if (!next.empty() && next.front() == '"') {
            combined += next.substr(1);
        } else {
            combined += next;
        }
        parser.consume();
    }
    return parser.arena_.strdup(combined);
}

Node* Parser::make_var(const char* name, int line) {
    Node* n = make_node(NK_VAR, line);
    n->name = name;
    return n;
}

Node* Parser::make_binop(const char* op, Node* l, Node* r, int line) {
    Node* n = make_node(NK_BINOP, line);
    n->op    = arena_.strdup(op);
    n->left  = l;
    n->right = r;
    return n;
}

Node* Parser::make_unary(const char* op, Node* operand, int line) {
    Node* n = make_node(NK_UNARY, line);
    n->op   = arena_.strdup(op);
    n->left = operand;
    return n;
}

Node* Parser::make_assign(const char* op, Node* lhs, Node* rhs, int line) {
    Node* n = make_node(NK_ASSIGN, line);
    n->op    = arena_.strdup(op);
    n->left  = lhs;
    n->right = rhs;
    return n;
}

Node* Parser::make_block(Node** stmts, int n_stmts, int line) {
    Node* n = make_node(NK_BLOCK, line);
    n->n_children = n_stmts;
    if (n_stmts > 0 && stmts) {
        n->children = arena_.alloc_array<Node*>(n_stmts);
        for (int i = 0; i < n_stmts; ++i) n->children[i] = stmts[i];
    }
    return n;
}

// ── AST dump ─────────────────────────────────────────────────────────────────

const char* operator_kind_mangled_name(OperatorKind ok) {
    switch (ok) {
        case OP_NONE:      return nullptr;
        case OP_SUBSCRIPT: return "operator_subscript";
        case OP_DEREF:     return "operator_deref";
        case OP_ARROW:     return "operator_arrow";
        case OP_PRE_INC:   return "operator_preinc";
        case OP_POST_INC:  return "operator_postinc";
        case OP_EQ:        return "operator_eq";
        case OP_NEQ:       return "operator_neq";
        case OP_BOOL:      return "operator_bool";
        case OP_PLUS:      return "operator_plus";
        case OP_MINUS:     return "operator_minus";
        case OP_ASSIGN:    return "operator_assign";
        case OP_CALL:      return "operator_call";
        case OP_LT:        return "operator_lt";
        case OP_GT:        return "operator_gt";
        case OP_LE:        return "operator_le";
        case OP_GE:        return "operator_ge";
        case OP_SPACESHIP: return "operator_spaceship";
    }
    return nullptr;
}

const char* node_kind_name(NodeKind k) {
    switch (k) {
        case NK_INT_LIT:      return "IntLit";
        case NK_FLOAT_LIT:    return "FloatLit";
        case NK_STR_LIT:      return "StrLit";
        case NK_CHAR_LIT:     return "CharLit";
        case NK_VAR:          return "Var";
        case NK_BINOP:        return "BinOp";
        case NK_UNARY:        return "Unary";
        case NK_POSTFIX:      return "Postfix";
        case NK_ADDR:         return "Addr";
        case NK_DEREF:        return "Deref";
        case NK_ASSIGN:       return "Assign";
        case NK_COMPOUND_ASSIGN: return "CompoundAssign";
        case NK_CALL:         return "Call";
        case NK_BUILTIN_CALL: return "BuiltinCall";
        case NK_INDEX:        return "Index";
        case NK_MEMBER:       return "Member";
        case NK_CAST:         return "Cast";
        case NK_TERNARY:      return "Ternary";
        case NK_SIZEOF_EXPR:  return "SizeofExpr";
        case NK_SIZEOF_TYPE:  return "SizeofType";
        case NK_SIZEOF_PACK:  return "SizeofPack";
        case NK_ALIGNOF_TYPE: return "AlignofType";
        case NK_ALIGNOF_EXPR: return "AlignofExpr";
        case NK_COMMA_EXPR:   return "CommaExpr";
        case NK_STMT_EXPR:    return "StmtExpr";
        case NK_COMPOUND_LIT: return "CompoundLit";
        case NK_VA_ARG:       return "VaArg";
        case NK_GENERIC:      return "Generic";
        case NK_LAMBDA:       return "Lambda";
        case NK_INIT_LIST:    return "InitList";
        case NK_INIT_ITEM:    return "InitItem";
        case NK_BLOCK:        return "Block";
        case NK_EXPR_STMT:    return "ExprStmt";
        case NK_RETURN:       return "Return";
        case NK_IF:           return "If";
        case NK_WHILE:        return "While";
        case NK_FOR:          return "For";
        case NK_RANGE_FOR:    return "RangeFor";
        case NK_DO_WHILE:     return "DoWhile";
        case NK_SWITCH:       return "Switch";
        case NK_CASE:         return "Case";
        case NK_CASE_RANGE:   return "CaseRange";
        case NK_DEFAULT:      return "Default";
        case NK_BREAK:        return "Break";
        case NK_CONTINUE:     return "Continue";
        case NK_GOTO:         return "Goto";
        case NK_LABEL:        return "Label";
        case NK_ASM:          return "Asm";
        case NK_EMPTY:        return "Empty";
        case NK_STATIC_ASSERT:return "StaticAssert";
        case NK_DECL:         return "Decl";
        case NK_FUNCTION:     return "Function";
        case NK_GLOBAL_VAR:   return "GlobalVar";
        case NK_STRUCT_DEF:   return "StructDef";
        case NK_ENUM_DEF:     return "EnumDef";
        case NK_OFFSETOF:     return "Offsetof";
        case NK_PRAGMA_WEAK:  return "PragmaWeak";
        case NK_PRAGMA_EXEC:  return "PragmaExec";
        case NK_NEW_EXPR:     return "NewExpr";
        case NK_DELETE_EXPR:  return "DeleteExpr";
        case NK_REAL_PART:    return "RealPart";
        case NK_IMAG_PART:    return "ImagPart";
        case NK_INVALID_EXPR: return "InvalidExpr";
        case NK_INVALID_STMT: return "InvalidStmt";
        case NK_PROGRAM:      return "Program";
        default:              return "Unknown";
    }
}

static void indent_print(int depth) {
    for (int i = 0; i < depth * 2; ++i) putchar(' ');
}

void ast_dump(const Node* n, int indent) {
    if (!n) { indent_print(indent); printf("<null>\n"); return; }
    indent_print(indent);
    printf("%s", node_kind_name(n->kind));
    switch (n->kind) {
        case NK_INT_LIT:   printf("(%lld)", n->ival); break;
        case NK_FLOAT_LIT: printf("(%g)", n->fval); break;
        case NK_CHAR_LIT:  printf("(%lld)", n->ival); break;
        case NK_STR_LIT:   printf("(...)"); break;
        case NK_VAR:       printf("(%s)", n->name ? n->name : "?"); break;
        case NK_BINOP:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_UNARY:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_POSTFIX:   printf("(%s)", n->op ? n->op : "?"); break;
        case NK_ASSIGN:    printf("(%s)", n->op ? n->op : "?"); break;
        case NK_MEMBER:    printf("(%s%s)", n->is_arrow ? "->" : ".", n->name ? n->name : "?"); break;
        case NK_FUNCTION:  printf("(%s)", n->name ? n->name : "?"); break;
        case NK_DECL:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_GLOBAL_VAR: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_PRAGMA_EXEC: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_STRUCT_DEF: printf("(%s%s)", n->is_union ? "union " : "struct ", n->name ? n->name : "?"); break;
        case NK_ENUM_DEF:  printf("(enum %s)", n->name ? n->name : "?"); break;
        case NK_GOTO:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_LABEL:     printf("(%s)", n->name ? n->name : "?"); break;
        case NK_ASM:       printf("(%s)", n->is_volatile_asm ? "volatile" : "plain"); break;
        case NK_LAMBDA: {
            const char* capture = "[]";
            if (n->lambda_capture_default == LCD_BY_REFERENCE) capture = "[&]";
            else if (n->lambda_capture_default == LCD_BY_COPY) capture = "[=]";
            printf("(%s", capture);
            if (n->lambda_has_parameter_list) printf(",()");
            printf(")");
            break;
        }
        default: break;
    }
    if (n->n_template_params > 0) {
        printf(" template<");
        for (int i = 0; i < n->n_template_params; ++i) {
            if (i) printf(", ");
            printf("%s", n->template_param_names[i] ? n->template_param_names[i] : "?");
        }
        printf(">");
    }
    if (n->n_template_args > 0) {
        printf(" specialize<%d>", n->n_template_args);
    }
    if (n->is_consteval) printf(" consteval");
    else if (n->is_constexpr) printf(" constexpr");
    if (n->source_language == SourceLanguage::Cxx) printf(" lang=c++");
    if (n->linkage_spec) printf(" linkage=\"%s\"", n->linkage_spec);
    printf("\n");

    // Recurse into children
    if (n->left)   ast_dump(n->left,   indent + 1);
    if (n->right)  ast_dump(n->right,  indent + 1);
    if (n->cond)   ast_dump(n->cond,   indent + 1);
    if (n->then_)  ast_dump(n->then_,  indent + 1);
    if (n->else_)  ast_dump(n->else_,  indent + 1);
    if (n->init)   ast_dump(n->init,   indent + 1);
    if (n->update) ast_dump(n->update, indent + 1);
    if (n->body)   ast_dump(n->body,   indent + 1);

    for (int i = 0; i < n->n_params; ++i) ast_dump(n->params[i], indent + 1);
    for (int i = 0; i < n->n_children; ++i) ast_dump(n->children[i], indent + 1);
    for (int i = 0; i < n->n_fields; ++i) ast_dump(n->fields[i], indent + 1);
}


}  // namespace c4c

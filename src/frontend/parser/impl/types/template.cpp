// template.cpp — template registry, NTTP evaluation
#include "../parser_impl.hpp"
#include "lexer.hpp"

#include <climits>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "types_helpers.hpp"

namespace c4c {

namespace {

QualifiedNameKey template_instantiation_name_key(
    Parser& parser,
    const Node* primary_tpl,
    std::string_view fallback_name) {
    if (!primary_tpl) return {};
    int context_id = primary_tpl->namespace_context_id >= 0
                         ? primary_tpl->namespace_context_id
                         : parser.current_namespace_context_id();
    const char* name = primary_tpl->unqualified_name;
    TextId name_text_id = primary_tpl->unqualified_text_id;
    if ((!name || !name[0]) && primary_tpl->name) {
        name = std::strrchr(primary_tpl->name, ':');
        name = name ? name + 1 : primary_tpl->name;
    }
    if (!name || !name[0]) {
        name = fallback_name.data();
    }
    if (name_text_id == kInvalidText && name && name[0]) {
        name_text_id = parser.parser_text_id_for_token(kInvalidText, name);
    }
    return parser.alias_template_key_in_context(context_id, name_text_id);
}

void mark_template_instantiation_dedup_keys(
    Parser& parser,
    const ParserTemplateState::TemplateInstantiationKey& structured_key) {
    if (structured_key.template_key.base_text_id != kInvalidText) {
        parser.template_state_.instantiated_template_struct_keys_by_key.insert(
            structured_key);
    }
}

}  // namespace

bool Parser::has_template_struct_primary(const QualifiedNameKey& key) const {
    return find_template_struct_primary(key) != nullptr;
}

bool Parser::has_template_struct_primary(int context_id,
                                         TextId name_text_id) const {
    return has_template_struct_primary(
        alias_template_key_in_context(context_id, name_text_id));
}

bool Parser::has_template_struct_primary(const QualifiedNameRef& name) const {
    return find_template_struct_primary(name) != nullptr;
}

Node* Parser::find_template_struct_primary(const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    auto it = template_state_.template_struct_defs_by_key.find(key);
    return it != template_state_.template_struct_defs_by_key.end() ? it->second
                                                                   : nullptr;
}

Node* Parser::find_template_struct_primary(int context_id,
                                           TextId name_text_id) const {
    if (context_id < 0 || name_text_id == kInvalidText) return nullptr;
    if (Node* direct = find_template_struct_primary(
            alias_template_key_in_context(context_id, name_text_id))) {
        return direct;
    }
    const std::string_view name = parser_text(name_text_id, {});
    const VisibleNameResult resolved_type =
        resolve_visible_type(name_text_id, name);
    if (!resolved_type) return nullptr;
    return find_template_struct_primary(alias_template_key_in_context(
        resolved_type.context_id, resolved_type.base_text_id));
}

Node* Parser::find_template_struct_primary(const QualifiedNameRef& name) const {
    const int context_id =
        name.qualifier_segments.empty()
            ? (name.is_global_qualified ? 0 : current_namespace_context_id())
            : resolve_namespace_context(name);
    return find_template_struct_primary(context_id, name.base_text_id);
}

const std::vector<Node*>* Parser::find_template_struct_specializations(
    const QualifiedNameKey& key) const {
    if (key.base_text_id == kInvalidText) return nullptr;
    auto it = template_state_.template_struct_specializations_by_key.find(key);
    return it != template_state_.template_struct_specializations_by_key.end()
               ? &it->second
               : nullptr;
}

const std::vector<Node*>* Parser::find_template_struct_specializations(
    int context_id, TextId name_text_id) const {
    if (context_id < 0 || name_text_id == kInvalidText) return nullptr;
    if (const auto* direct = find_template_struct_specializations(
            alias_template_key_in_context(context_id, name_text_id))) {
        return direct;
    }
    const std::string_view name = parser_text(name_text_id, {});
    const VisibleNameResult resolved_type =
        resolve_visible_type(name_text_id, name);
    if (!resolved_type) return nullptr;
    return find_template_struct_specializations(alias_template_key_in_context(
        resolved_type.context_id, resolved_type.base_text_id));
}

const std::vector<Node*>* Parser::find_template_struct_specializations(
    const Node* primary_tpl) const {
    if (!primary_tpl || !primary_tpl->name) return nullptr;
    if (primary_tpl->namespace_context_id >= 0 &&
        primary_tpl->unqualified_name && primary_tpl->unqualified_name[0]) {
        const TextId name_text_id =
            primary_tpl->unqualified_text_id != kInvalidText
                ? primary_tpl->unqualified_text_id
                : parser_text_id_for_token(kInvalidText,
                                           primary_tpl->unqualified_name);
        return find_template_struct_specializations(primary_tpl->namespace_context_id,
                                                    name_text_id);
    }
    return find_template_struct_specializations(current_namespace_context_id(),
                                                parser_text_id_for_token(
                                                    kInvalidText,
                                                    primary_tpl->name));
}

const std::vector<Node*>* Parser::find_template_struct_specializations(
    const QualifiedNameRef& name, const Node* primary_tpl) const {
    (void)primary_tpl;
    const int context_id =
        name.qualifier_segments.empty()
            ? (name.is_global_qualified ? 0 : current_namespace_context_id())
            : resolve_namespace_context(name);
    return find_template_struct_specializations(context_id, name.base_text_id);
}

const Node* Parser::select_template_struct_pattern_for_args(
    const std::vector<TemplateArgParseResult>& args,
    const Node* primary_tpl,
    const std::vector<Node*>* specializations,
    std::vector<std::pair<std::string, TypeSpec>>* out_type_bindings,
    std::vector<std::pair<std::string, long long>>* out_nttp_bindings) const {
    return select_template_struct_pattern(args, primary_tpl, specializations,
                                          *this, out_type_bindings,
                                          out_nttp_bindings);
}

void Parser::register_template_struct_primary(const QualifiedNameKey& key,
                                              Node* node) {
    if (!node || node->kind != NK_STRUCT_DEF || node->n_template_params <= 0 ||
        node->n_template_args != 0) {
        return;
    }
    if (key.base_text_id != kInvalidText) {
        template_state_.template_struct_defs_by_key[key] = node;
    }
}

void Parser::register_template_struct_primary(int context_id,
                                              TextId name_text_id,
                                              Node* node) {
    register_template_struct_primary(
        alias_template_key_in_context(context_id, name_text_id), node);
}

void Parser::register_template_struct_specialization(const QualifiedNameKey& key,
                                                     Node* node) {
    if (!node) return;
    if (key.base_text_id != kInvalidText) {
        template_state_.template_struct_specializations_by_key[key].push_back(
            node);
    }
}

void Parser::register_template_struct_specialization(
    int context_id, TextId primary_name_text_id, Node* node) {
    register_template_struct_specialization(
        alias_template_key_in_context(context_id, primary_name_text_id), node);
}

bool Parser::ensure_template_struct_instantiated_from_args(
    const std::string& template_name,
    const Node* primary_tpl,
    const std::vector<TemplateArgParseResult>& args,
    int line,
    std::string* out_mangled,
    const char* debug_reason,
    TypeSpec* out_resolved) {
    if (!primary_tpl || !out_mangled) return false;

    std::vector<std::pair<std::string, TypeSpec>> type_bindings;
    std::vector<std::pair<std::string, long long>> nttp_bindings;
    const auto* specializations =
        find_template_struct_specializations(primary_tpl);
    const Node* selected = select_template_struct_pattern_for_args(
        args, primary_tpl, specializations, &type_bindings, &nttp_bindings);
    if (!selected) return false;

    *out_mangled = build_template_struct_mangled_name(
        template_name, primary_tpl, selected, args);
    const ParserTemplateState::TemplateInstantiationKey structured_instance_key{
        template_instantiation_name_key(*this, primary_tpl, template_name),
        make_template_instantiation_argument_keys(args)};

    // Typed fast path: an explicit full specialization already exists as a
    // concrete struct node, so we can register/use it directly without
    // rebuilding tokens and reparsing the instantiation spelling.
    if (selected != primary_tpl && selected->n_template_params == 0 &&
        selected->name && selected->name[0]) {
        if (!definition_state_.struct_tag_def_map.count(*out_mangled)) {
            definition_state_.struct_tag_def_map[*out_mangled] =
                const_cast<Node*>(selected);
            definition_state_.defined_struct_tags.insert(*out_mangled);
        }
        if (out_resolved) {
            *out_resolved = {};
            out_resolved->array_size = -1;
            out_resolved->inner_rank = -1;
            out_resolved->base = TB_STRUCT;
            out_resolved->tag = core_input_state_.arena.strdup(out_mangled->c_str());
            out_resolved->record_def = const_cast<Node*>(selected);
        }
        mark_template_instantiation_dedup_keys(
            *this, structured_instance_key);
        return true;
    }

    if (!definition_state_.struct_tag_def_map.count(*out_mangled)) {
        if (!instantiate_template_struct_via_injected_parse(
                *this, template_name, args, line, debug_reason,
                out_resolved)) {
            return false;
        }
    }
    mark_template_instantiation_dedup_keys(
        *this, structured_instance_key);

    auto injected_it = definition_state_.struct_tag_def_map.find(*out_mangled);
    if (out_resolved && !out_resolved->tag &&
        injected_it != definition_state_.struct_tag_def_map.end()) {
        *out_resolved = {};
        out_resolved->array_size = -1;
        out_resolved->inner_rank = -1;
        out_resolved->base = TB_STRUCT;
        out_resolved->tag = core_input_state_.arena.strdup(out_mangled->c_str());
        out_resolved->record_def = injected_it->second;
    }
    return definition_state_.struct_tag_def_map.count(*out_mangled) > 0;
}

std::string Parser::build_template_struct_mangled_name(
    const std::string& template_name,
    const Node* primary_tpl,
    const Node* selected_tpl,
    const std::vector<TemplateArgParseResult>& args) const {
    if (!primary_tpl) return {};
    if (selected_tpl != primary_tpl && selected_tpl &&
        selected_tpl->n_template_params == 0 &&
        selected_tpl->name && selected_tpl->name[0]) {
        return selected_tpl->name;
    }

    const std::string family =
        (selected_tpl && selected_tpl->template_origin_name &&
         selected_tpl->template_origin_name[0])
            ? selected_tpl->template_origin_name
            : template_name;
    std::string mangled = family;
    int arg_index = 0;
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
        mangled += "_";
        mangled += primary_tpl->template_param_names[pi];
        const bool is_pack =
            primary_tpl->template_param_is_pack &&
            primary_tpl->template_param_is_pack[pi];
        if (is_pack) {
            while (arg_index < static_cast<int>(args.size())) {
                mangled += "_";
                if (args[arg_index].is_value) {
                    mangled += std::to_string(args[arg_index].value);
                } else {
                    append_type_mangled_suffix(mangled, args[arg_index].type);
                }
                ++arg_index;
            }
            continue;
        }
        mangled += "_";
        if (arg_index < static_cast<int>(args.size()) && args[arg_index].is_value) {
            mangled += std::to_string(args[arg_index].value);
        } else if (arg_index < static_cast<int>(args.size()) &&
                   !args[arg_index].is_value) {
            append_type_mangled_suffix(mangled, args[arg_index].type);
        } else {
            mangled +=
                primary_tpl->template_param_is_nttp[pi] ? "0" : "T";
        }
        ++arg_index;
    }
    return mangled;
}

bool Parser::decode_type_ref_text(const std::string& text, TypeSpec* out) {
    if (!out || text.empty()) return false;

    if (const TypeSpec* type =
            find_visible_typedef_type(find_parser_text_id(text))) {
        *out = *type;
        return true;
    }

    if (parse_mangled_type_suffix(text, out)) return true;

    auto init_tag = [&](TypeBase base, size_t prefix_len) {
        *out = {};
        out->array_size = -1;
        out->inner_rank = -1;
        out->base = base;
        out->tag = core_input_state_.arena.strdup(text.substr(prefix_len).c_str());
        out->array_rank = 0;
    };
    if (text.rfind("struct_", 0) == 0) {
        init_tag(TB_STRUCT, 7);
        return true;
    }
    if (text.rfind("union_", 0) == 0) {
        init_tag(TB_UNION, 6);
        return true;
    }
    if (text.rfind("enum_", 0) == 0) {
        init_tag(TB_ENUM, 5);
        return true;
    }

    ParserSnapshot snapshot = save_state();
    const int saved_pos = core_input_state_.pos;
    auto saved_tokens = std::move(core_input_state_.tokens);

    Lexer lexer(text, lex_profile_from(core_input_state_.source_profile));
    std::vector<Token> injected_toks = lexer.scan_all();
    if (injected_toks.empty()) {
        restore_state(snapshot);
        core_input_state_.tokens = std::move(saved_tokens);
        core_input_state_.pos = saved_pos;
        return false;
    }
    core_input_state_.tokens = std::move(injected_toks);
    core_input_state_.pos = 0;

    bool ok = false;
    try {
        *out = parse_type_name();
        *out = resolve_typedef_type_chain(*out);
        ok = core_input_state_.pos > 0 &&
             core_input_state_.pos >=
                 static_cast<int>(core_input_state_.tokens.size()) - 1;
    } catch (...) {
        ok = false;
    }

    restore_state(snapshot);
    core_input_state_.tokens = std::move(saved_tokens);
    core_input_state_.pos = saved_pos;
    if (ok) return true;
    return parse_builtin_typespec_text(text, out);
}

bool Parser::eval_deferred_nttp_expr_tokens(
    const std::string& tpl_name,
    const std::vector<Token>& toks,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings,
    long long* out) {
    if (toks.empty() || !out) return false;

    // Token-based mini expression evaluator for deferred NTTP defaults.
    // Supports: literals, NTTP param names, Trait<T>::member lookups,
    // unary !/+/-, binary */+-, and binary ||, &&, ==, !=, <, >, <=, >=.
    size_t ti = 0;

    // Forward declarations for recursive descent.
    std::function<bool(long long*)> eval_or;
    std::function<bool(long long*)> eval_and;
    std::function<bool(long long*)> eval_eq;
    std::function<bool(long long*)> eval_rel;
    std::function<bool(long long*)> eval_add;
    std::function<bool(long long*)> eval_mul;
    std::function<bool(long long*)> eval_unary;
    std::function<bool(long long*)> eval_primary;

    auto decode_type_tokens = [&](size_t start, size_t end, TypeSpec* out) -> bool {
        if (!out || start >= end || end > toks.size()) return false;
        if (end - start == 1) {
            const Token& tok = toks[start];
            if (tok.kind == TokenKind::Identifier) {
                for (const auto& [pn, pts] : type_bindings) {
                    if (token_spelling(tok) == pn) {
                        *out = pts;
                        return true;
                    }
                }
                if (const TypeSpec* type =
                        find_visible_typedef_type(tok.text_id)) {
                    *out = *type;
                    return true;
                }
                return decode_type_ref_text(std::string(token_spelling(tok)), out);
            }
        }

        std::string text = capture_template_arg_expr_text(
            *this, toks, static_cast<int>(start), static_cast<int>(end));
        return decode_type_ref_text(text, out);
    };

    auto eval_builtin_type_trait = [&](long long* val) -> bool {
        if (ti >= toks.size() || toks[ti].kind != TokenKind::Identifier) return false;
        const std::string builtin_name = std::string(token_spelling(toks[ti]));
        if (!is_builtin_type_trait_name(builtin_name) ||
            builtin_name == "__builtin_types_compatible_p") {
            return false;
        }

        auto find_builtin_type_arg_end = [&](size_t start) -> size_t {
            int angle_depth = 0;
            int paren_depth = 0;
            int bracket_depth = 0;
            int brace_depth = 0;
            size_t pos = start;
            while (pos < toks.size()) {
                const TokenKind tk = toks[pos].kind;
                if (tk == TokenKind::LParen) {
                    ++paren_depth;
                } else if (tk == TokenKind::RParen) {
                    if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0 &&
                        angle_depth == 0) {
                        break;
                    }
                    if (paren_depth > 0) --paren_depth;
                } else if (tk == TokenKind::LBracket) {
                    ++bracket_depth;
                } else if (tk == TokenKind::RBracket) {
                    if (bracket_depth > 0) --bracket_depth;
                } else if (tk == TokenKind::LBrace) {
                    ++brace_depth;
                } else if (tk == TokenKind::RBrace) {
                    if (brace_depth > 0) --brace_depth;
                } else if (tk == TokenKind::Less) {
                    ++angle_depth;
                } else if (tk == TokenKind::Greater) {
                    if (angle_depth > 0) --angle_depth;
                } else if (tk == TokenKind::GreaterGreater) {
                    angle_depth = std::max(0, angle_depth - 2);
                } else if (tk == TokenKind::Comma && paren_depth == 0 &&
                           bracket_depth == 0 && brace_depth == 0 &&
                           angle_depth == 0) {
                    break;
                }
                ++pos;
            }
            return pos;
        };

        const size_t saved_ti = ti;
        ++ti;
        if (ti >= toks.size() || toks[ti].kind != TokenKind::LParen) {
            ti = saved_ti;
            return false;
        }
        ++ti;

        const size_t arg1_start = ti;
        const size_t arg1_end = find_builtin_type_arg_end(arg1_start);
        if (arg1_end <= arg1_start || arg1_end > toks.size()) {
            ti = saved_ti;
            return false;
        }
        TypeSpec lhs{};
        if (!decode_type_tokens(arg1_start, arg1_end, &lhs)) {
            ti = saved_ti;
            return false;
        }
        lhs = resolve_typedef_type_chain(lhs);
        ti = arg1_end;

        auto is_integral_trait_type = [](const TypeSpec& ts) {
            if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
            switch (ts.base) {
                case TB_BOOL:
                case TB_CHAR:
                case TB_SCHAR:
                case TB_UCHAR:
                case TB_SHORT:
                case TB_USHORT:
                case TB_INT:
                case TB_UINT:
                case TB_LONG:
                case TB_ULONG:
                case TB_LONGLONG:
                case TB_ULONGLONG:
                case TB_INT128:
                case TB_UINT128:
                    return true;
                default:
                    return false;
            }
        };

        auto is_floating_trait_type = [](const TypeSpec& ts) {
            if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
            return ts.base == TB_FLOAT || ts.base == TB_DOUBLE ||
                   ts.base == TB_LONGDOUBLE;
        };

        auto is_const_trait_type = [](const TypeSpec& ts) {
            return ts.is_const && !ts.is_lvalue_ref && !ts.is_rvalue_ref;
        };

        auto is_reference_trait_type = [](const TypeSpec& ts) {
            return ts.is_lvalue_ref || ts.is_rvalue_ref;
        };

        auto is_enum_trait_type = [](const TypeSpec& ts) {
            if (ts.ptr_level > 0 || ts.is_fn_ptr || ts.array_rank > 0) return false;
            return ts.base == TB_ENUM;
        };

        if (builtin_name == "__is_same") {
            if (ti >= toks.size() || toks[ti].kind != TokenKind::Comma) {
                ti = saved_ti;
                return false;
            }
            ++ti;
            const size_t arg2_start = ti;
            const size_t arg2_end = find_builtin_type_arg_end(arg2_start);
            if (arg2_end <= arg2_start || arg2_end > toks.size()) {
                ti = saved_ti;
                return false;
            }
            TypeSpec rhs{};
            if (!decode_type_tokens(arg2_start, arg2_end, &rhs)) {
                ti = saved_ti;
                return false;
            }
            rhs = resolve_typedef_type_chain(rhs);
            ti = arg2_end;
            if (ti >= toks.size() || toks[ti].kind != TokenKind::RParen) {
                ti = saved_ti;
                return false;
            }
            ++ti;
            *val = are_types_compatible(lhs, rhs) ? 1 : 0;
            return true;
        }

        if (ti >= toks.size() || toks[ti].kind != TokenKind::RParen) {
            ti = saved_ti;
            return false;
        }
        ++ti;

        if (builtin_name == "__is_integral") {
            *val = is_integral_trait_type(lhs) ? 1 : 0;
            return true;
        }
        if (builtin_name == "__is_floating_point") {
            *val = is_floating_trait_type(lhs) ? 1 : 0;
            return true;
        }
        if (builtin_name == "__is_const") {
            *val = is_const_trait_type(lhs) ? 1 : 0;
            return true;
        }
        if (builtin_name == "__is_reference") {
            *val = is_reference_trait_type(lhs) ? 1 : 0;
            return true;
        }
        if (builtin_name == "__is_enum") {
            *val = is_enum_trait_type(lhs) ? 1 : 0;
            return true;
        }

        ti = saved_ti;
        return false;
    };

    // Helper: resolve a template arg token at position ti.
    auto resolve_template_arg = [&](std::vector<ParsedTemplateArg>& ref_args) -> bool {
        if (ti >= toks.size()) return false;
        if (toks[ti].kind == TokenKind::Identifier) {
            const Token& tok = toks[ti];
            bool found = false;
            for (const auto& [pn, pts] : type_bindings) {
                if (token_spelling(tok) == pn) {
                    ParsedTemplateArg a; a.is_value = false; a.type = pts;
                    ref_args.push_back(a); found = true; break;
                }
            }
            if (!found) {
                for (const auto& [pn, pv] : nttp_bindings) {
                    if (token_spelling(tok) == pn) {
                        ParsedTemplateArg a; a.is_value = true; a.value = pv;
                        ref_args.push_back(a); found = true; break;
                    }
                }
            }
            if (!found) {
                if (const TypeSpec* type =
                        find_visible_typedef_type(tok.text_id)) {
                    ParsedTemplateArg a; a.is_value = false;
                    a.type = *type;
                    ref_args.push_back(a);
                } else {
                    return false;
                }
            }
            ++ti;
            return true;
        } else if (toks[ti].kind == TokenKind::IntLit) {
            ParsedTemplateArg a; a.is_value = true;
            a.value =
                parse_int_lexeme(std::string(token_spelling(toks[ti])).c_str());
            ref_args.push_back(a); ++ti; return true;
        } else if (toks[ti].kind == TokenKind::KwTrue) {
            ParsedTemplateArg a; a.is_value = true; a.value = 1;
            ref_args.push_back(a); ++ti; return true;
        } else if (toks[ti].kind == TokenKind::KwFalse) {
            ParsedTemplateArg a; a.is_value = true; a.value = 0;
            ref_args.push_back(a); ++ti; return true;
        } else {
            // Multi-token arg: prefer the shared typed decoder so qualified
            // names and other non-builtin type spellings do not fall back to
            // debug-text-only handling.
            size_t arg_end = ti;
            int nested_angle_depth = 0;
            while (arg_end < toks.size()) {
                const TokenKind tk = toks[arg_end].kind;
                if (tk == TokenKind::Less) ++nested_angle_depth;
                else if (tk == TokenKind::Greater || tk == TokenKind::GreaterGreater) {
                    if (nested_angle_depth == 0) break;
                    nested_angle_depth -= std::min(nested_angle_depth,
                                                   tk == TokenKind::GreaterGreater ? 2 : 1);
                } else if (tk == TokenKind::Comma && nested_angle_depth == 0) break;
                ++arg_end;
            }
            TypeSpec decoded_ts{};
            if (arg_end > ti && decode_type_tokens(ti, arg_end, &decoded_ts)) {
                ParsedTemplateArg a; a.is_value = false; a.type = decoded_ts;
                ref_args.push_back(a); ti = arg_end; return true;
            }
            return false;
        }
    };

    // Helper: evaluate Trait<args>::member pattern starting at current ti.
    auto eval_member_lookup = [&](long long* val) -> bool {
        if (ti >= toks.size() || toks[ti].kind != TokenKind::Identifier) return false;
        const TextId ref_tpl_name_text_id = toks[ti].text_id;
        std::string ref_tpl_name = std::string(token_spelling(toks[ti]));
        size_t saved_ti = ti;
        ++ti;

        if (ti >= toks.size() || toks[ti].kind != TokenKind::Less) { ti = saved_ti; return false; }
        ++ti;

        std::vector<ParsedTemplateArg> ref_args;
        int angle_depth = 1;
        while (ti < toks.size() && angle_depth > 0) {
            if (toks[ti].kind == TokenKind::Greater) {
                --angle_depth;
                if (angle_depth == 0) { ++ti; break; }
                ++ti;
            } else if (toks[ti].kind == TokenKind::Less) {
                ++angle_depth; ++ti;
            } else if (toks[ti].kind == TokenKind::Comma && angle_depth == 1) {
                ++ti; continue;
            } else if (angle_depth == 1) {
                if (!resolve_template_arg(ref_args)) { ti = saved_ti; return false; }
            } else {
                ++ti;
            }
        }

        if (ti >= toks.size() || toks[ti].kind != TokenKind::ColonColon) { ti = saved_ti; return false; }
        ++ti;
        if (ti >= toks.size() || toks[ti].kind != TokenKind::Identifier) { ti = saved_ti; return false; }
        std::string member_name = std::string(token_spelling(toks[ti]));
        ++ti;

        std::string resolved_ref_tpl_name = ref_tpl_name;
        const Node* ref_primary = find_template_struct_primary(
            current_namespace_context_id(),
            parser_text_id_for_token(kInvalidText, resolved_ref_tpl_name));
        if (!ref_primary) {
            if (const TypeSpec* visible_type =
                    find_visible_typedef_type(ref_tpl_name_text_id)) {
                if (visible_type->tag && visible_type->tag[0]) {
                    resolved_ref_tpl_name = visible_type->tag;
                    ref_primary = find_template_struct_primary(
                        current_namespace_context_id(),
                        parser_text_id_for_token(kInvalidText,
                                                 resolved_ref_tpl_name));
                }
            }
        }
        if (!ref_primary) {
            const VisibleNameResult visible_type =
                resolve_visible_type(ref_tpl_name_text_id, ref_tpl_name);
            const std::string visible_name = visible_name_spelling(visible_type);
            if (visible_type && !visible_name.empty()) {
                resolved_ref_tpl_name = visible_name;
                ref_primary = find_template_struct_primary(
                    visible_type.context_id,
                    visible_type.base_text_id);
            }
        }
        if (!ref_primary) {
            const size_t scope_pos = tpl_name.rfind("::");
            if (scope_pos != std::string::npos) {
                resolved_ref_tpl_name =
                    tpl_name.substr(0, scope_pos + 2) + ref_tpl_name;
                ref_primary = find_template_struct_primary(
                    current_namespace_context_id(),
                    parser_text_id_for_token(kInvalidText,
                                             resolved_ref_tpl_name));
            }
        }
        if (!ref_primary) { ti = saved_ti; return false; }

        std::string ref_mangled;
        TypeSpec resolved_ref_ts{};
        if (!ensure_template_struct_instantiated_from_args(
                resolved_ref_tpl_name, ref_primary, ref_args,
                ref_primary->line, &ref_mangled,
                "template_member_lookup_instantiation",
                &resolved_ref_ts)) {
            ti = saved_ti;
            return false;
        }

        const Node* sdef = resolve_record_type_spec(
            resolved_ref_ts, &definition_state_.struct_tag_def_map);
        if (!sdef) {
            auto sdef_it = definition_state_.struct_tag_def_map.find(ref_mangled);
            if (sdef_it != definition_state_.struct_tag_def_map.end()) {
                sdef = sdef_it->second;
            }
        }
        if (!sdef) {
            ti = saved_ti;
            return false;
        }

        std::function<bool(const Node*)> lookup_static_member_recursive =
            [&](const Node* cur) -> bool {
                if (!cur) return false;
                for (int fi = 0; fi < cur->n_fields; ++fi) {
                    const Node* f = cur->fields[fi];
                    if (!f || f->kind != NK_DECL || !f->is_static) continue;
                    if (!f->name || member_name != f->name) continue;
                    if (f->init) {
                        long long v = 0;
                        if (eval_const_int(
                                f->init, &v, &definition_state_.struct_tag_def_map)) {
                            *val = v;
                            return true;
                        }
                    }
                    if (f->ival >= 0) { *val = f->ival; return true; }
                }
                for (int ci = 0; ci < cur->n_children; ++ci) {
                    const Node* child = cur->children[ci];
                    if (!child || child->kind != NK_DECL || !child->is_static) continue;
                    if (!child->name || member_name != child->name) continue;
                    if (child->init) {
                        long long v = 0;
                        if (eval_const_int(
                                child->init, &v,
                                &definition_state_.struct_tag_def_map)) {
                            *val = v;
                            return true;
                        }
                    }
                }
                for (int bi = 0; bi < cur->n_bases; ++bi) {
                    const TypeSpec& base_ts = cur->base_types[bi];
                    const Node* base_def = resolve_record_type_spec(
                        base_ts, &definition_state_.struct_tag_def_map);
                    if (base_def && lookup_static_member_recursive(base_def))
                        return true;
                }
                return false;
            };

        return lookup_static_member_recursive(sdef);
    };

    // Primary: literal, NTTP param name, parenthesized expr, or member lookup.
    eval_primary = [&](long long* val) -> bool {
        if (ti >= toks.size()) return false;
        if (eval_builtin_type_trait(val)) return true;
        // Parenthesized expression
        if (toks[ti].kind == TokenKind::LParen) {
            ++ti;
            if (!eval_or(val)) return false;
            if (ti >= toks.size() || toks[ti].kind != TokenKind::RParen) return false;
            ++ti;
            return true;
        }
        // Literal
        if (toks[ti].kind == TokenKind::KwTrue) { *val = 1; ++ti; return true; }
        if (toks[ti].kind == TokenKind::KwFalse) { *val = 0; ++ti; return true; }
        if (toks[ti].kind == TokenKind::IntLit) {
            *val =
                parse_int_lexeme(std::string(token_spelling(toks[ti])).c_str());
            ++ti;
            return true;
        }
        // NTTP param name
        if (toks[ti].kind == TokenKind::Identifier) {
            for (const auto& [pn, pv] : nttp_bindings) {
                if (token_spelling(toks[ti]) == pn) { *val = pv; ++ti; return true; }
            }
        }
        // Member lookup: Trait<args>::member
        return eval_member_lookup(val);
    };

    // Unary: !, +, and - prefixes.
    eval_unary = [&](long long* val) -> bool {
        if (ti < toks.size() && toks[ti].kind == TokenKind::Bang) {
            ++ti;
            long long inner = 0;
            if (!eval_unary(&inner)) return false;
            *val = inner ? 0 : 1;
            return true;
        }
        if (ti < toks.size() && toks[ti].kind == TokenKind::Minus) {
            ++ti;
            long long inner = 0;
            if (!eval_unary(&inner)) return false;
            *val = -inner;
            return true;
        }
        if (ti < toks.size() && toks[ti].kind == TokenKind::Plus) {
            ++ti;
            return eval_unary(val);
        }
        return eval_primary(val);
    };

    // Multiplicative: *, /
    eval_mul = [&](long long* val) -> bool {
        if (!eval_unary(val)) return false;
        while (ti < toks.size()) {
            if (toks[ti].kind == TokenKind::Star) {
                ++ti;
                long long rhs = 0;
                if (!eval_unary(&rhs)) return false;
                *val *= rhs;
            } else if (toks[ti].kind == TokenKind::Slash) {
                ++ti;
                long long rhs = 0;
                if (!eval_unary(&rhs) || rhs == 0) return false;
                *val /= rhs;
            } else {
                break;
            }
        }
        return true;
    };

    // Additive: +, -
    eval_add = [&](long long* val) -> bool {
        if (!eval_mul(val)) return false;
        while (ti < toks.size()) {
            if (toks[ti].kind == TokenKind::Plus) {
                ++ti;
                long long rhs = 0;
                if (!eval_mul(&rhs)) return false;
                *val += rhs;
            } else if (toks[ti].kind == TokenKind::Minus) {
                ++ti;
                long long rhs = 0;
                if (!eval_mul(&rhs)) return false;
                *val -= rhs;
            } else {
                break;
            }
        }
        return true;
    };

    // Relational: <, >, <=, >=
    eval_rel = [&](long long* val) -> bool {
        if (!eval_add(val)) return false;
        while (ti < toks.size()) {
            if (toks[ti].kind == TokenKind::LessEqual) {
                ++ti; long long rhs = 0;
                if (!eval_add(&rhs)) return false;
                *val = (*val <= rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::GreaterEqual) {
                ++ti; long long rhs = 0;
                if (!eval_add(&rhs)) return false;
                *val = (*val >= rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::Less) {
                ++ti; long long rhs = 0;
                if (!eval_add(&rhs)) return false;
                *val = (*val < rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::Greater) {
                ++ti; long long rhs = 0;
                if (!eval_add(&rhs)) return false;
                *val = (*val > rhs) ? 1 : 0;
            } else break;
        }
        return true;
    };

    // Equality: ==, !=
    eval_eq = [&](long long* val) -> bool {
        if (!eval_rel(val)) return false;
        while (ti < toks.size()) {
            if (toks[ti].kind == TokenKind::EqualEqual) {
                ++ti; long long rhs = 0;
                if (!eval_rel(&rhs)) return false;
                *val = (*val == rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::BangEqual) {
                ++ti; long long rhs = 0;
                if (!eval_rel(&rhs)) return false;
                *val = (*val != rhs) ? 1 : 0;
            } else break;
        }
        return true;
    };

    // Logical AND: &&
    eval_and = [&](long long* val) -> bool {
        if (!eval_eq(val)) return false;
        while (ti < toks.size() && toks[ti].kind == TokenKind::AmpAmp) {
            ++ti;
            long long rhs = 0;
            if (!eval_eq(&rhs)) return false;
            *val = (*val && rhs) ? 1 : 0;
        }
        return true;
    };

    // Logical OR: ||
    eval_or = [&](long long* val) -> bool {
        if (!eval_and(val)) return false;
        while (ti < toks.size() && toks[ti].kind == TokenKind::PipePipe) {
            ++ti;
            long long rhs = 0;
            if (!eval_and(&rhs)) return false;
            *val = (*val || rhs) ? 1 : 0;
        }
        return true;
    };

    long long value = 0;
    if (!eval_or(&value)) return false;
    if (ti != toks.size()) return false;
    *out = value;
    return true;
}

bool Parser::eval_deferred_nttp_default(
    const QualifiedNameKey& template_key,
    int param_idx,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings,
    long long* out) {
    if (template_key.base_text_id == kInvalidText || param_idx < 0) {
        return false;
    }
    const ParserTemplateState::NttpDefaultExprKey key{template_key, param_idx};
    auto structured_it =
        template_state_.nttp_default_expr_tokens_by_key.find(key);
    if (structured_it == template_state_.nttp_default_expr_tokens_by_key.end()) {
        return false;
    }

    const std::string template_name = bridge_name_in_context(
        template_key.context_id, template_key.base_text_id, {});
    return eval_deferred_nttp_expr_tokens(template_name, structured_it->second,
                                          type_bindings, nttp_bindings, out);
}

bool Parser::eval_deferred_nttp_default(
    const std::string& tpl_name, int param_idx,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings,
    long long* out) {
    const TextId template_text_id = find_parser_text_id(tpl_name);
    QualifiedNameKey structured_key;
    if (template_text_id != kInvalidText) {
        structured_key = alias_template_key_in_context(
            current_namespace_context_id(), template_text_id);
    }
    return eval_deferred_nttp_default(structured_key, param_idx, type_bindings,
                                      nttp_bindings, out);
}

void Parser::cache_nttp_default_expr_tokens(
    const QualifiedNameKey& template_key,
    int param_idx,
    std::vector<Token> toks) {
    if (template_key.base_text_id != kInvalidText && param_idx >= 0) {
        template_state_.nttp_default_expr_tokens_by_key
            [ParserTemplateState::NttpDefaultExprKey{template_key, param_idx}] =
            std::move(toks);
    }
}

}  // namespace c4c

#include "parser.hpp"
#include "parser_internal.hpp"
#include "lexer.hpp"

#include <climits>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace c4c {

namespace {

using ParsedTemplateArg = Parser::TemplateArgParseResult;

std::string spell_qualified_name_for_lookup(const Parser::QualifiedNameRef& qn) {
    std::string name;
    for (size_t i = 0; i < qn.qualifier_segments.size(); ++i) {
        if (i) name += "::";
        name += qn.qualifier_segments[i];
    }
    if (!name.empty()) name += "::";
    name += qn.base_name;
    return name;
}

std::string resolve_qualified_typedef_name(const Parser& parser,
                                           const Parser::QualifiedNameRef& qn) {
    std::string resolved = spell_qualified_name_for_lookup(qn);
    if (!resolved.empty() && parser.typedef_types_.count(resolved) > 0)
        return resolved;

    if (!qn.qualifier_segments.empty() || qn.is_global_qualified) {
        int context_id = parser.resolve_namespace_context(qn);
        if (context_id >= 0) {
            std::string canonical =
                parser.canonical_name_in_context(context_id, qn.base_name);
            if (parser.typedef_types_.count(canonical) > 0)
                return canonical;
        }
        return {};
    }

    resolved = parser.resolve_visible_type_name(qn.base_name);
    if (parser.typedef_types_.count(resolved) > 0)
        return resolved;
    return {};
}

bool parse_alignas_specifier(Parser* parser, TypeSpec* ts, int line) {
    if (!parser->check(TokenKind::KwAlignas)) return false;
    parser->consume();
    if (!parser->check(TokenKind::LParen)) return true;
    parser->consume();
    long long align_val = 0;
    bool have_align = false;
    if (!parser->check(TokenKind::RParen)) {
        if (parser->is_type_start()) {
            TypeSpec align_ts = parser->parse_type_name();
            Node* align_node = parser->make_node(NK_ALIGNOF_TYPE, line);
            align_node->type = align_ts;
            have_align = eval_const_int(align_node, &align_val,
                                        &parser->struct_tag_def_map_,
                                        &parser->const_int_bindings_);
        } else {
            Node* align_expr = parser->parse_assign_expr();
            have_align = eval_const_int(align_expr, &align_val,
                                        &parser->struct_tag_def_map_,
                                        &parser->const_int_bindings_);
        }
    }
    parser->expect(TokenKind::RParen);
    if (ts && have_align && align_val > ts->align_bytes)
        ts->align_bytes = static_cast<int>(align_val);
    return true;
}

bool is_type_template_param(const Node* tpl_def, const char* name) {
    if (!tpl_def || !name) return false;
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
        if (!tpl_def->template_param_is_nttp[i] &&
            tpl_def->template_param_names[i] &&
            std::strcmp(tpl_def->template_param_names[i], name) == 0)
            return true;
    }
    return false;
}

bool is_value_template_param(const Node* tpl_def, const char* name) {
    if (!tpl_def || !name) return false;
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
        if (tpl_def->template_param_is_nttp[i] &&
            tpl_def->template_param_names[i] &&
            std::strcmp(tpl_def->template_param_names[i], name) == 0)
            return true;
    }
    return false;
}

TypeSpec strip_pattern_qualifiers(TypeSpec ts, const TypeSpec& pattern) {
    if (pattern.is_const) ts.is_const = false;
    if (pattern.is_volatile) ts.is_volatile = false;
    ts.ptr_level -= pattern.ptr_level;
    if (ts.ptr_level < 0) ts.ptr_level = 0;
    if (pattern.is_lvalue_ref) ts.is_lvalue_ref = false;
    if (pattern.is_rvalue_ref) ts.is_rvalue_ref = false;
    if (pattern.array_rank > 0) {
        ts.array_rank -= pattern.array_rank;
        if (ts.array_rank < 0) ts.array_rank = 0;
        ts.array_size = ts.array_rank > 0 ? ts.array_dims[0] : -1;
    }
    return ts;
}

bool match_type_pattern(const TypeSpec& pattern_raw, const TypeSpec& actual_raw,
                        const Node* tpl_def,
                        std::unordered_map<std::string, TypeSpec>* type_bindings,
                        const std::unordered_map<std::string, TypeSpec>& typedef_types) {
    TypeSpec pattern = resolve_typedef_chain(pattern_raw, typedef_types);
    TypeSpec actual = resolve_typedef_chain(actual_raw, typedef_types);
    if (pattern.is_const && !actual.is_const) return false;
    if (pattern.is_volatile && !actual.is_volatile) return false;
    if (pattern.ptr_level > actual.ptr_level) return false;
    if (pattern.is_lvalue_ref && !actual.is_lvalue_ref) return false;
    if (pattern.is_rvalue_ref && !actual.is_rvalue_ref) return false;
    if (pattern.array_rank > actual.array_rank) return false;

    if (pattern.base == TB_TYPEDEF && pattern.tag &&
        is_type_template_param(tpl_def, pattern.tag)) {
        TypeSpec bound = strip_pattern_qualifiers(actual, pattern);
        auto it = type_bindings->find(pattern.tag);
        if (it == type_bindings->end()) {
            (*type_bindings)[pattern.tag] = bound;
            return true;
        }
        return types_compatible_p(it->second, bound, typedef_types);
    }

    return types_compatible_p(pattern, actual, typedef_types);
}

int specialization_match_score(const Node* spec) {
    if (!spec) return -1;
    if (spec->n_template_params == 0) return 100000;
    int score = 0;
    for (int i = 0; i < spec->n_template_args; ++i) {
        if (spec->template_arg_is_value && spec->template_arg_is_value[i]) {
            if (!(spec->template_arg_nttp_names && spec->template_arg_nttp_names[i])) score += 8;
            continue;
        }
        const TypeSpec& ts = spec->template_arg_types[i];
        if (ts.base != TB_TYPEDEF || !spec->template_param_names) score += 8;
        else if (!is_type_template_param(spec, ts.tag)) score += 4;
        if (ts.is_const || ts.is_volatile || ts.ptr_level > 0 ||
            ts.is_lvalue_ref || ts.is_rvalue_ref || ts.array_rank > 0)
            score += 4;
    }
    return score;
}

std::string canonical_template_struct_type_key(const TypeSpec& ts) {
    std::string out;
    if (ts.is_const) out += "const_";
    if (ts.is_volatile) out += "volatile_";
    switch (ts.base) {
        case TB_VOID: out += "void"; break;
        case TB_BOOL: out += "bool"; break;
        case TB_CHAR: out += "char"; break;
        case TB_SCHAR: out += "schar"; break;
        case TB_UCHAR: out += "uchar"; break;
        case TB_SHORT: out += "short"; break;
        case TB_USHORT: out += "ushort"; break;
        case TB_INT: out += "int"; break;
        case TB_UINT: out += "uint"; break;
        case TB_LONG: out += "long"; break;
        case TB_ULONG: out += "ulong"; break;
        case TB_LONGLONG: out += "llong"; break;
        case TB_ULONGLONG: out += "ullong"; break;
        case TB_FLOAT: out += "float"; break;
        case TB_DOUBLE: out += "double"; break;
        case TB_LONGDOUBLE: out += "ldouble"; break;
        case TB_INT128: out += "i128"; break;
        case TB_UINT128: out += "u128"; break;
        case TB_STRUCT:
            out += ts.tag ? std::string("struct.") + ts.tag : "struct.?";
            break;
        case TB_UNION:
            out += ts.tag ? std::string("union.") + ts.tag : "union.?";
            break;
        case TB_ENUM:
            out += ts.tag ? std::string("enum.") + ts.tag : "enum.?";
            break;
        case TB_TYPEDEF:
            if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
                out += std::string("pending.") + ts.tpl_struct_origin;
                out += "<";
                out += ts.tpl_struct_arg_refs ? ts.tpl_struct_arg_refs : "";
                out += ">";
            } else {
                out += ts.tag ? std::string("typedef.") + ts.tag : "typedef.?";
            }
            break;
        default:
            if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
                out += std::string("pending.") + ts.tpl_struct_origin;
                out += "<";
                out += ts.tpl_struct_arg_refs ? ts.tpl_struct_arg_refs : "";
                out += ">";
            } else {
                out += "unknown";
            }
            break;
    }
    for (int i = 0; i < ts.ptr_level; ++i) out += "*";
    if (ts.is_lvalue_ref) out += "&";
    if (ts.is_rvalue_ref) out += "&&";
    return out;
}

std::string make_template_struct_instance_key(
    const Node* primary_tpl,
    const std::vector<ParsedTemplateArg>& concrete_args) {
    if (!primary_tpl || !primary_tpl->name) return {};
    std::string key = primary_tpl->name;
    key += "<";
    bool first = true;
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
        const char* param_name = primary_tpl->template_param_names[pi];
        if (!param_name) continue;
        if (!first) key += ",";
        first = false;
        key += param_name;
        key += "=";
        if (pi >= static_cast<int>(concrete_args.size())) {
            key += "?";
            continue;
        }
        if (concrete_args[pi].is_value) {
            if (concrete_args[pi].nttp_name && concrete_args[pi].nttp_name[0] &&
                std::strncmp(concrete_args[pi].nttp_name, "$expr:", 6) == 0) {
                key += concrete_args[pi].nttp_name;
            } else {
                key += std::to_string(concrete_args[pi].value);
            }
        } else {
            key += canonical_template_struct_type_key(concrete_args[pi].type);
        }
    }
    key += ">";
    return key;
}

const Node* select_template_struct_pattern(
    const std::vector<ParsedTemplateArg>& actual_args,
    const Node* primary_tpl,
    const std::vector<Node*>* specialization_patterns,
    const std::unordered_map<std::string, TypeSpec>& typedef_types,
    std::vector<std::pair<std::string, TypeSpec>>* out_type_bindings,
    std::vector<std::pair<std::string, long long>>* out_nttp_bindings) {
    const Node* best = primary_tpl;
    int best_score = -1;

    auto try_candidate = [&](const Node* cand) {
        if (!cand) return;
        if (cand->n_template_args != static_cast<int>(actual_args.size())) return;
        std::unordered_map<std::string, TypeSpec> type_bindings_map;
        std::unordered_map<std::string, long long> value_bindings_map;
        for (int i = 0; i < cand->n_template_args; ++i) {
            const ParsedTemplateArg& actual = actual_args[i];
            const bool pattern_is_value =
                cand->template_arg_is_value && cand->template_arg_is_value[i];
            if (pattern_is_value != actual.is_value) return;
            if (pattern_is_value) {
                const char* pname = cand->template_arg_nttp_names ?
                    cand->template_arg_nttp_names[i] : nullptr;
                if (pname && is_value_template_param(cand, pname)) {
                    auto it = value_bindings_map.find(pname);
                    if (it == value_bindings_map.end()) value_bindings_map[pname] = actual.value;
                    else if (it->second != actual.value) return;
                } else {
                    if (cand->template_arg_values[i] != actual.value) return;
                }
            } else {
                if (!match_type_pattern(cand->template_arg_types[i], actual.type, cand,
                                        &type_bindings_map, typedef_types))
                    return;
            }
        }

        for (int i = 0; i < cand->n_template_params; ++i) {
            const char* pname = cand->template_param_names[i];
            if (!pname) continue;
            const bool has_default =
                cand->template_param_has_default &&
                cand->template_param_has_default[i];
            if (cand->template_param_is_nttp[i]) {
                if (!has_default && value_bindings_map.count(pname) == 0)
                    return;
            } else {
                if (!has_default && type_bindings_map.count(pname) == 0)
                    return;
            }
        }

        int score = specialization_match_score(cand);
        if (score <= best_score) return;
        best = cand;
        best_score = score;
        out_type_bindings->clear();
        out_nttp_bindings->clear();
        for (int i = 0; i < cand->n_template_params; ++i) {
            const char* pname = cand->template_param_names[i];
            if (!pname) continue;
            if (cand->template_param_is_nttp[i]) {
                auto it = value_bindings_map.find(pname);
                if (it != value_bindings_map.end()) {
                    out_nttp_bindings->push_back({pname, it->second});
                } else if (cand->template_param_has_default &&
                           cand->template_param_has_default[i]) {
                    out_nttp_bindings->push_back({pname, cand->template_param_default_values[i]});
                }
            } else {
                auto it = type_bindings_map.find(pname);
                if (it != type_bindings_map.end()) {
                    out_type_bindings->push_back({pname, it->second});
                } else if (cand->template_param_has_default &&
                           cand->template_param_has_default[i]) {
                    out_type_bindings->push_back({pname, cand->template_param_default_types[i]});
                }
            }
        }
    };

    if (specialization_patterns) {
        for (const Node* cand : *specialization_patterns) try_candidate(cand);
    }

    if (best != primary_tpl && best) return best;

    out_type_bindings->clear();
    out_nttp_bindings->clear();
    if (!primary_tpl) return nullptr;
    int arg_idx = 0;
    for (; arg_idx < static_cast<int>(actual_args.size()) &&
           arg_idx < primary_tpl->n_template_params; ++arg_idx) {
        const char* param_name = primary_tpl->template_param_names[arg_idx];
        if (primary_tpl->template_param_is_nttp[arg_idx]) {
            if (!actual_args[arg_idx].is_value) return nullptr;
            out_nttp_bindings->push_back({param_name, actual_args[arg_idx].value});
        } else {
            if (actual_args[arg_idx].is_value) return nullptr;
            out_type_bindings->push_back({param_name, actual_args[arg_idx].type});
        }
    }
    while (arg_idx < primary_tpl->n_template_params) {
        if (primary_tpl->template_param_has_default &&
            primary_tpl->template_param_has_default[arg_idx]) {
            const char* param_name = primary_tpl->template_param_names[arg_idx];
            if (primary_tpl->template_param_is_nttp[arg_idx]) {
                out_nttp_bindings->push_back({param_name, primary_tpl->template_param_default_values[arg_idx]});
            } else {
                out_type_bindings->push_back({param_name, primary_tpl->template_param_default_types[arg_idx]});
            }
        }
        ++arg_idx;
    }
    return primary_tpl;
}

}  // namespace

Node* Parser::find_template_struct_primary(const std::string& name) const {
    auto it = template_struct_defs_.find(name);
    return it != template_struct_defs_.end() ? it->second : nullptr;
}

const std::vector<Node*>* Parser::find_template_struct_specializations(
    const Node* primary_tpl) const {
    if (!primary_tpl || !primary_tpl->name) return nullptr;
    auto it = template_struct_specializations_.find(primary_tpl->name);
    return it != template_struct_specializations_.end() ? &it->second : nullptr;
}

void Parser::register_template_struct_primary(const std::string& name, Node* node) {
    if (!is_primary_template_struct_def(node)) return;
    template_struct_defs_[name] = node;
}

void Parser::register_template_struct_specialization(const char* primary_name, Node* node) {
    if (!primary_name || !primary_name[0] || !node) return;
    template_struct_specializations_[primary_name].push_back(node);
}

static void append_type_mangled_suffix(std::string& out, const TypeSpec& ts);
static std::string capture_template_arg_expr_text(
    const std::vector<Token>& toks, int start, int end);
static bool parse_builtin_typespec_text(const std::string& text, TypeSpec* out);
static int find_template_arg_expr_end(const std::vector<Token>& tokens, int start_pos);

static bool parse_builtin_typespec_text(const std::string& text, TypeSpec* out) {
    if (!out) return false;
    auto trim_local = [](std::string s0) {
        size_t start = 0;
        while (start < s0.size() &&
               std::isspace(static_cast<unsigned char>(s0[start]))) {
            ++start;
        }
        size_t end = s0.size();
        while (end > start &&
               std::isspace(static_cast<unsigned char>(s0[end - 1]))) {
            --end;
        }
        return s0.substr(start, end - start);
    };
    std::string s = trim_local(text);
    if (s.empty()) return false;

    TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;

    auto strip_prefix = [&](const char* prefix, bool* flag) {
        const std::string p(prefix);
        if (s.rfind(p, 0) == 0) {
            *flag = true;
            s = trim_local(s.substr(p.size()));
            return true;
        }
        return false;
    };

    bool progress = true;
    while (progress) {
        progress = false;
        progress |= strip_prefix("const ", &ts.is_const);
        progress |= strip_prefix("volatile ", &ts.is_volatile);
    }

    if (s == "void") ts.base = TB_VOID;
    else if (s == "bool") ts.base = TB_BOOL;
    else if (s == "char") ts.base = TB_CHAR;
    else if (s == "signed char") ts.base = TB_SCHAR;
    else if (s == "unsigned char") ts.base = TB_UCHAR;
    else if (s == "short") ts.base = TB_SHORT;
    else if (s == "unsigned short") ts.base = TB_USHORT;
    else if (s == "int") ts.base = TB_INT;
    else if (s == "unsigned" || s == "unsigned int") ts.base = TB_UINT;
    else if (s == "long") ts.base = TB_LONG;
    else if (s == "unsigned long") ts.base = TB_ULONG;
    else if (s == "long long") ts.base = TB_LONGLONG;
    else if (s == "unsigned long long") ts.base = TB_ULONGLONG;
    else if (s == "float") ts.base = TB_FLOAT;
    else if (s == "double") ts.base = TB_DOUBLE;
    else return false;

    *out = ts;
    return true;
}

// Evaluate a deferred NTTP default expression.
// Handles the pattern: TemplateName<TypeArgs>::static_member
// by instantiating the template with substituted args and looking up
// the static constexpr member value.
bool Parser::eval_deferred_nttp_expr_tokens(
    const std::string& tpl_name,
    const std::vector<Token>& toks,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings,
    long long* out) {
    if (toks.empty() || !out) return false;

    // Token-based mini expression evaluator for deferred NTTP defaults.
    // Supports: literals, NTTP param names, Trait<T>::member lookups,
    // unary !, and binary ||, &&, ==, !=, <, >, <=, >=.
    size_t ti = 0;

    // Forward declarations for recursive descent.
    std::function<bool(long long*)> eval_or;
    std::function<bool(long long*)> eval_and;
    std::function<bool(long long*)> eval_eq;
    std::function<bool(long long*)> eval_rel;
    std::function<bool(long long*)> eval_unary;
    std::function<bool(long long*)> eval_primary;

    // Helper: resolve a template arg token at position ti.
    auto resolve_template_arg = [&](std::vector<ParsedTemplateArg>& ref_args) -> bool {
        if (ti >= toks.size()) return false;
        if (toks[ti].kind == TokenKind::Identifier) {
            bool found = false;
            for (const auto& [pn, pts] : type_bindings) {
                if (toks[ti].lexeme == pn) {
                    ParsedTemplateArg a; a.is_value = false; a.type = pts;
                    ref_args.push_back(a); found = true; break;
                }
            }
            if (!found) {
                for (const auto& [pn, pv] : nttp_bindings) {
                    if (toks[ti].lexeme == pn) {
                        ParsedTemplateArg a; a.is_value = true; a.value = pv;
                        ref_args.push_back(a); found = true; break;
                    }
                }
            }
            if (!found) {
                if (typedef_types_.count(toks[ti].lexeme)) {
                    ParsedTemplateArg a; a.is_value = false;
                    a.type = typedef_types_[toks[ti].lexeme];
                    ref_args.push_back(a);
                } else {
                    return false;
                }
            }
            ++ti;
            return true;
        } else if (toks[ti].kind == TokenKind::IntLit) {
            ParsedTemplateArg a; a.is_value = true;
            a.value = parse_int_lexeme(toks[ti].lexeme.c_str());
            ref_args.push_back(a); ++ti; return true;
        } else if (toks[ti].kind == TokenKind::KwTrue) {
            ParsedTemplateArg a; a.is_value = true; a.value = 1;
            ref_args.push_back(a); ++ti; return true;
        } else if (toks[ti].kind == TokenKind::KwFalse) {
            ParsedTemplateArg a; a.is_value = true; a.value = 0;
            ref_args.push_back(a); ++ti; return true;
        } else {
            // Multi-token arg (e.g. builtin type keywords)
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
            std::string arg_text = capture_template_arg_expr_text(
                toks, static_cast<int>(ti), static_cast<int>(arg_end));
            TypeSpec builtin_ts{};
            if (!arg_text.empty() && parse_builtin_typespec_text(arg_text, &builtin_ts)) {
                ParsedTemplateArg a; a.is_value = false; a.type = builtin_ts;
                ref_args.push_back(a); ti = arg_end; return true;
            }
            return false;
        }
    };

    // Helper: evaluate Trait<args>::member pattern starting at current ti.
    auto eval_member_lookup = [&](long long* val) -> bool {
        if (ti >= toks.size() || toks[ti].kind != TokenKind::Identifier) return false;
        std::string ref_tpl_name = toks[ti].lexeme;
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
        std::string member_name = toks[ti].lexeme;
        ++ti;

        std::string resolved_ref_tpl_name = ref_tpl_name;
        const Node* ref_primary = find_template_struct_primary(resolved_ref_tpl_name);
        if (!ref_primary) {
            const std::string visible_name = resolve_visible_type_name(ref_tpl_name);
            if (!visible_name.empty()) {
                resolved_ref_tpl_name = visible_name;
                ref_primary = find_template_struct_primary(resolved_ref_tpl_name);
            }
        }
        if (!ref_primary) {
            const size_t scope_pos = tpl_name.rfind("::");
            if (scope_pos != std::string::npos) {
                resolved_ref_tpl_name =
                    tpl_name.substr(0, scope_pos + 2) + ref_tpl_name;
                ref_primary = find_template_struct_primary(resolved_ref_tpl_name);
            }
        }
        if (!ref_primary) { ti = saved_ti; return false; }

        std::vector<std::pair<std::string, TypeSpec>> ref_type_bindings;
        std::vector<std::pair<std::string, long long>> ref_nttp_bindings;
        const auto* ref_specializations = find_template_struct_specializations(ref_primary);
        const Node* ref_def = select_template_struct_pattern(
            ref_args, ref_primary, ref_specializations,
            typedef_types_, &ref_type_bindings, &ref_nttp_bindings);
        if (!ref_def) { ti = saved_ti; return false; }

        // Build the concrete struct tag.
        std::string ref_mangled;
        if (ref_def != ref_primary && ref_def->n_template_params == 0 &&
            ref_def->name && ref_def->name[0]) {
            ref_mangled = ref_def->name;
        } else {
            const std::string ref_family =
                (ref_def && ref_def->template_origin_name && ref_def->template_origin_name[0])
                    ? ref_def->template_origin_name : resolved_ref_tpl_name;
            ref_mangled = ref_family;
            for (int pi = 0; pi < ref_primary->n_template_params; ++pi) {
                ref_mangled += "_";
                ref_mangled += ref_primary->template_param_names[pi];
                ref_mangled += "_";
                if (pi < static_cast<int>(ref_args.size()) && ref_args[pi].is_value)
                    ref_mangled += std::to_string(ref_args[pi].value);
                else if (pi < static_cast<int>(ref_args.size()) && !ref_args[pi].is_value)
                    append_type_mangled_suffix(ref_mangled, ref_args[pi].type);
            }
        }

        // Ensure the referenced struct is instantiated via token injection.
        if (!struct_tag_def_map_.count(ref_mangled)) {
            std::vector<Token> inject_toks;
            Token t; t.line = ref_primary->line; t.column = 0;
            auto append_qualified_name_tokens = [&](const std::string& name) {
                size_t start = 0;
                while (start < name.size()) {
                    size_t sep = name.find("::", start);
                    Token name_tok = t;
                    name_tok.kind = TokenKind::Identifier;
                    name_tok.lexeme = sep == std::string::npos
                        ? name.substr(start)
                        : name.substr(start, sep - start);
                    inject_toks.push_back(name_tok);
                    if (sep == std::string::npos) break;
                    Token cc_tok = t;
                    cc_tok.kind = TokenKind::ColonColon;
                    cc_tok.lexeme = "::";
                    inject_toks.push_back(cc_tok);
                    start = sep + 2;
                }
            };
            append_qualified_name_tokens(resolved_ref_tpl_name);
            t.kind = TokenKind::Less; t.lexeme = "<";
            inject_toks.push_back(t);
            for (int ai = 0; ai < (int)ref_args.size(); ++ai) {
                if (ai > 0) { t.kind = TokenKind::Comma; t.lexeme = ","; inject_toks.push_back(t); }
                if (ref_args[ai].is_value) {
                    if (ref_args[ai].value == 0) { t.kind = TokenKind::KwFalse; t.lexeme = "false"; }
                    else if (ref_args[ai].value == 1) { t.kind = TokenKind::KwTrue; t.lexeme = "true"; }
                    else { t.kind = TokenKind::IntLit; t.lexeme = std::to_string(ref_args[ai].value); }
                    inject_toks.push_back(t);
                } else {
                    const TypeSpec& ats = ref_args[ai].type;
                    t.kind = TokenKind::Identifier;
                    if (ats.tag) {
                        t.lexeme = ats.tag;
                    } else {
                        switch (ats.base) {
                            case TB_INT: t.kind = TokenKind::KwInt; t.lexeme = "int"; break;
                            case TB_UINT: t.kind = TokenKind::KwUnsigned; t.lexeme = "unsigned"; break;
                            case TB_CHAR: t.kind = TokenKind::KwChar; t.lexeme = "char"; break;
                            case TB_VOID: t.kind = TokenKind::KwVoid; t.lexeme = "void"; break;
                            case TB_FLOAT: t.kind = TokenKind::KwFloat; t.lexeme = "float"; break;
                            case TB_DOUBLE: t.kind = TokenKind::KwDouble; t.lexeme = "double"; break;
                            case TB_LONG: t.kind = TokenKind::KwLong; t.lexeme = "long"; break;
                            case TB_SHORT: t.kind = TokenKind::KwShort; t.lexeme = "short"; break;
                            case TB_BOOL: t.kind = TokenKind::KwBool; t.lexeme = "bool"; break;
                            default: { std::string nm; append_type_mangled_suffix(nm, ats); t.lexeme = nm; } break;
                        }
                    }
                    inject_toks.push_back(t);
                }
            }
            t.kind = TokenKind::Greater; t.lexeme = ">";
            inject_toks.push_back(t);
            t.kind = TokenKind::Semi; t.lexeme = ";";
            inject_toks.push_back(t);

            int saved_pos = pos_;
            auto saved_parser_toks = std::move(tokens_);
            tokens_ = std::move(inject_toks);
            pos_ = 0;
            try { (void)parse_base_type(); } catch (...) {}
            tokens_ = std::move(saved_parser_toks);
            pos_ = saved_pos;
        }

        auto sdef_it = struct_tag_def_map_.find(ref_mangled);
        if (sdef_it == struct_tag_def_map_.end()) { ti = saved_ti; return false; }
        const Node* sdef = sdef_it->second;

        std::function<bool(const Node*)> lookup_static_member_recursive =
            [&](const Node* cur) -> bool {
                if (!cur) return false;
                for (int fi = 0; fi < cur->n_fields; ++fi) {
                    const Node* f = cur->fields[fi];
                    if (!f || f->kind != NK_DECL || !f->is_static) continue;
                    if (!f->name || member_name != f->name) continue;
                    if (f->init) {
                        long long v = 0;
                        if (eval_const_int(f->init, &v, &struct_tag_def_map_)) { *val = v; return true; }
                    }
                    if (f->ival >= 0) { *val = f->ival; return true; }
                }
                for (int ci = 0; ci < cur->n_children; ++ci) {
                    const Node* child = cur->children[ci];
                    if (!child || child->kind != NK_DECL || !child->is_static) continue;
                    if (!child->name || member_name != child->name) continue;
                    if (child->init) {
                        long long v = 0;
                        if (eval_const_int(child->init, &v, &struct_tag_def_map_)) { *val = v; return true; }
                    }
                }
                for (int bi = 0; bi < cur->n_bases; ++bi) {
                    const TypeSpec& base_ts = cur->base_types[bi];
                    if (!base_ts.tag || !base_ts.tag[0]) continue;
                    auto bit = struct_tag_def_map_.find(base_ts.tag);
                    if (bit != struct_tag_def_map_.end() &&
                        lookup_static_member_recursive(bit->second))
                        return true;
                }
                return false;
            };

        return lookup_static_member_recursive(sdef);
    };

    // Primary: literal, NTTP param name, parenthesized expr, or member lookup.
    eval_primary = [&](long long* val) -> bool {
        if (ti >= toks.size()) return false;
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
            *val = parse_int_lexeme(toks[ti].lexeme.c_str()); ++ti; return true;
        }
        // NTTP param name
        if (toks[ti].kind == TokenKind::Identifier) {
            for (const auto& [pn, pv] : nttp_bindings) {
                if (toks[ti].lexeme == pn) { *val = pv; ++ti; return true; }
            }
        }
        // Member lookup: Trait<args>::member
        return eval_member_lookup(val);
    };

    // Unary: ! and - prefixes.
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
        return eval_primary(val);
    };

    // Relational: <, >, <=, >=
    eval_rel = [&](long long* val) -> bool {
        if (!eval_unary(val)) return false;
        while (ti < toks.size()) {
            if (toks[ti].kind == TokenKind::LessEqual) {
                ++ti; long long rhs = 0;
                if (!eval_unary(&rhs)) return false;
                *val = (*val <= rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::GreaterEqual) {
                ++ti; long long rhs = 0;
                if (!eval_unary(&rhs)) return false;
                *val = (*val >= rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::Less) {
                ++ti; long long rhs = 0;
                if (!eval_unary(&rhs)) return false;
                *val = (*val < rhs) ? 1 : 0;
            } else if (toks[ti].kind == TokenKind::Greater) {
                ++ti; long long rhs = 0;
                if (!eval_unary(&rhs)) return false;
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
    const std::string& tpl_name, int param_idx,
    const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings,
    long long* out) {
    std::string key = tpl_name + ":" + std::to_string(param_idx);
    auto it = nttp_default_expr_tokens_.find(key);
    if (it == nttp_default_expr_tokens_.end()) return false;
    return eval_deferred_nttp_expr_tokens(tpl_name, it->second,
                                          type_bindings, nttp_bindings, out);
}

// Compute vector_lanes from vector_bytes and base type.
// Called after the final base type is resolved.
static void finalize_vector_type(TypeSpec& ts) {
    if (!ts.is_vector || ts.vector_bytes <= 0) return;
    long long elem_sz = 4;
    switch (ts.base) {
        case TB_CHAR: case TB_SCHAR: case TB_UCHAR: elem_sz = 1; break;
        case TB_SHORT: case TB_USHORT: elem_sz = 2; break;
        case TB_FLOAT: case TB_INT: case TB_UINT: elem_sz = 4; break;
        case TB_DOUBLE: case TB_LONG: case TB_ULONG:
        case TB_LONGLONG: case TB_ULONGLONG: elem_sz = 8; break;
        default: elem_sz = 4; break;
    }
    ts.vector_lanes = ts.vector_bytes / elem_sz;
}

// Reverse of append_type_mangled_suffix: convert mangled suffix back to TypeSpec.
static bool parse_mangled_type_suffix(const std::string& text, TypeSpec* out) {
    if (!out) return false;
    TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    if (text == "int") ts.base = TB_INT;
    else if (text == "uint") ts.base = TB_UINT;
    else if (text == "char") ts.base = TB_CHAR;
    else if (text == "schar") ts.base = TB_SCHAR;
    else if (text == "uchar") ts.base = TB_UCHAR;
    else if (text == "short") ts.base = TB_SHORT;
    else if (text == "ushort") ts.base = TB_USHORT;
    else if (text == "long") ts.base = TB_LONG;
    else if (text == "ulong") ts.base = TB_ULONG;
    else if (text == "llong") ts.base = TB_LONGLONG;
    else if (text == "ullong") ts.base = TB_ULONGLONG;
    else if (text == "float") ts.base = TB_FLOAT;
    else if (text == "double") ts.base = TB_DOUBLE;
    else if (text == "ldouble") ts.base = TB_LONGDOUBLE;
    else if (text == "void") ts.base = TB_VOID;
    else if (text == "bool") ts.base = TB_BOOL;
    else if (text == "i128") ts.base = TB_INT128;
    else if (text == "u128") ts.base = TB_UINT128;
    else return false;
    *out = ts;
    return true;
}

static void append_type_mangled_suffix(std::string& out, const TypeSpec& ts) {
    switch (ts.base) {
        case TB_INT: out += "int"; break;
        case TB_UINT: out += "uint"; break;
        case TB_CHAR: out += "char"; break;
        case TB_SCHAR: out += "schar"; break;
        case TB_UCHAR: out += "uchar"; break;
        case TB_SHORT: out += "short"; break;
        case TB_USHORT: out += "ushort"; break;
        case TB_LONG: out += "long"; break;
        case TB_ULONG: out += "ulong"; break;
        case TB_LONGLONG: out += "llong"; break;
        case TB_ULONGLONG: out += "ullong"; break;
        case TB_FLOAT: out += "float"; break;
        case TB_DOUBLE: out += "double"; break;
        case TB_LONGDOUBLE: out += "ldouble"; break;
        case TB_VOID: out += "void"; break;
        case TB_BOOL: out += "bool"; break;
        case TB_INT128: out += "i128"; break;
        case TB_UINT128: out += "u128"; break;
        case TB_STRUCT: out += "struct_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_UNION: out += "union_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_ENUM: out += "enum_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_TYPEDEF: out += (ts.tag ? ts.tag : "typedef"); break;
        default: out += "T"; break;
    }
    for (int p = 0; p < ts.ptr_level; ++p) out += "_ptr";
    if (ts.is_lvalue_ref) out += "_ref";
    if (ts.is_rvalue_ref) out += "_rref";
}

static std::string capture_template_arg_expr_text(
    const std::vector<Token>& tokens, int start_pos, int end_pos) {
    std::string text;
    for (int i = start_pos; i < end_pos && i < static_cast<int>(tokens.size()); ++i)
        text += tokens[i].lexeme;
    return text;
}

static int find_template_arg_expr_end(const std::vector<Token>& tokens, int start_pos) {
    auto can_open_nested_template = [&](int idx) -> bool {
        if (idx <= start_pos || idx >= static_cast<int>(tokens.size())) return false;
        const TokenKind prev = tokens[idx - 1].kind;
        switch (prev) {
            case TokenKind::Identifier:
            case TokenKind::ColonColon:
            case TokenKind::Greater:
            case TokenKind::GreaterGreater:
                return true;
            default:
                return false;
        }
    };
    int scan_pos = start_pos;
    int angle_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;
    while (scan_pos < static_cast<int>(tokens.size())) {
        const TokenKind tk = tokens[scan_pos].kind;
        if (tk == TokenKind::LParen) ++paren_depth;
        else if (tk == TokenKind::RParen) {
            if (paren_depth > 0) --paren_depth;
        } else if (tk == TokenKind::LBracket) ++bracket_depth;
        else if (tk == TokenKind::RBracket) {
            if (bracket_depth > 0) --bracket_depth;
        } else if (tk == TokenKind::LBrace) ++brace_depth;
        else if (tk == TokenKind::RBrace) {
            if (brace_depth > 0) --brace_depth;
            else break;  // unbalanced } — not inside a template argument
        } else if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0) {
            // Semicolons and unbalanced closing braces cannot appear inside
            // a template argument expression — stop scanning so the caller
            // treats the tentative template parse as failed.
            if (tk == TokenKind::Semi) break;
            if (tk == TokenKind::Comma && angle_depth == 0) break;
            if (tk == TokenKind::Greater || tk == TokenKind::GreaterGreater) {
                if (angle_depth == 0) break;
                angle_depth -= std::min(angle_depth,
                                        tk == TokenKind::GreaterGreater ? 2 : 1);
            } else if (tk == TokenKind::Less && can_open_nested_template(scan_pos)) {
                ++angle_depth;
            }
        }
        ++scan_pos;
    }
    return scan_pos;
}

static std::vector<Token> lex_template_expr_text(const std::string& text,
                                                 SourceProfile source_profile) {
    Lexer lexer(text, lex_profile_from(source_profile));
    std::vector<Token> toks = lexer.scan_all();
    if (!toks.empty() && toks.back().kind == TokenKind::EndOfFile) toks.pop_back();
    return toks;
}

static const char* extra_operator_mangled_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::Bang: return "operator_not";
        case TokenKind::Tilde: return "operator_bitnot";
        case TokenKind::MinusMinus: return "operator_dec_pending";
        case TokenKind::PlusPlus: return "operator_inc_pending";
        case TokenKind::Slash: return "operator_div";
        case TokenKind::Percent: return "operator_mod";
        case TokenKind::Amp: return "operator_bitand";
        case TokenKind::Pipe: return "operator_bitor";
        case TokenKind::Caret: return "operator_bitxor";
        case TokenKind::LessLess: return "operator_shl";
        case TokenKind::GreaterGreater: return "operator_shr";
        case TokenKind::PlusAssign: return "operator_plus_assign";
        case TokenKind::MinusAssign: return "operator_minus_assign";
        case TokenKind::StarAssign: return "operator_mul_assign";
        case TokenKind::SlashAssign: return "operator_div_assign";
        case TokenKind::PercentAssign: return "operator_mod_assign";
        case TokenKind::AmpAssign: return "operator_and_assign";
        case TokenKind::PipeAssign: return "operator_or_assign";
        case TokenKind::CaretAssign: return "operator_xor_assign";
        case TokenKind::LessLessAssign: return "operator_shl_assign";
        case TokenKind::GreaterGreaterAssign: return "operator_shr_assign";
        default: return nullptr;
    }
}

static void finalize_pending_operator_name(std::string& name, size_t param_count) {
    if (name.find("operator_star_pending") != std::string::npos) {
        name.replace(name.find("operator_star_pending"),
                     sizeof("operator_star_pending") - 1,
                     param_count == 0 ? "operator_deref" : "operator_mul");
    }
    if (name.find("operator_amp_pending") != std::string::npos) {
        name.replace(name.find("operator_amp_pending"),
                     sizeof("operator_amp_pending") - 1,
                     param_count == 0 ? "operator_addr" : "operator_bitand");
    }
    if (name.find("operator_inc_pending") != std::string::npos) {
        name.replace(name.find("operator_inc_pending"),
                     sizeof("operator_inc_pending") - 1,
                     param_count == 0 ? "operator_preinc" : "operator_postinc");
    }
    if (name.find("operator_dec_pending") != std::string::npos) {
        name.replace(name.find("operator_dec_pending"),
                     sizeof("operator_dec_pending") - 1,
                     param_count == 0 ? "operator_predec" : "operator_postdec");
    }
}
bool Parser::is_typedef_name(const std::string& s) const {
    return typedefs_.count(s) > 0;
}

void Parser::push_template_scope(TemplateScopeKind kind,
                                 const std::vector<TemplateScopeParam>& params) {
    template_scope_stack_.push_back({kind, params});
}

void Parser::pop_template_scope() {
    if (!template_scope_stack_.empty())
        template_scope_stack_.pop_back();
}

bool Parser::is_template_scope_type_param(const std::string& name) const {
    // Walk from innermost scope to outermost.
    for (int i = static_cast<int>(template_scope_stack_.size()) - 1; i >= 0; --i) {
        for (const auto& p : template_scope_stack_[i].params) {
            if (!p.is_nttp && p.name && name == p.name) return true;
        }
    }
    return false;
}

bool Parser::try_parse_template_type_arg(TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;
    int saved_pos = pos_;
    try {
        TypeSpec ts = parse_type_name();
        if (is_cpp_mode() && check(TokenKind::LParen)) skip_paren_group();
        if (is_cpp_mode() && check(TokenKind::Ellipsis)) consume();
        if (!check(TokenKind::Comma) && !check_template_close()) {
            pos_ = saved_pos;
            return false;
        }
        // Reject phantom type-id: if exactly one Identifier token was consumed
        // and that identifier is not a recognized type name, then parse_type_name()
        // fell through to the default TB_INT base with the identifier consumed as
        // a declarator name. This happens for forwarded NTTP names like <N>
        // where N is not a type. Reject so non-type parsing can handle it.
        if (is_cpp_mode() && pos_ == saved_pos + 1 &&
            tokens_[saved_pos].kind == TokenKind::Identifier &&
            !is_typedef_name(tokens_[saved_pos].lexeme) &&
            !is_template_scope_type_param(tokens_[saved_pos].lexeme)) {
            pos_ = saved_pos;
            return false;
        }
        out_arg->is_value = false;
        out_arg->type = ts;
        out_arg->value = 0;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        return true;
    } catch (...) {
        pos_ = saved_pos;
        return false;
    }
}

bool Parser::capture_template_arg_expr(int expr_start, TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;
    const int expr_end = find_template_arg_expr_end(tokens_, expr_start);
    const std::string expr_text =
        capture_template_arg_expr_text(tokens_, expr_start, expr_end);
    if (expr_text.empty()) return false;
    out_arg->is_value = true;
    out_arg->value = 0;
    out_arg->expr = nullptr;
    out_arg->nttp_name = arena_.strdup((std::string("$expr:") + expr_text).c_str());
    pos_ = expr_end;
    return true;
}

bool Parser::try_parse_template_non_type_arg(TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;
    const int expr_start = pos_;
    long long sign = 1;
    if (match(TokenKind::Minus)) sign = -1;
    if (check(TokenKind::KwTrue)) {
        out_arg->is_value = true;
        out_arg->value = 1 * sign;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        consume();
        return true;
    }
    if (check(TokenKind::KwFalse)) {
        out_arg->is_value = true;
        out_arg->value = 0;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        consume();
        return true;
    }
    if (check(TokenKind::CharLit)) {
        Node* lit = parse_primary();
        out_arg->is_value = true;
        out_arg->value = lit ? lit->ival * sign : 0;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        return true;
    }
    if (check(TokenKind::IntLit)) {
        out_arg->is_value = true;
        out_arg->value = parse_int_lexeme(cur().lexeme.c_str()) * sign;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        consume();
        return true;
    }
    if (check(TokenKind::Identifier)) {
        int saved_pos = pos_;
        const char* name = arena_.strdup(cur().lexeme.c_str());
        consume();
        if (check(TokenKind::Comma) || check_template_close()) {
            out_arg->is_value = true;
            out_arg->value = 0;
            out_arg->nttp_name = name;
            out_arg->expr = nullptr;
            return true;
        }
        pos_ = saved_pos;
    } else if (expr_start != pos_) {
        pos_ = expr_start;
    }

    // Prefer a real expression parse before falling back to token capture.
    // This lets `<` participate as an operator inside expressions such as
    // `T(-1) < T(0)` instead of being mistaken for a nested template open.
    {
        const int saved_pos = pos_;
        try {
            Node* expr = parse_assign_expr();
            if (pos_ > expr_start &&
                (check(TokenKind::Comma) || check_template_close())) {
                const std::string expr_text =
                    capture_template_arg_expr_text(tokens_, expr_start, pos_);
                if (!expr_text.empty()) {
                    out_arg->is_value = true;
                    out_arg->value = 0;
                    out_arg->expr = expr;
                    out_arg->nttp_name =
                        arena_.strdup((std::string("$expr:") + expr_text).c_str());
                    return true;
                }
            }
        } catch (...) {
        }
        pos_ = saved_pos;
    }

    return capture_template_arg_expr(expr_start, out_arg);
}

bool Parser::is_clearly_value_template_arg(const Node* primary_tpl, int arg_idx) const {
    const bool expect_value =
        primary_tpl && arg_idx < primary_tpl->n_template_params &&
        primary_tpl->template_param_is_nttp &&
        primary_tpl->template_param_is_nttp[arg_idx];
    return expect_value && (
        check(TokenKind::IntLit) || check(TokenKind::CharLit) ||
        check(TokenKind::KwTrue) || check(TokenKind::KwFalse) ||
        check(TokenKind::Minus) || check(TokenKind::Identifier));
}

bool Parser::parse_template_argument_list(std::vector<TemplateArgParseResult>* out_args,
                                         const Node* primary_tpl) {
    if (!out_args || !check(TokenKind::Less)) return false;

    consume();  // <
    int arg_idx = 0;
    while (!at_end() && !check_template_close()) {
        TemplateArgParseResult arg;
        bool ok = false;
        // Normalized disambiguation order (Clang ParseTemplateArgument style):
        //   1. try type-id
        //   2. try non-type expression
        // The primary_tpl hint (expect_value) is used only to prefer
        // non-type for tokens that are unambiguously value-like, avoiding
        // a redundant (and exception-throwing) type parse attempt.
        if (!is_clearly_value_template_arg(primary_tpl, arg_idx))
            ok = try_parse_template_type_arg(&arg);
        if (!ok) ok = try_parse_template_non_type_arg(&arg);
        if (!ok) return false;
        out_args->push_back(arg);
        ++arg_idx;
        if (!match(TokenKind::Comma)) break;
    }
    return match_template_close();
}

bool Parser::consume_qualified_type_spelling(bool allow_global,
                                             bool consume_final_template_args,
                                             std::string* out_name,
                                             QualifiedNameRef* out_qn) {
    const int saved_pos = pos_;
    QualifiedNameRef qn;

    auto consume_optional_template_args = [&]() -> bool {
        if (!check(TokenKind::Less)) return true;
        int depth = 1;
        consume(); // <
        while (!at_end() && depth > 0) {
            if (check(TokenKind::Less)) {
                ++depth;
                consume();
                continue;
            }
            if (check_template_close()) {
                --depth;
                if (depth == 0) break;
                match_template_close();
                continue;
            }
            consume();
        }
        if (!check_template_close()) {
            pos_ = saved_pos;
            return false;
        }
        match_template_close();
        return true;
    };

    if (allow_global && check(TokenKind::ColonColon)) {
        qn.is_global_qualified = true;
        consume();
    }

    if (!check(TokenKind::Identifier)) {
        pos_ = saved_pos;
        return false;
    }

    while (true) {
        qn.base_name = cur().lexeme;
        consume();

        if (consume_final_template_args && !consume_optional_template_args())
            return false;

        if (!(check(TokenKind::ColonColon) &&
              pos_ + 1 < static_cast<int>(tokens_.size()) &&
              tokens_[pos_ + 1].kind == TokenKind::Identifier))
            break;

        qn.qualifier_segments.push_back(qn.base_name);
        consume(); // ::
    }

    if (out_name) {
        std::string spelled;
        if (qn.is_global_qualified) spelled = "::";
        for (size_t i = 0; i < qn.qualifier_segments.size(); ++i) {
            if (!spelled.empty() && spelled != "::") spelled += "::";
            spelled += qn.qualifier_segments[i];
        }
        if (!spelled.empty() && spelled != "::") spelled += "::";
        spelled += qn.base_name;
        *out_name = std::move(spelled);
    }
    if (out_qn) *out_qn = std::move(qn);
    return true;
}

bool Parser::consume_template_args_followed_by_scope() {
    if (!check(TokenKind::Less)) return false;

    const int saved_pos = pos_;
    int probe = pos_;
    int depth = 0;
    bool balanced = false;
    while (probe < static_cast<int>(tokens_.size())) {
        const TokenKind k = tokens_[probe].kind;
        if (k == TokenKind::Less) {
            ++depth;
        } else if (k == TokenKind::Greater) {
            if (--depth <= 0) {
                ++probe;
                balanced = true;
                break;
            }
        } else if (k == TokenKind::GreaterGreater) {
            depth -= 2;
            if (depth <= 0) {
                ++probe;
                balanced = true;
                break;
            }
        } else if (k == TokenKind::Semi || k == TokenKind::LBrace) {
            break;
        }
        ++probe;
    }

    if (!balanced || probe >= static_cast<int>(tokens_.size()) ||
        tokens_[probe].kind != TokenKind::ColonColon) {
        pos_ = saved_pos;
        return false;
    }

    pos_ = probe;
    return true;
}

bool Parser::consume_member_pointer_owner_prefix() {
    if (!is_cpp_mode()) return false;

    const int saved_pos = pos_;
    if (!consume_qualified_type_spelling(/*allow_global=*/true,
                                         /*consume_final_template_args=*/false,
                                         nullptr, nullptr)) {
        return false;
    }
    consume_template_args_followed_by_scope();
    if (!(check(TokenKind::ColonColon) &&
          pos_ + 1 < static_cast<int>(tokens_.size()) &&
          tokens_[pos_ + 1].kind == TokenKind::Star)) {
        pos_ = saved_pos;
        return false;
    }

    consume(); // ::
    consume(); // *
    return true;
}

bool Parser::try_parse_declarator_member_pointer_prefix(TypeSpec& ts) {
    if (!is_cpp_mode()) return false;

    const int saved_pos = pos_;
    if (!consume_member_pointer_owner_prefix()) {
        pos_ = saved_pos;
        return false;
    }

    ts.ptr_level++;
    return true;
}

void Parser::apply_declarator_pointer_token(TypeSpec& ts, TokenKind pointer_tok,
                                            bool preserve_array_base) {
    if (pointer_tok == TokenKind::AmpAmp) {
        ts.is_rvalue_ref = true;
        return;
    }
    if (pointer_tok == TokenKind::Amp) {
        ts.is_lvalue_ref = true;
        return;
    }

    if (preserve_array_base && (ts.array_rank > 0 || ts.array_size != -1))
        ts.is_ptr_to_array = true;
    ts.ptr_level++;
}

bool Parser::parse_dependent_typename_specifier(std::string* out_name) {
    if (!is_cpp_mode() || !check(TokenKind::Identifier) || cur().lexeme != "typename")
        return false;

    const int saved_pos = pos_;
    consume(); // eat 'typename'

    std::string dep_name;
    if (!consume_qualified_type_spelling(/*allow_global=*/true,
                                         /*consume_final_template_args=*/true,
                                         &dep_name, nullptr)) {
        pos_ = saved_pos;
        return false;
    }

    if (out_name) {
        std::string resolved = resolve_visible_type_name(dep_name);
        if (typedef_types_.count(resolved) == 0) resolved = dep_name;
        *out_name = resolved;
    }
    return true;
}

bool Parser::parse_operator_declarator_name(std::string* out_name) {
    if (!out_name || !match(TokenKind::KwOperator)) return false;

    std::string op_name;
    if (check(TokenKind::KwNew)) {
        consume();
        if (check(TokenKind::LBracket)) {
            consume();
            expect(TokenKind::RBracket);
            op_name = "operator_new_array";
        } else {
            op_name = "operator_new";
        }
    } else if (check(TokenKind::KwDelete)) {
        consume();
        if (check(TokenKind::LBracket)) {
            consume();
            expect(TokenKind::RBracket);
            op_name = "operator_delete_array";
        } else {
            op_name = "operator_delete";
        }
    } else if (check(TokenKind::LBracket)) {
        consume();
        expect(TokenKind::RBracket);
        op_name = "operator_subscript";
    } else if (check(TokenKind::LParen)) {
        consume();
        expect(TokenKind::RParen);
        op_name = "operator_call";
    } else if (check(TokenKind::KwBool)) {
        consume();
        op_name = "operator_bool";
    } else if (check(TokenKind::Star)) {
        consume();
        op_name = "operator_star_pending";
    } else if (check(TokenKind::Arrow)) {
        consume();
        op_name = "operator_arrow";
    } else if (check(TokenKind::PlusPlus)) {
        consume();
        op_name = "operator_inc_pending";
    } else if (check(TokenKind::MinusMinus)) {
        consume();
        op_name = "operator_dec_pending";
    } else if (check(TokenKind::EqualEqual)) {
        consume();
        op_name = "operator_eq";
    } else if (check(TokenKind::BangEqual)) {
        consume();
        op_name = "operator_neq";
    } else if (check(TokenKind::Plus)) {
        consume();
        op_name = "operator_plus";
    } else if (check(TokenKind::Minus)) {
        consume();
        op_name = "operator_minus";
    } else if (check(TokenKind::Assign)) {
        consume();
        op_name = "operator_assign";
    } else if (check(TokenKind::LessEqual)) {
        consume();
        op_name = "operator_le";
    } else if (check(TokenKind::GreaterEqual)) {
        consume();
        op_name = "operator_ge";
    } else if (check(TokenKind::Less)) {
        consume();
        op_name = "operator_lt";
    } else if (check(TokenKind::Greater)) {
        consume();
        op_name = "operator_gt";
    } else if (check(TokenKind::Amp)) {
        consume();
        op_name = "operator_amp_pending";
    } else if (const char* extra_mangled = extra_operator_mangled_name(cur().kind)) {
        op_name = extra_mangled;
        consume();
    } else if (is_type_start()) {
        TypeSpec conv_ts = parse_base_type();
        parse_attributes(&conv_ts);
        while (check(TokenKind::Star)) {
            consume();
            conv_ts.ptr_level++;
        }
        if (check(TokenKind::AmpAmp)) {
            consume();
            conv_ts.is_rvalue_ref = true;
        } else if (check(TokenKind::Amp)) {
            consume();
            conv_ts.is_lvalue_ref = true;
        }
        op_name = "operator_conv_";
        append_type_mangled_suffix(op_name, conv_ts);
    } else {
        return false;
    }

    out_name->append(op_name);
    return true;
}

bool Parser::parse_qualified_declarator_name(std::string* out_name) {
    if (!out_name) return false;

    std::string qualified_name;
    bool parsed_qualified = false;

    auto append_scope_sep = [&]() {
        if (!qualified_name.empty() &&
            (qualified_name.size() < 2 ||
             qualified_name.substr(qualified_name.size() - 2) != "::")) {
            qualified_name += "::";
        }
    };

    if (match(TokenKind::ColonColon))
        qualified_name = "::";

    if (check(TokenKind::KwOperator)) {
        parsed_qualified = parse_operator_declarator_name(&qualified_name);
    } else if (check(TokenKind::Identifier)) {
        qualified_name += cur().lexeme;
        consume();
        parsed_qualified = true;
        consume_template_args_followed_by_scope();
    }

    while (parsed_qualified && match(TokenKind::ColonColon)) {
        append_scope_sep();
        if (check(TokenKind::Identifier)) {
            qualified_name += cur().lexeme;
            consume();
            consume_template_args_followed_by_scope();
            continue;
        }
        if (check(TokenKind::KwOperator)) {
            parsed_qualified = parse_operator_declarator_name(&qualified_name);
            break;
        }
        parsed_qualified = false;
        break;
    }

    if (!parsed_qualified) return false;
    *out_name = std::move(qualified_name);
    return true;
}

bool Parser::is_grouped_declarator_start() const {
    if (!check(TokenKind::LParen)) return false;

    int pk = pos_ + 1;
    while (pk < static_cast<int>(tokens_.size()) &&
           tokens_[pk].kind == TokenKind::KwAttribute) {
        ++pk;
        if (pk < static_cast<int>(tokens_.size()) &&
            tokens_[pk].kind == TokenKind::LParen) {
            int depth = 1;
            ++pk;
            while (pk < static_cast<int>(tokens_.size()) && depth > 0) {
                if (tokens_[pk].kind == TokenKind::LParen) ++depth;
                else if (tokens_[pk].kind == TokenKind::RParen) --depth;
                ++pk;
            }
        }
    }

    bool is_grouped = pk < static_cast<int>(tokens_.size()) &&
                      tokens_[pk].kind == TokenKind::Identifier;
    if (is_grouped && pk + 1 < static_cast<int>(tokens_.size())) {
        const auto next_k = tokens_[pk + 1].kind;
        if (next_k == TokenKind::Comma || next_k == TokenKind::Ellipsis)
            is_grouped = false;
    }
    return is_grouped;
}

bool Parser::is_parenthesized_pointer_declarator_start() {
    if (!check(TokenKind::LParen)) return false;

    int pk = pos_ + 1;
    while (pk < static_cast<int>(tokens_.size()) &&
           tokens_[pk].kind == TokenKind::KwAttribute) {
        ++pk;  // skip __attribute__
        if (pk < static_cast<int>(tokens_.size()) &&
            tokens_[pk].kind == TokenKind::LParen) {
            int depth = 1;
            ++pk;
            while (pk < static_cast<int>(tokens_.size()) && depth > 0) {
                if (tokens_[pk].kind == TokenKind::LParen) ++depth;
                else if (tokens_[pk].kind == TokenKind::RParen) --depth;
                ++pk;
            }
        }
    }
    if (pk >= static_cast<int>(tokens_.size())) return false;

    const TokenKind lookahead = tokens_[pk].kind;
    if (lookahead == TokenKind::Star || lookahead == TokenKind::Caret ||
        (is_cpp_mode() &&
         (lookahead == TokenKind::Amp || lookahead == TokenKind::AmpAmp))) {
        return true;
    }

    if (!is_cpp_mode() ||
        (lookahead != TokenKind::Identifier &&
         lookahead != TokenKind::ColonColon)) {
        return false;
    }

    const int saved_pos = pos_;
    pos_ = pk;
    const bool is_member_ptr_fn = consume_member_pointer_owner_prefix();
    pos_ = saved_pos;
    return is_member_ptr_fn;
}

void Parser::parse_declarator_prefix(TypeSpec& ts, bool* out_is_parameter_pack) {
    if (out_is_parameter_pack) *out_is_parameter_pack = false;

    // Skip qualifiers/attributes that can appear before pointer stars.
    // This handles trailing qualifiers on the base type: e.g. `struct S const *p`
    // where `const` appears after the struct tag but before `*`.
    parse_attributes(&ts);
    while (is_qualifier(cur().kind)) consume();
    parse_attributes(&ts);

    // C++ pointer-to-member declarator prefix: `C::*name` or `ns::C::*name`.
    // Model it as an additional pointer level for parser bring-up so headers
    // like EASTL's invoke_impl(R C::*func, ...) can parse successfully.
    try_parse_declarator_member_pointer_prefix(ts);

    // Count pointer stars (and qualifiers between them).
    while (check(TokenKind::Star)) {
        consume();
        // If the base type already has array dimensions, this star creates a
        // pointer-to-array (e.g. HARD_REG_SET *p where HARD_REG_SET = ulong[2]).
        apply_declarator_pointer_token(ts, TokenKind::Star,
                                       /*preserve_array_base=*/true);
        while (is_qualifier(cur().kind)) consume();
        parse_attributes(&ts);
    }

    if (is_cpp_mode() && check(TokenKind::AmpAmp)) {
        consume();
        apply_declarator_pointer_token(ts, TokenKind::AmpAmp,
                                       /*preserve_array_base=*/false);
        while (is_qualifier(cur().kind)) consume();
        parse_attributes(&ts);
    } else if (is_cpp_mode() && check(TokenKind::Amp)) {
        consume();
        apply_declarator_pointer_token(ts, TokenKind::Amp,
                                       /*preserve_array_base=*/false);
        while (is_qualifier(cur().kind)) consume();
        parse_attributes(&ts);
    }

    while (is_qualifier(cur().kind)) consume();
    if (check(TokenKind::Identifier)) {
        const std::string& lex = cur().lexeme;
        if (lex == "_Nullable" || lex == "_Nonnull" ||
            lex == "_Null_unspecified" || lex == "__nullable" ||
            lex == "__nonnull") {
            consume();
        }
    }

    // C++ template parameter pack declarator: `Args&&... args`
    // or `Ts... value`. The ellipsis belongs to the declarator, not the
    // function's variadic parameter list.
    if (is_cpp_mode() && check(TokenKind::Ellipsis)) {
        consume();
        if (out_is_parameter_pack) *out_is_parameter_pack = true;
    }
    parse_attributes(&ts);
}

bool Parser::try_parse_grouped_declarator(TypeSpec& ts, const char** out_name,
                                          std::vector<long long>* out_dims) {
    if (!is_grouped_declarator_start()) return false;

    consume();  // (
    if (out_name && check(TokenKind::Identifier)) {
        *out_name = arena_.strdup(cur().lexeme);
        consume();
    }
    parse_declarator_array_suffixes(ts, out_dims);
    expect(TokenKind::RParen);
    parse_declarator_array_suffixes(ts, out_dims);
    return true;
}

void Parser::parse_normal_declarator_tail(TypeSpec& ts, const char** out_name,
                                          std::vector<long long>* out_dims) {
    parse_attributes(&ts);

    // Normal declarator: optional identifier name, including C++ qualified names
    // such as `Type::method` or `Type::operator+=`, or free operator functions
    // like `operator==`.
    if (out_name && is_cpp_mode() &&
        (check(TokenKind::Identifier) || check(TokenKind::ColonColon) ||
         check(TokenKind::KwOperator))) {
        int saved_pos = pos_;
        std::string qualified_name;
        if (parse_qualified_declarator_name(&qualified_name)) {
            *out_name = arena_.strdup(qualified_name.c_str());
        } else {
            pos_ = saved_pos;
        }
    }

    if (out_name && !*out_name && check(TokenKind::Identifier)) {
        *out_name = arena_.strdup(cur().lexeme);
        consume();
    }

    parse_attributes(&ts);
    parse_declarator_array_suffixes(ts, out_dims);
}

void Parser::parse_declarator_parameter_list(
    std::vector<Node*>* out_params, bool* out_variadic) {
    if (out_params) out_params->clear();
    if (out_variadic) *out_variadic = false;

    expect(TokenKind::LParen);
    if (!check(TokenKind::RParen)) {
        while (!at_end()) {
            if (check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                consume();
                break;
            }
            if (check(TokenKind::RParen)) break;
            if (!is_type_start()) {
                consume();
                if (match(TokenKind::Comma)) continue;
                break;
            }
            Node* p = parse_param();
            if (p && out_params) out_params->push_back(p);
            if (!match(TokenKind::Comma)) break;
        }
    }
    expect(TokenKind::RParen);
}

void Parser::parse_parenthesized_function_pointer_suffix(
    TypeSpec& ts, bool is_nested_fn_ptr,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic) {
    if (!check(TokenKind::LParen)) return;

    std::vector<Node*> fn_ptr_params;
    bool fn_ptr_variadic = false;
    parse_declarator_parameter_list(&fn_ptr_params, &fn_ptr_variadic);

    // C++ pointer-to-member-function may have cv-qualifiers after params:
    // R (T::*pm)() const  or  R (T::*pm)() volatile
    while (is_qualifier(cur().kind)) consume();
    ts.is_fn_ptr = true;  // confirmed function pointer: (*name)(params)

    if (is_nested_fn_ptr) {
        // For nested fn_ptr: inner params already set on out_fn_ptr_params;
        // the outer params here are the RETURN type's fn_ptr params.
        store_declarator_function_pointer_params(
            out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
            out_ret_fn_ptr_variadic, fn_ptr_params, fn_ptr_variadic);
        return;
    }

    store_declarator_function_pointer_params(
        out_fn_ptr_params, out_n_fn_ptr_params,
        out_fn_ptr_variadic, fn_ptr_params, fn_ptr_variadic);
}

bool Parser::parse_parenthesized_pointer_declarator_inner(
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic) {
    while (check(TokenKind::LBracket)) {
        consume();  // [
        while (!at_end() && !check(TokenKind::RBracket)) consume();
        if (check(TokenKind::RBracket)) consume();  // ]
    }

    bool got_name = false;
    if (out_name && check(TokenKind::Identifier)) {
        *out_name = arena_.strdup(cur().lexeme);
        consume();
        got_name = true;
    }

    while (check(TokenKind::LBracket)) {
        consume();  // [
        while (!at_end() && !check(TokenKind::RBracket)) consume();
        if (check(TokenKind::RBracket)) consume();  // ]
    }

    if (got_name && check(TokenKind::LParen)) {
        skip_paren_group();  // skip own params: (int a, int b)
        return false;
    }

    if (check(TokenKind::LParen)) {
        TypeSpec inner_ts = ts;
        inner_ts.ptr_level = 0;
        parse_declarator(inner_ts, out_name,
                         out_fn_ptr_params, out_n_fn_ptr_params,
                         out_fn_ptr_variadic);
        return true;
    }

    return false;
}

void Parser::parse_parenthesized_pointer_declarator(
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic) {
    bool is_nested_fn_ptr = false;
    std::vector<long long> decl_dims;

    consume();  // (
    skip_attributes();  // skip any __attribute__((...)) before * or ^

    TokenKind pointer_tok = cur().kind;
    if (is_cpp_mode() && consume_member_pointer_owner_prefix()) {
        pointer_tok = TokenKind::Star;
    } else {
        consume();  // consume * or ^ or & or &&
    }
    apply_declarator_pointer_token(ts, pointer_tok,
                                   /*preserve_array_base=*/false);

    while (check(TokenKind::Star)) {
        consume();
        apply_declarator_pointer_token(ts, TokenKind::Star,
                                       /*preserve_array_base=*/false);
    }

    while (check(TokenKind::Identifier)) {
        const std::string& lex = cur().lexeme;
        if (lex == "_Nullable" || lex == "_Nonnull" ||
            lex == "_Null_unspecified" || lex == "__nullable" ||
            lex == "__nonnull" || lex == "__restrict" ||
            lex == "restrict" || lex == "const" || lex == "volatile") {
            consume();
        } else {
            break;
        }
    }

    while (is_qualifier(cur().kind)) consume();

    if (is_cpp_mode()) {
        if (check(TokenKind::AmpAmp)) {
            consume();
            ts.is_rvalue_ref = true;
        } else if (check(TokenKind::Amp)) {
            consume();
            ts.is_lvalue_ref = true;
        }
    }

    is_nested_fn_ptr = parse_parenthesized_pointer_declarator_inner(
        ts, out_name,
        out_fn_ptr_params, out_n_fn_ptr_params,
        out_fn_ptr_variadic);

    expect(TokenKind::RParen);
    parse_parenthesized_function_pointer_suffix(
        ts, is_nested_fn_ptr,
        out_fn_ptr_params, out_n_fn_ptr_params, out_fn_ptr_variadic,
        out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
        out_ret_fn_ptr_variadic);
    parse_declarator_array_suffixes(ts, &decl_dims);
    apply_declarator_array_dims(ts, decl_dims);
    if (!decl_dims.empty()) ts.is_ptr_to_array = true;
}

void Parser::parse_non_parenthesized_declarator(TypeSpec& ts,
                                                const char** out_name) {
    std::vector<long long> decl_dims;
    if (!try_parse_grouped_declarator(ts, out_name, &decl_dims)) {
        parse_normal_declarator_tail(ts, out_name, &decl_dims);
    }
    apply_declarator_array_dims(ts, decl_dims);
}

void Parser::parse_plain_function_declarator_suffix(
    TypeSpec& ts, bool decay_to_function_pointer) {
    if (!check(TokenKind::LParen)) return;

    // Plain `name(params)` suffixes stay with the surrounding declaration
    // parser. Parameters are the one place where the caller immediately
    // decays function types to pointer-to-function after the declarator pass.
    if (!decay_to_function_pointer) return;

    skip_paren_group();
    ts.is_fn_ptr = true;
    ts.ptr_level += 1;
}

void Parser::store_declarator_function_pointer_params(
    Node*** out_params, int* out_n_params, bool* out_variadic,
    const std::vector<Node*>& params, bool variadic) {
    if (out_n_params) *out_n_params = static_cast<int>(params.size());
    if (out_variadic) *out_variadic = variadic;
    if (!out_params || params.empty()) return;

    *out_params = arena_.alloc_array<Node*>(static_cast<int>(params.size()));
    for (int i = 0; i < static_cast<int>(params.size()); ++i)
        (*out_params)[i] = params[i];
}

namespace {

void clear_declarator_array_type(TypeSpec* ts) {
    ts->array_size = -1;
    ts->array_rank = 0;
    for (int i = 0; i < 8; ++i) ts->array_dims[i] = -1;
    ts->is_ptr_to_array = false;
    ts->array_size_expr = nullptr;
}

int declarator_base_array_rank(const TypeSpec& ts) {
    if (ts.array_rank > 0) return ts.array_rank;
    if (ts.array_size != -1) return 1;
    return 0;
}

long long declarator_base_array_dim(const TypeSpec& ts, int index) {
    if (ts.array_rank > 0) {
        if (index >= 0 && index < ts.array_rank) return ts.array_dims[index];
        return -1;
    }
    if (index == 0 && ts.array_size != -1) return ts.array_size;
    return -1;
}

}  // namespace

long long Parser::parse_one_declarator_array_dim(TypeSpec& ts) {
    expect(TokenKind::LBracket);
    long long dim = -2;  // unsized []
    if (!check(TokenKind::RBracket)) {
        // C99 array parameter qualifiers: [static N], [const N], [volatile N],
        // [restrict N], [const *] (VLA pointer). Skip qualifier keywords.
        while (is_qualifier(cur().kind) || cur().kind == TokenKind::KwStatic)
            consume();
        // [const *] or [static *]: unsized VLA pointer
        if (check(TokenKind::Star)) {
            consume();
            expect(TokenKind::RBracket);
            return -2;
        }
        if (check(TokenKind::RBracket)) {
            expect(TokenKind::RBracket);
            return dim;
        }
        Node* sz = parse_assign_expr();
        ts.array_size_expr = sz;
        long long cv = 0;
        if (sz && eval_const_int(sz, &cv, &struct_tag_def_map_) && cv > 0) {
            dim = cv;
        } else if (sz && sz->kind == NK_INT_LIT) {
            dim = sz->ival;
        } else {
            dim = 0;  // dynamic / unknown at parse time
        }
    }
    expect(TokenKind::RBracket);
    return dim;
}

void Parser::parse_declarator_array_suffixes(
    TypeSpec& ts, std::vector<long long>* out_dims) {
    if (!out_dims) return;
    while (check(TokenKind::LBracket))
        out_dims->push_back(parse_one_declarator_array_dim(ts));
}

void Parser::apply_declarator_array_dims(
    TypeSpec& ts, const std::vector<long long>& decl_dims) {
    if (decl_dims.empty()) return;
    // Preserve array_size_expr set by parse_one_declarator_array_dim (for first dim).
    Node* saved_size_expr = ts.array_size_expr;
    // If the base TypeSpec is already a pointer-to-array (e.g. A3_28* paa[2]),
    // record the old (typedef's) array rank as inner_rank so that llvm_ty
    // can produce [N x ptr] rather than [N x [inner x ...]].
    if (ts.ptr_level > 0 && ts.is_ptr_to_array) {
        ts.inner_rank = declarator_base_array_rank(ts);
        ts.is_ptr_to_array = false;
    }
    long long merged[8];
    for (int i = 0; i < 8; ++i) merged[i] = -1;
    int out = 0;
    for (size_t i = 0; i < decl_dims.size() && out < 8; ++i) {
        merged[out++] = decl_dims[i];
    }
    const int old_rank = declarator_base_array_rank(ts);
    for (int i = 0; i < old_rank && out < 8; ++i) {
        merged[out++] = declarator_base_array_dim(ts, i);
    }
    clear_declarator_array_type(&ts);
    ts.array_rank = out;
    for (int i = 0; i < out; ++i) ts.array_dims[i] = merged[i];
    ts.array_size = out > 0 ? ts.array_dims[0] : -1;
    // Restore array_size_expr for first dim when it couldn't be evaluated at parse time.
    if (saved_size_expr && !decl_dims.empty() && decl_dims[0] <= 0)
        ts.array_size_expr = saved_size_expr;
}

Parser::TypenameTemplateParamKind Parser::classify_typename_template_parameter() const {
    if (check(TokenKind::KwClass))
        return TypenameTemplateParamKind::TypeParameter;
    if (!is_cpp_mode() || !check(TokenKind::Identifier) || cur().lexeme != "typename")
        return TypenameTemplateParamKind::TypeParameter;

    int probe = pos_ + 1;
    if (probe < static_cast<int>(tokens_.size()) &&
        tokens_[probe].kind == TokenKind::Identifier) {
        ++probe;
    }

    if (probe >= static_cast<int>(tokens_.size()))
        return TypenameTemplateParamKind::TypeParameter;

    switch (tokens_[probe].kind) {
        case TokenKind::Assign:
        case TokenKind::Comma:
        case TokenKind::Greater:
        case TokenKind::GreaterGreater:
        case TokenKind::Ellipsis:
            return TypenameTemplateParamKind::TypeParameter;
        case TokenKind::KwClass:
            return TypenameTemplateParamKind::TypeParameter;
        case TokenKind::Identifier:
            if (tokens_[probe].lexeme == "typename")
                return TypenameTemplateParamKind::TypeParameter;
            break;
        default:
            break;
    }

    return TypenameTemplateParamKind::TypedNonTypeParameter;
}

bool Parser::is_type_start() const {
    TokenKind k = cur().kind;
    if (is_type_kw(k)) return true;
    if (is_qualifier(k)) return true;
    if (is_storage_class(k)) return true;
    if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval || k == TokenKind::KwExplicit) return true;
    if (k == TokenKind::KwAttribute) return true;  // __attribute__((x)) type cast
    if (k == TokenKind::KwAlignas) return true;
    if (k == TokenKind::KwStaticAssert) return false;
    if (k == TokenKind::Identifier) {
        if (is_cpp_mode() && cur().lexeme == "typename") return true;
        if (is_template_scope_type_param(cur().lexeme)) return true;
        if (is_typedef_name(cur().lexeme)) return true;
        if (typedef_types_.count(resolve_visible_type_name(cur().lexeme)) > 0) return true;
        // C++ fallback: identifier followed by < is likely a template type if
        // the name is registered as a template struct, or if we're inside a
        // struct body where namespace-scoped template names may not resolve.
        if (is_cpp_mode() &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::Less &&
            (find_template_struct_primary(cur().lexeme) ||
             find_template_struct_primary(resolve_visible_type_name(cur().lexeme)) ||
             !current_struct_tag_.empty())) return true;
    }
    if (k == TokenKind::ColonColon) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, true)) {
            std::string qualified = spell_qualified_name_for_lookup(qn);
            if (typedef_types_.count(qualified) > 0) return true;
            if (!qn.qualifier_segments.empty()) {
                int context_id = resolve_namespace_context(qn);
                if (context_id >= 0) qualified = canonical_name_in_context(context_id, qn.base_name);
            }
            if (typedef_types_.count(qualified) > 0) return true;
        }
    }
    // C++ qualified type: StructName::TypedefName or ns::ns2::Type
    if (k == TokenKind::Identifier &&
        pos_ + 2 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
        tokens_[pos_ + 2].kind == TokenKind::Identifier) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, false)) {
            std::string scoped = spell_qualified_name_for_lookup(qn);
            if (typedef_types_.count(scoped) > 0) return true;
            int context_id = resolve_namespace_context(qn);
            if (context_id >= 0) {
                std::string ns_scoped = canonical_name_in_context(context_id, qn.base_name);
                if (typedef_types_.count(ns_scoped) > 0) return true;
                // C++ heuristic: if the qualifier resolves to a known namespace,
                // treat it as a type even if not yet registered (e.g. struct tags
                // defined inside namespace blocks that aren't typedef-tracked).
                // Exception: if the token after the qualified name is '(', this
                // is likely a function call (e.g. eastl::advance(i, d2)), not a
                // type declaration.
                if (is_cpp_mode()) {
                    int after_pos = pos_ + 1 + 2 * static_cast<int>(qn.qualifier_segments.size());
                    if (after_pos < static_cast<int>(tokens_.size()) &&
                        tokens_[after_pos].kind == TokenKind::LParen) {
                        // Likely a function call — don't treat as type.
                    } else {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

// ── skip helpers ─────────────────────────────────────────────────────────────

void Parser::skip_paren_group() {
    expect(TokenKind::LParen);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LParen)) ++depth;
        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_brace_group() {
    expect(TokenKind::LBrace);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LBrace)) ++depth;
        else if (check(TokenKind::RBrace)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_attributes() {
    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (check(TokenKind::LParen)) skip_paren_group();
        if (check(TokenKind::LParen)) skip_paren_group();  // double-paren
    }
    // C++11 [[attribute]] syntax
    while (check(TokenKind::LBracket) && pos_ + 1 < tokens_.size() &&
           tokens_[pos_ + 1].kind == TokenKind::LBracket) {
        consume(); consume(); // [[
        int depth = 1;
        while (pos_ < tokens_.size() && depth > 0) {
            if (check(TokenKind::LBracket)) ++depth;
            else if (check(TokenKind::RBracket)) --depth;
            if (depth > 0) consume();
        }
        if (check(TokenKind::RBracket)) consume(); // first ]
        if (check(TokenKind::RBracket)) consume(); // second ]
    }
    while (check(TokenKind::KwExtension)) {
        consume();
    }
    while (check(TokenKind::KwNoreturn)) {
        consume();
    }
    while (check(TokenKind::Identifier) && cur().lexeme == "noexcept") {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

void Parser::skip_exception_spec() {
    // noexcept / noexcept(expr)
    while (check(TokenKind::Identifier) && cur().lexeme == "noexcept") {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
    // throw() / throw(type-list)
    if (check(TokenKind::Identifier) && cur().lexeme == "throw") {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

void Parser::parse_attributes(TypeSpec* ts) {
    auto apply_aligned_attr = [&](long long align) {
        if (!ts || align <= 0) return;
        if (align > ts->align_bytes) ts->align_bytes = static_cast<int>(align);
    };

    auto apply_vector_size_attr = [&](long long bytes) {
        if (!ts || bytes <= 0) return;
        ts->is_vector = true;
        ts->vector_bytes = bytes;
        // Leave vector_lanes=0; finalize_vector() computes it once the
        // final base type is known.
        ts->vector_lanes = 0;
    };

    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (!check(TokenKind::LParen)) continue;
        consume();  // (
        if (!check(TokenKind::LParen)) {
            skip_paren_group();
            continue;
        }
        consume();  // (

        while (!at_end() && !check(TokenKind::RParen)) {
            if (!check(TokenKind::Identifier)) {
                consume();
                continue;
            }

            const std::string attr_name = cur().lexeme;
            consume();

            if (attr_name == "aligned" || attr_name == "__aligned__") {
                long long align = 16;  // GCC bare aligned => target max useful alignment.
                if (check(TokenKind::LParen)) {
                    consume();
                    if (!check(TokenKind::RParen)) {
                        Node* align_expr = parse_assign_expr();
                        long long align_val = 0;
                        if (align_expr &&
                            eval_const_int(align_expr, &align_val, &struct_tag_def_map_,
                                           &const_int_bindings_) &&
                            align_val > 0) {
                            align = align_val;
                        }
                    }
                    expect(TokenKind::RParen);
                }
                apply_aligned_attr(align);
            } else if (attr_name == "packed" || attr_name == "__packed__") {
                if (ts) ts->is_packed = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else if (attr_name == "vector_size" || attr_name == "__vector_size__") {
                if (check(TokenKind::LParen)) {
                    consume();
                    Node* sz_expr = parse_assign_expr();
                    long long sz_val = 0;
                    eval_const_int(sz_expr, &sz_val, &struct_tag_def_map_,
                                   &const_int_bindings_);
                    expect(TokenKind::RParen);
                    apply_vector_size_attr(sz_val);
                }
            } else if (attr_name == "noinline" || attr_name == "__noinline__") {
                if (ts) ts->is_noinline = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else if (attr_name == "always_inline" || attr_name == "__always_inline__") {
                if (ts) ts->is_always_inline = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else {
                if (check(TokenKind::LParen)) skip_paren_group();
            }

            if (check(TokenKind::Comma)) consume();
        }

        expect(TokenKind::RParen);
        expect(TokenKind::RParen);
    }

    while (check(TokenKind::KwExtension)) consume();
    while (check(TokenKind::KwNoreturn)) consume();

    // If vector_size was set and the base type is already known, compute lanes.
    if (ts) finalize_vector_type(*ts);
}

void Parser::skip_asm() {
    if (check(TokenKind::KwAsm)) {
        consume();
        // Skip optional qualifiers: volatile, __volatile__, goto, inline
        while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
               check(TokenKind::KwGoto))
            consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

// ── type parsing ──────────────────────────────────────────────────────────────

TypeSpec Parser::parse_base_type() {
    last_enum_def_ = nullptr;
    TypeSpec ts{};
    ts.array_size = -1;
    ts.array_rank = 0;
    ts.inner_rank = -1;
    for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
    ts.align_bytes = 0;
    ts.vector_lanes = 0;
    ts.vector_bytes = 0;
    ts.base = TB_INT;  // default

    std::function<bool(const std::string&, const std::string&, TypeSpec*)>
        lookup_struct_member_typedef_recursive =
            [&](const std::string& tag, const std::string& member,
                TypeSpec* out) -> bool {
                if (tag.empty() || member.empty() || !out) return false;
                auto try_lookup = [&](const std::string& scoped) -> bool {
                    auto it = typedef_types_.find(scoped);
                    if (it == typedef_types_.end()) return false;
                    *out = it->second;
                    return true;
                };
                auto try_node_member_typedefs = [&](const Node* sdef) -> bool {
                    if (!sdef || sdef->n_member_typedefs <= 0) return false;
                    for (int i = 0; i < sdef->n_member_typedefs; ++i) {
                        const char* name = sdef->member_typedef_names[i];
                        if (name && member == name) {
                            *out = sdef->member_typedef_types[i];
                            return true;
                        }
                    }
                    return false;
                };
                if (try_lookup(tag + "::" + member)) return true;
                auto def_it = struct_tag_def_map_.find(tag);
                if (def_it == struct_tag_def_map_.end() || !def_it->second) return false;
                const Node* sdef = def_it->second;
                if (try_node_member_typedefs(sdef))
                    return true;
                for (int bi = 0; bi < sdef->n_bases; ++bi) {
                    const TypeSpec& base_ts = sdef->base_types[bi];
                    if (!base_ts.tag || !base_ts.tag[0]) continue;
                    if (lookup_struct_member_typedef_recursive(base_ts.tag, member, out))
                        return true;
                }
                return false;
            };

    bool has_signed   = false;
    bool has_unsigned = false;
    bool has_short    = false;
    int  long_count   = 0;
    bool has_int_kw   = false;
    bool has_char     = false;
    bool has_void     = false;
    bool has_float    = false;
    bool has_double   = false;
    bool has_bool     = false;
    bool has_complex  = false;
    bool has_struct   = false;
    bool has_union    = false;
    bool has_enum     = false;
    bool has_typedef  = false;
    bool done         = false;
    bool base_set     = false;  // true when ts.base was set directly (KwBuiltin, KwInt128, etc.)
    auto infer_typeof_like_operand_type = [&](TypeSpec* out, bool strip_qual) -> bool {
        if (!out) return false;
        if (check(TokenKind::Identifier) && cur().lexeme == "nullptr") {
            out->base = TB_VOID;
            out->ptr_level = 1;
            consume();
            return true;
        }
        if (is_type_start()) {
            bool save_c = out->is_const, save_v = out->is_volatile;
            *out = parse_type_name();
            if (strip_qual) {
                out->is_const = false;
                out->is_volatile = false;
            }
            out->is_const |= save_c;
            out->is_volatile |= save_v;
            return true;
        }
        if (check(TokenKind::Identifier)) {
            std::string id = cur().lexeme;
            consume();
            auto vit = var_types_.find(id);
            if (vit != var_types_.end()) {
                *out = vit->second;
            } else {
                out->base = TB_INT;  // enum constants and unknowns → int
            }
            if (strip_qual) {
                out->is_const = false;
                out->is_volatile = false;
            }
            return true;
        }
        if (check(TokenKind::FloatLit)) {
            const std::string lex = cur().lexeme;
            const bool is_f16 = lex.find("f16") != std::string::npos ||
                                lex.find("F16") != std::string::npos;
            const bool is_f64 = lex.find("f64") != std::string::npos ||
                                lex.find("F64") != std::string::npos;
            const bool is_f128 = lex.find("f128") != std::string::npos ||
                                 lex.find("F128") != std::string::npos;
            const bool is_bf16 = lex.find("bf16") != std::string::npos ||
                                 lex.find("BF16") != std::string::npos;
            const bool has_fixed_width_suffix = is_f16 || is_f64 || is_f128 || is_bf16;
            const bool is_f32 = lex.find("f32") != std::string::npos ||
                                lex.find("F32") != std::string::npos ||
                                (!has_fixed_width_suffix &&
                                 (lex.find('f') != std::string::npos ||
                                  lex.find('F') != std::string::npos));
            const bool is_ldbl = lex.find('l') != std::string::npos ||
                                 lex.find('L') != std::string::npos;
            out->base = (is_f16 || is_f32 || is_bf16) ? TB_FLOAT
                      : (is_f128 || is_ldbl) ? TB_LONGDOUBLE
                      : is_f64 ? TB_DOUBLE
                               : TB_DOUBLE;
            consume();
            return true;
        }
        if (check(TokenKind::IntLit) || check(TokenKind::CharLit) ||
            (is_cpp_mode() && (check(TokenKind::KwTrue) || check(TokenKind::KwFalse)))) {
            out->base = check(TokenKind::IntLit) || check(TokenKind::CharLit) ? TB_INT : TB_BOOL;
            consume();
            return true;
        }
        if (check(TokenKind::StrLit)) {
            out->base = TB_CHAR;
            out->ptr_level = 1;
            consume();
            return true;
        }
        return false;
    };
    while (!done && !at_end()) {
        TokenKind k = cur().kind;
        switch (k) {
            // Qualifiers / storage class — skip but record const
            case TokenKind::KwConst:    ts.is_const    = true; consume(); break;
            case TokenKind::KwVolatile: ts.is_volatile = true; consume(); break;
            case TokenKind::KwRestrict:
            case TokenKind::KwAtomic:
            case TokenKind::KwStatic:
            case TokenKind::KwExtern:
            case TokenKind::KwRegister:
            case TokenKind::KwInline:
            case TokenKind::KwExtension:
            case TokenKind::KwNoreturn:
            case TokenKind::KwThreadLocal:
            case TokenKind::KwConstexpr:
            case TokenKind::KwConsteval:
            case TokenKind::KwExplicit:
                consume(); break;
            case TokenKind::KwAuto:
                if (is_cpp_mode() && !base_set) {
                    ts.base = TB_AUTO;
                    base_set = true;
                    consume(); done = true; break;
                }
                consume(); break;  // ignore auto storage (C mode)

            // Signed / unsigned modifiers
            case TokenKind::KwSigned:   has_signed   = true; consume(); break;
            case TokenKind::KwUnsigned: has_unsigned = true; consume(); break;

            // Base type keywords
            case TokenKind::KwVoid:   has_void   = true; consume(); break;
            case TokenKind::KwChar:   has_char   = true; consume(); break;
            case TokenKind::KwShort:  has_short  = true; consume(); break;
            case TokenKind::KwInt:    has_int_kw = true; consume(); break;
            case TokenKind::KwLong:   long_count++;      consume(); break;
            case TokenKind::KwFloat:  has_float  = true; consume(); break;
            case TokenKind::KwDouble: has_double = true; consume(); break;
            case TokenKind::KwBool:   has_bool   = true; consume(); break;
            case TokenKind::KwComplex: has_complex = true; consume(); break;

            case TokenKind::KwInt128:  ts.base = has_unsigned ? TB_UINT128 : TB_INT128;
                                       base_set = true; consume(); done = true; break;
            case TokenKind::KwUInt128: ts.base = TB_UINT128; base_set = true; consume(); done = true; break;

            case TokenKind::KwBuiltin:
                ts.base = TB_VA_LIST;
                ts.tag  = arena_.strdup("__va_list");
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwAutoType:
                // __auto_type: treat as int for now
                ts.base = TB_INT;
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwStruct:
            case TokenKind::KwClass:
                has_struct = true;
                consume();
                done = true;
                break;

            case TokenKind::KwUnion: has_union = true; consume(); done = true; break;
            case TokenKind::KwEnum:  has_enum  = true; consume(); done = true; break;

            case TokenKind::KwTypeofUnqual:
            case TokenKind::KwTypeof: {
                // typeof(type) or typeof(expr)
                bool strip_qual = (cur().kind == TokenKind::KwTypeofUnqual);
                consume();  // consume typeof / typeof_unqual
                expect(TokenKind::LParen);  // consume (
                if (infer_typeof_like_operand_type(&ts, strip_qual)) {
                    // Skip remaining tokens (handles expressions like `i + 1`) to closing )
                    int depth = 0;
                    while (!at_end() && !(check(TokenKind::RParen) && depth == 0)) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) --depth;
                        consume();
                    }
                    expect(TokenKind::RParen);
                } else {
                    // Complex expression: skip to balanced ) returning int
                    int depth = 1;
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); break; } }
                        consume();
                    }
                    ts.base = TB_INT;
                    if (strip_qual) {
                        ts.is_const = false;
                        ts.is_volatile = false;
                    }
                }
                base_set = true;
                done = true; break;
            }

            case TokenKind::KwDecltype: {
                consume();  // consume decltype
                expect(TokenKind::LParen);
                if (infer_typeof_like_operand_type(&ts, /*strip_qual=*/false)) {
                    int depth = 0;
                    while (!at_end() && !(check(TokenKind::RParen) && depth == 0)) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) --depth;
                        consume();
                    }
                    expect(TokenKind::RParen);
                } else {
                    // C++ mode or unknown content: skip balanced parens.
                    // decltype expressions can be arbitrarily complex
                    // (qualified calls, template args, commas, etc.) so
                    // just skip to the closing paren and assume int.
                    int depth = 1;
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) {
                            if (--depth == 0) { consume(); break; }
                        }
                        consume();
                    }
                    ts.base = TB_INT;
                }
                base_set = true;
                done = true; break;
            }

            case TokenKind::KwStaticAssert: {
                // _Static_assert(expr, msg); - skip
                consume();
                skip_paren_group();
                match(TokenKind::Semi);
                done = true; break;
            }

            case TokenKind::ColonColon:
                if (!is_cpp_mode()) {
                    done = true;
                    break;
                }
                [[fallthrough]];

            case TokenKind::Identifier:
                // C++ 'typename' keyword: consume it and the full dependent type
                // expression (e.g., typename Container::value_type, typename T::type)
                if (is_cpp_mode() && cur().lexeme == "typename") {
                    std::string resolved;
                    if (parse_dependent_typename_specifier(&resolved)) {
                        has_typedef = true;
                        ts.tag = arena_.strdup(resolved.c_str());
                        done = true;
                    }
                    break;
                }
                if (is_cpp_mode()) {
                    QualifiedNameRef qn;
                    if (peek_qualified_name(&qn, true)) {
                        const bool already_have_base =
                            has_signed || has_unsigned || has_short || long_count > 0 ||
                            has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                            has_struct || has_union || has_enum || base_set;
                        std::string resolved_name = resolve_qualified_typedef_name(*this, qn);
                        if (!already_have_base && typedef_types_.count(resolved_name) > 0) {
                            has_typedef = true;
                            ts.tag = arena_.strdup(resolved_name.c_str());
                            ts.is_global_qualified = qn.is_global_qualified;
                            ts.n_qualifier_segments =
                                static_cast<int>(qn.qualifier_segments.size());
                            if (ts.n_qualifier_segments > 0) {
                                ts.qualifier_segments =
                                    arena_.alloc_array<const char*>(ts.n_qualifier_segments);
                                for (int i = 0; i < ts.n_qualifier_segments; ++i) {
                                    ts.qualifier_segments[i] =
                                        arena_.strdup(qn.qualifier_segments[i].c_str());
                                }
                            }
                            consume_qualified_type_spelling(
                                /*allow_global=*/true,
                                /*consume_final_template_args=*/false,
                                nullptr, nullptr);
                            done = true;
                            break;
                        }
                        // C++ fallback: unresolved qualified name (e.g. ns::Type)
                        // treated as an unresolved type so template/header parsing
                        // can proceed without failing on unknown namespace types.
                        if (!already_have_base && !qn.qualifier_segments.empty()) {
                            std::string full_name = spell_qualified_name_for_lookup(qn);
                            has_typedef = true;
                            ts.tag = arena_.strdup(full_name.c_str());
                            consume_qualified_type_spelling(
                                /*allow_global=*/true,
                                /*consume_final_template_args=*/false,
                                nullptr, nullptr);
                            done = true;
                            break;
                        }
                    }
                }
                if (is_typedef_name(cur().lexeme) ||
                    is_template_scope_type_param(cur().lexeme) ||
                    typedef_types_.count(resolve_visible_type_name(cur().lexeme)) > 0) {
                    // If we've already seen concrete type specifiers/modifiers,
                    // this identifier is the declarator name (e.g. `int s;` even
                    // when `s` is also a typedef name in outer scope).
                    bool already_have_base =
                        has_signed || has_unsigned || has_short || long_count > 0 ||
                        has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                        has_struct || has_union || has_enum || base_set;
                    // C++ pointer-to-member declarators start with a class name
                    // after the pointee type: `R C::*member`. In that shape the
                    // class name belongs to the declarator, not the base type.
                    if (is_cpp_mode() &&
                        pos_ + 2 < static_cast<int>(tokens_.size()) &&
                        tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
                        tokens_[pos_ + 2].kind == TokenKind::Star) {
                        done = true;
                        break;
                    }
                    if (!already_have_base) {
                        has_typedef = true;
                        const std::string resolved = resolve_visible_type_name(cur().lexeme);
                        ts.tag = arena_.strdup(resolved.c_str());
                        consume();
                    }
                    done = true;
                } else if (is_cpp_mode() &&
                           !(has_signed || has_unsigned || has_short || long_count > 0 ||
                             has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                             has_struct || has_union || has_enum || base_set) &&
                           pos_ + 1 < static_cast<int>(tokens_.size()) &&
                           tokens_[pos_ + 1].kind == TokenKind::Less) {
                    // C++ unresolved template type: identifier followed by <
                    // (e.g. reverse_iterator<Iterator1> inside a namespace where
                    // the typedef registration was lost due to template parsing).
                    has_typedef = true;
                    ts.tag = arena_.strdup(cur().lexeme);
                    consume();
                    done = true;
                } else {
                    done = true;  // end of type; identifier is the declarator name
                }
                break;

            // __attribute__ may appear within type specs (e.g., packed struct)
            case TokenKind::KwAttribute:
                parse_attributes(&ts);
                break;

            case TokenKind::KwAlignas: {
                parse_alignas_specifier(this, &ts, cur().line);
                break;
            }

            default:
                done = true; break;
        }
    }

    // Handle struct/union/enum after switch (needs extra parsing)
    if (has_struct) {
        Node* sd = parse_struct_or_union(false);
        ts.base = TB_STRUCT;
        ts.tag  = sd ? sd->name : nullptr;
        // In C++ mode, 'struct Pair<int>' should trigger template struct instantiation
        // just like 'Pair<int>' does via the typedef path. If the tag matches a known
        // template struct and '<' follows, fall through to the template instantiation
        // code below instead of returning immediately.
        if (!(is_cpp_mode() && ts.tag && find_template_struct_primary(ts.tag) &&
              check(TokenKind::Less))) {
            return ts;
        }
        // Fall through to template struct instantiation below
    }
    if (has_union) {
        Node* sd = parse_struct_or_union(true);
        ts.base = TB_UNION;
        ts.tag  = sd ? sd->name : nullptr;
        return ts;
    }
    if (has_enum) {
        Node* ed = parse_enum();
        last_enum_def_ = ed;
        ts.base = TB_ENUM;
        ts.tag  = ed ? ed->name : nullptr;
        return ts;
    }

    // If base was set directly (KwBuiltin, KwInt128, etc.), skip combined-specifier resolution.
    if (base_set) {
        finalize_vector_type(ts);
        return ts;
    }

    // Template struct instantiation for 'struct Pair<int>' syntax (has_struct fall-through).
    // ts.base is already TB_STRUCT and ts.tag is the template name.
    if (has_struct && is_cpp_mode() && ts.tag &&
        find_template_struct_primary(ts.tag) && check(TokenKind::Less)) {
        // Reuse the typedef-path template instantiation by setting has_typedef and
        // preparing ts as if the typedef had been resolved.
        has_typedef = true;
        has_struct = false;
    }

    // Resolve combined specifiers
    if (has_typedef) {
        // Try to resolve the typedef to its underlying TypeSpec
        const char* tname = ts.tag;
        if (tname) {
            auto it = typedef_types_.find(tname);
            if (it != typedef_types_.end()) {
                // Resolve: use the stored TypeSpec, preserving qualifiers from this context
                bool save_const = ts.is_const, save_vol = ts.is_volatile;
                ts = it->second;
                ts.is_const   |= save_const;
                ts.is_volatile |= save_vol;
                // Phase C: remember the typedef name for fn_ptr param propagation.
                last_resolved_typedef_ = tname;
                // Alias template application: e.g. bool_constant<expr> → integral_constant<bool, expr>
                if (is_cpp_mode() && check(TokenKind::Less)) {
                    auto ati_it = alias_template_info_.find(tname);
                    if (ati_it != alias_template_info_.end() &&
                        ati_it->second.aliased_type.tpl_struct_origin) {
                        const AliasTemplateInfo& ati = ati_it->second;
                        const int save_pos_alias = pos_;
                        std::vector<TemplateArgParseResult> alias_args;
                        if (!parse_template_argument_list(&alias_args) ||
                            alias_args.size() != ati.param_names.size()) {
                            pos_ = save_pos_alias;
                        } else {
                            std::unordered_map<std::string, std::string> subst;
                            bool alias_parse_ok = true;
                            for (size_t ai = 0; ai < alias_args.size(); ++ai) {
                                if (ati.param_is_nttp[ai] != alias_args[ai].is_value) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                if (alias_args[ai].is_value) {
                                    if (alias_args[ai].nttp_name && alias_args[ai].nttp_name[0]) {
                                        subst[ati.param_names[ai]] = alias_args[ai].nttp_name;
                                    } else {
                                        subst[ati.param_names[ai]] = std::to_string(alias_args[ai].value);
                                    }
                                } else {
                                    std::string mangled;
                                    if (alias_args[ai].type.tag) mangled = alias_args[ai].type.tag;
                                    else append_type_mangled_suffix(mangled, alias_args[ai].type);
                                    subst[ati.param_names[ai]] = mangled;
                                }
                            }
                            if (!alias_parse_ok) {
                                pos_ = save_pos_alias;
                            } else {
                            // Substitute alias params in the aliased type's arg_refs.
                                ts = ati.aliased_type;
                                if (ts.tpl_struct_arg_refs) {
                                    std::string old_refs = ts.tpl_struct_arg_refs;
                                    std::string new_refs;
                                    std::istringstream ss(old_refs);
                                    std::string part;
                                    bool first = true;
                                    while (std::getline(ss, part, ',')) {
                                        if (!first) new_refs += ",";
                                        first = false;
                                        auto s_it = subst.find(part);
                                        if (s_it != subst.end())
                                            new_refs += s_it->second;
                                        else
                                            new_refs += part;
                                    }
                                    ts.tpl_struct_arg_refs = arena_.strdup(new_refs.c_str());
                                    // Update tag (mangled name) to reflect substituted args.
                                    if (ts.tpl_struct_origin) {
                                        std::string new_tag = ts.tpl_struct_origin;
                                        std::istringstream ss2(new_refs);
                                        std::string p2;
                                        while (std::getline(ss2, p2, ',')) {
                                            new_tag += "_";
                                            new_tag += p2;
                                        }
                                        ts.tag = arena_.strdup(new_tag.c_str());
                                    }
                                }
                                ts.is_const   |= save_const;
                                ts.is_volatile |= save_vol;
                                return ts;
                            }
                        }
                    }
                }
                // Dependent template-template parameter application:
                // template<template<typename...> class Op> using X = Op<int>;
                // We only need to consume the argument list and keep the
                // dependent name as a placeholder type for later stages.
                if (is_cpp_mode() && tname &&
                    is_template_scope_type_param(tname) &&
                    check(TokenKind::Less)) {
                    std::vector<TemplateArgParseResult> ignored_args;
                    if (parse_template_argument_list(&ignored_args)) {
                        ts.base = TB_TYPEDEF;
                        ts.tag = tname;
                        ts.array_size = -1;
                        ts.array_rank = 0;
                        return ts;
                    }
                }
                // Template struct instantiation: Pair<int> or Array<int, 4> in type context.
                if (is_cpp_mode() && ts.base == TB_STRUCT && ts.tag &&
                    find_template_struct_primary(ts.tag) && check(TokenKind::Less)) {
                    std::string tpl_name = ts.tag;
                    const Node* primary_tpl = find_template_struct_primary(tpl_name);
                    std::vector<ParsedTemplateArg> actual_args;
                    if (!parse_template_argument_list(&actual_args, primary_tpl)) return ts;
                    // Fill in deferred NTTP defaults before pattern selection
                    // so specialization matching has the complete arg list.
                    if (primary_tpl) {
                        // Build preliminary type bindings from explicit args
                        std::vector<std::pair<std::string, TypeSpec>> prelim_tb;
                        std::vector<std::pair<std::string, long long>> prelim_nb;
                        for (int pi = 0; pi < static_cast<int>(actual_args.size()) &&
                                         pi < primary_tpl->n_template_params; ++pi) {
                            const char* pn = primary_tpl->template_param_names[pi];
                            if (primary_tpl->template_param_is_nttp[pi]) {
                                if (actual_args[pi].is_value)
                                    prelim_nb.push_back({pn, actual_args[pi].value});
                            } else {
                                if (!actual_args[pi].is_value)
                                    prelim_tb.push_back({pn, actual_args[pi].type});
                            }
                        }
                        // Evaluate deferred defaults for missing args
                        int sz = static_cast<int>(actual_args.size());
                        while (sz < primary_tpl->n_template_params) {
                            if (!primary_tpl->template_param_has_default ||
                                !primary_tpl->template_param_has_default[sz]) break;
                            ParsedTemplateArg da;
                            da.is_value = primary_tpl->template_param_is_nttp[sz];
                            if (da.is_value && primary_tpl->template_param_default_values[sz] == LLONG_MIN) {
                                long long ev = 0;
                                if (eval_deferred_nttp_default(tpl_name, sz, prelim_tb, prelim_nb, &ev)) {
                                    da.value = ev;
                                    actual_args.push_back(da);
                                } else if (primary_tpl->template_param_default_exprs &&
                                           primary_tpl->template_param_default_exprs[sz]) {
                                    std::vector<Token> expr_toks = lex_template_expr_text(
                                        primary_tpl->template_param_default_exprs[sz],
                                        source_profile_);
                                    if (eval_deferred_nttp_expr_tokens(tpl_name, expr_toks,
                                                                       prelim_tb, prelim_nb, &ev)) {
                                        da.value = ev;
                                        actual_args.push_back(da);
                                    } else {
                                        std::string tagged = "$expr:";
                                        tagged += primary_tpl->template_param_default_exprs[sz];
                                        da.nttp_name = arena_.strdup(tagged.c_str());
                                        da.value = 0;
                                        actual_args.push_back(da);
                                    }
                                } else {
                                    if (primary_tpl->template_param_default_exprs &&
                                        primary_tpl->template_param_default_exprs[sz]) {
                                        std::string tagged = "$expr:";
                                        tagged += primary_tpl->template_param_default_exprs[sz];
                                        da.nttp_name = arena_.strdup(tagged.c_str());
                                        da.value = 0;
                                        actual_args.push_back(da);
                                    } else {
                                        break;
                                    }
                                }
                            } else if (da.is_value) {
                                da.value = primary_tpl->template_param_default_values[sz];
                                actual_args.push_back(da);
                            } else {
                                da.type = primary_tpl->template_param_default_types[sz];
                                actual_args.push_back(da);
                            }
                            ++sz;
                        }
                    }
                    std::vector<std::pair<std::string, TypeSpec>> type_bindings;
                    std::vector<std::pair<std::string, long long>> nttp_bindings;
                    const auto* specialization_patterns =
                        find_template_struct_specializations(primary_tpl);
                    const Node* tpl_def = select_template_struct_pattern(
                        actual_args, primary_tpl, specialization_patterns,
                        typedef_types_, &type_bindings, &nttp_bindings);
                    if (!tpl_def) return ts;
                    std::vector<ParsedTemplateArg> concrete_args = actual_args;
                    while (primary_tpl && static_cast<int>(concrete_args.size()) < primary_tpl->n_template_params) {
                        int i = static_cast<int>(concrete_args.size());
                        if (!primary_tpl->template_param_has_default[i]) break;
                        ParsedTemplateArg arg;
                        arg.is_value = primary_tpl->template_param_is_nttp[i];
                        if (arg.is_value) {
                            if (primary_tpl->template_param_default_values[i] == LLONG_MIN) {
                                // Deferred NTTP default — evaluate with current bindings
                                long long eval_val = 0;
                                if (eval_deferred_nttp_default(tpl_name, i,
                                        type_bindings, nttp_bindings, &eval_val)) {
                                    arg.value = eval_val;
                                } else if (primary_tpl->template_param_default_exprs &&
                                           primary_tpl->template_param_default_exprs[i]) {
                                    std::vector<Token> expr_toks = lex_template_expr_text(
                                        primary_tpl->template_param_default_exprs[i],
                                        source_profile_);
                                    if (eval_deferred_nttp_expr_tokens(tpl_name, expr_toks,
                                                                       type_bindings, nttp_bindings,
                                                                       &eval_val)) {
                                        arg.value = eval_val;
                                    } else {
                                        std::string tagged = "$expr:";
                                        tagged += primary_tpl->template_param_default_exprs[i];
                                        arg.nttp_name = arena_.strdup(tagged.c_str());
                                        arg.value = 0;
                                    }
                                } else {
                                    if (primary_tpl->template_param_default_exprs &&
                                        primary_tpl->template_param_default_exprs[i]) {
                                        std::string tagged = "$expr:";
                                        tagged += primary_tpl->template_param_default_exprs[i];
                                        arg.nttp_name = arena_.strdup(tagged.c_str());
                                        arg.value = 0;
                                    } else {
                                        break;  // cannot evaluate
                                    }
                                }
                            } else {
                                arg.value = primary_tpl->template_param_default_values[i];
                            }
                        } else {
                            arg.type = primary_tpl->template_param_default_types[i];
                        }
                        concrete_args.push_back(arg);
                    }
                    const std::string instance_key =
                        make_template_struct_instance_key(primary_tpl, concrete_args);

                    // Build mangled name from the concrete family arguments.
                    const std::string family_name =
                        tpl_def->template_origin_name ? tpl_def->template_origin_name : tpl_name;
                    std::string mangled = family_name;
                    for (int pi = 0; primary_tpl && pi < primary_tpl->n_template_params; ++pi) {
                        mangled += "_";
                        mangled += primary_tpl->template_param_names[pi];
                        mangled += "_";
                        if (pi < static_cast<int>(concrete_args.size()) && concrete_args[pi].is_value) {
                            mangled += std::to_string(concrete_args[pi].value);
                        } else {
                            if (pi < static_cast<int>(concrete_args.size()) && !concrete_args[pi].is_value)
                                append_type_mangled_suffix(mangled, concrete_args[pi].type);
                            else
                                mangled += primary_tpl->template_param_is_nttp[pi] ? "0" : "T";
                        }
                    }
                    // Check if any type arg is an unresolved template param
                    // or a pending template struct (e.g. Pair<T> inside Box<Pair<T>>).
                    // If so, defer instantiation to HIR template function lowering.
                    bool has_unresolved_type_arg = false;
                    for (const auto& [pn, pts] : type_bindings) {
                        if (pts.base == TB_TYPEDEF || pts.tpl_struct_origin) {
                            has_unresolved_type_arg = true;
                            break;
                        }
                    }
                    bool has_deferred_nttp_arg = false;
                    for (const auto& arg : concrete_args) {
                        if (arg.is_value && arg.nttp_name && arg.nttp_name[0]) {
                            has_deferred_nttp_arg = true;
                            break;
                        }
                    }
                    // Also check: if concrete_args is incomplete (fewer than
                    // primary_tpl->n_template_params) because a deferred NTTP
                    // default could not be evaluated, treat as deferred.
                    bool has_incomplete_nttp_default = false;
                    if (primary_tpl &&
                        static_cast<int>(concrete_args.size()) < primary_tpl->n_template_params) {
                        has_incomplete_nttp_default = true;
                    }

                    if (has_unresolved_type_arg || has_deferred_nttp_arg || has_incomplete_nttp_default) {
                        // Build arg_refs: comma-separated list of arg values
                        // in template param order. Type args use the TypeSpec tag
                        // (e.g., "T"), NTTP args use the numeric value.
                        std::string arg_refs;
                        int ati = 0, ani = 0;
                        for (int pi = 0; pi < tpl_def->n_template_params; ++pi) {
                            if (tpl_def->template_param_is_nttp[pi]) {
                                const bool have_arg = pi < static_cast<int>(concrete_args.size());
                                const ParsedTemplateArg* carg =
                                    have_arg ? &concrete_args[pi] : nullptr;
                                bool have_value = ani < (int)nttp_bindings.size();
                                long long bound_value = have_value ? nttp_bindings[ani].second : 0;
                                const bool missing_default =
                                    (!have_arg &&
                                     (!have_value ||
                                      (bound_value == LLONG_MIN &&
                                       tpl_def->template_param_has_default &&
                                       tpl_def->template_param_has_default[pi])));
                                if (missing_default) break;
                                if (!arg_refs.empty()) arg_refs += ",";
                                if (carg && carg->nttp_name && carg->nttp_name[0]) {
                                    arg_refs += carg->nttp_name;
                                } else {
                                    arg_refs += std::to_string(bound_value);
                                }
                                ++ani;
                            } else {
                                if (ati < (int)type_bindings.size()) {
                                    if (!arg_refs.empty()) arg_refs += ",";
                                    const TypeSpec& ats = type_bindings[ati++].second;
                                    if (ats.tpl_struct_origin) {
                                        // Nested pending template struct: encode as @origin:args
                                        arg_refs += "@";
                                        arg_refs += ats.tpl_struct_origin;
                                        arg_refs += ":";
                                        arg_refs += ats.tpl_struct_arg_refs ? ats.tpl_struct_arg_refs : "";
                                    } else if (ats.tag) {
                                        arg_refs += ats.tag;
                                    } else {
                                        // Builtin type (bool, int, etc.) — encode as mangled name.
                                        std::string type_name;
                                        append_type_mangled_suffix(type_name, ats);
                                        arg_refs += type_name.empty() ? "?" : type_name;
                                    }
                                } else {
                                    if (!arg_refs.empty()) arg_refs += ",";
                                    arg_refs += "?";
                                }
                            }
                        }
                        ts.tpl_struct_origin = arena_.strdup(family_name.c_str());
                        ts.tpl_struct_arg_refs = arena_.strdup(arg_refs.c_str());
                        ts.tag = arena_.strdup(mangled.c_str());
                        return ts;
                    }

                    // Instantiate if not already done
                    if (!instantiated_template_struct_keys_.count(instance_key)) {
                        instantiated_template_struct_keys_.insert(instance_key);
                        // Create a concrete NK_STRUCT_DEF with substituted field types
                        Node* inst = make_node(NK_STRUCT_DEF, tpl_def->line);
                        inst->name = arena_.strdup(mangled.c_str());
                        inst->template_origin_name = arena_.strdup(family_name.c_str());
                        inst->is_union = tpl_def->is_union;
                        inst->pack_align = tpl_def->pack_align;
                        inst->struct_align = tpl_def->struct_align;
                        inst->n_bases = tpl_def->n_bases;
                        if (inst->n_bases > 0) {
                            inst->base_types = arena_.alloc_array<TypeSpec>(inst->n_bases);
                            for (int bi = 0; bi < inst->n_bases; ++bi) {
                                inst->base_types[bi] = tpl_def->base_types[bi];
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (inst->base_types[bi].base == TB_TYPEDEF &&
                                        inst->base_types[bi].tag &&
                                        std::string(inst->base_types[bi].tag) == pname) {
                                        inst->base_types[bi].base = pts.base;
                                        inst->base_types[bi].tag = pts.tag;
                                        inst->base_types[bi].ptr_level += pts.ptr_level;
                                        inst->base_types[bi].is_const |= pts.is_const;
                                        inst->base_types[bi].is_volatile |= pts.is_volatile;
                                    }
                                }
                                // Resolve pending template base types: e.g. is_const<T> → is_const<const int>
                                if (inst->base_types[bi].tpl_struct_origin) {
                                    std::string origin = inst->base_types[bi].tpl_struct_origin;
                                    std::string arg_refs_str = inst->base_types[bi].tpl_struct_arg_refs
                                        ? inst->base_types[bi].tpl_struct_arg_refs : "";
                                    // Substitute template param names in arg_refs
                                    bool all_resolved = true;
                                    std::vector<std::string> new_arg_parts;
                                    if (!arg_refs_str.empty()) {
                                        std::istringstream ss(arg_refs_str);
                                        std::string part;
                                        while (std::getline(ss, part, ',')) {
                                            bool found = false;
                                            for (const auto& [pname2, pts2] : type_bindings) {
                                                if (part == pname2) {
                                                    // Substitute with concrete type mangled suffix
                                                    std::string sub;
                                                    append_type_mangled_suffix(sub, pts2);
                                                    new_arg_parts.push_back(sub);
                                                    found = true;
                                                    break;
                                                }
                                            }
                                            if (!found) {
                                                for (const auto& [npname2, nval2] : nttp_bindings) {
                                                    if (part == npname2) {
                                                        new_arg_parts.push_back(std::to_string(nval2));
                                                        found = true;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (!found) {
                                                if (part.rfind("$expr:", 0) == 0) {
                                                    // Substitute template param names inside $expr text.
                                                    std::string expr_text = part.substr(6);
                                                    for (const auto& [pname3, pts3] : type_bindings) {
                                                        std::string mangled_name;
                                                        append_type_mangled_suffix(mangled_name, pts3);
                                                        // Replace whole-word occurrences of pname3 with mangled_name.
                                                        size_t search_pos = 0;
                                                        while ((search_pos = expr_text.find(pname3, search_pos)) != std::string::npos) {
                                                            bool is_word_start = (search_pos == 0 ||
                                                                (!std::isalnum((unsigned char)expr_text[search_pos - 1]) &&
                                                                 expr_text[search_pos - 1] != '_'));
                                                            size_t end_pos = search_pos + pname3.size();
                                                            bool is_word_end = (end_pos >= expr_text.size() ||
                                                                (!std::isalnum((unsigned char)expr_text[end_pos]) &&
                                                                 expr_text[end_pos] != '_'));
                                                            if (is_word_start && is_word_end) {
                                                                expr_text.replace(search_pos, pname3.size(), mangled_name);
                                                                search_pos += mangled_name.size();
                                                            } else {
                                                                search_pos += pname3.size();
                                                            }
                                                        }
                                                    }
                                                    for (const auto& [npname3, nval3] : nttp_bindings) {
                                                        std::string val_str = std::to_string(nval3);
                                                        size_t search_pos = 0;
                                                        while ((search_pos = expr_text.find(npname3, search_pos)) != std::string::npos) {
                                                            bool is_word_start = (search_pos == 0 ||
                                                                (!std::isalnum((unsigned char)expr_text[search_pos - 1]) &&
                                                                 expr_text[search_pos - 1] != '_'));
                                                            size_t end_pos = search_pos + npname3.size();
                                                            bool is_word_end = (end_pos >= expr_text.size() ||
                                                                (!std::isalnum((unsigned char)expr_text[end_pos]) &&
                                                                 expr_text[end_pos] != '_'));
                                                            if (is_word_start && is_word_end) {
                                                                expr_text.replace(search_pos, npname3.size(), val_str);
                                                                search_pos += val_str.size();
                                                            } else {
                                                                search_pos += npname3.size();
                                                            }
                                                        }
                                                    }
                                                    Lexer expr_lexer(expr_text, lex_profile_from(source_profile_));
                                                    std::vector<Token> expr_toks = expr_lexer.scan_all();
                                                    if (!expr_toks.empty() &&
                                                        expr_toks.back().kind == TokenKind::EndOfFile) {
                                                        expr_toks.pop_back();
                                                    }
                                                    long long expr_value = 0;
                                                    if (!expr_toks.empty() &&
                                                        eval_deferred_nttp_expr_tokens(origin, expr_toks,
                                                                                       type_bindings, nttp_bindings,
                                                                                       &expr_value)) {
                                                        new_arg_parts.push_back(std::to_string(expr_value));
                                                    } else {
                                                        new_arg_parts.push_back("$expr:" + expr_text);
                                                        all_resolved = false;
                                                    }
                                                    continue;
                                                }
                                                new_arg_parts.push_back(part);
                                                // Check if this arg is still unresolved.
                                                for (const auto& [pn, _] : type_bindings) {
                                                    (void)_;
                                                    if (part == pn) { all_resolved = false; break; }
                                                }
                                                for (const auto& [pn, _] : nttp_bindings) {
                                                    (void)_;
                                                    if (part == pn) { all_resolved = false; break; }
                                                }
                                            }
                                        }
                                    }
                                    // Always update arg_refs with substituted parts so
                                    // the HIR resolver can see concrete type/NTTP names.
                                    if (!new_arg_parts.empty()) {
                                        std::string updated_refs;
                                        for (size_t pi = 0; pi < new_arg_parts.size(); ++pi) {
                                            if (pi > 0) updated_refs += ",";
                                            updated_refs += new_arg_parts[pi];
                                        }
                                        inst->base_types[bi].tpl_struct_arg_refs =
                                            arena_.strdup(updated_refs.c_str());
                                    }
                                    if (all_resolved && find_template_struct_primary(origin)) {
                                        // Build ParsedTemplateArg list from resolved parts
                                        // and fill in deferred NTTP defaults, then trigger
                                        // a proper template instantiation.
                                        const Node* base_primary = find_template_struct_primary(origin);
                                        std::vector<ParsedTemplateArg> base_args;
                                        for (int pi = 0; pi < (int)new_arg_parts.size() &&
                                             pi < base_primary->n_template_params; ++pi) {
                                            ParsedTemplateArg ba;
                                            if (base_primary->template_param_is_nttp[pi]) {
                                                ba.is_value = true;
                                                try { ba.value = std::stoll(new_arg_parts[pi]); }
                                                catch (...) { ba.value = 0; }
                                            } else {
                                                ba.is_value = false;
                                                auto tit = typedef_types_.find(new_arg_parts[pi]);
                                                if (tit != typedef_types_.end()) {
                                                    ba.type = tit->second;
                                                } else if (parse_mangled_type_suffix(new_arg_parts[pi], &ba.type)) {
                                                    // Recognized mangled builtin suffix (e.g. "uint" → TB_UINT)
                                                } else if (parse_builtin_typespec_text(new_arg_parts[pi], &ba.type)) {
                                                    // Recognized C type name (e.g. "unsigned int")
                                                } else {
                                                    // Look up as mangled struct tag
                                                    ba.type = {};
                                                    ba.type.array_size = -1;
                                                    ba.type.inner_rank = -1;
                                                    ba.type.base = TB_STRUCT;
                                                    ba.type.tag = arena_.strdup(new_arg_parts[pi].c_str());
                                                }
                                            }
                                            base_args.push_back(ba);
                                        }
                                        // Fill in deferred NTTP defaults
                                        {
                                            std::vector<std::pair<std::string, TypeSpec>> base_prelim_tb;
                                            std::vector<std::pair<std::string, long long>> base_prelim_nb;
                                            for (int pi = 0; pi < (int)base_args.size() &&
                                                 pi < base_primary->n_template_params; ++pi) {
                                                const char* pn = base_primary->template_param_names[pi];
                                                if (base_primary->template_param_is_nttp[pi]) {
                                                    if (base_args[pi].is_value)
                                                        base_prelim_nb.push_back({pn, base_args[pi].value});
                                                } else {
                                                    if (!base_args[pi].is_value)
                                                        base_prelim_tb.push_back({pn, base_args[pi].type});
                                                }
                                            }
                                            int bsz = (int)base_args.size();
                                            while (bsz < base_primary->n_template_params) {
                                                if (!base_primary->template_param_has_default ||
                                                    !base_primary->template_param_has_default[bsz]) break;
                                                ParsedTemplateArg da;
                                                da.is_value = base_primary->template_param_is_nttp[bsz];
                                                if (da.is_value && base_primary->template_param_default_values[bsz] == LLONG_MIN) {
                                                    long long ev = 0;
                                                    if (eval_deferred_nttp_default(origin, bsz,
                                                            base_prelim_tb, base_prelim_nb, &ev)) {
                                                        da.value = ev;
                                                        base_args.push_back(da);
                                                    } else if (base_primary->template_param_default_exprs &&
                                                               base_primary->template_param_default_exprs[bsz]) {
                                                        std::vector<Token> expr_toks = lex_template_expr_text(
                                                            base_primary->template_param_default_exprs[bsz],
                                                            source_profile_);
                                                        if (eval_deferred_nttp_expr_tokens(origin, expr_toks,
                                                                                           base_prelim_tb, base_prelim_nb,
                                                                                           &ev)) {
                                                            da.value = ev;
                                                            base_args.push_back(da);
                                                        } else {
                                                            std::string tagged = "$expr:";
                                                            tagged += base_primary->template_param_default_exprs[bsz];
                                                            da.nttp_name = arena_.strdup(tagged.c_str());
                                                            da.value = 0;
                                                            base_args.push_back(da);
                                                        }
                                                    } else break;
                                                } else if (da.is_value) {
                                                    da.value = base_primary->template_param_default_values[bsz];
                                                    base_args.push_back(da);
                                                } else {
                                                    da.type = base_primary->template_param_default_types[bsz];
                                                    base_args.push_back(da);
                                                }
                                                ++bsz;
                                            }
                                        }
                                        // Select specialization and build mangled name
                                        std::vector<std::pair<std::string, TypeSpec>> base_tb;
                                        std::vector<std::pair<std::string, long long>> base_nb;
                                        const auto* base_specializations =
                                            find_template_struct_specializations(base_primary);
                                        const Node* base_sel = select_template_struct_pattern(
                                            base_args, base_primary, base_specializations,
                                            typedef_types_, &base_tb, &base_nb);
                                        if (!base_sel) base_sel = base_primary;
                                        const std::string base_family =
                                            base_sel->template_origin_name ? base_sel->template_origin_name : origin;
                                        std::string base_mangled = base_family;
                                        for (int pi = 0; pi < base_primary->n_template_params; ++pi) {
                                            base_mangled += "_";
                                            base_mangled += base_primary->template_param_names[pi];
                                            base_mangled += "_";
                                            if (pi < (int)base_args.size() && base_args[pi].is_value) {
                                                base_mangled += std::to_string(base_args[pi].value);
                                            } else if (pi < (int)base_args.size() && !base_args[pi].is_value) {
                                                append_type_mangled_suffix(base_mangled, base_args[pi].type);
                                            }
                                        }
                                        const std::string base_instance_key =
                                            make_template_struct_instance_key(base_primary, base_args);
                                        // Trigger instantiation if needed
                                        if (!instantiated_template_struct_keys_.count(base_instance_key) &&
                                            !struct_tag_def_map_.count(base_mangled)) {
                                            // Inject tokens to trigger parse_base_type instantiation
                                            // by building: origin < arg1, arg2, ... >
                                            std::vector<Token> inject_toks;
                                            Token t; t.line = tpl_def->line; t.column = 0;
                                            t.kind = TokenKind::Identifier;
                                            t.lexeme = origin;
                                            inject_toks.push_back(t);
                                            t.kind = TokenKind::Less; t.lexeme = "<";
                                            inject_toks.push_back(t);
                                            for (int ai = 0; ai < (int)base_args.size(); ++ai) {
                                                if (ai > 0) {
                                                    t.kind = TokenKind::Comma; t.lexeme = ",";
                                                    inject_toks.push_back(t);
                                                }
                                                if (base_args[ai].is_value) {
                                                    if (base_args[ai].value == 0) {
                                                        t.kind = TokenKind::KwFalse; t.lexeme = "false";
                                                    } else if (base_args[ai].value == 1) {
                                                        t.kind = TokenKind::KwTrue; t.lexeme = "true";
                                                    } else {
                                                        t.kind = TokenKind::IntLit;
                                                        t.lexeme = std::to_string(base_args[ai].value);
                                                    }
                                                    inject_toks.push_back(t);
                                                } else {
                                                    const TypeSpec& ats = base_args[ai].type;
                                                    if (ats.tag) {
                                                        t.kind = TokenKind::Identifier; t.lexeme = ats.tag;
                                                    } else {
                                                        // Emit proper keyword tokens for builtin types.
                                                        auto emit_kw = [&](TokenKind k, const char* lex) {
                                                            Token kt; kt.line = tpl_def->line; kt.column = 0;
                                                            kt.kind = k; kt.lexeme = lex;
                                                            inject_toks.push_back(kt);
                                                        };
                                                        bool emitted = true;
                                                        switch (ats.base) {
                                                            case TB_VOID: emit_kw(TokenKind::KwVoid, "void"); break;
                                                            case TB_BOOL: emit_kw(TokenKind::KwBool, "bool"); break;
                                                            case TB_CHAR: emit_kw(TokenKind::KwChar, "char"); break;
                                                            case TB_SCHAR: emit_kw(TokenKind::KwSigned, "signed"); emit_kw(TokenKind::KwChar, "char"); break;
                                                            case TB_UCHAR: emit_kw(TokenKind::KwUnsigned, "unsigned"); emit_kw(TokenKind::KwChar, "char"); break;
                                                            case TB_SHORT: emit_kw(TokenKind::KwShort, "short"); break;
                                                            case TB_USHORT: emit_kw(TokenKind::KwUnsigned, "unsigned"); emit_kw(TokenKind::KwShort, "short"); break;
                                                            case TB_INT: emit_kw(TokenKind::KwInt, "int"); break;
                                                            case TB_UINT: emit_kw(TokenKind::KwUnsigned, "unsigned"); emit_kw(TokenKind::KwInt, "int"); break;
                                                            case TB_LONG: emit_kw(TokenKind::KwLong, "long"); break;
                                                            case TB_ULONG: emit_kw(TokenKind::KwUnsigned, "unsigned"); emit_kw(TokenKind::KwLong, "long"); break;
                                                            case TB_LONGLONG: emit_kw(TokenKind::KwLong, "long"); emit_kw(TokenKind::KwLong, "long"); break;
                                                            case TB_ULONGLONG: emit_kw(TokenKind::KwUnsigned, "unsigned"); emit_kw(TokenKind::KwLong, "long"); emit_kw(TokenKind::KwLong, "long"); break;
                                                            case TB_FLOAT: emit_kw(TokenKind::KwFloat, "float"); break;
                                                            case TB_DOUBLE: emit_kw(TokenKind::KwDouble, "double"); break;
                                                            default: t.kind = TokenKind::Identifier; t.lexeme = "int"; emitted = false; break;
                                                        }
                                                        if (emitted) continue;  // tokens already pushed
                                                    }
                                                    inject_toks.push_back(t);
                                                }
                                            }
                                            t.kind = TokenKind::Greater; t.lexeme = ">";
                                            inject_toks.push_back(t);
                                            // Add a sentinel
                                            t.kind = TokenKind::Semi; t.lexeme = ";";
                                            inject_toks.push_back(t);

                                            // Save and inject tokens
                                            int saved_pos = pos_;
                                            auto saved_toks = std::move(tokens_);
                                            tokens_ = std::move(inject_toks);
                                            pos_ = 0;
                                            try {
                                                TypeSpec resolved_base = parse_base_type();
                                                inst->base_types[bi] = resolved_base;
                                            } catch (...) {}
                                            tokens_ = std::move(saved_toks);
                                            pos_ = saved_pos;
                                        } else if (struct_tag_def_map_.count(base_mangled)) {
                                            inst->base_types[bi] = TypeSpec{};
                                            inst->base_types[bi].array_size = -1;
                                            inst->base_types[bi].inner_rank = -1;
                                            inst->base_types[bi].base = TB_STRUCT;
                                            inst->base_types[bi].tag = arena_.strdup(base_mangled.c_str());
                                        }
                                    }
                                }
                                if (inst->base_types[bi].deferred_member_type_name &&
                                    inst->base_types[bi].tag &&
                                    inst->base_types[bi].tag[0]) {
                                    TypeSpec resolved_member{};
                                    if (lookup_struct_member_typedef_recursive(
                                            inst->base_types[bi].tag,
                                            inst->base_types[bi].deferred_member_type_name,
                                            &resolved_member)) {
                                        resolved_member.deferred_member_type_name = nullptr;
                                        inst->base_types[bi] = resolved_member;
                                    }
                                }
                            }
                        }
                        // Store template bindings so HIR can resolve pending types
                        // in method bodies and signatures.
                        {
                            int n = tpl_def->n_template_params;
                            inst->n_template_params = n;
                            inst->template_param_names = tpl_def->template_param_names;
                            inst->template_param_is_nttp = tpl_def->template_param_is_nttp;
                            inst->n_template_args = n;
                            inst->template_arg_types = arena_.alloc_array<TypeSpec>(n);
                            inst->template_arg_is_value = arena_.alloc_array<bool>(n);
                            inst->template_arg_values = arena_.alloc_array<long long>(n);
                            inst->template_arg_nttp_names = arena_.alloc_array<const char*>(n);
                            inst->template_arg_exprs = arena_.alloc_array<Node*>(n);
                            int tti = 0, nni = 0;
                            for (int pi = 0; pi < n; ++pi) {
                                if (tpl_def->template_param_is_nttp[pi]) {
                                    inst->template_arg_is_value[pi] = true;
                                    inst->template_arg_values[pi] =
                                        nni < (int)nttp_bindings.size() ? nttp_bindings[nni++].second : 0;
                                    inst->template_arg_nttp_names[pi] = nullptr;
                                    inst->template_arg_exprs[pi] = nullptr;
                                    inst->template_arg_types[pi] = {};
                                } else {
                                    inst->template_arg_is_value[pi] = false;
                                    inst->template_arg_types[pi] =
                                        tti < (int)type_bindings.size() ? type_bindings[tti++].second : TypeSpec{};
                                    inst->template_arg_values[pi] = 0;
                                    inst->template_arg_nttp_names[pi] = nullptr;
                                    inst->template_arg_exprs[pi] = nullptr;
                                }
                            }
                        }
                        // Clone member typedefs with type substitution and register
                        // them under the concrete instantiation scope so later
                        // `Template<Args>::member` lookup does not need to fall
                        // back to the template family name.
                        inst->n_member_typedefs = tpl_def->n_member_typedefs;
                        if (inst->n_member_typedefs > 0) {
                            inst->member_typedef_names =
                                arena_.alloc_array<const char*>(inst->n_member_typedefs);
                            inst->member_typedef_types =
                                arena_.alloc_array<TypeSpec>(inst->n_member_typedefs);
                            for (int ti = 0; ti < inst->n_member_typedefs; ++ti) {
                                inst->member_typedef_names[ti] =
                                    tpl_def->member_typedef_names[ti];
                                TypeSpec member_ts = tpl_def->member_typedef_types[ti];
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (member_ts.base == TB_TYPEDEF && member_ts.tag &&
                                        std::string(member_ts.tag) == pname) {
                                        member_ts.base = pts.base;
                                        member_ts.tag = pts.tag;
                                        member_ts.ptr_level += pts.ptr_level;
                                        member_ts.is_const |= pts.is_const;
                                        member_ts.is_volatile |= pts.is_volatile;
                                    }
                                }
                                if (member_ts.array_size_expr &&
                                    member_ts.array_size_expr->kind == NK_VAR &&
                                    member_ts.array_size_expr->name) {
                                    for (const auto& [npname, nval] : nttp_bindings) {
                                        if (std::string(member_ts.array_size_expr->name) == npname) {
                                            if (member_ts.array_rank > 0) {
                                                member_ts.array_dims[0] = nval;
                                                member_ts.array_size = nval;
                                            }
                                            member_ts.array_size_expr = nullptr;
                                            break;
                                        }
                                    }
                                }
                                inst->member_typedef_types[ti] = member_ts;
                                if (inst->member_typedef_names[ti] &&
                                    inst->member_typedef_names[ti][0]) {
                                    std::string scoped =
                                        mangled + "::" + inst->member_typedef_names[ti];
                                    struct_typedefs_[scoped] = member_ts;
                                    typedefs_.insert(scoped);
                                    typedef_types_[scoped] = member_ts;
                                }
                            }
                        }
                        // Clone fields with type and NTTP substitution
                        int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
                        inst->n_fields = num_fields;
                        inst->fields = arena_.alloc_array<Node*>(num_fields);
                        for (int fi = 0; fi < num_fields; ++fi) {
                            const Node* orig_f = tpl_def->fields[fi];
                            Node* new_f = make_node(NK_DECL, orig_f->line);
                            new_f->name = orig_f->name;
                            new_f->type = orig_f->type;
                            new_f->ival = orig_f->ival;
                            new_f->is_anon_field = orig_f->is_anon_field;
                            new_f->is_static = orig_f->is_static;
                            new_f->is_constexpr = orig_f->is_constexpr;
                            new_f->init = orig_f->init;
                            // Substitute template type parameters in field type
                            for (const auto& [pname, pts] : type_bindings) {
                                if (new_f->type.base == TB_TYPEDEF && new_f->type.tag &&
                                    std::string(new_f->type.tag) == pname) {
                                    new_f->type.base = pts.base;
                                    new_f->type.tag = pts.tag;
                                    new_f->type.ptr_level += pts.ptr_level;
                                    new_f->type.is_const |= pts.is_const;
                                    new_f->type.is_volatile |= pts.is_volatile;
                                }
                            }
                            // Substitute NTTP values in array dimensions
                            if (new_f->type.array_size_expr) {
                                Node* ase = new_f->type.array_size_expr;
                                if (ase->kind == NK_VAR && ase->name) {
                                    for (const auto& [npname, nval] : nttp_bindings) {
                                        if (std::string(ase->name) == npname) {
                                            // Replace the first array dim with the NTTP value
                                            if (new_f->type.array_rank > 0) {
                                                new_f->type.array_dims[0] = nval;
                                                new_f->type.array_size = nval;
                                            }
                                            new_f->type.array_size_expr = nullptr;
                                            break;
                                        }
                                    }
                                }
                            }
                            inst->fields[fi] = new_f;
                        }
                        // Clone methods with type substitution
                        int num_methods = tpl_def->n_children > 0 ? tpl_def->n_children : 0;
                        inst->n_children = num_methods;
                        if (num_methods > 0) {
                            inst->children = arena_.alloc_array<Node*>(num_methods);
                            for (int mi = 0; mi < num_methods; ++mi) {
                                const Node* orig_m = tpl_def->children[mi];
                                if (!orig_m || orig_m->kind != NK_FUNCTION) {
                                    inst->children[mi] = nullptr;
                                    continue;
                                }
                                Node* new_m = make_node(NK_FUNCTION, orig_m->line);
                                new_m->name = orig_m->name;
                                new_m->variadic = orig_m->variadic;
                                new_m->is_constexpr = orig_m->is_constexpr;
                                new_m->is_consteval = orig_m->is_consteval;
                                // Substitute template type params in return type
                                new_m->type = orig_m->type;
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (new_m->type.base == TB_TYPEDEF && new_m->type.tag &&
                                        std::string(new_m->type.tag) == pname) {
                                        new_m->type.base = pts.base;
                                        new_m->type.tag = pts.tag;
                                        new_m->type.ptr_level += pts.ptr_level;
                                    }
                                }
                                // Clone params with type substitution
                                new_m->n_params = orig_m->n_params;
                                if (new_m->n_params > 0) {
                                    new_m->params = arena_.alloc_array<Node*>(new_m->n_params);
                                    for (int pi = 0; pi < new_m->n_params; ++pi) {
                                        const Node* orig_p = orig_m->params[pi];
                                        Node* new_p = make_node(NK_DECL, orig_p->line);
                                        new_p->name = orig_p->name;
                                        new_p->type = orig_p->type;
                                        for (const auto& [pname, pts] : type_bindings) {
                                            if (new_p->type.base == TB_TYPEDEF && new_p->type.tag &&
                                                std::string(new_p->type.tag) == pname) {
                                                new_p->type.base = pts.base;
                                                new_p->type.tag = pts.tag;
                                                new_p->type.ptr_level += pts.ptr_level;
                                            }
                                        }
                                        new_m->params[pi] = new_p;
                                    }
                                }
                                // Body is shared (not deep-cloned) — HIR lowering
                                // handles field access via this->field resolution.
                                new_m->body = orig_m->body;
                                inst->children[mi] = new_m;
                            }
                        }
                        struct_defs_.push_back(inst);
                        struct_tag_def_map_[mangled] = inst;
                        defined_struct_tags_.insert(mangled);
                    }
                    ts.tag = arena_.strdup(mangled.c_str());
                }
                // C++ template using alias: typedef resolved but the resolved
                // type is not a primary template struct (e.g. using A = S<T>;
                // then A<int>).  Skip the <...> since the typedef already
                // encodes the target type.
                if (is_cpp_mode() && check(TokenKind::Less)) {
                    int depth = 1;
                    int paren_depth = 0;
                    consume();  // <
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) { ++paren_depth; consume(); continue; }
                        if (check(TokenKind::RParen)) { if (paren_depth > 0) --paren_depth; consume(); continue; }
                        // Only count <> as template brackets when not inside parens
                        if (paren_depth == 0 && check(TokenKind::Less)) ++depth;
                        else if (paren_depth == 0 && check_template_close()) {
                            --depth;
                            if (depth > 0) { match_template_close(); continue; }
                            break;
                        }
                        consume();
                    }
                    if (check_template_close()) match_template_close();
                }
                // Handle TemplateStruct<Args>::member suffix (e.g. bool_constant<true>::type).
                // Resolve the member as a typedef within the instantiated struct.
                if (is_cpp_mode() && check(TokenKind::ColonColon) &&
                    pos_ + 1 < static_cast<int>(tokens_.size()) &&
                    tokens_[pos_ + 1].kind == TokenKind::Identifier) {
                    std::string member = tokens_[pos_ + 1].lexeme;
                    TypeSpec resolved{};
                    if (lookup_struct_member_typedef_recursive(ts.tag, member, &resolved)) {
                        consume(); // ::
                        consume(); // member
                        // The typedef resolves to a type — for `typedef bool_constant type;` inside
                        // bool_constant<true>, `type` refers back to bool_constant<true> itself.
                        // The resolved typedef's tag might be the template name, not the instantiation.
                        // If the resolved type is the same template origin, use the instantiation tag.
                        if (resolved.base == TB_STRUCT && resolved.tag) {
                            auto inst_it = struct_tag_def_map_.find(ts.tag);
                            if (inst_it != struct_tag_def_map_.end() && inst_it->second &&
                                inst_it->second->template_origin_name &&
                                std::string(resolved.tag) == inst_it->second->template_origin_name) {
                                resolved.tag = ts.tag;
                            }
                        }
                        bool save_const = ts.is_const, save_vol = ts.is_volatile;
                        ts = resolved;
                        ts.is_const |= save_const;
                        ts.is_volatile |= save_vol;
                    } else if (ts.tpl_struct_origin || (ts.tag && ts.tag[0])) {
                        consume(); // ::
                        consume(); // member
                        ts.deferred_member_type_name = arena_.strdup(member.c_str());
                    }
                }
                return ts;
            }
        }
        ts.base = TB_TYPEDEF;
        // tag already set above
        // In C++ mode, skip unresolved template arguments (e.g. reverse_iterator<Iterator1>)
        if (is_cpp_mode() && check(TokenKind::Less)) {
            int depth = 1;
            consume();  // <
            while (!at_end() && depth > 0) {
                if (check(TokenKind::Less)) ++depth;
                else if (check_template_close()) {
                    --depth;
                    if (depth > 0) { match_template_close(); continue; }
                    break;
                }
                consume();
            }
            if (check_template_close()) match_template_close();
        }
    } else if (has_void) {
        ts.base = TB_VOID;
    } else if (has_bool) {
        ts.base = TB_BOOL;
    } else if (has_float) {
        ts.base = has_complex ? TB_COMPLEX_FLOAT : TB_FLOAT;
    } else if (has_double) {
        if (has_complex) ts.base = long_count > 0 ? TB_COMPLEX_LONGDOUBLE : TB_COMPLEX_DOUBLE;
        else ts.base = long_count > 0 ? TB_LONGDOUBLE : TB_DOUBLE;
    } else if (has_char) {
        if (has_complex)
            ts.base = has_unsigned ? TB_COMPLEX_UCHAR : TB_COMPLEX_SCHAR;
        else
            ts.base = has_unsigned ? TB_UCHAR : (has_signed ? TB_SCHAR : TB_CHAR);
    } else if (has_short) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_USHORT : TB_COMPLEX_SHORT)
                              : (has_unsigned ? TB_USHORT : TB_SHORT);
    } else if (long_count >= 2) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONGLONG : TB_COMPLEX_LONGLONG)
                              : (has_unsigned ? TB_ULONGLONG : TB_LONGLONG);
    } else if (long_count == 1) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONG : TB_COMPLEX_LONG)
                              : (has_unsigned ? TB_ULONG : TB_LONG);
    } else if (has_complex && !has_int_kw && !has_signed && !has_unsigned) {
        ts.base = TB_COMPLEX_DOUBLE;  // __complex__ with no type = double _Complex
    } else {
        // plain int (possibly unsigned)
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_UINT : TB_COMPLEX_INT)
                              : (has_unsigned ? TB_UINT : TB_INT);
    }

    finalize_vector_type(ts);
    return ts;
}

// Parse declarator: pointer stars, name, array/function suffixes.
// Modifies ts in place.  Sets *out_name to the declared name (or nullptr).
// Handles abstract declarators (no name) when out_name is allowed to be null.
void Parser::parse_declarator(TypeSpec& ts, const char** out_name,
                              Node*** out_fn_ptr_params,
                              int* out_n_fn_ptr_params,
                              bool* out_fn_ptr_variadic,
                              bool* out_is_parameter_pack,
                              Node*** out_ret_fn_ptr_params,
                              int* out_n_ret_fn_ptr_params,
                              bool* out_ret_fn_ptr_variadic) {
    if (out_name) *out_name = nullptr;
    if (out_fn_ptr_params) *out_fn_ptr_params = nullptr;
    if (out_n_fn_ptr_params) *out_n_fn_ptr_params = 0;
    if (out_fn_ptr_variadic) *out_fn_ptr_variadic = false;
    if (out_is_parameter_pack) *out_is_parameter_pack = false;
    if (out_ret_fn_ptr_params) *out_ret_fn_ptr_params = nullptr;
    if (out_n_ret_fn_ptr_params) *out_n_ret_fn_ptr_params = 0;
    if (out_ret_fn_ptr_variadic) *out_ret_fn_ptr_variadic = false;

    parse_declarator_prefix(ts, out_is_parameter_pack);

    // Check for parenthesised declarator: (*name) or (ATTR *name) — function pointer
    if (is_parenthesized_pointer_declarator_start()) {
        parse_parenthesized_pointer_declarator(
            ts, out_name,
            out_fn_ptr_params, out_n_fn_ptr_params, out_fn_ptr_variadic,
            out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
            out_ret_fn_ptr_variadic);
        return;
    }

    parse_non_parenthesized_declarator(ts, out_name);
    parse_plain_function_declarator_suffix(
        ts, /*decay_to_function_pointer=*/false);
}

TypeSpec Parser::parse_type_name() {
    TypeSpec ts = parse_base_type();
    const char* ignored = nullptr;
    parse_declarator(ts, &ignored);
    return ts;
}

// ── struct / union parsing ───────────────────────────────────────────────────

bool Parser::try_parse_record_access_label() {
    if (!(is_cpp_mode() && check(TokenKind::Identifier) &&
          (cur().lexeme == "public" || cur().lexeme == "private" ||
           cur().lexeme == "protected") &&
          pos_ + 1 < static_cast<int>(tokens_.size()) &&
          tokens_[pos_ + 1].kind == TokenKind::Colon)) {
        return false;
    }

    consume();
    consume();
    return true;
}

bool Parser::try_skip_record_friend_member() {
    if (!(is_cpp_mode() && check(TokenKind::Identifier) &&
          cur().lexeme == "friend")) {
        return false;
    }

    consume();
    int angle_depth = 0;
    int paren_depth = 0;
    while (!at_end()) {
        if (check(TokenKind::Less) && paren_depth == 0) {
            ++angle_depth;
            consume();
            continue;
        }
        if (check_template_close() && paren_depth == 0 && angle_depth > 0) {
            --angle_depth;
            match_template_close();
            continue;
        }
        if (check(TokenKind::LParen)) {
            ++paren_depth;
            consume();
            continue;
        }
        if (check(TokenKind::RParen) && paren_depth > 0) {
            --paren_depth;
            consume();
            continue;
        }
        if (check(TokenKind::LBrace) && angle_depth == 0 && paren_depth == 0) {
            skip_brace_group();
            break;
        }
        if (check(TokenKind::Semi) && angle_depth == 0 && paren_depth == 0) {
            consume();
            break;
        }
        if (check(TokenKind::RBrace) && angle_depth == 0 && paren_depth == 0) {
            break;
        }
        consume();
    }
    if (check(TokenKind::Assign)) {
        consume();
        if (check(TokenKind::KwDelete) || check(TokenKind::KwDefault))
            consume();
        match(TokenKind::Semi);
    }
    return true;
}

bool Parser::try_skip_record_static_assert_member() {
    if (!check(TokenKind::KwStaticAssert))
        return false;

    consume();
    if (check(TokenKind::LParen))
        skip_paren_group();
    match(TokenKind::Semi);
    return true;
}

bool Parser::recover_record_member_parse_error(int member_start_pos) {
    if (!is_cpp_mode())
        return false;

    int brace_depth = 0;
    while (!at_end()) {
        if (check(TokenKind::LBrace)) {
            ++brace_depth;
            consume();
        } else if (check(TokenKind::RBrace)) {
            if (brace_depth > 0) {
                --brace_depth;
                consume();
            } else {
                break;
            }
        } else if (check(TokenKind::Semi) && brace_depth == 0) {
            consume();
            break;
        } else {
            consume();
        }
    }

    if (pos_ == member_start_pos && !at_end()) consume();
    return true;
}

void Parser::parse_record_template_member_prelude(
    std::vector<std::string>* injected_type_params,
    bool* pushed_template_scope) {
    if (pushed_template_scope) *pushed_template_scope = false;
    if (!(is_cpp_mode() && check(TokenKind::KwTemplate)))
        return;

    consume();
    expect(TokenKind::Less);
    while (!at_end() && !check_template_close()) {
        if ((check(TokenKind::Identifier) && cur().lexeme == "typename") ||
            check(TokenKind::KwClass)) {
            consume();
            if (check(TokenKind::Ellipsis)) consume();
            if (check(TokenKind::Identifier)) {
                std::string pname = cur().lexeme;
                consume();
                typedefs_.insert(pname);
                TypeSpec param_ts{};
                param_ts.array_size = -1;
                param_ts.inner_rank = -1;
                param_ts.base = TB_TYPEDEF;
                param_ts.tag = arena_.strdup(pname.c_str());
                typedef_types_[pname] = param_ts;
                injected_type_params->push_back(std::move(pname));
            }
            if (check(TokenKind::Assign)) {
                consume();
                int saved_pos = pos_;
                bool parsed_default_type = false;
                try {
                    TypeSpec ignored_default = parse_type_name();
                    (void)ignored_default;
                    parsed_default_type = true;
                } catch (...) {
                    pos_ = saved_pos;
                }
                if (!parsed_default_type) {
                    int depth = 0, paren_depth = 0;
                    while (!at_end()) {
                        if (check(TokenKind::Less) || check(TokenKind::LParen)) {
                            if (check(TokenKind::LParen)) ++paren_depth;
                            else ++depth;
                        } else if (check(TokenKind::RParen)) {
                            if (paren_depth > 0) --paren_depth;
                        } else if (check(TokenKind::GreaterGreater)) {
                            if (paren_depth == 0 && depth <= 0) break;
                            if (paren_depth == 0 && depth == 1) {
                                parse_greater_than_in_template_list(false);
                                break;
                            }
                            if (depth >= 2) depth -= 2;
                            else if (depth == 1) --depth;
                            consume();
                            continue;
                        } else if (check(TokenKind::Greater)) {
                            if (paren_depth == 0 && depth == 0) break;
                            if (depth > 0) --depth;
                        } else if (check(TokenKind::Comma) && depth == 0 &&
                                   paren_depth == 0) {
                            break;
                        }
                        consume();
                    }
                }
            }
        } else if (is_type_kw(cur().kind) ||
                   (check(TokenKind::Identifier) &&
                    (is_typedef_name(cur().lexeme) ||
                     typedef_types_.count(
                         resolve_visible_type_name(cur().lexeme)) > 0))) {
            if (is_type_kw(cur().kind)) {
                while (!at_end() && is_type_kw(cur().kind)) consume();
            } else {
                consume();
            }
            while (check(TokenKind::Star) || is_qualifier(cur().kind)) consume();
            if (check(TokenKind::Ellipsis)) consume();
            if (check(TokenKind::Identifier)) consume();
            if (check(TokenKind::Assign)) {
                consume();
                int depth = 0, paren_depth = 0;
                while (!at_end()) {
                    if (check(TokenKind::Less) || check(TokenKind::LParen)) {
                        if (check(TokenKind::LParen)) ++paren_depth;
                        else ++depth;
                    } else if (check(TokenKind::RParen)) {
                        if (paren_depth > 0) --paren_depth;
                    } else if (check(TokenKind::GreaterGreater)) {
                        if (paren_depth == 0 && depth <= 0) break;
                        if (paren_depth == 0 && depth == 1) {
                            parse_greater_than_in_template_list(false);
                            break;
                        }
                        if (depth >= 2) depth -= 2;
                        else if (depth == 1) --depth;
                        consume();
                        continue;
                    } else if (check(TokenKind::Greater)) {
                        if (paren_depth == 0 && depth == 0) break;
                        if (depth > 0) --depth;
                    } else if (check(TokenKind::Comma) && depth == 0 &&
                               paren_depth == 0) {
                        break;
                    }
                    consume();
                }
            }
        } else {
            consume();
        }
        if (!match(TokenKind::Comma)) break;
    }
    expect_template_close();
    if (!injected_type_params->empty()) {
        std::vector<TemplateScopeParam> member_params;
        for (const auto& n : *injected_type_params) {
            TemplateScopeParam p;
            p.name = arena_.strdup(n.c_str());
            p.is_nttp = false;
            member_params.push_back(p);
        }
        push_template_scope(TemplateScopeKind::MemberTemplate, member_params);
        if (pushed_template_scope) *pushed_template_scope = true;
    }
}

bool Parser::try_parse_record_using_member(
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    if (!(is_cpp_mode() && check(TokenKind::KwUsing)))
        return false;

    consume();
    if (check(TokenKind::Identifier) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::Assign) {
        std::string alias_name = cur().lexeme;
        consume(); // name
        consume(); // '='
        TypeSpec alias_ts = parse_type_name();
        match(TokenKind::Semi);

        typedefs_.insert(alias_name);
        typedef_types_[alias_name] = alias_ts;
        member_typedef_names->push_back(arena_.strdup(alias_name.c_str()));
        member_typedef_types->push_back(alias_ts);
        if (!current_struct_tag_.empty()) {
            std::string scoped = current_struct_tag_ + "::" + alias_name;
            typedefs_.insert(scoped);
            typedef_types_[scoped] = alias_ts;
        }
        return true;
    }

    while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace))
        consume();
    match(TokenKind::Semi);
    return true;
}

bool Parser::try_parse_record_typedef_member(
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    if (!(is_cpp_mode() && check(TokenKind::KwTypedef)))
        return false;

    consume(); // eat 'typedef'
    TypeSpec td_base = parse_base_type();
    parse_attributes(&td_base);
    const char* tdname = nullptr;
    TypeSpec ts_copy = td_base;
    Node** td_fn_ptr_params = nullptr;
    int td_n_fn_ptr_params = 0;
    bool td_fn_ptr_variadic = false;
    parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                     &td_n_fn_ptr_params, &td_fn_ptr_variadic);
    if (tdname) {
        typedefs_.insert(tdname);
        user_typedefs_.insert(tdname);
        typedef_types_[tdname] = ts_copy;
        if (ts_copy.is_fn_ptr &&
            (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
            typedef_fn_ptr_info_[tdname] = {
                td_fn_ptr_params, td_n_fn_ptr_params,
                td_fn_ptr_variadic};
        }
        member_typedef_names->push_back(tdname);
        member_typedef_types->push_back(ts_copy);
        if (!current_struct_tag_.empty()) {
            std::string scoped = current_struct_tag_ + "::" + tdname;
            struct_typedefs_[scoped] = ts_copy;
            typedefs_.insert(scoped);
            typedef_types_[scoped] = ts_copy;
        }
    }

    while (match(TokenKind::Comma)) {
        TypeSpec ts2 = td_base;
        const char* tdn2 = nullptr;
        Node** td2_fn_ptr_params = nullptr;
        int td2_n_fn_ptr_params = 0;
        bool td2_fn_ptr_variadic = false;
        parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                         &td2_n_fn_ptr_params, &td2_fn_ptr_variadic);
        if (tdn2) {
            typedefs_.insert(tdn2);
            user_typedefs_.insert(tdn2);
            typedef_types_[tdn2] = ts2;
            if (ts2.is_fn_ptr &&
                (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                typedef_fn_ptr_info_[tdn2] = {
                    td2_fn_ptr_params, td2_n_fn_ptr_params,
                    td2_fn_ptr_variadic};
            }
            member_typedef_names->push_back(tdn2);
            member_typedef_types->push_back(ts2);
            if (!current_struct_tag_.empty()) {
                std::string scoped = current_struct_tag_ + "::" + tdn2;
                struct_typedefs_[scoped] = ts2;
                typedefs_.insert(scoped);
                typedef_types_[scoped] = ts2;
            }
        }
    }
    match(TokenKind::Semi);
    return true;
}

bool Parser::try_parse_nested_record_member(
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field) {
    if (!(check(TokenKind::KwStruct) || check(TokenKind::KwClass) ||
          check(TokenKind::KwUnion)))
        return false;

    bool inner_union = (cur().kind == TokenKind::KwUnion);
    consume();
    Node* inner = parse_struct_or_union(inner_union);
    if (inner) struct_defs_.push_back(inner);

    TypeSpec anon_fts{};
    anon_fts.array_size = -1;
    anon_fts.array_rank = 0;
    anon_fts.base = inner_union ? TB_UNION : TB_STRUCT;
    anon_fts.tag = inner ? inner->name : nullptr;
    parse_attributes(&anon_fts);

    bool has_declarator = !check(TokenKind::Semi) && !check(TokenKind::RBrace);
    if (has_declarator) {
        const char* fname = nullptr;
        parse_declarator(anon_fts, &fname);
        if (fname) {
            Node* f = make_node(NK_DECL, cur().line);
            f->type = anon_fts;
            f->name = fname;
            check_dup_field(fname);
            fields->push_back(f);
        } else if (inner && inner->name) {
            Node* f = make_node(NK_DECL, cur().line);
            f->type = anon_fts;
            f->name = inner->name;
            f->is_anon_field = true;
            fields->push_back(f);
        }
        while (match(TokenKind::Comma)) {
            TypeSpec fts2{};
            fts2.array_size = -1;
            fts2.array_rank = 0;
            fts2.base = inner_union ? TB_UNION : TB_STRUCT;
            fts2.tag = inner ? inner->name : nullptr;
            const char* fname2 = nullptr;
            parse_declarator(fts2, &fname2);
            if (fname2) {
                Node* f2 = make_node(NK_DECL, cur().line);
                f2->type = fts2;
                f2->name = fname2;
                check_dup_field(fname2);
                fields->push_back(f2);
            }
        }
    } else if (inner && inner->name) {
        Node* f = make_node(NK_DECL, cur().line);
        f->type = anon_fts;
        f->name = inner->name;
        f->is_anon_field = true;
        fields->push_back(f);
    }
    match(TokenKind::Semi);
    return true;
}

bool Parser::try_parse_record_enum_member(
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field) {
    if (!check(TokenKind::KwEnum))
        return false;

    consume();
    Node* ed = parse_enum();
    if (ed && parsing_top_level_context_) struct_defs_.push_back(ed);
    // After the enum body, there may be one or more field declarators:
    // e.g.  enum { X } x;   or   enum E { A, B } kind;
    // If the next token starts a declarator (not ; or }), parse it.
    if (!check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
        TypeSpec fts{};
        fts.array_size = -1;
        fts.array_rank = 0;
        // Base type is the enum (use int as fallback since enums are ints)
        fts.base = TB_INT;
        if (ed && ed->name) {
            // Prefer enum type tag if available
            fts.base = TB_ENUM;
            fts.tag  = ed->name;
        }
        while (true) {
            TypeSpec cur_fts = fts;
            const char* fname = nullptr;
            parse_declarator(cur_fts, &fname);
            skip_attributes();
            long long bf_width = -1;
            if (check(TokenKind::Colon)) {
                consume();
                Node* bfw = parse_assign_expr();
                if (bfw) eval_const_int(bfw, &bf_width, &struct_tag_def_map_);
            }
            if (fname) {
                Node* f = make_node(NK_DECL, cur().line);
                f->type = cur_fts;
                f->name = fname;
                f->ival = bf_width;  // -1 = not a bitfield; N = N-bit bitfield
                check_dup_field(fname);
                fields->push_back(f);
            }
            if (!match(TokenKind::Comma)) break;
        }
    }
    match(TokenKind::Semi);
    return true;
}

bool Parser::is_record_special_member_name(
    const std::string& lex, const std::string& struct_source_name) const {
    if (lex == current_struct_tag_) return true;
    return !struct_source_name.empty() && lex == struct_source_name;
}

bool Parser::try_parse_record_constructor_member(
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    if (!(is_cpp_mode() && !current_struct_tag_.empty()))
        return false;

    int probe = pos_;
    while (probe < static_cast<int>(tokens_.size()) &&
           (tokens_[probe].kind == TokenKind::KwConstexpr ||
            tokens_[probe].kind == TokenKind::KwConsteval ||
            tokens_[probe].kind == TokenKind::KwExplicit ||
            (tokens_[probe].kind == TokenKind::Identifier &&
             tokens_[probe].lexeme == "inline"))) {
        ++probe;
    }
    if (probe < static_cast<int>(tokens_.size()) &&
        tokens_[probe].kind == TokenKind::Identifier &&
        is_record_special_member_name(tokens_[probe].lexeme,
                                      struct_source_name) &&
        probe + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[probe + 1].kind == TokenKind::LParen) {
        while (pos_ < probe) consume();
    }

    if (!(check(TokenKind::Identifier) &&
          is_record_special_member_name(cur().lexeme, struct_source_name) &&
          pos_ + 1 < static_cast<int>(tokens_.size()) &&
          tokens_[pos_ + 1].kind == TokenKind::LParen)) {
        return false;
    }

    const char* ctor_name = arena_.strdup(cur().lexeme);
    consume();  // consume the struct tag name
    consume();  // consume '('
    std::vector<Node*> params;
    bool variadic = false;
    if (!check(TokenKind::RParen)) {
        while (!at_end()) {
            if (check(TokenKind::Ellipsis)) {
                variadic = true;
                consume();
                break;
            }
            if (check(TokenKind::RParen)) break;
            if (!is_type_start()) break;
            Node* p = parse_param();
            if (p) params.push_back(p);
            if (check(TokenKind::Ellipsis)) {
                variadic = true;
                consume();
                break;
            }
            if (!match(TokenKind::Comma)) break;
        }
    }
    expect(TokenKind::RParen);
    skip_exception_spec();
    Node* method = make_node(NK_FUNCTION, cur().line);
    method->type.base = TB_VOID;
    method->name = ctor_name;
    method->variadic = variadic;
    method->is_constructor = true;
    method->n_params = static_cast<int>(params.size());
    if (method->n_params > 0) {
        method->params = arena_.alloc_array<Node*>(method->n_params);
        for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
    }
    if (check(TokenKind::Colon)) {
        consume();  // ':'
        std::vector<const char*> init_names;
        std::vector<std::vector<Node*>> init_args_list;
        while (!at_end() && !check(TokenKind::LBrace)) {
            if (!check(TokenKind::Identifier)) break;
            const char* mem_name = arena_.strdup(cur().lexeme);
            consume();  // member name
            expect(TokenKind::LParen);
            std::vector<Node*> args;
            if (!check(TokenKind::RParen)) {
                while (true) {
                    Node* arg = parse_assign_expr();
                    if (arg) args.push_back(arg);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);
            init_names.push_back(mem_name);
            init_args_list.push_back(std::move(args));
            if (!match(TokenKind::Comma)) break;
        }
        method->n_ctor_inits = static_cast<int>(init_names.size());
        if (method->n_ctor_inits > 0) {
            method->ctor_init_names =
                arena_.alloc_array<const char*>(method->n_ctor_inits);
            method->ctor_init_args =
                arena_.alloc_array<Node**>(method->n_ctor_inits);
            method->ctor_init_nargs =
                arena_.alloc_array<int>(method->n_ctor_inits);
            for (int i = 0; i < method->n_ctor_inits; ++i) {
                method->ctor_init_names[i] = init_names[i];
                method->ctor_init_nargs[i] =
                    static_cast<int>(init_args_list[i].size());
                if (method->ctor_init_nargs[i] > 0) {
                    method->ctor_init_args[i] = arena_.alloc_array<Node*>(
                        method->ctor_init_nargs[i]);
                    for (int j = 0; j < method->ctor_init_nargs[i]; ++j)
                        method->ctor_init_args[i][j] = init_args_list[i][j];
                } else {
                    method->ctor_init_args[i] = nullptr;
                }
            }
        }
    }
    if (check(TokenKind::LBrace)) {
        method->body = parse_block();
    } else if (is_cpp_mode() && check(TokenKind::Assign) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::KwDelete) {
        consume();  // '='
        consume();  // 'delete'
        method->is_deleted = true;
        match(TokenKind::Semi);
    } else if (is_cpp_mode() && check(TokenKind::Assign) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::KwDefault) {
        consume();  // '='
        consume();  // 'default'
        method->is_defaulted = true;
        match(TokenKind::Semi);
    } else {
        match(TokenKind::Semi);
    }
    methods->push_back(method);
    return true;
}

bool Parser::try_parse_record_destructor_member(
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    if (!(is_cpp_mode() && !current_struct_tag_.empty() &&
          check(TokenKind::Tilde) &&
          pos_ + 1 < static_cast<int>(tokens_.size()) &&
          tokens_[pos_ + 1].kind == TokenKind::Identifier &&
          is_record_special_member_name(tokens_[pos_ + 1].lexeme,
                                        struct_source_name))) {
        return false;
    }

    consume();  // consume '~'
    const char* dtor_name = arena_.strdup(cur().lexeme);
    consume();  // consume struct tag name
    expect(TokenKind::LParen);
    expect(TokenKind::RParen);
    skip_exception_spec();
    std::string mangled = std::string("~") + dtor_name;
    Node* method = make_node(NK_FUNCTION, cur().line);
    method->type.base = TB_VOID;
    method->name = arena_.strdup(mangled.c_str());
    method->is_destructor = true;
    method->n_params = 0;
    if (check(TokenKind::LBrace)) {
        method->body = parse_block();
    } else if (is_cpp_mode() && check(TokenKind::Assign) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::KwDelete) {
        consume();  // '='
        consume();  // 'delete'
        method->is_deleted = true;
        match(TokenKind::Semi);
    } else if (is_cpp_mode() && check(TokenKind::Assign) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::KwDefault) {
        consume();  // '='
        consume();  // 'default'
        method->is_defaulted = true;
        match(TokenKind::Semi);
    } else {
        match(TokenKind::Semi);
    }
    methods->push_back(method);
    return true;
}

bool Parser::try_parse_record_method_or_field_member(
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    const std::function<void(const char*)>& check_dup_field) {
    // Regular field declaration
    // C++ conversion operators (e.g., operator bool()) have no return
    // type prefix, so KwOperator can appear directly here.
    bool is_conversion_operator = false;
    if (is_cpp_mode() && check(TokenKind::KwOperator)) {
        is_conversion_operator = true;
    } else if (!is_type_start()) {
        // unknown token in struct body — skip
        consume();
        return true;
    }

    // Track C++ storage-class specifiers before parse_base_type consumes them.
    bool field_is_static = false;
    bool field_is_constexpr = false;
    if (is_cpp_mode()) {
        int save_pos = pos_;
        while (!at_end() && !check(TokenKind::RBrace)) {
            if (check(TokenKind::KwStatic)) { field_is_static = true; consume(); continue; }
            if (check(TokenKind::KwConstexpr)) { field_is_constexpr = true; consume(); continue; }
            // Only peek through qualifiers/storage-class keywords
            if (check(TokenKind::KwConst) || check(TokenKind::KwVolatile) ||
                check(TokenKind::KwInline) || check(TokenKind::KwExtern) ||
                check(TokenKind::KwConsteval) || check(TokenKind::KwExplicit)) {
                consume();
                continue;
            }
            break;
        }
        pos_ = save_pos;
    }

    TypeSpec fts{};
    if (!is_conversion_operator) {
        fts = parse_base_type();
        parse_attributes(&fts);
    }

    // C++ operator method: <return-type> operator<symbol>(<params>) { ... }
    // Consume pointer/reference declarator tokens between the base type
    // and the 'operator' keyword (e.g., Inner* operator->(), int& operator*()).
    if (is_cpp_mode() && !is_conversion_operator && check(TokenKind::Star)) {
        while (check(TokenKind::Star)) { consume(); fts.ptr_level++; }
    }
    if (is_cpp_mode() && !is_conversion_operator && check(TokenKind::AmpAmp)) {
        consume();
        fts.is_rvalue_ref = true;
    } else if (is_cpp_mode() && !is_conversion_operator && check(TokenKind::Amp)) {
        consume();
        fts.is_lvalue_ref = true;
    }
    if (is_cpp_mode() && (is_conversion_operator || check(TokenKind::KwOperator))) {
        OperatorKind op_kind = OP_NONE;
        const char* op_mangled = nullptr;
        consume(); // eat 'operator'

        // Determine which operator
        std::string conversion_mangled_name;
        if (is_type_start()) {
            // Conversion operator: operator T() — the token after 'operator'
            // is a type name. This covers both standalone 'operator T()' and
            // prefix forms like 'explicit operator T()' / 'constexpr explicit operator T()'.
            is_conversion_operator = true;
            fts = parse_base_type();
            parse_attributes(&fts);
            if (check(TokenKind::Star)) {
                while (check(TokenKind::Star)) {
                    consume();
                    fts.ptr_level++;
                }
            }
            if (check(TokenKind::AmpAmp)) {
                consume();
                fts.is_rvalue_ref = true;
            } else if (check(TokenKind::Amp)) {
                consume();
                fts.is_lvalue_ref = true;
            }
            if (fts.base == TB_BOOL && fts.ptr_level == 0 &&
                !fts.is_lvalue_ref && !fts.is_rvalue_ref) {
                op_kind = OP_BOOL;
            } else {
                conversion_mangled_name = "operator_conv_";
                append_type_mangled_suffix(conversion_mangled_name, fts);
            }
        } else if (check(TokenKind::KwNew)) {
            consume();
            conversion_mangled_name = check(TokenKind::LBracket)
                ? "operator_new_array"
                : "operator_new";
            if (check(TokenKind::LBracket)) {
                consume();
                expect(TokenKind::RBracket);
            }
        } else if (check(TokenKind::KwDelete)) {
            consume();
            conversion_mangled_name = check(TokenKind::LBracket)
                ? "operator_delete_array"
                : "operator_delete";
            if (check(TokenKind::LBracket)) {
                consume();
                expect(TokenKind::RBracket);
            }
        } else if (check(TokenKind::LBracket)) {
            consume(); expect(TokenKind::RBracket);
            op_kind = OP_SUBSCRIPT;
        } else if (check(TokenKind::Star)) {
            consume();
            op_kind = OP_DEREF;
            conversion_mangled_name = "operator_star_pending";
        } else if (check(TokenKind::Arrow)) {
            consume();
            op_kind = OP_ARROW;
        } else if (check(TokenKind::PlusPlus)) {
            consume();
            op_kind = OP_PRE_INC; // may become OP_POST_INC based on params
        } else if (check(TokenKind::EqualEqual)) {
            consume();
            op_kind = OP_EQ;
        } else if (check(TokenKind::BangEqual)) {
            consume();
            op_kind = OP_NEQ;
        } else if (check(TokenKind::Plus)) {
            consume();
            op_kind = OP_PLUS;
        } else if (check(TokenKind::Minus)) {
            consume();
            op_kind = OP_MINUS;
        } else if (check(TokenKind::Assign)) {
            consume();
            op_kind = OP_ASSIGN;
        } else if (check(TokenKind::LessEqual)) {
            consume();
            op_kind = OP_LE;
        } else if (check(TokenKind::GreaterEqual)) {
            consume();
            op_kind = OP_GE;
        } else if (check(TokenKind::Less)) {
            consume();
            op_kind = OP_LT;
        } else if (check(TokenKind::Greater)) {
            consume();
            op_kind = OP_GT;
        } else if (check(TokenKind::LParen)) {
            // operator() — function call operator
            consume(); // eat '('
            expect(TokenKind::RParen); // eat ')'
            op_kind = OP_CALL;
        } else if (check(TokenKind::Amp)) {
            consume();
            conversion_mangled_name = "operator_amp_pending";
        } else if (const char* extra_mangled = extra_operator_mangled_name(cur().kind)) {
            consume();
            conversion_mangled_name = extra_mangled;
        } else if (check(TokenKind::KwBool)) {
            // operator bool — conversion operator; return type is bool
            consume();
            op_kind = OP_BOOL;
            fts = TypeSpec{};
            fts.base = TB_BOOL;
        } else {
            // Unknown operator token — error
            throw std::runtime_error(
                std::string("unsupported operator overload token '") +
                cur().lexeme + "' at line " + std::to_string(cur().line));
        }

        op_mangled = operator_kind_mangled_name(op_kind);
        if (!conversion_mangled_name.empty()) {
            op_mangled = arena_.strdup(conversion_mangled_name.c_str());
        }

        // Parse parameter list
        expect(TokenKind::LParen);
        std::vector<Node*> params;
        bool variadic = false;
        if (!check(TokenKind::RParen)) {
            while (!at_end()) {
                if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                if (check(TokenKind::RParen)) break;
                if (!is_type_start()) break;
                Node* p = parse_param();
                if (p) params.push_back(p);
                if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                if (!match(TokenKind::Comma)) break;
            }
        }
        expect(TokenKind::RParen);

        // Distinguish prefix vs postfix operator++ by parameter count:
        // prefix: operator++() — 0 params; postfix: operator++(int) — 1 param
        if (op_kind == OP_PRE_INC && !params.empty()) {
            op_kind = OP_POST_INC;
            op_mangled = operator_kind_mangled_name(op_kind);
        }
        if (!conversion_mangled_name.empty()) {
            finalize_pending_operator_name(conversion_mangled_name, params.size());
            op_mangled = arena_.strdup(conversion_mangled_name.c_str());
        }

        // Parse trailing qualifiers (const, constexpr, consteval)
        bool is_method_const = false;
        bool is_method_constexpr = false;
        bool is_method_consteval = false;
        while (true) {
            if (match(TokenKind::KwConst)) { is_method_const = true; }
            else if (is_cpp_mode() && match(TokenKind::KwConstexpr)) {
                is_method_constexpr = true;
            } else if (is_cpp_mode() && match(TokenKind::KwConsteval)) {
                is_method_consteval = true;
            } else {
                break;
            }
        }
        // Skip C++ ref-qualifiers: & or &&
        if (is_cpp_mode()) {
            if (check(TokenKind::AmpAmp)) consume();
            else if (check(TokenKind::Amp)) consume();
        }
        skip_exception_spec();
        if (is_cpp_mode() && match(TokenKind::Arrow)) {
            fts = parse_type_name();
            parse_attributes(&fts);
        }

        Node* method = make_node(NK_FUNCTION, cur().line);
        method->type = fts;
        method->name = arena_.strdup(op_mangled);
        method->operator_kind = op_kind;
        method->variadic = variadic;
        method->is_const_method = is_method_const;
        method->is_constexpr = is_method_constexpr;
        method->is_consteval = is_method_consteval;
        method->n_params = static_cast<int>(params.size());
        if (method->n_params > 0) {
            method->params = arena_.alloc_array<Node*>(method->n_params);
            for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
        }
        if (check(TokenKind::LBrace)) {
            method->body = parse_block();
        } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                   pos_ + 1 < static_cast<int>(tokens_.size()) &&
                   tokens_[pos_ + 1].kind == TokenKind::KwDelete) {
            consume(); // '='
            consume(); // 'delete'
            method->is_deleted = true;
            match(TokenKind::Semi);
        } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                   pos_ + 1 < static_cast<int>(tokens_.size()) &&
                   tokens_[pos_ + 1].kind == TokenKind::KwDefault) {
            consume(); // '='
            consume(); // 'default'
            method->is_defaulted = true;
            match(TokenKind::Semi);
        } else {
            match(TokenKind::Semi);
        }
        methods->push_back(method);
        return true;
    }

    // Handle anonymous bitfield: just ': expr;'
    if (check(TokenKind::Colon)) {
        consume();
        parse_assign_expr();  // skip bitfield width
        match(TokenKind::Semi);
        return true;
    }

    // If semicolon follows immediately: anonymous struct field (no name)
    if (check(TokenKind::Semi)) {
        match(TokenKind::Semi);
        return true;
    }

    bool first = true;
    while (true) {
        TypeSpec cur_fts = fts;
        if (!first) {
            // For multi-declarator fields, re-use base type.
        }
        first = false;
        const char* fname = nullptr;
        Node** fn_ptr_params = nullptr;
        int n_fn_ptr_params = 0;
        bool fn_ptr_variadic = false;
        parse_declarator(cur_fts, &fname, &fn_ptr_params,
                         &n_fn_ptr_params, &fn_ptr_variadic);
        skip_attributes();

        if (fname && check(TokenKind::LParen)) {
            consume();
            std::vector<Node*> params;
            bool variadic = false;
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) {
                        variadic = true;
                        consume();
                        break;
                    }
                    if (check(TokenKind::RParen)) break;
                    if (!is_type_start()) break;
                    Node* p = parse_param();
                    if (p) params.push_back(p);
                    if (check(TokenKind::Ellipsis)) {
                        variadic = true;
                        consume();
                        break;
                    }
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);
            bool is_method_const = false;
            bool is_method_constexpr = false;
            bool is_method_consteval = false;
            while (true) {
                if (match(TokenKind::KwConst)) { is_method_const = true; }
                else if (is_cpp_mode() && match(TokenKind::KwConstexpr)) {
                    is_method_constexpr = true;
                } else if (is_cpp_mode() && match(TokenKind::KwConsteval)) {
                    is_method_consteval = true;
                } else {
                    break;
                }
            }
            if (is_cpp_mode()) {
                if (check(TokenKind::AmpAmp)) consume();
                else if (check(TokenKind::Amp)) consume();
            }
            skip_exception_spec();
            if (is_cpp_mode() && match(TokenKind::Arrow)) {
                cur_fts = parse_type_name();
                parse_attributes(&cur_fts);
            }
            Node* method = make_node(NK_FUNCTION, cur().line);
            method->type = cur_fts;
            method->name = fname;
            method->variadic = variadic;
            method->is_const_method = is_method_const;
            method->is_constexpr = is_method_constexpr;
            method->is_consteval = is_method_consteval;
            method->n_params = static_cast<int>(params.size());
            if (method->n_params > 0) {
                method->params = arena_.alloc_array<Node*>(method->n_params);
                for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
            }
            if (check(TokenKind::LBrace)) {
                method->body = parse_block();
            } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                       pos_ + 1 < static_cast<int>(tokens_.size()) &&
                       tokens_[pos_ + 1].kind == TokenKind::KwDelete) {
                consume(); // '='
                consume(); // 'delete'
                method->is_deleted = true;
                match(TokenKind::Semi);
            } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                       pos_ + 1 < static_cast<int>(tokens_.size()) &&
                       tokens_[pos_ + 1].kind == TokenKind::KwDefault) {
                consume(); // '='
                consume(); // 'default'
                method->is_defaulted = true;
                match(TokenKind::Semi);
            } else {
                match(TokenKind::Semi);
            }
            methods->push_back(method);
            if (!match(TokenKind::Comma)) break;
            continue;
        }

        long long bf_width = -1;
        if (check(TokenKind::Colon)) {
            consume();
            Node* bfw = parse_assign_expr();
            if (bfw) eval_const_int(bfw, &bf_width, &struct_tag_def_map_);
        }

        Node* field_init_expr = nullptr;
        if (is_cpp_mode() && check(TokenKind::Assign)) {
            consume(); // eat '='
            if (field_is_static) {
                try {
                    field_init_expr = parse_assign_expr();
                } catch (...) {
                    field_init_expr = nullptr;
                }
            }
            if (!field_init_expr) {
                int depth = 0, angle_depth = 0;
                while (!at_end()) {
                    if (check(TokenKind::LParen) || check(TokenKind::LBrace) ||
                        check(TokenKind::LBracket)) { ++depth; consume(); }
                    else if (check(TokenKind::RParen) || check(TokenKind::RBrace) ||
                             check(TokenKind::RBracket)) {
                        if (depth == 0) break;
                        --depth; consume();
                    }
                    else if (check(TokenKind::Less) && depth == 0) {
                        ++angle_depth; consume();
                    }
                    else if (check(TokenKind::Greater) && angle_depth > 0 && depth == 0) {
                        --angle_depth; consume();
                    }
                    else if (check(TokenKind::GreaterGreater) && angle_depth > 0 && depth == 0) {
                        angle_depth -= std::min(angle_depth, 2);
                        consume();
                    }
                    else if ((check(TokenKind::Semi) || check(TokenKind::Comma)) &&
                             depth == 0 && angle_depth == 0) break;
                    else consume();
                }
            }
        }

        if (fname) {
            Node* f = make_node(NK_DECL, cur().line);
            f->type = cur_fts;
            f->name = fname;
            f->ival = bf_width;
            f->is_static = field_is_static;
            f->is_constexpr = field_is_constexpr;
            f->init = field_init_expr;
            f->fn_ptr_params = fn_ptr_params;
            f->n_fn_ptr_params = n_fn_ptr_params;
            f->fn_ptr_variadic = fn_ptr_variadic;
            if (cur_fts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
                auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
                if (tdit != typedef_fn_ptr_info_.end()) {
                    f->fn_ptr_params = tdit->second.params;
                    f->n_fn_ptr_params = tdit->second.n_params;
                    f->fn_ptr_variadic = tdit->second.variadic;
                }
            }
            check_dup_field(fname);
            fields->push_back(f);
        }

        if (!match(TokenKind::Comma)) break;
    }
    match(TokenKind::Semi);
    return true;
}

bool Parser::try_parse_record_member_dispatch(
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    if (try_parse_record_using_member(member_typedef_names,
                                      member_typedef_types)) {
        return true;
    }
    if (try_parse_nested_record_member(fields, check_dup_field)) return true;
    if (try_parse_record_enum_member(fields, check_dup_field)) return true;
    if (try_parse_record_typedef_member(member_typedef_names,
                                        member_typedef_types)) {
        return true;
    }
    if (try_parse_record_constructor_member(struct_source_name, methods)) {
        return true;
    }
    if (try_parse_record_destructor_member(struct_source_name, methods)) {
        return true;
    }
    return try_parse_record_method_or_field_member(fields, methods,
                                                   check_dup_field);
}

bool Parser::try_parse_record_member(
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    skip_attributes();
    if (check(TokenKind::RBrace)) return false;

    if (try_parse_record_access_label()) return true;
    if (try_skip_record_friend_member()) return true;
    if (try_skip_record_static_assert_member()) return true;

    struct TemplateParamGuard {
        std::set<std::string>& typedefs;
        std::unordered_map<std::string, TypeSpec>& typedef_types;
        std::vector<TemplateScopeFrame>& scope_stack;
        std::vector<std::string> names;
        bool pushed_scope = false;
        ~TemplateParamGuard() {
            if (pushed_scope) scope_stack.pop_back();
            for (auto& n : names) {
                typedefs.erase(n);
                typedef_types.erase(n);
            }
        }
    } tmpl_guard{typedefs_, typedef_types_, template_scope_stack_, {}};
    parse_record_template_member_prelude(&tmpl_guard.names,
                                         &tmpl_guard.pushed_scope);

    return try_parse_record_member_dispatch(struct_source_name, fields, methods,
                                            member_typedef_names,
                                            member_typedef_types,
                                            check_dup_field);
}

void Parser::begin_record_body_context(const char* tag,
                                       const char* template_origin_name,
                                       std::string* saved_struct_tag,
                                       std::string* struct_source_name) {
    // Make the current record name available for self-type parsing within the
    // body before member dispatch starts.
    if (is_cpp_mode() && tag && tag[0])
        typedefs_.insert(tag);

    if (saved_struct_tag)
        *saved_struct_tag = current_struct_tag_;
    current_struct_tag_ = (tag && tag[0]) ? tag : "";

    if (struct_source_name)
        struct_source_name->clear();
    if (!(template_origin_name && template_origin_name[0]))
        return;

    if (struct_source_name)
        *struct_source_name = template_origin_name;
    if (!is_cpp_mode())
        return;

    typedefs_.insert(template_origin_name);
    if (typedef_types_.count(template_origin_name) == 0) {
        TypeSpec src_ts{};
        src_ts.array_size = -1;
        src_ts.inner_rank = -1;
        src_ts.base = TB_STRUCT;
        src_ts.tag = tag;
    typedef_types_[template_origin_name] = src_ts;
    }
}

void Parser::parse_record_body(
    const std::string& struct_source_name,
    RecordBodyState* body_state) {
    if (!body_state)
        return;

    std::unordered_set<std::string> field_names_seen;
    auto check_dup_field = [&](const char* fname) {
        if (!fname) return;
        std::string n(fname);
        if (field_names_seen.count(n)) {
            if (!is_cpp_mode())
                throw std::runtime_error(std::string("duplicate field name: ") + n);
            // C++ mode: allow duplicates (template specializations, complex types)
            return;
        }
        field_names_seen.insert(n);
    };

    while (!at_end() && !check(TokenKind::RBrace)) {
        int member_start_pos = pos_;
        try {
            if (try_parse_record_member(struct_source_name, &body_state->fields,
                                        &body_state->methods,
                                        &body_state->member_typedef_names,
                                        &body_state->member_typedef_types,
                                        check_dup_field)) {
                continue;
            }
        } catch (const std::exception&) {
            if (!recover_record_member_parse_error(member_start_pos))
                throw;
        }
    }
}

void Parser::parse_record_body_with_context(
    const char* tag,
    const char* template_origin_name,
    RecordBodyState* body_state) {
    std::string saved_struct_tag = current_struct_tag_;
    std::string struct_source_name;
    begin_record_body_context(tag, template_origin_name, &saved_struct_tag,
                              &struct_source_name);
    parse_record_body(struct_source_name, body_state);
    expect(TokenKind::RBrace);
    current_struct_tag_ = saved_struct_tag;
}

void Parser::parse_record_prebody_setup(
    int line,
    TypeSpec* attr_ts,
    const char** tag,
    const char** template_origin_name,
    std::vector<TemplateArgParseResult>* specialization_args,
    std::vector<TypeSpec>* base_types) {
    if (!attr_ts || !tag || !template_origin_name || !specialization_args ||
        !base_types) {
        return;
    }

    *attr_ts = {};
    *tag = nullptr;
    *template_origin_name = nullptr;
    specialization_args->clear();
    base_types->clear();

    auto skip_cpp11_attrs = [&]() {
        while (check(TokenKind::LBracket) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::LBracket) {
            consume();
            consume();
            int depth = 1;
            while (pos_ < static_cast<int>(tokens_.size()) && depth > 0) {
                if (check(TokenKind::LBracket)) ++depth;
                else if (check(TokenKind::RBracket)) --depth;
                if (depth > 0) consume();
            }
            if (check(TokenKind::RBracket)) consume();
            if (check(TokenKind::RBracket)) consume();
        }
    };
    auto parse_decl_attrs = [&]() {
        skip_cpp11_attrs();
        while (check(TokenKind::KwAlignas))
            parse_alignas_specifier(this, attr_ts, line);
        parse_attributes(attr_ts);
        skip_cpp11_attrs();
    };
    parse_decl_attrs();

    if (check(TokenKind::Identifier)) {
        *tag = arena_.strdup(cur().lexeme);
        consume();
    }
    parse_decl_attrs();

    if (*tag && is_cpp_mode() && check(TokenKind::Less)) {
        int probe = pos_;
        int depth = 0;
        bool balanced = false;
        while (probe < static_cast<int>(tokens_.size())) {
            if (tokens_[probe].kind == TokenKind::Less) ++depth;
            else if (tokens_[probe].kind == TokenKind::Greater) {
                if (--depth <= 0) {
                    ++probe;
                    balanced = true;
                    break;
                }
            } else if (tokens_[probe].kind == TokenKind::GreaterGreater) {
                depth -= 2;
                if (depth <= 0) {
                    ++probe;
                    balanced = true;
                    break;
                }
            }
            ++probe;
        }
        const bool is_specialization =
            balanced && probe < static_cast<int>(tokens_.size()) &&
            (tokens_[probe].kind == TokenKind::LBrace ||
             tokens_[probe].kind == TokenKind::Colon);
        if (is_specialization) {
            *template_origin_name = arena_.strdup(*tag);
            const int saved_pos = pos_;
            bool parse_ok = true;
            try {
                const Node* primary_tpl = find_template_struct_primary(*tag);
                if (!parse_template_argument_list(specialization_args,
                                                  primary_tpl)) {
                    throw std::runtime_error(
                        "failed to parse specialization args");
                }
            } catch (...) {
                parse_ok = false;
                pos_ = saved_pos;
                specialization_args->clear();
                if (check(TokenKind::Less)) {
                    int angle_depth = 1;
                    consume();
                    while (!at_end() && angle_depth > 0) {
                        if (check(TokenKind::Less)) ++angle_depth;
                        else if (check_template_close()) {
                            --angle_depth;
                            if (angle_depth > 0) {
                                match_template_close();
                                continue;
                            }
                            break;
                        }
                        consume();
                    }
                    if (check_template_close()) match_template_close();
                }
            }
            (void)parse_ok;
            std::string mangled(*tag);
            mangled += "__spec_";
            mangled += std::to_string(anon_counter_++);
            *tag = arena_.strdup(mangled.c_str());
        }
    }

    if (is_cpp_mode() && check(TokenKind::Colon)) {
        consume();
        while (!at_end() && !check(TokenKind::LBrace)) {
            if (check(TokenKind::Identifier) &&
                (cur().lexeme == "public" || cur().lexeme == "private" ||
                 cur().lexeme == "protected" ||
                 cur().lexeme == "virtual")) {
                consume();
                continue;
            }
            try {
                TypeSpec base_ts = parse_base_type();
                while (check(TokenKind::Star)) {
                    consume();
                    base_ts.ptr_level++;
                }
                if (check(TokenKind::AmpAmp)) {
                    consume();
                    base_ts.is_rvalue_ref = true;
                } else if (check(TokenKind::Amp)) {
                    consume();
                    base_ts.is_lvalue_ref = true;
                }
                while (!check(TokenKind::LBrace) && !check(TokenKind::Comma) &&
                       !check(TokenKind::RBrace) && !at_end()) {
                    int angle_depth = 0;
                    int paren_depth = 0;
                    while (!at_end()) {
                        if (check(TokenKind::LBrace) && angle_depth == 0 &&
                            paren_depth == 0) {
                            break;
                        }
                        if (check(TokenKind::Comma) && angle_depth == 0 &&
                            paren_depth == 0) {
                            break;
                        }
                        if (check(TokenKind::Less) && paren_depth == 0)
                            ++angle_depth;
                        else if (check(TokenKind::Greater) &&
                                 paren_depth == 0 && angle_depth > 0)
                            --angle_depth;
                        else if (check(TokenKind::GreaterGreater) &&
                                 paren_depth == 0 && angle_depth > 0) {
                            angle_depth -= std::min(angle_depth, 2);
                            consume();
                            continue;
                        } else if (check(TokenKind::LParen))
                            ++paren_depth;
                        else if (check(TokenKind::RParen) && paren_depth > 0)
                            --paren_depth;
                        consume();
                    }
                    break;
                }
                base_types->push_back(base_ts);
            } catch (...) {
                int angle_depth = 0;
                int paren_depth = 0;
                while (!at_end()) {
                    if (check(TokenKind::LBrace) && angle_depth == 0 &&
                        paren_depth == 0) {
                        break;
                    }
                    if (check(TokenKind::Comma) && angle_depth == 0 &&
                        paren_depth == 0) {
                        break;
                    }
                    if (check(TokenKind::Less) && paren_depth == 0)
                        ++angle_depth;
                    else if (check(TokenKind::Greater) &&
                             paren_depth == 0 && angle_depth > 0)
                        --angle_depth;
                    else if (check(TokenKind::GreaterGreater) &&
                             paren_depth == 0 && angle_depth > 0) {
                        angle_depth -= std::min(angle_depth, 2);
                        consume();
                        continue;
                    } else if (check(TokenKind::LParen))
                        ++paren_depth;
                    else if (check(TokenKind::RParen) && paren_depth > 0)
                        --paren_depth;
                    consume();
                }
            }
            if (!match(TokenKind::Comma))
                break;
        }
    }
}

Node* Parser::parse_record_tag_setup(int line,
                                     bool is_union,
                                     const char** tag) {
    if (!check(TokenKind::LBrace)) {
        const char* resolved_tag = tag ? *tag : nullptr;
        if (!resolved_tag) {
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_%d", anon_counter_++);
            resolved_tag = arena_.strdup(buf);
        } else {
            const std::string qtag =
                canonical_name_in_context(current_namespace_context_id(),
                                          resolved_tag);
            resolved_tag = arena_.strdup(qtag.c_str());
        }

        if (tag)
            *tag = resolved_tag;

        Node* ref = make_node(NK_STRUCT_DEF, line);
        ref->name = resolved_tag;
        ref->is_union = is_union;
        ref->n_fields = -1;  // -1 = forward reference (no body)
        if (is_cpp_mode() && parsing_top_level_context_)
            struct_defs_.push_back(ref);
        return ref;
    }

    const char* resolved_tag = tag ? *tag : nullptr;
    if (!resolved_tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_%d", anon_counter_++);
        resolved_tag = arena_.strdup(buf);
    } else {
        const std::string qtag =
            canonical_name_in_context(current_namespace_context_id(),
                                      resolved_tag);
        if (defined_struct_tags_.count(qtag)) {
            if (parsing_top_level_context_ && !is_cpp_mode()) {
                throw std::runtime_error(std::string("redefinition of ") +
                                         (is_union ? "union " : "struct ") +
                                         resolved_tag);
            }
            char buf[64];
            snprintf(buf, sizeof(buf), "%s.__shadow_%d", resolved_tag,
                     anon_counter_++);
            resolved_tag = arena_.strdup(buf);
        }
        defined_struct_tags_.insert(qtag);
    }

    if (tag)
        *tag = resolved_tag;

    return nullptr;
}

Node* Parser::initialize_record_definition(
    int line,
    bool is_union,
    const char* tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types) {
    Node* sd = make_node(NK_STRUCT_DEF, line);
    sd->name = tag;
    sd->is_union = is_union;
    sd->template_origin_name = template_origin_name;

    if (!base_types.empty()) {
        sd->n_bases = static_cast<int>(base_types.size());
        sd->base_types = arena_.alloc_array<TypeSpec>(sd->n_bases);
        for (int i = 0; i < sd->n_bases; ++i)
            sd->base_types[i] = base_types[i];
    }

    // __attribute__((packed)) => pack_align=1; otherwise use #pragma pack state
    sd->pack_align = attr_ts.is_packed ? 1 : pack_alignment_;
    sd->struct_align = attr_ts.align_bytes;  // __attribute__((aligned(N)))

    if (!specialization_args.empty()) {
        sd->n_template_args = static_cast<int>(specialization_args.size());
        sd->template_arg_types = arena_.alloc_array<TypeSpec>(sd->n_template_args);
        sd->template_arg_is_value = arena_.alloc_array<bool>(sd->n_template_args);
        sd->template_arg_values = arena_.alloc_array<long long>(sd->n_template_args);
        sd->template_arg_nttp_names =
            arena_.alloc_array<const char*>(sd->n_template_args);
        sd->template_arg_exprs = arena_.alloc_array<Node*>(sd->n_template_args);
        for (int i = 0; i < sd->n_template_args; ++i) {
            sd->template_arg_is_value[i] = specialization_args[i].is_value;
            sd->template_arg_types[i] = specialization_args[i].type;
            sd->template_arg_values[i] = specialization_args[i].value;
            sd->template_arg_nttp_names[i] = specialization_args[i].nttp_name;
            sd->template_arg_exprs[i] = specialization_args[i].expr;
        }
    }

    return sd;
}

void Parser::apply_record_trailing_type_attributes(Node* sd) {
    if (!sd || !check(TokenKind::KwAttribute))
        return;

    int save = pos_;
    TypeSpec trailing_attr{};
    parse_attributes(&trailing_attr);
    if (trailing_attr.is_packed && sd->pack_align == 0)
        sd->pack_align = 1;
    if (trailing_attr.align_bytes > 0 && sd->struct_align == 0)
        sd->struct_align = trailing_attr.align_bytes;
    // Restore position so the caller can re-parse attributes for the declaration.
    pos_ = save;
}

void Parser::store_record_body_members(
    Node* sd,
    const RecordBodyState& body_state) {
    if (!sd)
        return;

    sd->n_fields = static_cast<int>(body_state.fields.size());
    if (sd->n_fields > 0) {
        sd->fields = arena_.alloc_array<Node*>(sd->n_fields);
        for (int i = 0; i < sd->n_fields; ++i)
            sd->fields[i] = body_state.fields[i];
    }

    sd->n_member_typedefs =
        static_cast<int>(body_state.member_typedef_names.size());
    if (sd->n_member_typedefs > 0) {
        sd->member_typedef_names =
            arena_.alloc_array<const char*>(sd->n_member_typedefs);
        sd->member_typedef_types =
            arena_.alloc_array<TypeSpec>(sd->n_member_typedefs);
        for (int i = 0; i < sd->n_member_typedefs; ++i) {
            sd->member_typedef_names[i] = body_state.member_typedef_names[i];
            sd->member_typedef_types[i] = body_state.member_typedef_types[i];
        }
    }

    sd->n_children = static_cast<int>(body_state.methods.size());
    if (sd->n_children > 0) {
        sd->children = arena_.alloc_array<Node*>(sd->n_children);
        for (int i = 0; i < sd->n_children; ++i)
            sd->children[i] = body_state.methods[i];
    }
}

void Parser::finalize_record_definition(Node* sd,
                                        bool is_union,
                                        const char* source_tag) {
    if (!sd || !(source_tag && source_tag[0]))
        return;

    const std::string canonical =
        canonical_name_in_context(current_namespace_context_id(), source_tag);
    sd->name = arena_.strdup(canonical.c_str());
    apply_decl_namespace(sd, current_namespace_context_id(), source_tag);
    struct_tag_def_map_[source_tag] = sd;
    struct_tag_def_map_[sd->name] = sd;

    if (!is_cpp_mode() || !(sd->name && sd->name[0]))
        return;

    typedefs_.insert(sd->name);
    TypeSpec injected_ts{};
    injected_ts.array_size = -1;
    injected_ts.array_rank = 0;
    injected_ts.base = is_union ? TB_UNION : TB_STRUCT;
    injected_ts.tag = sd->name;
    typedef_types_[sd->name] = injected_ts;
}

void Parser::complete_record_definition(
    Node* sd,
    bool is_union,
    const char* source_tag,
    const RecordBodyState& body_state) {
    apply_record_trailing_type_attributes(sd);
    store_record_body_members(sd, body_state);
    finalize_record_definition(sd, is_union, source_tag);
    struct_defs_.push_back(sd);
}

Node* Parser::parse_struct_or_union(bool is_union) {
    int ln = cur().line;
    TypeSpec attr_ts{};
    const char* tag = nullptr;
    const char* template_origin_name = nullptr;
    std::vector<ParsedTemplateArg> specialization_args;
    std::vector<TypeSpec> base_types;
    parse_record_prebody_setup(ln, &attr_ts, &tag, &template_origin_name,
                               &specialization_args, &base_types);

    if (Node* ref = parse_record_tag_setup(ln, is_union, &tag))
        return ref;

    const char* source_tag = tag;
    Node* sd = initialize_record_definition(ln, is_union, tag,
                                            template_origin_name, attr_ts,
                                            specialization_args, base_types);

    consume();  // consume {

    RecordBodyState body_state;
    parse_record_body_with_context(tag, template_origin_name, &body_state);

    complete_record_definition(sd, is_union, source_tag, body_state);
    return sd;
}

// ── enum parsing ─────────────────────────────────────────────────────────────

Node* Parser::parse_enum() {
    int ln = cur().line;
    skip_attributes();

    const char* tag = nullptr;
    if (check(TokenKind::Identifier)) {
        tag = arena_.strdup(cur().lexeme);
        consume();
    }
    skip_attributes();

    if (!check(TokenKind::LBrace)) {
        if (!tag) {
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_enum_%d", anon_counter_++);
            tag = arena_.strdup(buf);
        }
        Node* ref = make_node(NK_ENUM_DEF, ln);
        ref->name = arena_.strdup(canonical_name_in_context(
            current_namespace_context_id(), tag).c_str());
        apply_decl_namespace(ref, current_namespace_context_id(), tag);
        ref->n_enum_variants = -1;
        return ref;
    }

    if (!tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_enum_%d", anon_counter_++);
        tag = arena_.strdup(buf);
    }

    Node* ed = make_node(NK_ENUM_DEF, ln);
    ed->name = arena_.strdup(canonical_name_in_context(
        current_namespace_context_id(), tag).c_str());
    apply_decl_namespace(ed, current_namespace_context_id(), tag);

    consume();  // consume {

    std::vector<const char*> names;
    std::vector<long long>   vals;
    std::unordered_set<std::string> seen_names;
    long long cur_val = 0;

    while (!at_end() && !check(TokenKind::RBrace)) {
        skip_attributes();
        if (!check(TokenKind::Identifier)) { consume(); continue; }
        const char* vname = arena_.strdup(cur().lexeme);
        std::string vname_s(vname ? vname : "");
        if (seen_names.count(vname_s))
            throw std::runtime_error("duplicate enumerator: " + vname_s);
        seen_names.insert(vname_s);
        consume();
        // Skip __attribute__((...)) between enum constant name and '='
        // e.g. _CLOCK_REALTIME __attribute__((availability(...))) = 0,
        skip_attributes();
        long long vval = cur_val;
        if (match(TokenKind::Assign)) {
            Node* ve = parse_assign_expr();
            if (!eval_enum_expr(ve, enum_consts_, &vval)) {
                if (is_cpp_mode() && is_dependent_enum_expr(ve, enum_consts_)) {
                    vval = 0;  // Placeholder until template/dependent evaluation exists.
                } else {
                throw std::runtime_error("enum initializer is not an integer constant expression");
                }
            }
        }
        // Skip __attribute__((...)) annotations after enum value (e.g. availability macros)
        skip_attributes();
        cur_val = vval + 1;
        names.push_back(vname);
        vals.push_back(vval);
        // Track enum constant for use in subsequent enum initializers
        enum_consts_[std::string(vname)] = vval;
        if (!match(TokenKind::Comma)) break;
        // Trailing comma before } is allowed
        if (check(TokenKind::RBrace)) break;
    }
    expect(TokenKind::RBrace);

    ed->n_enum_variants = (int)names.size();
    if (ed->n_enum_variants > 0) {
        ed->enum_names = arena_.alloc_array<const char*>(ed->n_enum_variants);
        ed->enum_vals  = arena_.alloc_array<long long>(ed->n_enum_variants);
        for (int i = 0; i < ed->n_enum_variants; ++i) {
            ed->enum_names[i] = names[i];
            ed->enum_vals[i]  = vals[i];
        }
    }
    if (parsing_top_level_context_) struct_defs_.push_back(ed);
    return ed;
}

// ── parameter parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_param() {
    int ln = cur().line;
    if (check(TokenKind::Ellipsis)) {
        // variadic marker — handled by caller
        consume();
        return nullptr;
    }
    if (check(TokenKind::KwStatic)) {
        throw std::runtime_error(
            std::string("invalid use of storage class 'static' in parameter declaration at line ") +
            std::to_string(cur().line));
    }
    TypeSpec pts = parse_base_type();
    const char* pname = nullptr;
    Node** fn_ptr_params = nullptr;
    int n_fn_ptr_params = 0;
    bool fn_ptr_variadic = false;
    bool is_parameter_pack = false;
    parse_declarator(pts, &pname, &fn_ptr_params, &n_fn_ptr_params, &fn_ptr_variadic,
                     &is_parameter_pack);
    parse_plain_function_declarator_suffix(
        pts, /*decay_to_function_pointer=*/true);
    skip_attributes();

    // C++ default parameter value: skip '= expr' (balanced, stopping at , or ) at depth 0)
    if (check(TokenKind::Assign)) {
        consume(); // eat '='
        int depth = 0;
        while (!at_end()) {
            if (check(TokenKind::LParen) || check(TokenKind::LBracket) ||
                check(TokenKind::LBrace)) {
                ++depth;
                consume();
            } else if (check(TokenKind::RParen) || check(TokenKind::RBracket) ||
                       check(TokenKind::RBrace)) {
                if (depth == 0) break;
                --depth;
                consume();
            } else if (check(TokenKind::Comma) && depth == 0) {
                break;
            } else {
                consume();
            }
        }
    }

    Node* p = make_node(NK_DECL, ln);
    p->type = pts;
    p->name = pname ? pname : nullptr;
    p->fn_ptr_params = fn_ptr_params;
    p->n_fn_ptr_params = n_fn_ptr_params;
    p->fn_ptr_variadic = fn_ptr_variadic;
    p->is_parameter_pack = is_parameter_pack;
    // Phase C: propagate fn_ptr params from typedef if not set by declarator.
    if (pts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
        auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
        if (tdit != typedef_fn_ptr_info_.end()) {
            p->fn_ptr_params = tdit->second.params;
            p->n_fn_ptr_params = tdit->second.n_params;
            p->fn_ptr_variadic = tdit->second.variadic;
        }
    }
    return p;
}

// ── expression parsing ────────────────────────────────────────────────────────

// Operator precedence table (higher = tighter binding).

}  // namespace c4c

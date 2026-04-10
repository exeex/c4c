// parser_types_template.cpp — template registry, NTTP evaluation
#include "parser.hpp"
#include "lexer.hpp"

#include <climits>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "types_helpers.hpp"

namespace c4c {

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
    if (!node || node->kind != NK_STRUCT_DEF || node->n_template_params <= 0 ||
        node->n_template_args != 0) {
        return;
    }
    template_struct_defs_[name] = node;
}

void Parser::register_template_struct_specialization(const char* primary_name, Node* node) {
    if (!primary_name || !primary_name[0] || !node) return;
    template_struct_specializations_[primary_name].push_back(node);
    if (!node->name) return;
    if (std::strstr(primary_name, "::")) return;
    std::string spelled_name = node->name;
    const size_t scope_sep = spelled_name.rfind("::");
    if (scope_sep == std::string::npos) return;
    std::string qualified_primary =
        spelled_name.substr(0, scope_sep + 2) + primary_name;
    template_struct_specializations_[qualified_primary].push_back(node);
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
                    if (tok.lexeme == pn) {
                        *out = pts;
                        return true;
                    }
                }
                auto tit = typedef_types_.find(tok.lexeme);
                if (tit != typedef_types_.end()) {
                    *out = tit->second;
                    return true;
                }
                if (parse_mangled_type_suffix(tok.lexeme, out)) return true;

                auto init_tag_type = [&](TypeBase base, const std::string& prefix) {
                    out->array_size = -1;
                    out->inner_rank = -1;
                    out->base = base;
                    out->tag = arena_.strdup(tok.lexeme.substr(prefix.size()).c_str());
                    out->ptr_level = 0;
                    out->is_lvalue_ref = false;
                    out->is_rvalue_ref = false;
                    out->is_const = false;
                    out->is_volatile = false;
                    out->is_fn_ptr = false;
                    out->array_rank = 0;
                };

                if (tok.lexeme.rfind("struct_", 0) == 0) {
                    init_tag_type(TB_STRUCT, "struct_");
                    return true;
                }
                if (tok.lexeme.rfind("union_", 0) == 0) {
                    init_tag_type(TB_UNION, "union_");
                    return true;
                }
                if (tok.lexeme.rfind("enum_", 0) == 0) {
                    init_tag_type(TB_ENUM, "enum_");
                    return true;
                }
            }
        }

        auto init_prefixed_tag_type = [&](const std::string& text, TypeSpec* parsed) -> bool {
            if (!parsed) return false;
            auto init_tag = [&](TypeBase base, size_t prefix_len) {
                *parsed = {};
                parsed->array_size = -1;
                parsed->inner_rank = -1;
                parsed->base = base;
                parsed->tag = arena_.strdup(text.substr(prefix_len).c_str());
                parsed->array_rank = 0;
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
            return false;
        };

        auto try_parse_type_name_from_tokens = [&](TypeSpec* parsed) -> bool {
            if (!parsed) return false;
            ParserSnapshot snapshot = save_state();
            const int saved_pos = pos_;
            auto saved_tokens = std::move(tokens_);

            std::vector<Token> injected_toks;
            injected_toks.reserve(end - start + 1);
            for (size_t i = start; i < end; ++i) {
                injected_toks.push_back(toks[i]);
            }
            Token eof{};
            eof.kind = TokenKind::EndOfFile;
            if (!injected_toks.empty()) {
                eof.file = injected_toks.back().file;
                eof.line = injected_toks.back().line;
                eof.column = injected_toks.back().column;
            }
            injected_toks.push_back(std::move(eof));

            tokens_ = std::move(injected_toks);
            pos_ = 0;
            for (const auto& [pn, pts] : type_bindings) {
                typedef_types_[pn] = pts;
            }

            bool ok = false;
            try {
                *parsed = parse_type_name();
                *parsed = resolve_typedef_chain(*parsed, typedef_types_);
                ok = pos_ > 0 &&
                     pos_ >= static_cast<int>(tokens_.size()) - 1;
            } catch (...) {
                ok = false;
            }

            restore_state(snapshot);
            tokens_ = std::move(saved_tokens);
            pos_ = saved_pos;
            return ok;
        };

        std::string text = capture_template_arg_expr_text(
            toks, static_cast<int>(start), static_cast<int>(end));
        if (init_prefixed_tag_type(text, out)) return true;
        if (try_parse_type_name_from_tokens(out)) return true;
        return parse_builtin_typespec_text(text, out);
    };

    auto eval_builtin_type_trait = [&](long long* val) -> bool {
        if (ti >= toks.size() || toks[ti].kind != TokenKind::Identifier) return false;
        const std::string builtin_name = toks[ti].lexeme;
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
        lhs = resolve_typedef_chain(lhs, typedef_types_);
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
            rhs = resolve_typedef_chain(rhs, typedef_types_);
            ti = arg2_end;
            if (ti >= toks.size() || toks[ti].kind != TokenKind::RParen) {
                ti = saved_ti;
                return false;
            }
            ++ti;
            *val = types_compatible_p(lhs, rhs, typedef_types_) ? 1 : 0;
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
            (void)instantiate_template_struct_via_injected_parse(
                *this, resolved_ref_tpl_name, ref_args, ref_primary->line,
                "template_member_lookup_instantiation");
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

}  // namespace c4c

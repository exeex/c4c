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
    if (!is_primary_template_struct_def(node)) return;
    template_struct_defs_[name] = node;
}

void Parser::register_template_struct_specialization(const char* primary_name, Node* node) {
    if (!primary_name || !primary_name[0] || !node) return;
    template_struct_specializations_[primary_name].push_back(node);
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

            // Token injection: temporarily swap tokens_ to parse injected text.
            // TentativeParseGuard does not snapshot tokens_, so manual
            // save/restore of tokens_ and pos_ is intentionally kept here.
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

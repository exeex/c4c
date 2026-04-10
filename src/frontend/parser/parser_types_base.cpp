// parser_types_base.cpp — base type parsing, skip helpers, parse_base_type
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

bool Parser::is_type_start() const {
    TokenKind k = cur().kind;
    if (k == TokenKind::LBracket &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::LBracket) {
        return true;
    }
    if (is_type_kw(k)) return true;
    if (is_qualifier(k)) return true;
    if (is_storage_class(k)) return true;
    if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval || k == TokenKind::KwExplicit) return true;
    if (k == TokenKind::KwAttribute) return true;  // __attribute__((x)) type cast
    if (k == TokenKind::KwAlignas) return true;
    if (k == TokenKind::KwStaticAssert) return false;
    if (k == TokenKind::KwTypename) return true;
    if (k == TokenKind::Identifier) {
        if (is_concept_name(cur().lexeme)) return false;
        if (starts_with_value_like_template_expr(*this, tokens_, pos_)) return false;
        if (match_floatn_keyword_base(cur().lexeme, nullptr)) return true;
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
        // Keep declaration probes aligned with parse_base_type(): even when
        // template-type registration was lost, `Identifier<...>` should still
        // be allowed to enter type parsing so the unresolved-template fallback
        // can recover declarations such as `holder<T> value`.
        if (is_cpp_mode() &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::Less) return true;
    }
    if (k == TokenKind::ColonColon) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, true)) {
            const QualifiedTypeProbe probe = probe_qualified_type(*this, qn);
            const int after_pos =
                pos_ + (qn.is_global_qualified ? 1 : 0) +
                2 * static_cast<int>(qn.qualifier_segments.size()) + 1;
            const TokenKind trailing_kind =
                after_pos < static_cast<int>(tokens_.size())
                    ? tokens_[after_pos].kind
                    : TokenKind::EndOfFile;
            if (!probe.has_resolved_typedef &&
                probe.has_unresolved_qualified_fallback &&
                trailing_kind == TokenKind::Less &&
                starts_with_value_like_template_expr(*this, tokens_, pos_)) {
                return false;
            }
            if (trailing_kind == TokenKind::LParen &&
                starts_parenthesized_member_pointer_declarator(*this, after_pos)) {
                return true;
            }
            if (can_start_qualified_type_declaration(*this, probe, after_pos,
                                                     trailing_kind))
                return true;
        }
    }
    // C++ qualified type: StructName::TypedefName or ns::ns2::Type
    if (k == TokenKind::Identifier &&
        pos_ + 2 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
        tokens_[pos_ + 2].kind == TokenKind::Identifier) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, false)) {
            const QualifiedTypeProbe probe = probe_qualified_type(*this, qn);
            const int after_pos =
                pos_ + 1 + 2 * static_cast<int>(qn.qualifier_segments.size());
            const TokenKind trailing_kind =
                after_pos < static_cast<int>(tokens_.size())
                    ? tokens_[after_pos].kind
                    : TokenKind::EndOfFile;
            if (!probe.has_resolved_typedef &&
                probe.has_unresolved_qualified_fallback &&
                trailing_kind == TokenKind::Less &&
                starts_with_value_like_template_expr(*this, tokens_, pos_)) {
                return false;
            }
            if (trailing_kind == TokenKind::LParen &&
                starts_parenthesized_member_pointer_declarator(*this, after_pos)) {
                return true;
            }
            if (can_start_qualified_type_declaration(*this, probe, after_pos,
                                                     trailing_kind))
                return true;
        }
    }
    return false;
}

bool Parser::can_start_parameter_type() const {
    if (is_type_start()) return true;
    if (!is_cpp_mode()) return false;

    if (looks_like_unresolved_identifier_type_head(pos_)) return true;

    if (check(TokenKind::Identifier) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::Identifier) {
        return true;
    }

    if (check(TokenKind::Identifier) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        (tokens_[pos_ + 1].kind == TokenKind::Amp ||
         tokens_[pos_ + 1].kind == TokenKind::AmpAmp)) {
        return true;
    }

    if (check(TokenKind::Identifier) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::Less) {
        return true;
    }

    if (!(check(TokenKind::ColonColon) ||
          (check(TokenKind::Identifier) &&
           pos_ + 1 < static_cast<int>(tokens_.size()) &&
           tokens_[pos_ + 1].kind == TokenKind::ColonColon))) {
        return false;
    }

    QualifiedNameRef qn;
    if (!peek_qualified_name(&qn, true)) return false;

    const int after_pos =
        pos_ + (qn.is_global_qualified ? 1 : 0) +
        2 * static_cast<int>(qn.qualifier_segments.size()) + 1;
    return after_pos < static_cast<int>(tokens_.size()) &&
           (tokens_[after_pos].kind == TokenKind::Less ||
            (tokens_[after_pos].kind == TokenKind::LParen &&
             starts_parenthesized_member_pointer_declarator(*this, after_pos)));
}

bool Parser::looks_like_unresolved_identifier_type_head(int pos) const {
    if (!is_cpp_mode()) return false;
    if (pos < 0 || pos >= static_cast<int>(tokens_.size())) return false;
    if (tokens_[pos].kind != TokenKind::Identifier) return false;
    if (is_concept_name(tokens_[pos].lexeme)) return false;

    const int next = pos + 1;
    if (next >= static_cast<int>(tokens_.size())) return false;

    switch (tokens_[next].kind) {
        case TokenKind::Identifier:
        case TokenKind::Amp:
        case TokenKind::AmpAmp:
        case TokenKind::Star:
        case TokenKind::Less:
        case TokenKind::Comma:
        case TokenKind::RParen:
        case TokenKind::Ellipsis:
            return true;
        default:
            break;
    }

    return is_qualifier(tokens_[next].kind);
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
    while (check(TokenKind::KwNoexcept) ||
           (check(TokenKind::Identifier) && cur().lexeme == "noexcept")) {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

void Parser::skip_exception_spec() {
    // noexcept / noexcept(expr)
    while (check(TokenKind::KwNoexcept) ||
           (check(TokenKind::Identifier) && cur().lexeme == "noexcept")) {
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
                auto resolve_struct_like = [&](TypeSpec ts) -> TypeSpec {
                    ts = resolve_typedef_chain(ts, typedef_types_);
                    if (ts.base == TB_TYPEDEF && ts.tag && typedef_types_.count(ts.tag) > 0) {
                        ts = typedef_types_.at(ts.tag);
                    }
                    return ts;
                };
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
                auto apply_template_bindings =
                    [&](TypeSpec member_ts,
                        const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
                        const std::vector<std::pair<std::string, long long>>& nttp_bindings,
                        bool* substituted_type = nullptr) {
                        if (substituted_type) *substituted_type = false;
                        for (const auto& [pname, pts] : type_bindings) {
                            if (member_ts.base == TB_TYPEDEF && member_ts.tag &&
                                std::string(member_ts.tag) == pname) {
                                const bool outer_lref = member_ts.is_lvalue_ref;
                                const bool outer_rref = member_ts.is_rvalue_ref;
                                const bool outer_const = member_ts.is_const;
                                const bool outer_volatile = member_ts.is_volatile;
                                member_ts = pts;
                                member_ts.is_lvalue_ref =
                                    member_ts.is_lvalue_ref || outer_lref;
                                member_ts.is_rvalue_ref =
                                    !member_ts.is_lvalue_ref &&
                                    (member_ts.is_rvalue_ref || outer_rref);
                                member_ts.is_const = member_ts.is_const || outer_const;
                                member_ts.is_volatile =
                                    member_ts.is_volatile || outer_volatile;
                                if (substituted_type) *substituted_type = true;
                                break;
                            }
                        }
                        if (member_ts.array_size_expr &&
                            member_ts.array_size_expr->kind == NK_VAR &&
                            member_ts.array_size_expr->name) {
                            for (const auto& [pname, value] : nttp_bindings) {
                                if (std::string(member_ts.array_size_expr->name) == pname) {
                                    if (member_ts.array_rank > 0) {
                                        member_ts.array_dims[0] = value;
                                        member_ts.array_size = value;
                                    }
                                    member_ts.array_size_expr = nullptr;
                                    break;
                                }
                            }
                        }
                        return member_ts;
                    };
                auto try_selected_specialization_member_typedefs =
                    [&](const Node* owner) -> bool {
                        if (!owner || owner->n_template_args <= 0) return false;
                        const Node* primary_tpl = nullptr;
                        if (owner->template_origin_name && owner->template_origin_name[0]) {
                            primary_tpl = find_template_struct_primary(
                                owner->template_origin_name);
                        } else if (is_primary_template_struct_def(owner)) {
                            primary_tpl = owner;
                        } else if (owner->name && owner->name[0]) {
                            primary_tpl = find_template_struct_primary(owner->name);
                        }
                        if (!primary_tpl) return false;

                        std::vector<ParsedTemplateArg> actual_args;
                        actual_args.reserve(owner->n_template_args);
                        for (int i = 0; i < owner->n_template_args; ++i) {
                            ParsedTemplateArg arg;
                            arg.is_value =
                                owner->template_arg_is_value &&
                                owner->template_arg_is_value[i];
                            if (arg.is_value) {
                                arg.value = owner->template_arg_values[i];
                            } else if (owner->template_arg_types) {
                                arg.type = owner->template_arg_types[i];
                            }
                            actual_args.push_back(arg);
                        }

                        std::vector<std::pair<std::string, TypeSpec>> type_bindings;
                        std::vector<std::pair<std::string, long long>> nttp_bindings;
                        const auto* specializations =
                            find_template_struct_specializations(primary_tpl);
                        const Node* selected = select_template_struct_pattern(
                            actual_args, primary_tpl, specializations,
                            typedef_types_, &type_bindings, &nttp_bindings);
                        if (selected && selected->template_arg_types &&
                            selected->template_arg_is_value) {
                            for (int ai = 0;
                                 ai < selected->n_template_args &&
                                 ai < static_cast<int>(actual_args.size());
                                 ++ai) {
                                if (selected->template_arg_is_value[ai] ||
                                    actual_args[ai].is_value) {
                                    continue;
                                }
                                const TypeSpec pattern_arg =
                                    selected->template_arg_types[ai];
                                if (!(pattern_arg.base == TB_TYPEDEF &&
                                      pattern_arg.tag &&
                                      pattern_arg.tag[0])) {
                                    continue;
                                }
                                bool is_selected_param = false;
                                for (int pi = 0; pi < selected->n_template_params; ++pi) {
                                    const char* param_name =
                                        selected->template_param_names
                                            ? selected->template_param_names[pi]
                                            : nullptr;
                                    if (param_name &&
                                        std::string(pattern_arg.tag) == param_name) {
                                        is_selected_param = true;
                                        break;
                                    }
                                }
                                if (!is_selected_param) continue;
                                bool already_bound = false;
                                for (const auto& [bound_name, _] : type_bindings) {
                                    if (bound_name == pattern_arg.tag) {
                                        already_bound = true;
                                        break;
                                    }
                                }
                                if (!already_bound) {
                                    type_bindings.push_back(
                                        {pattern_arg.tag, actual_args[ai].type});
                                }
                            }
                        }
                        if (!selected || selected->n_member_typedefs <= 0) return false;
                        for (int i = 0; i < selected->n_member_typedefs; ++i) {
                            const char* name = selected->member_typedef_names[i];
                            if (!name || member != name) continue;
                            TypeSpec selected_member_ts =
                                selected->member_typedef_types[i];
                            bool substituted_type = false;
                            *out = apply_template_bindings(
                                selected_member_ts,
                                type_bindings, nttp_bindings, &substituted_type);
                            if (!substituted_type && member == "type" &&
                                type_bindings.size() == 1) {
                                *out = type_bindings.front().second;
                            }
                            return true;
                        }
                        return false;
                    };
                std::string resolved_tag = tag;
                auto typedef_it = typedef_types_.find(tag);
                if (typedef_it != typedef_types_.end()) {
                    TypeSpec resolved = resolve_struct_like(typedef_it->second);
                    if (resolved.tag && resolved.tag[0])
                        resolved_tag = resolved.tag;
                }
                auto def_it = struct_tag_def_map_.find(resolved_tag);
                if (def_it == struct_tag_def_map_.end() || !def_it->second) return false;
                const Node* sdef = def_it->second;
                if (try_selected_specialization_member_typedefs(sdef))
                    return true;
                if (try_node_member_typedefs(sdef))
                    return true;
                if (try_lookup(resolved_tag + "::" + member)) return true;
                for (int bi = 0; bi < sdef->n_bases; ++bi) {
                    TypeSpec base_ts = resolve_struct_like(sdef->base_types[bi]);
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
    auto parse_builtin_transform_type = [&](TypeSpec* out) -> bool {
        if (!out || !check(TokenKind::Identifier)) return false;
        if (cur().lexeme != "__underlying_type") return false;

        consume();
        expect(TokenKind::LParen);
        // Keep transform-type builtins parseable in declaration contexts even
        // when semantic lowering still treats them conservatively.
        *out = parse_type_name();
        expect(TokenKind::RParen);
        return true;
    };
    auto infer_typeof_like_operand_type = [&](TypeSpec* out, bool strip_qual) -> bool {
        if (!out) return false;
        if (check(TokenKind::LParen)) {
            const int saved_pos = pos_;
            consume();
            if (is_type_start()) {
                bool save_c = out->is_const, save_v = out->is_volatile;
                *out = parse_type_name();
                if (check(TokenKind::RParen)) {
                    consume();
                    if (strip_qual) {
                        out->is_const = false;
                        out->is_volatile = false;
                    }
                    out->is_const |= save_c;
                    out->is_volatile |= save_v;
                    return true;
                }
            }
            pos_ = saved_pos;
        }
        if (check(TokenKind::KwNullptr) ||
            (check(TokenKind::Identifier) && cur().lexeme == "nullptr")) {
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
            case TokenKind::KwMutable:
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

            case TokenKind::KwTypename:
            case TokenKind::Identifier:
                {
                    if (parse_builtin_transform_type(&ts)) {
                        base_set = true;
                        done = true;
                        break;
                    }
                    TypeBase floatn_base;
                    if (match_floatn_keyword_base(cur().lexeme, &floatn_base)) {
                        ts.base = floatn_base;
                        base_set = true;
                        consume();
                        done = true;
                        break;
                    }
                }
                if (is_cpp_mode()) {
                    const bool already_have_base =
                        has_signed || has_unsigned || has_short || long_count > 0 ||
                        has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                        has_struct || has_union || has_enum || base_set;
                    const bool simple_unqualified_known_type_head =
                        k == TokenKind::Identifier &&
                        !(pos_ + 1 < static_cast<int>(tokens_.size()) &&
                          tokens_[pos_ + 1].kind == TokenKind::ColonColon) &&
                        (is_typedef_name(cur().lexeme) ||
                         is_template_scope_type_param(cur().lexeme) ||
                         typedef_types_.count(resolve_visible_type_name(cur().lexeme)) > 0);
                    if (!simple_unqualified_known_type_head &&
                        try_parse_cpp_scoped_base_type(already_have_base, &ts)) {
                        has_typedef = true;
                        done = true;
                        break;
                    }
                }
                if (k == TokenKind::KwTypename) {
                    done = true;
                    break;
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
                           looks_like_unresolved_identifier_type_head(pos_)) {
                    // C++ unresolved simple type in a declarator or parameter:
                    // treat identifier spellings such as `Box value`,
                    // `Box& value`, `Box* value`, `Box const& value`, and
                    // unnamed forms like `true_type)` as a type head even if
                    // typedef or injected-class-name registration was lost.
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
                    if (ati_it == alias_template_info_.end()) {
                        const std::string resolved_alias_name =
                            resolve_visible_type_name(tname);
                        if (!resolved_alias_name.empty())
                            ati_it = alias_template_info_.find(resolved_alias_name);
                    }
                    if (ati_it != alias_template_info_.end()) {
                        const AliasTemplateInfo& ati = ati_it->second;
                        const std::string alias_template_name = ati_it->first;
                        TentativeParseGuard alias_guard(*this);
                        std::vector<TemplateArgParseResult> alias_args;
                        auto alias_args_match = [&](const std::vector<TemplateArgParseResult>& args)
                            -> bool {
                            size_t ai = 0;
                            for (size_t pi = 0; pi < ati.param_names.size(); ++pi) {
                                const bool is_pack =
                                    pi < ati.param_is_pack.size() && ati.param_is_pack[pi];
                                const bool expects_value =
                                    pi < ati.param_is_nttp.size() && ati.param_is_nttp[pi];
                                if (is_pack) {
                                    while (ai < args.size()) {
                                        if (args[ai].is_value != expects_value) return false;
                                        ++ai;
                                    }
                                    return true;
                                }
                                if (ai >= args.size()) {
                                    const bool has_default =
                                        pi < ati.param_has_default.size() &&
                                        ati.param_has_default[pi];
                                    if (!has_default) return false;
                                    continue;
                                }
                                if (args[ai].is_value != expects_value) return false;
                                ++ai;
                            }
                            return ai == args.size();
                        };
                        if (!parse_template_argument_list(
                                &alias_args,
                                nullptr,
                                &ati.param_is_nttp) ||
                            !alias_args_match(alias_args)) {
                            // alias_guard restores pos_ on scope exit
                        } else {
                            bool alias_parse_ok = true;
                            std::vector<TemplateArgParseResult> resolved_alias_args =
                                alias_args;
                            for (size_t pi = resolved_alias_args.size();
                                 pi < ati.param_names.size(); ++pi) {
                                const bool is_pack =
                                    pi < ati.param_is_pack.size() && ati.param_is_pack[pi];
                                if (is_pack) break;
                                const bool has_default =
                                    pi < ati.param_has_default.size() &&
                                    ati.param_has_default[pi];
                                if (!has_default) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                TemplateArgParseResult default_arg;
                                default_arg.is_value =
                                    pi < ati.param_is_nttp.size() &&
                                    ati.param_is_nttp[pi];
                                if (default_arg.is_value) {
                                    if (pi >= ati.param_default_values.size() ||
                                        ati.param_default_values[pi] == LLONG_MIN) {
                                        alias_parse_ok = false;
                                        break;
                                    }
                                    default_arg.value = ati.param_default_values[pi];
                                } else {
                                    if (pi >= ati.param_default_types.size()) {
                                        alias_parse_ok = false;
                                        break;
                                    }
                                    default_arg.type = ati.param_default_types[pi];
                                }
                                resolved_alias_args.push_back(default_arg);
                            }
                            auto render_alias_arg_ref =
                                [&](const TemplateArgParseResult& arg) -> std::string {
                                    if (arg.is_value) {
                                        if (arg.nttp_name && arg.nttp_name[0])
                                            return arg.nttp_name;
                                        return std::to_string(arg.value);
                                    }
                                    if (arg.type.tpl_struct_origin) {
                                        std::string ref = "@";
                                        ref += arg.type.tpl_struct_origin;
                                        ref += ":";
                                        ref += arg.type.tpl_struct_arg_refs
                                            ? arg.type.tpl_struct_arg_refs
                                            : "";
                                        if (arg.type.deferred_member_type_name &&
                                            arg.type.deferred_member_type_name[0]) {
                                            ref += "$";
                                            ref += arg.type.deferred_member_type_name;
                                        }
                                        return ref;
                                    }
                                    std::string mangled;
                                    append_type_mangled_suffix(mangled, arg.type);
                                    if (mangled.empty() && arg.type.tag)
                                        return arg.type.tag;
                                    return mangled;
                                };
                            auto build_template_arg_refs =
                                [&](const std::vector<TemplateArgParseResult>& args)
                                -> std::string {
                                    std::string arg_refs;
                                    for (const auto& arg : args) {
                                        if (!arg_refs.empty()) arg_refs += ",";
                                        arg_refs += render_alias_arg_ref(arg);
                                    }
                                    return arg_refs;
                                };

                            std::unordered_map<std::string, std::string> subst;
                            size_t bound_arg_index = 0;
                            for (size_t pi = 0; pi < ati.param_names.size(); ++pi) {
                                const bool is_pack =
                                    pi < ati.param_is_pack.size() && ati.param_is_pack[pi];
                                const bool expects_value =
                                    pi < ati.param_is_nttp.size() && ati.param_is_nttp[pi];
                                if (is_pack) {
                                    std::string packed_refs;
                                    while (bound_arg_index < resolved_alias_args.size()) {
                                        if (resolved_alias_args[bound_arg_index].is_value !=
                                            expects_value) {
                                            alias_parse_ok = false;
                                            break;
                                        }
                                        if (!packed_refs.empty()) packed_refs += ",";
                                        packed_refs +=
                                            render_alias_arg_ref(
                                                resolved_alias_args[bound_arg_index]);
                                        ++bound_arg_index;
                                    }
                                    if (!alias_parse_ok) break;
                                    subst[ati.param_names[pi]] = packed_refs;
                                    continue;
                                }
                                if (bound_arg_index >= resolved_alias_args.size()) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                if (resolved_alias_args[bound_arg_index].is_value !=
                                    expects_value) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                subst[ati.param_names[pi]] =
                                    render_alias_arg_ref(
                                        resolved_alias_args[bound_arg_index]);
                                ++bound_arg_index;
                            }
                            if (alias_parse_ok &&
                                bound_arg_index != resolved_alias_args.size()) {
                                alias_parse_ok = false;
                            }
                            if (!alias_parse_ok) {
                                // alias_guard restores pos_ on scope exit
                            } else {
                            // Substitute alias params in the aliased type.
                                ts = ati.aliased_type;
                                auto split_template_arg_refs =
                                    [&](const std::string& refs)
                                    -> std::vector<std::string> {
                                        std::vector<std::string> parts;
                                        if (refs.empty()) return parts;
                                        size_t start = 0;
                                        int angle_depth = 0;
                                        int paren_depth = 0;
                                        int bracket_depth = 0;
                                        int brace_depth = 0;
                                        for (size_t i = 0; i < refs.size(); ++i) {
                                            const char ch = refs[i];
                                            switch (ch) {
                                                case '<': ++angle_depth; break;
                                                case '>':
                                                    if (angle_depth > 0) --angle_depth;
                                                    break;
                                                case '(':
                                                    ++paren_depth;
                                                    break;
                                                case ')':
                                                    if (paren_depth > 0) --paren_depth;
                                                    break;
                                                case '[':
                                                    ++bracket_depth;
                                                    break;
                                                case ']':
                                                    if (bracket_depth > 0) --bracket_depth;
                                                    break;
                                                case '{':
                                                    ++brace_depth;
                                                    break;
                                                case '}':
                                                    if (brace_depth > 0) --brace_depth;
                                                    break;
                                                case ',':
                                                    if (angle_depth == 0 &&
                                                        paren_depth == 0 &&
                                                        bracket_depth == 0 &&
                                                        brace_depth == 0) {
                                                        parts.push_back(
                                                            refs.substr(start, i - start));
                                                        start = i + 1;
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        parts.push_back(refs.substr(start));
                                        return parts;
                                    };
                                auto substitute_template_arg_refs =
                                    [&](const char* refs_text) -> std::string {
                                        if (!refs_text || !refs_text[0]) return {};
                                        const auto old_parts =
                                            split_template_arg_refs(refs_text);
                                        std::string updated_refs;
                                        bool first_part = true;
                                        for (const auto& old_part : old_parts) {
                                            std::string substituted = old_part;
                                            auto s_it = subst.find(old_part);
                                            if (s_it != subst.end())
                                                substituted = s_it->second;
                                            if (substituted.empty()) continue;
                                            if (!first_part) updated_refs += ",";
                                            updated_refs += substituted;
                                            first_part = false;
                                        }
                                        return updated_refs;
                                    };
                                auto parse_template_arg_ref =
                                    [&](const std::string& ref,
                                        ParsedTemplateArg* out) -> bool {
                                        if (!out || ref.empty()) return false;
                                        ParsedTemplateArg parsed{};
                                        char* end = nullptr;
                                        const long long numeric =
                                            std::strtoll(ref.c_str(), &end, 10);
                                        if (end && *end == '\0') {
                                            parsed.is_value = true;
                                            parsed.value = numeric;
                                            *out = parsed;
                                            return true;
                                        }
                                        parsed.is_value = false;
                                        if (ref[0] == '@') {
                                            const size_t sep = ref.find(':', 1);
                                            if (sep == std::string::npos) return false;
                                            parsed.type.array_size = -1;
                                            parsed.type.inner_rank = -1;
                                            parsed.type.base = TB_STRUCT;
                                            const std::string origin =
                                                ref.substr(1, sep - 1);
                                            const size_t member_sep =
                                                ref.find('$', sep + 1);
                                            const std::string nested_refs =
                                                member_sep == std::string::npos
                                                    ? ref.substr(sep + 1)
                                                    : ref.substr(
                                                          sep + 1,
                                                          member_sep - (sep + 1));
                                            parsed.type.tag =
                                                arena_.strdup(origin.c_str());
                                            parsed.type.tpl_struct_origin =
                                                parsed.type.tag;
                                            parsed.type.tpl_struct_arg_refs =
                                                arena_.strdup(nested_refs.c_str());
                                            if (member_sep != std::string::npos &&
                                                member_sep + 1 < ref.size()) {
                                                parsed.type.deferred_member_type_name =
                                                    arena_.strdup(
                                                        ref.substr(member_sep + 1).c_str());
                                            }
                                            *out = parsed;
                                            return true;
                                        }
                                        auto tit = typedef_types_.find(ref);
                                        if (tit != typedef_types_.end()) {
                                            parsed.type = tit->second;
                                            *out = parsed;
                                            return true;
                                        }
                                        if (parse_mangled_type_suffix(ref, &parsed.type) ||
                                            parse_builtin_typespec_text(ref, &parsed.type)) {
                                            *out = parsed;
                                            return true;
                                        }
                                        parsed.type.array_size = -1;
                                        parsed.type.inner_rank = -1;
                                        parsed.type.base = TB_STRUCT;
                                        parsed.type.tag = arena_.strdup(ref.c_str());
                                        *out = parsed;
                                        return true;
                                    };
                                auto build_transformed_owner_args =
                                    [&](const std::string& owner_name,
                                        std::vector<ParsedTemplateArg>* out_args)
                                    -> bool {
                                        if (!out_args) return false;
                                        out_args->clear();
                                        if (!ts.tpl_struct_origin ||
                                            !ts.tpl_struct_arg_refs ||
                                            owner_name != ts.tpl_struct_origin) {
                                            for (const auto& arg : resolved_alias_args) {
                                                ParsedTemplateArg actual = arg;
                                                out_args->push_back(actual);
                                            }
                                            return true;
                                        }
                                        const std::string updated_refs =
                                            substitute_template_arg_refs(
                                                ts.tpl_struct_arg_refs);
                                        if (updated_refs.empty()) return false;
                                        const auto parts =
                                            split_template_arg_refs(updated_refs);
                                        for (const auto& part : parts) {
                                            ParsedTemplateArg actual{};
                                            if (!parse_template_arg_ref(part, &actual))
                                                return false;
                                            out_args->push_back(actual);
                                        }
                                        return true;
                                    };
                                auto type_mentions_template_scope_param =
                                    [&](const TypeSpec& type) -> bool {
                                        auto text_mentions_template_scope_param =
                                            [&](const char* text) -> bool {
                                                if (!text || !text[0]) return false;
                                                const char* cur = text;
                                                while (*cur) {
                                                    if (std::isalpha(
                                                            static_cast<unsigned char>(*cur)) ||
                                                        *cur == '_') {
                                                        const char* start = cur++;
                                                        while (std::isalnum(
                                                                   static_cast<unsigned char>(*cur)) ||
                                                               *cur == '_') {
                                                            ++cur;
                                                        }
                                                        if (is_template_scope_type_param(
                                                                std::string(
                                                                    start,
                                                                    static_cast<size_t>(cur - start)))) {
                                                            return true;
                                                        }
                                                        continue;
                                                    }
                                                    ++cur;
                                                }
                                                return false;
                                            };
                                        return text_mentions_template_scope_param(type.tag) ||
                                               text_mentions_template_scope_param(
                                                   type.tpl_struct_arg_refs) ||
                                               text_mentions_template_scope_param(
                                                   type.deferred_member_type_name);
                                    };
                                auto has_dependent_owner_args =
                                    [&](const std::string& owner_name) -> bool {
                                        std::vector<ParsedTemplateArg> actual_args;
                                        if (!build_transformed_owner_args(
                                                owner_name, &actual_args)) {
                                            return false;
                                        }
                                        for (const auto& actual : actual_args) {
                                            if (!actual.is_value &&
                                                type_mentions_template_scope_param(
                                                    actual.type)) {
                                                return true;
                                            }
                                        }
                                        return false;
                                    };
                                auto preserve_deferred_alias_member =
                                    [&](const std::string& owner_name,
                                        const std::string& member_name) -> bool {
                                        if (owner_name.empty() || member_name.empty())
                                            return false;
                                        std::vector<ParsedTemplateArg> actual_args;
                                        if (!build_transformed_owner_args(
                                                owner_name, &actual_args)) {
                                            return false;
                                        }
                                        ts.base = TB_STRUCT;
                                        ts.tag = arena_.strdup(owner_name.c_str());
                                        ts.tpl_struct_origin = ts.tag;
                                        const std::string arg_refs =
                                            build_template_arg_refs(actual_args);
                                        ts.tpl_struct_arg_refs =
                                            arena_.strdup(arg_refs.c_str());
                                        ts.deferred_member_type_name =
                                            arena_.strdup(member_name.c_str());
                                        return true;
                                    };
                                auto resolve_alias_member_type =
                                    [&](const std::string& owner_name,
                                        const std::string& member_name) -> bool {
                                        if (owner_name.empty() || member_name.empty())
                                            return false;
                                        const std::string owner_lookup_name =
                                            (ts.tpl_struct_origin &&
                                             ts.deferred_member_type_name &&
                                             member_name == ts.deferred_member_type_name)
                                                ? std::string(ts.tpl_struct_origin)
                                                : owner_name;
                                        const Node* primary_tpl =
                                            find_template_struct_primary(
                                                owner_lookup_name);
                                        if (!primary_tpl) {
                                            const std::string resolved_owner =
                                                resolve_visible_type_name(
                                                    owner_lookup_name);
                                            if (!resolved_owner.empty())
                                                primary_tpl =
                                                    find_template_struct_primary(
                                                        resolved_owner);
                                        }
                                        if (!primary_tpl) {
                                            const std::string canonical_owner =
                                                canonical_name_in_context(
                                                    current_namespace_context_id(),
                                                    owner_lookup_name);
                                            primary_tpl =
                                                find_template_struct_primary(
                                                    canonical_owner);
                                        }
                                        if (!primary_tpl) return false;

                                        std::vector<ParsedTemplateArg> actual_args;
                                        if (!build_transformed_owner_args(
                                                owner_lookup_name, &actual_args))
                                            return false;

                                        std::vector<std::pair<std::string, TypeSpec>> type_bindings;
                                        std::vector<std::pair<std::string, long long>> nttp_bindings;
                                        const std::vector<Node*>* specializations =
                                            find_template_struct_specializations(primary_tpl);
                                        if (!specializations) {
                                            auto it =
                                                template_struct_specializations_.find(
                                                    owner_lookup_name);
                                            if (it != template_struct_specializations_.end())
                                                specializations = &it->second;
                                        }
                                        const Node* selected = select_template_struct_pattern(
                                            actual_args, primary_tpl, specializations,
                                            typedef_types_, &type_bindings, &nttp_bindings);
                                        if (!selected || selected->n_member_typedefs <= 0)
                                            return false;
                                        for (int mi = 0; mi < selected->n_member_typedefs; ++mi) {
                                            const char* member = selected->member_typedef_names[mi];
                                            if (!member || member_name != member) continue;
                                            ts = selected->member_typedef_types[mi];
                                            bool substituted = false;
                                            for (const auto& [pname, pts] : type_bindings) {
                                                if (ts.base == TB_TYPEDEF && ts.tag &&
                                                    std::string(ts.tag) == pname) {
                                                    const int outer_ptr_level =
                                                        ts.ptr_level;
                                                    const bool outer_lref =
                                                        ts.is_lvalue_ref;
                                                    const bool outer_rref =
                                                        ts.is_rvalue_ref;
                                                    const bool outer_const =
                                                        ts.is_const;
                                                    const bool outer_volatile =
                                                        ts.is_volatile;
                                                    ts = pts;
                                                    ts.ptr_level += outer_ptr_level;
                                                    ts.is_lvalue_ref =
                                                        ts.is_lvalue_ref || outer_lref;
                                                    ts.is_rvalue_ref =
                                                        !ts.is_lvalue_ref &&
                                                        (ts.is_rvalue_ref ||
                                                         outer_rref);
                                                    ts.is_const =
                                                        ts.is_const || outer_const;
                                                    ts.is_volatile =
                                                        ts.is_volatile ||
                                                        outer_volatile;
                                                    substituted = true;
                                                    break;
                                                }
                                            }
                                            if (!substituted && ts.base == TB_TYPEDEF &&
                                                ts.tag && member_name == "type" &&
                                                type_bindings.size() == 1) {
                                                ts = type_bindings.front().second;
                                            }
                                            return true;
                                        }
                                        return false;
                                    };

                                bool resolved_alias_member = false;
                                {
                                    std::string owner_name;
                                    std::string member_name;
                                    auto derive_alias_owner_and_member =
                                        [&](const std::string& alias_name)
                                        -> std::pair<std::string, std::string> {
                                            const size_t alias_sep =
                                                alias_name.rfind("::");
                                            std::string alias_base =
                                                alias_sep == std::string::npos
                                                    ? alias_name
                                                    : alias_name.substr(alias_sep + 2);
                                            if (alias_base.size() > 2 &&
                                                alias_base.substr(alias_base.size() - 2) == "_t") {
                                                return {
                                                    alias_sep == std::string::npos
                                                        ? alias_base.substr(0, alias_base.size() - 2)
                                                        : alias_name.substr(0, alias_sep + 2) +
                                                              alias_base.substr(
                                                                  0, alias_base.size() - 2),
                                                    "type"};
                                            }
                                            return {"", ""};
                                        };
                                    auto apply_unary_alias_transform =
                                        [&](const std::string& owner_name,
                                            const std::string& member_name) -> bool {
                                            auto alias_matches_trait_family =
                                                [&](const char* suffix) -> bool {
                                                    const size_t suffix_len =
                                                        std::strlen(suffix);
                                                    if (owner_name == suffix)
                                                        return true;
                                                    if (owner_name.size() <=
                                                        suffix_len + 2) {
                                                        return false;
                                                    }
                                                    return owner_name.compare(
                                                               owner_name.size() - suffix_len,
                                                               suffix_len,
                                                               suffix) == 0 &&
                                                           owner_name.compare(
                                                               owner_name.size() -
                                                                   suffix_len - 2,
                                                               2,
                                                               "::") == 0;
                                                };
                                            if (member_name != "type" ||
                                                resolved_alias_args.size() != 1 ||
                                                resolved_alias_args[0].is_value) {
                                                return false;
                                            }
                                            ts = resolved_alias_args[0].type;
                                            if (alias_matches_trait_family("remove_const")) {
                                                ts.is_const = false;
                                                return true;
                                            }
                                            if (alias_matches_trait_family("remove_volatile")) {
                                                ts.is_volatile = false;
                                                return true;
                                            }
                                            if (alias_matches_trait_family("remove_cv")) {
                                                ts.is_const = false;
                                                ts.is_volatile = false;
                                                return true;
                                            }
                                            if (alias_matches_trait_family("remove_reference")) {
                                                ts.is_lvalue_ref = false;
                                                ts.is_rvalue_ref = false;
                                                return true;
                                            }
                                            return false;
                                        };
                                    if (ts.base == TB_TYPEDEF && ts.tag) {
                                        std::string tag_text = ts.tag;
                                        const size_t member_pos = tag_text.rfind("::");
                                        if (member_pos != std::string::npos) {
                                            owner_name = tag_text.substr(0, member_pos);
                                            member_name = tag_text.substr(member_pos + 2);
                                        }
                                    }
                                    if (!owner_name.empty() && !member_name.empty())
                                        resolved_alias_member =
                                            (has_dependent_owner_args(owner_name) &&
                                             preserve_deferred_alias_member(
                                                 owner_name, member_name)) ||
                                            apply_unary_alias_transform(owner_name, member_name) ||
                                            resolve_alias_member_type(owner_name, member_name);
                                    if (!resolved_alias_member &&
                                        ts.deferred_member_type_name &&
                                        ((ts.tpl_struct_origin &&
                                          ts.tpl_struct_origin[0]) ||
                                         (ts.tag && ts.tag[0]))) {
                                        // Preserve explicit aliased `Owner<...>::type`
                                        // spellings for later HIR resolution instead of
                                        // replacing them with alias-name-derived `_t`
                                        // heuristics.
                                        resolved_alias_member = true;
                                    }
                                    const bool can_try_derived_alias_member =
                                        (ts.base == TB_TYPEDEF && ts.tag) ||
                                        ts.base == TB_STRUCT ||
                                        (!owner_name.empty() && !member_name.empty());
                                    if (!resolved_alias_member &&
                                        can_try_derived_alias_member) {
                                        auto [derived_owner, derived_member] =
                                            derive_alias_owner_and_member(alias_template_name);
                                        if (!derived_owner.empty() && !derived_member.empty())
                                            resolved_alias_member =
                                                (has_dependent_owner_args(
                                                     derived_owner) &&
                                                 preserve_deferred_alias_member(
                                                     derived_owner,
                                                     derived_member)) ||
                                                apply_unary_alias_transform(
                                                    derived_owner, derived_member) ||
                                                resolve_alias_member_type(
                                                    derived_owner, derived_member);
                                        if (!resolved_alias_member &&
                                            !derived_owner.empty() &&
                                            !derived_member.empty()) {
                                            ts.base = TB_STRUCT;
                                            ts.tag =
                                                arena_.strdup(derived_owner.c_str());
                                            ts.tpl_struct_origin = ts.tag;
                                            const std::string arg_refs =
                                                build_template_arg_refs(
                                                    resolved_alias_args);
                                            ts.tpl_struct_arg_refs =
                                                arena_.strdup(arg_refs.c_str());
                                            ts.deferred_member_type_name =
                                                arena_.strdup(derived_member.c_str());
                                            resolved_alias_member = true;
                                        }
                                    }
                                }
                                if (!resolved_alias_member) {
                                    for (size_t ai = 0;
                                         ai < resolved_alias_args.size(); ++ai) {
                                        if (ati.param_is_nttp[ai]) continue;
                                        if (ts.base == TB_TYPEDEF && ts.tag &&
                                            std::string(ts.tag) == ati.param_names[ai]) {
                                            const bool outer_lref = ts.is_lvalue_ref;
                                            const bool outer_rref = ts.is_rvalue_ref;
                                            const bool outer_const = ts.is_const;
                                            const bool outer_volatile = ts.is_volatile;
                                            ts = resolved_alias_args[ai].type;
                                            ts.is_lvalue_ref = ts.is_lvalue_ref || outer_lref;
                                            ts.is_rvalue_ref =
                                                !ts.is_lvalue_ref &&
                                                (ts.is_rvalue_ref || outer_rref);
                                            ts.is_const = ts.is_const || outer_const;
                                            ts.is_volatile = ts.is_volatile || outer_volatile;
                                            break;
                                        }
                                    }
                                }
                                if (ts.tpl_struct_arg_refs) {
                                    const std::string new_refs =
                                        substitute_template_arg_refs(
                                            ts.tpl_struct_arg_refs);
                                    ts.tpl_struct_arg_refs = arena_.strdup(new_refs.c_str());
                                    // Update tag (mangled name) to reflect substituted args.
                                    if (ts.tpl_struct_origin) {
                                        std::string new_tag = ts.tpl_struct_origin;
                                        for (const auto& p2 :
                                             split_template_arg_refs(new_refs)) {
                                        new_tag += "_";
                                        new_tag += p2;
                                        }
                                        ts.tag = arena_.strdup(new_tag.c_str());
                                    }
                                }
                                ts.is_const   |= save_const;
                                ts.is_volatile |= save_vol;
                                alias_guard.commit();
                                return ts;
                            }
                    }
                }
            } else if (is_cpp_mode() && find_template_struct_primary(tname)) {
                // Qualified names such as `ns::Template<T>` may resolve directly
                // to a template primary without first passing through typedef
                // registration. Reuse the existing template-struct
                // instantiation/member-suffix path below.
                ts.base = TB_STRUCT;
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
                    for (auto& arg : actual_args) {
                        if (arg.is_value) continue;
                        TypeSpec& arg_ts = arg.type;
                        if (!arg_ts.deferred_member_type_name ||
                            !arg_ts.tag || !arg_ts.tag[0]) {
                            continue;
                        }
                        TypeSpec resolved_member{};
                        if (!lookup_struct_member_typedef_recursive(
                                arg_ts.tag,
                                arg_ts.deferred_member_type_name,
                                &resolved_member)) {
                            continue;
                        }
                        resolved_member.deferred_member_type_name = nullptr;
                        arg_ts = resolved_member;
                    }
                    bool has_pack_param = false;
                    if (primary_tpl && primary_tpl->template_param_is_pack) {
                        for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
                            if (primary_tpl->template_param_is_pack[pi]) {
                                has_pack_param = true;
                                break;
                            }
                        }
                    }
                    if (primary_tpl && has_pack_param) {
                        std::string arg_refs;
                        for (const auto& arg : actual_args) {
                            if (!arg_refs.empty()) arg_refs += ",";
                            if (arg.is_value) {
                                if (arg.nttp_name && arg.nttp_name[0]) arg_refs += arg.nttp_name;
                                else arg_refs += std::to_string(arg.value);
                            } else if (arg.type.tpl_struct_origin) {
                                arg_refs += "@";
                                arg_refs += arg.type.tpl_struct_origin;
                                arg_refs += ":";
                                arg_refs += arg.type.tpl_struct_arg_refs ? arg.type.tpl_struct_arg_refs : "";
                                if (arg.type.deferred_member_type_name &&
                                    arg.type.deferred_member_type_name[0]) {
                                    arg_refs += "$";
                                    arg_refs += arg.type.deferred_member_type_name;
                                }
                            } else if (arg.type.tag) {
                                arg_refs += arg.type.tag;
                            } else {
                                std::string type_name;
                                append_type_mangled_suffix(type_name, arg.type);
                                arg_refs += type_name.empty() ? "?" : type_name;
                            }
                        }
                        ts.tpl_struct_origin = arena_.strdup(tpl_name.c_str());
                        ts.tpl_struct_arg_refs = arena_.strdup(arg_refs.c_str());
                        ts.tag = arena_.strdup(tpl_name.c_str());
                        return ts;
                    }
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
                    int mangled_arg_index = 0;
                    for (int pi = 0; primary_tpl && pi < primary_tpl->n_template_params; ++pi) {
                        mangled += "_";
                        mangled += primary_tpl->template_param_names[pi];
                        const bool is_pack =
                            primary_tpl->template_param_is_pack &&
                            primary_tpl->template_param_is_pack[pi];
                        if (is_pack) {
                            while (mangled_arg_index < static_cast<int>(concrete_args.size())) {
                                mangled += "_";
                                if (concrete_args[mangled_arg_index].is_value) {
                                    mangled += std::to_string(
                                        concrete_args[mangled_arg_index].value);
                                } else {
                                    append_type_mangled_suffix(
                                        mangled,
                                        concrete_args[mangled_arg_index].type);
                                }
                                ++mangled_arg_index;
                            }
                            continue;
                        }
                        mangled += "_";
                        if (mangled_arg_index < static_cast<int>(concrete_args.size()) &&
                            concrete_args[mangled_arg_index].is_value) {
                            mangled +=
                                std::to_string(concrete_args[mangled_arg_index].value);
                        } else {
                            if (mangled_arg_index < static_cast<int>(concrete_args.size()) &&
                                !concrete_args[mangled_arg_index].is_value) {
                                append_type_mangled_suffix(
                                    mangled,
                                    concrete_args[mangled_arg_index].type);
                            } else {
                                mangled +=
                                    primary_tpl->template_param_is_nttp[pi] ? "0" : "T";
                            }
                        }
                        ++mangled_arg_index;
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
                                        if (ats.deferred_member_type_name &&
                                            ats.deferred_member_type_name[0]) {
                                            arg_refs += "$";
                                            arg_refs += ats.deferred_member_type_name;
                                        }
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

                                            // Token injection: temporarily swap tokens_ to parse injected text.
                                            // TentativeParseGuard does not snapshot tokens_, so manual
                                            // save/restore of tokens_ and pos_ is intentionally kept here.
                                            int saved_pos = pos_;
                                            const std::string injected_detail =
                                                "reason=template_base_instantiation saved_pos=" +
                                                std::to_string(saved_pos) +
                                                " token_count=" +
                                                std::to_string(inject_toks.size());
                                            auto saved_toks = std::move(tokens_);
                                            tokens_ = std::move(inject_toks);
                                            pos_ = 0;
                                            note_parse_debug_event_for("injected_parse_begin",
                                                                       "parse_base_type",
                                                                       injected_detail.c_str());
                                            try {
                                                TypeSpec resolved_base = parse_base_type();
                                                inst->base_types[bi] = resolved_base;
                                            } catch (...) {}
                                            note_parse_debug_event_for("injected_parse_end",
                                                                       "parse_base_type",
                                                                       injected_detail.c_str());
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
                                if (tpl_def->template_arg_types &&
                                    tpl_def->template_arg_is_value) {
                                    for (int ai = 0;
                                         ai < tpl_def->n_template_args &&
                                         ai < static_cast<int>(actual_args.size());
                                         ++ai) {
                                        if (tpl_def->template_arg_is_value[ai] ||
                                            actual_args[ai].is_value) {
                                            continue;
                                        }
                                        const TypeSpec pattern_arg =
                                            tpl_def->template_arg_types[ai];
                                        if (!(member_ts.base == TB_TYPEDEF &&
                                              member_ts.tag &&
                                              pattern_arg.base == TB_TYPEDEF &&
                                              pattern_arg.tag &&
                                              std::string(member_ts.tag) ==
                                                  pattern_arg.tag)) {
                                            continue;
                                        }
                                        member_ts = actual_args[ai].type;
                                        break;
                                    }
                                }
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (member_ts.base == TB_TYPEDEF && member_ts.tag &&
                                        std::string(member_ts.tag) == pname) {
                                        const bool outer_lref = member_ts.is_lvalue_ref;
                                        const bool outer_rref = member_ts.is_rvalue_ref;
                                        member_ts.base = pts.base;
                                        member_ts.tag = pts.tag;
                                        member_ts.ptr_level += pts.ptr_level;
                                        member_ts.is_lvalue_ref =
                                            pts.is_lvalue_ref || outer_lref;
                                        member_ts.is_rvalue_ref =
                                            !member_ts.is_lvalue_ref &&
                                            (pts.is_rvalue_ref || outer_rref);
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
                                new_m->unqualified_name = orig_m->unqualified_name;
                                new_m->template_origin_name = orig_m->template_origin_name;
                                new_m->execution_domain = orig_m->execution_domain;
                                new_m->variadic = orig_m->variadic;
                                new_m->is_static = orig_m->is_static;
                                new_m->is_inline = orig_m->is_inline;
                                new_m->is_constexpr = orig_m->is_constexpr;
                                new_m->is_consteval = orig_m->is_consteval;
                                new_m->is_const_method = orig_m->is_const_method;
                                new_m->is_lvalue_ref_method = orig_m->is_lvalue_ref_method;
                                new_m->is_rvalue_ref_method = orig_m->is_rvalue_ref_method;
                                new_m->is_constructor = orig_m->is_constructor;
                                new_m->is_destructor = orig_m->is_destructor;
                                new_m->is_deleted = orig_m->is_deleted;
                                new_m->is_defaulted = orig_m->is_defaulted;
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
                                new_m->n_ctor_inits = orig_m->n_ctor_inits;
                                new_m->ctor_init_names = orig_m->ctor_init_names;
                                new_m->ctor_init_args = orig_m->ctor_init_args;
                                new_m->ctor_init_nargs = orig_m->ctor_init_nargs;
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
                    const bool should_preserve_deferred_template_member =
                        ts.tpl_struct_origin && ts.tpl_struct_arg_refs &&
                        ts.tpl_struct_arg_refs[0] && member == "type";
                    if (should_preserve_deferred_template_member) {
                        consume(); // ::
                        consume(); // member
                        ts.deferred_member_type_name = arena_.strdup(member.c_str());
                        return ts;
                    }
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

}  // namespace c4c

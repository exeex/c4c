// declarator.cpp — declarator parsing, template argument parsing
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

bool Parser::is_typedef_name(TextId name_text_id) const {
    if (has_typedef_name(name_text_id)) return true;
    return find_local_visible_typedef_type(name_text_id) != nullptr;
}

void Parser::push_template_scope(TemplateScopeKind kind,
                                 const std::vector<TemplateScopeParam>& params) {
    template_state_.template_scope_stack.push_back({kind, params});
}

void Parser::pop_template_scope() {
    if (!template_state_.template_scope_stack.empty())
        template_state_.template_scope_stack.pop_back();
}

bool Parser::is_template_scope_type_param(TextId name_text_id) const {
    if (name_text_id == kInvalidText) return false;
    // Walk from innermost scope to outermost.
    for (int i = static_cast<int>(template_state_.template_scope_stack.size()) - 1;
         i >= 0; --i) {
        for (const auto& p : template_state_.template_scope_stack[i].params) {
            if (p.is_nttp) continue;
            if (p.name_text_id == name_text_id) return true;
        }
    }
    return false;
}

bool Parser::parse_next_template_argument(std::vector<TemplateArgParseResult>* out_args,
                                          const Node* primary_tpl, int arg_idx,
                                          bool* out_has_more,
                                          const std::vector<bool>* explicit_param_is_nttp) {
    ParseContextGuard trace(this, __func__);
    if (!out_args || !out_has_more) return false;

    TemplateArgParseResult arg;
    bool ok = false;
    // Normalized disambiguation order (Clang ParseTemplateArgument style):
    //   1. try type-id
    //   2. try non-type expression
    // The primary_tpl hint (expect_value) is used only to prefer
    // non-type for tokens that are unambiguously value-like, avoiding
    // a redundant (and exception-throwing) type parse attempt.
    if (!is_clearly_value_template_arg(primary_tpl, arg_idx,
                                       explicit_param_is_nttp))
        ok = try_parse_template_type_arg(&arg);
    if (!ok) ok = try_parse_template_non_type_arg(&arg);
    if (!ok) return false;

    out_args->push_back(arg);
    *out_has_more = match(TokenKind::Comma);
    return true;
}

bool Parser::try_parse_template_type_arg(TemplateArgParseResult* out_arg) {
    ParseContextGuard trace(this, __func__);
    if (!out_arg) return false;
    const int start_pos = core_input_state_.pos;
    const auto starts_template_id_type_head = [&]() -> bool {
        int probe = core_input_state_.pos;
        if (probe >= static_cast<int>(core_input_state_.tokens.size())) return false;
        if (core_input_state_.tokens[probe].kind == TokenKind::KwTypename) ++probe;
        if (probe < static_cast<int>(core_input_state_.tokens.size()) &&
            core_input_state_.tokens[probe].kind == TokenKind::ColonColon) {
            ++probe;
        }
        if (probe >= static_cast<int>(core_input_state_.tokens.size()) ||
            core_input_state_.tokens[probe].kind != TokenKind::Identifier) {
            return false;
        }
        ++probe;
        while (probe + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[probe].kind == TokenKind::ColonColon &&
               core_input_state_.tokens[probe + 1].kind == TokenKind::Identifier) {
            probe += 2;
        }
        return probe < static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[probe].kind == TokenKind::Less;
    };

    const auto is_simple_known_template_type_head = [&]() -> bool {
        if (check(TokenKind::KwTypename)) return true;
        if (check(TokenKind::ColonColon) || check(TokenKind::Identifier)) {
            QualifiedNameRef qn;
            if (peek_qualified_name(&qn, /*allow_global=*/true) &&
                (qn.is_global_qualified || !qn.qualifier_segments.empty())) {
                const QualifiedTypeProbe probe = probe_qualified_type(*this, qn);
                if (has_qualified_type_parse_fallback(probe))
                    return true;
            }
        }
        if (!check(TokenKind::Identifier)) return false;
        const TextId name_text_id = cur().text_id;
        const std::string_view name = token_spelling(cur());
        return is_known_simple_type_head(*this, name_text_id, name);
    };

    if (is_simple_known_template_type_head() &&
        !starts_template_id_type_head()) {
        TentativeParseGuard fast_guard(*this);
        try {
            TypeSpec ts = parse_base_type();
            if (is_cpp_mode() && check(TokenKind::Ellipsis)) consume();
            if (check(TokenKind::Comma) || check_template_close()) {
                out_arg->is_value = false;
                out_arg->type = ts;
                out_arg->value = 0;
                out_arg->nttp_name = nullptr;
                out_arg->expr = nullptr;
                fast_guard.commit();
                return true;
            }
        } catch (...) {
            // Fall back to the full type-name parser below.
        }
    }

    TentativeParseGuard guard(*this);
    try {
        TypeSpec ts = parse_type_name();
        if (is_cpp_mode() && check(TokenKind::LParen)) skip_paren_group();
        if (is_cpp_mode() && check(TokenKind::Ellipsis)) consume();
        if (!check(TokenKind::Comma) && !check_template_close()) {
            return false;
        }
        // Reject phantom type-id: if exactly one Identifier token was consumed
        // and that identifier is not a recognized type name, then parse_type_name()
        // fell through to the default TB_INT base with the identifier consumed as
        // a declarator name. This happens for forwarded NTTP names like <N>
        // where N is not a type. Reject so non-type parsing can handle it.
        if (is_cpp_mode() && core_input_state_.pos == start_pos + 1 &&
            core_input_state_.tokens[start_pos].kind == TokenKind::Identifier) {
            const TextId start_text_id = core_input_state_.tokens[start_pos].text_id;
            const std::string_view start_name =
                token_spelling(core_input_state_.tokens[start_pos]);
            if (!is_typedef_name(start_text_id) &&
                !has_visible_typedef_type(start_text_id) &&
                !is_template_scope_type_param(start_text_id)) {
                return false;
            }
        }
        out_arg->is_value = false;
        out_arg->type = ts;
        out_arg->value = 0;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        guard.commit();
        return true;
    } catch (...) {
        return false;
    }
}

bool Parser::capture_template_arg_expr(int expr_start, TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;
    const int expr_end =
        find_template_arg_expr_end(core_input_state_.tokens, expr_start);
    const std::string expr_text = capture_template_arg_expr_text(
        *this, core_input_state_.tokens, expr_start, expr_end);
    if (expr_text.empty()) return false;
    out_arg->is_value = true;
    out_arg->value = 0;
    out_arg->expr = nullptr;
    out_arg->nttp_name = arena_.strdup((std::string("$expr:") + expr_text).c_str());
    pos_ = expr_end;
    return true;
}

bool Parser::try_parse_template_non_type_expr(int expr_start,
                                              TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;

    TentativeParseGuard guard(*this);
    try {
        ++active_context_state_.template_arg_expr_depth;
        Node* expr = parse_assign_expr(*this);
        --active_context_state_.template_arg_expr_depth;
        if (core_input_state_.pos > expr_start &&
            (check(TokenKind::Comma) || check_template_close())) {
            const std::string expr_text = capture_template_arg_expr_text(
                *this, core_input_state_.tokens, expr_start, core_input_state_.pos);
            if (!expr_text.empty()) {
                out_arg->is_value = true;
                out_arg->value = 0;
                out_arg->expr = expr;
                out_arg->nttp_name =
                    arena_.strdup((std::string("$expr:") + expr_text).c_str());
                guard.commit();
                return true;
            }
        }
    } catch (...) {
        if (active_context_state_.template_arg_expr_depth > 0)
            --active_context_state_.template_arg_expr_depth;
    }

    return false;
}

bool Parser::try_parse_template_non_type_arg(TemplateArgParseResult* out_arg) {
    if (!out_arg) return false;
    const int expr_start = core_input_state_.pos;
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
        Node* lit = parse_primary(*this);
        out_arg->is_value = true;
        out_arg->value = lit ? lit->ival * sign : 0;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        return true;
    }
    if (check(TokenKind::IntLit)) {
        out_arg->is_value = true;
        out_arg->value =
            parse_int_lexeme(std::string(token_spelling(cur())).c_str()) * sign;
        out_arg->nttp_name = nullptr;
        out_arg->expr = nullptr;
        consume();
        return true;
    }
    if (check(TokenKind::Identifier)) {
        TentativeParseGuard id_guard(*this);
        const char* name =
            arena_.strdup(std::string(token_spelling(cur())).c_str());
        consume();
        if (check(TokenKind::Comma) || check_template_close()) {
            out_arg->is_value = true;
            out_arg->value = 0;
            out_arg->nttp_name = name;
            out_arg->expr = nullptr;
            id_guard.commit();
            return true;
        }
        // id_guard destructor restores pos_ on scope exit
    } else if (expr_start != core_input_state_.pos) {
        pos_ = expr_start;
    }

    // Prefer a real expression parse before falling back to token capture.
    // This lets `<` participate as an operator inside expressions such as
    // `T(-1) < T(0)` instead of being mistaken for a nested template open.
    if (try_parse_template_non_type_expr(expr_start, out_arg))
        return true;

    return capture_template_arg_expr(expr_start, out_arg);
}

bool Parser::is_clearly_value_template_arg(const Node* primary_tpl, int arg_idx,
                                           const std::vector<bool>* explicit_param_is_nttp) const {
    bool expect_value = false;
    if (explicit_param_is_nttp &&
        arg_idx >= 0 &&
        arg_idx < static_cast<int>(explicit_param_is_nttp->size())) {
        expect_value = (*explicit_param_is_nttp)[arg_idx];
    } else {
        expect_value =
            primary_tpl && arg_idx < primary_tpl->n_template_params &&
            primary_tpl->template_param_is_nttp &&
            primary_tpl->template_param_is_nttp[arg_idx];
    }
    if (check(TokenKind::Identifier)) {
        const TextId name_text_id = cur().text_id;
        const std::string_view name = token_spelling(cur());
        const QualifiedNameKey alias_key = alias_template_key_in_context(
            current_namespace_context_id(), name_text_id);
        if (find_alias_template_info(alias_key) ||
            find_alias_template_info_in_context(current_namespace_context_id(), name_text_id)) {
            return false;
        }
    }
    if (starts_with_value_like_template_expr(
            *this, core_input_state_.tokens, core_input_state_.pos))
        return true;
    return expect_value && (
        check(TokenKind::IntLit) || check(TokenKind::CharLit) ||
        check(TokenKind::KwTrue) || check(TokenKind::KwFalse) ||
        check(TokenKind::Minus) || check(TokenKind::Identifier));
}

bool Parser::parse_template_argument_list(std::vector<TemplateArgParseResult>* out_args,
                                         const Node* primary_tpl,
                                         const std::vector<bool>* explicit_param_is_nttp) {
    if (!out_args || !check(TokenKind::Less)) return false;

    consume();  // <
    int arg_idx = 0;
    while (!at_end() && !check_template_close()) {
        bool has_more = false;
        if (!parse_next_template_argument(out_args, primary_tpl, arg_idx, &has_more,
                                          explicit_param_is_nttp))
            return false;
        ++arg_idx;
        if (!has_more) break;
    }
    return match_template_close();
}

bool Parser::consume_qualified_type_spelling_with_typename(
    bool require_typename, bool allow_global,
    bool consume_final_template_args,
    std::string* out_name,
    QualifiedNameRef* out_qn) {
    ParseContextGuard trace(this, __func__);
    TentativeParseGuardLite guard(*this);
    if (require_typename) {
        if (!is_cpp_mode() || !check(TokenKind::KwTypename))
            return false;
        consume();
    }

    if (consume_qualified_type_spelling(allow_global, consume_final_template_args,
                                        out_name, out_qn)) {
        guard.commit();
        return true;
    }

    return false;
}

bool Parser::consume_qualified_type_spelling(bool allow_global,
                                             bool consume_final_template_args,
                                             std::string* out_name,
                                             QualifiedNameRef* out_qn) {
    ParseContextGuard trace(this, __func__);
    TentativeParseGuardLite guard(*this);
    const int start_pos = guard.snapshot.pos;
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
            pos_ = start_pos;
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
        return false;
    }

    while (true) {
        qn.base_name = token_spelling(cur());
        qn.base_text_id = parser_text_id_for_token(cur().text_id, qn.base_name);
        consume();

        if (consume_final_template_args && !consume_optional_template_args())
            return false;

        if (!check(TokenKind::ColonColon))
            break;

        int lookahead = core_input_state_.pos + 1;
        if (lookahead >= static_cast<int>(core_input_state_.tokens.size()))
            break;
        if (core_input_state_.tokens[lookahead].kind == TokenKind::KwTemplate)
            ++lookahead;
        if (lookahead >= static_cast<int>(core_input_state_.tokens.size()) ||
            core_input_state_.tokens[lookahead].kind != TokenKind::Identifier)
            break;

        qn.qualifier_segments.push_back(qn.base_name);
        qn.qualifier_text_ids.push_back(qn.base_text_id);
        consume(); // ::
        if (check(TokenKind::KwTemplate))
            consume();
    }

    if (out_name) {
        *out_name = qualified_name_text(qn, true);
    }
    populate_qualified_name_symbol_ids(&qn);
    if (out_qn) *out_qn = std::move(qn);
    guard.commit();
    return true;
}

bool Parser::consume_template_parameter_type_start(bool allow_typename_keyword) {
    if (allow_typename_keyword && check(TokenKind::KwTypename)) {
        TentativeParseGuard guard(*this);
        consume();
        if (consume_qualified_type_spelling(/*allow_global=*/true,
                                            /*consume_final_template_args=*/true,
                                            nullptr, nullptr)) {
            guard.commit();
            return true;
        }
        // guard restores pos_ on scope exit
    }

    if (is_type_kw(cur().kind)) {
        while (!at_end() && is_type_kw(cur().kind)) consume();
        return true;
    }

    if (consume_qualified_type_spelling(/*allow_global=*/true,
                                        /*consume_final_template_args=*/true,
                                        nullptr, nullptr)) {
        return true;
    }

    return false;
}

bool Parser::consume_template_args_before_scope() {
    if (!check(TokenKind::Less)) return false;

    int probe = core_input_state_.pos;
    int depth = 0;
    bool balanced = false;
    while (probe < static_cast<int>(core_input_state_.tokens.size())) {
        const TokenKind k = core_input_state_.tokens[probe].kind;
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

    if (!balanced || probe >= static_cast<int>(core_input_state_.tokens.size()) ||
        core_input_state_.tokens[probe].kind != TokenKind::ColonColon) {
        return false;
    }

    pos_ = probe;
    return true;
}

bool Parser::consume_member_pointer_owner_prefix() {
    if (!is_cpp_mode()) return false;

    TentativeParseGuard guard(*this);
    if (check(TokenKind::KwTypename)) consume();
    if (!consume_qualified_type_spelling(/*allow_global=*/true,
                                         /*consume_final_template_args=*/true,
                                         nullptr, nullptr)) {
        return false;
    }
    if (!(check(TokenKind::ColonColon) &&
          core_input_state_.pos + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
          core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::Star)) {
        return false;
    }

    consume(); // ::
    consume(); // *
    guard.commit();
    return true;
}

bool try_parse_declarator_member_pointer_prefix(Parser& parser, TypeSpec& ts) {
    if (!parser.is_cpp_mode()) return false;

    if (!parser.consume_member_pointer_owner_prefix()) {
        return false;
    }

    ts.ptr_level++;
    return true;
}

void apply_declarator_pointer_token(Parser& parser, TypeSpec& ts,
                                    TokenKind pointer_tok,
                                    bool preserve_array_base) {
    (void)parser;
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

bool Parser::parse_dependent_typename_specifier(std::string* out_name,
                                                TypeSpec* out_type,
                                                bool* out_has_type) {
    ParseContextGuard trace(this, __func__);
    if (out_has_type) *out_has_type = false;
    const int start_pos = core_input_state_.pos;
    std::string dep_name;
    QualifiedNameRef qn;
    if (!consume_qualified_type_spelling_with_typename(
            /*require_typename=*/true,
            /*allow_global=*/true,
            /*consume_final_template_args=*/true,
            &dep_name, &qn)) {
        return false;
    }

    if (out_name || out_type || out_has_type) {
        auto join_token_lexemes = [&](int start, int end) -> std::string {
            std::string out;
            for (int i = start; i < end; ++i) {
                out += token_spelling(core_input_state_.tokens[i]);
            }
            return out;
        };
        const int owner_start =
            (start_pos < core_input_state_.pos &&
             core_input_state_.tokens[start_pos].kind == TokenKind::KwTypename)
                ? (start_pos + 1)
                : start_pos;
        const std::string spelled_name =
            join_token_lexemes(owner_start, core_input_state_.pos);
        std::string resolved = dep_name;
        TypeSpec resolved_type{};
        const TypeSpec* resolved_type_payload = nullptr;
        const int owner_prefix_start = owner_start;
        auto final_owner_scope_pos = [&]() -> int {
            int final_scope_pos = -1;
            for (int i = owner_prefix_start; i + 1 < core_input_state_.pos; ++i) {
                if (core_input_state_.tokens[i].kind == TokenKind::ColonColon &&
                    core_input_state_.tokens[i + 1].kind == TokenKind::Identifier) {
                    final_scope_pos = i;
                }
            }
            return final_scope_pos;
        };
        auto owner_prefix_contains_typename = [&](int final_scope_pos) -> bool {
            if (final_scope_pos <= owner_prefix_start) return false;
            for (int i = owner_prefix_start; i < final_scope_pos; ++i) {
                if (core_input_state_.tokens[i].kind == TokenKind::KwTypename)
                    return true;
            }
            return false;
        };
        auto try_make_deferred_template_owner_member =
            [&](TypeSpec* out_owner_ts) -> bool {
                if (!out_owner_ts || spelled_name.find('<') == std::string::npos)
                    return false;
                const int final_scope_pos = final_owner_scope_pos();
                if (final_scope_pos <= owner_prefix_start ||
                    owner_prefix_contains_typename(final_scope_pos)) {
                    return false;
                }
                TypeSpec owner_ts{};
                owner_ts.array_size = -1;
                owner_ts.inner_rank = -1;
                std::vector<Token> inject_toks;
                inject_toks.reserve(
                    static_cast<size_t>(final_scope_pos - owner_prefix_start + 1));
                for (int i = owner_prefix_start; i < final_scope_pos; ++i) {
                    inject_toks.push_back(core_input_state_.tokens[i]);
                }
                Token sentinel_seed{};
                if (owner_prefix_start < final_scope_pos) {
                    sentinel_seed = core_input_state_.tokens[owner_prefix_start];
                }
                Token sentinel =
                    make_injected_token(sentinel_seed, TokenKind::Semi, ";");
                inject_toks.push_back(sentinel);

                int saved_pos = pos_;
                auto saved_toks = std::move(tokens_);
                tokens_ = std::move(inject_toks);
                pos_ = 0;
                try {
                    owner_ts = parse_base_type();
                } catch (...) {
                }
                tokens_ = std::move(saved_toks);
                pos_ = saved_pos;

                if (!((owner_ts.tpl_struct_origin &&
                       owner_ts.tpl_struct_origin[0]) ||
                      (owner_ts.tag && owner_ts.tag[0]))) {
                    return false;
                }
                owner_ts.deferred_member_type_name =
                    arena_.strdup(std::string(token_spelling(
                                      core_input_state_.tokens[final_scope_pos + 1]))
                                      .c_str());
                owner_ts.deferred_member_type_text_id =
                    core_input_state_.tokens[final_scope_pos + 1].text_id;
                *out_owner_ts = owner_ts;
                return true;
            };
        const bool is_qualified_typename =
            qn.is_global_qualified || !qn.qualifier_segments.empty();
        auto attach_qualified_typename_metadata = [&](TypeSpec* type) {
            if (!type || !is_qualified_typename) return;
            type->tag_text_id = qn.base_text_id;
            type->is_global_qualified = qn.is_global_qualified;
            type->namespace_context_id = resolve_namespace_context(qn);
            type->n_qualifier_segments =
                static_cast<int>(qn.qualifier_segments.size());
            if (type->n_qualifier_segments <= 0) return;
            type->qualifier_segments =
                arena_.alloc_array<const char*>(type->n_qualifier_segments);
            type->qualifier_text_ids =
                arena_.alloc_array<TextId>(type->n_qualifier_segments);
            for (int i = 0; i < type->n_qualifier_segments; ++i) {
                type->qualifier_segments[i] =
                    arena_.strdup(qn.qualifier_segments[i].c_str());
                type->qualifier_text_ids[i] =
                    i < static_cast<int>(qn.qualifier_text_ids.size())
                        ? qn.qualifier_text_ids[i]
                        : kInvalidText;
            }
        };
        auto write_resolved_type_output = [&]() {
            if (out_type && resolved_type_payload) {
                *out_type = *resolved_type_payload;
                attach_qualified_typename_metadata(out_type);
            }
            if (out_has_type) *out_has_type = resolved_type_payload != nullptr;
        };
        const QualifiedNameKey structured_typedef_key =
            is_qualified_typename ? qualified_name_key(qn) : QualifiedNameKey{};
        auto find_structured_typedef = [&]() -> const TypeSpec* {
            if (structured_typedef_key.base_text_id != kInvalidText) {
                if (const TypeSpec* type =
                        find_typedef_type(structured_typedef_key)) {
                    return type;
                }
            }
            if (qn.qualifier_segments.empty() ||
                qn.base_text_id == kInvalidText) {
                return nullptr;
            }
            const QualifiedNameRef owner_qn = qualified_owner_name(*this, qn);
            if (owner_qn.base_text_id == kInvalidText) return nullptr;
            const int owner_context = resolve_namespace_context(owner_qn);
            if (owner_context < 0) return nullptr;
            const QualifiedNameKey member_key =
                record_member_typedef_key_in_context(
                    owner_context, owner_qn.base_text_id, qn.base_text_id);
            return find_typedef_type(member_key);
        };
        const TypeSpec* structured_typedef = find_structured_typedef();
        const bool has_structured_typedef = structured_typedef != nullptr;
        if (structured_typedef) {
            resolved_type_payload = structured_typedef;
            TypeSpec deferred_owner_member{};
            const bool structured_typedef_needs_owner_handoff =
                structured_typedef->is_lvalue_ref ||
                structured_typedef->is_rvalue_ref ||
                template_state_.template_scope_stack.empty();
            if (structured_typedef_needs_owner_handoff &&
                try_make_deferred_template_owner_member(&deferred_owner_member)) {
                deferred_owner_member.is_lvalue_ref =
                    deferred_owner_member.is_lvalue_ref ||
                    structured_typedef->is_lvalue_ref;
                deferred_owner_member.is_rvalue_ref =
                    !deferred_owner_member.is_lvalue_ref &&
                    (deferred_owner_member.is_rvalue_ref ||
                     structured_typedef->is_rvalue_ref);
                resolved_type = deferred_owner_member;
                resolved_type_payload = &resolved_type;
            }
        }
        const TypeSpec* visible_dep_typedef =
            (!qn.is_global_qualified && qn.qualifier_segments.empty())
                ? find_visible_typedef_type(qn.base_text_id)
                : nullptr;
        if (visible_dep_typedef) {
            resolved_type_payload = visible_dep_typedef;
        }
        if (!visible_dep_typedef &&
            !has_visible_typedef_type(qn.base_text_id)) {
            const VisibleNameResult visible_type =
                resolve_visible_type(qn.base_text_id);
            if (visible_type) {
                resolved = visible_name_spelling(visible_type);
            }
        }
        if (!visible_dep_typedef &&
            !has_structured_typedef &&
            !has_typedef_type(find_parser_text_id(resolved))) {
            bool preserved_template_owner_member = false;
            if (spelled_name.find('<') != std::string::npos &&
                parser_text(qn.base_text_id, qn.base_name) == "type") {
                const int final_scope_pos = final_owner_scope_pos();
                if (final_scope_pos > owner_start) {
                    TypeSpec owner_ts{};
                    owner_ts.array_size = -1;
                    owner_ts.inner_rank = -1;
                    std::vector<Token> inject_toks;
                    inject_toks.reserve(static_cast<size_t>(final_scope_pos - owner_start + 1));
                    for (int i = owner_start; i < final_scope_pos; ++i) {
                        inject_toks.push_back(core_input_state_.tokens[i]);
                    }
                    Token sentinel_seed{};
                    if (owner_start < final_scope_pos) {
                        sentinel_seed = core_input_state_.tokens[owner_start];
                    }
                    Token sentinel =
                        make_injected_token(sentinel_seed, TokenKind::Semi, ";");
                    inject_toks.push_back(sentinel);

                    int saved_pos = pos_;
                    auto saved_toks = std::move(tokens_);
                    tokens_ = std::move(inject_toks);
                    pos_ = 0;
                    try {
                        owner_ts = parse_base_type();
                    } catch (...) {
                    }
                    tokens_ = std::move(saved_toks);
                    pos_ = saved_pos;

                    if ((owner_ts.tpl_struct_origin &&
                         owner_ts.tpl_struct_origin[0]) ||
                        (owner_ts.tag && owner_ts.tag[0])) {
                        owner_ts.deferred_member_type_name =
                            arena_.strdup(std::string(token_spelling(
                                              core_input_state_.tokens[final_scope_pos + 1]))
                                              .c_str());
                        owner_ts.deferred_member_type_text_id =
                            core_input_state_.tokens[final_scope_pos + 1].text_id;
                        if (structured_typedef_key.base_text_id != kInvalidText) {
                            register_structured_typedef_binding(
                                structured_typedef_key, owner_ts);
                        }
                        resolved = spelled_name;
                        resolved_type = owner_ts;
                        resolved_type_payload = &resolved_type;
                        preserved_template_owner_member = true;
                    }
                }
            }
            if (preserved_template_owner_member) {
                if (out_name) *out_name = resolved;
                write_resolved_type_output();
                return true;
            }
            resolved = dep_name;
            if (!has_structured_typedef &&
                !has_typedef_type(find_parser_text_id(resolved)) &&
                !qn.qualifier_segments.empty()) {
                auto follow_nested_owner =
                    [&](const QualifiedNameRef& owner_name) -> const Node* {
                    if (owner_name.qualifier_segments.empty()) return nullptr;

                    auto owner_segment_text_id = [&](size_t index) -> TextId {
                        if (index < owner_name.qualifier_text_ids.size() &&
                            owner_name.qualifier_text_ids[index] != kInvalidText) {
                            return owner_name.qualifier_text_ids[index];
                        }
                        if (index >= owner_name.qualifier_segments.size()) {
                            return kInvalidText;
                        }
                        return parser_text_id_for_token(
                            kInvalidText, owner_name.qualifier_segments[index]);
                    };

                    auto owner_segment_text = [&](size_t index) -> std::string_view {
                        if (index >= owner_name.qualifier_segments.size()) return {};
                        return parser_text(owner_segment_text_id(index),
                                           owner_name.qualifier_segments[index]);
                    };

                    for (size_t owner_start = 0;
                         owner_start < owner_name.qualifier_segments.size();
                         ++owner_start) {
                        QualifiedNameRef owner_qn;
                        owner_qn.is_global_qualified =
                            owner_name.is_global_qualified;
                        owner_qn.qualifier_segments.assign(
                            owner_name.qualifier_segments.begin(),
                            owner_name.qualifier_segments.begin() + owner_start);
                        owner_qn.qualifier_text_ids.reserve(owner_start);
                        for (size_t i = 0; i < owner_start; ++i) {
                            owner_qn.qualifier_text_ids.push_back(
                                owner_segment_text_id(i));
                        }
                        owner_qn.base_name =
                            std::string(owner_segment_text(owner_start));
                        owner_qn.base_text_id = owner_segment_text_id(owner_start);
                        const Node* owner =
                            qualified_type_record_definition_from_structured_authority(
                                *this, owner_qn);
                        if (!owner)
                            continue;

                        bool ok = true;
                        for (size_t i = owner_start + 1;
                             i < owner_name.qualifier_segments.size(); ++i) {
                            const Node* nested_decl = nullptr;
                            const Node* nested_owner = nullptr;
                            const TextId segment_text_id = owner_segment_text_id(i);
                            const std::string_view segment_text =
                                owner_segment_text(i);
                            auto matches_owner_segment = [&](const Node* candidate) -> bool {
                                if (!candidate) return false;
                                const std::string_view candidate_unqualified =
                                    parser_text(candidate->unqualified_text_id,
                                                candidate->unqualified_name
                                                    ? candidate->unqualified_name
                                                    : "");
                                if (!candidate_unqualified.empty() &&
                                    segment_text == candidate_unqualified) {
                                    return true;
                                }
                                if (candidate->name && candidate->name[0] &&
                                    segment_text == candidate->name) {
                                    return true;
                                }
                                if (candidate->n_qualifier_segments > 0 &&
                                    candidate->qualifier_segments &&
                                    candidate->unqualified_name &&
                                    segment_text ==
                                        parser_text(candidate->unqualified_text_id,
                                                    candidate->unqualified_name)) {
                                    return true;
                                }
                                if (candidate->name) {
                                    const char* tail = std::strrchr(candidate->name, ':');
                                    if (tail && tail[1] &&
                                        segment_text ==
                                            parser_text(segment_text_id, tail + 1)) {
                                        return true;
                                    }
                                }
                                return false;
                            };
                            for (int fi = 0; fi < owner->n_fields; ++fi) {
                                const Node* field = owner->fields[fi];
                                if (!field || field->kind != NK_DECL ||
                                    !matches_owner_segment(field)) {
                                    continue;
                                }
                                nested_decl = field;
                                break;
                            }
                            if (!nested_decl) {
                                for (int ci = 0; ci < owner->n_children; ++ci) {
                                    const Node* child = owner->children[ci];
                                    if (!child || child->kind != NK_STRUCT_DEF ||
                                        !matches_owner_segment(child)) {
                                        continue;
                                    }
                                    nested_owner = child;
                                    break;
                                }
                            }
                            if (nested_owner) {
                                owner = nested_owner;
                                continue;
                            }
                            if (!nested_decl) {
                                ok = false;
                                break;
                            }
                            const Node* resolved_nested_owner =
                                resolve_record_type_spec(nested_decl->type, nullptr);
                            if (!resolved_nested_owner) {
                                ok = false;
                                break;
                            }
                            owner = resolved_nested_owner;
                        }
                        if (ok) return owner;
                    }
                    return nullptr;
                };

                TypeSpec resolved_member{};
                if (const Node* owner = follow_nested_owner(qn)) {
                    bool found_member = false;
                    for (int i = 0; !found_member && i < owner->n_member_typedefs; ++i) {
                        const char* name = owner->member_typedef_names[i];
                        if (!name ||
                            parser_text(qn.base_text_id, qn.base_name) != name)
                            continue;
                        resolved_member = owner->member_typedef_types[i];
                        found_member = true;
                    }
                    if (found_member) {
                        if (structured_typedef_key.base_text_id != kInvalidText) {
                            register_structured_typedef_binding(
                                structured_typedef_key, resolved_member);
                        }
                        if (resolved_member.is_lvalue_ref ||
                            resolved_member.is_rvalue_ref) {
                            resolved_type = {};
                            resolved_type.array_size = -1;
                            resolved_type.inner_rank = -1;
                            resolved_type.base = TB_TYPEDEF;
                            resolved_type.tag =
                                arena_.strdup(dep_name.c_str());
                            resolved_type.is_lvalue_ref =
                                resolved_member.is_lvalue_ref;
                            resolved_type.is_rvalue_ref =
                                !resolved_type.is_lvalue_ref &&
                                resolved_member.is_rvalue_ref;
                        } else {
                            resolved_type = resolved_member;
                        }
                        resolved_type_payload = &resolved_type;
                        resolved = dep_name;
                    }
                }
            }
        }
        if (out_name) *out_name = resolved;
        write_resolved_type_output();
    }
    return true;
}

bool try_parse_cpp_scoped_base_type(Parser& parser, bool already_have_base,
                                    TypeSpec* out_ts) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (!out_ts || !parser.is_cpp_mode()) return false;

    if (parser.check(TokenKind::KwTypename)) {
        std::string resolved;
        TypeSpec resolved_type{};
        bool has_resolved_type = false;
        if (!parser.parse_dependent_typename_specifier(&resolved,
                                                       &resolved_type,
                                                       &has_resolved_type))
            return false;
        if (has_resolved_type) {
            *out_ts = resolved_type;
            return true;
        }
        out_ts->tag = parser.arena_.strdup(resolved.c_str());
        return true;
    }

    if (already_have_base) return false;
    return try_parse_qualified_base_type(parser, out_ts);
}

bool try_parse_qualified_base_type(Parser& parser, TypeSpec* out_ts) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (!out_ts || !parser.is_cpp_mode()) return false;

    Parser::QualifiedNameRef qn;
    if (!parser.peek_qualified_name(&qn, true)) return false;

    auto attach_qualified_typespec_metadata = [&]() {
        out_ts->tag_text_id = qn.base_text_id;
        out_ts->is_global_qualified = qn.is_global_qualified;
        out_ts->namespace_context_id = parser.resolve_namespace_context(qn);
        out_ts->n_qualifier_segments =
            static_cast<int>(qn.qualifier_segments.size());
        if (out_ts->n_qualifier_segments > 0) {
            out_ts->qualifier_segments =
                parser.arena_.alloc_array<const char*>(out_ts->n_qualifier_segments);
            out_ts->qualifier_text_ids =
                parser.arena_.alloc_array<TextId>(out_ts->n_qualifier_segments);
            for (int i = 0; i < out_ts->n_qualifier_segments; ++i) {
                out_ts->qualifier_segments[i] =
                    parser.arena_.strdup(qn.qualifier_segments[i].c_str());
                out_ts->qualifier_text_ids[i] =
                    i < static_cast<int>(qn.qualifier_text_ids.size())
                        ? qn.qualifier_text_ids[i]
                        : kInvalidText;
            }
        }
    };

    const QualifiedTypeProbe probe = probe_qualified_type(parser, qn);
    if (probe.has_resolved_typedef) {
        if (probe.record_member_typedef_type) {
            *out_ts = *probe.record_member_typedef_type;
        } else {
            out_ts->tag =
                parser.arena_.strdup(probe.resolved_typedef_name.c_str());
        }
        if (probe.record_def) {
            out_ts->base = probe.record_def->is_union ? TB_UNION : TB_STRUCT;
            out_ts->record_def = probe.record_def;
        }
        attach_qualified_typespec_metadata();
        parser.consume_qualified_type_spelling_with_typename(
            /*require_typename=*/false,
            /*allow_global=*/true,
            /*consume_final_template_args=*/false,
            nullptr, nullptr);
        return true;
    }

    const bool is_qualified =
        qn.is_global_qualified || !qn.qualifier_segments.empty();
    const int after_pos =
        parser.core_input_state_.pos + (qn.is_global_qualified ? 1 : 0) +
        2 * static_cast<int>(qn.qualifier_segments.size()) + 1;
    if (is_qualified &&
        after_pos < static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[after_pos].kind == TokenKind::Less) {
        Parser::TentativeParseGuard guard(parser);
        std::string qualified_name;
        if (!parser.consume_qualified_type_spelling_with_typename(
                /*require_typename=*/false,
                /*allow_global=*/true,
                /*consume_final_template_args=*/false,
                &qualified_name, nullptr)) {
            return false;
        }
        std::vector<Parser::TemplateArgParseResult> parsed_args;
        if (!parser.parse_template_argument_list(&parsed_args)) return false;

        bool has_dependent_type_arg = false;
        for (const Parser::TemplateArgParseResult& arg : parsed_args) {
            if (arg.is_value || arg.type.base != TB_TYPEDEF || !arg.type.tag)
                continue;
            const TextId arg_text_id = parser.find_parser_text_id(arg.type.tag);
            if (parser.is_template_scope_type_param(arg_text_id)) {
                has_dependent_type_arg = true;
                break;
            }
        }
        if (!has_dependent_type_arg) return false;

        out_ts->base = TB_TYPEDEF;
        out_ts->tag = parser.arena_.strdup(qualified_name.c_str());
        out_ts->tag_text_id = qn.base_text_id;
        out_ts->tpl_struct_origin = parser.arena_.strdup(qualified_name.c_str());
        attach_qualified_typespec_metadata();
        guard.commit();
        return true;
    }

    return false;
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
    } else if (check(TokenKind::Spaceship)) {
        consume();
        op_name = "operator_spaceship";
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
    } else if (is_type_start() || can_start_parameter_type() ||
               check(TokenKind::Identifier) ||
               check(TokenKind::ColonColon)) {
        TypeSpec conv_ts{};
        if (check(TokenKind::Identifier) && !is_type_start() &&
            !can_start_parameter_type()) {
            conv_ts.base = TB_TYPEDEF;
            conv_ts.tag = arena_.strdup(std::string(token_spelling(cur())));
            conv_ts.array_size = -1;
            conv_ts.inner_rank = -1;
            consume();
        } else {
            conv_ts = parse_base_type();
        }
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

bool parse_qualified_declarator_name(Parser& parser, std::string* out_name,
                                     TextId* out_name_text_id) {
    if (!out_name) return false;
    if (out_name_text_id) *out_name_text_id = kInvalidText;

    std::string qualified_name;
    bool parsed_qualified = false;
    bool is_unqualified_identifier = false;
    TextId name_text_id = kInvalidText;

    auto append_scope_sep = [&]() {
        if (!qualified_name.empty() &&
            (qualified_name.size() < 2 ||
             qualified_name.substr(qualified_name.size() - 2) != "::")) {
            qualified_name += "::";
        }
    };

    if (parser.match(TokenKind::ColonColon))
        qualified_name = "::";

    if (parser.check(TokenKind::KwOperator)) {
        parsed_qualified =
            parser.parse_operator_declarator_name(&qualified_name);
    } else if (parser.check(TokenKind::Identifier)) {
        name_text_id =
            parser.parser_text_id_for_token(
                parser.cur().text_id, parser.token_spelling(parser.cur()));
        qualified_name += parser.token_spelling(parser.cur());
        parser.consume();
        parsed_qualified = true;
        is_unqualified_identifier = true;
        parser.consume_template_args_before_scope();
    }

    while (parsed_qualified && parser.match(TokenKind::ColonColon)) {
        is_unqualified_identifier = false;
        name_text_id = kInvalidText;
        append_scope_sep();
        if (parser.check(TokenKind::Identifier)) {
            qualified_name += parser.token_spelling(parser.cur());
            parser.consume();
            parser.consume_template_args_before_scope();
            continue;
        }
        if (parser.check(TokenKind::KwOperator)) {
            parsed_qualified =
                parser.parse_operator_declarator_name(&qualified_name);
            break;
        }
        parsed_qualified = false;
        break;
    }

    if (!parsed_qualified) return false;
    *out_name = std::move(qualified_name);
    if (out_name_text_id && is_unqualified_identifier) {
        *out_name_text_id = name_text_id;
    }
    return true;
}

bool is_grouped_declarator_start(Parser& parser) {
    if (!parser.check(TokenKind::LParen)) return false;

    int pk = parser.core_input_state_.pos + 1;
    while (pk < static_cast<int>(parser.core_input_state_.tokens.size()) &&
           parser.core_input_state_.tokens[pk].kind == TokenKind::KwAttribute) {
        ++pk;
        if (pk < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[pk].kind == TokenKind::LParen) {
            int depth = 1;
            ++pk;
            while (pk < static_cast<int>(parser.core_input_state_.tokens.size()) && depth > 0) {
                if (parser.core_input_state_.tokens[pk].kind == TokenKind::LParen) ++depth;
                else if (parser.core_input_state_.tokens[pk].kind == TokenKind::RParen) --depth;
                ++pk;
            }
        }
    }

    bool is_grouped = pk < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                      parser.core_input_state_.tokens[pk].kind == TokenKind::Identifier;
    if (is_grouped && pk + 1 < static_cast<int>(parser.core_input_state_.tokens.size())) {
        const auto next_k = parser.core_input_state_.tokens[pk + 1].kind;
        // Keep parameter-list heads such as `(T&&)` and `(Type<Args>&)` on the
        // surrounding declaration path instead of misrouting them into grouped
        // declarator parsing.
        if (parser.is_cpp_mode() && parser.looks_like_unresolved_identifier_type_head(pk) &&
            (next_k == TokenKind::Amp || next_k == TokenKind::AmpAmp ||
             next_k == TokenKind::Star || next_k == TokenKind::Less ||
             next_k == TokenKind::ColonColon || is_qualifier(next_k))) {
            is_grouped = false;
        }
        if (next_k == TokenKind::Comma || next_k == TokenKind::Ellipsis)
            is_grouped = false;
    }
    return is_grouped;
}

bool is_parenthesized_pointer_declarator_start(Parser& parser) {
    if (!parser.check(TokenKind::LParen)) return false;

    int pk = parser.core_input_state_.pos + 1;
    while (pk < static_cast<int>(parser.core_input_state_.tokens.size()) &&
           parser.core_input_state_.tokens[pk].kind == TokenKind::KwAttribute) {
        ++pk;  // skip __attribute__
        if (pk < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[pk].kind == TokenKind::LParen) {
            int depth = 1;
            ++pk;
            while (pk < static_cast<int>(parser.core_input_state_.tokens.size()) && depth > 0) {
                if (parser.core_input_state_.tokens[pk].kind == TokenKind::LParen) ++depth;
                else if (parser.core_input_state_.tokens[pk].kind == TokenKind::RParen) --depth;
                ++pk;
            }
        }
    }
    if (pk >= static_cast<int>(parser.core_input_state_.tokens.size())) return false;

    const TokenKind lookahead = parser.core_input_state_.tokens[pk].kind;
    if (lookahead == TokenKind::Star || lookahead == TokenKind::Caret ||
        (parser.is_cpp_mode() &&
         (lookahead == TokenKind::Amp || lookahead == TokenKind::AmpAmp))) {
        return true;
    }

    if (!parser.is_cpp_mode() ||
        (lookahead != TokenKind::Identifier &&
         lookahead != TokenKind::ColonColon &&
         lookahead != TokenKind::KwTypename)) {
        return false;
    }

    {
        Parser::TentativeParseGuard probe_guard(parser);
        parser.pos_ = pk;
        const bool is_member_ptr_fn =
            parser.consume_member_pointer_owner_prefix();
        // probe_guard restores pos_ to pre-pk state on scope exit
        return is_member_ptr_fn;
    }
}

void parse_pointer_ref_qualifiers(Parser& parser, TypeSpec& ts,
                                  TokenKind pointer_tok,
                                  bool preserve_array_base,
                                  bool consume_pointer_token) {
    if (consume_pointer_token) parser.consume();
    apply_declarator_pointer_token(parser, ts, pointer_tok, preserve_array_base);
    while (is_qualifier(parser.cur().kind)) parser.consume();
    parser.parse_attributes(&ts);
}

void consume_declarator_post_pointer_qualifiers(Parser& parser) {
    while (parser.check(TokenKind::Identifier)) {
        const std::string_view lex = parser.token_spelling(parser.cur());
        if (lex == "_Nullable" || lex == "_Nonnull" ||
            lex == "_Null_unspecified" || lex == "__nullable" ||
            lex == "__nonnull" || lex == "__restrict" ||
            lex == "restrict" || lex == "const" || lex == "volatile") {
            parser.consume();
        } else {
            break;
        }
    }

    while (is_qualifier(parser.cur().kind)) parser.consume();
}

void parse_declarator_prefix(Parser& parser, TypeSpec& ts,
                             bool* out_is_parameter_pack) {
    if (out_is_parameter_pack) *out_is_parameter_pack = false;

    // Skip qualifiers/attributes that can appear before pointer stars.
    // This handles trailing qualifiers on the base type: e.g. `struct S const *p`
    // where `const` appears after the struct tag but before `*`.
    parser.parse_attributes(&ts);
    while (is_qualifier(parser.cur().kind)) parser.consume();
    parser.parse_attributes(&ts);

    // C++ pointer-to-member declarator prefix: `C::*name` or `ns::C::*name`.
    // Model it as an additional pointer level for parser bring-up so headers
    // like EASTL's invoke_impl(R C::*func, ...) can parse successfully.
    try_parse_declarator_member_pointer_prefix(parser, ts);

    // Count pointer stars (and qualifiers between them).
    while (parser.check(TokenKind::Star)) {
        // If the base type already has array dimensions, this star creates a
        // pointer-to-array (e.g. HARD_REG_SET *p where HARD_REG_SET = ulong[2]).
        parse_pointer_ref_qualifiers(parser, ts, TokenKind::Star,
                                     /*preserve_array_base=*/true);
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::AmpAmp)) {
        parse_pointer_ref_qualifiers(parser, ts, TokenKind::AmpAmp,
                                     /*preserve_array_base=*/false);
    } else if (parser.is_cpp_mode() && parser.check(TokenKind::Amp)) {
        parse_pointer_ref_qualifiers(parser, ts, TokenKind::Amp,
                                     /*preserve_array_base=*/false);
    }

    consume_declarator_post_pointer_qualifiers(parser);

    // C++ template parameter pack declarator: `Args&&... args`
    // or `Ts... value`. The ellipsis belongs to the declarator, not the
    // function's variadic parameter list.
    if (parser.is_cpp_mode() && parser.check(TokenKind::Ellipsis)) {
        parser.consume();
        if (out_is_parameter_pack) *out_is_parameter_pack = true;
    }
    parser.parse_attributes(&ts);
}

bool try_parse_grouped_declarator(Parser& parser, TypeSpec& ts,
                                  const char** out_name,
                                  TextId* out_name_text_id,
                                  std::vector<long long>* out_dims) {
    if (!is_grouped_declarator_start(parser)) return false;

    parser.consume();  // (
    if (out_name && parser.check(TokenKind::Identifier)) {
        if (out_name_text_id) {
            *out_name_text_id =
                parser.parser_text_id_for_token(
                    parser.cur().text_id, parser.token_spelling(parser.cur()));
        }
        *out_name = parser.arena_.strdup(
            std::string(parser.token_spelling(parser.cur())));
        parser.consume();
    }
    parse_declarator_array_suffixes(parser, ts, out_dims);
    parser.expect(TokenKind::RParen);
    parse_declarator_array_suffixes(parser, ts, out_dims);
    return true;
}

void parse_normal_declarator_tail(Parser& parser, TypeSpec& ts,
                                  const char** out_name,
                                  TextId* out_name_text_id,
                                  std::vector<long long>* out_dims) {
    parser.parse_attributes(&ts);

    // Normal declarator: optional identifier name, including C++ qualified names
    // such as `Type::method` or `Type::operator+=`, or free operator functions
    // like `operator==`.
    if (out_name && parser.is_cpp_mode() &&
        (parser.check(TokenKind::Identifier) ||
         parser.check(TokenKind::ColonColon) ||
         parser.check(TokenKind::KwOperator))) {
        Parser::TentativeParseGuard guard(parser);
        std::string qualified_name;
        if (parse_qualified_declarator_name(parser, &qualified_name,
                                            out_name_text_id)) {
            *out_name = parser.arena_.strdup(qualified_name.c_str());
            guard.commit();
        }
        // guard restores pos_ on scope exit if not committed
    }

    if (out_name && !*out_name && parser.check(TokenKind::Identifier)) {
        if (out_name_text_id) {
            *out_name_text_id =
                parser.parser_text_id_for_token(
                    parser.cur().text_id, parser.token_spelling(parser.cur()));
        }
        *out_name = parser.arena_.strdup(
            std::string(parser.token_spelling(parser.cur())));
        parser.consume();
    }

    parser.parse_attributes(&ts);
    parse_declarator_array_suffixes(parser, ts, out_dims);
}

void parse_declarator_parameter_list(
    Parser& parser,
    std::vector<Node*>* out_params, bool* out_variadic) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (out_params) out_params->clear();
    if (out_variadic) *out_variadic = false;

    parser.expect(TokenKind::LParen);
    if (!parser.check(TokenKind::RParen)) {
        while (!parser.at_end()) {
            if (parser.check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                parser.consume();
                break;
            }
            if (parser.check(TokenKind::RParen)) break;
            if (!parser.can_start_parameter_type()) {
                parser.consume();
                if (parser.match(TokenKind::Comma)) continue;
                break;
            }
            Node* p = parse_param(parser);
            if (p && out_params) out_params->push_back(p);
            if (!parser.match(TokenKind::Comma)) break;
        }
    }
    parser.expect(TokenKind::RParen);
}

void parse_parenthesized_function_pointer_suffix(
    Parser& parser,
    TypeSpec& ts, bool is_nested_fn_ptr,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic) {
    if (!parser.check(TokenKind::LParen)) return;

    std::vector<Node*> fn_ptr_params;
    bool fn_ptr_variadic = false;
    parse_declarator_parameter_list(parser, &fn_ptr_params, &fn_ptr_variadic);

    // C++ pointer-to-member-function declarators may carry trailing
    // cv/ref-qualifiers after the parameter list:
    // R (T::*pm)() const, R (T::*pm)() volatile,
    // R (T::*pm)() &, or R (T::*pm)() &&
    while (is_qualifier(parser.cur().kind)) parser.consume();
    if (parser.is_cpp_mode()) {
        if (parser.match(TokenKind::AmpAmp)) {
            // Parse-only: preserve declaration disambiguation even though
            // TypeSpec does not currently model member-function ref-qualifiers.
        } else if (parser.match(TokenKind::Amp)) {
            // Same as above for lvalue-qualified member-function-pointer forms.
        }
    }
    ts.is_fn_ptr = true;  // confirmed function pointer: (*name)(params)

    if (is_nested_fn_ptr) {
        // For nested fn_ptr: inner params already set on out_fn_ptr_params;
        // the outer params here are the RETURN type's fn_ptr params.
        store_declarator_function_pointer_params(
            parser,
            out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
            out_ret_fn_ptr_variadic, fn_ptr_params, fn_ptr_variadic);
        return;
    }

    store_declarator_function_pointer_params(
        parser,
        out_fn_ptr_params, out_n_fn_ptr_params,
        out_fn_ptr_variadic, fn_ptr_params, fn_ptr_variadic);
}

void parse_parenthesized_pointer_declarator_prefix(Parser& parser,
                                                   TypeSpec& ts) {
    parser.consume();  // (
    parser.skip_attributes();  // skip any __attribute__((...)) before * or owner prefix

    TokenKind pointer_tok = parser.cur().kind;
    bool consumed_member_pointer_prefix = false;
    if (parser.is_cpp_mode() && parser.consume_member_pointer_owner_prefix()) {
        consumed_member_pointer_prefix = true;
        pointer_tok = TokenKind::Star;
    }
    parse_pointer_ref_qualifiers(
        parser,
        ts, pointer_tok,
        /*preserve_array_base=*/false,
        /*consume_pointer_token=*/!consumed_member_pointer_prefix);

    while (parser.check(TokenKind::Star)) {
        parse_pointer_ref_qualifiers(parser, ts, TokenKind::Star,
                                     /*preserve_array_base=*/false);
    }

    consume_declarator_post_pointer_qualifiers(parser);

    if (parser.is_cpp_mode()) {
        if (parser.check(TokenKind::AmpAmp)) {
            parser.consume();
            ts.is_rvalue_ref = true;
        } else if (parser.check(TokenKind::Amp)) {
            parser.consume();
            ts.is_lvalue_ref = true;
        }
    }
}

void skip_parenthesized_pointer_declarator_array_chunks(Parser& parser) {
    while (parser.check(TokenKind::LBracket)) {
        parser.consume();  // [
        while (!parser.at_end() && !parser.check(TokenKind::RBracket)) {
            parser.consume();
        }
        if (parser.check(TokenKind::RBracket)) parser.consume();  // ]
    }
}

bool parse_parenthesized_pointer_declarator_name(
    Parser& parser,
    const char** out_name, TextId* out_name_text_id) {
    if (!out_name || !parser.check(TokenKind::Identifier)) return false;

    if (out_name_text_id) {
        *out_name_text_id =
            parser.parser_text_id_for_token(parser.cur().text_id,
                                            parser.token_spelling(parser.cur()));
    }
    *out_name =
        parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
    parser.consume();
    return true;
}

bool try_parse_nested_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic, TextId* out_name_text_id) {
    if (!parser.check(TokenKind::LParen)) return false;

    TypeSpec inner_ts = ts;
    inner_ts.ptr_level = 0;
    parser.parse_declarator(inner_ts, out_name,
                            out_fn_ptr_params, out_n_fn_ptr_params,
                            out_fn_ptr_variadic, nullptr, nullptr, nullptr,
                            nullptr, out_name_text_id);
    return true;
}

bool parse_parenthesized_pointer_declarator_inner(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic, TextId* out_name_text_id) {
    skip_parenthesized_pointer_declarator_array_chunks(parser);
    const bool got_name =
        parse_parenthesized_pointer_declarator_name(parser, out_name,
                                                    out_name_text_id);
    skip_parenthesized_pointer_declarator_array_chunks(parser);

    if (got_name && parser.check(TokenKind::LParen)) {
        parser.skip_paren_group();  // skip own params: (int a, int b)
        return false;
    }

    return try_parse_nested_parenthesized_pointer_declarator(
        parser,
        ts, out_name,
        out_fn_ptr_params, out_n_fn_ptr_params,
        out_fn_ptr_variadic, out_name_text_id);
}

void finalize_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, bool is_nested_fn_ptr, std::vector<long long>* decl_dims,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic) {
    parser.expect(TokenKind::RParen);
    parse_parenthesized_function_pointer_suffix(
        parser,
        ts, is_nested_fn_ptr,
        out_fn_ptr_params, out_n_fn_ptr_params, out_fn_ptr_variadic,
        out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
        out_ret_fn_ptr_variadic);
    parse_declarator_array_suffixes(parser, ts, decl_dims);
    apply_declarator_array_dims(parser, ts, *decl_dims);
    if (!decl_dims->empty()) ts.is_ptr_to_array = true;
}

void parse_parenthesized_pointer_declarator(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    Node*** out_fn_ptr_params, int* out_n_fn_ptr_params,
    bool* out_fn_ptr_variadic,
    Node*** out_ret_fn_ptr_params, int* out_n_ret_fn_ptr_params,
    bool* out_ret_fn_ptr_variadic,
    TextId* out_name_text_id) {
    bool is_nested_fn_ptr = false;
    std::vector<long long> decl_dims;

    parse_parenthesized_pointer_declarator_prefix(parser, ts);

    is_nested_fn_ptr = parse_parenthesized_pointer_declarator_inner(
        parser,
        ts, out_name,
        out_fn_ptr_params, out_n_fn_ptr_params,
        out_fn_ptr_variadic, out_name_text_id);

    finalize_parenthesized_pointer_declarator(
        parser,
        ts, is_nested_fn_ptr, &decl_dims,
        out_fn_ptr_params, out_n_fn_ptr_params, out_fn_ptr_variadic,
        out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
        out_ret_fn_ptr_variadic);
}

void parse_non_parenthesized_declarator(Parser& parser, TypeSpec& ts,
                                        const char** out_name) {
    std::vector<long long> decl_dims;
    parse_non_parenthesized_declarator_suffixes(parser, ts, out_name, nullptr,
                                                &decl_dims);
    apply_declarator_array_dims(parser, ts, decl_dims);
}

void parse_non_parenthesized_declarator_tail(
    Parser& parser,
    TypeSpec& ts, const char** out_name,
    bool decay_plain_function_suffix, TextId* out_name_text_id) {
    std::vector<long long> decl_dims;
    parse_non_parenthesized_declarator_suffixes(
        parser, ts, out_name, out_name_text_id, &decl_dims);
    apply_declarator_array_dims(parser, ts, decl_dims);
    parse_plain_function_declarator_suffix(
        parser, ts, decay_plain_function_suffix);
}

void parse_non_parenthesized_declarator_suffixes(
    Parser& parser, TypeSpec& ts, const char** out_name,
    TextId* out_name_text_id, std::vector<long long>* out_dims) {
    if (try_parse_grouped_declarator(parser, ts, out_name, out_name_text_id,
                                     out_dims)) {
        return;
    }
    parse_normal_declarator_tail(parser, ts, out_name, out_name_text_id,
                                 out_dims);
}

void parse_plain_function_declarator_suffix(
    Parser& parser, TypeSpec& ts, bool decay_to_function_pointer) {
    if (!parser.check(TokenKind::LParen)) return;

    // Plain `name(params)` suffixes stay with the surrounding declaration
    // parser. Parameters are the one place where the caller immediately
    // decays function types to pointer-to-function after the declarator pass.
    if (!decay_to_function_pointer) return;

    parser.skip_paren_group();
    ts.is_fn_ptr = true;
    ts.ptr_level += 1;
}

void store_declarator_function_pointer_params(
    Parser& parser, Node*** out_params, int* out_n_params, bool* out_variadic,
    const std::vector<Node*>& params, bool variadic) {
    if (out_n_params) *out_n_params = static_cast<int>(params.size());
    if (out_variadic) *out_variadic = variadic;
    if (!out_params || params.empty()) return;

    *out_params =
        parser.arena_.alloc_array<Node*>(static_cast<int>(params.size()));
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

long long parse_one_declarator_array_dim(Parser& parser, TypeSpec& ts) {
    parser.expect(TokenKind::LBracket);
    long long dim = -2;  // unsized []
    if (!parser.check(TokenKind::RBracket)) {
        // C99 array parameter qualifiers: [static N], [const N], [volatile N],
        // [restrict N], [const *] (VLA pointer). Skip qualifier keywords.
        while (is_qualifier(parser.cur().kind) ||
               parser.cur().kind == TokenKind::KwStatic)
            parser.consume();
        // [const *] or [static *]: unsized VLA pointer
        if (parser.check(TokenKind::Star)) {
            parser.consume();
            parser.expect(TokenKind::RBracket);
            return -2;
        }
        if (parser.check(TokenKind::RBracket)) {
            parser.expect(TokenKind::RBracket);
            return dim;
        }
        Node* sz = parse_assign_expr(parser);
        ts.array_size_expr = sz;
        long long cv = 0;
        if (sz &&
            eval_const_int(sz, &cv,
                           &parser.definition_state_.struct_tag_def_map) &&
            cv > 0) {
            dim = cv;
        } else if (sz && sz->kind == NK_INT_LIT) {
            dim = sz->ival;
        } else {
            dim = 0;  // dynamic / unknown at parse time
        }
    }
    parser.expect(TokenKind::RBracket);
    return dim;
}

void parse_declarator_array_suffixes(
    Parser& parser, TypeSpec& ts, std::vector<long long>* out_dims) {
    if (!out_dims) return;
    while (parser.check(TokenKind::LBracket))
        out_dims->push_back(parse_one_declarator_array_dim(parser, ts));
}

void apply_declarator_array_dims(
    Parser&, TypeSpec& ts, const std::vector<long long>& decl_dims) {
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
    if (!is_cpp_mode() || !check(TokenKind::KwTypename))
        return TypenameTemplateParamKind::TypeParameter;

    int probe = core_input_state_.pos + 1;
    if (probe < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[probe].kind == TokenKind::Identifier) {
        ++probe;
    }

    if (probe >= static_cast<int>(core_input_state_.tokens.size()))
        return TypenameTemplateParamKind::TypeParameter;

    switch (core_input_state_.tokens[probe].kind) {
        case TokenKind::Assign:
        case TokenKind::Comma:
        case TokenKind::Greater:
        case TokenKind::GreaterGreater:
        case TokenKind::Ellipsis:
            return TypenameTemplateParamKind::TypeParameter;
        case TokenKind::KwClass:
            return TypenameTemplateParamKind::TypeParameter;
        case TokenKind::KwTypename:
            return TypenameTemplateParamKind::TypeParameter;
        case TokenKind::Identifier:
            if (token_spelling(core_input_state_.tokens[probe]) == "typename")
                return TypenameTemplateParamKind::TypeParameter;
            break;
        default:
            break;
    }

    return TypenameTemplateParamKind::TypedNonTypeParameter;
}

void Parser::parse_declarator(TypeSpec& ts, const char** out_name,
                              Node*** out_fn_ptr_params,
                              int* out_n_fn_ptr_params,
                              bool* out_fn_ptr_variadic,
                              bool* out_is_parameter_pack,
                              Node*** out_ret_fn_ptr_params,
                              int* out_n_ret_fn_ptr_params,
                              bool* out_ret_fn_ptr_variadic,
                              TextId* out_name_text_id) {
    if (out_name) *out_name = nullptr;
    if (out_name_text_id) *out_name_text_id = kInvalidText;
    if (out_fn_ptr_params) *out_fn_ptr_params = nullptr;
    if (out_n_fn_ptr_params) *out_n_fn_ptr_params = 0;
    if (out_fn_ptr_variadic) *out_fn_ptr_variadic = false;
    if (out_is_parameter_pack) *out_is_parameter_pack = false;
    if (out_ret_fn_ptr_params) *out_ret_fn_ptr_params = nullptr;
    if (out_n_ret_fn_ptr_params) *out_n_ret_fn_ptr_params = 0;
    if (out_ret_fn_ptr_variadic) *out_ret_fn_ptr_variadic = false;

    parse_declarator_prefix(*this, ts, out_is_parameter_pack);

    // Check for parenthesised declarator: (*name) or (ATTR *name) — function pointer
    if (is_parenthesized_pointer_declarator_start(*this)) {
        parse_parenthesized_pointer_declarator(
            *this,
            ts, out_name,
            out_fn_ptr_params, out_n_fn_ptr_params, out_fn_ptr_variadic,
            out_ret_fn_ptr_params, out_n_ret_fn_ptr_params,
            out_ret_fn_ptr_variadic, out_name_text_id);
        return;
    }

    parse_non_parenthesized_declarator_tail(
        *this,
        ts, out_name, /*decay_plain_function_suffix=*/false, out_name_text_id);
}

TypeSpec Parser::parse_type_name() {
    TypeSpec ts = parse_base_type();
    const char* ignored = nullptr;
    parse_declarator(ts, &ignored);
    return ts;
}

}  // namespace c4c

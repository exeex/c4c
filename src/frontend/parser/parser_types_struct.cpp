// parser_types_struct.cpp — struct/union/enum parsing, parse_param
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

// ── struct / union parsing ───────────────────────────────────────────────────

bool Parser::try_parse_record_access_label() {
    if (!(is_cpp_mode() &&
          (check(TokenKind::KwPublic) || check(TokenKind::KwPrivate) ||
           check(TokenKind::KwProtected) ||
           (check(TokenKind::Identifier) &&
            (cur().lexeme == "public" || cur().lexeme == "private" ||
             cur().lexeme == "protected"))) &&
          pos_ + 1 < static_cast<int>(tokens_.size()) &&
          tokens_[pos_ + 1].kind == TokenKind::Colon)) {
        return false;
    }

    consume();
    consume();
    return true;
}

bool Parser::try_skip_record_friend_member() {
    if (!(is_cpp_mode() &&
          (check(TokenKind::KwFriend) ||
           (check(TokenKind::Identifier) && cur().lexeme == "friend")))) {
        return false;
    }

    consume();
    int angle_depth = 0;
    int paren_depth = 0;
    while (!at_end()) {
        if (check(TokenKind::KwOperator) && angle_depth == 0 &&
            paren_depth == 0) {
            std::string ignored_name;
            if (parse_operator_declarator_name(&ignored_name)) {
                continue;
            }
        }
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

bool Parser::try_skip_record_static_assert_member(std::vector<Node*>* methods) {
    if (!check(TokenKind::KwStaticAssert))
        return false;

    if (methods) {
        methods->push_back(parse_static_assert_declaration());
    } else {
        (void)parse_static_assert_declaration();
    }
    return true;
}

Parser::RecordMemberRecoveryResult
Parser::recover_record_member_parse_error(int member_start_pos) {
    if (!is_cpp_mode())
        return RecordMemberRecoveryResult::Failed;

    int brace_depth = 0;
    while (!at_end()) {
        if (brace_depth == 0 &&
            is_record_member_recovery_boundary(*this, member_start_pos)) {
            return RecordMemberRecoveryResult::StoppedAtNextMember;
        }
        if (check(TokenKind::LBrace)) {
            ++brace_depth;
            consume();
        } else if (check(TokenKind::RBrace)) {
            if (brace_depth > 0) {
                --brace_depth;
                consume();
            } else {
                if (pos_ == member_start_pos && !at_end()) consume();
                return RecordMemberRecoveryResult::StoppedAtRBrace;
            }
        } else if (check(TokenKind::Semi) && brace_depth == 0) {
            consume();
            return RecordMemberRecoveryResult::SyncedAtSemicolon;
        } else {
            consume();
        }
    }

    if (pos_ == member_start_pos && !at_end()) consume();
    return RecordMemberRecoveryResult::Failed;
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
        if (check(TokenKind::KwTypename) ||
            check(TokenKind::KwClass)) {
            if (classify_typename_template_parameter() ==
                TypenameTemplateParamKind::TypedNonTypeParameter) {
                TypeSpec param_ts = parse_type_name();
                (void)param_ts;
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
                if (!match(TokenKind::Comma)) break;
                continue;
            }

            consume();
            if (check(TokenKind::Ellipsis)) consume();
            if (check(TokenKind::Identifier)) {
                std::string pname = cur().lexeme;
                consume();
                register_synthesized_typedef_binding(pname);
                injected_type_params->push_back(std::move(pname));
            }
            if (check(TokenKind::Assign)) {
                consume();
                bool parsed_default_type = false;
                {
                    TentativeParseGuard guard(*this);
                    try {
                        TypeSpec ignored_default = parse_type_name();
                        (void)ignored_default;
                        guard.commit();
                        parsed_default_type = true;
                    } catch (...) {
                        // guard restores pos_ on scope exit
                    }
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
        } else if (consume_template_parameter_type_start(/*allow_typename_keyword=*/true)) {
            while (check(TokenKind::Star) || is_qualifier(cur().kind)) consume();
            if (check(TokenKind::Ellipsis)) consume();
            if (check(TokenKind::Identifier)) {
                std::string pname = cur().lexeme;
                consume();
                register_synthesized_typedef_binding(pname);
                injected_type_params->push_back(std::move(pname));
            }
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
        } else if (is_cpp_mode()) {
            bool parsed_constrained_type_param = false;
            {
                TentativeParseGuard guard(*this);
                std::string constraint_name;
                if (consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &constraint_name, nullptr) &&
                    (check(TokenKind::Ellipsis) || check(TokenKind::Identifier))) {
                    if (check(TokenKind::Ellipsis)) consume();
                    if (check(TokenKind::Identifier)) {
                        std::string pname = cur().lexeme;
                        consume();
                        register_synthesized_typedef_binding(pname);
                        injected_type_params->push_back(std::move(pname));
                    }
                    if (check(TokenKind::Assign)) {
                        consume();
                        bool parsed_default_type = false;
                        {
                            TentativeParseGuard default_guard(*this);
                            try {
                                TypeSpec ignored_default = parse_type_name();
                                (void)ignored_default;
                                default_guard.commit();
                                parsed_default_type = true;
                            } catch (...) {
                                // default_guard restores pos_ on scope exit
                            }
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
                    parsed_constrained_type_param =
                        check(TokenKind::Comma) || check_template_close();
                    if (parsed_constrained_type_param) guard.commit();
                }
            }
            if (parsed_constrained_type_param) {
                // C++20 constrained type parameter, e.g. `template<C T>`.
            } else {
            bool parsed_typed_nttp = false;
            {
                TentativeParseGuard guard(*this);
                try {
                    TypeSpec param_ts = parse_type_name();
                    (void)param_ts;
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
                    parsed_typed_nttp =
                        check(TokenKind::Comma) || check_template_close();
                    if (parsed_typed_nttp) guard.commit();
                } catch (...) {
                    // guard restores pos_ on scope exit
                }
            }
            if (parsed_typed_nttp) {
                // Typed NTTP with a typedef/alias type head, e.g.
                // `__enable_if_t<Cond, bool> = true`.
            } else {
                // pos_ is back to the start of the NTTP token
                const int saved_type_head_pos = pos_;
                std::string type_head_name;
                const bool consumed_type_head = consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &type_head_name, nullptr);
                if (consumed_type_head &&
                    (check(TokenKind::Assign) ||
                     check(TokenKind::Comma) ||
                     check_template_close())) {
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
                    pos_ = saved_type_head_pos;
                    const int saved_constraint_pos = pos_;
                    std::string constraint_name;
                    if (consume_qualified_type_spelling(
                            /*allow_global=*/true,
                            /*consume_final_template_args=*/true,
                            &constraint_name, nullptr) &&
                        (check(TokenKind::Ellipsis) || check(TokenKind::Identifier))) {
                        if (check(TokenKind::Ellipsis)) consume();
                        if (check(TokenKind::Identifier)) {
                            std::string pname = cur().lexeme;
                            consume();
                            register_synthesized_typedef_binding(pname);
                            injected_type_params->push_back(std::move(pname));
                        }
                        if (check(TokenKind::Assign)) {
                            consume();
                            bool parsed_default_type = false;
                            {
                                TentativeParseGuard default_guard(*this);
                                try {
                                    TypeSpec ignored_default = parse_type_name();
                                    (void)ignored_default;
                                    default_guard.commit();
                                    parsed_default_type = true;
                                } catch (...) {
                                    // default_guard restores pos_ on scope exit
                                }
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
                    } else {
                        pos_ = saved_constraint_pos;
                        consume();
                    }
                }
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

void Parser::parse_decl_attrs_for_record(int line, TypeSpec* attr_ts) {
    if (!attr_ts)
        return;

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

    skip_cpp11_attrs();
    while (check(TokenKind::KwAlignas))
        parse_alignas_specifier(this, attr_ts, line);
    parse_attributes(attr_ts);
    skip_cpp11_attrs();
}

void Parser::skip_record_base_specifier_tail() {
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
}

bool Parser::try_parse_record_base_specifier(TypeSpec* base_ts) {
    while (check(TokenKind::KwPublic) || check(TokenKind::KwPrivate) ||
           check(TokenKind::KwProtected) || check(TokenKind::KwVirtual) ||
           (check(TokenKind::Identifier) &&
            (cur().lexeme == "public" || cur().lexeme == "private" ||
             cur().lexeme == "protected" || cur().lexeme == "virtual"))) {
        consume();
    }

    try {
        TypeSpec parsed_base = parse_base_type();
        while (check(TokenKind::Star)) {
            consume();
            parsed_base.ptr_level++;
        }
        if (check(TokenKind::AmpAmp)) {
            consume();
            parsed_base.is_rvalue_ref = true;
        } else if (check(TokenKind::Amp)) {
            consume();
            parsed_base.is_lvalue_ref = true;
        }
        skip_record_base_specifier_tail();
        if (base_ts)
            *base_ts = parsed_base;
        return true;
    } catch (...) {
        skip_record_base_specifier_tail();
        return false;
    }
}

void Parser::parse_record_base_clause(std::vector<TypeSpec>* base_types) {
    if (!base_types || !is_cpp_mode() || !check(TokenKind::Colon))
        return;

    consume();
    while (!at_end() && !check(TokenKind::LBrace)) {
        TypeSpec base_ts{};
        if (try_parse_record_base_specifier(&base_ts))
            base_types->push_back(base_ts);
        if (!match(TokenKind::Comma))
            break;
    }
}

bool Parser::try_parse_record_using_member(
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    ParseContextGuard trace(this, __func__);
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
        expect(TokenKind::Semi);

        register_typedef_binding(alias_name, alias_ts, false);
        member_typedef_names->push_back(arena_.strdup(alias_name.c_str()));
        member_typedef_types->push_back(alias_ts);
        if (!current_struct_tag_.empty()) {
            std::string scoped = current_struct_tag_ + "::" + alias_name;
            register_struct_member_typedef_binding(scoped, alias_ts);
        }
        return true;
    }

    if (check(TokenKind::KwTypename))
        consume();
    (void)parse_qualified_name(/*allow_global=*/true);
    expect(TokenKind::Semi);
    return true;
}

bool Parser::try_parse_record_typedef_member(
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    ParseContextGuard trace(this, __func__);
    if (!(is_cpp_mode() && check(TokenKind::KwTypedef)))
        return false;

    consume(); // eat 'typedef'
    TypeSpec td_base = parse_base_type();
    parse_attributes(&td_base);
    auto register_record_typedef = [&](const char* name, const TypeSpec& type,
                                       Node** fn_ptr_params,
                                       int n_fn_ptr_params,
                                       bool fn_ptr_variadic) {
        register_typedef_binding(name, type, true);
        if (type.is_fn_ptr && (n_fn_ptr_params > 0 || fn_ptr_variadic)) {
            typedef_fn_ptr_info_[name] = {
                fn_ptr_params, n_fn_ptr_params, fn_ptr_variadic};
        }
        member_typedef_names->push_back(name);
        member_typedef_types->push_back(type);
        if (!current_struct_tag_.empty()) {
            std::string scoped = current_struct_tag_ + "::" + name;
            register_struct_member_typedef_binding(scoped, type);
        }
    };

    const char* tdname = nullptr;
    TypeSpec ts_copy = td_base;
    Node** td_fn_ptr_params = nullptr;
    int td_n_fn_ptr_params = 0;
    bool td_fn_ptr_variadic = false;
    parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                     &td_n_fn_ptr_params, &td_fn_ptr_variadic);
    if (tdname) {
        register_record_typedef(tdname, ts_copy, td_fn_ptr_params,
                                td_n_fn_ptr_params, td_fn_ptr_variadic);
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
            register_record_typedef(tdn2, ts2, td2_fn_ptr_params,
                                    td2_n_fn_ptr_params, td2_fn_ptr_variadic);
        }
    }
    expect(TokenKind::Semi);
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
            auto it = typedef_types_.find(ed->name);
            if (it != typedef_types_.end())
                fts.enum_underlying_base = it->second.enum_underlying_base;
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
    ParseContextGuard trace(this, __func__);
    if (!(is_cpp_mode() && !current_struct_tag_.empty()))
        return false;

    int probe = pos_;
    while (probe < static_cast<int>(tokens_.size())) {
        if (tokens_[probe].kind == TokenKind::KwConstexpr ||
            tokens_[probe].kind == TokenKind::KwConsteval ||
            (tokens_[probe].kind == TokenKind::Identifier &&
             tokens_[probe].lexeme == "inline")) {
            ++probe;
            continue;
        }
        if (tokens_[probe].kind == TokenKind::KwExplicit) {
            ++probe;
            if (probe < static_cast<int>(tokens_.size()) &&
                tokens_[probe].kind == TokenKind::LParen) {
                int depth = 1;
                ++probe;
                while (probe < static_cast<int>(tokens_.size()) && depth > 0) {
                    if (tokens_[probe].kind == TokenKind::LParen) ++depth;
                    else if (tokens_[probe].kind == TokenKind::RParen) --depth;
                    ++probe;
                }
            }
            continue;
        }
        break;
    }
    if (probe < static_cast<int>(tokens_.size()) &&
        tokens_[probe].kind == TokenKind::Identifier &&
        is_record_special_member_name(tokens_[probe].lexeme,
                                      struct_source_name) &&
        probe + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[probe + 1].kind == TokenKind::LParen) {
        while (pos_ < probe) {
            if (check(TokenKind::KwExplicit))
                consume_optional_cpp_explicit_specifier(this);
            else
                consume();
        }
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
            if (!can_start_parameter_type()) break;
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
    parse_optional_cpp20_trailing_requires_clause(*this);
    Node* method = make_node(NK_FUNCTION, cur().line);
    method->type.base = TB_VOID;
    method->name = ctor_name;
    method->execution_domain = execution_domain_;
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
    consume_optional_cpp_member_virt_specifier_seq(this);
    std::string mangled = std::string("~") + dtor_name;
    Node* method = make_node(NK_FUNCTION, cur().line);
    method->type.base = TB_VOID;
    method->name = arena_.strdup(mangled.c_str());
    method->execution_domain = execution_domain_;
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
    ParseContextGuard trace(this, __func__);
    // Regular field declaration
    // C++ conversion operators (e.g., operator bool()) have no return
    // type prefix, so KwOperator can appear directly here.
    bool is_conversion_operator = false;
    if (is_cpp_mode()) {
        TentativeParseGuard operator_guard(*this);
        while (!at_end()) {
            if (check(TokenKind::KwOperator)) {
                is_conversion_operator = true;
                break;
            }
            if (check(TokenKind::KwConst) || check(TokenKind::KwVolatile) ||
                check(TokenKind::KwInline) || check(TokenKind::KwExtern) ||
                check(TokenKind::KwStatic) || check(TokenKind::KwConstexpr) ||
                check(TokenKind::KwConsteval) || check(TokenKind::KwMutable)) {
                consume();
                continue;
            }
            if (check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(this);
                continue;
            }
            break;
        }
    }
    if (!is_conversion_operator && !is_type_start()) {
        // unknown token in struct body — skip
        consume();
        return true;
    }

    // Track C++ storage-class specifiers before parse_base_type consumes them.
    bool field_is_static = false;
    bool field_is_constexpr = false;
    if (is_cpp_mode()) {
        TentativeParseGuard peek_guard(*this);
        while (!at_end() && !check(TokenKind::RBrace)) {
            if (check(TokenKind::KwStatic)) { field_is_static = true; consume(); continue; }
            if (check(TokenKind::KwConstexpr)) { field_is_constexpr = true; consume(); continue; }
            // Only peek through qualifiers/storage-class keywords
            if (check(TokenKind::KwConst) || check(TokenKind::KwVolatile) ||
                check(TokenKind::KwInline) || check(TokenKind::KwExtern) ||
                check(TokenKind::KwConsteval) ||
                check(TokenKind::KwMutable)) {
                consume();
                continue;
            }
            if (check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(this);
                continue;
            }
            break;
        }
        // peek_guard restores pos_ on scope exit (committed=false)
    }

    TypeSpec fts{};
    if (!is_conversion_operator) {
        fts = parse_base_type();
        parse_attributes(&fts);
    } else if (is_cpp_mode()) {
        while (!at_end()) {
            if (check(TokenKind::KwOperator)) break;
            if (check(TokenKind::KwConst) || check(TokenKind::KwVolatile) ||
                check(TokenKind::KwInline) || check(TokenKind::KwExtern) ||
                check(TokenKind::KwStatic) || check(TokenKind::KwConstexpr) ||
                check(TokenKind::KwConsteval) || check(TokenKind::KwMutable)) {
                consume();
                continue;
            }
            if (check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(this);
                continue;
            }
            break;
        }
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
        if (is_type_start() || can_start_parameter_type() ||
            check(TokenKind::Identifier) ||
            check(TokenKind::ColonColon)) {
            // Conversion operator: operator T() — the token after 'operator'
            // is a type name. This covers both standalone 'operator T()' and
            // prefix forms like 'explicit operator T()' / 'constexpr explicit operator T()'.
            is_conversion_operator = true;
            if (check(TokenKind::Identifier) && !is_type_start() &&
                !can_start_parameter_type()) {
                fts = TypeSpec{};
                fts.base = TB_TYPEDEF;
                fts.tag = arena_.strdup(cur().lexeme);
                fts.array_size = -1;
                fts.inner_rank = -1;
                consume();
            } else {
                fts = parse_base_type();
            }
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
        } else if (check(TokenKind::Spaceship)) {
            consume();
            op_kind = OP_SPACESHIP;
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

        // Function attributes such as [[nodiscard]] can appear between the
        // operator name and its parameter list.
        skip_attributes();

        // Parse parameter list
        expect(TokenKind::LParen);
        std::vector<Node*> params;
        bool variadic = false;
        if (!check(TokenKind::RParen)) {
            while (!at_end()) {
                if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                if (check(TokenKind::RParen)) break;
                if (!can_start_parameter_type()) break;
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
        bool is_method_lvalue_ref = false;
        bool is_method_rvalue_ref = false;
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
            if (match(TokenKind::AmpAmp)) is_method_rvalue_ref = true;
            else if (match(TokenKind::Amp)) is_method_lvalue_ref = true;
        }
        skip_exception_spec();
        if (is_cpp_mode() && match(TokenKind::Arrow)) {
            fts = parse_type_name();
            parse_attributes(&fts);
        }
        parse_optional_cpp20_trailing_requires_clause(*this);
        consume_optional_cpp_member_virt_specifier_seq(this);

        Node* method = make_node(NK_FUNCTION, cur().line);
        method->type = fts;
        method->name = arena_.strdup(op_mangled);
        method->execution_domain = execution_domain_;
        method->operator_kind = op_kind;
        method->variadic = variadic;
        method->is_const_method = is_method_const;
        method->is_lvalue_ref_method = is_method_lvalue_ref;
        method->is_rvalue_ref_method = is_method_rvalue_ref;
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
                    if (!can_start_parameter_type()) break;
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
            bool is_method_lvalue_ref = false;
            bool is_method_rvalue_ref = false;
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
                if (match(TokenKind::AmpAmp)) is_method_rvalue_ref = true;
                else if (match(TokenKind::Amp)) is_method_lvalue_ref = true;
            }
            skip_exception_spec();
            if (is_cpp_mode() && match(TokenKind::Arrow)) {
                cur_fts = parse_type_name();
                parse_attributes(&cur_fts);
            }
            parse_optional_cpp20_trailing_requires_clause(*this);
            consume_optional_cpp_member_virt_specifier_seq(this);
            Node* method = make_node(NK_FUNCTION, cur().line);
            method->type = cur_fts;
            method->name = fname;
            method->execution_domain = execution_domain_;
            method->variadic = variadic;
            method->is_const_method = is_method_const;
            method->is_lvalue_ref_method = is_method_lvalue_ref;
            method->is_rvalue_ref_method = is_method_rvalue_ref;
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

bool Parser::try_parse_record_type_like_member_dispatch(
    std::vector<Node*>* fields,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    ParseContextGuard trace(this, __func__);
    if (try_parse_record_using_member(member_typedef_names,
                                      member_typedef_types)) {
        return true;
    }
    if (try_parse_nested_record_member(fields, check_dup_field)) return true;
    if (try_parse_record_enum_member(fields, check_dup_field)) return true;
    return try_parse_record_typedef_member(member_typedef_names,
                                           member_typedef_types);
}

bool Parser::try_parse_record_member_dispatch(
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    ParseContextGuard trace(this, __func__);
    if (try_parse_record_type_like_member_dispatch(
            fields, member_typedef_names, member_typedef_types,
            check_dup_field)) {
        return true;
    }
    if (try_parse_record_special_member_dispatch(struct_source_name, methods))
        return true;
    return try_parse_record_method_or_field_member(fields, methods,
                                                   check_dup_field);
}

bool Parser::try_parse_record_special_member_dispatch(
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    if (try_parse_record_constructor_member(struct_source_name, methods)) {
        return true;
    }
    return try_parse_record_destructor_member(struct_source_name, methods);
}

bool Parser::try_parse_record_member_with_template_prelude(
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    struct TemplateParamGuard {
        Parser* parser;
        std::set<std::string>& typedefs;
        std::vector<TemplateScopeFrame>& scope_stack;
        std::vector<std::string> names;
        bool pushed_scope = false;
        ~TemplateParamGuard() {
            if (pushed_scope) scope_stack.pop_back();
            if (!parser) return;
            for (auto& n : names) parser->unregister_typedef_binding(n);
        }
    } tmpl_guard{this, typedefs_, template_scope_stack_, {}};

    parse_record_template_member_prelude(&tmpl_guard.names,
                                         &tmpl_guard.pushed_scope);
    parse_optional_cpp20_requires_clause(*this);
    if (try_skip_record_friend_member()) return true;
    if (try_skip_record_static_assert_member(methods)) return true;
    return try_parse_record_member_dispatch(struct_source_name, fields, methods,
                                            member_typedef_names,
                                            member_typedef_types,
                                            check_dup_field);
}

bool Parser::begin_record_member_parse() {
    skip_attributes();
    return !check(TokenKind::RBrace);
}

bool Parser::try_parse_record_member_prelude(std::vector<Node*>* methods) {
    if (try_parse_record_access_label()) return true;
    if (try_skip_record_friend_member()) return true;
    return try_skip_record_static_assert_member(methods);
}

bool Parser::try_parse_record_member(
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    if (!begin_record_member_parse()) return false;
    if (try_parse_record_member_prelude(methods)) return true;
    return try_parse_record_member_with_template_prelude(
        struct_source_name, fields, methods, member_typedef_names,
        member_typedef_types, check_dup_field);
}

bool Parser::try_parse_record_body_member(
    const std::string& struct_source_name,
    RecordBodyState* body_state,
    const std::function<void(const char*)>& check_dup_field) {
    if (!body_state)
        return false;

    const int member_start_pos = pos_;
    try {
        return try_parse_record_member(struct_source_name, &body_state->fields,
                                       &body_state->methods,
                                       &body_state->member_typedef_names,
                                       &body_state->member_typedef_types,
                                       check_dup_field);
    } catch (const std::exception&) {
        const RecordMemberRecoveryResult recovery =
            recover_record_member_parse_error(member_start_pos);
        if (recovery != RecordMemberRecoveryResult::SyncedAtSemicolon &&
            recovery != RecordMemberRecoveryResult::StoppedAtNextMember)
            throw;
        return false;
    }
}

void Parser::begin_record_body_context(const char* tag,
                                       const char* template_origin_name,
                                       std::string* saved_struct_tag,
                                       std::string* struct_source_name) {
    // Make the current record name available for self-type parsing within the
    // body before member dispatch starts.
    if (is_cpp_mode() && tag && tag[0])
        register_typedef_name(tag, false);

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

    register_typedef_name(template_origin_name, false);
    if (typedef_types_.count(template_origin_name) == 0) {
        register_tag_type_binding(template_origin_name, TB_STRUCT, tag);
    }
}

void Parser::parse_record_body(
    const std::string& struct_source_name,
    RecordBodyState* body_state) {
    if (!body_state)
        return;

    std::unordered_set<std::string> field_names_seen;
    auto check_dup_field =
        make_record_field_duplicate_checker(this, &field_names_seen);

    while (!at_end() && !check(TokenKind::RBrace)) {
        if (try_parse_record_body_member(struct_source_name, body_state,
                                         check_dup_field)) {
            continue;
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
    finish_record_body_context(saved_struct_tag);
}

void Parser::finish_record_body_context(const std::string& saved_struct_tag) {
    expect(TokenKind::RBrace);
    current_struct_tag_ = saved_struct_tag;
}

void Parser::parse_record_definition_prelude(
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

    parse_decl_attrs_for_record(line, attr_ts);

    if (is_cpp_mode()) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, /*allow_global=*/true)) {
            qn = parse_qualified_name(/*allow_global=*/true);
            std::string spelled;
            for (size_t i = 0; i < qn.qualifier_segments.size(); ++i) {
                if (!spelled.empty()) spelled += "::";
                spelled += qn.qualifier_segments[i];
            }
            if (!spelled.empty()) spelled += "::";
            spelled += qn.base_name;
            *tag = arena_.strdup(spelled.c_str());
        }
    } else if (check(TokenKind::Identifier)) {
        *tag = arena_.strdup(cur().lexeme);
        consume();
    }
    parse_decl_attrs_for_record(line, attr_ts);

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
             tokens_[probe].kind == TokenKind::Colon ||
             tokens_[probe].kind == TokenKind::Semi);
        if (is_specialization) {
            *template_origin_name = arena_.strdup(*tag);
            bool parse_ok = true;
            {
                TentativeParseGuard guard(*this);
                try {
                    const Node* primary_tpl = find_template_struct_primary(*tag);
                    if (!parse_template_argument_list(specialization_args,
                                                      primary_tpl)) {
                        throw std::runtime_error(
                            "failed to parse specialization args");
                    }
                    guard.commit();
                } catch (...) {
                    parse_ok = false;
                    specialization_args->clear();
                    // guard restores pos_ on scope exit
                }
            }
            if (!parse_ok && check(TokenKind::Less)) {
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
            (void)parse_ok;
            std::string mangled(*tag);
            mangled += "__spec_";
            mangled += std::to_string(anon_counter_++);
            *tag = arena_.strdup(mangled.c_str());
        }
    }

    // C++ class-virt-specifier support: accept the contextual `final`
    // keyword between the record name and any base-clause/body.
    if (is_cpp_mode() && check(TokenKind::KwFinal)) {
        consume();
    }

    parse_record_base_clause(base_types);
}

Node* Parser::parse_record_tag_setup(int line,
                                     bool is_union,
                                     const char** tag,
                                     const char* template_origin_name,
                                     const TypeSpec& attr_ts,
                                     const std::vector<TemplateArgParseResult>& specialization_args) {
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

        Node* ref = nullptr;
        if (template_origin_name && template_origin_name[0] &&
            !specialization_args.empty()) {
            std::vector<TypeSpec> empty_base_types;
            ref = build_record_definition_node(
                line, is_union, resolved_tag, template_origin_name, attr_ts,
                specialization_args, empty_base_types);
            ref->n_fields = -1;  // forward-declared explicit specialization
        } else {
            ref = make_node(NK_STRUCT_DEF, line);
            ref->name = resolved_tag;
            ref->is_union = is_union;
            ref->n_fields = -1;  // -1 = forward reference (no body)
        }
        if (is_cpp_mode() && resolved_tag && resolved_tag[0]) {
            register_tag_type_binding(resolved_tag,
                                      is_union ? TB_UNION : TB_STRUCT,
                                      resolved_tag);
        }
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
            std::strstr(resolved_tag, "::")
                ? std::string(resolved_tag)
                : canonical_name_in_context(current_namespace_context_id(),
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

Node* Parser::build_record_definition_node(
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

Node* Parser::parse_record_definition_after_tag_setup(
    int line,
    bool is_union,
    const char* tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types) {
    const char* source_tag = tag;
    Node* sd = build_record_definition_node(line, is_union, tag,
                                            template_origin_name, attr_ts,
                                            specialization_args, base_types);
    parse_record_definition_body(sd, is_union, source_tag, tag,
                                 template_origin_name);
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

void Parser::register_record_definition(Node* sd,
                                        bool is_union,
                                        const char* source_tag) {
    if (!sd || !(source_tag && source_tag[0]))
        return;

    const std::string canonical =
        std::strstr(source_tag, "::")
            ? std::string(source_tag)
            : canonical_name_in_context(current_namespace_context_id(),
                                        source_tag);
    sd->name = arena_.strdup(canonical.c_str());
    apply_decl_namespace(sd, current_namespace_context_id(), source_tag);
    struct_tag_def_map_[source_tag] = sd;
    struct_tag_def_map_[sd->name] = sd;

    if (!is_cpp_mode() || !(sd->name && sd->name[0]))
        return;

    register_tag_type_binding(sd->name, is_union ? TB_UNION : TB_STRUCT,
                              sd->name);
    if (source_tag && source_tag[0] && std::strcmp(source_tag, sd->name) != 0) {
        register_tag_type_binding(source_tag, is_union ? TB_UNION : TB_STRUCT,
                                  sd->name);
    }
}

void Parser::finalize_record_definition(
    Node* sd,
    bool is_union,
    const char* source_tag,
    const RecordBodyState& body_state) {
    apply_record_trailing_type_attributes(sd);
    store_record_body_members(sd, body_state);
    register_record_definition(sd, is_union, source_tag);
    struct_defs_.push_back(sd);
}

void Parser::parse_record_definition_body(Node* sd,
                                          bool is_union,
                                          const char* source_tag,
                                          const char* tag,
                                          const char* template_origin_name) {
    consume();  // consume {

    RecordBodyState body_state;
    parse_record_body_with_context(tag, template_origin_name, &body_state);
    finalize_record_definition(sd, is_union, source_tag, body_state);
}

Node* Parser::parse_struct_or_union(bool is_union) {
    int ln = cur().line;
    TypeSpec attr_ts{};
    const char* tag = nullptr;
    const char* template_origin_name = nullptr;
    std::vector<ParsedTemplateArg> specialization_args;
    std::vector<TypeSpec> base_types;
    parse_record_definition_prelude(ln, &attr_ts, &tag, &template_origin_name,
                                    &specialization_args, &base_types);

    if (Node* ref = parse_record_tag_setup(ln, is_union, &tag,
                                           template_origin_name, attr_ts,
                                           specialization_args))
        return ref;

    return parse_record_definition_after_tag_setup(
        ln, is_union, tag, template_origin_name, attr_ts, specialization_args,
        base_types);
}

// ── enum parsing ─────────────────────────────────────────────────────────────

Node* Parser::parse_enum() {
    int ln = cur().line;
    skip_attributes();

    bool is_scoped_enum = false;
    if (check(TokenKind::KwClass) || check(TokenKind::KwStruct)) {
        is_scoped_enum = true;
        consume();
        skip_attributes();
    }

    const char* tag = nullptr;
    if (check(TokenKind::Identifier)) {
        tag = arena_.strdup(cur().lexeme);
        consume();
    }
    skip_attributes();

    TypeBase enum_underlying_base = TB_INT;
    if (match(TokenKind::Colon)) {
        // Preserve fixed underlying-type metadata for layout-sensitive queries.
        TypeSpec underlying_ts = parse_base_type();
        underlying_ts = resolve_typedef_chain(underlying_ts, typedef_types_);
        enum_underlying_base = effective_scalar_base(underlying_ts);
        skip_attributes();
    }

    auto register_enum_type = [&](const char* source_tag, const char* canonical_tag) {
        if (!source_tag || !source_tag[0] || !canonical_tag || !canonical_tag[0])
            return;
        register_tag_type_binding(source_tag, TB_ENUM, canonical_tag,
                                  enum_underlying_base);
        if (strcmp(source_tag, canonical_tag) != 0)
            register_tag_type_binding(canonical_tag, TB_ENUM, canonical_tag,
                                      enum_underlying_base);
    };

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
        register_enum_type(tag, ref->name);
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
    register_enum_type(tag, ed->name);

    consume();  // consume {

    std::vector<const char*> names;
    std::vector<long long>   vals;
    std::unordered_set<std::string> seen_names;
    std::unordered_map<std::string, long long> local_enum_consts = enum_consts_;
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
            if (!eval_enum_expr(ve, local_enum_consts, &vval)) {
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
        // Track enum constants for subsequent initializers in this enum body.
        local_enum_consts[std::string(vname)] = vval;
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
    if (!is_scoped_enum) {
        for (int i = 0; i < ed->n_enum_variants; ++i)
            enum_consts_[std::string(ed->enum_names[i])] = ed->enum_vals[i];
    }
    if (parsing_top_level_context_) struct_defs_.push_back(ed);
    return ed;
}

// ── parameter parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_param() {
    ParseContextGuard trace(this, __func__);
    int ln = cur().line;
    auto skip_cpp11_attrs_only = [&]() {
        while (check(TokenKind::LBracket) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::LBracket) {
            consume();
            consume();
            int depth = 1;
            while (!at_end() && depth > 0) {
                if (check(TokenKind::LBracket) &&
                    pos_ + 1 < static_cast<int>(tokens_.size()) &&
                    tokens_[pos_ + 1].kind == TokenKind::LBracket) {
                    consume();
                    consume();
                    ++depth;
                    continue;
                }
                if (check(TokenKind::RBracket) &&
                    pos_ + 1 < static_cast<int>(tokens_.size()) &&
                    tokens_[pos_ + 1].kind == TokenKind::RBracket) {
                    consume();
                    consume();
                    --depth;
                    continue;
                }
                consume();
            }
        }
    };

    skip_cpp11_attrs_only();
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
    skip_cpp11_attrs_only();
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

// struct.cpp — struct/union/enum parsing, parse_param
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

struct ParserFunctionParamScopeGuard {
    Parser* parser = nullptr;
    bool active = false;

    ParserFunctionParamScopeGuard(Parser& p, const std::vector<Node*>& params)
        : parser(&p) {
        if (params.empty()) return;
        parser->push_local_binding_scope();
        active = true;
        for (Node* param : params) {
            if (!param || !param->name || !param->name[0]) continue;
            if (param->unqualified_text_id == kInvalidText) continue;
            parser->register_var_type_binding(param->unqualified_text_id,
                                              param->type);
        }
    }

    ~ParserFunctionParamScopeGuard() {
        if (active) parser->pop_local_binding_scope();
    }
};

static void restore_current_struct_tag(Parser& parser, TextId text_id,
                                       const std::string& fallback) {
    const std::string_view tag = parser.parser_text(text_id, fallback);
    if (tag.empty()) {
        parser.clear_current_struct_tag();
        return;
    }
    parser.set_current_struct_tag(text_id, tag);
}

// ── struct / union parsing ───────────────────────────────────────────────────

bool try_parse_record_access_label(Parser& parser) {
    if (!(parser.is_cpp_mode() &&
          (parser.check(TokenKind::KwPublic) || parser.check(TokenKind::KwPrivate) ||
           parser.check(TokenKind::KwProtected) ||
           (parser.check(TokenKind::Identifier) &&
            (parser.token_spelling(parser.cur()) == "public" ||
             parser.token_spelling(parser.cur()) == "private" ||
             parser.token_spelling(parser.cur()) == "protected"))) &&
          parser.core_input_state_.pos + 1 <
              static_cast<int>(parser.core_input_state_.tokens.size()) &&
          parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
              TokenKind::Colon)) {
        return false;
    }

    parser.consume();
    parser.consume();
    return true;
}

bool try_skip_record_friend_member(Parser& parser) {
    if (!(parser.is_cpp_mode() &&
          (parser.check(TokenKind::KwFriend) ||
           (parser.check(TokenKind::Identifier) &&
            parser.token_spelling(parser.cur()) == "friend")))) {
        return false;
    }

    parser.consume();
    int angle_depth = 0;
    int paren_depth = 0;
    while (!parser.at_end()) {
        if (parser.check(TokenKind::KwOperator) && angle_depth == 0 &&
            paren_depth == 0) {
            std::string ignored_name;
            if (parser.parse_operator_declarator_name(&ignored_name)) {
                continue;
            }
        }
        if (parser.check(TokenKind::Less) && paren_depth == 0) {
            ++angle_depth;
            parser.consume();
            continue;
        }
        if (parser.check_template_close() && paren_depth == 0 && angle_depth > 0) {
            --angle_depth;
            parser.match_template_close();
            continue;
        }
        if (parser.check(TokenKind::LParen)) {
            ++paren_depth;
            parser.consume();
            continue;
        }
        if (parser.check(TokenKind::RParen) && paren_depth > 0) {
            --paren_depth;
            parser.consume();
            continue;
        }
        if (parser.check(TokenKind::LBrace) && angle_depth == 0 && paren_depth == 0) {
            parser.skip_brace_group();
            break;
        }
        if (parser.check(TokenKind::Semi) && angle_depth == 0 && paren_depth == 0) {
            parser.consume();
            break;
        }
        if (parser.check(TokenKind::RBrace) && angle_depth == 0 && paren_depth == 0) {
            break;
        }
        parser.consume();
    }
    if (parser.check(TokenKind::Assign)) {
        parser.consume();
        if (parser.check(TokenKind::KwDelete) || parser.check(TokenKind::KwDefault))
            parser.consume();
        parser.match(TokenKind::Semi);
    }
    return true;
}

bool try_skip_record_static_assert_member(Parser& parser,
                                          std::vector<Node*>* methods) {
    if (!parser.check(TokenKind::KwStaticAssert))
        return false;

    if (methods) {
        methods->push_back(parse_static_assert_declaration(parser));
    } else {
        (void)parse_static_assert_declaration(parser);
    }
    return true;
}

Parser::RecordMemberRecoveryResult recover_record_member_parse_error(
    Parser& parser, int member_start_pos) {
    if (!parser.is_cpp_mode())
        return Parser::RecordMemberRecoveryResult::Failed;

    int brace_depth = 0;
    while (!parser.at_end()) {
        if (brace_depth == 0 &&
            is_record_member_recovery_boundary(parser, member_start_pos)) {
            return Parser::RecordMemberRecoveryResult::StoppedAtNextMember;
        }
        if (parser.check(TokenKind::LBrace)) {
            ++brace_depth;
            parser.consume();
        } else if (parser.check(TokenKind::RBrace)) {
            if (brace_depth > 0) {
                --brace_depth;
                parser.consume();
            } else {
                if (parser.pos_ == member_start_pos && !parser.at_end()) parser.consume();
                return Parser::RecordMemberRecoveryResult::StoppedAtRBrace;
            }
        } else if (parser.check(TokenKind::Semi) && brace_depth == 0) {
            parser.consume();
            return Parser::RecordMemberRecoveryResult::SyncedAtSemicolon;
        } else {
            parser.consume();
        }
    }

    if (parser.pos_ == member_start_pos && !parser.at_end()) parser.consume();
    return Parser::RecordMemberRecoveryResult::Failed;
}

void parse_record_template_member_prelude(
    Parser& parser,
    std::vector<Parser::InjectedTemplateParam>* injected_type_params,
    bool* pushed_template_scope) {
    auto at_end = [&]() { return parser.at_end(); };
    auto check = [&](TokenKind kind) { return parser.check(kind); };
    auto check_template_close = [&]() { return parser.check_template_close(); };
    auto consume = [&]() { parser.consume(); };
    auto consume_qualified_type_spelling =
        [&](bool allow_global, bool consume_final_template_args,
            std::string* out, Parser::QualifiedNameRef* out_qn) {
            return parser.consume_qualified_type_spelling(
                allow_global, consume_final_template_args, out, out_qn);
        };
    auto consume_template_parameter_type_start =
        [&](bool allow_typename_keyword) {
            return parser.consume_template_parameter_type_start(
                allow_typename_keyword);
        };
    auto cur = [&]() -> const Token& { return parser.cur(); };
    auto expect = [&](TokenKind kind) { parser.expect(kind); };
    auto expect_template_close = [&]() { parser.expect_template_close(); };
    auto is_cpp_mode = [&]() { return parser.is_cpp_mode(); };
    auto match = [&](TokenKind kind) { return parser.match(kind); };
    auto classify_typename_template_parameter = [&]() {
        return parser.classify_typename_template_parameter();
    };
    auto parse_greater_than_in_template_list = [&](bool allow_shift) {
        parser.parse_greater_than_in_template_list(allow_shift);
    };
    auto parse_type_name = [&]() { return parser.parse_type_name(); };
    auto parser_text_id_for_token = [&](TextId token_text_id,
                                        const std::string& fallback) {
        return parser.parser_text_id_for_token(token_text_id, fallback);
    };
    auto register_synthesized_typedef_binding =
        [&](TextId name_text_id) {
            parser.register_synthesized_typedef_binding(name_text_id);
        };
    auto token_spelling = [&](const Token& token) {
        return parser.token_spelling(token);
    };
    auto& arena_ = parser.arena_;
    auto& pos_ = parser.pos_;
    if (pushed_template_scope) *pushed_template_scope = false;
    if (!(is_cpp_mode() && check(TokenKind::KwTemplate)))
        return;

    consume();
    expect(TokenKind::Less);
    while (!at_end() && !check_template_close()) {
        if (check(TokenKind::KwTypename) ||
            check(TokenKind::KwClass)) {
            if (classify_typename_template_parameter() ==
                Parser::TypenameTemplateParamKind::TypedNonTypeParameter) {
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
                const std::string pname = std::string(token_spelling(cur()));
                const TextId pname_text_id =
                    parser_text_id_for_token(cur().text_id, pname);
                consume();
                register_synthesized_typedef_binding(pname_text_id);
                injected_type_params->push_back(
                    {pname_text_id, arena_.strdup(pname.c_str())});
            }
            if (check(TokenKind::Assign)) {
                consume();
                bool parsed_default_type = false;
                {
                    Parser::TentativeParseGuard guard(parser);
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
                const std::string pname = std::string(token_spelling(cur()));
                const TextId pname_text_id =
                    parser_text_id_for_token(cur().text_id, pname);
                consume();
                register_synthesized_typedef_binding(pname_text_id);
                injected_type_params->push_back(
                    {pname_text_id, arena_.strdup(pname.c_str())});
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
                Parser::TentativeParseGuard guard(parser);
                std::string constraint_name;
                if (consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &constraint_name, nullptr) &&
                    (check(TokenKind::Ellipsis) || check(TokenKind::Identifier))) {
                    if (check(TokenKind::Ellipsis)) consume();
                    if (check(TokenKind::Identifier)) {
                        const std::string pname = std::string(token_spelling(cur()));
                        const TextId pname_text_id =
                            parser_text_id_for_token(cur().text_id, pname);
                        consume();
                        register_synthesized_typedef_binding(pname_text_id);
                        injected_type_params->push_back(
                            {pname_text_id, arena_.strdup(pname.c_str())});
                    }
                    if (check(TokenKind::Assign)) {
                        consume();
                        bool parsed_default_type = false;
                        {
                            Parser::TentativeParseGuard default_guard(parser);
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
                Parser::TentativeParseGuard guard(parser);
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
                            const std::string pname = std::string(token_spelling(cur()));
                            const TextId pname_text_id =
                                parser_text_id_for_token(cur().text_id, pname);
                            consume();
                            register_synthesized_typedef_binding(pname_text_id);
                            injected_type_params->push_back(
                                {pname_text_id, arena_.strdup(pname.c_str())});
                        }
                        if (check(TokenKind::Assign)) {
                            consume();
                            bool parsed_default_type = false;
                            {
                                Parser::TentativeParseGuard default_guard(parser);
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
        std::vector<Parser::TemplateScopeParam> member_params;
        for (const auto& injected : *injected_type_params) {
            Parser::TemplateScopeParam p;
            p.name_text_id = injected.name_text_id;
            p.name = injected.name;
            p.is_nttp = false;
            member_params.push_back(p);
        }
        parser.push_template_scope(Parser::TemplateScopeKind::MemberTemplate,
                                   member_params);
        if (pushed_template_scope) *pushed_template_scope = true;
    }
}

void parse_decl_attrs_for_record(Parser& parser, int line, TypeSpec* attr_ts) {
    if (!attr_ts)
        return;

    auto check = [&](TokenKind kind) { return parser.check(kind); };
    auto consume = [&]() { parser.consume(); };
    auto parse_attributes = [&](TypeSpec* ts) {
        parser.parse_attributes(ts);
    };
    auto parse_alignas_specifier = [&](TypeSpec* ts) {
        c4c::parse_alignas_specifier(&parser, ts, line);
    };
    auto& core_input_state_ = parser.core_input_state_;
    auto& pos_ = parser.pos_;
    auto& tokens_ = parser.tokens_;

    auto skip_cpp11_attrs = [&]() {
        while (check(TokenKind::LBracket) &&
               core_input_state_.pos + 1 <
                   static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
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
        parse_alignas_specifier(attr_ts);
    parse_attributes(attr_ts);
    skip_cpp11_attrs();
}

void skip_record_base_specifier_tail(Parser& parser) {
    while (!parser.check(TokenKind::LBrace) && !parser.check(TokenKind::Comma) &&
           !parser.check(TokenKind::RBrace) && !parser.at_end()) {
        int angle_depth = 0;
        int paren_depth = 0;
        while (!parser.at_end()) {
            if (parser.check(TokenKind::LBrace) && angle_depth == 0 &&
                paren_depth == 0) {
                break;
            }
            if (parser.check(TokenKind::Comma) && angle_depth == 0 &&
                paren_depth == 0) {
                break;
            }
            if (parser.check(TokenKind::Less) && paren_depth == 0)
                ++angle_depth;
            else if (parser.check(TokenKind::Greater) &&
                     paren_depth == 0 && angle_depth > 0)
                --angle_depth;
            else if (parser.check(TokenKind::GreaterGreater) &&
                     paren_depth == 0 && angle_depth > 0) {
                angle_depth -= std::min(angle_depth, 2);
                parser.consume();
                continue;
            } else if (parser.check(TokenKind::LParen))
                ++paren_depth;
            else if (parser.check(TokenKind::RParen) && paren_depth > 0)
                --paren_depth;
            parser.consume();
        }
        break;
    }
}

bool try_parse_record_base_specifier(Parser& parser, TypeSpec* base_ts) {
    while (parser.check(TokenKind::KwPublic) || parser.check(TokenKind::KwPrivate) ||
           parser.check(TokenKind::KwProtected) || parser.check(TokenKind::KwVirtual) ||
           (parser.check(TokenKind::Identifier) &&
            (parser.token_spelling(parser.cur()) == "public" ||
             parser.token_spelling(parser.cur()) == "private" ||
             parser.token_spelling(parser.cur()) == "protected" ||
             parser.token_spelling(parser.cur()) == "virtual"))) {
        parser.consume();
    }

    try {
        TypeSpec parsed_base = parser.parse_base_type();
        while (parser.check(TokenKind::Star)) {
            parser.consume();
            parsed_base.ptr_level++;
        }
        if (parser.check(TokenKind::AmpAmp)) {
            parser.consume();
            parsed_base.is_rvalue_ref = true;
        } else if (parser.check(TokenKind::Amp)) {
            parser.consume();
            parsed_base.is_lvalue_ref = true;
        }
        skip_record_base_specifier_tail(parser);
        if (base_ts)
            *base_ts = parsed_base;
        return true;
    } catch (...) {
        skip_record_base_specifier_tail(parser);
        return false;
    }
}

void parse_record_base_clause(Parser& parser, std::vector<TypeSpec>* base_types) {
    if (!base_types || !parser.is_cpp_mode() || !parser.check(TokenKind::Colon))
        return;

    parser.consume();
    while (!parser.at_end() && !parser.check(TokenKind::LBrace)) {
        TypeSpec base_ts{};
        if (try_parse_record_base_specifier(parser, &base_ts))
            base_types->push_back(base_ts);
        if (!parser.match(TokenKind::Comma))
            break;
    }
}

bool try_parse_record_using_member(
    Parser& parser,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (!(parser.is_cpp_mode() && parser.check(TokenKind::KwUsing)))
        return false;

    parser.consume();
    if (parser.check(TokenKind::Identifier) &&
        parser.core_input_state_.pos + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind == TokenKind::Assign) {
        const TextId alias_name_text_id = parser.cur().text_id;
        std::string alias_name = std::string(parser.token_spelling(parser.cur()));
        parser.consume(); // name
        parser.consume(); // '='
        TypeSpec alias_ts = parser.parse_type_name();
        parser.expect(TokenKind::Semi);

        parser.register_typedef_binding(alias_name_text_id, alias_ts, false);
        member_typedef_names->push_back(parser.arena_.strdup(alias_name.c_str()));
        member_typedef_types->push_back(alias_ts);
        return true;
    }

    if (parser.check(TokenKind::KwTypename))
        parser.consume();
    (void)parser.parse_qualified_name(/*allow_global=*/true);
    parser.expect(TokenKind::Semi);
    return true;
}

bool try_parse_record_typedef_member(
    Parser& parser,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (!(parser.is_cpp_mode() && parser.check(TokenKind::KwTypedef)))
        return false;

    parser.consume(); // eat 'typedef'
    TypeSpec td_base = parser.parse_base_type();
    parser.parse_attributes(&td_base);
    auto register_record_typedef = [&](const char* name, TextId name_text_id,
                                       const TypeSpec& type,
                                       Node** fn_ptr_params,
                                       int n_fn_ptr_params,
                                       bool fn_ptr_variadic) {
        parser.register_typedef_binding(name_text_id, type, true);
        if (type.is_fn_ptr && (n_fn_ptr_params > 0 || fn_ptr_variadic)) {
            if (name_text_id != kInvalidText) {
                parser.binding_state_.typedef_fn_ptr_info[name_text_id] = {
                    fn_ptr_params, n_fn_ptr_params, fn_ptr_variadic};
            }
        }
        member_typedef_names->push_back(name);
        member_typedef_types->push_back(type);
    };

    const char* tdname = nullptr;
    TextId tdname_text_id = kInvalidText;
    TypeSpec ts_copy = td_base;
    Node** td_fn_ptr_params = nullptr;
    int td_n_fn_ptr_params = 0;
    bool td_fn_ptr_variadic = false;
    parser.parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                            &td_n_fn_ptr_params, &td_fn_ptr_variadic, nullptr,
                            nullptr, nullptr, nullptr, &tdname_text_id);
    if (tdname) {
        register_record_typedef(tdname, tdname_text_id, ts_copy,
                                td_fn_ptr_params, td_n_fn_ptr_params,
                                td_fn_ptr_variadic);
    }

    while (parser.match(TokenKind::Comma)) {
        TypeSpec ts2 = td_base;
        const char* tdn2 = nullptr;
        TextId tdn2_text_id = kInvalidText;
        Node** td2_fn_ptr_params = nullptr;
        int td2_n_fn_ptr_params = 0;
        bool td2_fn_ptr_variadic = false;
        parser.parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                                &td2_n_fn_ptr_params, &td2_fn_ptr_variadic,
                                nullptr, nullptr, nullptr, nullptr,
                                &tdn2_text_id);
        if (tdn2) {
            register_record_typedef(tdn2, tdn2_text_id, ts2, td2_fn_ptr_params,
                                    td2_n_fn_ptr_params, td2_fn_ptr_variadic);
        }
    }
    parser.expect(TokenKind::Semi);
    return true;
}

bool try_parse_nested_record_member(
    Parser& parser,
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field) {
    if (!(parser.check(TokenKind::KwStruct) || parser.check(TokenKind::KwClass) ||
          parser.check(TokenKind::KwUnion)))
        return false;

    bool inner_union = (parser.cur().kind == TokenKind::KwUnion);
    parser.consume();
    Node* inner = parse_struct_or_union(parser, inner_union);
    if (inner) parser.definition_state_.struct_defs.push_back(inner);

    TypeSpec anon_fts{};
    anon_fts.array_size = -1;
    anon_fts.array_rank = 0;
    anon_fts.base = inner_union ? TB_UNION : TB_STRUCT;
    anon_fts.tag = inner ? inner->name : nullptr;
    anon_fts.record_def = (inner && inner->n_fields >= 0) ? inner : nullptr;
    parser.parse_attributes(&anon_fts);

    bool has_declarator = !parser.check(TokenKind::Semi) && !parser.check(TokenKind::RBrace);
    if (has_declarator) {
        const char* fname = nullptr;
        parser.parse_declarator(anon_fts, &fname);
        if (fname) {
            Node* f = parser.make_node(NK_DECL, parser.cur().line);
            f->type = anon_fts;
            f->name = fname;
            check_dup_field(fname);
            fields->push_back(f);
        } else if (inner && inner->name) {
            Node* f = parser.make_node(NK_DECL, parser.cur().line);
            f->type = anon_fts;
            f->name = inner->name;
            f->is_anon_field = true;
            fields->push_back(f);
        }
        while (parser.match(TokenKind::Comma)) {
            TypeSpec fts2{};
            fts2.array_size = -1;
            fts2.array_rank = 0;
            fts2.base = inner_union ? TB_UNION : TB_STRUCT;
            fts2.tag = inner ? inner->name : nullptr;
            fts2.record_def = (inner && inner->n_fields >= 0) ? inner : nullptr;
            const char* fname2 = nullptr;
            parser.parse_declarator(fts2, &fname2);
            if (fname2) {
                Node* f2 = parser.make_node(NK_DECL, parser.cur().line);
                f2->type = fts2;
                f2->name = fname2;
                check_dup_field(fname2);
                fields->push_back(f2);
            }
        }
    } else if (inner && inner->name) {
        Node* f = parser.make_node(NK_DECL, parser.cur().line);
        f->type = anon_fts;
        f->name = inner->name;
        f->is_anon_field = true;
        fields->push_back(f);
    }
    parser.match(TokenKind::Semi);
    return true;
}

bool try_parse_record_enum_member(
    Parser& parser,
    std::vector<Node*>* fields,
    const std::function<void(const char*)>& check_dup_field) {
    if (!parser.check(TokenKind::KwEnum))
        return false;

    parser.consume();
    Node* ed = parse_enum(parser);
    if (ed && parser.active_context_state_.parsing_top_level_context)
        parser.definition_state_.struct_defs.push_back(ed);
    // After the enum body, there may be one or more field declarators:
    // e.g.  enum { X } x;   or   enum E { A, B } kind;
    // If the next token starts a declarator (not ; or }), parse it.
    if (!parser.check(TokenKind::Semi) && !parser.check(TokenKind::RBrace)) {
        TypeSpec fts{};
        fts.array_size = -1;
        fts.array_rank = 0;
        // Base type is the enum (use int as fallback since enums are ints)
        fts.base = TB_INT;
        if (ed && ed->name) {
            // Prefer enum type tag if available
            fts.base = TB_ENUM;
            fts.tag  = ed->name;
            if (const TypeSpec* enum_type = parser.find_typedef_type(parser.find_parser_text_id(ed->name)))
                fts.enum_underlying_base = enum_type->enum_underlying_base;
        }
        while (true) {
            TypeSpec cur_fts = fts;
            const char* fname = nullptr;
            parser.parse_declarator(cur_fts, &fname);
            parser.skip_attributes();
            long long bf_width = -1;
            if (parser.check(TokenKind::Colon)) {
                parser.consume();
                Node* bfw = parse_assign_expr(parser);
                if (bfw)
                    eval_const_int(
                        bfw, &bf_width, &parser.definition_state_.struct_tag_def_map);
            }
            if (fname) {
                Node* f = parser.make_node(NK_DECL, parser.cur().line);
                f->type = cur_fts;
                f->name = fname;
                f->ival = bf_width;  // -1 = not a bitfield; N = N-bit bitfield
                check_dup_field(fname);
                fields->push_back(f);
            }
            if (!parser.match(TokenKind::Comma)) break;
        }
    }
    parser.match(TokenKind::Semi);
    return true;
}

bool is_record_special_member_name(Parser& parser, const std::string& lex,
                                   const std::string& struct_source_name) {
    if (lex == parser.current_struct_tag_text()) return true;
    return !struct_source_name.empty() && lex == struct_source_name;
}

bool try_parse_record_constructor_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (!(parser.is_cpp_mode() && !parser.current_struct_tag_text().empty()))
        return false;

    int probe = parser.core_input_state_.pos;
    while (probe < static_cast<int>(parser.core_input_state_.tokens.size())) {
        if (parser.core_input_state_.tokens[probe].kind == TokenKind::KwConstexpr ||
            parser.core_input_state_.tokens[probe].kind == TokenKind::KwConsteval ||
            (parser.core_input_state_.tokens[probe].kind == TokenKind::Identifier &&
             parser.token_spelling(parser.core_input_state_.tokens[probe]) == "inline")) {
            ++probe;
            continue;
        }
        if (parser.core_input_state_.tokens[probe].kind == TokenKind::KwExplicit) {
            ++probe;
            if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                parser.core_input_state_.tokens[probe].kind == TokenKind::LParen) {
                int depth = 1;
                ++probe;
                while (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                       depth > 0) {
                    if (parser.core_input_state_.tokens[probe].kind == TokenKind::LParen)
                        ++depth;
                    else if (parser.core_input_state_.tokens[probe].kind ==
                             TokenKind::RParen)
                        --depth;
                    ++probe;
                }
            }
            continue;
        }
        break;
    }
    if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[probe].kind == TokenKind::Identifier &&
        is_record_special_member_name(
            parser,
            std::string(parser.token_spelling(parser.core_input_state_.tokens[probe])),
            struct_source_name) &&
        probe + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[probe + 1].kind == TokenKind::LParen) {
        while (parser.pos_ < probe) {
            if (parser.check(TokenKind::KwExplicit))
                consume_optional_cpp_explicit_specifier(&parser);
            else
                parser.consume();
        }
    }

    if (!(parser.check(TokenKind::Identifier) &&
          is_record_special_member_name(
            parser, std::string(parser.token_spelling(parser.cur())),
              struct_source_name) &&
          parser.core_input_state_.pos + 1 <
              static_cast<int>(parser.core_input_state_.tokens.size()) &&
          parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
              TokenKind::LParen)) {
        return false;
    }

    const char* ctor_name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
    parser.consume();  // consume the struct tag name
    parser.consume();  // consume '('
    std::vector<Node*> params;
    bool variadic = false;
    if (!parser.check(TokenKind::RParen)) {
        while (!parser.at_end()) {
            if (parser.check(TokenKind::Ellipsis)) {
                variadic = true;
                parser.consume();
                break;
            }
            if (parser.check(TokenKind::RParen)) break;
            if (!parser.can_start_parameter_type()) break;
            Node* p = parse_param(parser);
            if (p) params.push_back(p);
            if (parser.check(TokenKind::Ellipsis)) {
                variadic = true;
                parser.consume();
                break;
            }
            if (!parser.match(TokenKind::Comma)) break;
        }
    }
    parser.expect(TokenKind::RParen);
    parser.skip_exception_spec();
    parse_optional_cpp20_trailing_requires_clause(parser);
    Node* method = parser.make_node(NK_FUNCTION, parser.cur().line);
    method->type.base = TB_VOID;
    method->name = ctor_name;
    method->execution_domain = parser.pragma_state_.execution_domain;
    method->variadic = variadic;
    method->is_constructor = true;
    method->n_params = static_cast<int>(params.size());
    if (method->n_params > 0) {
        method->params = parser.arena_.alloc_array<Node*>(method->n_params);
        for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
    }
    if (parser.check(TokenKind::Colon)) {
        parser.consume();  // ':'
        std::vector<const char*> init_names;
        std::vector<std::vector<Node*>> init_args_list;
        while (!parser.at_end() && !parser.check(TokenKind::LBrace)) {
            if (!parser.check(TokenKind::Identifier)) break;
            const char* mem_name =
                parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
            parser.consume();  // member name
            parser.expect(TokenKind::LParen);
            std::vector<Node*> args;
            if (!parser.check(TokenKind::RParen)) {
                while (true) {
                    Node* arg = parse_assign_expr(parser);
                    if (arg) args.push_back(arg);
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);
            init_names.push_back(mem_name);
            init_args_list.push_back(std::move(args));
            if (!parser.match(TokenKind::Comma)) break;
        }
        method->n_ctor_inits = static_cast<int>(init_names.size());
        if (method->n_ctor_inits > 0) {
            method->ctor_init_names =
                parser.arena_.alloc_array<const char*>(method->n_ctor_inits);
            method->ctor_init_args =
                parser.arena_.alloc_array<Node**>(method->n_ctor_inits);
            method->ctor_init_nargs =
                parser.arena_.alloc_array<int>(method->n_ctor_inits);
            for (int i = 0; i < method->n_ctor_inits; ++i) {
                method->ctor_init_names[i] = init_names[i];
                method->ctor_init_nargs[i] =
                    static_cast<int>(init_args_list[i].size());
                if (method->ctor_init_nargs[i] > 0) {
                    method->ctor_init_args[i] = parser.arena_.alloc_array<Node*>(
                        method->ctor_init_nargs[i]);
                    for (int j = 0; j < method->ctor_init_nargs[i]; ++j)
                        method->ctor_init_args[i][j] = init_args_list[i][j];
                } else {
                    method->ctor_init_args[i] = nullptr;
                }
            }
        }
    }
    if (parser.check(TokenKind::LBrace)) {
        ParserFunctionParamScopeGuard param_scope(parser, params);
        method->body = parse_block(parser);
    } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::KwDelete) {
        parser.consume();  // '='
        parser.consume();  // 'delete'
        method->is_deleted = true;
        parser.match(TokenKind::Semi);
    } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::KwDefault) {
        parser.consume();  // '='
        parser.consume();  // 'default'
        method->is_defaulted = true;
        parser.match(TokenKind::Semi);
    } else {
        parser.match(TokenKind::Semi);
    }
    methods->push_back(method);
    return true;
}

bool try_parse_record_destructor_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    if (!(parser.is_cpp_mode() && !parser.current_struct_tag_text().empty() &&
          parser.check(TokenKind::Tilde) &&
          parser.core_input_state_.pos + 1 <
              static_cast<int>(parser.core_input_state_.tokens.size()) &&
          parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
              TokenKind::Identifier &&
          is_record_special_member_name(
            parser,
              std::string(parser.token_spelling(
                  parser.core_input_state_.tokens[parser.core_input_state_.pos + 1])),
              struct_source_name))) {
        return false;
    }

    parser.consume();  // consume '~'
    const char* dtor_name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
    parser.consume();  // consume struct tag name
    parser.expect(TokenKind::LParen);
    parser.expect(TokenKind::RParen);
    parser.skip_exception_spec();
    consume_optional_cpp_member_virt_specifier_seq(&parser);
    std::string mangled = std::string("~") + dtor_name;
    Node* method = parser.make_node(NK_FUNCTION, parser.cur().line);
    method->type.base = TB_VOID;
    method->name = parser.arena_.strdup(mangled.c_str());
    method->execution_domain = parser.pragma_state_.execution_domain;
    method->is_destructor = true;
    method->n_params = 0;
    if (parser.check(TokenKind::LBrace)) {
        method->body = parse_block(parser);
    } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::KwDelete) {
        parser.consume();  // '='
        parser.consume();  // 'delete'
        method->is_deleted = true;
        parser.match(TokenKind::Semi);
    } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::KwDefault) {
        parser.consume();  // '='
        parser.consume();  // 'default'
        method->is_defaulted = true;
        parser.match(TokenKind::Semi);
    } else {
        parser.match(TokenKind::Semi);
    }
    methods->push_back(method);
    return true;
}

bool try_parse_record_method_or_field_member(
    Parser& parser,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    const std::function<void(const char*)>& check_dup_field) {
    Parser::ParseContextGuard trace(&parser, __func__);
    // Regular field declaration
    // C++ conversion operators (e.g., operator bool()) have no return
    // type prefix, so KwOperator can appear directly here.
    bool is_conversion_operator = false;
    if (parser.is_cpp_mode()) {
        Parser::TentativeParseGuard operator_guard(parser);
        while (!parser.at_end()) {
            if (parser.check(TokenKind::KwOperator)) {
                is_conversion_operator = true;
                break;
            }
            if (parser.check(TokenKind::KwConst) || parser.check(TokenKind::KwVolatile) ||
                parser.check(TokenKind::KwInline) || parser.check(TokenKind::KwExtern) ||
                parser.check(TokenKind::KwStatic) || parser.check(TokenKind::KwConstexpr) ||
                parser.check(TokenKind::KwConsteval) || parser.check(TokenKind::KwMutable)) {
                parser.consume();
                continue;
            }
            if (parser.check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(&parser);
                continue;
            }
            break;
        }
    }
    if (!is_conversion_operator && !parser.is_type_start()) {
        // unknown token in struct body — skip
        parser.consume();
        return true;
    }

    // Track C++ storage-class specifiers before parse_base_type consumes them.
    bool field_is_static = false;
    bool field_is_constexpr = false;
    if (parser.is_cpp_mode()) {
        Parser::TentativeParseGuard peek_guard(parser);
        while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
            if (parser.check(TokenKind::KwStatic)) { field_is_static = true; parser.consume(); continue; }
            if (parser.check(TokenKind::KwConstexpr)) { field_is_constexpr = true; parser.consume(); continue; }
            // Only peek through qualifiers/storage-class keywords
            if (parser.check(TokenKind::KwConst) || parser.check(TokenKind::KwVolatile) ||
                parser.check(TokenKind::KwInline) || parser.check(TokenKind::KwExtern) ||
                parser.check(TokenKind::KwConsteval) ||
                parser.check(TokenKind::KwMutable)) {
                parser.consume();
                continue;
            }
            if (parser.check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(&parser);
                continue;
            }
            break;
        }
        // peek_guard restores parser.pos_ on scope exit (committed=false)
    }

    TypeSpec fts{};
    if (!is_conversion_operator) {
        fts = parser.parse_base_type();
        parser.parse_attributes(&fts);
    } else if (parser.is_cpp_mode()) {
        while (!parser.at_end()) {
            if (parser.check(TokenKind::KwOperator)) break;
            if (parser.check(TokenKind::KwConst) || parser.check(TokenKind::KwVolatile) ||
                parser.check(TokenKind::KwInline) || parser.check(TokenKind::KwExtern) ||
                parser.check(TokenKind::KwStatic) || parser.check(TokenKind::KwConstexpr) ||
                parser.check(TokenKind::KwConsteval) || parser.check(TokenKind::KwMutable)) {
                parser.consume();
                continue;
            }
            if (parser.check(TokenKind::KwExplicit)) {
                consume_optional_cpp_explicit_specifier(&parser);
                continue;
            }
            break;
        }
    }

    // C++ operator method: <return-type> operator<symbol>(<params>) { ... }
    // Consume pointer/reference declarator tokens between the base type
    // and the 'operator' keyword (e.g., Inner* operator->(), int& operator*()).
    if (parser.is_cpp_mode() && !is_conversion_operator && parser.check(TokenKind::Star)) {
        while (parser.check(TokenKind::Star)) { parser.consume(); fts.ptr_level++; }
    }
    if (parser.is_cpp_mode() && !is_conversion_operator && parser.check(TokenKind::AmpAmp)) {
        parser.consume();
        fts.is_rvalue_ref = true;
    } else if (parser.is_cpp_mode() && !is_conversion_operator && parser.check(TokenKind::Amp)) {
        parser.consume();
        fts.is_lvalue_ref = true;
    }
    if (parser.is_cpp_mode() && (is_conversion_operator || parser.check(TokenKind::KwOperator))) {
        OperatorKind op_kind = OP_NONE;
        const char* op_mangled = nullptr;
        parser.consume(); // eat 'operator'

        // Determine which operator
        std::string conversion_mangled_name;
        if (parser.is_type_start() || parser.can_start_parameter_type() ||
            parser.check(TokenKind::Identifier) ||
            parser.check(TokenKind::ColonColon)) {
            // Conversion operator: operator T() — the token after 'operator'
            // is a type name. This covers both standalone 'operator T()' and
            // prefix forms like 'explicit operator T()' / 'constexpr explicit operator T()'.
            is_conversion_operator = true;
            if (parser.check(TokenKind::Identifier) && !parser.is_type_start() &&
                !parser.can_start_parameter_type()) {
                fts = TypeSpec{};
                fts.base = TB_TYPEDEF;
                fts.tag = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                fts.array_size = -1;
                fts.inner_rank = -1;
                parser.consume();
            } else {
                fts = parser.parse_base_type();
            }
            parser.parse_attributes(&fts);
            if (parser.check(TokenKind::Star)) {
                while (parser.check(TokenKind::Star)) {
                    parser.consume();
                    fts.ptr_level++;
                }
            }
            if (parser.check(TokenKind::AmpAmp)) {
                parser.consume();
                fts.is_rvalue_ref = true;
            } else if (parser.check(TokenKind::Amp)) {
                parser.consume();
                fts.is_lvalue_ref = true;
            }
            if (fts.base == TB_BOOL && fts.ptr_level == 0 &&
                !fts.is_lvalue_ref && !fts.is_rvalue_ref) {
                op_kind = OP_BOOL;
            } else {
                conversion_mangled_name = "operator_conv_";
                append_type_mangled_suffix(conversion_mangled_name, fts);
            }
        } else if (parser.check(TokenKind::KwNew)) {
            parser.consume();
            conversion_mangled_name = parser.check(TokenKind::LBracket)
                ? "operator_new_array"
                : "operator_new";
            if (parser.check(TokenKind::LBracket)) {
                parser.consume();
                parser.expect(TokenKind::RBracket);
            }
        } else if (parser.check(TokenKind::KwDelete)) {
            parser.consume();
            conversion_mangled_name = parser.check(TokenKind::LBracket)
                ? "operator_delete_array"
                : "operator_delete";
            if (parser.check(TokenKind::LBracket)) {
                parser.consume();
                parser.expect(TokenKind::RBracket);
            }
        } else if (parser.check(TokenKind::LBracket)) {
            parser.consume(); parser.expect(TokenKind::RBracket);
            op_kind = OP_SUBSCRIPT;
        } else if (parser.check(TokenKind::Star)) {
            parser.consume();
            op_kind = OP_DEREF;
            conversion_mangled_name = "operator_star_pending";
        } else if (parser.check(TokenKind::Arrow)) {
            parser.consume();
            op_kind = OP_ARROW;
        } else if (parser.check(TokenKind::PlusPlus)) {
            parser.consume();
            op_kind = OP_PRE_INC; // may become OP_POST_INC based on params
        } else if (parser.check(TokenKind::EqualEqual)) {
            parser.consume();
            op_kind = OP_EQ;
        } else if (parser.check(TokenKind::BangEqual)) {
            parser.consume();
            op_kind = OP_NEQ;
        } else if (parser.check(TokenKind::Plus)) {
            parser.consume();
            op_kind = OP_PLUS;
        } else if (parser.check(TokenKind::Minus)) {
            parser.consume();
            op_kind = OP_MINUS;
        } else if (parser.check(TokenKind::Assign)) {
            parser.consume();
            op_kind = OP_ASSIGN;
        } else if (parser.check(TokenKind::LessEqual)) {
            parser.consume();
            op_kind = OP_LE;
        } else if (parser.check(TokenKind::Spaceship)) {
            parser.consume();
            op_kind = OP_SPACESHIP;
        } else if (parser.check(TokenKind::GreaterEqual)) {
            parser.consume();
            op_kind = OP_GE;
        } else if (parser.check(TokenKind::Less)) {
            parser.consume();
            op_kind = OP_LT;
        } else if (parser.check(TokenKind::Greater)) {
            parser.consume();
            op_kind = OP_GT;
        } else if (parser.check(TokenKind::LParen)) {
            // operator() — function call operator
            parser.consume(); // eat '('
            parser.expect(TokenKind::RParen); // eat ')'
            op_kind = OP_CALL;
        } else if (parser.check(TokenKind::Amp)) {
            parser.consume();
            conversion_mangled_name = "operator_amp_pending";
        } else if (const char* extra_mangled = extra_operator_mangled_name(parser.cur().kind)) {
            parser.consume();
            conversion_mangled_name = extra_mangled;
        } else if (parser.check(TokenKind::KwBool)) {
            // operator bool — conversion operator; return type is bool
            parser.consume();
            op_kind = OP_BOOL;
            fts = TypeSpec{};
            fts.base = TB_BOOL;
        } else {
            // Unknown operator token — error
            throw std::runtime_error(
                std::string("unsupported operator overload token '") +
                std::string(parser.token_spelling(parser.cur())) + "' at line " +
                std::to_string(parser.cur().line));
        }

        op_mangled = operator_kind_mangled_name(op_kind);
        if (!conversion_mangled_name.empty()) {
            op_mangled = parser.arena_.strdup(conversion_mangled_name.c_str());
        }

        // Function attributes such as [[nodiscard]] can appear between the
        // operator name and its parameter list.
        parser.skip_attributes();

        // Parse parameter list
        parser.expect(TokenKind::LParen);
        std::vector<Node*> params;
        bool variadic = false;
        if (!parser.check(TokenKind::RParen)) {
            while (!parser.at_end()) {
                if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                if (parser.check(TokenKind::RParen)) break;
                if (!parser.can_start_parameter_type()) break;
                Node* p = parse_param(parser);
                if (p) params.push_back(p);
                if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                if (!parser.match(TokenKind::Comma)) break;
            }
        }
        parser.expect(TokenKind::RParen);

        // Distinguish prefix vs postfix operator++ by parameter count:
        // prefix: operator++() — 0 params; postfix: operator++(int) — 1 param
        if (op_kind == OP_PRE_INC && !params.empty()) {
            op_kind = OP_POST_INC;
            op_mangled = operator_kind_mangled_name(op_kind);
        }
        if (!conversion_mangled_name.empty()) {
            finalize_pending_operator_name(conversion_mangled_name, params.size());
            op_mangled = parser.arena_.strdup(conversion_mangled_name.c_str());
        }

        // Parse trailing qualifiers (const, constexpr, consteval)
        bool is_method_const = false;
        bool is_method_constexpr = false;
        bool is_method_consteval = false;
        bool is_method_lvalue_ref = false;
        bool is_method_rvalue_ref = false;
        while (true) {
            if (parser.match(TokenKind::KwConst)) { is_method_const = true; }
            else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) {
                is_method_constexpr = true;
            } else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConsteval)) {
                is_method_consteval = true;
            } else {
                break;
            }
        }
        // Skip C++ ref-qualifiers: & or &&
        if (parser.is_cpp_mode()) {
            if (parser.match(TokenKind::AmpAmp)) is_method_rvalue_ref = true;
            else if (parser.match(TokenKind::Amp)) is_method_lvalue_ref = true;
        }
        parser.skip_exception_spec();
        if (parser.is_cpp_mode() && parser.match(TokenKind::Arrow)) {
            fts = parser.parse_type_name();
            parser.parse_attributes(&fts);
        }
        parse_optional_cpp20_trailing_requires_clause(parser);
        consume_optional_cpp_member_virt_specifier_seq(&parser);

        Node* method = parser.make_node(NK_FUNCTION, parser.cur().line);
        method->type = fts;
        method->name = parser.arena_.strdup(op_mangled);
        method->execution_domain = parser.pragma_state_.execution_domain;
        method->operator_kind = op_kind;
        method->variadic = variadic;
        method->is_const_method = is_method_const;
        method->is_lvalue_ref_method = is_method_lvalue_ref;
        method->is_rvalue_ref_method = is_method_rvalue_ref;
        method->is_constexpr = is_method_constexpr;
        method->is_consteval = is_method_consteval;
        method->n_params = static_cast<int>(params.size());
        if (method->n_params > 0) {
            method->params = parser.arena_.alloc_array<Node*>(method->n_params);
            for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
        }
        if (parser.check(TokenKind::LBrace)) {
            ParserFunctionParamScopeGuard param_scope(parser, params);
            method->body = parse_block(parser);
        } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                   parser.core_input_state_.pos + 1 <
                       static_cast<int>(parser.core_input_state_.tokens.size()) &&
                   parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                       TokenKind::KwDelete) {
            parser.consume(); // '='
            parser.consume(); // 'delete'
            method->is_deleted = true;
            parser.match(TokenKind::Semi);
        } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                   parser.core_input_state_.pos + 1 <
                       static_cast<int>(parser.core_input_state_.tokens.size()) &&
                   parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                       TokenKind::KwDefault) {
            parser.consume(); // '='
            parser.consume(); // 'default'
            method->is_defaulted = true;
            parser.match(TokenKind::Semi);
        } else {
            parser.match(TokenKind::Semi);
        }
        methods->push_back(method);
        return true;
    }

    // Handle anonymous bitfield: just ': expr;'
    if (parser.check(TokenKind::Colon)) {
        parser.consume();
        parse_assign_expr(parser);  // skip bitfield width
        parser.match(TokenKind::Semi);
        return true;
    }

    // If semicolon follows immediately: anonymous struct field (no name)
    if (parser.check(TokenKind::Semi)) {
        parser.match(TokenKind::Semi);
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
        parser.parse_declarator(cur_fts, &fname, &fn_ptr_params,
                         &n_fn_ptr_params, &fn_ptr_variadic);
        parser.skip_attributes();

        if (fname && parser.check(TokenKind::LParen)) {
            parser.consume();
            std::vector<Node*> params;
            bool variadic = false;
            if (!parser.check(TokenKind::RParen)) {
                while (!parser.at_end()) {
                    if (parser.check(TokenKind::Ellipsis)) {
                        variadic = true;
                        parser.consume();
                        break;
                    }
                    if (parser.check(TokenKind::RParen)) break;
                    if (!parser.can_start_parameter_type()) break;
                    Node* p = parse_param(parser);
                    if (p) params.push_back(p);
                    if (parser.check(TokenKind::Ellipsis)) {
                        variadic = true;
                        parser.consume();
                        break;
                    }
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);
            bool is_method_const = false;
            bool is_method_constexpr = false;
            bool is_method_consteval = false;
            bool is_method_lvalue_ref = false;
            bool is_method_rvalue_ref = false;
            while (true) {
                if (parser.match(TokenKind::KwConst)) { is_method_const = true; }
                else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) {
                    is_method_constexpr = true;
                } else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConsteval)) {
                    is_method_consteval = true;
                } else {
                    break;
                }
            }
            if (parser.is_cpp_mode()) {
                if (parser.match(TokenKind::AmpAmp)) is_method_rvalue_ref = true;
                else if (parser.match(TokenKind::Amp)) is_method_lvalue_ref = true;
            }
            parser.skip_exception_spec();
            if (parser.is_cpp_mode() && parser.match(TokenKind::Arrow)) {
                cur_fts = parser.parse_type_name();
                parser.parse_attributes(&cur_fts);
            }
            parse_optional_cpp20_trailing_requires_clause(parser);
            consume_optional_cpp_member_virt_specifier_seq(&parser);
            Node* method = parser.make_node(NK_FUNCTION, parser.cur().line);
            method->type = cur_fts;
            method->name = fname;
            method->execution_domain = parser.pragma_state_.execution_domain;
            method->variadic = variadic;
            method->is_const_method = is_method_const;
            method->is_lvalue_ref_method = is_method_lvalue_ref;
            method->is_rvalue_ref_method = is_method_rvalue_ref;
            method->is_constexpr = is_method_constexpr;
            method->is_consteval = is_method_consteval;
            method->n_params = static_cast<int>(params.size());
            if (method->n_params > 0) {
                method->params = parser.arena_.alloc_array<Node*>(method->n_params);
                for (int i = 0; i < method->n_params; ++i) method->params[i] = params[i];
            }
            if (parser.check(TokenKind::LBrace)) {
                ParserFunctionParamScopeGuard param_scope(parser, params);
                method->body = parse_block(parser);
            } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                       parser.core_input_state_.pos + 1 <
                           static_cast<int>(parser.core_input_state_.tokens.size()) &&
                       parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                           TokenKind::KwDelete) {
                parser.consume(); // '='
                parser.consume(); // 'delete'
                method->is_deleted = true;
                parser.match(TokenKind::Semi);
            } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                       parser.core_input_state_.pos + 1 <
                           static_cast<int>(parser.core_input_state_.tokens.size()) &&
                       parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                           TokenKind::KwDefault) {
                parser.consume(); // '='
                parser.consume(); // 'default'
                method->is_defaulted = true;
                parser.match(TokenKind::Semi);
            } else {
                parser.match(TokenKind::Semi);
            }
            methods->push_back(method);
            if (!parser.match(TokenKind::Comma)) break;
            continue;
        }

        long long bf_width = -1;
        if (parser.check(TokenKind::Colon)) {
            parser.consume();
            Node* bfw = parse_assign_expr(parser);
            if (bfw)
                eval_const_int(
                    bfw, &bf_width, &parser.definition_state_.struct_tag_def_map);
        }

        Node* field_init_expr = nullptr;
        if (parser.is_cpp_mode() && parser.check(TokenKind::Assign)) {
            parser.consume(); // eat '='
            if (field_is_static) {
                try {
                    field_init_expr = parse_assign_expr(parser);
                } catch (...) {
                    field_init_expr = nullptr;
                }
            }
            if (!field_init_expr) {
                int depth = 0, angle_depth = 0;
                while (!parser.at_end()) {
                    if (parser.check(TokenKind::LParen) || parser.check(TokenKind::LBrace) ||
                        parser.check(TokenKind::LBracket)) { ++depth; parser.consume(); }
                    else if (parser.check(TokenKind::RParen) || parser.check(TokenKind::RBrace) ||
                             parser.check(TokenKind::RBracket)) {
                        if (depth == 0) break;
                        --depth; parser.consume();
                    }
                    else if (parser.check(TokenKind::Less) && depth == 0) {
                        ++angle_depth; parser.consume();
                    }
                    else if (parser.check(TokenKind::Greater) && angle_depth > 0 && depth == 0) {
                        --angle_depth; parser.consume();
                    }
                    else if (parser.check(TokenKind::GreaterGreater) && angle_depth > 0 && depth == 0) {
                        angle_depth -= std::min(angle_depth, 2);
                        parser.consume();
                    }
                    else if ((parser.check(TokenKind::Semi) || parser.check(TokenKind::Comma)) &&
                             depth == 0 && angle_depth == 0) break;
                    else parser.consume();
                }
            }
        }

        if (fname) {
            Node* f = parser.make_node(NK_DECL, parser.cur().line);
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
                if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                    f->fn_ptr_params = info->params;
                    f->n_fn_ptr_params = info->n_params;
                    f->fn_ptr_variadic = info->variadic;
                }
            }
            check_dup_field(fname);
            fields->push_back(f);
        }

        if (!parser.match(TokenKind::Comma)) break;
    }
    parser.match(TokenKind::Semi);
    return true;
}

bool try_parse_record_type_like_member_dispatch(
    Parser& parser,
    std::vector<Node*>* fields,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (try_parse_record_using_member(parser, member_typedef_names,
                                      member_typedef_types)) {
        return true;
    }
    if (try_parse_nested_record_member(parser, fields, check_dup_field))
        return true;
    if (try_parse_record_enum_member(parser, fields, check_dup_field))
        return true;
    return try_parse_record_typedef_member(parser, member_typedef_names,
                                           member_typedef_types);
}

bool try_parse_record_member_dispatch(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (try_parse_record_type_like_member_dispatch(
            parser, fields, member_typedef_names, member_typedef_types,
            check_dup_field)) {
        return true;
    }
    if (try_parse_record_special_member_dispatch(parser, struct_source_name,
                                                 methods))
        return true;
    return try_parse_record_method_or_field_member(parser, fields, methods,
                                                   check_dup_field);
}

bool try_parse_record_special_member_dispatch(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* methods) {
    if (try_parse_record_constructor_member(parser, struct_source_name,
                                            methods)) {
        return true;
    }
    return try_parse_record_destructor_member(parser, struct_source_name,
                                              methods);
}

bool try_parse_record_member_with_template_prelude(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    Parser::RecordTemplatePreludeGuard tmpl_guard(&parser);
    parse_record_template_member_prelude(parser, &tmpl_guard.injected_type_params,
                                         &tmpl_guard.pushed_template_scope);
    parse_optional_cpp20_requires_clause(parser);
    if (try_skip_record_friend_member(parser)) return true;
    if (try_skip_record_static_assert_member(parser, methods)) return true;
    return try_parse_record_member_dispatch(
        parser, struct_source_name, fields, methods, member_typedef_names,
        member_typedef_types, check_dup_field);
}

bool begin_record_member_parse(Parser& parser) {
    parser.skip_attributes();
    return !parser.check(TokenKind::RBrace);
}

bool try_parse_record_member_prelude(Parser& parser,
                                     std::vector<Node*>* methods) {
    if (try_parse_record_access_label(parser)) return true;
    if (try_skip_record_friend_member(parser)) return true;
    return try_skip_record_static_assert_member(parser, methods);
}

bool try_parse_record_member(
    Parser& parser,
    const std::string& struct_source_name,
    std::vector<Node*>* fields,
    std::vector<Node*>* methods,
    std::vector<const char*>* member_typedef_names,
    std::vector<TypeSpec>* member_typedef_types,
    const std::function<void(const char*)>& check_dup_field) {
    if (!begin_record_member_parse(parser)) return false;
    if (try_parse_record_member_prelude(parser, methods)) return true;
    return try_parse_record_member_with_template_prelude(
        parser, struct_source_name, fields, methods, member_typedef_names,
        member_typedef_types, check_dup_field);
}

bool try_parse_record_body_member(
    Parser& parser,
    const std::string& struct_source_name,
    Parser::RecordBodyState* body_state,
    const std::function<void(const char*)>& check_dup_field) {
    if (!body_state)
        return false;

    const int member_start_pos = parser.pos_;
    try {
        return try_parse_record_member(parser, struct_source_name,
                                       &body_state->fields,
                                       &body_state->methods,
                                       &body_state->member_typedef_names,
                                       &body_state->member_typedef_types,
                                       check_dup_field);
    } catch (const std::exception&) {
        const Parser::RecordMemberRecoveryResult recovery =
            recover_record_member_parse_error(parser, member_start_pos);
        if (recovery != Parser::RecordMemberRecoveryResult::SyncedAtSemicolon &&
            recovery != Parser::RecordMemberRecoveryResult::StoppedAtNextMember)
            throw;
        return false;
    }
}

void begin_record_body_context(Parser& parser,
                               const char* tag,
                               TextId tag_text_id,
                               const char* template_origin_name,
                               std::string* saved_struct_tag,
                               std::string* struct_source_name) {
    // Make the current record name available for self-type parsing within the
    // body before member dispatch starts.
    if (parser.is_cpp_mode() && tag && tag[0])
        parser.register_typedef_name(
            parser.parser_text_id_for_token(kInvalidText, tag), false);

    if (saved_struct_tag)
        *saved_struct_tag = parser.active_context_state_.current_struct_tag;
    if (tag && tag[0]) {
        parser.set_current_struct_tag(tag_text_id, tag);
    } else {
        parser.clear_current_struct_tag();
    }

    if (struct_source_name)
        struct_source_name->clear();
    if (!(template_origin_name && template_origin_name[0]))
        return;

    if (struct_source_name)
        *struct_source_name = template_origin_name;
    if (!parser.is_cpp_mode())
        return;

    const TextId template_origin_text_id =
        parser.parser_text_id_for_token(kInvalidText, template_origin_name);
    parser.register_typedef_name(template_origin_text_id, false);
    const bool has_existing_template_origin_type =
        std::strstr(template_origin_name, "::") == nullptr
            ? parser.has_visible_typedef_type(template_origin_text_id)
            : parser.has_typedef_type(template_origin_text_id);
    if (!has_existing_template_origin_type) {
        parser.register_tag_type_binding(
            template_origin_text_id, TB_STRUCT, tag,
            tag ? parser.parser_text_id_for_token(kInvalidText, tag)
                : kInvalidText);
    }
}

void parse_record_body(
    Parser& parser,
    const std::string& struct_source_name,
    Parser::RecordBodyState* body_state) {
    if (!body_state)
        return;

    std::unordered_set<TextId> field_names_seen;
    auto check_dup_field =
        make_record_field_duplicate_checker(&parser, &field_names_seen);

    while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
        if (try_parse_record_body_member(parser, struct_source_name, body_state,
                                         check_dup_field)) {
            continue;
        }
    }
}

void parse_record_body_with_context(
    Parser& parser,
    const char* tag,
    TextId tag_text_id,
    const char* template_origin_name,
    Parser::RecordBodyState* body_state) {
    const TextId saved_struct_tag_text_id =
        parser.active_context_state_.current_struct_tag_text_id;
    std::string saved_struct_tag =
        parser.active_context_state_.current_struct_tag;
    std::string struct_source_name;
    begin_record_body_context(parser, tag, tag_text_id, template_origin_name,
                              &saved_struct_tag, &struct_source_name);
    parse_record_body(parser, struct_source_name, body_state);
    finish_record_body_context(parser, saved_struct_tag);
    restore_current_struct_tag(parser, saved_struct_tag_text_id,
                               saved_struct_tag);
}

void finish_record_body_context(Parser& parser,
                                const std::string& saved_struct_tag) {
    parser.expect(TokenKind::RBrace);
    restore_current_struct_tag(parser,
                               parser.find_parser_text_id(saved_struct_tag),
                               saved_struct_tag);
}

void parse_record_definition_prelude(
    Parser& parser,
    int line,
    TypeSpec* attr_ts,
    const char** tag,
    TextId* tag_text_id,
    const char** template_origin_name,
    std::vector<Parser::TemplateArgParseResult>* specialization_args,
    std::vector<TypeSpec>* base_types) {
    if (!attr_ts || !tag || !tag_text_id || !template_origin_name ||
        !specialization_args || !base_types) {
        return;
    }

    *attr_ts = {};
    *tag = nullptr;
    *tag_text_id = kInvalidText;
    *template_origin_name = nullptr;
    specialization_args->clear();
    base_types->clear();

    parse_decl_attrs_for_record(parser, line, attr_ts);

    Parser::QualifiedNameRef tag_qn;
    bool has_tag_qn = false;
    if (parser.is_cpp_mode()) {
        if (parser.peek_qualified_name(&tag_qn, /*allow_global=*/true)) {
            tag_qn = parser.parse_qualified_name(/*allow_global=*/true);
            has_tag_qn = true;
            const std::string spelled =
                parser.qualified_name_text(tag_qn,
                                           /*include_global_prefix=*/false);
            *tag = parser.arena_.strdup(spelled.c_str());
            *tag_text_id = tag_qn.is_unqualified_atom() ? tag_qn.base_text_id
                                                        : kInvalidText;
        }
    } else if (parser.check(TokenKind::Identifier)) {
        *tag_text_id = parser.cur().text_id;
        *tag = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        parser.consume();
    }
    parse_decl_attrs_for_record(parser, line, attr_ts);

    if (*tag && parser.is_cpp_mode() && parser.check(TokenKind::Less)) {
        int probe = parser.core_input_state_.pos;
        int depth = 0;
        bool balanced = false;
        while (probe < static_cast<int>(parser.core_input_state_.tokens.size())) {
            if (parser.core_input_state_.tokens[probe].kind == TokenKind::Less) ++depth;
            else if (parser.core_input_state_.tokens[probe].kind == TokenKind::Greater) {
                if (--depth <= 0) {
                    ++probe;
                    balanced = true;
                    break;
                }
            } else if (parser.core_input_state_.tokens[probe].kind ==
                       TokenKind::GreaterGreater) {
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
            balanced &&
            probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            (parser.core_input_state_.tokens[probe].kind == TokenKind::LBrace ||
             parser.core_input_state_.tokens[probe].kind == TokenKind::Colon ||
             parser.core_input_state_.tokens[probe].kind == TokenKind::Semi);
        if (is_specialization) {
            *template_origin_name = parser.arena_.strdup(*tag);
            bool parse_ok = true;
            {
                Parser::TentativeParseGuard guard(parser);
                try {
                    const Node* primary_tpl = nullptr;
                    if (has_tag_qn &&
                        (!tag_qn.qualifier_segments.empty() ||
                         tag_qn.is_global_qualified)) {
                        primary_tpl = parser.find_template_struct_primary(tag_qn);
                    } else {
                        primary_tpl = parser.find_template_struct_primary(
                            parser.current_namespace_context_id(),
                            parser.parser_text_id_for_token(kInvalidText,
                                                            *tag));
                    }
                    if (!parser.parse_template_argument_list(
                            specialization_args, primary_tpl)) {
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
            if (!parse_ok && parser.check(TokenKind::Less)) {
                int angle_depth = 1;
                parser.consume();
                while (!parser.at_end() && angle_depth > 0) {
                    if (parser.check(TokenKind::Less)) ++angle_depth;
                    else if (parser.check_template_close()) {
                        --angle_depth;
                        if (angle_depth > 0) {
                            parser.match_template_close();
                            continue;
                        }
                        break;
                    }
                    parser.consume();
                }
                if (parser.check_template_close()) parser.match_template_close();
            }
            (void)parse_ok;
            std::string mangled(*tag);
            mangled += "__spec_";
            mangled += std::to_string(parser.definition_state_.anon_counter++);
            *tag = parser.arena_.strdup(mangled.c_str());
        }
    }

    // C++ class-virt-specifier support: accept the contextual `final`
    // keyword between the record name and any base-clause/body.
    if (parser.is_cpp_mode() && parser.check(TokenKind::KwFinal)) {
        parser.consume();
    }

    parse_record_base_clause(parser, base_types);
}

Node* parse_record_tag_setup(
    Parser& parser,
    int line,
    bool is_union,
    const char** tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args) {
    if (!parser.check(TokenKind::LBrace)) {
        const char* resolved_tag = tag ? *tag : nullptr;
        const char* source_tag = resolved_tag;
        if (!resolved_tag) {
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_%d",
                     parser.definition_state_.anon_counter++);
            resolved_tag = parser.arena_.strdup(buf);
        } else {
            const TextId resolved_tag_text_id =
                parser.parser_text_id_for_token(kInvalidText, resolved_tag);
            const std::string qtag = parser.render_name_in_context(
                parser.current_namespace_context_id(),
                resolved_tag_text_id);
            resolved_tag = parser.arena_.strdup(qtag.c_str());
        }

        if (tag)
            *tag = resolved_tag;

        Node* ref = nullptr;
        if (template_origin_name && template_origin_name[0] &&
            !specialization_args.empty()) {
            std::vector<TypeSpec> empty_base_types;
            ref = build_record_definition_node(
                parser, line, is_union, resolved_tag, template_origin_name,
                attr_ts, specialization_args, empty_base_types);
            ref->n_fields = -1;  // forward-declared explicit specialization
        } else {
            ref = parser.make_node(NK_STRUCT_DEF, line);
            ref->name = resolved_tag;
            ref->is_union = is_union;
            ref->n_fields = -1;  // -1 = forward reference (no body)
        }
        if (parser.is_cpp_mode() && resolved_tag && resolved_tag[0]) {
            const TextId resolved_tag_text_id =
                parser.parser_text_id_for_token(kInvalidText, resolved_tag);
            parser.register_tag_type_binding(resolved_tag_text_id,
                                             is_union ? TB_UNION : TB_STRUCT,
                                             resolved_tag, resolved_tag_text_id);
            if (source_tag && source_tag[0] &&
                std::strcmp(source_tag, resolved_tag) != 0) {
                parser.register_tag_type_binding(
                    parser.parser_text_id_for_token(kInvalidText, source_tag),
                    is_union ? TB_UNION : TB_STRUCT, resolved_tag,
                    resolved_tag_text_id);
            }
        }
        if (parser.is_cpp_mode() &&
            parser.active_context_state_.parsing_top_level_context)
            parser.definition_state_.struct_defs.push_back(ref);
        return ref;
    }

    const char* resolved_tag = tag ? *tag : nullptr;
    if (!resolved_tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_%d",
                 parser.definition_state_.anon_counter++);
        resolved_tag = parser.arena_.strdup(buf);
    } else {
        const std::string qtag =
            std::strstr(resolved_tag, "::")
                ? std::string(resolved_tag)
                : parser.render_name_in_context(
                      parser.current_namespace_context_id(),
                      parser.parser_text_id_for_token(kInvalidText, resolved_tag));
        if (parser.definition_state_.defined_struct_tags.count(qtag)) {
            if (parser.active_context_state_.parsing_top_level_context &&
                !parser.is_cpp_mode()) {
                throw std::runtime_error(std::string("redefinition of ") +
                                         (is_union ? "union " : "struct ") +
                                         resolved_tag);
            }
            char buf[64];
            snprintf(buf, sizeof(buf), "%s.__shadow_%d", resolved_tag,
                     parser.definition_state_.anon_counter++);
            resolved_tag = parser.arena_.strdup(buf);
        }
        parser.definition_state_.defined_struct_tags.insert(qtag);
    }

    if (tag)
        *tag = resolved_tag;

    return nullptr;
}

Node* build_record_definition_node(
    Parser& parser,
    int line,
    bool is_union,
    const char* tag,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types) {
    Node* sd = parser.make_node(NK_STRUCT_DEF, line);
    sd->name = tag;
    sd->is_union = is_union;
    sd->template_origin_name = template_origin_name;

    if (!base_types.empty()) {
        sd->n_bases = static_cast<int>(base_types.size());
        sd->base_types = parser.arena_.alloc_array<TypeSpec>(sd->n_bases);
        for (int i = 0; i < sd->n_bases; ++i)
            sd->base_types[i] = base_types[i];
    }

    // __attribute__((packed)) => pack_align=1; otherwise use #pragma pack state
    sd->pack_align =
        attr_ts.is_packed ? 1 : parser.pragma_state_.pack_alignment;
    sd->struct_align = attr_ts.align_bytes;  // __attribute__((aligned(N)))

    if (!specialization_args.empty()) {
        sd->n_template_args = static_cast<int>(specialization_args.size());
        sd->template_arg_types =
            parser.arena_.alloc_array<TypeSpec>(sd->n_template_args);
        sd->template_arg_is_value =
            parser.arena_.alloc_array<bool>(sd->n_template_args);
        sd->template_arg_values =
            parser.arena_.alloc_array<long long>(sd->n_template_args);
        sd->template_arg_nttp_names =
            parser.arena_.alloc_array<const char*>(sd->n_template_args);
        sd->template_arg_exprs =
            parser.arena_.alloc_array<Node*>(sd->n_template_args);
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

Node* parse_record_definition_after_tag_setup(
    Parser& parser,
    int line,
    bool is_union,
    const char* tag,
    TextId tag_text_id,
    const char* template_origin_name,
    const TypeSpec& attr_ts,
    const std::vector<Parser::TemplateArgParseResult>& specialization_args,
    const std::vector<TypeSpec>& base_types) {
    const char* source_tag = tag;
    Node* sd = build_record_definition_node(parser, line, is_union, tag,
                                            template_origin_name, attr_ts,
                                            specialization_args, base_types);
    parse_record_definition_body(parser, sd, is_union, source_tag, tag,
                                 tag_text_id,
                                 template_origin_name);
    return sd;
}

void apply_record_trailing_type_attributes(Parser& parser, Node* sd) {
    if (!sd || !parser.check(TokenKind::KwAttribute))
        return;

    int save = parser.pos_;
    TypeSpec trailing_attr{};
    parser.parse_attributes(&trailing_attr);
    if (trailing_attr.is_packed && sd->pack_align == 0)
        sd->pack_align = 1;
    if (trailing_attr.align_bytes > 0 && sd->struct_align == 0)
        sd->struct_align = trailing_attr.align_bytes;
    // Restore position so the caller can re-parse attributes for the declaration.
    parser.pos_ = save;
}

void store_record_body_members(
    Parser& parser,
    Node* sd,
    const Parser::RecordBodyState& body_state) {
    if (!sd)
        return;

    sd->n_fields = static_cast<int>(body_state.fields.size());
    if (sd->n_fields > 0) {
        sd->fields = parser.arena_.alloc_array<Node*>(sd->n_fields);
        for (int i = 0; i < sd->n_fields; ++i)
            sd->fields[i] = body_state.fields[i];
    }

    sd->n_member_typedefs =
        static_cast<int>(body_state.member_typedef_names.size());
    if (sd->n_member_typedefs > 0) {
        sd->member_typedef_names =
            parser.arena_.alloc_array<const char*>(sd->n_member_typedefs);
        sd->member_typedef_types =
            parser.arena_.alloc_array<TypeSpec>(sd->n_member_typedefs);
        for (int i = 0; i < sd->n_member_typedefs; ++i) {
            sd->member_typedef_names[i] = body_state.member_typedef_names[i];
            sd->member_typedef_types[i] = body_state.member_typedef_types[i];
        }
    }

    sd->n_children = static_cast<int>(body_state.methods.size());
    if (sd->n_children > 0) {
        sd->children = parser.arena_.alloc_array<Node*>(sd->n_children);
        for (int i = 0; i < sd->n_children; ++i)
            sd->children[i] = body_state.methods[i];
    }
}

void register_record_definition(Parser& parser,
                                Node* sd,
                                bool is_union,
                                const char* source_tag) {
    if (!sd || !(source_tag && source_tag[0]))
        return;

    const std::string canonical =
        std::strstr(source_tag, "::")
            ? std::string(source_tag)
            : parser.render_name_in_context(
                  parser.current_namespace_context_id(),
                  parser.parser_text_id_for_token(kInvalidText, source_tag));
    sd->name = parser.arena_.strdup(canonical.c_str());
    parser.apply_decl_namespace(sd, parser.current_namespace_context_id(),
                                source_tag);
    parser.definition_state_.struct_tag_def_map[source_tag] = sd;
    parser.definition_state_.struct_tag_def_map[sd->name] = sd;

    if (!parser.is_cpp_mode() || !(sd->name && sd->name[0]))
        return;

    const TextId canonical_tag_text_id =
        parser.parser_text_id_for_token(kInvalidText, sd->name);
    parser.register_tag_type_binding(canonical_tag_text_id,
                                     is_union ? TB_UNION : TB_STRUCT,
                                     sd->name, canonical_tag_text_id);
    if (source_tag && source_tag[0] && std::strcmp(source_tag, sd->name) != 0) {
        parser.register_tag_type_binding(
            parser.parser_text_id_for_token(kInvalidText, source_tag),
            is_union ? TB_UNION : TB_STRUCT, sd->name,
            canonical_tag_text_id);
    }
}

void register_record_member_typedef_bindings(Parser& parser, Node* sd,
                                             const char* source_tag) {
    if (!sd || sd->n_member_typedefs <= 0 || !sd->member_typedef_names ||
        !sd->member_typedef_types) {
        return;
    }

    const int context_id =
        sd->namespace_context_id >= 0 ? sd->namespace_context_id
                                      : parser.current_namespace_context_id();
    for (int i = 0; i < sd->n_member_typedefs; ++i) {
        const char* member_name = sd->member_typedef_names[i];
        if (!(member_name && member_name[0])) continue;
        const TextId member_text_id =
            parser.parser_text_id_for_token(kInvalidText, member_name);
        const QualifiedNameKey member_key =
            sd->unqualified_text_id != kInvalidText
                ? parser.record_member_typedef_key_in_context(
                      context_id, sd->unqualified_text_id, member_text_id)
                : QualifiedNameKey{};
        const bool has_template_dependent_context =
            sd->n_template_params > 0 ||
            (sd->template_origin_name && sd->template_origin_name[0]) ||
            !parser.template_state_.template_scope_stack.empty();
        if (has_template_dependent_context &&
            member_key.base_text_id != kInvalidText) {
            parser.register_dependent_record_member_typedef_binding(
                member_key, sd->member_typedef_types[i]);
        }
    }
}

void finalize_record_definition(
    Parser& parser,
    Node* sd,
    bool is_union,
    const char* source_tag,
    const Parser::RecordBodyState& body_state) {
    apply_record_trailing_type_attributes(parser, sd);
    store_record_body_members(parser, sd, body_state);
    register_record_definition(parser, sd, is_union, source_tag);
    register_record_member_typedef_bindings(parser, sd, source_tag);
    parser.definition_state_.struct_defs.push_back(sd);
}

void parse_record_definition_body(Parser& parser,
                                  Node* sd,
                                  bool is_union,
                                  const char* source_tag,
                                  const char* tag,
                                  TextId tag_text_id,
                                  const char* template_origin_name) {
    parser.consume();  // consume {

    Parser::RecordBodyState body_state;
    parse_record_body_with_context(parser, tag, tag_text_id, template_origin_name,
                                   &body_state);
    finalize_record_definition(parser, sd, is_union, source_tag, body_state);
}

Node* parse_struct_or_union(Parser& parser, bool is_union) {
    int ln = parser.cur().line;
    TypeSpec attr_ts{};
    const char* tag = nullptr;
    TextId tag_text_id = kInvalidText;
    const char* template_origin_name = nullptr;
    std::vector<ParsedTemplateArg> specialization_args;
    std::vector<TypeSpec> base_types;
    parse_record_definition_prelude(parser, ln, &attr_ts, &tag, &tag_text_id,
                                    &template_origin_name,
                                    &specialization_args, &base_types);

    if (Node* ref = parse_record_tag_setup(parser, ln, is_union, &tag,
                                           template_origin_name, attr_ts,
                                           specialization_args))
        return ref;

    return parse_record_definition_after_tag_setup(
        parser, ln, is_union, tag, tag_text_id, template_origin_name, attr_ts,
        specialization_args, base_types);
}

// ── enum parsing ─────────────────────────────────────────────────────────────

Node* parse_enum(Parser& parser) {
    int ln = parser.cur().line;
    parser.skip_attributes();

    bool is_scoped_enum = false;
    if (parser.check(TokenKind::KwClass) || parser.check(TokenKind::KwStruct)) {
        is_scoped_enum = true;
        parser.consume();
        parser.skip_attributes();
    }

    const char* tag = nullptr;
    if (parser.check(TokenKind::Identifier)) {
        tag = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        parser.consume();
    }
    parser.skip_attributes();

    TypeBase enum_underlying_base = TB_INT;
    if (parser.match(TokenKind::Colon)) {
        // Preserve fixed underlying-type metadata for layout-sensitive queries.
        TypeSpec underlying_ts = parser.parse_base_type();
        underlying_ts = parser.resolve_typedef_type_chain(underlying_ts);
        enum_underlying_base = effective_scalar_base(underlying_ts);
        parser.skip_attributes();
    }

    auto register_enum_type = [&](const char* source_tag, const char* canonical_tag) {
        if (!source_tag || !source_tag[0] || !canonical_tag || !canonical_tag[0])
            return;
        const TextId canonical_tag_text_id =
            parser.parser_text_id_for_token(kInvalidText, canonical_tag);
        parser.register_tag_type_binding(
            parser.parser_text_id_for_token(kInvalidText, source_tag), TB_ENUM,
            canonical_tag, canonical_tag_text_id, enum_underlying_base);
        if (strcmp(source_tag, canonical_tag) != 0)
            parser.register_tag_type_binding(canonical_tag_text_id, TB_ENUM,
                                             canonical_tag,
                                             canonical_tag_text_id,
                                             enum_underlying_base);
    };

    if (!parser.check(TokenKind::LBrace)) {
        if (!tag) {
            char buf[32];
            snprintf(buf, sizeof(buf), "_anon_enum_%d",
                     parser.definition_state_.anon_counter++);
            tag = parser.arena_.strdup(buf);
        }
        Node* ref = parser.make_node(NK_ENUM_DEF, ln);
        ref->name = parser.arena_.strdup(parser.render_name_in_context(
            parser.current_namespace_context_id(),
            parser.parser_text_id_for_token(kInvalidText, tag)).c_str());
        parser.apply_decl_namespace(ref, parser.current_namespace_context_id(), tag);
        ref->n_enum_variants = -1;
        register_enum_type(tag, ref->name);
        return ref;
    }

    if (!tag) {
        char buf[32];
        snprintf(buf, sizeof(buf), "_anon_enum_%d",
                 parser.definition_state_.anon_counter++);
        tag = parser.arena_.strdup(buf);
    }

    Node* ed = parser.make_node(NK_ENUM_DEF, ln);
    ed->name = parser.arena_.strdup(parser.render_name_in_context(
        parser.current_namespace_context_id(),
        parser.parser_text_id_for_token(kInvalidText, tag)).c_str());
    parser.apply_decl_namespace(ed, parser.current_namespace_context_id(), tag);
    register_enum_type(tag, ed->name);

    parser.consume();  // consume {

    std::vector<const char*> names;
    std::vector<TextId> name_text_ids;
    std::vector<long long>   vals;
    std::unordered_set<TextId> seen_names;
    ParserEnumConstTable local_enum_consts = parser.binding_state_.enum_consts;
    long long cur_val = 0;

    while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
        parser.skip_attributes();
        if (!parser.check(TokenKind::Identifier)) { parser.consume(); continue; }
        const TextId vname_text_id = parser.cur().text_id;
        const char* vname =
            parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        std::string vname_s(vname ? vname : "");
        if (vname_text_id != kInvalidText && seen_names.count(vname_text_id))
            throw std::runtime_error("duplicate enumerator: " + vname_s);
        if (vname_text_id != kInvalidText) seen_names.insert(vname_text_id);
        parser.consume();
        // Skip __attribute__((...)) between enum constant name and '='
        // e.g. _CLOCK_REALTIME __attribute__((availability(...))) = 0,
        parser.skip_attributes();
        long long vval = cur_val;
        if (parser.match(TokenKind::Assign)) {
            Node* ve = parse_assign_expr(parser);
            if (!eval_enum_expr(ve, local_enum_consts, &vval)) {
                if (parser.is_cpp_mode() &&
                    is_dependent_enum_expr(ve, parser.binding_state_.enum_consts)) {
                    vval = 0;  // Placeholder until template/dependent evaluation exists.
                } else {
                throw std::runtime_error("enum initializer is not an integer constant expression");
                }
            }
        }
        // Skip __attribute__((...)) annotations after enum value (e.g. availability macros)
        parser.skip_attributes();
        cur_val = vval + 1;
        names.push_back(vname);
        name_text_ids.push_back(vname_text_id);
        vals.push_back(vval);
        // Track enum constants for subsequent initializers in this enum body.
        if (vname_text_id != kInvalidText) local_enum_consts[vname_text_id] = vval;
        if (!parser.match(TokenKind::Comma)) break;
        // Trailing comma before } is allowed
        if (parser.check(TokenKind::RBrace)) break;
    }
    parser.expect(TokenKind::RBrace);

    ed->n_enum_variants = (int)names.size();
    if (ed->n_enum_variants > 0) {
        ed->enum_names =
            parser.arena_.alloc_array<const char*>(ed->n_enum_variants);
        ed->enum_name_text_ids =
            parser.arena_.alloc_array<TextId>(ed->n_enum_variants);
        ed->enum_vals =
            parser.arena_.alloc_array<long long>(ed->n_enum_variants);
        for (int i = 0; i < ed->n_enum_variants; ++i) {
            ed->enum_names[i] = names[i];
            ed->enum_name_text_ids[i] = name_text_ids[i];
            ed->enum_vals[i]  = vals[i];
        }
    }
    if (!is_scoped_enum) {
        for (int i = 0; i < ed->n_enum_variants; ++i)
            if (ed->enum_names[i] && ed->enum_names[i][0]) {
                const TextId enum_name_text_id =
                    ed->enum_name_text_ids ? ed->enum_name_text_ids[i] : kInvalidText;
                if (enum_name_text_id != kInvalidText)
                    parser.binding_state_.enum_consts[enum_name_text_id] =
                        ed->enum_vals[i];
            }
    }
    if (parser.active_context_state_.parsing_top_level_context)
        parser.definition_state_.struct_defs.push_back(ed);
    return ed;
}

// ── parameter parsing ─────────────────────────────────────────────────────────

Node* parse_param(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    auto skip_cpp11_attrs_only = [&]() {
        while (parser.check(TokenKind::LBracket) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
            parser.consume();
            parser.consume();
            int depth = 1;
            while (!parser.at_end() && depth > 0) {
                if (parser.check(TokenKind::LBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::LBracket) {
                    parser.consume();
                    parser.consume();
                    ++depth;
                    continue;
                }
                if (parser.check(TokenKind::RBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::RBracket) {
                    parser.consume();
                    parser.consume();
                    --depth;
                    continue;
                }
                parser.consume();
            }
        }
    };

    skip_cpp11_attrs_only();
    if (parser.check(TokenKind::Ellipsis)) {
        // variadic marker — handled by caller
        parser.consume();
        return nullptr;
    }
    if (parser.check(TokenKind::KwStatic)) {
        throw std::runtime_error(
            std::string("invalid use of storage class 'static' in parameter declaration at line ") +
            std::to_string(parser.cur().line));
    }
    TypeSpec pts = parser.parse_base_type();
    const char* pname = nullptr;
    TextId pname_text_id = kInvalidText;
    Node** fn_ptr_params = nullptr;
    int n_fn_ptr_params = 0;
    bool fn_ptr_variadic = false;
    bool is_parameter_pack = false;
    parser.parse_declarator(pts, &pname, &fn_ptr_params, &n_fn_ptr_params,
                            &fn_ptr_variadic, &is_parameter_pack,
                            nullptr, nullptr, nullptr, &pname_text_id);
    skip_cpp11_attrs_only();
    parse_plain_function_declarator_suffix(
        parser, pts, /*decay_to_function_pointer=*/true);
    parser.skip_attributes();

    // C++ default parameter value: skip '= expr' (balanced, stopping at , or ) at depth 0)
    if (parser.check(TokenKind::Assign)) {
        parser.consume(); // eat '='
        int depth = 0;
        while (!parser.at_end()) {
            if (parser.check(TokenKind::LParen) || parser.check(TokenKind::LBracket) ||
                parser.check(TokenKind::LBrace)) {
                ++depth;
                parser.consume();
            } else if (parser.check(TokenKind::RParen) || parser.check(TokenKind::RBracket) ||
                       parser.check(TokenKind::RBrace)) {
                if (depth == 0) break;
                --depth;
                parser.consume();
            } else if (parser.check(TokenKind::Comma) && depth == 0) {
                break;
            } else {
                parser.consume();
            }
        }
    }

    Node* p = parser.make_node(NK_DECL, ln);
    p->type = pts;
    p->name = pname ? pname : nullptr;
    if (pname && pname[0]) {
        p->unqualified_name = pname;
        p->unqualified_text_id =
            parser.parser_text_id_for_token(pname_text_id, pname);
    }
    p->fn_ptr_params = fn_ptr_params;
    p->n_fn_ptr_params = n_fn_ptr_params;
    p->fn_ptr_variadic = fn_ptr_variadic;
    p->is_parameter_pack = is_parameter_pack;
    // Phase C: propagate fn_ptr params from typedef if not set by declarator.
    if (pts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
        if (const Parser::FnPtrTypedefInfo* info =
                parser.find_current_typedef_fn_ptr_info()) {
            p->fn_ptr_params = info->params;
            p->n_fn_ptr_params = info->n_params;
            p->fn_ptr_variadic = info->variadic;
        }
    }
    return p;
}

// ── expression parsing ────────────────────────────────────────────────────────

// Operator precedence table (higher = tighter binding).


}  // namespace c4c

#include <cctype>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "parser_impl.hpp"
#include "types/types_helpers.hpp"

namespace c4c {
namespace {

struct LexicalBindingScopeGuard {
    Parser* parser = nullptr;

    explicit LexicalBindingScopeGuard(Parser* parser_in) : parser(parser_in) {
        if (parser) parser->push_local_binding_scope();
    }

    ~LexicalBindingScopeGuard() {
        if (parser) parser->pop_local_binding_scope();
    }
};

}  // namespace

Node* parse_block(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    parser.expect(TokenKind::LBrace);
    LexicalBindingScopeGuard lexical_scope_guard(&parser);
    // Save enum constant scope — inner block enums must not leak to outer scope.
    auto saved_enum_consts = parser.binding_state_.enum_consts;
    std::vector<Node*> stmts;
    while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
        int stmt_start = parser.core_input_state_.pos;
        try {
            Node* s = parse_stmt(parser);
            if (s) stmts.push_back(s);
        } catch (const std::exception& e) {
            // Statement-level recovery: emit diagnostic, skip to ; or },
            // produce NK_INVALID_STMT, and continue parsing next statement.
            int err_idx = parser.diagnostic_state_.best_parse_failure.active
                              ? parser.diagnostic_state_.best_parse_failure.token_index
                              : (!parser.at_end() ? parser.core_input_state_.pos
                                           : (parser.core_input_state_.pos > 0
                                                  ? parser.core_input_state_.pos - 1
                                                  : -1));
            int err_line = parser.diagnostic_state_.best_parse_failure.active
                               ? parser.diagnostic_state_.best_parse_failure.line
                               : ((!parser.at_end())
                                      ? parser.cur().line
                                      : (parser.core_input_state_.pos > 0
                                             ? parser.core_input_state_.tokens[parser.core_input_state_.pos - 1].line
                                             : 1));
            int err_col  = parser.diagnostic_state_.best_parse_failure.active
                               ? parser.diagnostic_state_.best_parse_failure.column
                               : ((!parser.at_end()) ? parser.cur().column : 1);
            std::string diag = parser.format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: %s\n",
                    parser.diag_file_at(err_idx), err_line, err_col,
                    diag.empty() ? e.what() : diag.c_str());
            parser.dump_parse_debug_trace();
            parser.diagnostic_state_.had_error = true;
            ++parser.diagnostic_state_.parse_error_count;
            if (parser.diagnostic_state_.parse_error_count >=
                parser.diagnostic_state_.max_parse_errors) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        parser.diag_file_at(err_idx), err_line, err_col);
                break;
            }
            // Advance at least one token to avoid infinite loop.
            if (parser.core_input_state_.pos == stmt_start && !parser.at_end()) parser.consume();
            // Skip to next statement boundary (; or }).
            while (!parser.at_end() && !parser.check(TokenKind::Semi) && !parser.check(TokenKind::RBrace)) {
                parser.consume();
            }
            if (!parser.at_end() && parser.check(TokenKind::Semi)) parser.consume();
            stmts.push_back(parser.make_node(NK_INVALID_STMT, err_line));
        }
    }
    parser.expect(TokenKind::RBrace);
    // Restore enum constants so inner-block definitions don't leak.
    parser.binding_state_.enum_consts = std::move(saved_enum_consts);
    return parser.make_block(stmts.empty() ? nullptr : stmts.data(), (int)stmts.size(), ln);
}

Node* parse_stmt(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    auto is_local_using_recovery_boundary = [&]() -> bool {
        switch (parser.cur().kind) {
            case TokenKind::KwUsing:
            case TokenKind::KwReturn:
            case TokenKind::KwIf:
            case TokenKind::KwSwitch:
            case TokenKind::KwWhile:
            case TokenKind::KwDo:
            case TokenKind::KwFor:
            case TokenKind::KwBreak:
            case TokenKind::KwContinue:
            case TokenKind::KwGoto:
            case TokenKind::KwStaticAssert:
            case TokenKind::LBrace:
            case TokenKind::KwVoid:
            case TokenKind::KwBool:
            case TokenKind::KwChar:
            case TokenKind::KwShort:
            case TokenKind::KwInt:
            case TokenKind::KwLong:
            case TokenKind::KwSigned:
            case TokenKind::KwUnsigned:
            case TokenKind::KwFloat:
            case TokenKind::KwDouble:
            case TokenKind::KwStruct:
            case TokenKind::KwUnion:
            case TokenKind::KwEnum:
            case TokenKind::KwConst:
            case TokenKind::KwVolatile:
            case TokenKind::KwStatic:
            case TokenKind::KwExtern:
            case TokenKind::KwInline:
            case TokenKind::KwAuto:
            case TokenKind::KwRegister:
            case TokenKind::KwRestrict:
            case TokenKind::KwTypedef:
            case TokenKind::KwAttribute:
                return true;
            default:
                return false;
        }
    };
    auto recover_malformed_local_using = [&]() -> Node* {
        while (!parser.at_end()) {
            if (parser.check(TokenKind::Semi)) {
                parser.consume();
                break;
            }
            if (parser.check(TokenKind::RBrace) || is_local_using_recovery_boundary())
                break;
            parser.consume();
        }
        return parser.make_node(NK_INVALID_STMT, ln);
    };

    // Skip leading __attribute__ UNLESS a real type keyword follows, in which
    // case we must let parse_local_decl / parse_base_type capture the attribute
    // (e.g. __attribute__((vector_size(N))) char c0).
    if (parser.check(TokenKind::KwAttribute)) {
        int save = parser.core_input_state_.pos;
        parser.skip_attributes();
        if (parser.is_type_start()) {
            parser.core_input_state_.pos = save;  // restore: let parse_local_decl handle it
        }
    }

    // C++ using type alias inside function body: using R = type;
    // Registered as a local typedef so subsequent code can reference it.
    if (parser.is_cpp_mode() && parser.check(TokenKind::KwUsing)) {
        parser.consume(); // eat 'using'
        if (parser.check(TokenKind::Identifier) &&
            parser.core_input_state_.pos + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind == TokenKind::Assign) {
            std::string alias_name = std::string(parser.token_spelling(parser.cur()));
            parser.consume(); // eat name
            parser.consume(); // eat '='
            const int alias_type_pos = parser.core_input_state_.pos;
            TypeSpec alias_ts{};
            try {
                alias_ts = parser.parse_type_name();
            } catch (const std::exception&) {
                parser.core_input_state_.pos = alias_type_pos;
                return recover_malformed_local_using();
            }
            if (!parser.match(TokenKind::Semi)) {
                parser.core_input_state_.pos = alias_type_pos;
                int depth = 0;
                bool consumed_type_tokens = false;
                while (!parser.at_end()) {
                    if (depth == 0 && parser.check(TokenKind::Semi)) {
                        parser.consume();
                        break;
                    }
                    if (depth == 0 && consumed_type_tokens &&
                        is_local_using_recovery_boundary()) {
                        break;
                    }
                    if (parser.check(TokenKind::Less) || parser.check(TokenKind::LParen) ||
                        parser.check(TokenKind::LBracket)) {
                        ++depth;
                    } else if ((parser.check(TokenKind::Greater) ||
                                parser.check(TokenKind::RParen) ||
                                parser.check(TokenKind::RBracket)) &&
                               depth > 0) {
                        --depth;
                    } else if (parser.check(TokenKind::GreaterGreater) && depth > 0) {
                        depth -= std::min(depth, 2);
                        consumed_type_tokens = true;
                        parser.consume();
                        continue;
                    }
                    consumed_type_tokens = true;
                    parser.consume();
                }
                return parser.make_node(NK_INVALID_STMT, ln);
            }
            // Register as typedef so subsequent code recognizes the name.
            parser.register_typedef_binding(
                parser.parser_text_id_for_token(kInvalidText, alias_name),
                alias_ts, true);
            return parser.make_node(NK_EMPTY, ln);
        }

        if (parser.match(TokenKind::KwNamespace)) {
            if (!parser.check(TokenKind::Identifier))
                return recover_malformed_local_using();
            (void)parser.parse_qualified_name(true);
            if (!parser.match(TokenKind::Semi))
                return recover_malformed_local_using();
            return parser.make_node(NK_EMPTY, ln);
        }

        if (!parser.check(TokenKind::Identifier) && !parser.check(TokenKind::ColonColon))
            return recover_malformed_local_using();

        if (parser.match(TokenKind::ColonColon)) {
            if (!parser.check(TokenKind::Identifier))
                return recover_malformed_local_using();
            parser.consume();
        } else {
            parser.consume();
        }
        while (parser.match(TokenKind::ColonColon)) {
            if (!parser.check(TokenKind::Identifier))
                return recover_malformed_local_using();
            parser.consume();
        }
        if (!parser.match(TokenKind::Semi))
            return recover_malformed_local_using();
        return parser.make_node(NK_EMPTY, ln);
    }

    switch (parser.cur().kind) {
        case TokenKind::PragmaPack: {
            handle_pragma_pack(parser, parser.cur().text_id == kInvalidText
                                           ? std::string()
                                           : std::string(parser.token_spelling(parser.cur())));
            parser.consume();
            return parser.make_node(NK_EMPTY, ln);
        }

        case TokenKind::PragmaWeak: {
            auto* node = parser.make_node(NK_PRAGMA_WEAK, ln);
            node->name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
            parser.consume();
            return node;
        }

        case TokenKind::PragmaExec: {
            auto* node = parser.make_node(NK_PRAGMA_EXEC, ln);
            node->name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
            handle_pragma_exec(parser, std::string(parser.token_spelling(parser.cur())));
            parser.consume();
            return node;
        }

        case TokenKind::KwStaticAssert: {
            return parse_static_assert_declaration(parser);
        }

        case TokenKind::LBrace:
            return parse_block(parser);

        case TokenKind::KwReturn: {
            parser.consume();
            Node* val = nullptr;
            if (!parser.check(TokenKind::Semi)) {
                val = parse_expr(parser);
            }
            parser.match(TokenKind::Semi);
            Node* n = parser.make_node(NK_RETURN, ln);
            n->left = val;
            return n;
        }

        case TokenKind::KwIf: {
            parser.consume();
            bool is_constexpr_if = false;
            if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) {
                is_constexpr_if = true;
            }
            parser.expect(TokenKind::LParen);
            std::unique_ptr<LexicalBindingScopeGuard> condition_scope_guard;
            auto can_start_if_condition_decl = [&]() -> bool {
                if (!parser.is_cpp_mode()) return false;
                TokenKind k = parser.cur().kind;
                if (is_type_kw(k) || is_qualifier(k) || is_storage_class(k)) return true;
                if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval ||
                    k == TokenKind::KwAttribute || k == TokenKind::KwAlignas) {
                    return true;
                }
                if (k == TokenKind::KwTypename) return true;
                if (k != TokenKind::Identifier) return false;
                return is_known_simple_type_head(parser, parser.cur().text_id,
                                                 parser.token_spelling(parser.cur()));
            };

            auto can_use_lite_if_condition_decl = [&]() -> bool {
                if (!can_start_if_condition_decl()) return false;

                TokenKind k = parser.cur().kind;
                switch (k) {
                    case TokenKind::KwStruct:
                    case TokenKind::KwClass:
                    case TokenKind::KwUnion:
                    case TokenKind::KwEnum:
                    case TokenKind::KwTypename:
                    case TokenKind::ColonColon:
                        return false;
                    case TokenKind::Identifier:
                        if (parser.core_input_state_.pos + 1 <
                                static_cast<int>(parser.core_input_state_.tokens.size()) &&
                            (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::ColonColon ||
                             parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::Less)) {
                            return false;
                        }
                        return true;
                    default:
                        return true;
                }
            };

            auto parse_if_condition_decl = [&]() -> Node* {
                if (!can_start_if_condition_decl()) return nullptr;

                auto try_parse_if_condition_decl =
                    [&](auto& guard,
                        std::unique_ptr<LexicalBindingScopeGuard>& pending_scope)
                    -> Node* {
                        TypeSpec base_ts = parser.parse_base_type();
                        parser.parse_attributes(&base_ts);

                        TypeSpec ts = base_ts;
                        ts.array_size_expr = nullptr;
                        const char* vname = nullptr;
                        TextId vname_text_id = kInvalidText;
                        parser.parse_declarator(ts, &vname, nullptr, nullptr,
                                                nullptr, nullptr, nullptr, nullptr,
                                                nullptr, &vname_text_id);
                        parser.skip_attributes();
                        parser.skip_asm();
                        parser.skip_attributes();

                        if (!vname) {
                            return nullptr;
                        }

                        Node* init_node = nullptr;
                        if (parser.match(TokenKind::Assign) ||
                            (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) {
                            init_node = parse_initializer(parser);
                        }

                        if (!parser.check(TokenKind::RParen)) {
                            return nullptr;
                        }

                        Node* decl = parser.make_node(NK_DECL, ln);
                        decl->type = ts;
                        decl->name = vname;
                        decl->unqualified_name = vname;
                        decl->unqualified_text_id =
                            parser.parser_text_id_for_token(vname_text_id, vname);
                        decl->init = init_node;
                        parser.register_var_type_binding(decl->unqualified_text_id, ts);
                        condition_scope_guard = std::move(pending_scope);
                        guard.commit();
                        return decl;
                    };

                try {
                    if (can_use_lite_if_condition_decl()) {
                        Parser::TentativeParseGuardLite guard(parser);
                        auto pending_scope =
                            std::make_unique<LexicalBindingScopeGuard>(&parser);
                        return try_parse_if_condition_decl(guard, pending_scope);
                    }

                    Parser::TentativeParseGuard guard(parser);
                    auto pending_scope =
                        std::make_unique<LexicalBindingScopeGuard>(&parser);
                    return try_parse_if_condition_decl(guard, pending_scope);
                } catch (const std::exception&) {
                    return nullptr;
                }
            };

            Node* cond_decl = parse_if_condition_decl();
            Node* cnd = nullptr;
            if (cond_decl) {
                cnd = parser.make_var(cond_decl->name, cond_decl->line);
                cnd->unqualified_name = cond_decl->unqualified_name;
                cnd->unqualified_text_id = cond_decl->unqualified_text_id;
            } else {
                cnd = parse_expr(parser);
            }
            parser.expect(TokenKind::RParen);
            if (parser.is_cpp_mode()) {
                parser.skip_attributes();
            }
            Node* then_stmt = parse_stmt(parser);
            Node* else_stmt = nullptr;
            if (parser.match(TokenKind::KwElse)) {
                else_stmt = parse_stmt(parser);
            }
            Node* n = parser.make_node(NK_IF, ln);
            n->cond  = cnd;
            n->then_ = then_stmt;
            n->else_ = else_stmt;
            n->is_constexpr = is_constexpr_if;
            if (!cond_decl) return n;

            Node* scoped = parser.make_node(NK_BLOCK, ln);
            scoped->n_children = 2;
            scoped->children = parser.arena_.alloc_array<Node*>(2);
            scoped->children[0] = cond_decl;
            scoped->children[1] = n;
            return scoped;
        }

        case TokenKind::KwWhile: {
            parser.consume();
            parser.expect(TokenKind::LParen);
            std::unique_ptr<LexicalBindingScopeGuard> condition_scope_guard;
            auto can_start_while_condition_decl = [&]() -> bool {
                if (!parser.is_cpp_mode()) return false;
                TokenKind k = parser.cur().kind;
                if (is_type_kw(k) || is_qualifier(k) || is_storage_class(k)) return true;
                if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval ||
                    k == TokenKind::KwAttribute || k == TokenKind::KwAlignas) {
                    return true;
                }
                if (k == TokenKind::KwTypename) return true;
                if (k != TokenKind::Identifier) return false;
                return is_known_simple_type_head(parser, parser.cur().text_id,
                                                 parser.token_spelling(parser.cur()));
            };

            auto can_use_lite_while_condition_decl = [&]() -> bool {
                if (!can_start_while_condition_decl()) return false;

                TokenKind k = parser.cur().kind;
                switch (k) {
                    case TokenKind::KwStruct:
                    case TokenKind::KwClass:
                    case TokenKind::KwUnion:
                    case TokenKind::KwEnum:
                    case TokenKind::KwTypename:
                    case TokenKind::ColonColon:
                        return false;
                    case TokenKind::Identifier:
                        if (parser.core_input_state_.pos + 1 <
                                static_cast<int>(parser.core_input_state_.tokens.size()) &&
                            (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::ColonColon ||
                             parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::Less)) {
                            return false;
                        }
                        return true;
                    default:
                        return true;
                }
            };

            auto parse_while_condition_decl = [&]() -> Node* {
                if (!can_start_while_condition_decl()) return nullptr;

                auto try_parse_while_condition_decl =
                    [&](auto& guard,
                        std::unique_ptr<LexicalBindingScopeGuard>& pending_scope)
                    -> Node* {
                        TypeSpec base_ts = parser.parse_base_type();
                        parser.parse_attributes(&base_ts);

                        TypeSpec ts = base_ts;
                        ts.array_size_expr = nullptr;
                        const char* vname = nullptr;
                        TextId vname_text_id = kInvalidText;
                        parser.parse_declarator(ts, &vname, nullptr, nullptr,
                                                nullptr, nullptr, nullptr, nullptr,
                                                nullptr, &vname_text_id);
                        parser.skip_attributes();
                        parser.skip_asm();
                        parser.skip_attributes();

                        if (!vname) {
                            return nullptr;
                        }

                        Node* init_node = nullptr;
                        if (parser.match(TokenKind::Assign) ||
                            (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) {
                            init_node = parse_initializer(parser);
                        }

                        if (!parser.check(TokenKind::RParen)) {
                            return nullptr;
                        }

                        Node* decl = parser.make_node(NK_DECL, ln);
                        decl->type = ts;
                        decl->name = vname;
                        decl->unqualified_name = vname;
                        decl->unqualified_text_id =
                            parser.parser_text_id_for_token(vname_text_id, vname);
                        decl->init = init_node;
                        parser.register_var_type_binding(decl->unqualified_text_id, ts);
                        condition_scope_guard = std::move(pending_scope);
                        guard.commit();
                        return decl;
                    };

                try {
                    if (can_use_lite_while_condition_decl()) {
                        Parser::TentativeParseGuardLite guard(parser);
                        auto pending_scope =
                            std::make_unique<LexicalBindingScopeGuard>(&parser);
                        return try_parse_while_condition_decl(guard, pending_scope);
                    }

                    Parser::TentativeParseGuard guard(parser);
                    auto pending_scope =
                        std::make_unique<LexicalBindingScopeGuard>(&parser);
                    return try_parse_while_condition_decl(guard, pending_scope);
                } catch (const std::exception&) {
                    return nullptr;
                }
            };

            Node* cond_decl = parse_while_condition_decl();
            Node* cnd = nullptr;
            if (cond_decl) {
                cnd = parser.make_var(cond_decl->name, cond_decl->line);
                cnd->unqualified_name = cond_decl->unqualified_name;
                cnd->unqualified_text_id = cond_decl->unqualified_text_id;
            } else {
                cnd = parse_expr(parser);
            }
            parser.expect(TokenKind::RParen);
            Node* bd = parse_stmt(parser);
            Node* n = parser.make_node(NK_WHILE, ln);
            n->cond = cnd;
            n->body = bd;
            if (!cond_decl) return n;

            Node* scoped = parser.make_node(NK_BLOCK, ln);
            scoped->n_children = 2;
            scoped->children = parser.arena_.alloc_array<Node*>(2);
            scoped->children[0] = cond_decl;
            scoped->children[1] = n;
            return scoped;
        }

        case TokenKind::KwDo: {
            parser.consume();
            Node* bd = parse_stmt(parser);
            parser.expect(TokenKind::KwWhile);
            parser.expect(TokenKind::LParen);
            Node* cnd = parse_expr(parser);
            parser.expect(TokenKind::RParen);
            parser.match(TokenKind::Semi);
            Node* n = parser.make_node(NK_DO_WHILE, ln);
            n->body = bd;
            n->cond = cnd;
            return n;
        }

        case TokenKind::KwFor: {
            parser.consume();
            parser.expect(TokenKind::LParen);
            Node* for_init = nullptr;
            if (!parser.check(TokenKind::Semi)) {
                if (parser.is_type_start()) {
                    auto can_use_lite_range_for_probe = [&]() -> bool {
                        if (!parser.is_cpp_mode()) return false;

                        TokenKind k = parser.cur().kind;
                        switch (k) {
                            case TokenKind::LBracket:
                            case TokenKind::KwAttribute:
                            case TokenKind::KwAlignas:
                            case TokenKind::KwTypedef:
                            case TokenKind::KwStruct:
                            case TokenKind::KwClass:
                            case TokenKind::KwUnion:
                            case TokenKind::KwEnum:
                            case TokenKind::KwTypename:
                            case TokenKind::ColonColon:
                                return false;
                            case TokenKind::Identifier:
                                if (parser.core_input_state_.pos + 1 <
                                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                                    (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                         TokenKind::ColonColon ||
                                     parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                         TokenKind::Less)) {
                                    return false;
                                }
                                return true;
                            default:
                                return true;
                        }
                    };

                    // C++ range-for detection: save position, parse decl,
                    // check if ':' follows (range-for) vs ';' (regular for).
                    if (parser.is_cpp_mode()) {
                        Node* decl = nullptr;
                        bool is_range_for = false;
                        if (can_use_lite_range_for_probe()) {
                            Parser::TentativeParseGuardLite range_guard(parser);
                            Parser::LocalVarBindingSuppressionGuard binding_guard(parser);
                            decl = parse_local_decl(parser);
                            is_range_for = parser.check(TokenKind::Colon);
                            if (is_range_for) {
                                range_guard.commit();
                            }
                        } else {
                            Parser::TentativeParseGuard range_guard(parser);
                            decl = parse_local_decl(parser);
                            is_range_for = parser.check(TokenKind::Colon);
                            if (is_range_for) {
                                range_guard.commit();
                            }
                        }
                        if (is_range_for) {
                            // Range-for: for (Type var : range_expr) body
                            parser.consume(); // consume ':'
                            Node* range_expr = parse_expr(parser);
                            parser.expect(TokenKind::RParen);
                            LexicalBindingScopeGuard loop_scope(&parser);
                            if (decl && decl->name) {
                                const TextId decl_name_text_id =
                                    parser.find_parser_text_id(decl->name);
                                parser.bind_local_value(decl_name_text_id, decl->type);
                            }
                            Node* bd = parse_stmt(parser);
                            Node* n = parser.make_node(NK_RANGE_FOR, ln);
                            n->init  = decl;       // loop variable declaration
                            n->right = range_expr;  // range expression
                            n->body  = bd;
                            return n;
                        }
                        // Not range-for — Parser::TentativeParseGuard restores state
                        // on scope exit since commit() was not called.
                    }
                    {
                        LexicalBindingScopeGuard loop_scope(&parser);
                        for_init = parse_local_decl(parser);
                        // parse_local_decl already consumed the ';' — do NOT
                        // consume another one, or we eat the condition separator.
                        Node* for_cond = nullptr;
                        if (!parser.check(TokenKind::Semi)) {
                            for_cond = parse_expr(parser);
                        }
                        parser.match(TokenKind::Semi);
                        Node* for_update = nullptr;
                        if (!parser.check(TokenKind::RParen)) {
                            for_update = parse_expr(parser);
                        }
                        parser.expect(TokenKind::RParen);
                        Node* bd = parse_stmt(parser);
                        Node* n = parser.make_node(NK_FOR, ln);
                        n->init   = for_init;
                        n->cond   = for_cond;
                        n->update = for_update;
                        n->body   = bd;
                        return n;
                    }
                } else {
                    for_init = parse_expr(parser);
                    parser.match(TokenKind::Semi);
                }
            } else {
                parser.consume();  // consume ;
            }
            Node* for_cond = nullptr;
            if (!parser.check(TokenKind::Semi)) {
                for_cond = parse_expr(parser);
            }
            parser.match(TokenKind::Semi);
            Node* for_update = nullptr;
            if (!parser.check(TokenKind::RParen)) {
                for_update = parse_expr(parser);
            }
            parser.expect(TokenKind::RParen);
            Node* bd = parse_stmt(parser);
            Node* n = parser.make_node(NK_FOR, ln);
            n->init   = for_init;
            n->cond   = for_cond;
            n->update = for_update;
            n->body   = bd;
            return n;
        }

        case TokenKind::KwSwitch: {
            parser.consume();
            parser.expect(TokenKind::LParen);
            std::unique_ptr<LexicalBindingScopeGuard> condition_scope_guard;
            auto can_start_switch_condition_decl = [&]() -> bool {
                if (!parser.is_cpp_mode()) return false;
                TokenKind k = parser.cur().kind;
                if (is_type_kw(k) || is_qualifier(k) || is_storage_class(k)) return true;
                if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval ||
                    k == TokenKind::KwAttribute || k == TokenKind::KwAlignas) {
                    return true;
                }
                if (k == TokenKind::KwTypename) return true;
                if (k != TokenKind::Identifier) return false;
                return is_known_simple_type_head(parser, parser.cur().text_id,
                                                 parser.token_spelling(parser.cur()));
            };

            auto can_use_lite_switch_condition_decl = [&]() -> bool {
                if (!can_start_switch_condition_decl()) return false;

                TokenKind k = parser.cur().kind;
                switch (k) {
                    case TokenKind::KwStruct:
                    case TokenKind::KwClass:
                    case TokenKind::KwUnion:
                    case TokenKind::KwEnum:
                    case TokenKind::KwTypename:
                    case TokenKind::ColonColon:
                        return false;
                    case TokenKind::Identifier:
                        if (parser.core_input_state_.pos + 1 <
                                static_cast<int>(parser.core_input_state_.tokens.size()) &&
                            (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::ColonColon ||
                             parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                                 TokenKind::Less)) {
                            return false;
                        }
                        return true;
                    default:
                        return true;
                }
            };

            auto parse_switch_condition_decl = [&]() -> Node* {
                if (!can_start_switch_condition_decl()) return nullptr;

                auto try_parse_switch_condition_decl =
                    [&](auto& guard,
                        std::unique_ptr<LexicalBindingScopeGuard>& pending_scope)
                    -> Node* {
                        TypeSpec base_ts = parser.parse_base_type();
                        parser.parse_attributes(&base_ts);

                        TypeSpec ts = base_ts;
                        ts.array_size_expr = nullptr;
                        const char* vname = nullptr;
                        TextId vname_text_id = kInvalidText;
                        parser.parse_declarator(ts, &vname, nullptr, nullptr,
                                                nullptr, nullptr, nullptr, nullptr,
                                                nullptr, &vname_text_id);
                        parser.skip_attributes();
                        parser.skip_asm();
                        parser.skip_attributes();

                        if (!vname) {
                            return nullptr;
                        }

                        Node* init_node = nullptr;
                        if (parser.match(TokenKind::Assign) ||
                            (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) {
                            init_node = parse_initializer(parser);
                        }

                        if (!parser.check(TokenKind::RParen)) {
                            return nullptr;
                        }

                        Node* decl = parser.make_node(NK_DECL, ln);
                        decl->type = ts;
                        decl->name = vname;
                        decl->unqualified_name = vname;
                        decl->unqualified_text_id =
                            parser.parser_text_id_for_token(vname_text_id, vname);
                        decl->init = init_node;
                        parser.register_var_type_binding(decl->unqualified_text_id, ts);
                        condition_scope_guard = std::move(pending_scope);
                        guard.commit();
                        return decl;
                    };

                try {
                    if (can_use_lite_switch_condition_decl()) {
                        Parser::TentativeParseGuardLite guard(parser);
                        auto pending_scope =
                            std::make_unique<LexicalBindingScopeGuard>(&parser);
                        return try_parse_switch_condition_decl(guard, pending_scope);
                    }

                    Parser::TentativeParseGuard guard(parser);
                    auto pending_scope =
                        std::make_unique<LexicalBindingScopeGuard>(&parser);
                    return try_parse_switch_condition_decl(guard, pending_scope);
                } catch (const std::exception&) {
                    return nullptr;
                }
            };

            Node* cond_decl = parse_switch_condition_decl();
            Node* cnd = nullptr;
            if (cond_decl) {
                cnd = parser.make_var(cond_decl->name, cond_decl->line);
                cnd->unqualified_name = cond_decl->unqualified_name;
                cnd->unqualified_text_id = cond_decl->unqualified_text_id;
            } else {
                cnd = parse_expr(parser);
            }
            parser.expect(TokenKind::RParen);
            Node* bd = parse_stmt(parser);
            Node* n = parser.make_node(NK_SWITCH, ln);
            n->cond = cnd;
            n->body = bd;
            if (!cond_decl) return n;

            Node* scoped = parser.make_node(NK_BLOCK, ln);
            scoped->n_children = 2;
            scoped->children = parser.arena_.alloc_array<Node*>(2);
            scoped->children[0] = cond_decl;
            scoped->children[1] = n;
            return scoped;
        }

        case TokenKind::KwCase: {
            parser.consume();
            Node* val_lo = parse_assign_expr(parser);
            Node* val_hi = nullptr;
            if (parser.check(TokenKind::Ellipsis)) {
                parser.consume();
                val_hi = parse_assign_expr(parser);
            }
            parser.expect(TokenKind::Colon);
            // Accumulate case body as the next statement
            Node* body_stmt = nullptr;
            if (!parser.check(TokenKind::RBrace) && !parser.check(TokenKind::KwCase) &&
                !parser.check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt(parser);
            }
            if (val_hi) {
                Node* n = parser.make_node(NK_CASE_RANGE, ln);
                n->left  = val_lo;
                n->right = val_hi;
                n->body  = body_stmt;
                return n;
            }
            Node* n = parser.make_node(NK_CASE, ln);
            n->left = val_lo;
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwDefault: {
            parser.consume();
            parser.expect(TokenKind::Colon);
            Node* body_stmt = nullptr;
            if (!parser.check(TokenKind::RBrace) && !parser.check(TokenKind::KwCase) &&
                !parser.check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt(parser);
            }
            Node* n = parser.make_node(NK_DEFAULT, ln);
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwBreak:
            parser.consume(); parser.match(TokenKind::Semi);
            return parser.make_node(NK_BREAK, ln);

        case TokenKind::KwContinue:
            parser.consume(); parser.match(TokenKind::Semi);
            return parser.make_node(NK_CONTINUE, ln);

        case TokenKind::KwGoto: {
            parser.consume();
            const char* lbl = nullptr;
            if (parser.check(TokenKind::Identifier)) {
                lbl = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                parser.consume();
            } else if (parser.check(TokenKind::Star)) {
                // computed goto: goto *expr;
                parser.consume();
                Node* target = parse_unary(parser);
                parser.match(TokenKind::Semi);
                Node* n = parser.make_node(NK_GOTO, ln);
                n->name = "__computed__";
                n->left = target;
                return n;
            }
            parser.match(TokenKind::Semi);
            Node* n = parser.make_node(NK_GOTO, ln);
            n->name = lbl;
            return n;
        }

        case TokenKind::KwAsm: {
            struct AsmOperand {
                std::string constraint;
                Node* expr = nullptr;
            };
            std::vector<Node*> clobbers;
            auto parse_asm_operand = [&]() -> AsmOperand {
                AsmOperand op{};
                if (parser.check(TokenKind::StrLit)) {
                    op.constraint = consume_adjacent_string_literal(parser);
                }
                if (parser.match(TokenKind::LParen)) {
                    op.expr = parse_expr(parser);
                    parser.expect(TokenKind::RParen);
                }
                return op;
            };
            auto parse_asm_operand_list = [&]() -> std::vector<AsmOperand> {
                std::vector<AsmOperand> ops;
                if (parser.check(TokenKind::Colon) || parser.check(TokenKind::RParen)) return ops;
                while (!parser.at_end() && !parser.check(TokenKind::Colon) && !parser.check(TokenKind::RParen)) {
                    ops.push_back(parse_asm_operand());
                    if (!parser.match(TokenKind::Comma)) break;
                }
                return ops;
            };
            auto strip_quotes = [](const std::string& s) -> std::string {
                if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
                    return s.substr(1, s.size() - 2);
                return s;
            };
            auto is_digit_constraint = [](const std::string& s) -> bool {
                if (s.empty()) return false;
                for (char ch : s) {
                    if (!std::isdigit(static_cast<unsigned char>(ch))) return false;
                }
                return true;
            };

            parser.consume();
            // Skip optional qualifiers: volatile, goto, inline
            bool saw_volatile = false;
            while (parser.check(TokenKind::KwVolatile) || parser.check(TokenKind::KwInline) ||
                   parser.check(TokenKind::KwGoto)) {
                if (parser.check(TokenKind::KwVolatile)) saw_volatile = true;
                parser.consume();
            }
            if (!parser.check(TokenKind::LParen)) {
                parser.match(TokenKind::Semi);
                return parser.make_node(NK_EMPTY, ln);
            }
            parser.consume();  // (

            Node* asm_template = nullptr;
            if (parser.check(TokenKind::StrLit)) {
                asm_template = parser.make_str_lit(consume_adjacent_string_literal(parser), parser.cur().line);
            } else if (!parser.check(TokenKind::Colon) && !parser.check(TokenKind::RParen)) {
                asm_template = parse_expr(parser);
            }

            std::vector<AsmOperand> outputs;
            std::vector<AsmOperand> inputs;
            if (parser.match(TokenKind::Colon)) {
                outputs = parse_asm_operand_list();
                if (parser.match(TokenKind::Colon)) {
                    inputs = parse_asm_operand_list();
                    if (parser.match(TokenKind::Colon)) {
                        while (!parser.at_end() && !parser.check(TokenKind::RParen)) {
                            if (parser.check(TokenKind::StrLit)) {
                                clobbers.push_back(parser.make_str_lit(
                                    consume_adjacent_string_literal(parser), parser.cur().line));
                            }
                            else if (parser.check(TokenKind::LParen)) parser.skip_paren_group();
                            else parser.consume();
                            if (!parser.match(TokenKind::Comma)) break;
                        }
                        if (parser.match(TokenKind::Colon)) {
                            while (!parser.at_end() && !parser.check(TokenKind::RParen)) parser.consume();
                        }
                    }
                }
            }
            parser.expect(TokenKind::RParen);
            parser.match(TokenKind::Semi);

            const bool empty_template =
                asm_template && asm_template->kind == NK_STR_LIT && asm_template->sval &&
                strip_quotes(asm_template->sval).empty();
            if (empty_template && !outputs.empty() && !saw_volatile &&
                clobbers.empty()) {
                std::vector<Node*> lowered;
                bool can_lower_all_outputs = true;
                for (int oi = 0; oi < static_cast<int>(outputs.size()); ++oi) {
                    const AsmOperand& out = outputs[oi];
                    if (!out.expr) continue;
                    const std::string out_constraint = strip_quotes(out.constraint);
                    if (out_constraint.find('+') != std::string::npos) {
                        // read-write tied to itself; empty asm leaves value unchanged,
                        // but we must still evaluate the expression for side effects.
                        Node* expr_stmt = parser.make_node(NK_EXPR_STMT, ln);
                        expr_stmt->left = out.expr;
                        lowered.push_back(expr_stmt);
                        continue;
                    }
                    Node* rhs = nullptr;
                    for (const AsmOperand& in : inputs) {
                        const std::string in_constraint = strip_quotes(in.constraint);
                        if (!is_digit_constraint(in_constraint)) continue;
                        if (std::stoi(in_constraint) == oi) {
                            rhs = in.expr;
                            break;
                        }
                    }
                    if (!rhs) {
                        can_lower_all_outputs = false;
                        break;
                    }
                    Node* expr_stmt = parser.make_node(NK_EXPR_STMT, ln);
                    expr_stmt->left = parser.make_assign("=", out.expr, rhs, ln);
                    lowered.push_back(expr_stmt);
                }
                if (!can_lower_all_outputs) lowered.clear();
                if (lowered.empty()) return parser.make_node(NK_EMPTY, ln);
                return parser.make_block(lowered.data(), static_cast<int>(lowered.size()), ln);
            }

            if (asm_template && asm_template->kind == NK_STR_LIT) {
                Node* n = parser.make_node(NK_ASM, ln);
                n->left = asm_template;
                n->is_volatile_asm = saw_volatile;
                n->asm_num_outputs = static_cast<int>(outputs.size());
                n->asm_num_inputs = static_cast<int>(inputs.size());
                n->asm_num_clobbers = static_cast<int>(clobbers.size());
                n->asm_n_constraints = n->asm_num_outputs + n->asm_num_inputs;
                if (n->asm_n_constraints > 0) {
                    n->asm_constraints = parser.arena_.alloc_array<const char*>(n->asm_n_constraints);
                    int ci = 0;
                    for (const AsmOperand& out : outputs) {
                        n->asm_constraints[ci++] = parser.arena_.strdup(out.constraint.c_str());
                    }
                    for (const AsmOperand& in : inputs) {
                        n->asm_constraints[ci++] = parser.arena_.strdup(in.constraint.c_str());
                    }
                }
                n->n_children = n->asm_num_outputs + n->asm_num_inputs + n->asm_num_clobbers;
                if (n->n_children > 0) {
                    n->children = parser.arena_.alloc_array<Node*>(n->n_children);
                    int idx = 0;
                    for (const AsmOperand& out : outputs) n->children[idx++] = out.expr;
                    for (const AsmOperand& in : inputs) n->children[idx++] = in.expr;
                    for (Node* clobber : clobbers) n->children[idx++] = clobber;
                }
                return n;
            }
            return parser.make_node(NK_EMPTY, ln);
        }

        case TokenKind::Semi:
            parser.consume();
            return parser.make_node(NK_EMPTY, ln);

        default:
            break;
    }

    // GCC __label__ local label declaration — treat as no-op
    // e.g. __label__ foo, bar;
    if (parser.check(TokenKind::Identifier) && parser.token_spelling(parser.cur()) == std::string_view("__label__")) {
        parser.consume();  // consume __label__
        while (parser.check(TokenKind::Identifier)) {
            parser.consume();  // consume label name
            if (!parser.match(TokenKind::Comma)) break;
        }
        parser.match(TokenKind::Semi);
        return parser.make_node(NK_EMPTY, ln);
    }

    // Identifier followed by colon → label
    if (parser.check(TokenKind::Identifier) && parser.peek_next_is(TokenKind::Colon)) {
        const char* lbl = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        parser.consume(); parser.consume();  // consume name and :
        Node* body_stmt = nullptr;
        if (!parser.check(TokenKind::RBrace)) {
            body_stmt = parse_stmt(parser);
        }
        Node* n = parser.make_node(NK_LABEL, ln);
        n->name = lbl;
        n->body = body_stmt;
        return n;
    }

    // constexpr / consteval local declarations (C++ mode)
    if (parser.is_cpp_mode() && (parser.check(TokenKind::KwConstexpr) || parser.check(TokenKind::KwConsteval))) {
        return parse_local_decl(parser);
    }

    if (parser.check(TokenKind::KwAlignas)) {
        return parse_local_decl(parser);
    }

    if (parser.is_cpp_mode() &&
        starts_qualified_member_pointer_type_id(parser, parser.core_input_state_.pos)) {
        return parse_local_decl(parser);
    }

    // Local declaration?
    // Disambiguate: Type::Method(args) is a qualified call expression, not a
    // declaration.  When the first identifier is a type but Type::Ident is NOT
    // a known type, route to expression parsing for qualified calls.
    if (parser.is_type_start()) {
        auto follows_assignment_operator = [&](int pos) -> bool {
            if (pos < 0 ||
                pos >= static_cast<int>(parser.core_input_state_.tokens.size())) return false;
            switch (parser.core_input_state_.tokens[pos].kind) {
                case TokenKind::Assign:
                case TokenKind::PlusAssign:
                case TokenKind::MinusAssign:
                case TokenKind::StarAssign:
                case TokenKind::SlashAssign:
                case TokenKind::PercentAssign:
                case TokenKind::AmpAssign:
                case TokenKind::PipeAssign:
                case TokenKind::CaretAssign:
                case TokenKind::LessLessAssign:
                case TokenKind::GreaterGreaterAssign:
                    return true;
                default:
                    return false;
            }
        };
        auto classify_visible_stmt_starter = [&](int pos, int* after_pos) -> int {
            if (after_pos) *after_pos = pos;
            if (pos < 0 || pos >= static_cast<int>(parser.core_input_state_.tokens.size())) {
                return 0;
            }

            const TokenKind first_kind = parser.core_input_state_.tokens[pos].kind;
            if (first_kind != TokenKind::Identifier &&
                first_kind != TokenKind::ColonColon) {
                return 0;
            }

            return parser.classify_visible_value_or_type_head(pos, after_pos);
        };
        int starter_tail_pos = parser.core_input_state_.pos;
        const int starter_kind =
            classify_visible_stmt_starter(parser.core_input_state_.pos, &starter_tail_pos);
        if (parser.is_cpp_mode() && starter_tail_pos <
                                 static_cast<int>(parser.core_input_state_.tokens.size()) &&
            (parser.core_input_state_.tokens[starter_tail_pos].kind == TokenKind::Dot ||
             parser.core_input_state_.tokens[starter_tail_pos].kind == TokenKind::Arrow)) {
            goto expr_stmt;
        }
        if (parser.is_cpp_mode() && starter_kind > 0 &&
            follows_assignment_operator(starter_tail_pos)) {
            goto expr_stmt;
        }
        if (parser.is_cpp_mode() && starter_kind > 0 &&
            starter_tail_pos < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            (parser.core_input_state_.tokens[starter_tail_pos].kind == TokenKind::Dot ||
             parser.core_input_state_.tokens[starter_tail_pos].kind == TokenKind::Arrow)) {
                goto expr_stmt;
        }
        if (parser.is_cpp_mode() && starter_kind > 0 &&
            starter_tail_pos < static_cast<int>(parser.core_input_state_.tokens.size())) {
            const TokenKind starter_tail_kind =
                parser.core_input_state_.tokens[starter_tail_pos].kind;
            if (starter_tail_kind == TokenKind::LParen ||
                (starter_tail_kind == TokenKind::Less &&
                 starts_with_value_like_template_expr(
                     parser, parser.core_input_state_.tokens, parser.core_input_state_.pos))) {
                goto expr_stmt;
            }
        }
        if (parser.is_cpp_mode() && parser.check(TokenKind::Identifier) &&
            parser.core_input_state_.pos + 2 <
                static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                TokenKind::ColonColon &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                TokenKind::KwOperator) {
            goto expr_stmt;
        }
        if (parser.is_cpp_mode() && parser.check(TokenKind::ColonColon) &&
            parser.core_input_state_.pos + 3 <
                static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                TokenKind::Identifier &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                TokenKind::ColonColon &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 3].kind ==
                TokenKind::KwOperator) {
            goto expr_stmt;
        }
        if (parser.is_cpp_mode() &&
            (parser.check(TokenKind::ColonColon) ||
             (parser.check(TokenKind::Identifier) &&
              parser.core_input_state_.pos + 1 <
                  static_cast<int>(parser.core_input_state_.tokens.size()) &&
              parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                  TokenKind::ColonColon))) {
            Parser::QualifiedNameRef qn;
            if (parser.peek_qualified_name(&qn, true) &&
                (qn.is_global_qualified || !qn.qualifier_segments.empty())) {
                const int after_pos =
                    parser.core_input_state_.pos + (qn.is_global_qualified ? 1 : 0) +
                    1 + 2 * static_cast<int>(qn.qualifier_segments.size());
                if (after_pos < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[after_pos].kind ==
                        TokenKind::KwOperator) {
                    // `Type::operator...(...)` in block scope is an
                    // expression statement, not a local declaration.
                    goto expr_stmt;
                }
                if (after_pos + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[after_pos].kind ==
                        TokenKind::ColonColon &&
                    parser.core_input_state_.tokens[after_pos + 1].kind ==
                        TokenKind::KwOperator) {
                    goto expr_stmt;
                }
                if (after_pos < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[after_pos].kind == TokenKind::LParen &&
                    starts_parenthesized_member_pointer_declarator(parser, after_pos)) {
                    return parse_local_decl(parser);
                }

                const TokenKind after_kind =
                    after_pos < static_cast<int>(parser.core_input_state_.tokens.size())
                        ? parser.core_input_state_.tokens[after_pos].kind
                        : TokenKind::EndOfFile;
                const bool call_like_head =
                    after_kind == TokenKind::LParen ||
                    (after_kind == TokenKind::Less &&
                     starts_with_value_like_template_expr(
                         parser, parser.core_input_state_.tokens, parser.core_input_state_.pos));
                if (call_like_head) {
                    if (parser.classify_visible_value_or_type_starter(
                            parser.core_input_state_.pos) > 0) {
                        goto expr_stmt;
                    }

                    const QualifiedTypeProbe probe = probe_qualified_type(parser, qn);
                    if (!probe.has_resolved_typedef) {
                        // Likely a qualified call: Type::Method(args)
                        goto expr_stmt;
                    }
                }
            }
        }
        return parse_local_decl(parser);
    }

expr_stmt:

    // Expression statement
    Node* expr = parse_expr(parser);
    parser.match(TokenKind::Semi);
    Node* n = parser.make_node(NK_EXPR_STMT, ln);
    n->left = expr;
    return n;
}

// ── local declaration parsing ─────────────────────────────────────────────────


}  // namespace c4c

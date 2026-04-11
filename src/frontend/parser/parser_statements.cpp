#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

#include "lexer.hpp"
#include "parser.hpp"
#include "types_helpers.hpp"

namespace c4c {
Node* Parser::parse_block() {
    ParseContextGuard trace(this, __func__);
    int ln = cur().line;
    expect(TokenKind::LBrace);
    // Save enum constant scope — inner block enums must not leak to outer scope.
    auto saved_enum_consts = enum_consts_;
    std::vector<Node*> stmts;
    while (!at_end() && !check(TokenKind::RBrace)) {
        int stmt_start = pos_;
        try {
            Node* s = parse_stmt();
            if (s) stmts.push_back(s);
        } catch (const std::exception& e) {
            // Statement-level recovery: emit diagnostic, skip to ; or },
            // produce NK_INVALID_STMT, and continue parsing next statement.
            int err_idx = best_parse_failure_.active
                              ? best_parse_failure_.token_index
                              : (!at_end() ? pos_ : (pos_ > 0 ? pos_ - 1 : -1));
            int err_line = best_parse_failure_.active
                               ? best_parse_failure_.line
                               : ((!at_end()) ? cur().line : (pos_ > 0 ? tokens_[pos_ - 1].line : 1));
            int err_col  = best_parse_failure_.active
                               ? best_parse_failure_.column
                               : ((!at_end()) ? cur().column : 1);
            std::string diag = format_best_parse_failure();
            fprintf(stderr, "%s:%d:%d: error: %s\n",
                    diag_file_at(err_idx), err_line, err_col,
                    diag.empty() ? e.what() : diag.c_str());
            dump_parse_debug_trace();
            had_error_ = true;
            ++parse_error_count_;
            if (parse_error_count_ >= max_parse_errors_) {
                fprintf(stderr, "%s:%d:%d: error: too many errors emitted, stopping now\n",
                        diag_file_at(err_idx), err_line, err_col);
                break;
            }
            // Advance at least one token to avoid infinite loop.
            if (pos_ == stmt_start && !at_end()) consume();
            // Skip to next statement boundary (; or }).
            while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                consume();
            }
            if (!at_end() && check(TokenKind::Semi)) consume();
            stmts.push_back(make_node(NK_INVALID_STMT, err_line));
        }
    }
    expect(TokenKind::RBrace);
    // Restore enum constants so inner-block definitions don't leak.
    enum_consts_ = std::move(saved_enum_consts);
    return make_block(stmts.empty() ? nullptr : stmts.data(), (int)stmts.size(), ln);
}

Node* Parser::parse_stmt() {
    ParseContextGuard trace(this, __func__);
    int ln = cur().line;
    auto is_local_using_recovery_boundary = [&]() -> bool {
        switch (cur().kind) {
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
        while (!at_end()) {
            if (check(TokenKind::Semi)) {
                consume();
                break;
            }
            if (check(TokenKind::RBrace) || is_local_using_recovery_boundary())
                break;
            consume();
        }
        return make_node(NK_INVALID_STMT, ln);
    };

    // Skip leading __attribute__ UNLESS a real type keyword follows, in which
    // case we must let parse_local_decl / parse_base_type capture the attribute
    // (e.g. __attribute__((vector_size(N))) char c0).
    if (check(TokenKind::KwAttribute)) {
        int save = pos_;
        skip_attributes();
        if (is_type_start()) {
            pos_ = save;  // restore: let parse_local_decl handle it
        }
    }

    // C++ using type alias inside function body: using R = type;
    // Registered as a local typedef so subsequent code can reference it.
    if (is_cpp_mode() && check(TokenKind::KwUsing)) {
        consume(); // eat 'using'
        if (check(TokenKind::Identifier) &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::Assign) {
            std::string alias_name = cur().lexeme;
            consume(); // eat name
            consume(); // eat '='
            const int alias_type_pos = pos_;
            TypeSpec alias_ts{};
            try {
                alias_ts = parse_type_name();
            } catch (const std::exception&) {
                pos_ = alias_type_pos;
                return recover_malformed_local_using();
            }
            if (!match(TokenKind::Semi)) {
                pos_ = alias_type_pos;
                int depth = 0;
                bool consumed_type_tokens = false;
                while (!at_end()) {
                    if (depth == 0 && check(TokenKind::Semi)) {
                        consume();
                        break;
                    }
                    if (depth == 0 && consumed_type_tokens &&
                        is_local_using_recovery_boundary()) {
                        break;
                    }
                    if (check(TokenKind::Less) || check(TokenKind::LParen) ||
                        check(TokenKind::LBracket)) {
                        ++depth;
                    } else if ((check(TokenKind::Greater) ||
                                check(TokenKind::RParen) ||
                                check(TokenKind::RBracket)) &&
                               depth > 0) {
                        --depth;
                    } else if (check(TokenKind::GreaterGreater) && depth > 0) {
                        depth -= std::min(depth, 2);
                        consumed_type_tokens = true;
                        consume();
                        continue;
                    }
                    consumed_type_tokens = true;
                    consume();
                }
                return make_node(NK_INVALID_STMT, ln);
            }
            // Register as typedef so subsequent code recognizes the name.
            register_typedef_binding(alias_name, alias_ts, true);
            return make_node(NK_EMPTY, ln);
        }

        if (match(TokenKind::KwNamespace)) {
            if (!check(TokenKind::Identifier))
                return recover_malformed_local_using();
            (void)parse_qualified_name(true);
            if (!match(TokenKind::Semi))
                return recover_malformed_local_using();
            return make_node(NK_EMPTY, ln);
        }

        if (!check(TokenKind::Identifier) && !check(TokenKind::ColonColon))
            return recover_malformed_local_using();

        if (match(TokenKind::ColonColon)) {
            if (!check(TokenKind::Identifier))
                return recover_malformed_local_using();
            consume();
        } else {
            consume();
        }
        while (match(TokenKind::ColonColon)) {
            if (!check(TokenKind::Identifier))
                return recover_malformed_local_using();
            consume();
        }
        if (!match(TokenKind::Semi))
            return recover_malformed_local_using();
        return make_node(NK_EMPTY, ln);
    }

    switch (cur().kind) {
        case TokenKind::PragmaPack: {
            handle_pragma_pack(cur().lexeme);
            consume();
            return make_node(NK_EMPTY, ln);
        }

        case TokenKind::PragmaWeak: {
            auto* node = make_node(NK_PRAGMA_WEAK, ln);
            node->name = arena_.strdup(cur().lexeme);
            consume();
            return node;
        }

        case TokenKind::PragmaExec: {
            auto* node = make_node(NK_PRAGMA_EXEC, ln);
            node->name = arena_.strdup(cur().lexeme);
            handle_pragma_exec(cur().lexeme);
            consume();
            return node;
        }

        case TokenKind::KwStaticAssert: {
            return parse_static_assert_declaration();
        }

        case TokenKind::LBrace:
            return parse_block();

        case TokenKind::KwReturn: {
            consume();
            Node* val = nullptr;
            if (!check(TokenKind::Semi)) {
                val = parse_expr();
            }
            match(TokenKind::Semi);
            Node* n = make_node(NK_RETURN, ln);
            n->left = val;
            return n;
        }

        case TokenKind::KwIf: {
            consume();
            bool is_constexpr_if = false;
            if (is_cpp_mode() && match(TokenKind::KwConstexpr)) {
                is_constexpr_if = true;
            }
            expect(TokenKind::LParen);
            auto can_start_if_condition_decl = [&]() -> bool {
                if (!is_cpp_mode()) return false;
                TokenKind k = cur().kind;
                if (is_type_kw(k) || is_qualifier(k) || is_storage_class(k)) return true;
                if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval ||
                    k == TokenKind::KwAttribute || k == TokenKind::KwAlignas) {
                    return true;
                }
                if (k == TokenKind::KwTypename) return true;
                if (k != TokenKind::Identifier) return false;
                if (is_template_scope_type_param(cur().lexeme)) return true;
                if (is_typedef_name(cur().lexeme)) return true;
                return has_typedef_type(resolve_visible_type_name(cur().lexeme));
            };

            auto can_use_lite_if_condition_decl = [&]() -> bool {
                if (!can_start_if_condition_decl()) return false;

                TokenKind k = cur().kind;
                switch (k) {
                    case TokenKind::KwStruct:
                    case TokenKind::KwClass:
                    case TokenKind::KwUnion:
                    case TokenKind::KwEnum:
                    case TokenKind::KwTypename:
                    case TokenKind::ColonColon:
                        return false;
                    case TokenKind::Identifier:
                        if (pos_ + 1 < static_cast<int>(tokens_.size()) &&
                            (tokens_[pos_ + 1].kind == TokenKind::ColonColon ||
                             tokens_[pos_ + 1].kind == TokenKind::Less)) {
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
                    [&](auto& guard) -> Node* {
                        TypeSpec base_ts = parse_base_type();
                        parse_attributes(&base_ts);

                        TypeSpec ts = base_ts;
                        ts.array_size_expr = nullptr;
                        const char* vname = nullptr;
                        parse_declarator(ts, &vname);
                        skip_attributes();
                        skip_asm();
                        skip_attributes();

                        if (!vname) {
                            return nullptr;
                        }

                        Node* init_node = nullptr;
                        if (match(TokenKind::Assign) ||
                            (is_cpp_mode() && check(TokenKind::LBrace))) {
                            init_node = parse_initializer();
                        }

                        if (!check(TokenKind::RParen)) {
                            return nullptr;
                        }

                        Node* decl = make_node(NK_DECL, ln);
                        decl->type = ts;
                        decl->name = vname;
                        decl->init = init_node;
                        register_var_type_binding(vname, ts);
                        guard.commit();
                        return decl;
                    };

                try {
                    if (can_use_lite_if_condition_decl()) {
                        TentativeParseGuardLite guard(*this);
                        return try_parse_if_condition_decl(guard);
                    }

                    TentativeParseGuard guard(*this);
                    return try_parse_if_condition_decl(guard);
                } catch (const std::exception&) {
                    return nullptr;
                }
            };

            Node* cond_decl = parse_if_condition_decl();
            Node* cnd = nullptr;
            if (cond_decl) {
                cnd = make_var(cond_decl->name, cond_decl->line);
            } else {
                cnd = parse_expr();
            }
            expect(TokenKind::RParen);
            if (is_cpp_mode()) {
                skip_attributes();
            }
            Node* then_stmt = parse_stmt();
            Node* else_stmt = nullptr;
            if (match(TokenKind::KwElse)) {
                else_stmt = parse_stmt();
            }
            Node* n = make_node(NK_IF, ln);
            n->cond  = cnd;
            n->then_ = then_stmt;
            n->else_ = else_stmt;
            n->is_constexpr = is_constexpr_if;
            if (!cond_decl) return n;

            Node* scoped = make_node(NK_BLOCK, ln);
            scoped->n_children = 2;
            scoped->children = arena_.alloc_array<Node*>(2);
            scoped->children[0] = cond_decl;
            scoped->children[1] = n;
            return scoped;
        }

        case TokenKind::KwWhile: {
            consume();
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_WHILE, ln);
            n->cond = cnd;
            n->body = bd;
            return n;
        }

        case TokenKind::KwDo: {
            consume();
            Node* bd = parse_stmt();
            expect(TokenKind::KwWhile);
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            match(TokenKind::Semi);
            Node* n = make_node(NK_DO_WHILE, ln);
            n->body = bd;
            n->cond = cnd;
            return n;
        }

        case TokenKind::KwFor: {
            consume();
            expect(TokenKind::LParen);
            Node* for_init = nullptr;
            if (!check(TokenKind::Semi)) {
                if (is_type_start()) {
                    auto can_use_lite_range_for_probe = [&]() -> bool {
                        if (!is_cpp_mode()) return false;

                        TokenKind k = cur().kind;
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
                                if (pos_ + 1 < static_cast<int>(tokens_.size()) &&
                                    (tokens_[pos_ + 1].kind == TokenKind::ColonColon ||
                                     tokens_[pos_ + 1].kind == TokenKind::Less)) {
                                    return false;
                                }
                                return true;
                            default:
                                return true;
                        }
                    };

                    // C++ range-for detection: save position, parse decl,
                    // check if ':' follows (range-for) vs ';' (regular for).
                    if (is_cpp_mode()) {
                        Node* decl = nullptr;
                        bool is_range_for = false;
                        if (can_use_lite_range_for_probe()) {
                            TentativeParseGuardLite range_guard(*this);
                            LocalVarBindingSuppressionGuard binding_guard(*this);
                            decl = parse_local_decl();
                            is_range_for = check(TokenKind::Colon);
                            if (is_range_for) {
                                range_guard.commit();
                            }
                        } else {
                            TentativeParseGuard range_guard(*this);
                            decl = parse_local_decl();
                            is_range_for = check(TokenKind::Colon);
                            if (is_range_for) {
                                range_guard.commit();
                            }
                        }
                        if (is_range_for) {
                            // Range-for: for (Type var : range_expr) body
                            consume(); // consume ':'
                            Node* range_expr = parse_expr();
                            expect(TokenKind::RParen);
                            Node* bd = parse_stmt();
                            Node* n = make_node(NK_RANGE_FOR, ln);
                            n->init  = decl;       // loop variable declaration
                            n->right = range_expr;  // range expression
                            n->body  = bd;
                            return n;
                        }
                        // Not range-for — TentativeParseGuard restores state
                        // on scope exit since commit() was not called.
                    }
                    for_init = parse_local_decl();
                    // parse_local_decl already consumed the ';' — do NOT
                    // consume another one, or we eat the condition separator.
                } else {
                    for_init = parse_expr();
                    match(TokenKind::Semi);
                }
            } else {
                consume();  // consume ;
            }
            Node* for_cond = nullptr;
            if (!check(TokenKind::Semi)) {
                for_cond = parse_expr();
            }
            match(TokenKind::Semi);
            Node* for_update = nullptr;
            if (!check(TokenKind::RParen)) {
                for_update = parse_expr();
            }
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_FOR, ln);
            n->init   = for_init;
            n->cond   = for_cond;
            n->update = for_update;
            n->body   = bd;
            return n;
        }

        case TokenKind::KwSwitch: {
            consume();
            expect(TokenKind::LParen);
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
            Node* bd = parse_stmt();
            Node* n = make_node(NK_SWITCH, ln);
            n->cond = cnd;
            n->body = bd;
            return n;
        }

        case TokenKind::KwCase: {
            consume();
            Node* val_lo = parse_assign_expr();
            Node* val_hi = nullptr;
            if (check(TokenKind::Ellipsis)) {
                consume();
                val_hi = parse_assign_expr();
            }
            expect(TokenKind::Colon);
            // Accumulate case body as the next statement
            Node* body_stmt = nullptr;
            if (!check(TokenKind::RBrace) && !check(TokenKind::KwCase) &&
                !check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt();
            }
            if (val_hi) {
                Node* n = make_node(NK_CASE_RANGE, ln);
                n->left  = val_lo;
                n->right = val_hi;
                n->body  = body_stmt;
                return n;
            }
            Node* n = make_node(NK_CASE, ln);
            n->left = val_lo;
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwDefault: {
            consume();
            expect(TokenKind::Colon);
            Node* body_stmt = nullptr;
            if (!check(TokenKind::RBrace) && !check(TokenKind::KwCase) &&
                !check(TokenKind::KwDefault)) {
                body_stmt = parse_stmt();
            }
            Node* n = make_node(NK_DEFAULT, ln);
            n->body = body_stmt;
            return n;
        }

        case TokenKind::KwBreak:
            consume(); match(TokenKind::Semi);
            return make_node(NK_BREAK, ln);

        case TokenKind::KwContinue:
            consume(); match(TokenKind::Semi);
            return make_node(NK_CONTINUE, ln);

        case TokenKind::KwGoto: {
            consume();
            const char* lbl = nullptr;
            if (check(TokenKind::Identifier)) {
                lbl = arena_.strdup(cur().lexeme);
                consume();
            } else if (check(TokenKind::Star)) {
                // computed goto: goto *expr;
                consume();
                Node* target = parse_unary();
                match(TokenKind::Semi);
                Node* n = make_node(NK_GOTO, ln);
                n->name = "__computed__";
                n->left = target;
                return n;
            }
            match(TokenKind::Semi);
            Node* n = make_node(NK_GOTO, ln);
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
                if (check(TokenKind::StrLit)) {
                    op.constraint = cur().lexeme;
                    consume();
                }
                if (match(TokenKind::LParen)) {
                    op.expr = parse_expr();
                    expect(TokenKind::RParen);
                }
                return op;
            };
            auto parse_asm_operand_list = [&]() -> std::vector<AsmOperand> {
                std::vector<AsmOperand> ops;
                if (check(TokenKind::Colon) || check(TokenKind::RParen)) return ops;
                while (!at_end() && !check(TokenKind::Colon) && !check(TokenKind::RParen)) {
                    ops.push_back(parse_asm_operand());
                    if (!match(TokenKind::Comma)) break;
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

            consume();
            // Skip optional qualifiers: volatile, goto, inline
            bool saw_volatile = false;
            while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
                   check(TokenKind::KwGoto)) {
                if (check(TokenKind::KwVolatile)) saw_volatile = true;
                consume();
            }
            if (!check(TokenKind::LParen)) {
                match(TokenKind::Semi);
                return make_node(NK_EMPTY, ln);
            }
            consume();  // (

            Node* asm_template = nullptr;
            if (check(TokenKind::StrLit)) {
                asm_template = make_str_lit(arena_.strdup(cur().lexeme), cur().line);
                consume();
            } else if (!check(TokenKind::Colon) && !check(TokenKind::RParen)) {
                asm_template = parse_expr();
            }

            std::vector<AsmOperand> outputs;
            std::vector<AsmOperand> inputs;
            if (match(TokenKind::Colon)) {
                outputs = parse_asm_operand_list();
                if (match(TokenKind::Colon)) {
                    inputs = parse_asm_operand_list();
                    if (match(TokenKind::Colon)) {
                        while (!at_end() && !check(TokenKind::RParen)) {
                            if (check(TokenKind::StrLit)) {
                                clobbers.push_back(
                                    make_str_lit(arena_.strdup(cur().lexeme), cur().line));
                                consume();
                            }
                            else if (check(TokenKind::LParen)) skip_paren_group();
                            else consume();
                            if (!match(TokenKind::Comma)) break;
                        }
                        if (match(TokenKind::Colon)) {
                            while (!at_end() && !check(TokenKind::RParen)) consume();
                        }
                    }
                }
            }
            expect(TokenKind::RParen);
            match(TokenKind::Semi);

            const bool empty_template =
                asm_template && asm_template->kind == NK_STR_LIT && asm_template->sval &&
                strip_quotes(asm_template->sval).empty();
            if (empty_template && !outputs.empty()) {
                std::vector<Node*> lowered;
                for (int oi = 0; oi < static_cast<int>(outputs.size()); ++oi) {
                    const AsmOperand& out = outputs[oi];
                    if (!out.expr) continue;
                    const std::string out_constraint = strip_quotes(out.constraint);
                    if (out_constraint.find('+') != std::string::npos) {
                        // read-write tied to itself; empty asm leaves value unchanged,
                        // but we must still evaluate the expression for side effects.
                        Node* expr_stmt = make_node(NK_EXPR_STMT, ln);
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
                    if (!rhs) continue;
                    Node* expr_stmt = make_node(NK_EXPR_STMT, ln);
                    expr_stmt->left = make_assign("=", out.expr, rhs, ln);
                    lowered.push_back(expr_stmt);
                }
                if (lowered.empty()) return make_node(NK_EMPTY, ln);
                return make_block(lowered.data(), static_cast<int>(lowered.size()), ln);
            }

            if (asm_template && asm_template->kind == NK_STR_LIT) {
                Node* n = make_node(NK_ASM, ln);
                n->left = asm_template;
                n->is_volatile_asm = saw_volatile;
                n->asm_num_outputs = static_cast<int>(outputs.size());
                n->asm_num_inputs = static_cast<int>(inputs.size());
                n->asm_num_clobbers = static_cast<int>(clobbers.size());
                n->asm_n_constraints = n->asm_num_outputs + n->asm_num_inputs;
                if (n->asm_n_constraints > 0) {
                    n->asm_constraints = arena_.alloc_array<const char*>(n->asm_n_constraints);
                    int ci = 0;
                    for (const AsmOperand& out : outputs) {
                        n->asm_constraints[ci++] = arena_.strdup(out.constraint.c_str());
                    }
                    for (const AsmOperand& in : inputs) {
                        n->asm_constraints[ci++] = arena_.strdup(in.constraint.c_str());
                    }
                }
                n->n_children = n->asm_num_outputs + n->asm_num_inputs + n->asm_num_clobbers;
                if (n->n_children > 0) {
                    n->children = arena_.alloc_array<Node*>(n->n_children);
                    int idx = 0;
                    for (const AsmOperand& out : outputs) n->children[idx++] = out.expr;
                    for (const AsmOperand& in : inputs) n->children[idx++] = in.expr;
                    for (Node* clobber : clobbers) n->children[idx++] = clobber;
                }
                return n;
            }
            return make_node(NK_EMPTY, ln);
        }

        case TokenKind::Semi:
            consume();
            return make_node(NK_EMPTY, ln);

        default:
            break;
    }

    // GCC __label__ local label declaration — treat as no-op
    // e.g. __label__ foo, bar;
    if (check(TokenKind::Identifier) && cur().lexeme == std::string_view("__label__")) {
        consume();  // consume __label__
        while (check(TokenKind::Identifier)) {
            consume();  // consume label name
            if (!match(TokenKind::Comma)) break;
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Identifier followed by colon → label
    if (check(TokenKind::Identifier) && peek_next_is(TokenKind::Colon)) {
        const char* lbl = arena_.strdup(cur().lexeme);
        consume(); consume();  // consume name and :
        Node* body_stmt = nullptr;
        if (!check(TokenKind::RBrace)) {
            body_stmt = parse_stmt();
        }
        Node* n = make_node(NK_LABEL, ln);
        n->name = lbl;
        n->body = body_stmt;
        return n;
    }

    // constexpr / consteval local declarations (C++ mode)
    if (is_cpp_mode() && (check(TokenKind::KwConstexpr) || check(TokenKind::KwConsteval))) {
        return parse_local_decl();
    }

    if (check(TokenKind::KwAlignas)) {
        return parse_local_decl();
    }

    if (is_cpp_mode() &&
        starts_qualified_member_pointer_type_id(*this, pos_)) {
        return parse_local_decl();
    }

    // Local declaration?
    // Disambiguate: Type::Method(args) is a qualified call expression, not a
    // declaration.  When the first identifier is a type but Type::Ident is NOT
    // a known type, route to expression parsing for qualified calls.
    if (is_type_start()) {
        auto follows_assignment_operator = [&](int pos) -> bool {
            if (pos < 0 || pos >= static_cast<int>(tokens_.size())) return false;
            switch (tokens_[pos].kind) {
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
        auto has_visible_value_binding = [&](const std::string& name) -> bool {
            if (name.empty()) return false;
            const std::string resolved = resolve_visible_value_name(name);
            return has_var_type(name) || has_known_fn_name(name) ||
                   has_var_type(resolved) || has_known_fn_name(resolved);
        };
        if (is_cpp_mode() && check(TokenKind::Identifier) &&
            has_visible_value_binding(cur().lexeme) &&
            follows_assignment_operator(pos_ + 1)) {
            goto expr_stmt;
        }
        if (is_cpp_mode() && check(TokenKind::Identifier) &&
            (peek(1).kind == TokenKind::Dot || peek(1).kind == TokenKind::Arrow)) {
            const std::string visible_value = resolve_visible_value_name(cur().lexeme);
            if (!visible_value.empty()) {
                goto expr_stmt;
            }
        }
        if (is_cpp_mode() && check(TokenKind::Identifier) &&
            pos_ + 2 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
            tokens_[pos_ + 2].kind == TokenKind::KwOperator) {
            // `Type::operator...(...)` in block scope is an expression
            // statement, not a local declaration. `peek_qualified_name()`
            // only walks identifier segments, so route operator-owned
            // statements before the generic qualified-type probe below.
            goto expr_stmt;
        }
        if (is_cpp_mode() && check(TokenKind::Identifier) &&
            pos_ + 2 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
            tokens_[pos_ + 2].kind == TokenKind::Identifier) {
            // Peek the full qualified name to check if it resolves to a type.
            QualifiedNameRef qn;
            if (peek_qualified_name(&qn, false) && !qn.qualifier_segments.empty()) {
                const int after_pos =
                    pos_ + 1 + 2 * static_cast<int>(qn.qualifier_segments.size());
                if (after_pos < static_cast<int>(tokens_.size()) &&
                    tokens_[after_pos].kind == TokenKind::LParen &&
                    starts_parenthesized_member_pointer_declarator(*this, after_pos)) {
                    return parse_local_decl();
                }
                const std::string full_name = qn.spelled();
                // Check if the full qualified name is a known type
                bool is_known_type = is_typedef_name(full_name) ||
                    has_typedef_type(full_name) ||
                    has_typedef_type(resolve_visible_type_name(full_name));
                // Also try namespace resolution
                if (!is_known_type) {
                    int ctx = resolve_namespace_context(qn);
                    if (ctx >= 0) {
                        std::string ns_name = canonical_name_in_context(ctx, qn.base_name);
                        is_known_type = has_typedef_type(ns_name);
                    }
                }
                if (!is_known_type) {
                    // Likely a qualified call: Type::Method(args)
                    goto expr_stmt;
                }
            }
        }
        return parse_local_decl();
    }

expr_stmt:

    // Expression statement
    Node* expr = parse_expr();
    match(TokenKind::Semi);
    Node* n = make_node(NK_EXPR_STMT, ln);
    n->left = expr;
    return n;
}

// ── local declaration parsing ─────────────────────────────────────────────────


}  // namespace c4c

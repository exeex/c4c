#include "parser.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace tinyc2ll::frontend_cxx {
Node* Parser::parse_block() {
    int ln = cur().line;
    expect(TokenKind::LBrace);
    // Save enum constant scope — inner block enums must not leak to outer scope.
    auto saved_enum_consts = enum_consts_;
    std::vector<Node*> stmts;
    while (!at_end() && !check(TokenKind::RBrace)) {
        Node* s = parse_stmt();
        if (s) stmts.push_back(s);
    }
    expect(TokenKind::RBrace);
    // Restore enum constants so inner-block definitions don't leak.
    enum_consts_ = std::move(saved_enum_consts);
    return make_block(stmts.empty() ? nullptr : stmts.data(), (int)stmts.size(), ln);
}

Node* Parser::parse_stmt() {
    int ln = cur().line;

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
            Node* cnd = parse_expr();
            expect(TokenKind::RParen);
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
            return n;
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
                    for_init = parse_local_decl();
                    // for-decl init already consumed the semicolon via parse_local_decl
                    // Actually parse_local_decl doesn't consume ';' for for-init context
                    // We'll handle with a flag — just consume ; here
                    match(TokenKind::Semi);
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
    if (check(TokenKind::Identifier) && check2(TokenKind::Colon)) {
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

    // Local declaration?
    if (is_type_start()) {
        return parse_local_decl();
    }

    // Expression statement
    Node* expr = parse_expr();
    match(TokenKind::Semi);
    Node* n = make_node(NK_EXPR_STMT, ln);
    n->left = expr;
    return n;
}

// ── local declaration parsing ─────────────────────────────────────────────────


}  // namespace tinyc2ll::frontend_cxx

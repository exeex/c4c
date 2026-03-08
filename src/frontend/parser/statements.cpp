#include "parser.hpp"

namespace tinyc2ll::frontend_cxx {
Node* Parser::parse_block() {
    int ln = cur().line;
    expect(TokenKind::LBrace);
    std::vector<Node*> stmts;
    while (!at_end() && !check(TokenKind::RBrace)) {
        Node* s = parse_stmt();
        if (s) stmts.push_back(s);
    }
    expect(TokenKind::RBrace);
    return make_block(stmts.empty() ? nullptr : stmts.data(), (int)stmts.size(), ln);
}

Node* Parser::parse_stmt() {
    int ln = cur().line;
    skip_attributes();

    switch (cur().kind) {
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
            // asm volatile ("..." : outputs : inputs : clobbers) — skip entirely
            consume();
            // Skip optional qualifiers: volatile, goto, inline
            while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
                   check(TokenKind::KwGoto))
                consume();
            if (check(TokenKind::LParen)) skip_paren_group();
            match(TokenKind::Semi);
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

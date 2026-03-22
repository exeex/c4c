#include "parser.hpp"
#include "parser_internal.hpp"

#include <cassert>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace c4c {
int Parser::bin_prec(TokenKind k) {
    switch (k) {
        case TokenKind::PipePipe:       return 4;
        case TokenKind::AmpAmp:         return 5;
        case TokenKind::Pipe:           return 6;
        case TokenKind::Caret:          return 7;
        case TokenKind::Amp:            return 8;
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:      return 9;
        case TokenKind::Less:
        case TokenKind::LessEqual:
        case TokenKind::Greater:
        case TokenKind::GreaterEqual:   return 10;
        case TokenKind::LessLess:
        case TokenKind::GreaterGreater: return 11;
        case TokenKind::Plus:
        case TokenKind::Minus:          return 12;
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:        return 13;
        default:                        return -1;
    }
}

Node* Parser::parse_expr() {
    Node* lhs = parse_assign_expr();
    while (check(TokenKind::Comma)) {
        int ln = cur().line;
        consume();
        Node* rhs = parse_assign_expr();
        Node* n = make_node(NK_COMMA_EXPR, ln);
        n->left  = lhs;
        n->right = rhs;
        lhs = n;
    }
    return lhs;
}

Node* Parser::parse_assign_expr() {
    Node* lhs = parse_ternary();
    int ln = cur().line;
    TokenKind k = cur().kind;
    const char* op = nullptr;
    switch (k) {
        case TokenKind::Assign:               op = "=";   break;
        case TokenKind::PlusAssign:           op = "+=";  break;
        case TokenKind::MinusAssign:          op = "-=";  break;
        case TokenKind::StarAssign:           op = "*=";  break;
        case TokenKind::SlashAssign:          op = "/=";  break;
        case TokenKind::PercentAssign:        op = "%=";  break;
        case TokenKind::AmpAssign:            op = "&=";  break;
        case TokenKind::PipeAssign:           op = "|=";  break;
        case TokenKind::CaretAssign:          op = "^=";  break;
        case TokenKind::LessLessAssign:       op = "<<="; break;
        case TokenKind::GreaterGreaterAssign: op = ">>="; break;
        default: break;
    }
    if (op) {
        consume();
        Node* rhs = parse_assign_expr();  // right-associative
        if (op[0] == '=' && op[1] == '\0') {
            return make_assign("=", lhs, rhs, ln);
        } else {
            return make_assign(op, lhs, rhs, ln);
        }
    }
    return lhs;
}

Node* Parser::parse_ternary() {
    Node* cond = parse_binary(4);
    if (!check(TokenKind::Question)) return cond;
    int ln = cur().line;
    consume();  // ?

    Node* then_node;
    if (check(TokenKind::Colon)) {
        // GNU extension: cond ?: else  (omitted-middle)
        then_node = cond;
    } else {
        then_node = parse_expr();
    }
    expect(TokenKind::Colon);
    Node* else_node = parse_assign_expr();

    Node* n = make_node(NK_TERNARY, ln);
    n->cond  = cond;
    n->then_ = then_node;
    n->else_ = else_node;
    return n;
}

Node* Parser::parse_binary(int min_prec) {
    Node* lhs = parse_unary();
    while (true) {
        int prec = bin_prec(cur().kind);
        if (prec < min_prec) break;
        TokenKind k = cur().kind;
        int ln = cur().line;
        const char* op;
        switch (k) {
            case TokenKind::Plus:             op = "+";  break;
            case TokenKind::Minus:            op = "-";  break;
            case TokenKind::Star:             op = "*";  break;
            case TokenKind::Slash:            op = "/";  break;
            case TokenKind::Percent:          op = "%";  break;
            case TokenKind::Amp:              op = "&";  break;
            case TokenKind::Pipe:             op = "|";  break;
            case TokenKind::Caret:            op = "^";  break;
            case TokenKind::LessLess:         op = "<<"; break;
            case TokenKind::GreaterGreater:   op = ">>"; break;
            case TokenKind::EqualEqual:       op = "=="; break;
            case TokenKind::BangEqual:        op = "!="; break;
            case TokenKind::Less:             op = "<";  break;
            case TokenKind::LessEqual:        op = "<="; break;
            case TokenKind::Greater:          op = ">";  break;
            case TokenKind::GreaterEqual:     op = ">="; break;
            case TokenKind::AmpAmp:           op = "&&"; break;
            case TokenKind::PipePipe:         op = "||"; break;
            default:                          op = "?";  break;
        }
        consume();
        Node* rhs = parse_binary(prec + 1);  // left-associative
        lhs = make_binop(op, lhs, rhs, ln);
    }
    return lhs;
}

Node* Parser::parse_unary() {
    int ln = cur().line;
    switch (cur().kind) {
        case TokenKind::Bang: {
            consume();
            Node* operand = parse_unary();
            return make_unary("!", operand, ln);
        }
        case TokenKind::Tilde: {
            consume();
            Node* operand = parse_unary();
            return make_unary("~", operand, ln);
        }
        case TokenKind::Minus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("-", operand, ln);
        }
        case TokenKind::Plus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("+", operand, ln);
        }
        case TokenKind::PlusPlus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("++pre", operand, ln);
        }
        case TokenKind::MinusMinus: {
            consume();
            Node* operand = parse_unary();
            return make_unary("--pre", operand, ln);
        }
        case TokenKind::AmpAmp: {
            // GCC label-address: &&label_name  (computed goto extension)
            consume();
            if (check(TokenKind::Identifier)) {
                // Emit as a null void* placeholder; computed goto not fully supported
                const char* lbl_name = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_INT_LIT, ln);
                n->ival = 0;
                n->name = lbl_name;  // store label name for future use
                return n;
            }
            // Fall back: treat as bitwise AND of two address-of
            Node* operand = parse_unary();
            Node* addr = make_node(NK_ADDR, ln);
            addr->left = operand;
            Node* addr2 = make_node(NK_ADDR, ln);
            addr2->left = addr;
            return addr2;
        }
        case TokenKind::Amp: {
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_ADDR, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::Star: {
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_DEREF, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwDelete: {
            // C++ delete / delete[] expression.
            // Parse the operand but emit as a no-op (0) since we don't
            // support heap allocation; the intent is to not choke on the syntax.
            consume();  // consume 'delete'
            if (check(TokenKind::LBracket) &&
                pos_ + 1 < static_cast<int>(tokens_.size()) &&
                tokens_[pos_ + 1].kind == TokenKind::RBracket) {
                consume();  // '['
                consume();  // ']'
            }
            // Parse the operand expression (the pointer being freed).
            (void)parse_unary();
            // Return a no-op integer 0 (void-like).
            Node* n = make_node(NK_INT_LIT, ln);
            n->ival = 0;
            return n;
        }
        case TokenKind::KwSizeof: {
            consume();
            // sizeof...(pack) — C++ parameter pack size; return 0 placeholder
            if (is_cpp_mode() && check(TokenKind::Ellipsis)) {
                consume(); // eat '...'
                expect(TokenKind::LParen);
                if (!check(TokenKind::RParen)) consume(); // eat pack name
                expect(TokenKind::RParen);
                Node* n = make_node(NK_INT_LIT, ln);
                n->type.base = TB_ULONG;
                n->ival = 0;
                return n;
            }
            if (check(TokenKind::LParen)) {
                // Could be sizeof(type) or sizeof(expr)
                // Disambiguate: if first token inside is a type keyword, it's a type
                int save_pos = pos_;
                consume();  // consume (
                if (is_type_start()) {
                    TypeSpec ts = parse_type_name();
                    expect(TokenKind::RParen);
                    Node* n = make_node(NK_SIZEOF_TYPE, ln);
                    n->type = ts;
                    return n;
                } else {
                    // Restore and parse as sizeof(expr)
                    pos_ = save_pos;
                    consume();  // consume (
                    Node* inner = parse_assign_expr();
                    expect(TokenKind::RParen);
                    Node* n = make_node(NK_SIZEOF_EXPR, ln);
                    n->left = inner;
                    return n;
                }
            } else {
                Node* inner = parse_unary();
                Node* n = make_node(NK_SIZEOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwAlignof:
        case TokenKind::KwGnuAlignof: {
            consume();
            expect(TokenKind::LParen);
            if (is_type_start()) {
                TypeSpec ts = parse_type_name();
                expect(TokenKind::RParen);
                Node* n = make_node(NK_ALIGNOF_TYPE, ln);
                n->type = ts;
                return n;
            } else {
                Node* inner = parse_assign_expr();
                expect(TokenKind::RParen);
                Node* n = make_node(NK_ALIGNOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwBuiltinVaArg: {
            // __builtin_va_arg(ap, type)
            consume();
            expect(TokenKind::LParen);
            Node* ap = parse_assign_expr();
            expect(TokenKind::Comma);
            TypeSpec ts = parse_type_name();
            expect(TokenKind::RParen);
            Node* n = make_node(NK_VA_ARG, ln);
            n->left = ap;
            n->type = ts;
            return n;
        }
        case TokenKind::KwExtension: {
            // GNU __extension__: suppress extension warnings, semantically transparent.
            consume();
            return parse_unary();
        }
        case TokenKind::KwGccReal: {
            // GNU extension: __real__ expr
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_REAL_PART, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwGccImag: {
            // GNU extension: __imag__ expr
            consume();
            Node* operand = parse_unary();
            Node* n = make_node(NK_IMAG_PART, ln);
            n->left = operand;
            return n;
        }
        default:
            return parse_postfix(parse_primary());
    }
}

Node* Parser::parse_postfix(Node* base) {
    while (true) {
        int ln = cur().line;
        switch (cur().kind) {
            case TokenKind::PlusPlus: {
                consume();
                Node* n = make_node(NK_POSTFIX, ln);
                n->op   = "++";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::MinusMinus: {
                consume();
                Node* n = make_node(NK_POSTFIX, ln);
                n->op   = "--";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::Dot: {
                consume();
                if (!check(TokenKind::Identifier)) break;
                const char* fname = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = fname;
                n->is_arrow = false;
                base = n;
                break;
            }
            case TokenKind::Arrow: {
                consume();
                if (!check(TokenKind::Identifier)) break;
                const char* fname = arena_.strdup(cur().lexeme);
                consume();
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = fname;
                n->is_arrow = true;
                base = n;
                break;
            }
            case TokenKind::LBracket: {
                consume();
                Node* idx = parse_expr();
                expect(TokenKind::RBracket);
                Node* n = make_node(NK_INDEX, ln);
                n->left  = base;
                n->right = idx;
                base = n;
                break;
            }
            case TokenKind::LParen: {
                // Function call
                consume();
                const BuiltinInfo* builtin = nullptr;
                if (base && base->kind == NK_VAR && base->name) {
                    builtin = builtin_by_name(base->name);
                }
                std::vector<Node*> args;
                while (!at_end() && !check(TokenKind::RParen)) {
                    args.push_back(parse_assign_expr());
                    // C++ pack expansion in call args: func(args...)
                    if (is_cpp_mode() && check(TokenKind::Ellipsis)) consume();
                    if (!match(TokenKind::Comma)) break;
                }
                expect(TokenKind::RParen);
                const NodeKind call_kind =
                    (builtin && (builtin->node_kind == BuiltinNodeKind::BuiltinCall ||
                                 builtin->node_kind == BuiltinNodeKind::CanonicalCall))
                        ? NK_BUILTIN_CALL
                        : NK_CALL;
                Node* n = make_node(call_kind, ln);
                n->left       = base;
                if (call_kind == NK_BUILTIN_CALL && builtin) n->builtin_id = builtin->id;
                n->n_children = (int)args.size();
                if (n->n_children > 0) {
                    n->children = arena_.alloc_array<Node*>(n->n_children);
                    for (int i = 0; i < n->n_children; ++i) n->children[i] = args[i];
                }
                base = n;
                break;
            }
            case TokenKind::Ellipsis: {
                if (!is_cpp_mode()) return base;
                // C++ pack expansion suffix: expr...
                // Keep the underlying expression node and treat the expansion
                // marker as parse-only for now so template-heavy initializers
                // like EASTL piecewise pair constructors stay balanced.
                consume();
                break;
            }
            default:
                return base;
        }
    }
}


Node* Parser::parse_primary() {
    int ln = cur().line;

    // Integer literal
    if (check(TokenKind::IntLit)) {
        long long v = parse_int_lexeme(cur().lexeme.c_str());
        bool imag   = lexeme_is_imaginary(cur().lexeme.c_str());
        const char* lex = arena_.strdup(cur().lexeme);
        consume();
        Node* n = make_int_lit(v, ln);
        n->sval = lex;
        n->is_imaginary = imag;
        return n;
    }

    if (is_cpp_mode() && check(TokenKind::KwTrue)) {
        consume();
        return make_int_lit(1, ln);
    }

    if (is_cpp_mode() && check(TokenKind::KwFalse)) {
        consume();
        return make_int_lit(0, ln);
    }

    // Float literal
    if (check(TokenKind::FloatLit)) {
        bool imag   = lexeme_is_imaginary(cur().lexeme.c_str());
        double v    = parse_float_lexeme(cur().lexeme.c_str());
        const char* raw = arena_.strdup(cur().lexeme);
        consume();
        Node* n = make_float_lit(v, raw, ln);
        n->is_imaginary = imag;
        return n;
    }

    // String literal (possibly multiple adjacent)
    if (check(TokenKind::StrLit)) {
        std::string combined = cur().lexeme;
        consume();
        while (check(TokenKind::StrLit)) {
            // Adjacent string concatenation: trim closing quote of first,
            // trim opening quote of second, join
            if (!combined.empty() && combined.back() == '"') combined.pop_back();
            const std::string& next = cur().lexeme;
            if (!next.empty() && next.front() == '"') {
                combined += next.substr(1);
            } else {
                combined += next;
            }
            consume();
        }
        return make_str_lit(arena_.strdup(combined), ln);
    }

    // Char literal
    if (check(TokenKind::CharLit)) {
        const std::string& lex = cur().lexeme;
        long long cv = 0;
        // lex is like 'a' or '\n' or L'\0' etc.
        // Skip wide/unicode prefix if present (L, u, U)
        int char_start = 0;
        if (!lex.empty() && (lex[0] == 'L' || lex[0] == 'u' || lex[0] == 'U'))
            char_start = 1;
        // Now lex[char_start] should be the opening quote '
        if ((int)lex.size() >= char_start + 3) {
            if (lex[char_start + 1] == '\\') {
                switch (lex[char_start + 2]) {
                    case 'n':  cv = '\n'; break;
                    case 't':  cv = '\t'; break;
                    case 'r':  cv = '\r'; break;
                    case '\\': cv = '\\'; break;
                    case '\'': cv = '\''; break;
                    case '"':  cv = '"';  break;
                    case 'a':  cv = '\a'; break;
                    case 'b':  cv = '\b'; break;
                    case 'f':  cv = '\f'; break;
                    case 'v':  cv = '\v'; break;
                    case 'x': {
                        // \xNN
                        cv = strtol(lex.c_str() + char_start + 3, nullptr, 16);
                        break;
                    }
                    case 'u': {
                        // \uXXXX — 4 hex digits
                        const char* p = lex.c_str() + char_start + 3;
                        cv = 0;
                        for (int k = 0; k < 4 && *p && isxdigit((unsigned char)*p); k++, p++)
                            cv = cv * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                            tolower((unsigned char)*p) - 'a' + 10);
                        break;
                    }
                    case 'U': {
                        // \UXXXXXXXX — 8 hex digits
                        const char* p = lex.c_str() + char_start + 3;
                        cv = 0;
                        for (int k = 0; k < 8 && *p && isxdigit((unsigned char)*p); k++, p++)
                            cv = cv * 16 + (isdigit((unsigned char)*p) ? *p - '0' :
                                            tolower((unsigned char)*p) - 'a' + 10);
                        break;
                    }
                    default:
                        if (lex[char_start + 2] >= '0' && lex[char_start + 2] <= '7') {
                            cv = strtol(lex.c_str() + char_start + 2, nullptr, 8);
                        } else {
                            cv = (unsigned char)lex[char_start + 2];
                        }
                        break;
                }
            } else {
                // For wide chars (L'...'), decode UTF-8 to Unicode codepoint
                const unsigned char c0 = (unsigned char)lex[char_start + 1];
                if (char_start > 0 && c0 >= 0x80) {
                    // Multi-byte UTF-8
                    const char* p = lex.c_str() + char_start + 1;
                    if ((c0 & 0xE0) == 0xC0 && p[1] && p[1] != '\'') {
                        cv = ((c0 & 0x1F) << 6) | ((unsigned char)p[1] & 0x3F);
                    } else if ((c0 & 0xF0) == 0xE0 && p[1] && p[2] && p[2] != '\'') {
                        cv = ((c0 & 0x0F) << 12) | (((unsigned char)p[1] & 0x3F) << 6) |
                             ((unsigned char)p[2] & 0x3F);
                    } else if ((c0 & 0xF8) == 0xF0 && p[1] && p[2] && p[3] && p[3] != '\'') {
                        cv = ((c0 & 0x07) << 18) | (((unsigned char)p[1] & 0x3F) << 12) |
                             (((unsigned char)p[2] & 0x3F) << 6) | ((unsigned char)p[3] & 0x3F);
                    } else {
                        cv = c0;
                    }
                } else {
                    cv = c0;
                }
            }
        }
        const char* raw_lex = arena_.strdup(lex);
        consume();
        Node* n = make_node(NK_CHAR_LIT, ln);
        n->ival = cv;
        n->sval = raw_lex;  // store raw lexeme to detect L prefix for wide char
        return n;
    }

    // C++ named casts: static_cast<T>(expr), reinterpret_cast<T>(expr), const_cast<T>(expr)
    if (check(TokenKind::KwStaticCast) ||
        check(TokenKind::KwReinterpretCast) ||
        check(TokenKind::KwConstCast)) {
        consume();  // consume cast keyword
        expect(TokenKind::Less);       // <
        TypeSpec cast_ts = parse_type_name();
        expect(TokenKind::Greater);    // >
        expect(TokenKind::LParen);     // (
        Node* operand = parse_assign_expr();
        expect(TokenKind::RParen);     // )
        Node* n = make_node(NK_CAST, ln);
        n->type = cast_ts;
        n->left = operand;
        return n;
    }

    // Parenthesised expression or cast or compound literal
    if (check(TokenKind::LParen)) {
        consume();  // consume (

        // Check for cast: (type-name)expr
        if (is_type_start()) {
            int save_pos = pos_;
            TypeSpec cast_ts = parse_type_name();
            if (check(TokenKind::RParen)) {
                consume();  // consume )
                // Check for compound literal: (type){ ... }
                if (check(TokenKind::LBrace)) {
                    Node* init_list = parse_init_list();
                    Node* n = make_node(NK_COMPOUND_LIT, ln);
                    n->type     = cast_ts;
                    n->left     = init_list;
                    return n;
                }
                // Regular cast
                Node* operand = parse_unary();
                Node* n = make_node(NK_CAST, ln);
                n->type = cast_ts;
                n->left = operand;
                // Phase C: propagate fn_ptr params from typedef onto cast node.
                if (cast_ts.is_fn_ptr && !last_resolved_typedef_.empty()) {
                    auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
                    if (tdit != typedef_fn_ptr_info_.end()) {
                        n->fn_ptr_params = tdit->second.params;
                        n->n_fn_ptr_params = tdit->second.n_params;
                        n->fn_ptr_variadic = tdit->second.variadic;
                    }
                }
                return n;
            } else {
                // Not a type name after all — restore and parse as expression
                pos_ = save_pos;
            }
        }

        // GCC statement expression: ({ ... })
        if (check(TokenKind::LBrace)) {
            Node* block = parse_block();
            expect(TokenKind::RParen);
            Node* n = make_node(NK_STMT_EXPR, ln);
            n->body = block;
            return n;
        }

        Node* inner = parse_expr();
        expect(TokenKind::RParen);
        return parse_postfix(inner);
    }

    // Global-qualified identifier (::name or ::ns::name)
    if (is_cpp_mode() && check(TokenKind::ColonColon) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::Identifier) {
        QualifiedNameRef qn = parse_qualified_name(true);
        std::string qualified_name = qn.base_name;
        if (!qn.qualifier_segments.empty()) {
            int context_id = resolve_namespace_context(qn);
            if (context_id >= 0) {
                qualified_name = canonical_name_in_context(context_id, qn.base_name);
            } else {
                qualified_name.clear();
                for (size_t i = 0; i < qn.qualifier_segments.size(); ++i) {
                    qualified_name += qn.qualifier_segments[i];
                    qualified_name += "::";
                }
                qualified_name += qn.base_name;
            }
        }
        const char* nm = arena_.strdup(qualified_name.c_str());
        Node* var_node = make_node(NK_VAR, ln);
        var_node->name = nm;
        apply_qualified_name(var_node, qn, nm);
        return parse_postfix(var_node);
    }

    // Identifier / label address
    if (check(TokenKind::Identifier)) {
        int ident_start = pos_;
        QualifiedNameRef qn = parse_qualified_name(false);
        auto resolve_type_name_for_qn = [&](const QualifiedNameRef& type_qn) -> std::string {
            std::string resolved;
            if (!type_qn.qualifier_segments.empty()) {
                std::string scoped;
                for (size_t i = 0; i < type_qn.qualifier_segments.size(); ++i) {
                    if (i) scoped += "::";
                    scoped += type_qn.qualifier_segments[i];
                }
                if (!scoped.empty()) scoped += "::";
                scoped += type_qn.base_name;
                if (typedef_types_.count(scoped) > 0) return scoped;
                int context_id = resolve_namespace_context(type_qn);
                if (context_id >= 0) {
                    resolved = canonical_name_in_context(context_id, type_qn.base_name);
                    if (typedef_types_.count(resolved) > 0) return resolved;
                }
                return scoped;
            }
            resolved = resolve_visible_type_name(type_qn.base_name);
            return resolved.empty() ? type_qn.base_name : resolved;
        };
        if (is_cpp_mode() && (check(TokenKind::Less) || check(TokenKind::LParen))) {
            const std::string candidate_type_name = resolve_type_name_for_qn(qn);
            if (!candidate_type_name.empty() &&
                typedef_types_.count(candidate_type_name) > 0) {
                pos_ = ident_start;
                std::string saved_typedef = last_resolved_typedef_;
                TypeSpec cast_ts = parse_base_type();
                while (check(TokenKind::Star)) {
                    consume();
                    cast_ts.ptr_level++;
                }
                if (check(TokenKind::AmpAmp)) {
                    consume();
                    cast_ts.is_rvalue_ref = true;
                } else if (check(TokenKind::Amp)) {
                    consume();
                    cast_ts.is_lvalue_ref = true;
                }
                if (check(TokenKind::LParen)) {
                    consume();
                    std::vector<Node*> args;
                    if (!check(TokenKind::RParen)) {
                        while (!at_end() && !check(TokenKind::RParen)) {
                            args.push_back(parse_assign_expr());
                            if (!match(TokenKind::Comma)) break;
                        }
                    }
                    expect(TokenKind::RParen);
                    const bool ctor_like_type =
                        cast_ts.base == TB_STRUCT || cast_ts.base == TB_UNION ||
                        cast_ts.base == TB_TYPEDEF;
                    if (ctor_like_type && args.size() != 1) {
                        Node* callee = make_var(cast_ts.tag ? cast_ts.tag : candidate_type_name.c_str(), ln);
                        Node* call = make_node(NK_CALL, ln);
                        call->left = callee;
                        call->n_children = static_cast<int>(args.size());
                        if (!args.empty()) {
                            call->children = arena_.alloc_array<Node*>(call->n_children);
                            for (int i = 0; i < call->n_children; ++i) call->children[i] = args[i];
                        }
                        return parse_postfix(call);
                    }
                    Node* operand = args.empty() ? make_int_lit(0, ln) : args[0];
                    Node* n = make_node(NK_CAST, ln);
                    n->type = cast_ts;
                    n->left = operand;
                    if (cast_ts.is_fn_ptr && !last_resolved_typedef_.empty()) {
                        auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
                        if (tdit != typedef_fn_ptr_info_.end()) {
                            n->fn_ptr_params = tdit->second.params;
                            n->n_fn_ptr_params = tdit->second.n_params;
                            n->fn_ptr_variadic = tdit->second.variadic;
                        }
                    }
                    return parse_postfix(n);
                }
                pos_ = ident_start;
                last_resolved_typedef_ = saved_typedef;
                qn = parse_qualified_name(false);
            }
        }
        std::string qualified_name;
        if (!qn.qualifier_segments.empty()) {
            int context_id = resolve_namespace_context(qn);
            if (context_id >= 0) {
                auto alias_it = using_value_aliases_.find(context_id);
                if (alias_it != using_value_aliases_.end()) {
                    auto value_it = alias_it->second.find(qn.base_name);
                    if (value_it != alias_it->second.end()) {
                        qualified_name = value_it->second;
                    }
                }
                if (qualified_name.empty()) {
                    qualified_name = canonical_name_in_context(context_id, qn.base_name);
                }
            } else {
                for (size_t i = 0; i < qn.qualifier_segments.size(); ++i) {
                    if (i) qualified_name += "::";
                    qualified_name += qn.qualifier_segments[i];
                }
                if (!qualified_name.empty()) qualified_name += "::";
                qualified_name += qn.base_name;
            }
        } else {
            qualified_name = resolve_visible_value_name(qn.base_name);
        }
        const char* nm = arena_.strdup(qualified_name.c_str());
        const BuiltinId builtin_id = builtin_id_from_name(nm);
        // __builtin_offsetof(type, member) — parse as constant expression when possible
        if (builtin_id == BuiltinId::Offsetof) {
            expect(TokenKind::LParen);
            TypeSpec base_ts = parse_type_name();
            expect(TokenKind::Comma);
            if (!check(TokenKind::Identifier)) {
                throw std::runtime_error("expected member name in __builtin_offsetof");
            }
            std::string field_path = cur().lexeme;
            consume();
            while (match(TokenKind::Dot)) {
                if (!check(TokenKind::Identifier)) {
                    throw std::runtime_error("expected member name after '.' in __builtin_offsetof");
                }
                field_path += ".";
                field_path += cur().lexeme;
                consume();
            }
            expect(TokenKind::RParen);
            Node* off = make_node(NK_OFFSETOF, ln);
            off->type = base_ts;
            off->name = arena_.strdup(field_path);
            long long cv = 0;
            if (eval_const_int(off, &cv, &struct_tag_def_map_)) return make_int_lit(cv, ln);
            return off;
        }
        // __builtin_types_compatible_p(type1, type2) — evaluate at parse time
        if (builtin_id == BuiltinId::TypesCompatibleP) {
            expect(TokenKind::LParen);
            TypeSpec ts1 = parse_type_name();
            expect(TokenKind::Comma);
            TypeSpec ts2 = parse_type_name();
            expect(TokenKind::RParen);
            int result = types_compatible_p(ts1, ts2, typedef_types_) ? 1 : 0;
            return make_int_lit(result, ln);
        }
        Node* ident = make_var(nm, ln);
        apply_qualified_name(ident, qn, nm);
        if (is_cpp_mode() && check(TokenKind::Less)) {
            int save_pos = pos_;
            consume();  // <
            std::vector<TypeSpec> template_args;
            std::vector<bool> template_arg_is_val;
            std::vector<long long> template_arg_vals;
            std::vector<const char*> template_arg_nttp_names;
            bool ok = true;
            while (!at_end() && !check_template_close()) {
                if (is_type_start()) {
                    template_args.push_back(parse_type_name());
                    template_arg_is_val.push_back(false);
                    template_arg_vals.push_back(0);
                    template_arg_nttp_names.push_back(nullptr);
                } else if (is_cpp_mode() && check(TokenKind::KwSizeof) &&
                           pos_ + 1 < static_cast<int>(tokens_.size()) &&
                           tokens_[pos_ + 1].kind == TokenKind::Ellipsis) {
                    // C++ non-type template argument: sizeof...(Pack)
                    consume(); // sizeof
                    consume(); // ...
                    expect(TokenKind::LParen);
                    if (!check(TokenKind::RParen)) consume(); // pack name
                    expect(TokenKind::RParen);
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_args.push_back(dummy);
                    template_arg_is_val.push_back(true);
                    template_arg_vals.push_back(0);
                    template_arg_nttp_names.push_back(nullptr);
                } else if (check(TokenKind::IntLit) ||
                           check(TokenKind::CharLit) ||
                           (is_cpp_mode() &&
                            (check(TokenKind::KwTrue) || check(TokenKind::KwFalse))) ||
                           (check(TokenKind::Minus) && pos_ + 1 < (int)tokens_.size() &&
                            tokens_[pos_ + 1].kind == TokenKind::IntLit)) {
                    // Non-type template argument: integer constant.
                    long long sign = 1;
                    if (match(TokenKind::Minus)) sign = -1;
                    long long val = 0;
                    if (check(TokenKind::KwTrue)) {
                        val = 1;
                        consume();
                    } else if (check(TokenKind::KwFalse)) {
                        val = 0;
                        consume();
                    } else if (check(TokenKind::CharLit)) {
                        Node* lit = parse_primary();
                        val = lit ? lit->ival : 0;
                    } else {
                        val = parse_int_lexeme(cur().lexeme.c_str());
                        consume();
                    }
                    val *= sign;
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_args.push_back(dummy);
                    template_arg_is_val.push_back(true);
                    template_arg_vals.push_back(val);
                    template_arg_nttp_names.push_back(nullptr);
                } else if (check(TokenKind::Identifier) && !is_type_start()) {
                    // Non-type template argument: forwarded NTTP name (e.g. compute<N>()).
                    const char* fwd_name = arena_.strdup(cur().lexeme.c_str());
                    consume();
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_args.push_back(dummy);
                    template_arg_is_val.push_back(true);
                    template_arg_vals.push_back(0);  // placeholder; resolved by HIR
                    template_arg_nttp_names.push_back(fwd_name);
                } else {
                    ok = false;
                    break;
                }
                if (!match(TokenKind::Comma)) break;
            }
            if (ok && match_template_close()) {
                if (check(TokenKind::LParen) || check(TokenKind::ColonColon)) {
                    // Accept template instantiation followed by ( (call) or :: (static member).
                    ident->has_template_args = true;
                    ident->n_template_args = (int)template_args.size();
                    ident->template_arg_types = arena_.alloc_array<TypeSpec>(ident->n_template_args);
                    ident->template_arg_is_value = arena_.alloc_array<bool>(ident->n_template_args);
                    ident->template_arg_values = arena_.alloc_array<long long>(ident->n_template_args);
                    ident->template_arg_nttp_names = arena_.alloc_array<const char*>(ident->n_template_args);
                    for (int i = 0; i < ident->n_template_args; ++i) {
                        ident->template_arg_types[i] = template_args[i];
                        ident->template_arg_is_value[i] = template_arg_is_val[i];
                        ident->template_arg_values[i] = template_arg_vals[i];
                        ident->template_arg_nttp_names[i] = template_arg_nttp_names[i];
                    }
                    // Template<A,B>::member — resolve as qualified name
                    if (check(TokenKind::ColonColon)) {
                        consume(); // eat ::
                        if (check(TokenKind::Identifier)) {
                            std::string member_name = std::string(ident->name) + "::" + cur().lexeme;
                            consume();
                            ident->name = arena_.strdup(member_name.c_str());
                        }
                    }
                } else {
                    pos_ = save_pos;
                }
            } else {
                pos_ = save_pos;
            }
        }
        return parse_postfix(ident);
    }

    // _Generic selection
    if (check(TokenKind::KwGeneric)) {
        consume();
        expect(TokenKind::LParen);
        Node* ctrl = parse_assign_expr();
        std::vector<Node*> assocs;
        while (!check(TokenKind::RParen) && !at_end()) {
            if (!match(TokenKind::Comma)) break;
            if (check(TokenKind::RParen)) break;
            Node* assoc = make_node(NK_CAST, cur().line);
            if (check(TokenKind::KwDefault)) {
                consume();
                assoc->ival = 1;  // marks as default association
            } else {
                assoc->type = parse_type_name();
            }
            expect(TokenKind::Colon);
            assoc->left = parse_assign_expr();
            assocs.push_back(assoc);
        }
        expect(TokenKind::RParen);
        Node* n = make_node(NK_GENERIC, ln);
        n->left = ctrl;
        n->children = (Node**)arena_.alloc(assocs.size() * sizeof(Node*));
        n->n_children = (int)assocs.size();
        for (int i = 0; i < (int)assocs.size(); i++) n->children[i] = assocs[i];
        return n;
    }

    // Initializer list in some contexts — just parse it
    if (check(TokenKind::LBrace)) {
        return parse_init_list();
    }

    // C++ functional cast: T(expr)
    if (is_cpp_mode() && is_type_start()) {
        int save_pos = pos_;
        std::string saved_typedef = last_resolved_typedef_;
        TypeSpec cast_ts = parse_base_type();
        while (check(TokenKind::Star)) {
            consume();
            cast_ts.ptr_level++;
        }
        if (check(TokenKind::AmpAmp)) {
            consume();
            cast_ts.is_rvalue_ref = true;
        } else if (check(TokenKind::Amp)) {
            consume();
            cast_ts.is_lvalue_ref = true;
        }
        if (check(TokenKind::LParen)) {
            consume();  // (
            std::vector<Node*> args;
            if (!check(TokenKind::RParen)) {
                while (!at_end() && !check(TokenKind::RParen)) {
                    Node* arg = nullptr;
                    if (is_cpp_mode() && check(TokenKind::Identifier) &&
                        pos_ + 2 < static_cast<int>(tokens_.size()) &&
                        tokens_[pos_ + 1].kind == TokenKind::ColonColon &&
                        tokens_[pos_ + 2].kind == TokenKind::Identifier) {
                        QualifiedNameRef operand_name = parse_qualified_name(false);
                        std::string qualified_name;
                        int context_id = resolve_namespace_context(operand_name);
                        if (context_id >= 0) {
                            qualified_name = canonical_name_in_context(context_id, operand_name.base_name);
                        } else {
                            for (size_t i = 0; i < operand_name.qualifier_segments.size(); ++i) {
                                if (i) qualified_name += "::";
                                qualified_name += operand_name.qualifier_segments[i];
                            }
                            if (!qualified_name.empty()) qualified_name += "::";
                            qualified_name += operand_name.base_name;
                        }
                        const char* nm = arena_.strdup(qualified_name.c_str());
                        arg = make_var(nm, ln);
                        apply_qualified_name(arg, operand_name, nm);
                    } else {
                        arg = parse_assign_expr();
                    }
                    if (arg) args.push_back(arg);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);
            const bool ctor_like_type =
                cast_ts.base == TB_STRUCT || cast_ts.base == TB_UNION ||
                cast_ts.base == TB_TYPEDEF;
            if (ctor_like_type && args.size() != 1) {
                Node* callee = make_var(cast_ts.tag ? cast_ts.tag : "<ctor>", ln);
                Node* call = make_node(NK_CALL, ln);
                call->left = callee;
                call->n_children = static_cast<int>(args.size());
                if (!args.empty()) {
                    call->children = arena_.alloc_array<Node*>(call->n_children);
                    for (int i = 0; i < call->n_children; ++i) call->children[i] = args[i];
                }
                return parse_postfix(call);
            }
            Node* operand = args.empty() ? make_int_lit(0, ln) : args[0];
            Node* n = make_node(NK_CAST, ln);
            n->type = cast_ts;
            n->left = operand;
            if (cast_ts.is_fn_ptr && !last_resolved_typedef_.empty()) {
                auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
                if (tdit != typedef_fn_ptr_info_.end()) {
                    n->fn_ptr_params = tdit->second.params;
                    n->n_fn_ptr_params = tdit->second.n_params;
                    n->fn_ptr_variadic = tdit->second.variadic;
                }
            }
            return parse_postfix(n);
        }
        pos_ = save_pos;
        last_resolved_typedef_ = saved_typedef;
    }

    // Hard error for unrecognized primaries (was permissive fallback to 0).
    std::string tok = at_end() ? "<eof>" : cur().lexeme;
    throw std::runtime_error("unexpected token in expression: " + tok);
}

// ── initializer parsing ───────────────────────────────────────────────────────

Node* Parser::parse_initializer() {
    if (check(TokenKind::LBrace)) {
        return parse_init_list();
    }
    return parse_assign_expr();
}

Node* Parser::parse_init_list() {
    int ln = cur().line;
    expect(TokenKind::LBrace);
    std::vector<Node*> items;
    while (!at_end() && !check(TokenKind::RBrace)) {
        Node* item = make_node(NK_INIT_ITEM, cur().line);

        // Designator?
        // Old GCC-style: `fieldname: value` (same as `.fieldname = value`)
        if (check(TokenKind::Identifier) &&
            peek(1).kind == TokenKind::Colon) {
            item->desig_field    = arena_.strdup(cur().lexeme);
            item->is_designated  = true;
            item->is_index_desig = false;
            consume();  // identifier
            consume();  // ':'
            item->left = parse_initializer();
            items.push_back(item);
            if (!match(TokenKind::Comma)) break;
            if (check(TokenKind::RBrace)) break;
            continue;
        }
        if (check(TokenKind::Dot)) {
            consume();
            if (check(TokenKind::Identifier)) {
                item->desig_field    = arena_.strdup(cur().lexeme);
                item->is_designated  = true;
                item->is_index_desig = false;
                consume();
            }
            // Multi-level designator: .a.b = val → synthesize nested { .b = val }
            if (check(TokenKind::Dot) || check(TokenKind::LBracket)) {
                // Recursively build nested init list for the remaining chain
                std::function<Node*()> build_nested = [&]() -> Node* {
                    int ln2 = cur().line;
                    Node* inner = make_node(NK_INIT_ITEM, ln2);
                    if (check(TokenKind::Dot)) {
                        consume();
                        if (check(TokenKind::Identifier)) {
                            inner->desig_field    = arena_.strdup(cur().lexeme);
                            inner->is_designated  = true;
                            inner->is_index_desig = false;
                            consume();
                        }
                    } else if (check(TokenKind::LBracket)) {
                        consume();
                        Node* ie = parse_assign_expr();
                        long long iv = (ie && ie->kind == NK_INT_LIT) ? ie->ival : 0;
                        expect(TokenKind::RBracket);
                        inner->is_designated  = true;
                        inner->is_index_desig = true;
                        inner->desig_val      = iv;
                    }
                    if (check(TokenKind::Dot) || check(TokenKind::LBracket)) {
                        inner->left = build_nested();
                    } else {
                        match(TokenKind::Assign);
                        inner->left = parse_initializer();
                    }
                    Node* lst2 = make_node(NK_INIT_LIST, ln2);
                    Node** ch = (Node**)arena_.alloc(sizeof(Node*));
                    ch[0] = inner;
                    lst2->children = ch;
                    lst2->n_children = 1;
                    return lst2;
                };
                item->left = build_nested();
                items.push_back(item);
                if (!match(TokenKind::Comma)) break;
                if (check(TokenKind::RBrace)) break;
                continue;
            }
            match(TokenKind::Assign);
        } else if (check(TokenKind::LBracket)) {
            consume();
            Node* idx_expr = parse_assign_expr();
            long long idx_lo = 0;
            if (idx_expr && idx_expr->kind == NK_INT_LIT) idx_lo = idx_expr->ival;
            // GCC range: [lo ... hi]
            long long idx_hi = idx_lo;
            if (check(TokenKind::Ellipsis)) {
                consume();
                Node* hi_expr = parse_assign_expr();
                if (hi_expr && hi_expr->kind == NK_INT_LIT) idx_hi = hi_expr->ival;
            }
            expect(TokenKind::RBracket);
            match(TokenKind::Assign);
            item->is_designated  = true;
            item->is_index_desig = true;
            item->desig_val      = idx_lo;
            // For ranges, store hi in right->ival
            if (idx_hi != idx_lo) {
                Node* hi_node = make_int_lit(idx_hi, cur().line);
                item->right = hi_node;
            }
        }

        item->left = parse_initializer();
        items.push_back(item);

        if (!match(TokenKind::Comma)) break;
        if (check(TokenKind::RBrace)) break;
    }
    expect(TokenKind::RBrace);

    std::vector<Node*> expanded_items;
    expanded_items.reserve(items.size());
    for (Node* item : items) {
        if (!item || item->kind != NK_INIT_ITEM || !item->is_designated ||
            !item->is_index_desig || !item->right || item->right->kind != NK_INT_LIT ||
            item->right->ival <= item->desig_val) {
            expanded_items.push_back(item);
            continue;
        }

        const long long lo = item->desig_val;
        const long long hi = item->right->ival;
        for (long long idx = lo; idx <= hi; ++idx) {
            Node* clone = make_node(NK_INIT_ITEM, item->line);
            clone->is_designated = true;
            clone->is_index_desig = true;
            clone->desig_val = idx;
            clone->left = item->left;
            expanded_items.push_back(clone);
        }
    }

    Node* list = make_node(NK_INIT_LIST, ln);
    list->n_children = (int)expanded_items.size();
    if (list->n_children > 0) {
        list->children = arena_.alloc_array<Node*>(list->n_children);
        for (int i = 0; i < list->n_children; ++i) list->children[i] = expanded_items[i];
    }
    return list;
}

// ── statement parsing ─────────────────────────────────────────────────────────


}  // namespace c4c

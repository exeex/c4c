#include "parser.hpp"
#include "parser_internal.hpp"

#include <cassert>
#include <cstring>
#include <functional>
#include <stdexcept>

namespace c4c {

static void append_type_mangled_suffix_local(std::string& out, const TypeSpec& ts) {
    switch (ts.base) {
        case TB_INT: out += "int"; break;
        case TB_UINT: out += "uint"; break;
        case TB_CHAR: out += "char"; break;
        case TB_SCHAR: out += "schar"; break;
        case TB_UCHAR: out += "uchar"; break;
        case TB_SHORT: out += "short"; break;
        case TB_USHORT: out += "ushort"; break;
        case TB_LONG: out += "long"; break;
        case TB_ULONG: out += "ulong"; break;
        case TB_LONGLONG: out += "llong"; break;
        case TB_ULONGLONG: out += "ullong"; break;
        case TB_FLOAT: out += "float"; break;
        case TB_DOUBLE: out += "double"; break;
        case TB_LONGDOUBLE: out += "ldouble"; break;
        case TB_VOID: out += "void"; break;
        case TB_BOOL: out += "bool"; break;
        case TB_INT128: out += "i128"; break;
        case TB_UINT128: out += "u128"; break;
        case TB_STRUCT: out += "struct_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_UNION: out += "union_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_ENUM: out += "enum_"; out += (ts.tag ? ts.tag : "anon"); break;
        case TB_TYPEDEF: out += (ts.tag ? ts.tag : "typedef"); break;
        default: out += "T"; break;
    }
    for (int p = 0; p < ts.ptr_level; ++p) out += "_ptr";
    if (ts.is_lvalue_ref) out += "_ref";
    if (ts.is_rvalue_ref) out += "_rref";
}

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
    ParseContextGuard trace(this, __func__);
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
    ParseContextGuard trace(this, __func__);
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
    ParseContextGuard trace(this, __func__);
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

Node* Parser::parse_sizeof_pack_expr(int ln) {
    expect(TokenKind::Ellipsis);
    expect(TokenKind::LParen);

    int pack_start = pos_;
    Node* pack_expr = nullptr;
    if (!check(TokenKind::RParen)) {
        pack_expr = parse_assign_expr();
    }
    int pack_end = pos_;

    expect(TokenKind::RParen);

    std::string pack_text;
    for (int i = pack_start; i < pack_end && i < static_cast<int>(tokens_.size()); ++i) {
        pack_text += tokens_[i].lexeme;
    }

    Node* n = make_node(NK_SIZEOF_PACK, ln);
    n->left = pack_expr;
    n->sval = arena_.strdup(pack_text.c_str());
    n->type.base = TB_ULONG;
    return n;
}

Node* Parser::parse_unary() {
    ParseContextGuard trace(this, __func__);
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
            consume();  // consume 'delete'
            bool is_array = false;
            if (check(TokenKind::LBracket) &&
                pos_ + 1 < static_cast<int>(tokens_.size()) &&
                tokens_[pos_ + 1].kind == TokenKind::RBracket) {
                consume();  // '['
                consume();  // ']'
                is_array = true;
            }
            Node* operand = parse_unary();
            Node* n = make_node(NK_DELETE_EXPR, ln);
            n->left = operand;
            n->ival = is_array ? 1 : 0;
            return n;
        }
        case TokenKind::KwNew: {
            // C++ new expression: new T, new T(args), new T[n]
            return parse_new_expr(ln, false /*not global-qualified*/);
        }
        case TokenKind::KwSizeof: {
            consume();
            if (is_cpp_mode() && check(TokenKind::Ellipsis)) {
                return parse_sizeof_pack_expr(ln);
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
        case TokenKind::KwNoexcept: {
            consume();
            expect(TokenKind::LParen);
            Node* inner = parse_assign_expr();
            expect(TokenKind::RParen);
            return make_unary("noexcept", inner, ln);
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

Node* Parser::parse_new_expr(int ln, bool global_qualified) {
    // Called after 'new' has been consumed.
    // Syntax: [::] new [( placement_args )] type [( ctor_args )] | type [ array_size ]
    consume(); // consume 'new'

    Node* n = make_node(NK_NEW_EXPR, ln);
    n->is_global_qualified = global_qualified;

    // Optional placement args: new (args...) T(...)
    // Placement args are comma-separated expressions in parentheses.
    // Stored in n->params / n->n_params to avoid conflicts with ctor args.
    if (check(TokenKind::LParen)) {
        // Disambiguate: could be placement args or a parenthesized type.
        // Peek: if '(' followed by a type start and then ')', it's a cast-style type.
        // For placement new, arguments are expressions.
        int saved = pos_;
        consume(); // '('
        bool is_placement = true;
        if (is_type_start()) {
            // Still parse as expression — placement args like (void*)ptr are common.
            pos_ = saved;
            consume(); // '('
        }
        // Parse comma-separated placement args.
        std::vector<Node*> pargs;
        while (!check(TokenKind::RParen)) {
            pargs.push_back(parse_assign_expr());
            if (!check(TokenKind::RParen)) expect(TokenKind::Comma);
        }
        expect(TokenKind::RParen);
        n->n_params = static_cast<int>(pargs.size());
        if (!pargs.empty()) {
            n->params = arena_.alloc_array<Node*>(n->n_params);
            for (int i = 0; i < n->n_params; ++i) n->params[i] = pargs[i];
            n->left = pargs[0]; // backward compat: first placement arg
        }
    }

    // Parse the allocated type.
    TypeSpec ts = parse_base_type();
    // Handle pointer levels in the type: new int*, new char* etc.
    while (check(TokenKind::Star)) {
        consume();
        ts.ptr_level++;
    }
    n->type = ts;

    // Check for array form: new T[n]
    if (check(TokenKind::LBracket)) {
        consume(); // '['
        Node* size_expr = parse_assign_expr();
        expect(TokenKind::RBracket);
        n->right = size_expr;
        n->ival = 1; // array new
    } else {
        n->ival = 0; // scalar new
    }

    // Optional constructor args: new T(args...)
    if (n->ival == 0 && check(TokenKind::LParen)) {
        consume(); // '('
        std::vector<Node*> args;
        while (!check(TokenKind::RParen)) {
            args.push_back(parse_assign_expr());
            if (!check(TokenKind::RParen)) expect(TokenKind::Comma);
        }
        expect(TokenKind::RParen);
        n->n_children = static_cast<int>(args.size());
        if (!args.empty()) {
            n->children = arena_.alloc_array<Node*>(n->n_children);
            for (int i = 0; i < n->n_children; ++i) n->children[i] = args[i];
        }
    }

    return n;
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
                std::string member_name;
                if (is_cpp_mode() && match(TokenKind::Tilde)) {
                    member_name = "~";
                    if (check(TokenKind::Identifier)) {
                        member_name += cur().lexeme;
                        consume();
                    } else {
                        throw std::runtime_error("expected destructor type name after '~'");
                    }
                } else if (check(TokenKind::Identifier)) {
                    member_name = cur().lexeme;
                    consume();
                } else if (!try_parse_operator_function_id(member_name)) {
                    break;
                }
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = arena_.strdup(member_name.c_str());
                n->is_arrow = false;
                base = n;
                break;
            }
            case TokenKind::Arrow: {
                consume();
                std::string member_name;
                if (is_cpp_mode() && match(TokenKind::Tilde)) {
                    member_name = "~";
                    if (check(TokenKind::Identifier)) {
                        member_name += cur().lexeme;
                        consume();
                    } else {
                        throw std::runtime_error("expected destructor type name after '~'");
                    }
                } else if (check(TokenKind::Identifier)) {
                    member_name = cur().lexeme;
                    consume();
                } else if (!try_parse_operator_function_id(member_name)) {
                    break;
                }
                Node* n = make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = arena_.strdup(member_name.c_str());
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
                    Node* arg = parse_assign_expr();
                    // C++ pack expansion in call args: func(args...)
                    if (is_cpp_mode() && check(TokenKind::Ellipsis)) {
                        consume();
                        Node* pack = make_node(NK_PACK_EXPANSION, ln);
                        pack->left = arg;
                        arg = pack;
                    }
                    args.push_back(arg);
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
                consume();
                Node* n = make_node(NK_PACK_EXPANSION, ln);
                n->left = base;
                base = n;
                break;
            }
            default:
                return base;
        }
    }
}

bool Parser::try_parse_operator_function_id(std::string& out_name) {
    if (!is_cpp_mode() || !match(TokenKind::KwOperator)) return false;

    if (match(TokenKind::KwNew)) {
        if (check(TokenKind::LBracket) &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::RBracket) {
            consume();
            consume();
            out_name = "operator_new_array";
        } else {
            out_name = "operator_new";
        }
        return true;
    }

    if (match(TokenKind::KwDelete)) {
        if (check(TokenKind::LBracket) &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::RBracket) {
            consume();
            consume();
            out_name = "operator_delete_array";
        } else {
            out_name = "operator_delete";
        }
        return true;
    }

    if (check(TokenKind::LBracket)) {
        consume();
        expect(TokenKind::RBracket);
        out_name = "operator_subscript";
        return true;
    }

    if (check(TokenKind::LParen)) {
        consume();
        expect(TokenKind::RParen);
        out_name = "operator_call";
        return true;
    }

    if (match(TokenKind::KwBool))              out_name = "operator_bool";
    else if (match(TokenKind::Star))           out_name = "operator_mul";
    else if (match(TokenKind::Arrow))          out_name = "operator_arrow";
    else if (match(TokenKind::PlusPlus))       out_name = "operator_preinc";
    else if (match(TokenKind::MinusMinus))     out_name = "operator_predec";
    else if (match(TokenKind::EqualEqual))     out_name = "operator_eq";
    else if (match(TokenKind::BangEqual))      out_name = "operator_neq";
    else if (match(TokenKind::Plus))           out_name = "operator_plus";
    else if (match(TokenKind::Minus))          out_name = "operator_minus";
    else if (match(TokenKind::Assign))         out_name = "operator_assign";
    else if (match(TokenKind::LessEqual))      out_name = "operator_le";
    else if (match(TokenKind::GreaterEqual))   out_name = "operator_ge";
    else if (match(TokenKind::Less))           out_name = "operator_lt";
    else if (match(TokenKind::Greater))        out_name = "operator_gt";
    else if (match(TokenKind::AmpAmp))         out_name = "operator_and";
    else if (match(TokenKind::PipePipe))       out_name = "operator_or";
    else if (match(TokenKind::Bang))           out_name = "operator_not";
    else if (match(TokenKind::Tilde))          out_name = "operator_bitnot";
    else if (match(TokenKind::Slash))          out_name = "operator_div";
    else if (match(TokenKind::Percent))        out_name = "operator_mod";
    else if (match(TokenKind::Amp))            out_name = "operator_bitand";
    else if (match(TokenKind::Pipe))           out_name = "operator_bitor";
    else if (match(TokenKind::Caret))          out_name = "operator_bitxor";
    else if (match(TokenKind::LessLess))       out_name = "operator_shl";
    else if (match(TokenKind::GreaterGreater)) out_name = "operator_shr";
    else if (match(TokenKind::PlusAssign))     out_name = "operator_plus_assign";
    else if (match(TokenKind::MinusAssign))    out_name = "operator_minus_assign";
    else if (match(TokenKind::StarAssign))     out_name = "operator_mul_assign";
    else if (match(TokenKind::SlashAssign))    out_name = "operator_div_assign";
    else if (match(TokenKind::PercentAssign))  out_name = "operator_mod_assign";
    else if (match(TokenKind::AmpAssign))      out_name = "operator_and_assign";
    else if (match(TokenKind::PipeAssign))     out_name = "operator_or_assign";
    else if (match(TokenKind::CaretAssign))    out_name = "operator_xor_assign";
    else if (match(TokenKind::LessLessAssign)) out_name = "operator_shl_assign";
    else if (match(TokenKind::GreaterGreaterAssign)) out_name = "operator_shr_assign";
    else return false;

    return true;
}

Node* Parser::parse_lambda_expr(int ln) {
    expect(TokenKind::LBracket);

    LambdaCaptureDefault capture_default = LCD_NONE;
    if (match(TokenKind::RBracket)) {
        capture_default = LCD_NONE;
    } else if (match(TokenKind::Amp)) {
        capture_default = LCD_BY_REFERENCE;
        expect(TokenKind::RBracket);
    } else if (match(TokenKind::Assign)) {
        capture_default = LCD_BY_COPY;
        expect(TokenKind::RBracket);
    } else {
        throw std::runtime_error("unsupported lambda capture list");
    }

    bool has_parameter_list = false;
    if (match(TokenKind::LParen)) {
        has_parameter_list = true;
        expect(TokenKind::RParen);
    }

    if (!check(TokenKind::LBrace)) {
        throw std::runtime_error("lambda body must be a compound statement");
    }

    Node* lambda = make_node(NK_LAMBDA, ln);
    lambda->lambda_capture_default = capture_default;
    lambda->lambda_has_parameter_list = has_parameter_list;
    lambda->body = parse_block();
    return lambda;
}


Node* Parser::parse_primary() {
    ParseContextGuard trace(this, __func__);
    int ln = cur().line;

    auto parse_cpp_requires_expression = [&]() -> Node* {
        if (!(is_cpp_mode() && check(TokenKind::KwRequires))) {
            return nullptr;
        }

        consume();  // requires
        if (check(TokenKind::LParen))
            skip_paren_group();
        if (!check(TokenKind::LBrace))
            return nullptr;

        skip_brace_group();
        return make_int_lit(1, ln);
    };

    auto parse_operator_ref = [&]() -> Node* {
        if (!is_cpp_mode()) return nullptr;
        int saved_pos = pos_;
        std::string qualified_name;

        if (match(TokenKind::ColonColon))
            qualified_name = "::";

        while (check(TokenKind::Identifier) &&
               pos_ + 1 < static_cast<int>(tokens_.size()) &&
               tokens_[pos_ + 1].kind == TokenKind::ColonColon) {
            if (!qualified_name.empty() &&
                qualified_name.substr(qualified_name.size() - 2) != "::") {
                qualified_name += "::";
            }
            qualified_name += cur().lexeme;
            consume();
            consume();
        }

        std::string op_name;
        if (!try_parse_operator_function_id(op_name)) {
            pos_ = saved_pos;
            return nullptr;
        }

        if (qualified_name == "::")
            qualified_name.clear();
        if (!qualified_name.empty() &&
            qualified_name.substr(qualified_name.size() - 2) != "::") {
            qualified_name += "::";
        }
        qualified_name += op_name;
        Node* ident = make_var(arena_.strdup(qualified_name.c_str()), ln);
        return parse_postfix(ident);
    };

    auto has_balanced_template_id_ahead = [&]() -> bool {
        if (!(is_cpp_mode() && check(TokenKind::Identifier) &&
              pos_ + 1 < static_cast<int>(tokens_.size()) &&
              tokens_[pos_ + 1].kind == TokenKind::Less)) {
            return true;
        }

        int depth = 0;
        for (int probe = pos_ + 1; probe < static_cast<int>(tokens_.size()); ++probe) {
            TokenKind k = tokens_[probe].kind;
            if (k == TokenKind::Less) {
                ++depth;
                continue;
            }
            if (k == TokenKind::Greater) {
                if (depth > 0 && --depth == 0) return true;
                continue;
            }
            if (k == TokenKind::GreaterGreater) {
                if (depth > 0) {
                    depth -= 2;
                    if (depth <= 0) return true;
                }
                continue;
            }
            if ((k == TokenKind::RParen || k == TokenKind::Comma ||
                 k == TokenKind::Semi || k == TokenKind::Question ||
                 k == TokenKind::Colon || k == TokenKind::LessEqual ||
                 k == TokenKind::GreaterEqual) &&
                depth > 0) {
                return false;
            }
        }
        return false;
    };

    auto parse_gcc_type_trait_builtin = [&]() -> Node* {
        if (!check(TokenKind::Identifier) ||
            pos_ + 1 >= static_cast<int>(tokens_.size()) ||
            tokens_[pos_ + 1].kind != TokenKind::LParen) {
            return nullptr;
        }

        const std::string builtin_name = cur().lexeme;
        if (!is_builtin_type_trait_name(builtin_name) ||
            builtin_name == "__builtin_types_compatible_p") {
            return nullptr;
        }

        if (!is_parser_supported_builtin_type_trait_name(builtin_name)) {
            throw std::runtime_error("not implemented builtin type trait: " +
                                     builtin_name);
        }

        const int saved_pos = pos_;
        consume();  // builtin identifier
        consume();  // (

        int parsed_args = 0;
        try {
            while (!check(TokenKind::RParen)) {
                parse_type_name();
                ++parsed_args;
                if (check(TokenKind::Ellipsis)) consume();
                if (!match(TokenKind::Comma)) break;
            }
            expect(TokenKind::RParen);
        } catch (...) {
            pos_ = saved_pos;
            return nullptr;
        }

        if (parsed_args == 0) {
            pos_ = saved_pos;
            return nullptr;
        }

        // These GCC/Clang type-trait builtins are parse-only placeholders for
        // now. The important behavior for this slice is to consume their
        // type-name argument lists instead of misparsing them as expressions.
        return make_int_lit(1, ln);
    };

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

    if (is_cpp_mode() && check(TokenKind::KwNullptr)) {
        consume();
        return make_int_lit(0, ln);
    }

    if (Node* requires_expr = parse_cpp_requires_expression())
        return requires_expr;

    if (is_cpp_mode() && check(TokenKind::LBracket)) {
        const int saved_pos = pos_;
        try {
            return parse_lambda_expr(ln);
        } catch (const std::exception&) {
            pos_ = saved_pos;
        }
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
        if (is_type_start() && has_balanced_template_id_ahead()) {
            int save_pos = pos_;
            TypeSpec cast_ts = parse_type_name();
            if (check(TokenKind::RParen)) {
                // In C++ mode, check if this is really a cast or a parenthesized
                // expression.  A C-style cast (type)expr requires a valid operand
                // after ')'.  If the token after ')' is itself ')', ';', ',', or
                // another expression terminator, this cannot be a cast — it must
                // be a parenthesized expression like ((ns::value)).
                if (is_cpp_mode()) {
                    int rp_pos = pos_;  // points at ')'
                    int after_rp = rp_pos + 1;
                    if (after_rp < static_cast<int>(tokens_.size())) {
                        auto ak = tokens_[after_rp].kind;
                        if (ak == TokenKind::RParen || ak == TokenKind::Semi ||
                            ak == TokenKind::Comma || ak == TokenKind::RBrace ||
                            ak == TokenKind::RBracket || ak == TokenKind::Colon ||
                            ak == TokenKind::Question ||
                            ak == TokenKind::PipePipe || ak == TokenKind::AmpAmp ||
                            ak == TokenKind::EqualEqual || ak == TokenKind::BangEqual ||
                            ak == TokenKind::LessEqual || ak == TokenKind::GreaterEqual ||
                            ak == TokenKind::Less || ak == TokenKind::Greater ||
                            ak == TokenKind::Pipe || ak == TokenKind::Caret) {
                            // Not a cast — restore and parse as paren expression.
                            pos_ = save_pos;
                            goto not_a_cast;
                        }
                    }
                }
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

        not_a_cast:
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

    if (Node* op_ref = parse_operator_ref()) {
        return op_ref;
    }

    if (Node* type_trait = parse_gcc_type_trait_builtin()) {
        return type_trait;
    }

    // C++ ::new expression (global-qualified new)
    if (is_cpp_mode() && check(TokenKind::ColonColon) &&
        pos_ + 1 < static_cast<int>(tokens_.size()) &&
        tokens_[pos_ + 1].kind == TokenKind::KwNew) {
        consume(); // consume '::'
        return parse_new_expr(ln, true /*global_qualified*/);
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
            std::vector<TemplateArgParseResult> parsed_args;
            if (parse_template_argument_list(&parsed_args)) {
                // Accept template instantiation when followed by a token that
                // would be valid after an expression (call, scope, brace-init,
                // close-paren, semicolon, comma, binary ops, etc.).
                // Reject only when the follow token looks like a new expression
                // start that would make more sense with < as comparison.
                auto is_valid_after_template = [&]() -> bool {
                    if (check(TokenKind::LParen) || check(TokenKind::ColonColon) ||
                        check(TokenKind::LBrace)) return true;
                    // Expression terminators / continuations
                    if (check(TokenKind::RParen) || check(TokenKind::Semi) ||
                        check(TokenKind::Comma) || check(TokenKind::Ellipsis) ||
                        check(TokenKind::RBracket) ||
                        check(TokenKind::RBrace)) return true;
                    // Binary operators
                    if (check(TokenKind::Pipe) || check(TokenKind::PipePipe) ||
                        check(TokenKind::AmpAmp) || check(TokenKind::Amp) ||
                        check(TokenKind::Question) || check(TokenKind::Colon) ||
                        check(TokenKind::EqualEqual) || check(TokenKind::BangEqual) ||
                        check(TokenKind::LessEqual) || check(TokenKind::GreaterEqual) ||
                        check(TokenKind::Plus) || check(TokenKind::Minus) ||
                        check(TokenKind::Star) || check(TokenKind::Slash) ||
                        check(TokenKind::Percent) || check(TokenKind::Assign) ||
                        check(TokenKind::Dot) || check(TokenKind::Arrow)) return true;
                    return false;
                };
                if (is_valid_after_template()) {
                    // Accept template instantiation.
                    ident->has_template_args = true;
                    ident->n_template_args = static_cast<int>(parsed_args.size());
                    ident->template_arg_types = arena_.alloc_array<TypeSpec>(ident->n_template_args);
                    ident->template_arg_is_value = arena_.alloc_array<bool>(ident->n_template_args);
                    ident->template_arg_values = arena_.alloc_array<long long>(ident->n_template_args);
                    ident->template_arg_nttp_names = arena_.alloc_array<const char*>(ident->n_template_args);
                    ident->template_arg_exprs = arena_.alloc_array<Node*>(ident->n_template_args);
                    for (int i = 0; i < ident->n_template_args; ++i) {
                        ident->template_arg_types[i] = parsed_args[i].type;
                        ident->template_arg_is_value[i] = parsed_args[i].is_value;
                        ident->template_arg_values[i] = parsed_args[i].value;
                        ident->template_arg_nttp_names[i] = parsed_args[i].nttp_name;
                        ident->template_arg_exprs[i] = parsed_args[i].expr;
                    }
                    // Template<A,B>::member — resolve as qualified name.
                    // Re-parse from ident_start to trigger template struct
                    // instantiation and get the mangled tag.
                    if (check(TokenKind::ColonColon)) {
                        consume(); // eat ::
                        if (check(TokenKind::Identifier)) {
                            std::string member = cur().lexeme;
                            consume();
                            std::string struct_tag = ident->name;
                            // Try to instantiate via parse_base_type to get mangled tag
                            int after_member_pos = pos_;
                            pos_ = ident_start;
                            try {
                                TypeSpec ts = parse_base_type();
                                if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
                                    // Deferred template struct: use the original
                                    // template name so HIR can find the primary
                                    // and resolve with concrete bindings later.
                                    struct_tag = ts.tpl_struct_origin;
                                } else if (ts.tag && ts.tag[0]) {
                                    struct_tag = ts.tag;
                                }
                            } catch (...) {
                                // fallback: use raw name
                            }
                            pos_ = after_member_pos;
                            std::string member_name = struct_tag + "::" + member;
                            ident->name = arena_.strdup(member_name.c_str());
                        }
                    }
                    // Brace-init after template: Type<Args>{} or Type<Args>{a, b}
                    // Preserve empty-brace temporaries as a zero-arg call shape.
                    if (check(TokenKind::LBrace)) {
                        const bool empty_braces =
                            pos_ + 1 < static_cast<int>(tokens_.size()) &&
                            tokens_[pos_ + 1].kind == TokenKind::RBrace;
                        consume(); // {
                        int depth = 1;
                        while (!at_end() && depth > 0) {
                            if (check(TokenKind::LBrace)) ++depth;
                            else if (check(TokenKind::RBrace)) --depth;
                            if (depth > 0) consume();
                        }
                        if (check(TokenKind::RBrace)) consume(); // }
                        if (empty_braces) {
                            Node* ctor = make_node(NK_CALL, ln);
                            ctor->left = ident;
                            ctor->n_children = 0;
                            ident = ctor;
                        }
                    }
                } else {
                    pos_ = save_pos;
                }
            } else {
                pos_ = save_pos;
            }
        }
        if (is_cpp_mode() && check(TokenKind::LBrace) &&
            pos_ + 1 < static_cast<int>(tokens_.size()) &&
            tokens_[pos_ + 1].kind == TokenKind::RBrace) {
            consume();
            consume();
            Node* ctor = make_node(NK_CALL, ln);
            ctor->left = ident;
            ctor->n_children = 0;
            ident = ctor;
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
    if (is_cpp_mode() && is_type_start() && has_balanced_template_id_ahead()) {
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

    // C++ lambda expression: [captures](params) -> ret { body }
    // Skip the entire lambda and return a placeholder (0) so template
    // function bodies and in-class initializers can parse through them.
    if (is_cpp_mode() && check(TokenKind::LBracket)) {
        // Skip capture list [...]
        consume(); // [
        int depth = 1;
        while (!at_end() && depth > 0) {
            if (check(TokenKind::LBracket)) ++depth;
            else if (check(TokenKind::RBracket)) --depth;
            if (depth > 0) consume();
        }
        if (check(TokenKind::RBracket)) consume(); // ]
        // Skip optional (params)
        if (check(TokenKind::LParen)) skip_paren_group();
        // Skip optional mutable/constexpr/consteval
        while (check(TokenKind::KwMutable) || check(TokenKind::KwConstexpr) ||
               check(TokenKind::KwConsteval) ||
               (check(TokenKind::Identifier) &&
                (cur().lexeme == "mutable" || cur().lexeme == "constexpr" ||
                 cur().lexeme == "consteval"))) consume();
        // Skip optional noexcept(...)
        skip_exception_spec();
        // Skip optional -> return_type
        if (check(TokenKind::Arrow)) {
            consume();
            parse_type_name();
        }
        // Skip body { ... }
        if (check(TokenKind::LBrace)) skip_brace_group();
        return make_int_lit(0, ln);
    }

    // Hard error for unrecognized primaries (was permissive fallback to 0).
    std::string tok = at_end() ? "<eof>" : cur().lexeme;
    note_parse_failure_message(("unexpected token in expression: " + tok).c_str());
    throw std::runtime_error("unexpected token in expression: " + tok);
}

// ── initializer parsing ───────────────────────────────────────────────────────

Node* Parser::parse_initializer() {
    ParseContextGuard trace(this, __func__);
    if (check(TokenKind::LBrace)) {
        return parse_init_list();
    }
    return parse_assign_expr();
}

Node* Parser::parse_init_list() {
    ParseContextGuard trace(this, __func__);
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

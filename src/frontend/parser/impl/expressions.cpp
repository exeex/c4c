#include <cassert>
#include <algorithm>
#include <cstring>
#include <functional>
#include <stdexcept>

#include "lexer.hpp"
#include "parser_impl.hpp"
#include "types/types_helpers.hpp"

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

static const Node* nested_record_definition_from_member_carrier(
    const Node* owner, TextId nested_text_id) {
    if (!owner || owner->n_fields <= 0 || !owner->fields ||
        nested_text_id == kInvalidText) {
        return nullptr;
    }

    for (int i = 0; i < owner->n_fields; ++i) {
        const Node* field = owner->fields[i];
        if (!field || field->unqualified_text_id != nested_text_id) continue;
        if (field->type.record_def &&
            field->type.record_def->kind == NK_STRUCT_DEF) {
            return field->type.record_def;
        }
    }
    return nullptr;
}

static TextId qualified_name_qualifier_text_id(
    const Parser& parser, const Parser::QualifiedNameRef& qn, size_t index) {
    if (index < qn.qualifier_text_ids.size() &&
        qn.qualifier_text_ids[index] != kInvalidText) {
        return qn.qualifier_text_ids[index];
    }
    if (index >= qn.qualifier_segments.size()) return kInvalidText;
    return parser.parser_text_id_for_token(kInvalidText,
                                           qn.qualifier_segments[index]);
}

static const Node* static_member_owner_record_from_structured_path(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    const size_t owner_segment_count = qn.qualifier_segments.size();
    if (owner_segment_count == 0) return nullptr;

    for (size_t first_record = 0; first_record < owner_segment_count;
         ++first_record) {
        Parser::QualifiedNameRef record_qn;
        record_qn.is_global_qualified = qn.is_global_qualified;
        record_qn.qualifier_segments.assign(qn.qualifier_segments.begin(),
                                            qn.qualifier_segments.begin() +
                                                first_record);
        for (size_t i = 0; i < first_record; ++i) {
            record_qn.qualifier_text_ids.push_back(
                qualified_name_qualifier_text_id(parser, qn, i));
        }
        record_qn.base_name = qn.qualifier_segments[first_record];
        record_qn.base_text_id =
            qualified_name_qualifier_text_id(parser, qn, first_record);

        const Node* record =
            qualified_type_record_definition_from_structured_authority(
                parser, record_qn);
        if (!record) continue;

        for (size_t nested_index = first_record + 1;
             record && nested_index < owner_segment_count; ++nested_index) {
            record = nested_record_definition_from_member_carrier(
                record,
                qualified_name_qualifier_text_id(parser, qn, nested_index));
        }
        if (record) return record;
    }

    return nullptr;
}

static const Node* canonical_static_member_owner_record(
    const Parser& parser, Parser::QualifiedNameRef& qn) {
    if (qn.qualifier_segments.empty()) return nullptr;

    Parser::QualifiedNameRef owner_qn;
    owner_qn.is_global_qualified = qn.is_global_qualified;
    const size_t owner_segment_count = qn.qualifier_segments.size();
    owner_qn.qualifier_segments.assign(qn.qualifier_segments.begin(),
                                       qn.qualifier_segments.end() - 1);
    owner_qn.qualifier_text_ids.assign(qn.qualifier_text_ids.begin(),
                                       qn.qualifier_text_ids.begin() +
                                           std::min(qn.qualifier_text_ids.size(),
                                                    owner_segment_count - 1));
    owner_qn.base_name = qn.qualifier_segments.back();
    owner_qn.base_text_id =
        qn.qualifier_text_ids.size() >= owner_segment_count
            ? qn.qualifier_text_ids[owner_segment_count - 1]
            : parser.parser_text_id_for_token(kInvalidText, owner_qn.base_name);

    const Node* owner =
        qualified_type_record_definition_from_structured_authority(parser,
                                                                   owner_qn);
    if (!owner) {
        owner = static_member_owner_record_from_structured_path(parser, qn);
    }
    if (!owner || owner->unqualified_text_id == kInvalidText) return nullptr;

    qn.qualifier_text_ids.resize(owner_segment_count, kInvalidText);
    qn.qualifier_text_ids[owner_segment_count - 1] =
        owner->unqualified_text_id;
    return owner;
}

int bin_prec(TokenKind k) {
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
        case TokenKind::Spaceship:
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

Node* parse_expr(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    Node* lhs = parse_assign_expr(parser);
    while (parser.check(TokenKind::Comma)) {
        int ln = parser.cur().line;
        parser.consume();
        Node* rhs = parse_assign_expr(parser);
        Node* n = parser.make_node(NK_COMMA_EXPR, ln);
        n->left  = lhs;
        n->right = rhs;
        lhs = n;
    }
    return lhs;
}

Node* parse_assign_expr(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    Node* lhs = parse_ternary(parser);
    if (parser.active_context_state_.template_arg_expr_depth > 0 &&
        parser.check_template_close())
        return lhs;
    int ln = parser.cur().line;
    TokenKind k = parser.cur().kind;
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
        parser.consume();
        Node* rhs = parse_assign_expr(parser);  // right-associative
        if (op[0] == '=' && op[1] == '\0') {
            return parser.make_assign("=", lhs, rhs, ln);
        } else {
            return parser.make_assign(op, lhs, rhs, ln);
        }
    }
    return lhs;
}

Node* parse_ternary(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    Node* cond = parse_binary(parser, 4);
    if (!parser.check(TokenKind::Question)) return cond;
    int ln = parser.cur().line;
    parser.consume();  // ?

    Node* then_node;
    if (parser.check(TokenKind::Colon)) {
        // GNU extension: cond ?: else  (omitted-middle)
        then_node = cond;
    } else {
        then_node = parse_expr(parser);
    }
    parser.expect(TokenKind::Colon);
    Node* else_node = parse_assign_expr(parser);

    Node* n = parser.make_node(NK_TERNARY, ln);
    n->cond  = cond;
    n->then_ = then_node;
    n->else_ = else_node;
    return n;
}

Node* parse_binary(Parser& parser, int min_prec) {
    Node* lhs = parse_unary(parser);
    while (true) {
        if (parser.active_context_state_.template_arg_expr_depth > 0 &&
            parser.check_template_close())
            break;
        int prec = bin_prec(parser.cur().kind);
        if (prec < min_prec) break;
        TokenKind k = parser.cur().kind;
        int ln = parser.cur().line;
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
            case TokenKind::Spaceship:        op = "<=>"; break;
            case TokenKind::Greater:          op = ">";  break;
            case TokenKind::GreaterEqual:     op = ">="; break;
            case TokenKind::AmpAmp:           op = "&&"; break;
            case TokenKind::PipePipe:         op = "||"; break;
            default:                          op = "?";  break;
        }
        parser.consume();
        Node* rhs = parse_binary(parser, prec + 1);  // left-associative
        lhs = parser.make_binop(op, lhs, rhs, ln);
    }
    return lhs;
}

Node* parse_sizeof_pack_expr(Parser& parser, int ln) {
    parser.expect(TokenKind::Ellipsis);
    parser.expect(TokenKind::LParen);

    int pack_start = parser.core_input_state_.pos;
    Node* pack_expr = nullptr;
    if (!parser.check(TokenKind::RParen)) {
        pack_expr = parse_assign_expr(parser);
    }
    int pack_end = parser.core_input_state_.pos;

    parser.expect(TokenKind::RParen);

    std::string pack_text;
    for (int i = pack_start; i < pack_end &&
                    i < static_cast<int>(parser.core_input_state_.tokens.size());
         ++i) {
        pack_text += parser.token_spelling(parser.core_input_state_.tokens[i]);
    }

    Node* n = parser.make_node(NK_SIZEOF_PACK, ln);
    n->left = pack_expr;
    n->sval = parser.arena_.strdup(pack_text.c_str());
    n->type.base = TB_ULONG;
    return n;
}

Node* parse_unary(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    switch (parser.cur().kind) {
        case TokenKind::Bang: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("!", operand, ln);
        }
        case TokenKind::Tilde: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("~", operand, ln);
        }
        case TokenKind::Minus: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("-", operand, ln);
        }
        case TokenKind::Plus: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("+", operand, ln);
        }
        case TokenKind::PlusPlus: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("++pre", operand, ln);
        }
        case TokenKind::MinusMinus: {
            parser.consume();
            Node* operand = parse_unary(parser);
            return parser.make_unary("--pre", operand, ln);
        }
        case TokenKind::AmpAmp: {
            // GCC label-address: &&label_name  (computed goto extension)
            parser.consume();
            if (parser.check(TokenKind::Identifier)) {
                // Emit as a null void* placeholder; computed goto not fully supported
                const char* lbl_name =
                    parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                parser.consume();
                Node* n = parser.make_node(NK_INT_LIT, ln);
                n->ival = 0;
                n->name = lbl_name;  // store label name for future use
                return n;
            }
            // Fall back: treat as bitwise AND of two address-of
            Node* operand = parse_unary(parser);
            Node* addr = parser.make_node(NK_ADDR, ln);
            addr->left = operand;
            Node* addr2 = parser.make_node(NK_ADDR, ln);
            addr2->left = addr;
            return addr2;
        }
        case TokenKind::Amp: {
            parser.consume();
            Node* operand = parse_unary(parser);
            Node* n = parser.make_node(NK_ADDR, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::Star: {
            parser.consume();
            Node* operand = parse_unary(parser);
            Node* n = parser.make_node(NK_DEREF, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwDelete: {
            // C++ delete / delete[] expression.
            parser.consume();  // consume 'delete'
            bool is_array = false;
            if (parser.check(TokenKind::LBracket) &&
                parser.core_input_state_.pos + 1 <
                    static_cast<int>(parser.core_input_state_.tokens.size()) &&
                parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                    TokenKind::RBracket) {
                parser.consume();  // '['
                parser.consume();  // ']'
                is_array = true;
            }
            Node* operand = parse_unary(parser);
            Node* n = parser.make_node(NK_DELETE_EXPR, ln);
            n->left = operand;
            n->ival = is_array ? 1 : 0;
            return n;
        }
        case TokenKind::KwNew: {
            // C++ new expression: new T, new T(args), new T[n]
            return parse_new_expr(parser, ln, false /*not global-qualified*/);
        }
        case TokenKind::KwSizeof: {
            parser.consume();
            if (parser.is_cpp_mode() && parser.check(TokenKind::Ellipsis)) {
                return parse_sizeof_pack_expr(parser, ln);
            }
            if (parser.check(TokenKind::LParen)) {
                // Could be sizeof(type) or sizeof(expr)
                // Disambiguate: if first token inside is a type keyword, it's a type
                {
                    Parser::TentativeParseGuardLite guard(parser);
                    parser.consume();  // consume (
                    if (parser.is_type_start() ||
                        starts_qualified_member_pointer_type_id(
                            parser, parser.core_input_state_.pos)) {
                        TypeSpec ts = parser.parse_type_name();
                        parser.expect(TokenKind::RParen);
                        guard.commit();
                        Node* n = parser.make_node(NK_SIZEOF_TYPE, ln);
                        n->type = ts;
                        return n;
                    }
                    // guard restores pos_ on scope exit (not committed)
                }
                {
                    // Parse as sizeof(expr) — pos_ already restored by guard above
                    parser.consume();  // consume (
                    Node* inner = parse_assign_expr(parser);
                    parser.expect(TokenKind::RParen);
                    Node* n = parser.make_node(NK_SIZEOF_EXPR, ln);
                    n->left = inner;
                    return n;
                }
            } else {
                Node* inner = parse_unary(parser);
                Node* n = parser.make_node(NK_SIZEOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwNoexcept: {
            parser.consume();
            parser.expect(TokenKind::LParen);
            Node* inner = parse_assign_expr(parser);
            parser.expect(TokenKind::RParen);
            return parser.make_unary("noexcept", inner, ln);
        }
        case TokenKind::KwAlignof:
        case TokenKind::KwGnuAlignof: {
            parser.consume();
            parser.expect(TokenKind::LParen);
            if (parser.is_type_start() ||
                starts_qualified_member_pointer_type_id(
                    parser, parser.core_input_state_.pos)) {
                TypeSpec ts = parser.parse_type_name();
                parser.expect(TokenKind::RParen);
                Node* n = parser.make_node(NK_ALIGNOF_TYPE, ln);
                n->type = ts;
                return n;
            } else {
                Node* inner = parse_assign_expr(parser);
                parser.expect(TokenKind::RParen);
                Node* n = parser.make_node(NK_ALIGNOF_EXPR, ln);
                n->left = inner;
                return n;
            }
        }
        case TokenKind::KwBuiltinVaArg: {
            // __builtin_va_arg(ap, type)
            parser.consume();
            parser.expect(TokenKind::LParen);
            Node* ap = parse_assign_expr(parser);
            parser.expect(TokenKind::Comma);
            TypeSpec ts = parser.parse_type_name();
            parser.expect(TokenKind::RParen);
            Node* n = parser.make_node(NK_VA_ARG, ln);
            n->left = ap;
            n->type = ts;
            return n;
        }
        case TokenKind::KwExtension: {
            // GNU __extension__: suppress extension warnings, semantically transparent.
            parser.consume();
            return parse_unary(parser);
        }
        case TokenKind::KwGccReal: {
            // GNU extension: __real__ expr
            parser.consume();
            Node* operand = parse_unary(parser);
            Node* n = parser.make_node(NK_REAL_PART, ln);
            n->left = operand;
            return n;
        }
        case TokenKind::KwGccImag: {
            // GNU extension: __imag__ expr
            parser.consume();
            Node* operand = parse_unary(parser);
            Node* n = parser.make_node(NK_IMAG_PART, ln);
            n->left = operand;
            return n;
        }
        default:
            return parse_postfix(parser, parse_primary(parser));
    }
}

Node* parse_new_expr(Parser& parser, int ln, bool global_qualified) {
    // Called after 'new' has been consumed.
    // Syntax: [::] new [( placement_args )] type [( ctor_args )] | type [ array_size ]
    parser.consume(); // consume 'new'

    Node* n = parser.make_node(NK_NEW_EXPR, ln);
    n->is_global_qualified = global_qualified;

    // Optional placement args: new (args...) T(...)
    // Placement args are comma-separated expressions in parentheses.
    // Stored in n->params / n->n_params to avoid conflicts with ctor args.
    if (parser.check(TokenKind::LParen)) {
        // Disambiguate: could be placement args or a parenthesized type.
        // Peek: if '(' followed by a type start and then ')', it's a cast-style type.
        // For placement new, arguments are expressions.
        // NOTE: pos_ saved/restored only to re-consume '(' after a peek; not a
        // full tentative parse, so TentativeParseGuard is not used here.
        int saved = parser.pos_;
        parser.consume(); // '('
        if (parser.is_type_start()) {
            // Still parse as expression — placement args like (void*)ptr are common.
            parser.pos_ = saved;
            parser.consume(); // '('
        }
        // Parse comma-separated placement args.
        std::vector<Node*> pargs;
        while (!parser.check(TokenKind::RParen)) {
            pargs.push_back(parse_assign_expr(parser));
            if (!parser.check(TokenKind::RParen)) parser.expect(TokenKind::Comma);
        }
        parser.expect(TokenKind::RParen);
        n->n_params = static_cast<int>(pargs.size());
        if (!pargs.empty()) {
            n->params = parser.arena_.alloc_array<Node*>(n->n_params);
            for (int i = 0; i < n->n_params; ++i) n->params[i] = pargs[i];
            n->left = pargs[0]; // backward compat: first placement arg
        }
    }

    // Parse the allocated type.
    TypeSpec ts = parser.parse_base_type();
    // Handle pointer levels in the type: new int*, new char* etc.
    while (parser.check(TokenKind::Star)) {
        parser.consume();
        ts.ptr_level++;
    }
    n->type = ts;

    // Check for array form: new T[n]
    if (parser.check(TokenKind::LBracket)) {
        parser.consume(); // '['
        Node* size_expr = parse_assign_expr(parser);
        parser.expect(TokenKind::RBracket);
        n->right = size_expr;
        n->ival = 1; // array new
    } else {
        n->ival = 0; // scalar new
    }

    // Optional constructor args: new T(args...)
    if (n->ival == 0 && parser.check(TokenKind::LParen)) {
        parser.consume(); // '('
        std::vector<Node*> args;
        while (!parser.check(TokenKind::RParen)) {
            args.push_back(parse_assign_expr(parser));
            if (!parser.check(TokenKind::RParen)) parser.expect(TokenKind::Comma);
        }
        parser.expect(TokenKind::RParen);
        n->n_children = static_cast<int>(args.size());
        if (!args.empty()) {
            n->children = parser.arena_.alloc_array<Node*>(n->n_children);
            for (int i = 0; i < n->n_children; ++i) n->children[i] = args[i];
        }
    }

    return n;
}

Node* parse_postfix(Parser& parser, Node* base) {
    while (true) {
        int ln = parser.cur().line;
        switch (parser.cur().kind) {
            case TokenKind::PlusPlus: {
                parser.consume();
                Node* n = parser.make_node(NK_POSTFIX, ln);
                n->op   = "++";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::MinusMinus: {
                parser.consume();
                Node* n = parser.make_node(NK_POSTFIX, ln);
                n->op   = "--";
                n->left = base;
                base = n;
                break;
            }
            case TokenKind::Dot: {
                parser.consume();
                std::string member_name;
                if (parser.is_cpp_mode() && parser.match(TokenKind::Tilde)) {
                    member_name = "~";
                    if (parser.check(TokenKind::Identifier)) {
                        member_name += parser.token_spelling(parser.cur());
                        parser.consume();
                    } else {
                        throw std::runtime_error("expected destructor type name after '~'");
                    }
                } else if (parser.check(TokenKind::Identifier)) {
                    member_name = parser.token_spelling(parser.cur());
                    parser.consume();
                } else if (!parser.try_parse_operator_function_id(member_name)) {
                    break;
                }
                Node* n = parser.make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = parser.arena_.strdup(member_name.c_str());
                n->is_arrow = false;
                base = n;
                break;
            }
            case TokenKind::Arrow: {
                parser.consume();
                std::string member_name;
                if (parser.is_cpp_mode() && parser.match(TokenKind::Tilde)) {
                    member_name = "~";
                    if (parser.check(TokenKind::Identifier)) {
                        member_name += parser.token_spelling(parser.cur());
                        parser.consume();
                    } else {
                        throw std::runtime_error("expected destructor type name after '~'");
                    }
                } else if (parser.check(TokenKind::Identifier)) {
                    member_name = parser.token_spelling(parser.cur());
                    parser.consume();
                } else if (!parser.try_parse_operator_function_id(member_name)) {
                    break;
                }
                Node* n = parser.make_node(NK_MEMBER, ln);
                n->left     = base;
                n->name     = parser.arena_.strdup(member_name.c_str());
                n->is_arrow = true;
                base = n;
                break;
            }
            case TokenKind::LBracket: {
                parser.consume();
                Node* idx = parse_expr(parser);
                parser.expect(TokenKind::RBracket);
                Node* n = parser.make_node(NK_INDEX, ln);
                n->left  = base;
                n->right = idx;
                base = n;
                break;
            }
            case TokenKind::LParen: {
                // Function call
                parser.consume();
                const BuiltinInfo* builtin = nullptr;
                if (base && base->kind == NK_VAR && base->name) {
                    builtin = builtin_by_name(base->name);
                }
                std::vector<Node*> args;
                while (!parser.at_end() && !parser.check(TokenKind::RParen)) {
                    Node* arg = parse_assign_expr(parser);
                    // C++ pack expansion in call args: func(args...)
                    if (parser.is_cpp_mode() && parser.check(TokenKind::Ellipsis)) {
                        parser.consume();
                        Node* pack = parser.make_node(NK_PACK_EXPANSION, ln);
                        pack->left = arg;
                        arg = pack;
                    }
                    args.push_back(arg);
                    if (!parser.match(TokenKind::Comma)) break;
                }
                parser.expect(TokenKind::RParen);
                const NodeKind call_kind =
                    (builtin && (builtin->node_kind == BuiltinNodeKind::BuiltinCall ||
                                 builtin->node_kind == BuiltinNodeKind::CanonicalCall))
                        ? NK_BUILTIN_CALL
                        : NK_CALL;
                Node* n = parser.make_node(call_kind, ln);
                n->left       = base;
                if (call_kind == NK_BUILTIN_CALL && builtin) n->builtin_id = builtin->id;
                n->n_children = (int)args.size();
                if (n->n_children > 0) {
                    n->children = parser.arena_.alloc_array<Node*>(n->n_children);
                    for (int i = 0; i < n->n_children; ++i) n->children[i] = args[i];
                }
                base = n;
                break;
            }
            case TokenKind::Ellipsis: {
                if (!parser.is_cpp_mode()) return base;
                // C++ pack expansion suffix: expr...
                parser.consume();
                Node* n = parser.make_node(NK_PACK_EXPANSION, ln);
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
            core_input_state_.pos + 1 <
                static_cast<int>(core_input_state_.tokens.size()) &&
            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                TokenKind::RBracket) {
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
            core_input_state_.pos + 1 <
                static_cast<int>(core_input_state_.tokens.size()) &&
            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                TokenKind::RBracket) {
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
    else if (match(TokenKind::Spaceship))      out_name = "operator_spaceship";
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

Node* parse_lambda_expr(Parser& parser, int ln) {
    parser.expect(TokenKind::LBracket);

    LambdaCaptureDefault capture_default = LCD_NONE;
    if (parser.match(TokenKind::RBracket)) {
        capture_default = LCD_NONE;
    } else if (parser.match(TokenKind::Amp)) {
        capture_default = LCD_BY_REFERENCE;
        parser.expect(TokenKind::RBracket);
    } else if (parser.match(TokenKind::Assign)) {
        capture_default = LCD_BY_COPY;
        parser.expect(TokenKind::RBracket);
    } else {
        throw std::runtime_error("unsupported lambda capture list");
    }

    bool has_parameter_list = false;
    if (parser.match(TokenKind::LParen)) {
        has_parameter_list = true;
        parser.expect(TokenKind::RParen);
    }

    if (!parser.check(TokenKind::LBrace)) {
        throw std::runtime_error("lambda body must be a compound statement");
    }

    Node* lambda = parser.make_node(NK_LAMBDA, ln);
    lambda->lambda_capture_default = capture_default;
    lambda->lambda_has_parameter_list = has_parameter_list;
    lambda->body = parse_block(parser);
    return lambda;
}


Node* parse_primary(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;

    auto parse_cpp_requires_expression = [&]() -> Node* {
        if (!(parser.is_cpp_mode() && parser.check(TokenKind::KwRequires))) {
            return nullptr;
        }

        parser.consume();  // requires
        if (parser.check(TokenKind::LParen))
            parser.skip_paren_group();
        if (!parser.check(TokenKind::LBrace))
            return nullptr;

        parser.skip_brace_group();
        return parser.make_int_lit(1, ln);
    };

    auto parse_operator_ref = [&]() -> Node* {
        if (!parser.is_cpp_mode()) return nullptr;
        Parser::TentativeParseGuard guard(parser);
        std::string qualified_name;
        std::string qualifier_owner;
        TextId qualifier_owner_text_id = kInvalidText;

        if (parser.match(TokenKind::ColonColon))
            qualified_name = "::";

        while (parser.check(TokenKind::Identifier) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
                parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::ColonColon) {
            if (qualifier_owner.empty())
                qualifier_owner_text_id = parser.cur().text_id;
            if (qualifier_owner.empty())
                qualifier_owner = parser.token_spelling(parser.cur());
            if (!qualified_name.empty() &&
                qualified_name.substr(qualified_name.size() - 2) != "::") {
                qualified_name += "::";
            }
            qualified_name += parser.token_spelling(parser.cur());
            parser.consume();
            parser.consume();
        }

        std::string op_name;
        if (!parser.try_parse_operator_function_id(op_name)) {
            return nullptr;
        }

        if (qualified_name == "::")
            qualified_name.clear();
        if (!qualifier_owner.empty() &&
            !parser.active_context_state_.current_struct_tag.empty() &&
            parser.check(TokenKind::LParen)) {
            const std::string current_tag(parser.current_struct_tag_text());
            const std::string resolved_owner =
                visible_type_head_name(
                    parser, qualifier_owner_text_id,
                    parser.parser_text(qualifier_owner_text_id, qualifier_owner));
            if (parser.has_visible_typedef_type(qualifier_owner_text_id) ||
                (!resolved_owner.empty() &&
                 (parser.has_visible_typedef_type(parser.find_parser_text_id(resolved_owner)) ||
                  resolved_owner == current_tag ||
                  qualifier_owner == current_tag))) {
                // Inside a method body, `BaseAlias::operator...(args)` names a
                // qualified member call on the current object rather than a
                // free function symbol. Collapse the owner spelling here so
                // later call lowering reuses the existing implicit-`this`
                // method path, including inherited-base lookup.
                qualified_name.clear();
            }
        }
        if (!qualified_name.empty() &&
            qualified_name.substr(qualified_name.size() - 2) != "::") {
            qualified_name += "::";
        }
        qualified_name += op_name;
        Node* ident = parser.make_var(parser.arena_.strdup(qualified_name.c_str()), ln);
        guard.commit();
        return parse_postfix(parser, ident);
    };

    auto has_balanced_template_id_ahead = [&]() -> bool {
        if (!parser.is_cpp_mode()) return true;

        int probe = parser.core_input_state_.pos;
        if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[probe].kind == TokenKind::ColonColon) {
            ++probe;
        }

        bool saw_template_args = false;
        while (probe < static_cast<int>(parser.core_input_state_.tokens.size())) {
            if (parser.core_input_state_.tokens[probe].kind == TokenKind::KwTemplate)
                ++probe;
            if (probe >= static_cast<int>(parser.core_input_state_.tokens.size()) ||
                parser.core_input_state_.tokens[probe].kind !=
                    TokenKind::Identifier) {
                return true;
            }
            ++probe;

            if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                parser.core_input_state_.tokens[probe].kind == TokenKind::Less) {
                saw_template_args = true;
                int depth = 0;
                while (probe < static_cast<int>(parser.core_input_state_.tokens.size())) {
                    TokenKind k = parser.core_input_state_.tokens[probe].kind;
                    if (k == TokenKind::Less) {
                        ++depth;
                        ++probe;
                        continue;
                    }
                    if (k == TokenKind::Greater) {
                        ++probe;
                        if (depth > 0 && --depth == 0) break;
                        continue;
                    }
                    if (k == TokenKind::GreaterGreater) {
                        ++probe;
                        if (depth > 0) {
                            depth -= 2;
                            if (depth <= 0) break;
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
                    ++probe;
                }
                if (depth > 0) return false;

                if (probe >= static_cast<int>(parser.core_input_state_.tokens.size()))
                    return false;
                TokenKind after_template = parser.core_input_state_.tokens[probe].kind;
                if (after_template == TokenKind::EqualEqual ||
                    after_template == TokenKind::BangEqual ||
                    after_template == TokenKind::LessEqual ||
                    after_template == TokenKind::GreaterEqual ||
                    after_template == TokenKind::Comma ||
                    after_template == TokenKind::Semi ||
                    after_template == TokenKind::Question ||
                    after_template == TokenKind::Colon ||
                    after_template == TokenKind::RParen ||
                    after_template == TokenKind::RBracket ||
                    after_template == TokenKind::RBrace ||
                    after_template == TokenKind::PipePipe ||
                    after_template == TokenKind::Pipe ||
                    after_template == TokenKind::Caret) {
                    return false;
                }
            }

            if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                parser.core_input_state_.tokens[probe].kind == TokenKind::ColonColon) {
                ++probe;
                continue;
            }
            break;
        }

        if (!saw_template_args) return true;
        if (probe >= static_cast<int>(parser.core_input_state_.tokens.size()))
            return false;

        switch (parser.core_input_state_.tokens[probe].kind) {
            case TokenKind::LParen:
            case TokenKind::Star:
            case TokenKind::Amp:
            case TokenKind::AmpAmp:
            case TokenKind::Identifier:
            case TokenKind::KwConst:
            case TokenKind::KwVolatile:
            case TokenKind::KwRestrict:
            case TokenKind::KwAtomic:
            case TokenKind::KwAttribute:
            case TokenKind::KwAlignas:
            case TokenKind::LBracket:
                return true;
            default:
                return false;
        }
    };

    auto parse_gcc_type_trait_builtin = [&]() -> Node* {
        if (!parser.check(TokenKind::Identifier) ||
            parser.core_input_state_.pos + 1 >=
                static_cast<int>(parser.core_input_state_.tokens.size()) ||
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind !=
                TokenKind::LParen) {
            return nullptr;
        }

        const std::string builtin_name = std::string(parser.token_spelling(parser.cur()));
        if (!is_builtin_type_trait_name(builtin_name) ||
            builtin_name == "__builtin_types_compatible_p") {
            return nullptr;
        }

        if (!is_parser_supported_builtin_type_trait_name(builtin_name)) {
            throw std::runtime_error("not implemented builtin type trait: " +
                                     builtin_name);
        }

        Parser::TentativeParseGuard guard(parser);
        parser.consume();  // builtin identifier
        parser.consume();  // (

        if (builtin_name == "__is_same") {
            try {
                TypeSpec lhs = parser.parse_type_name();
                parser.expect(TokenKind::Comma);
                TypeSpec rhs = parser.parse_type_name();
                parser.expect(TokenKind::RParen);
                const int result = parser.are_types_compatible(lhs, rhs) ? 1 : 0;
                guard.commit();
                return parser.make_int_lit(result, ln);
            } catch (...) {
                return nullptr;
            }
        }

        int parsed_args = 0;
        try {
            while (!parser.check(TokenKind::RParen)) {
                parser.parse_type_name();
                ++parsed_args;
                if (parser.check(TokenKind::Ellipsis)) parser.consume();
                if (!parser.match(TokenKind::Comma)) break;
            }
            parser.expect(TokenKind::RParen);
        } catch (...) {
            return nullptr;
        }

        if (parsed_args == 0) {
            return nullptr;
        }

        // These GCC/Clang type-trait builtins are parse-only placeholders for
        // now. The important behavior for this slice is to consume their
        // type-name argument lists instead of misparsing them as expressions.
        guard.commit();
        return parser.make_int_lit(1, ln);
    };

    // Integer literal
    if (parser.check(TokenKind::IntLit)) {
        const std::string spelled = std::string(parser.token_spelling(parser.cur()));
        long long v = parse_int_lexeme(spelled.c_str());
        bool imag   = lexeme_is_imaginary(spelled.c_str());
        const char* lex = parser.arena_.strdup(spelled);
        parser.consume();
        Node* n = parser.make_int_lit(v, ln);
        n->sval = lex;
        n->is_imaginary = imag;
        return n;
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::KwTrue)) {
        parser.consume();
        return parser.make_int_lit(1, ln);
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::KwFalse)) {
        parser.consume();
        return parser.make_int_lit(0, ln);
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::KwNullptr)) {
        parser.consume();
        return parser.make_int_lit(0, ln);
    }

    if (Node* requires_expr = parse_cpp_requires_expression())
        return requires_expr;

    if (parser.is_cpp_mode() && parser.check(TokenKind::LBracket)) {
        Parser::TentativeParseGuard guard(parser);
        try {
            Node* result = parse_lambda_expr(parser, ln);
            guard.commit();
            return result;
        } catch (const std::exception&) {
            // guard restores parser.pos_ on scope exit
        }
    }

    // Float literal
    if (parser.check(TokenKind::FloatLit)) {
        const std::string spelled = std::string(parser.token_spelling(parser.cur()));
        bool imag   = lexeme_is_imaginary(spelled.c_str());
        double v    = parse_float_lexeme(spelled.c_str());
        const char* raw = parser.arena_.strdup(spelled);
        parser.consume();
        Node* n = parser.make_float_lit(v, raw, ln);
        n->is_imaginary = imag;
        return n;
    }

    // String literal (possibly multiple adjacent)
    if (parser.check(TokenKind::StrLit)) {
        return parser.make_str_lit(consume_adjacent_string_literal(parser), ln);
    }

    // Char literal
    if (parser.check(TokenKind::CharLit)) {
        const std::string lex = std::string(parser.token_spelling(parser.cur()));
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
        const char* raw_lex = parser.arena_.strdup(lex);
        parser.consume();
        Node* n = parser.make_node(NK_CHAR_LIT, ln);
        n->ival = cv;
        n->sval = raw_lex;  // store raw lexeme to detect L prefix for wide char
        return n;
    }

    // C++ named casts: static_cast<T>(expr), reinterpret_cast<T>(expr), const_cast<T>(expr)
    if (parser.check(TokenKind::KwStaticCast) ||
        parser.check(TokenKind::KwReinterpretCast) ||
        parser.check(TokenKind::KwConstCast)) {
        parser.consume();  // consume cast keyword
        parser.expect(TokenKind::Less);       // <
        TypeSpec cast_ts = parser.parse_type_name();
        parser.expect(TokenKind::Greater);    // >
        parser.expect(TokenKind::LParen);     // (
        Node* operand = parse_assign_expr(parser);
        parser.expect(TokenKind::RParen);     // )
        Node* n = parser.make_node(NK_CAST, ln);
        n->type = cast_ts;
        n->left = operand;
        return n;
    }

    // Parenthesised expression or cast or compound literal
    if (parser.check(TokenKind::LParen)) {
        parser.consume();  // parser.consume(

        // Check for cast: (type-name)expr
        // NOTE: This site saves parser.tokens_ in addition to parser.pos_ (for template edge cases).
        // Parser::TentativeParseGuard does not snapshot parser.tokens_, so manual save/restore is kept here.
        if ((parser.is_type_start() ||
             starts_qualified_member_pointer_type_id(
                 parser, parser.core_input_state_.pos)) &&
            has_balanced_template_id_ahead()) {
            int save_pos = parser.pos_;
            std::vector<Token> saved_tokens = parser.tokens_;
            TypeSpec cast_ts = parser.parse_type_name();
            if (parser.check(TokenKind::RParen)) {
                // In C++ mode, check if this is really a cast or a parenthesized
                // expression.  A C-style cast (type)expr requires a valid operand
                // after ')'.  If the token after ')' is itself ')', ';', ',', or
                // another expression terminator, this cannot be a cast — it must
                // be a parenthesized expression like ((ns::value)).
                if (parser.is_cpp_mode()) {
                    int rp_pos = parser.pos_;  // points at ')'
                    int after_rp = rp_pos + 1;
                    if (after_rp <
                        static_cast<int>(parser.core_input_state_.tokens.size())) {
                        auto ak = parser.core_input_state_.tokens[after_rp].kind;
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
                            parser.pos_ = save_pos;
                            parser.tokens_ = saved_tokens;
                            goto not_a_cast;
                        }
                    }
                }
                parser.consume();  // consume )
                // Check for compound literal: (type){ ... }
                if (parser.check(TokenKind::LBrace)) {
                    Node* init_list = parse_init_list(parser);
                    Node* n = parser.make_node(NK_COMPOUND_LIT, ln);
                    n->type     = cast_ts;
                    n->left     = init_list;
                    return n;
                }
                // Regular cast
                Node* operand = parse_unary(parser);
                Node* n = parser.make_node(NK_CAST, ln);
                n->type = cast_ts;
                n->left = operand;
                // Phase C: propagate fn_ptr params from typedef onto cast node.
                if (cast_ts.is_fn_ptr) {
                    if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                        n->fn_ptr_params = info->params;
                        n->n_fn_ptr_params = info->n_params;
                        n->fn_ptr_variadic = info->variadic;
                    }
                }
                return n;
            } else {
                // Not a type name after all — restore and parse as expression
                parser.pos_ = save_pos;
                parser.tokens_ = saved_tokens;
            }
        }

        not_a_cast:
        // GCC statement expression: ({ ... })
        if (parser.check(TokenKind::LBrace)) {
            Node* block = parse_block(parser);
            parser.expect(TokenKind::RParen);
            Node* n = parser.make_node(NK_STMT_EXPR, ln);
            n->body = block;
            return n;
        }

        Node* inner = parse_expr(parser);
        parser.expect(TokenKind::RParen);
        return parse_postfix(parser, inner);
    }

    if (Node* op_ref = parse_operator_ref()) {
        return op_ref;
    }

    if (Node* type_trait = parse_gcc_type_trait_builtin()) {
        return type_trait;
    }

    // C++ ::new expression (global-qualified new)
    if (parser.is_cpp_mode() && parser.check(TokenKind::ColonColon) &&
        parser.core_input_state_.pos + 1 <
            static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
            TokenKind::KwNew) {
        parser.consume(); // consume '::'
        return parse_new_expr(parser, ln, true /*global_qualified*/);
    }

    // Global-qualified identifier (::name or ::ns::name)
    if (parser.is_cpp_mode() && parser.check(TokenKind::ColonColon) &&
        parser.core_input_state_.pos + 1 <
            static_cast<int>(parser.core_input_state_.tokens.size()) &&
        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
            TokenKind::Identifier) {
        Parser::QualifiedNameRef qn = parser.parse_qualified_name(true);
        const Parser::VisibleNameResult resolved_value = parser.resolve_qualified_value(qn);
        std::string qualified_name = parser.visible_name_spelling(resolved_value);
        if (qualified_name.empty()) {
            qualified_name = parser.qualified_name_text(
                qn, true /* include_global_prefix */);
        }
        const Node* static_member_owner =
            canonical_static_member_owner_record(parser, qn);
        const char* nm = parser.arena_.strdup(qualified_name.c_str());
        Node* ident = parser.make_node(NK_VAR, ln);
        ident->name = nm;
        parser.apply_qualified_name(ident, qn, nm);
        if (static_member_owner &&
            static_member_owner->namespace_context_id >= 0) {
            ident->namespace_context_id =
                static_member_owner->namespace_context_id;
        }
        parser.apply_using_value_alias_target(ident, resolved_value);
        if (parser.is_cpp_mode() && parser.check(TokenKind::Less)) {
            Parser::TentativeParseGuard guard(parser);
            std::vector<Parser::TemplateArgParseResult> parsed_args;
            if (parser.parse_template_argument_list(&parsed_args)) {
                auto is_valid_after_template = [&]() -> bool {
                    if (parser.check(TokenKind::LParen) || parser.check(TokenKind::ColonColon) ||
                        parser.check(TokenKind::LBrace)) return true;
                    if (parser.check(TokenKind::RParen) || parser.check(TokenKind::Semi) ||
                        parser.check(TokenKind::Comma) || parser.check(TokenKind::Ellipsis) ||
                        parser.check(TokenKind::RBracket) || parser.check_template_close() ||
                        parser.check(TokenKind::RBrace)) return true;
                    if (parser.check(TokenKind::Pipe) || parser.check(TokenKind::PipePipe) ||
                        parser.check(TokenKind::AmpAmp) || parser.check(TokenKind::Amp) ||
                        parser.check(TokenKind::Question) || parser.check(TokenKind::Colon) ||
                        parser.check(TokenKind::EqualEqual) || parser.check(TokenKind::BangEqual) ||
                        parser.check(TokenKind::LessEqual) || parser.check(TokenKind::GreaterEqual) ||
                        parser.check(TokenKind::Plus) || parser.check(TokenKind::Minus) ||
                        parser.check(TokenKind::Star) || parser.check(TokenKind::Slash) ||
                        parser.check(TokenKind::Percent) || parser.check(TokenKind::Assign) ||
                        parser.check(TokenKind::Dot) || parser.check(TokenKind::Arrow)) return true;
                    return false;
                };
                if (is_valid_after_template()) {
                    guard.commit();
                    ident->has_template_args = true;
                    ident->n_template_args = static_cast<int>(parsed_args.size());
                    ident->template_arg_types =
                        parser.arena_.alloc_array<TypeSpec>(ident->n_template_args);
                    ident->template_arg_is_value =
                        parser.arena_.alloc_array<bool>(ident->n_template_args);
                    ident->template_arg_values =
                        parser.arena_.alloc_array<long long>(ident->n_template_args);
                    ident->template_arg_nttp_names =
                        parser.arena_.alloc_array<const char*>(ident->n_template_args);
                    ident->template_arg_nttp_text_ids =
                        parser.arena_.alloc_array<TextId>(ident->n_template_args);
                    ident->template_arg_exprs =
                        parser.arena_.alloc_array<Node*>(ident->n_template_args);
                    for (int i = 0; i < ident->n_template_args; ++i) {
                        ident->template_arg_types[i] = parsed_args[i].type;
                        ident->template_arg_is_value[i] = parsed_args[i].is_value;
                        ident->template_arg_values[i] = parsed_args[i].value;
                        ident->template_arg_nttp_names[i] =
                            parsed_args[i].nttp_name;
                        ident->template_arg_nttp_text_ids[i] =
                            parsed_args[i].nttp_text_id;
                        ident->template_arg_exprs[i] = parsed_args[i].expr;
                    }
                    ident->is_concept_id =
                        parser.is_concept_name(
                            parser.parser_text_id_for_token(
                                qn.base_text_id, ident->name ? ident->name : ""));
                }
            }
        }
        return parse_postfix(parser, ident);
    }

    // Identifier / label address
    if (parser.check(TokenKind::Identifier)) {
        int ident_start = parser.pos_;
        Parser::QualifiedNameRef qn = parser.parse_qualified_name(false);
        const std::string_view qn_base_name =
            parser.parser_text(qn.base_text_id, qn.base_name);
        const Parser::VisibleNameResult direct_resolved_type =
            qn.qualifier_segments.empty()
                ? parser.resolve_visible_type(qn.base_text_id)
                : Parser::VisibleNameResult{};
        if (parser.is_cpp_mode() && qn.qualifier_segments.empty() &&
            parser.check(TokenKind::LParen)) {
            const TypeSpec* direct_typedef =
                parser.find_visible_typedef_type(qn.base_text_id);
            if (direct_typedef) {
                TypeSpec cast_ts = *direct_typedef;
                const std::string visible_typedef_name =
                    !direct_resolved_type
                        ? std::string(qn_base_name)
                        : parser.visible_name_spelling(direct_resolved_type);
                const TextId visible_typedef_text_id =
                    direct_resolved_type &&
                            direct_resolved_type.base_text_id != kInvalidText
                        ? direct_resolved_type.base_text_id
                        : qn.base_text_id;
                parser.set_last_resolved_typedef(visible_typedef_text_id,
                                                 visible_typedef_name);
                parser.consume();
                std::vector<Node*> args;
                if (!parser.check(TokenKind::RParen)) {
                    while (!parser.at_end() && !parser.check(TokenKind::RParen)) {
                        args.push_back(parse_assign_expr(parser));
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }
                parser.expect(TokenKind::RParen);
                const bool record_ctor_like =
                    parser.resolves_to_record_ctor_type(cast_ts);
                const bool multi_arg_typedef_cast =
                    cast_ts.base == TB_TYPEDEF && args.size() != 1;
                if (record_ctor_like || multi_arg_typedef_cast) {
                    Node* callee =
                        parser.make_var(cast_ts.tag ? cast_ts.tag
                                             : visible_typedef_name.c_str(),
                                 ln);
                    Node* call = parser.make_node(NK_CALL, ln);
                    call->left = callee;
                    call->n_children = static_cast<int>(args.size());
                    if (!args.empty()) {
                        call->children = parser.arena_.alloc_array<Node*>(call->n_children);
                        for (int i = 0; i < call->n_children; ++i) {
                            call->children[i] = args[i];
                        }
                    }
                    return parse_postfix(parser, call);
                }
                Node* operand = args.empty() ? parser.make_int_lit(0, ln) : args[0];
                Node* n = parser.make_node(NK_CAST, ln);
                n->type = cast_ts;
                n->left = operand;
                if (cast_ts.is_fn_ptr) {
                    if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                        n->fn_ptr_params = info->params;
                        n->n_fn_ptr_params = info->n_params;
                        n->fn_ptr_variadic = info->variadic;
                    }
                }
                return parse_postfix(parser, n);
            }
        }
        if (parser.is_cpp_mode() && (parser.check(TokenKind::Less) || parser.check(TokenKind::LParen))) {
            const int after_qualified_name_pos = parser.pos_;
            const QualifiedTypeProbe type_probe = probe_qualified_type(parser, qn);
            const std::string candidate_type_name =
                !type_probe.resolved_typedef_name.empty()
                    ? type_probe.resolved_typedef_name
                    : type_probe.spelled_name;
            if (type_probe.has_resolved_typedef) {
                const bool has_direct_member_typedef =
                    type_probe.record_member_typedef_type != nullptr;
                parser.pos_ = has_direct_member_typedef
                                  ? after_qualified_name_pos
                                  : ident_start;
                std::vector<Token> saved_tokens = parser.tokens_;
                const std::string saved_typedef_fallback =
                    parser.active_context_state_.last_resolved_typedef;
                const TextId saved_typedef_text_id =
                    parser.active_context_state_.last_resolved_typedef_text_id;
                auto restore_last_resolved_typedef = [&]() {
                    parser.clear_last_resolved_typedef();
                    if (saved_typedef_text_id != kInvalidText) {
                        parser.set_last_resolved_typedef(
                            saved_typedef_text_id, saved_typedef_fallback);
                    }
                };
                TypeSpec cast_ts =
                    has_direct_member_typedef
                        ? *type_probe.record_member_typedef_type
                        : parser.parse_base_type();
                if (has_direct_member_typedef) {
                    parser.set_last_resolved_typedef(qn.base_text_id,
                                                     candidate_type_name);
                }
                while (parser.check(TokenKind::Star)) {
                    parser.consume();
                    cast_ts.ptr_level++;
                }
                if (parser.check(TokenKind::AmpAmp)) {
                    parser.consume();
                    cast_ts.is_rvalue_ref = true;
                } else if (parser.check(TokenKind::Amp)) {
                    parser.consume();
                    cast_ts.is_lvalue_ref = true;
                }
                if (parser.check(TokenKind::LParen) &&
                    starts_parenthesized_member_pointer_declarator(parser, parser.pos_)) {
                    parser.pos_ = ident_start;
                    parser.tokens_ = saved_tokens;
                    restore_last_resolved_typedef();
                    qn = parser.parse_qualified_name(false);
                } else if (parser.check(TokenKind::LParen)) {
                    parser.consume();
                    std::vector<Node*> args;
                    if (!parser.check(TokenKind::RParen)) {
                        while (!parser.at_end() && !parser.check(TokenKind::RParen)) {
                            args.push_back(parse_assign_expr(parser));
                            if (!parser.match(TokenKind::Comma)) break;
                        }
                    }
                    parser.expect(TokenKind::RParen);
                    const bool record_ctor_like =
                        parser.resolves_to_record_ctor_type(cast_ts);
                    const bool multi_arg_typedef_cast =
                        cast_ts.base == TB_TYPEDEF && args.size() != 1;
                    if (record_ctor_like || multi_arg_typedef_cast) {
                        Node* callee = parser.make_var(cast_ts.tag ? cast_ts.tag : candidate_type_name.c_str(), ln);
                        Node* call = parser.make_node(NK_CALL, ln);
                        call->left = callee;
                        call->n_children = static_cast<int>(args.size());
                        if (!args.empty()) {
                            call->children = parser.arena_.alloc_array<Node*>(call->n_children);
                            for (int i = 0; i < call->n_children; ++i) call->children[i] = args[i];
                        }
                        return parse_postfix(parser, call);
                    }
                    Node* operand = args.empty() ? parser.make_int_lit(0, ln) : args[0];
                    Node* n = parser.make_node(NK_CAST, ln);
                    n->type = cast_ts;
                    n->left = operand;
                    if (cast_ts.is_fn_ptr) {
                        if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                            n->fn_ptr_params = info->params;
                            n->n_fn_ptr_params = info->n_params;
                            n->fn_ptr_variadic = info->variadic;
                        }
                    }
                    return parse_postfix(parser, n);
                }
                parser.pos_ = ident_start;
                parser.tokens_ = saved_tokens;
                restore_last_resolved_typedef();
                qn = parser.parse_qualified_name(false);
            }
        }
        const Parser::VisibleNameResult resolved_value = parser.resolve_qualified_value(qn);
        std::string qualified_name = parser.visible_name_spelling(resolved_value);
        if (qualified_name.empty()) {
            qualified_name = parser.qualified_name_text(
                qn, true /* include_global_prefix */);
        }
        const Node* static_member_owner =
            canonical_static_member_owner_record(parser, qn);
        const char* nm = parser.arena_.strdup(qualified_name.c_str());
        const BuiltinId builtin_id = builtin_id_from_name(nm);
        // __builtin_offsetof(type, member) — parse as constant expression when possible
        if (builtin_id == BuiltinId::Offsetof) {
            parser.expect(TokenKind::LParen);
            TypeSpec base_ts = parser.parse_type_name();
            parser.expect(TokenKind::Comma);
            if (!parser.check(TokenKind::Identifier)) {
                throw std::runtime_error("expected member name in __builtin_offsetof");
            }
            std::string field_path = std::string(parser.token_spelling(parser.cur()));
            parser.consume();
            while (parser.match(TokenKind::Dot)) {
                if (!parser.check(TokenKind::Identifier)) {
                    throw std::runtime_error("expected member name after '.' in __builtin_offsetof");
                }
                field_path += ".";
                field_path += parser.token_spelling(parser.cur());
                parser.consume();
            }
            parser.expect(TokenKind::RParen);
            Node* off = parser.make_node(NK_OFFSETOF, ln);
            off->type = base_ts;
            off->name = parser.arena_.strdup(field_path);
            long long cv = 0;
            if (parser.eval_const_int_with_parser_tables(off, &cv))
                return parser.make_int_lit(cv, ln);
            return off;
        }
        // __builtin_types_compatible_p(type1, type2) — evaluate at parse time
        if (builtin_id == BuiltinId::TypesCompatibleP) {
            parser.expect(TokenKind::LParen);
            TypeSpec ts1 = parser.parse_type_name();
            parser.expect(TokenKind::Comma);
            TypeSpec ts2 = parser.parse_type_name();
            parser.expect(TokenKind::RParen);
            int result = parser.are_types_compatible(ts1, ts2) ? 1 : 0;
            return parser.make_int_lit(result, ln);
        }
        Node* ident = parser.make_var(nm, ln);
        parser.apply_qualified_name(ident, qn, nm);
        if (static_member_owner &&
            static_member_owner->namespace_context_id >= 0) {
            ident->namespace_context_id =
                static_member_owner->namespace_context_id;
        }
        parser.apply_using_value_alias_target(ident, resolved_value);
        if (parser.is_cpp_mode() && parser.check(TokenKind::Less)) {
            Parser::TentativeParseGuard guard(parser);
            std::vector<Parser::TemplateArgParseResult> parsed_args;
            if (parser.parse_template_argument_list(&parsed_args)) {
                // Accept template instantiation when followed by a token that
                // would be valid after an expression (call, scope, brace-init,
                // close-paren, semicolon, comma, binary ops, etc.).
                // Reject only when the follow token looks like a new expression
                // start that would make more sense with < as comparison.
                auto is_valid_after_template = [&]() -> bool {
                    if (parser.check(TokenKind::LParen) || parser.check(TokenKind::ColonColon) ||
                        parser.check(TokenKind::LBrace)) return true;
                    // Expression terminators / continuations
                    if (parser.check(TokenKind::RParen) || parser.check(TokenKind::Semi) ||
                        parser.check(TokenKind::Comma) || parser.check(TokenKind::Ellipsis) ||
                        parser.check(TokenKind::RBracket) || parser.check_template_close() ||
                        parser.check(TokenKind::RBrace)) return true;
                    // Binary operators
                    if (parser.check(TokenKind::Pipe) || parser.check(TokenKind::PipePipe) ||
                        parser.check(TokenKind::AmpAmp) || parser.check(TokenKind::Amp) ||
                        parser.check(TokenKind::Question) || parser.check(TokenKind::Colon) ||
                        parser.check(TokenKind::EqualEqual) || parser.check(TokenKind::BangEqual) ||
                        parser.check(TokenKind::LessEqual) || parser.check(TokenKind::GreaterEqual) ||
                        parser.check(TokenKind::Plus) || parser.check(TokenKind::Minus) ||
                        parser.check(TokenKind::Star) || parser.check(TokenKind::Slash) ||
                        parser.check(TokenKind::Percent) || parser.check(TokenKind::Assign) ||
                        parser.check(TokenKind::Dot) || parser.check(TokenKind::Arrow)) return true;
                    return false;
                };
                if (is_valid_after_template()) {
                    // Accept template instantiation.
                    guard.commit();
                    ident->has_template_args = true;
                    ident->n_template_args = static_cast<int>(parsed_args.size());
                    ident->template_arg_types = parser.arena_.alloc_array<TypeSpec>(ident->n_template_args);
                    ident->template_arg_is_value = parser.arena_.alloc_array<bool>(ident->n_template_args);
                    ident->template_arg_values = parser.arena_.alloc_array<long long>(ident->n_template_args);
                    ident->template_arg_nttp_names = parser.arena_.alloc_array<const char*>(ident->n_template_args);
                    ident->template_arg_nttp_text_ids = parser.arena_.alloc_array<TextId>(ident->n_template_args);
                    ident->template_arg_exprs = parser.arena_.alloc_array<Node*>(ident->n_template_args);
                    for (int i = 0; i < ident->n_template_args; ++i) {
                        ident->template_arg_types[i] = parsed_args[i].type;
                        ident->template_arg_is_value[i] = parsed_args[i].is_value;
                        ident->template_arg_values[i] = parsed_args[i].value;
                        ident->template_arg_nttp_names[i] = parsed_args[i].nttp_name;
                        ident->template_arg_nttp_text_ids[i] = parsed_args[i].nttp_text_id;
                        ident->template_arg_exprs[i] = parsed_args[i].expr;
                    }
                    ident->is_concept_id =
                        parser.is_concept_name(
                            parser.parser_text_id_for_token(
                                qn.base_text_id, ident->name ? ident->name : ""));
                    // Template<A,B>::member — resolve as qualified name.
                    // Re-parse from ident_start to trigger template struct
                    // instantiation and get the mangled tag.
                    bool first_template_scope_member = true;
                    while (parser.check(TokenKind::ColonColon)) {
                        parser.consume(); // eat ::
                        std::string member;
                        TextId member_text_id = kInvalidText;
                        if (parser.check(TokenKind::Identifier)) {
                            const Token& member_tok = parser.cur();
                            member = parser.token_spelling(parser.cur());
                            member_text_id =
                                parser.parser_text_id_for_token(member_tok.text_id,
                                                                member);
                            parser.consume();
                        } else {
                            parser.try_parse_operator_function_id(member);
                            member_text_id =
                                parser.parser_text_id_for_token(kInvalidText,
                                                                member);
                        }
                        if (!member.empty()) {
                            Parser::QualifiedNameRef member_qn;
                            std::string owner_name =
                                ident->name ? ident->name : "";
                            int owner_namespace_context_id =
                                ident->namespace_context_id;
                            if (first_template_scope_member) {
                                TextId owner_text_id = kInvalidText;
                                // Try to instantiate via parse_base_type to get
                                // the structured owner for Template<Args>.
                                int after_member_pos = parser.pos_;
                                parser.pos_ = ident_start;
                                bool template_owner_requires_structured_metadata = false;
                                try {
                                    TypeSpec ts = parser.parse_base_type();
                                    owner_namespace_context_id =
                                        ts.namespace_context_id;
                                    template_owner_requires_structured_metadata =
                                        template_owner_requires_structured_metadata ||
                                        ts.tpl_struct_origin_key.base_text_id !=
                                            kInvalidText ||
                                        (ts.tpl_struct_args.data &&
                                         ts.tpl_struct_args.size > 0);
                                    if (ts.tpl_struct_origin &&
                                        ts.tpl_struct_origin[0]) {
                                        owner_name = ts.tpl_struct_origin;
                                        if (ts.record_def &&
                                            ts.record_def->unqualified_text_id !=
                                                kInvalidText) {
                                            owner_text_id =
                                                ts.record_def->unqualified_text_id;
                                            if (ts.record_def->namespace_context_id >= 0) {
                                                owner_namespace_context_id =
                                                    ts.record_def
                                                        ->namespace_context_id;
                                            }
                                        } else if (ts.tag_text_id !=
                                                   kInvalidText) {
                                            owner_text_id = ts.tag_text_id;
                                        }
                                    } else if (ts.record_def &&
                                               ts.record_def->unqualified_text_id !=
                                                   kInvalidText) {
                                        owner_name =
                                            ts.record_def->name &&
                                                    ts.record_def->name[0]
                                                ? ts.record_def->name
                                                : owner_name;
                                        owner_text_id =
                                            ts.record_def->unqualified_text_id;
                                        if (ts.record_def->namespace_context_id >= 0) {
                                            owner_namespace_context_id =
                                                ts.record_def
                                                    ->namespace_context_id;
                                        }
                                    } else if (ts.tag_text_id != kInvalidText) {
                                        if (ts.tag && ts.tag[0])
                                            owner_name = ts.tag;
                                        owner_text_id = ts.tag_text_id;
                                    } else if (ts.tag && ts.tag[0]) {
                                        owner_name = ts.tag;
                                    }
                                } catch (...) {
                                    // fallback: use current expression owner
                                }
                                parser.pos_ = after_member_pos;
                                if (owner_name.rfind("::", 0) == 0) {
                                    member_qn.is_global_qualified = true;
                                    owner_name.erase(0, 2);
                                }
                                if (owner_text_id != kInvalidText) {
                                    member_qn.qualifier_segments.push_back(
                                        owner_name);
                                    member_qn.qualifier_text_ids.push_back(
                                        owner_text_id);
                                } else if (!template_owner_requires_structured_metadata) {
                                    size_t segment_start = 0;
                                    while (segment_start <= owner_name.size()) {
                                        const size_t segment_end =
                                            owner_name.find("::",
                                                            segment_start);
                                        const std::string segment =
                                            segment_end == std::string::npos
                                                ? owner_name.substr(
                                                      segment_start)
                                                : owner_name.substr(
                                                      segment_start,
                                                      segment_end -
                                                          segment_start);
                                        if (!segment.empty()) {
                                            member_qn.qualifier_segments
                                                .push_back(segment);
                                            member_qn.qualifier_text_ids
                                                .push_back(
                                                    parser
                                                        .parser_text_id_for_token(
                                                            kInvalidText,
                                                            segment));
                                        }
                                        if (segment_end == std::string::npos)
                                            break;
                                        segment_start = segment_end + 2;
                                    }
                                }
                            } else {
                                member_qn.is_global_qualified =
                                    ident->is_global_qualified;
                                for (int qi = 0;
                                     qi < ident->n_qualifier_segments; ++qi) {
                                    const char* segment =
                                        ident->qualifier_segments
                                            ? ident->qualifier_segments[qi]
                                            : nullptr;
                                    if (!segment || !segment[0]) continue;
                                    member_qn.qualifier_segments.emplace_back(
                                        segment);
                                    member_qn.qualifier_text_ids.push_back(
                                        ident->qualifier_text_ids
                                            ? ident->qualifier_text_ids[qi]
                                            : parser.parser_text_id_for_token(
                                                  kInvalidText, segment));
                                }
                                const char* owner_segment =
                                    ident->unqualified_name
                                        ? ident->unqualified_name
                                        : member_qn.qualifier_segments.empty()
                                              ? owner_name.c_str()
                                              : nullptr;
                                if (owner_segment && owner_segment[0]) {
                                    member_qn.qualifier_segments.emplace_back(
                                        owner_segment);
                                    member_qn.qualifier_text_ids.push_back(
                                        ident->unqualified_text_id != kInvalidText
                                            ? ident->unqualified_text_id
                                            : parser.parser_text_id_for_token(
                                                  kInvalidText,
                                                  owner_segment));
                                }
                            }
                            const std::string member_name =
                                owner_name + "::" + member;
                            ident->name =
                                parser.arena_.strdup(member_name.c_str());
                            member_qn.base_name = member;
                            member_qn.base_text_id = member_text_id;
                            parser.apply_qualified_name(ident, member_qn,
                                                        ident->name);
                            if (owner_namespace_context_id >= 0) {
                                ident->namespace_context_id =
                                    owner_namespace_context_id;
                            }
                        }
                        first_template_scope_member = false;
                    }
                    // Brace-init after template: Type<Args>{} or Type<Args>{a, b}
                    // Preserve empty-brace temporaries as a zero-arg call shape.
                    if (parser.check(TokenKind::LBrace)) {
                        const bool empty_braces =
                            parser.pos_ + 1 < static_cast<int>(parser.tokens_.size()) &&
                            parser.tokens_[parser.pos_ + 1].kind == TokenKind::RBrace;
                        parser.consume(); // {
                        int depth = 1;
                        while (!parser.at_end() && depth > 0) {
                            if (parser.check(TokenKind::LBrace)) ++depth;
                            else if (parser.check(TokenKind::RBrace)) --depth;
                            if (depth > 0) parser.consume();
                        }
                        if (parser.check(TokenKind::RBrace)) parser.consume(); // }
                        if (empty_braces) {
                            Node* ctor = parser.make_node(NK_CALL, ln);
                            ctor->left = ident;
                            ctor->n_children = 0;
                            ident = ctor;
                        }
                    }
                }
                // else: guard restores parser.pos_ on scope exit (not valid after template)
            }
            // else: guard restores parser.pos_ on scope exit (template arg list parse failed)
        }
        if (parser.is_cpp_mode() && parser.check(TokenKind::LBrace) &&
            parser.pos_ + 1 < static_cast<int>(parser.tokens_.size()) &&
            parser.tokens_[parser.pos_ + 1].kind == TokenKind::RBrace) {
            parser.consume();
            parser.consume();
            Node* ctor = parser.make_node(NK_CALL, ln);
            ctor->left = ident;
            ctor->n_children = 0;
            ident = ctor;
        }
        return parse_postfix(parser, ident);
    }

    // _Generic selection
    if (parser.check(TokenKind::KwGeneric)) {
        parser.consume();
        parser.expect(TokenKind::LParen);
        Node* ctrl = parse_assign_expr(parser);
        std::vector<Node*> assocs;
        while (!parser.check(TokenKind::RParen) && !parser.at_end()) {
            if (!parser.match(TokenKind::Comma)) break;
            if (parser.check(TokenKind::RParen)) break;
            Node* assoc = parser.make_node(NK_CAST, parser.cur().line);
            if (parser.check(TokenKind::KwDefault)) {
                parser.consume();
                assoc->ival = 1;  // marks as default association
            } else {
                assoc->type = parser.parse_type_name();
            }
            parser.expect(TokenKind::Colon);
            assoc->left = parse_assign_expr(parser);
            assocs.push_back(assoc);
        }
        parser.expect(TokenKind::RParen);
        Node* n = parser.make_node(NK_GENERIC, ln);
        n->left = ctrl;
        n->children = (Node**)parser.arena_.alloc(assocs.size() * sizeof(Node*));
        n->n_children = (int)assocs.size();
        for (int i = 0; i < (int)assocs.size(); i++) n->children[i] = assocs[i];
        return n;
    }

    // Initializer list in some contexts — just parse it
    if (parser.check(TokenKind::LBrace)) {
        return parse_init_list(parser);
    }

    // C++ functional cast: T(expr)
    if (parser.is_cpp_mode() && parser.is_type_start() && has_balanced_template_id_ahead()) {
        Parser::TentativeParseGuard guard(parser);
        TypeSpec cast_ts = parser.parse_base_type();
        while (parser.check(TokenKind::Star)) {
            parser.consume();
            cast_ts.ptr_level++;
        }
        if (parser.check(TokenKind::AmpAmp)) {
            parser.consume();
            cast_ts.is_rvalue_ref = true;
        } else if (parser.check(TokenKind::Amp)) {
            parser.consume();
            cast_ts.is_lvalue_ref = true;
        }
        if (parser.check(TokenKind::LParen)) {
            parser.consume();  // (
            std::vector<Node*> args;
            if (!parser.check(TokenKind::RParen)) {
                while (!parser.at_end() && !parser.check(TokenKind::RParen)) {
                    Node* arg = nullptr;
                    if (parser.is_cpp_mode() && parser.check(TokenKind::Identifier) &&
                        parser.pos_ + 2 < static_cast<int>(parser.tokens_.size()) &&
                        parser.tokens_[parser.pos_ + 1].kind == TokenKind::ColonColon &&
                        parser.tokens_[parser.pos_ + 2].kind == TokenKind::Identifier &&
                        !(parser.pos_ + 3 < static_cast<int>(parser.tokens_.size()) &&
                          parser.tokens_[parser.pos_ + 3].kind == TokenKind::Less)) {
                        Parser::QualifiedNameRef operand_name = parser.parse_qualified_name(false);
                        const Parser::VisibleNameResult resolved_operand =
                            parser.resolve_qualified_value(operand_name);
                        std::string qualified_name =
                            parser.visible_name_spelling(resolved_operand);
                        if (qualified_name.empty()) {
                            qualified_name = parser.qualified_name_text(
                                operand_name, true /* include_global_prefix */);
                        }
                        const char* nm = parser.arena_.strdup(qualified_name.c_str());
                        arg = parser.make_var(nm, ln);
                        parser.apply_qualified_name(arg, operand_name, nm);
                        parser.apply_using_value_alias_target(
                            arg, resolved_operand);
                    } else {
                        arg = parse_assign_expr(parser);
                    }
                    if (arg) args.push_back(arg);
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);
            const bool record_ctor_like =
                parser.resolves_to_record_ctor_type(cast_ts);
            const bool multi_arg_typedef_cast =
                cast_ts.base == TB_TYPEDEF && args.size() != 1;
            if (record_ctor_like || multi_arg_typedef_cast) {
                Node* callee = parser.make_var(cast_ts.tag ? cast_ts.tag : "<ctor>", ln);
                Node* call = parser.make_node(NK_CALL, ln);
                call->left = callee;
                call->n_children = static_cast<int>(args.size());
                if (!args.empty()) {
                    call->children = parser.arena_.alloc_array<Node*>(call->n_children);
                    for (int i = 0; i < call->n_children; ++i) call->children[i] = args[i];
                }
                guard.commit();
                return parse_postfix(parser, call);
            }
            Node* operand = args.empty() ? parser.make_int_lit(0, ln) : args[0];
            Node* n = parser.make_node(NK_CAST, ln);
            n->type = cast_ts;
            n->left = operand;
            if (cast_ts.is_fn_ptr) {
                if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                    n->fn_ptr_params = info->params;
                    n->n_fn_ptr_params = info->n_params;
                    n->fn_ptr_variadic = info->variadic;
                }
            }
            guard.commit();
            return parse_postfix(parser, n);
        }
        // guard restores parser.pos_ and last_resolved_typedef_ on scope exit
    }

    // C++ lambda expression: [captures](params) -> ret { body }
    // Skip the entire lambda and return a placeholder (0) so template
    // function bodies and in-class initializers can parse through them.
    if (parser.is_cpp_mode() && parser.check(TokenKind::LBracket)) {
        // Skip capture list [...]
        parser.consume(); // [
        int depth = 1;
        while (!parser.at_end() && depth > 0) {
            if (parser.check(TokenKind::LBracket)) ++depth;
            else if (parser.check(TokenKind::RBracket)) --depth;
            if (depth > 0) parser.consume();
        }
        if (parser.check(TokenKind::RBracket)) parser.consume(); // ]
        // Skip optional (params)
        if (parser.check(TokenKind::LParen)) parser.skip_paren_group();
        // Skip optional mutable/constexpr/consteval
        while (parser.check(TokenKind::KwMutable) || parser.check(TokenKind::KwConstexpr) ||
               parser.check(TokenKind::KwConsteval) ||
               (parser.check(TokenKind::Identifier) &&
                (parser.token_spelling(parser.cur()) == "mutable" || parser.token_spelling(parser.cur()) == "constexpr" ||
                 parser.token_spelling(parser.cur()) == "consteval"))) parser.consume();
        // Skip optional noexcept(...)
        parser.skip_exception_spec();
        // Skip optional -> return_type
        if (parser.check(TokenKind::Arrow)) {
            parser.consume();
            parser.parse_type_name();
        }
        // Skip body { ... }
        if (parser.check(TokenKind::LBrace)) parser.skip_brace_group();
        return parser.make_int_lit(0, ln);
    }

    // Hard error for unrecognized primaries (was permissive fallback to 0).
    std::string tok = parser.at_end() ? "<eof>" : std::string(parser.token_spelling(parser.cur()));
    parser.note_parse_failure_message(("unexpected token in expression: " + tok).c_str());
    throw std::runtime_error("unexpected token in expression: " + tok);
}

// ── initializer parsing ───────────────────────────────────────────────────────

Node* parse_initializer(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (parser.check(TokenKind::LBrace)) {
        return parse_init_list(parser);
    }
    return parse_assign_expr(parser);
}

Node* parse_init_list(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    parser.expect(TokenKind::LBrace);
    std::vector<Node*> items;
    while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
        Node* item = parser.make_node(NK_INIT_ITEM, parser.cur().line);

        // Designator?
        // Old GCC-style: `fieldname: value` (same as `.fieldname = value`)
        if (parser.check(TokenKind::Identifier) &&
            parser.peek(1).kind == TokenKind::Colon) {
            item->desig_field =
                parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
            item->is_designated  = true;
            item->is_index_desig = false;
            parser.consume();  // identifier
            parser.consume();  // ':'
            item->left = parse_initializer(parser);
            items.push_back(item);
            if (!parser.match(TokenKind::Comma)) break;
            if (parser.check(TokenKind::RBrace)) break;
            continue;
        }
        if (parser.check(TokenKind::Dot)) {
            parser.consume();
            if (parser.check(TokenKind::Identifier)) {
                item->desig_field =
                    parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                item->is_designated  = true;
                item->is_index_desig = false;
                parser.consume();
            }
            // Multi-level designator: .a.b = val → synthesize nested { .b = val }
            if (parser.check(TokenKind::Dot) || parser.check(TokenKind::LBracket)) {
                // Recursively build nested init list for the remaining chain
                std::function<Node*()> build_nested = [&]() -> Node* {
                    int ln2 = parser.cur().line;
                    Node* inner = parser.make_node(NK_INIT_ITEM, ln2);
                    if (parser.check(TokenKind::Dot)) {
                        parser.consume();
                        if (parser.check(TokenKind::Identifier)) {
                            inner->desig_field =
                                parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                            inner->is_designated  = true;
                            inner->is_index_desig = false;
                            parser.consume();
                        }
                    } else if (parser.check(TokenKind::LBracket)) {
                        parser.consume();
                        Node* ie = parse_assign_expr(parser);
                        long long iv = (ie && ie->kind == NK_INT_LIT) ? ie->ival : 0;
                        parser.expect(TokenKind::RBracket);
                        inner->is_designated  = true;
                        inner->is_index_desig = true;
                        inner->desig_val      = iv;
                    }
                    if (parser.check(TokenKind::Dot) || parser.check(TokenKind::LBracket)) {
                        inner->left = build_nested();
                    } else {
                        parser.match(TokenKind::Assign);
                        inner->left = parse_initializer(parser);
                    }
                    Node* lst2 = parser.make_node(NK_INIT_LIST, ln2);
                    Node** ch = (Node**)parser.arena_.alloc(sizeof(Node*));
                    ch[0] = inner;
                    lst2->children = ch;
                    lst2->n_children = 1;
                    return lst2;
                };
                item->left = build_nested();
                items.push_back(item);
                if (!parser.match(TokenKind::Comma)) break;
                if (parser.check(TokenKind::RBrace)) break;
                continue;
            }
            parser.match(TokenKind::Assign);
        } else if (parser.check(TokenKind::LBracket)) {
            parser.consume();
            Node* idx_expr = parse_assign_expr(parser);
            long long idx_lo = 0;
            if (idx_expr && idx_expr->kind == NK_INT_LIT) idx_lo = idx_expr->ival;
            // GCC range: [lo ... hi]
            long long idx_hi = idx_lo;
            if (parser.check(TokenKind::Ellipsis)) {
                parser.consume();
                Node* hi_expr = parse_assign_expr(parser);
                if (hi_expr && hi_expr->kind == NK_INT_LIT) idx_hi = hi_expr->ival;
            }
            parser.expect(TokenKind::RBracket);
            parser.match(TokenKind::Assign);
            item->is_designated  = true;
            item->is_index_desig = true;
            item->desig_val      = idx_lo;
            // For ranges, store hi in right->ival
            if (idx_hi != idx_lo) {
                Node* hi_node = parser.make_int_lit(idx_hi, parser.cur().line);
                item->right = hi_node;
            }
        }

        item->left = parse_initializer(parser);
        items.push_back(item);

        if (!parser.match(TokenKind::Comma)) break;
        if (parser.check(TokenKind::RBrace)) break;
    }
    parser.expect(TokenKind::RBrace);

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
            Node* clone = parser.make_node(NK_INIT_ITEM, item->line);
            clone->is_designated = true;
            clone->is_index_desig = true;
            clone->desig_val = idx;
            clone->left = item->left;
            expanded_items.push_back(clone);
        }
    }

    Node* list = parser.make_node(NK_INIT_LIST, ln);
    list->n_children = (int)expanded_items.size();
    if (list->n_children > 0) {
        list->children = parser.arena_.alloc_array<Node*>(list->n_children);
        for (int i = 0; i < list->n_children; ++i) list->children[i] = expanded_items[i];
    }
    return list;
}

// ── statement parsing ─────────────────────────────────────────────────────────


}  // namespace c4c

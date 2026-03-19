#include "parser.hpp"

#include <cstdio>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace c4c {
Parser::Parser(std::vector<Token> tokens, Arena& arena, SourceProfile source_profile)
    : tokens_(std::move(tokens)), pos_(0), arena_(arena), source_profile_(source_profile),
      anon_counter_(0), had_error_(false), parsing_top_level_context_(false),
      last_enum_def_(nullptr) {
    // Pre-seed well-known typedef names from standard / system headers
    // so the parser can disambiguate type-name vs identifier.
    static const char* seed[] = {
        "size_t", "ssize_t", "ptrdiff_t", "intptr_t", "uintptr_t",
        "int8_t", "int16_t", "int32_t", "int64_t",
        "uint8_t", "uint16_t", "uint32_t", "uint64_t",
        "int_least8_t", "int_least16_t", "int_least32_t", "int_least64_t",
        "uint_least8_t","uint_least16_t","uint_least32_t","uint_least64_t",
        "int_fast8_t", "int_fast16_t", "int_fast32_t", "int_fast64_t",
        "uint_fast8_t", "uint_fast16_t", "uint_fast32_t", "uint_fast64_t",
        "intmax_t", "uintmax_t",
        "off_t", "pid_t", "uid_t", "gid_t",
        "time_t", "clock_t", "suseconds_t",
        "FILE", "DIR", "fpos_t",
        "va_list", "__va_list", "__va_list_tag",
        "wchar_t", "wint_t", "wctype_t",
        "bool",
        "NULL",
        "__builtin_va_list", "__gnuc_va_list",
        "jmp_buf", "sigjmp_buf",
        "pthread_t", "pthread_mutex_t", "pthread_cond_t",
        "socklen_t", "in_addr_t", "in_port_t",
        "mode_t", "ino_t", "dev_t", "nlink_t", "blksize_t", "blkcnt_t",
        "div_t", "ldiv_t", "lldiv_t",
        "__int128_t", "__uint128_t",
        "__Float32x4_t", "__Float64x2_t",
        "__SVFloat32_t", "__SVFloat64_t", "__SVBool_t",
        "OSStatus", "OSErr",
        nullptr
    };
    for (int i = 0; seed[i]; ++i) typedefs_.insert(seed[i]);

    // Seed typedef_types_ for well-known types so they resolve to correct LLVM types
    // instead of the TB_TYPEDEF fallback (which emits i32).
    {
        TypeSpec va_ts{};
        va_ts.array_size = -1;
        va_ts.array_rank = 0;
        va_ts.is_ptr_to_array = false;
        va_ts.base = TB_VA_LIST;
        typedef_types_["va_list"]          = va_ts;
        typedef_types_["__va_list"]        = va_ts;
        typedef_types_["__builtin_va_list"]= va_ts;
        typedef_types_["__gnuc_va_list"]   = va_ts;

        // size_t / uintptr_t etc. → u64 on 64-bit platforms
        TypeSpec u64_ts{};
        u64_ts.array_size = -1;
        u64_ts.array_rank = 0;
        u64_ts.is_ptr_to_array = false;
        u64_ts.base = TB_ULONGLONG;  // 64-bit unsigned
        typedef_types_["size_t"]    = u64_ts;
        typedef_types_["uintptr_t"] = u64_ts;
        typedef_types_["uintmax_t"] = u64_ts;
        typedef_types_["uint64_t"]  = u64_ts;
        typedef_types_["uint_least64_t"] = u64_ts;
        typedef_types_["uint_fast64_t"]  = u64_ts;

        TypeSpec i64_ts{};
        i64_ts.array_size = -1;
        i64_ts.array_rank = 0;
        i64_ts.is_ptr_to_array = false;
        i64_ts.base = TB_LONGLONG;
        typedef_types_["ssize_t"]   = i64_ts;
        typedef_types_["ptrdiff_t"] = i64_ts;
        typedef_types_["intptr_t"]  = i64_ts;
        typedef_types_["intmax_t"]  = i64_ts;
        typedef_types_["int64_t"]   = i64_ts;
        typedef_types_["int_least64_t"] = i64_ts;
        typedef_types_["int_fast64_t"]  = i64_ts;
        typedef_types_["off_t"]     = i64_ts;

        TypeSpec u32_ts{};
        u32_ts.array_size = -1;
        u32_ts.array_rank = 0;
        u32_ts.is_ptr_to_array = false;
        u32_ts.base = TB_UINT;
        typedef_types_["uint32_t"]  = u32_ts;
        typedef_types_["uint_least32_t"] = u32_ts;
        typedef_types_["uint_fast32_t"]  = u32_ts;

        TypeSpec i32_ts{};
        i32_ts.array_size = -1;
        i32_ts.array_rank = 0;
        i32_ts.is_ptr_to_array = false;
        i32_ts.base = TB_INT;
        typedef_types_["int32_t"]   = i32_ts;
        typedef_types_["int_least32_t"] = i32_ts;
        typedef_types_["int_fast32_t"]  = i32_ts;

        TypeSpec u16_ts{};
        u16_ts.array_size = -1;
        u16_ts.array_rank = 0;
        u16_ts.is_ptr_to_array = false;
        u16_ts.base = TB_USHORT;
        typedef_types_["uint16_t"]  = u16_ts;
        typedef_types_["uint_least16_t"] = u16_ts;
        typedef_types_["uint_fast16_t"]  = u16_ts;

        TypeSpec i16_ts{};
        i16_ts.array_size = -1;
        i16_ts.array_rank = 0;
        i16_ts.is_ptr_to_array = false;
        i16_ts.base = TB_SHORT;
        typedef_types_["int16_t"]   = i16_ts;
        typedef_types_["int_least16_t"] = i16_ts;
        typedef_types_["int_fast16_t"]  = i16_ts;

        TypeSpec u8_ts{};
        u8_ts.array_size = -1;
        u8_ts.array_rank = 0;
        u8_ts.is_ptr_to_array = false;
        u8_ts.base = TB_UCHAR;
        typedef_types_["uint8_t"]   = u8_ts;
        typedef_types_["uint_least8_t"] = u8_ts;
        typedef_types_["uint_fast8_t"]  = u8_ts;

        TypeSpec i8_ts{};
        i8_ts.array_size = -1;
        i8_ts.array_rank = 0;
        i8_ts.is_ptr_to_array = false;
        i8_ts.base = TB_SCHAR;
        typedef_types_["int8_t"]    = i8_ts;
        typedef_types_["int_least8_t"] = i8_ts;
        typedef_types_["int_fast8_t"]  = i8_ts;

        // wchar_t on macOS/ARM64 is i32
        TypeSpec wchar_ts{};
        wchar_ts.array_size = -1;
        wchar_ts.array_rank = 0;
        wchar_ts.is_ptr_to_array = false;
        wchar_ts.base = TB_INT;
        typedef_types_["wchar_t"]  = wchar_ts;
        typedef_types_["wint_t"]   = wchar_ts;
    }
}

// ── pragma helpers ────────────────────────────────────────────────────────────

void Parser::handle_pragma_pack(const std::string& args) {
    // #pragma pack(N)        — set pack alignment to N
    // #pragma pack()         — reset to default (0)
    // #pragma pack(push)     — push current alignment
    // #pragma pack(push,N)   — push current alignment, set to N
    // #pragma pack(pop)      — pop previous alignment
    // #pragma pack(pop,N)    — pop and set to N
    // The lexeme has whitespace stripped and contains just the args, e.g. "1", "push,2", "pop", ""

    if (args.empty()) {
        pack_alignment_ = 0;
        return;
    }

    if (args.substr(0, 4) == "push") {
        pack_stack_.push_back(pack_alignment_);
        if (args.size() > 4 && args[4] == ',') {
            pack_alignment_ = std::stoi(args.substr(5));
        }
    } else if (args.substr(0, 3) == "pop") {
        if (!pack_stack_.empty()) {
            pack_alignment_ = pack_stack_.back();
            pack_stack_.pop_back();
        } else {
            pack_alignment_ = 0;
        }
        if (args.size() > 3 && args[3] == ',') {
            pack_alignment_ = std::stoi(args.substr(4));
        }
    } else {
        // Simple numeric value
        pack_alignment_ = std::stoi(args);
    }
}

void Parser::handle_pragma_gcc_visibility(const std::string& args) {
    // Lexeme format: "push,<visibility>" or "pop"
    if (args == "pop") {
        if (!visibility_stack_.empty()) {
            visibility_ = visibility_stack_.back();
            visibility_stack_.pop_back();
        } else {
            visibility_ = 0;  // default
        }
    } else if (args.substr(0, 5) == "push,") {
        visibility_stack_.push_back(visibility_);
        const std::string vis = args.substr(5);
        if (vis == "hidden") visibility_ = 1;
        else if (vis == "protected") visibility_ = 2;
        else visibility_ = 0;  // "default" or unknown → default
    }
}

// ── token cursor helpers ──────────────────────────────────────────────────────

const Token& Parser::cur() const {
    return tokens_[pos_];
}

const Token& Parser::peek(int offset) const {
    int idx = pos_ + offset;
    if (idx < 0 || idx >= (int)tokens_.size()) {
        static Token eof{TokenKind::EndOfFile, "", 0, 0};
        return eof;
    }
    return tokens_[idx];
}

const Token& Parser::consume() {
    const Token& t = tokens_[pos_];
    if (pos_ + 1 < (int)tokens_.size()) ++pos_;
    return t;
}

bool Parser::at_end() const {
    return tokens_[pos_].kind == TokenKind::EndOfFile;
}

bool Parser::check(TokenKind k) const {
    return tokens_[pos_].kind == k;
}

bool Parser::check2(TokenKind k) const {
    int idx = pos_ + 1;
    if (idx >= (int)tokens_.size()) return false;
    return tokens_[idx].kind == k;
}

bool Parser::match(TokenKind k) {
    if (check(k)) { consume(); return true; }
    return false;
}

void Parser::expect(TokenKind k) {
    if (!check(k)) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(k) << " but got '"
            << cur().lexeme << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
    consume();
}

bool Parser::check_template_close() const {
    return check(TokenKind::Greater) || check(TokenKind::GreaterGreater);
}

bool Parser::match_template_close() {
    if (check(TokenKind::Greater)) { consume(); return true; }
    if (check(TokenKind::GreaterGreater)) {
        // Split >> into >: mutate current token to > and don't advance.
        tokens_[pos_].kind = TokenKind::Greater;
        tokens_[pos_].lexeme = ">";
        return true;
    }
    return false;
}

void Parser::expect_template_close() {
    if (!match_template_close()) {
        std::ostringstream msg;
        msg << "expected " << token_kind_name(TokenKind::Greater) << " but got '"
            << cur().lexeme << "' at line " << cur().line;
        throw std::runtime_error(msg.str());
    }
}

void Parser::skip_until(TokenKind k) {
    while (!at_end() && !check(k)) consume();
    if (!at_end()) consume();  // consume the terminator
}

Node* Parser::parse() {
    std::vector<Node*> items;
    had_error_ = false;

    while (!at_end()) {
        Node* item = nullptr;
        try {
            item = parse_top_level();
        } catch (const std::exception& e) {
            // Parse error: print warning and try to recover to next semicolon / }
            fprintf(stderr, "parse error: %s (skipping to next ';' or '}')\n", e.what());
            had_error_ = true;
            while (!at_end() && !check(TokenKind::Semi) && !check(TokenKind::RBrace)) {
                consume();
            }
            if (!at_end()) consume();
            continue;
        }
        if (item) items.push_back(item);
    }

    // Prepend collected struct/enum definitions (so IR builder sees them first)
    std::vector<Node*> all;
    for (Node* sd : struct_defs_) all.push_back(sd);
    for (Node* it : items)        all.push_back(it);

    Node* prog = make_node(NK_PROGRAM, 0);
    prog->n_children = (int)all.size();
    if (prog->n_children > 0) {
        prog->children = arena_.alloc_array<Node*>(prog->n_children);
        for (int i = 0; i < prog->n_children; ++i) prog->children[i] = all[i];
    }
    return prog;
}

// ── node constructors ─────────────────────────────────────────────────────────

Node* Parser::make_node(NodeKind k, int line) {
    Node* n = arena_.alloc_array<Node>(1);
    std::memset(n, 0, sizeof(Node));
    n->kind = k;
    n->line = line;
    n->ival = -1;  // -1 = not a bitfield (for struct field declarations)
    n->builtin_id = BuiltinId::Unknown;
    n->type.array_size = -1;
    n->type.array_rank = 0;
    for (int i = 0; i < 8; ++i) n->type.array_dims[i] = -1;
    n->type.inner_rank = -1;
    n->type.is_ptr_to_array = false;
    return n;
}

Node* Parser::make_int_lit(long long v, int line) {
    Node* n = make_node(NK_INT_LIT, line);
    n->ival = v;
    return n;
}

Node* Parser::make_float_lit(double v, const char* raw, int line) {
    Node* n = make_node(NK_FLOAT_LIT, line);
    n->fval = v;
    n->sval = raw;
    return n;
}

Node* Parser::make_str_lit(const char* raw, int line) {
    Node* n = make_node(NK_STR_LIT, line);
    n->sval = raw;
    return n;
}

Node* Parser::make_var(const char* name, int line) {
    Node* n = make_node(NK_VAR, line);
    n->name = name;
    return n;
}

Node* Parser::make_binop(const char* op, Node* l, Node* r, int line) {
    Node* n = make_node(NK_BINOP, line);
    n->op    = arena_.strdup(op);
    n->left  = l;
    n->right = r;
    return n;
}

Node* Parser::make_unary(const char* op, Node* operand, int line) {
    Node* n = make_node(NK_UNARY, line);
    n->op   = arena_.strdup(op);
    n->left = operand;
    return n;
}

Node* Parser::make_assign(const char* op, Node* lhs, Node* rhs, int line) {
    Node* n = make_node(NK_ASSIGN, line);
    n->op    = arena_.strdup(op);
    n->left  = lhs;
    n->right = rhs;
    return n;
}

Node* Parser::make_block(Node** stmts, int n_stmts, int line) {
    Node* n = make_node(NK_BLOCK, line);
    n->n_children = n_stmts;
    if (n_stmts > 0 && stmts) {
        n->children = arena_.alloc_array<Node*>(n_stmts);
        for (int i = 0; i < n_stmts; ++i) n->children[i] = stmts[i];
    }
    return n;
}

// ── AST dump ─────────────────────────────────────────────────────────────────

const char* operator_kind_mangled_name(OperatorKind ok) {
    switch (ok) {
        case OP_NONE:      return nullptr;
        case OP_SUBSCRIPT: return "operator_subscript";
        case OP_DEREF:     return "operator_deref";
        case OP_ARROW:     return "operator_arrow";
        case OP_PRE_INC:   return "operator_preinc";
        case OP_POST_INC:  return "operator_postinc";
        case OP_EQ:        return "operator_eq";
        case OP_NEQ:       return "operator_neq";
        case OP_BOOL:      return "operator_bool";
        case OP_PLUS:      return "operator_plus";
        case OP_MINUS:     return "operator_minus";
        case OP_ASSIGN:    return "operator_assign";
        case OP_CALL:      return "operator_call";
        case OP_LT:        return "operator_lt";
        case OP_GT:        return "operator_gt";
        case OP_LE:        return "operator_le";
        case OP_GE:        return "operator_ge";
    }
    return nullptr;
}

const char* node_kind_name(NodeKind k) {
    switch (k) {
        case NK_INT_LIT:      return "IntLit";
        case NK_FLOAT_LIT:    return "FloatLit";
        case NK_STR_LIT:      return "StrLit";
        case NK_CHAR_LIT:     return "CharLit";
        case NK_VAR:          return "Var";
        case NK_BINOP:        return "BinOp";
        case NK_UNARY:        return "Unary";
        case NK_POSTFIX:      return "Postfix";
        case NK_ADDR:         return "Addr";
        case NK_DEREF:        return "Deref";
        case NK_ASSIGN:       return "Assign";
        case NK_COMPOUND_ASSIGN: return "CompoundAssign";
        case NK_CALL:         return "Call";
        case NK_BUILTIN_CALL: return "BuiltinCall";
        case NK_INDEX:        return "Index";
        case NK_MEMBER:       return "Member";
        case NK_CAST:         return "Cast";
        case NK_TERNARY:      return "Ternary";
        case NK_SIZEOF_EXPR:  return "SizeofExpr";
        case NK_SIZEOF_TYPE:  return "SizeofType";
        case NK_ALIGNOF_TYPE: return "AlignofType";
        case NK_ALIGNOF_EXPR: return "AlignofExpr";
        case NK_COMMA_EXPR:   return "CommaExpr";
        case NK_STMT_EXPR:    return "StmtExpr";
        case NK_COMPOUND_LIT: return "CompoundLit";
        case NK_VA_ARG:       return "VaArg";
        case NK_GENERIC:      return "Generic";
        case NK_INIT_LIST:    return "InitList";
        case NK_INIT_ITEM:    return "InitItem";
        case NK_BLOCK:        return "Block";
        case NK_EXPR_STMT:    return "ExprStmt";
        case NK_RETURN:       return "Return";
        case NK_IF:           return "If";
        case NK_WHILE:        return "While";
        case NK_FOR:          return "For";
        case NK_RANGE_FOR:    return "RangeFor";
        case NK_DO_WHILE:     return "DoWhile";
        case NK_SWITCH:       return "Switch";
        case NK_CASE:         return "Case";
        case NK_CASE_RANGE:   return "CaseRange";
        case NK_DEFAULT:      return "Default";
        case NK_BREAK:        return "Break";
        case NK_CONTINUE:     return "Continue";
        case NK_GOTO:         return "Goto";
        case NK_LABEL:        return "Label";
        case NK_ASM:          return "Asm";
        case NK_EMPTY:        return "Empty";
        case NK_DECL:         return "Decl";
        case NK_FUNCTION:     return "Function";
        case NK_GLOBAL_VAR:   return "GlobalVar";
        case NK_STRUCT_DEF:   return "StructDef";
        case NK_ENUM_DEF:     return "EnumDef";
        case NK_PROGRAM:      return "Program";
        default:              return "Unknown";
    }
}

static void indent_print(int depth) {
    for (int i = 0; i < depth * 2; ++i) putchar(' ');
}

void ast_dump(const Node* n, int indent) {
    if (!n) { indent_print(indent); printf("<null>\n"); return; }
    indent_print(indent);
    printf("%s", node_kind_name(n->kind));
    switch (n->kind) {
        case NK_INT_LIT:   printf("(%lld)", n->ival); break;
        case NK_FLOAT_LIT: printf("(%g)", n->fval); break;
        case NK_CHAR_LIT:  printf("(%lld)", n->ival); break;
        case NK_STR_LIT:   printf("(...)"); break;
        case NK_VAR:       printf("(%s)", n->name ? n->name : "?"); break;
        case NK_BINOP:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_UNARY:     printf("(%s)", n->op ? n->op : "?"); break;
        case NK_POSTFIX:   printf("(%s)", n->op ? n->op : "?"); break;
        case NK_ASSIGN:    printf("(%s)", n->op ? n->op : "?"); break;
        case NK_MEMBER:    printf("(%s%s)", n->is_arrow ? "->" : ".", n->name ? n->name : "?"); break;
        case NK_FUNCTION:  printf("(%s)", n->name ? n->name : "?"); break;
        case NK_DECL:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_GLOBAL_VAR: printf("(%s)", n->name ? n->name : "?"); break;
        case NK_STRUCT_DEF: printf("(%s%s)", n->is_union ? "union " : "struct ", n->name ? n->name : "?"); break;
        case NK_ENUM_DEF:  printf("(enum %s)", n->name ? n->name : "?"); break;
        case NK_GOTO:      printf("(%s)", n->name ? n->name : "?"); break;
        case NK_LABEL:     printf("(%s)", n->name ? n->name : "?"); break;
        case NK_ASM:       printf("(%s)", n->is_volatile_asm ? "volatile" : "plain"); break;
        default: break;
    }
    if (n->n_template_params > 0) {
        printf(" template<");
        for (int i = 0; i < n->n_template_params; ++i) {
            if (i) printf(", ");
            printf("%s", n->template_param_names[i] ? n->template_param_names[i] : "?");
        }
        printf(">");
    }
    if (n->n_template_args > 0) {
        printf(" specialize<%d>", n->n_template_args);
    }
    if (n->is_consteval) printf(" consteval");
    else if (n->is_constexpr) printf(" constexpr");
    if (n->linkage_spec) printf(" linkage=\"%s\"", n->linkage_spec);
    printf("\n");

    // Recurse into children
    if (n->left)   ast_dump(n->left,   indent + 1);
    if (n->right)  ast_dump(n->right,  indent + 1);
    if (n->cond)   ast_dump(n->cond,   indent + 1);
    if (n->then_)  ast_dump(n->then_,  indent + 1);
    if (n->else_)  ast_dump(n->else_,  indent + 1);
    if (n->init)   ast_dump(n->init,   indent + 1);
    if (n->update) ast_dump(n->update, indent + 1);
    if (n->body)   ast_dump(n->body,   indent + 1);

    for (int i = 0; i < n->n_params; ++i) ast_dump(n->params[i], indent + 1);
    for (int i = 0; i < n->n_children; ++i) ast_dump(n->children[i], indent + 1);
    for (int i = 0; i < n->n_fields; ++i) ast_dump(n->fields[i], indent + 1);
}


}  // namespace c4c

#include "parser.hpp"
#include "parser_internal.hpp"

#include <cstring>
#include <stdexcept>

namespace tinyc2ll::frontend_cxx {
Node* Parser::parse_local_decl() {
    int ln = cur().line;

    bool is_static  = false;
    bool is_extern  = false;
    bool is_typedef = false;

    // Consume storage class specifiers before the type
    while (true) {
        if (match(TokenKind::KwStatic))  { is_static = true; }
        else if (match(TokenKind::KwExtern))  { is_extern = true; }
        else if (match(TokenKind::KwTypedef)) { is_typedef = true; }
        else if (match(TokenKind::KwRegister)) {}
        else if (match(TokenKind::KwInline))   {}
        else if (match(TokenKind::KwExtension)) {}
        else break;
    }

    TypeSpec base_ts = parse_base_type();
    skip_attributes();

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (!ts.tag) return true;
        auto it = struct_tag_def_map_.find(ts.tag);
        if (it == struct_tag_def_map_.end()) return true;
        Node* def = it->second;
        return !def || def->n_fields < 0;
    };

    if (is_typedef) {
        // Simple typedef: typedef base_type name[...];
        // Also handles typedef base_type (*fn_name)(...);
        const char* tdname = nullptr;
        TypeSpec ts_copy = base_ts;
        parse_declarator(ts_copy, &tdname);
        if (tdname) {
            auto it = typedef_types_.find(tdname);
            if (user_typedefs_.count(tdname) && it != typedef_types_.end() &&
                !types_compatible_p(it->second, ts_copy, typedef_types_))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            typedefs_.insert(tdname);
            user_typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
        }
        // multiple declarators in typedef
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            parse_declarator(ts2, &tdn2);
            if (tdn2) {
                auto it = typedef_types_.find(tdn2);
                if (user_typedefs_.count(tdn2) && it != typedef_types_.end() &&
                    !types_compatible_p(it->second, ts2, typedef_types_))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                typedefs_.insert(tdn2);
                user_typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
            }
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Collect all declarators for this declaration
    std::vector<Node*> decls;
    do {
        TypeSpec ts = base_ts;
        // Preserve typedef array dims so that e.g. HARD_REG_SET x[2] (where HARD_REG_SET=ulong[2])
        // produces ulong[2][2]. apply_decl_dims prepends declarator dims to base typedef dims.
        ts.array_size_expr = nullptr;
        const char* vname = nullptr;
        parse_declarator(ts, &vname);
        // Skip K&R-style function-type suffix: `float fx ()` in local decls
        if (check(TokenKind::LParen)) skip_paren_group();
        skip_attributes();
        skip_asm();
        skip_attributes();

        if (!vname) {
            // Abstract declarator in statement position — skip
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }
        if (is_incomplete_object_type(ts))
            throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));

        Node* init_node = nullptr;
        if (match(TokenKind::Assign)) {
            init_node = parse_initializer();
        }

        Node* d = make_node(NK_DECL, ln);
        d->type      = ts;
        d->name      = vname;
        d->init      = init_node;
        d->is_static = is_static;
        d->is_extern = is_extern;
        if (vname) var_types_[vname] = ts;  // for typeof(var) resolution
        decls.push_back(d);
    } while (match(TokenKind::Comma));

    match(TokenKind::Semi);

    if (decls.size() == 1) return decls[0];

    // Multiple declarators in one declaration statement (e.g. `int a, b;`):
    // wrap as a synthetic block so statement shape stays single-node.
    // Mark it via ival=1 so IR emission can keep the surrounding scope.
    Node* blk = make_node(NK_BLOCK, ln);
    blk->ival = 1;
    blk->n_children = (int)decls.size();
    blk->children   = arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = decls[i];
    return blk;
}

// ── top-level parsing ─────────────────────────────────────────────────────────

Node* Parser::parse_top_level() {
    struct TopLevelFlagGuard {
        bool& ref;
        bool old;
        explicit TopLevelFlagGuard(bool& r) : ref(r), old(r) { ref = true; }
        ~TopLevelFlagGuard() { ref = old; }
    } top_level_guard(parsing_top_level_context_);

    int ln = cur().line;
    if (at_end()) return nullptr;

    skip_attributes();
    if (at_end()) return nullptr;

    // Handle top-level asm
    if (check(TokenKind::KwAsm)) {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // _Static_assert
    if (check(TokenKind::KwStaticAssert)) {
        consume();
        expect(TokenKind::LParen);
        Node* cond = parse_assign_expr();
        long long cv = 0;
        if (!eval_const_int(cond, &cv, &struct_tag_def_map_))
            throw std::runtime_error("_Static_assert requires an integer constant expression");
        if (cv == 0)
            throw std::runtime_error("_Static_assert condition is false");
        if (match(TokenKind::Comma)) {
            // message argument (typically string literal)
            parse_assign_expr();
        }
        expect(TokenKind::RParen);
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    bool is_static   = false;
    bool is_extern   = false;
    bool is_typedef  = false;
    bool is_inline   = false;

    // Consume storage class specifiers
    while (true) {
        if (match(TokenKind::KwStatic))    { is_static  = true; }
        else if (match(TokenKind::KwExtern))   { is_extern  = true; }
        else if (match(TokenKind::KwTypedef))  { is_typedef = true; }
        else if (match(TokenKind::KwInline))   { is_inline  = true; }
        else if (match(TokenKind::KwExtension)) {}
        else if (match(TokenKind::KwNoreturn))  {}
        else break;
    }
    skip_attributes();

    TypeSpec base_ts{};
    if (!is_type_start()) {
        if (is_typedef) {
            std::string nm = check(TokenKind::Identifier) ? cur().lexeme : "<non-identifier>";
            throw std::runtime_error(std::string("typedef uses unknown base type: ") + nm);
        }
        // K&R/old-style implicit-int function at file scope:
        // e.g. `main() { ... }` or `static foo() { ... }`.
        // Treat as `int name(...)` instead of discarding the whole definition.
        bool maybe_implicit_int_fn =
            check(TokenKind::Identifier) && check2(TokenKind::LParen);
        if (!maybe_implicit_int_fn) {
            skip_until(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }
        base_ts.array_size = -1;
        base_ts.array_rank = 0;
        for (int i = 0; i < 8; ++i) base_ts.array_dims[i] = -1;
        base_ts.base = TB_INT;
    } else {
        base_ts = parse_base_type();
        skip_attributes();
        // Re-scan for storage class specifiers that appear after the base type
        // (e.g. `struct a {...} static g = ...` — GCC allows storage class mid-decl).
        while (true) {
            if (match(TokenKind::KwStatic))    { is_static  = true; }
            else if (match(TokenKind::KwExtern))   { is_extern  = true; }
            else if (match(TokenKind::KwTypedef))  { is_typedef = true; }
            else if (match(TokenKind::KwInline))   { is_inline  = true; }
            else if (match(TokenKind::KwExtension)) {}
            else if (match(TokenKind::KwNoreturn))  {}
            else break;
        }
        skip_attributes();
    }

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (!ts.tag) return true;
        auto it = struct_tag_def_map_.find(ts.tag);
        if (it == struct_tag_def_map_.end()) return true;
        Node* def = it->second;
        return !def || def->n_fields < 0;
    };

    if (is_typedef) {
        // Local typedef
        const char* tdname = nullptr;
        TypeSpec ts_copy = base_ts;
        parse_declarator(ts_copy, &tdname);
        if (tdname) {
            auto it = typedef_types_.find(tdname);
            if (user_typedefs_.count(tdname) && it != typedef_types_.end() &&
                !types_compatible_p(it->second, ts_copy, typedef_types_))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            typedefs_.insert(tdname);
            user_typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
        }
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            parse_declarator(ts2, &tdn2);
            if (tdn2) {
                auto it = typedef_types_.find(tdn2);
                if (user_typedefs_.count(tdn2) && it != typedef_types_.end() &&
                    !types_compatible_p(it->second, ts2, typedef_types_))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                typedefs_.insert(tdn2);
                user_typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
            }
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Peek to disambiguate: struct/union/enum declaration only (no declarator)
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        return make_node(NK_EMPTY, ln);
    }

    // Parse declarator (name + pointer stars + maybe function params)
    TypeSpec ts = base_ts;
    // Preserve typedef array dims: apply_decl_dims prepends declarator dims to base typedef dims.
    ts.array_size_expr = nullptr;
    const char* decl_name = nullptr;
    bool is_fptr_global = false;

    // Check for function-pointer global: type (*name)(...);
    // Also handles function-returning-fptr: int (* f1(params))(ret_params);
    bool fn_returning_fptr = false;
    std::vector<Node*> fptr_fn_params;
    bool fptr_fn_variadic = false;
    if (check(TokenKind::LParen) && check2(TokenKind::Star)) {
        consume();  // (
        consume();  // *
        ts.ptr_level++;
        while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
        // Skip qualifiers after * (e.g. `double (* const a[])`)
        while (is_qualifier(cur().kind)) consume();
        // Skip nullability annotations: (* _Nullable name) or (* _Nonnull name)
        while (check(TokenKind::Identifier)) {
            const std::string& ql = cur().lexeme;
            if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                ql == "__nullable" || ql == "__nonnull")
                consume();
            else break;
        }
        skip_attributes();
        if (check(TokenKind::Identifier)) {
            decl_name = arena_.strdup(cur().lexeme);
            consume();
        }
        // Handle array-of-function-pointer: void (*func2[6])(void)
        while (check(TokenKind::LBracket)) {
            consume();  // [
            long long dim = -2;
            if (!check(TokenKind::RBracket)) {
                Node* sz = parse_expr();
                if (sz) { long long v; if (eval_const_int(sz, &v)) dim = v; }
            }
            expect(TokenKind::RBracket);
            if (ts.array_rank < 8) ts.array_dims[ts.array_rank++] = dim;
            if (ts.array_rank == 1) ts.array_size = ts.array_dims[0];
        }
        // Detect function-returning-fptr: (* f1(params)) — has inner params before ')'
        if (check(TokenKind::LParen)) {
            fn_returning_fptr = true;
            // Parse the function's own parameter list properly
            consume();  // consume '('
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) {
                        fptr_fn_variadic = true; consume(); break;
                    }
                    if (check(TokenKind::RParen)) break;
                    if (!is_type_start()) { consume(); if (match(TokenKind::Comma)) continue; break; }
                    Node* p = parse_param();
                    if (p) fptr_fn_params.push_back(p);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);  // close function's own params
        }
        expect(TokenKind::RParen);  // close (*name...)
        if (check(TokenKind::LParen)) skip_paren_group();  // skip return-fptr params
        if (!fn_returning_fptr) is_fptr_global = true;
    } else {
    parse_declarator(ts, &decl_name);
    if (is_incomplete_object_type(ts) && !check(TokenKind::LParen))
        throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
    }

    skip_attributes();
    skip_asm();
    skip_attributes();

    if (!decl_name) {
        // No name: just a type declaration; skip to ;
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    // Handle function-returning-fptr: int (* f1(a, b))(c, d) { body }
    // Params were already parsed into fptr_fn_params; now look for { body }.
    if (fn_returning_fptr && decl_name) {
        skip_attributes(); skip_asm(); skip_attributes();
        if (check(TokenKind::LBrace)) {
            bool saved_top = parsing_top_level_context_;
            parsing_top_level_context_ = false;
            Node* body = parse_block();
            parsing_top_level_context_ = saved_top;
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = decl_name;
            fn->variadic  = fptr_fn_variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->body      = body;
            fn->n_params  = (int)fptr_fn_params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
            }
            return fn;
        }
        // Function declaration (no body): consume semicolon and return
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = decl_name;
        fn->variadic  = fptr_fn_variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->body      = nullptr;
        fn->n_params  = (int)fptr_fn_params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
        }
        return fn;
    }

    // Check for function definition: name followed by ( params ) {
    // We need to figure out if we're looking at a function or a variable.
    // The key distinction: if we have ( params ) { body }, it's a function.
    // We might have already consumed the function params in parse_declarator.
    // Re-check: if the next token is { or if the type's base indicates function.

    // Actually, after parse_declarator, if we encounter '(' next, it means
    // the declarator name was just the name and the function params are next.
    // OR: the declarator already handled the suffix.
    // Let's handle it here.

    if (!is_fptr_global && check(TokenKind::LParen)) {
        // Function declaration or definition
        consume();  // consume (
        std::vector<Node*> params;
        std::vector<const char*> knr_param_names;
        bool variadic = false;
        if (!check(TokenKind::RParen)) {
            while (!at_end()) {
                if (check(TokenKind::Ellipsis)) {
                    variadic = true;
                    consume();
                    break;
                }
                if (check(TokenKind::RParen)) break;
                // K&R-style: just identifiers
                if (!is_type_start() && check(TokenKind::Identifier)) {
                    knr_param_names.push_back(arena_.strdup(cur().lexeme));
                    consume();
                    if (match(TokenKind::Comma)) continue;
                    break;
                }
                if (!is_type_start()) break;
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
        skip_attributes();
        skip_asm();
        skip_attributes();

        // K&R style parameter declarations before body:
        //   f(a, b) int a; short *b; { ... }
        // Bind declared types back to identifier list order.
        if (!knr_param_names.empty()) {
            std::unordered_map<std::string, Node*> knr_param_decl_map;
            while (is_type_start() && !check(TokenKind::LBrace)) {
                TypeSpec knr_base = parse_base_type();
                do {
                    TypeSpec knr_ts = knr_base;
                    knr_ts.array_size = -1;
                    knr_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) knr_ts.array_dims[i] = -1;
                    knr_ts.is_ptr_to_array = false;
                    knr_ts.array_size_expr = nullptr;
                    const char* knr_name = nullptr;
                    parse_declarator(knr_ts, &knr_name);
                    if (!knr_name) continue;
                    bool listed = false;
                    for (const char* pnm : knr_param_names) {
                        if (pnm && strcmp(pnm, knr_name) == 0) { listed = true; break; }
                    }
                    if (!listed) continue;
                    Node* p = make_node(NK_DECL, ln);
                    p->name = knr_name;
                    p->type = knr_ts;
                    knr_param_decl_map[knr_name] = p;
                } while (match(TokenKind::Comma));
                match(TokenKind::Semi);
            }
            params.clear();
            for (const char* pnm : knr_param_names) {
                auto it = knr_param_decl_map.find(pnm ? pnm : "");
                if (it != knr_param_decl_map.end()) {
                    params.push_back(it->second);
                    continue;
                }
                // Old-style fallback: undeclared parameter defaults to int.
                Node* p = make_node(NK_DECL, ln);
                p->name = pnm;
                TypeSpec pts{};
                pts.array_size = -1;
                pts.array_rank = 0;
                for (int i = 0; i < 8; ++i) pts.array_dims[i] = -1;
                pts.base = TB_INT;
                p->type = pts;
                params.push_back(p);
            }
        } else {
            // Non-K&R: ignore declaration-only lines before body.
            while (is_type_start() && !check(TokenKind::LBrace)) {
                skip_until(TokenKind::Semi);
            }
        }

        if (check(TokenKind::LBrace)) {
            // Function definition
            bool saved_top = parsing_top_level_context_;
            parsing_top_level_context_ = false;
            Node* body = parse_block();
            parsing_top_level_context_ = saved_top;
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = decl_name;
            fn->variadic  = variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->body      = body;
            fn->n_params  = (int)params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }
            return fn;
        }

        // Function declaration only
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = decl_name;
        fn->variadic  = variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->body      = nullptr;  // declaration only
        fn->n_params  = (int)params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
        }
        return fn;
    }

    // Global variable (possibly with initializer and multiple declarators)
    std::vector<Node*> gvars;

    auto make_gvar = [&](TypeSpec gts, const char* gname, Node* ginit) {
        Node* gv = make_node(NK_GLOBAL_VAR, ln);
        gv->type      = gts;
        gv->name      = gname;
        gv->init      = ginit;
        gv->is_static = is_static;
        gv->is_extern = is_extern;
        if (gname) var_types_[gname] = gts;  // for typeof(var) resolution
        return gv;
    };

    Node* first_init = nullptr;
    if (match(TokenKind::Assign)) {
        first_init = parse_initializer();
    }
    gvars.push_back(make_gvar(ts, decl_name, first_init));

    while (match(TokenKind::Comma)) {
        TypeSpec ts2 = base_ts;
        // Preserve typedef ptr_level and array dims for each additional declarator.
        ts2.array_size_expr = nullptr;
        const char* n2 = nullptr;
        parse_declarator(ts2, &n2);
        skip_attributes();
        skip_asm();
        Node* init2 = nullptr;
        if (match(TokenKind::Assign)) init2 = parse_initializer();
        if (n2) gvars.push_back(make_gvar(ts2, n2, init2));
    }
    match(TokenKind::Semi);

    if (gvars.size() == 1) return gvars[0];

    // Multiple global vars — wrap in a block
    Node* blk = make_node(NK_BLOCK, ln);
    blk->n_children = (int)gvars.size();
    blk->children   = arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = gvars[i];
    return blk;
}

}  // namespace tinyc2ll::frontend_cxx

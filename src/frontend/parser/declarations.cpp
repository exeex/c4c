#include "parser.hpp"
#include "parser_internal.hpp"

#include <cstring>
#include <stdexcept>

namespace c4c {
namespace {

const char* maybe_parse_linkage_spec(Parser* parser) {
    if (!parser->is_cpp_mode() || !parser->check(TokenKind::StrLit)) return nullptr;
    if (parser->cur().lexeme == "\"C\"") {
        parser->consume();
        return "C";
    }
    if (parser->cur().lexeme == "\"C++\"") {
        parser->consume();
        return "C++";
    }
    return nullptr;
}

const char* make_anon_template_param_name(Arena& arena, bool is_nttp, size_t index) {
    std::string name = is_nttp ? "__anon_nttp_" : "__anon_tparam_";
    name += std::to_string(index);
    return arena.strdup(name.c_str());
}

}  // namespace

Node* Parser::parse_local_decl() {
    int ln = cur().line;

    bool is_static  = false;
    bool is_extern  = false;
    bool is_typedef = false;
    bool is_constexpr = false;
    bool is_consteval = false;
    const char* linkage_spec = nullptr;

    // Consume storage class specifiers before the type
    while (true) {
        if (match(TokenKind::KwStatic))  { is_static = true; }
        else if (match(TokenKind::KwExtern))  {
            is_extern = true;
            if (const char* spec = maybe_parse_linkage_spec(this)) linkage_spec = spec;
        }
        else if (match(TokenKind::KwTypedef)) { is_typedef = true; }
        else if (match(TokenKind::KwRegister)) {}
        else if (match(TokenKind::KwInline))   {}
        else if (is_cpp_mode() && match(TokenKind::KwConstexpr)) { is_constexpr = true; }
        else if (is_cpp_mode() && match(TokenKind::KwConsteval)) { is_consteval = true; }
        else if (match(TokenKind::KwExtension)) {}
        else break;
    }

    // C++ consteval is only valid on functions, not local variable declarations.
    if (is_consteval) {
        throw std::runtime_error(
            std::string("error: 'consteval' can only be applied to a function declaration (line ")
            + std::to_string(ln) + ")");
    }

    TypeSpec base_ts = parse_base_type();
    parse_attributes(&base_ts);

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (ts.tpl_struct_origin) return false;  // pending template struct — resolved at HIR level
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
        Node** td_fn_ptr_params = nullptr;
        int td_n_fn_ptr_params = 0;
        bool td_fn_ptr_variadic = false;
        parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic);
        if (tdname) {
            auto it = typedef_types_.find(tdname);
            if (user_typedefs_.count(tdname) && it != typedef_types_.end() &&
                !types_compatible_p(it->second, ts_copy, typedef_types_))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            typedefs_.insert(tdname);
            user_typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
            // Phase C: store fn_ptr param info for typedef'd function pointers.
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
                typedef_fn_ptr_info_[tdname] = {td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
            }
        }
        // multiple declarators in typedef
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic);
            if (tdn2) {
                auto it = typedef_types_.find(tdn2);
                if (user_typedefs_.count(tdn2) && it != typedef_types_.end() &&
                    !types_compatible_p(it->second, ts2, typedef_types_))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                typedefs_.insert(tdn2);
                user_typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                    typedef_fn_ptr_info_[tdn2] = {td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
                }
            }
        }
        match(TokenKind::Semi);
        // Keep enum definition nodes for `typedef enum { ... } T;` so
        // downstream semantic passes can bind enumerator constants.
        if (base_ts.base == TB_ENUM && last_enum_def_) return last_enum_def_;
        return make_node(NK_EMPTY, ln);
    }

    // declaration-only tag type in local scope: `enum/struct/union ... ;`
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        if (base_ts.base == TB_ENUM && last_enum_def_) {
            return last_enum_def_;
        }
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
        Node** fn_ptr_params = nullptr;
        int n_fn_ptr_params = 0;
        bool fn_ptr_variadic = false;
        Node** ret_fn_ptr_params = nullptr;
        int n_ret_fn_ptr_params = 0;
        bool ret_fn_ptr_variadic = false;
        parse_declarator(ts, &vname, &fn_ptr_params, &n_fn_ptr_params, &fn_ptr_variadic,
                         &ret_fn_ptr_params, &n_ret_fn_ptr_params, &ret_fn_ptr_variadic);
        // C++ constructor invocation: `Type var(args)` where Type is a struct.
        // In C mode this is a K&R function-type suffix that we skip.
        bool is_kr_fn_decl = false;
        bool is_ctor_init = false;
        std::vector<Node*> ctor_args;
        if (check(TokenKind::LParen)) {
            if (is_cpp_mode() && vname &&
                (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
                 (base_ts.base == TB_TYPEDEF && base_ts.tag))) {
                // C++ constructor call: parse arguments
                consume();  // '('
                if (!check(TokenKind::RParen)) {
                    while (!at_end()) {
                        if (check(TokenKind::RParen)) break;
                        Node* arg = parse_assign_expr();
                        if (arg) ctor_args.push_back(arg);
                        if (!match(TokenKind::Comma)) break;
                    }
                }
                expect(TokenKind::RParen);
                is_ctor_init = true;
            } else {
                // K&R-style function-type suffix: `float fx ()` in local decls.
                skip_paren_group();
                is_kr_fn_decl = true;
            }
        }
        skip_attributes();
        skip_asm();
        skip_attributes();

        if (!vname) {
            // Abstract declarator in statement position — skip
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }

        if (is_kr_fn_decl) continue;  // K&R fn decl: no local variable

        if (is_incomplete_object_type(ts))
            throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));

        Node* init_node = nullptr;
        if (!is_ctor_init) {
            if (match(TokenKind::Assign) ||
                (is_cpp_mode() && check(TokenKind::LBrace))) {
                init_node = parse_initializer();
            }
        }

        Node* d = make_node(NK_DECL, ln);
        d->type      = ts;
        if (is_constexpr) d->type.is_const = true;
        d->name      = vname;
        d->init      = init_node;
        d->is_ctor_init = is_ctor_init;
        if (is_ctor_init && !ctor_args.empty()) {
            d->n_children = (int)ctor_args.size();
            d->children = arena_.alloc_array<Node*>(d->n_children);
            for (int i = 0; i < d->n_children; ++i) d->children[i] = ctor_args[i];
        }
        d->is_static = is_static;
        d->is_extern = is_extern;
        d->is_constexpr = is_constexpr;
        d->is_consteval = is_consteval;
        d->linkage_spec = linkage_spec;
        d->fn_ptr_params = fn_ptr_params;
        d->n_fn_ptr_params = n_fn_ptr_params;
        d->fn_ptr_variadic = fn_ptr_variadic;
        d->ret_fn_ptr_params = ret_fn_ptr_params;
        d->n_ret_fn_ptr_params = n_ret_fn_ptr_params;
        d->ret_fn_ptr_variadic = ret_fn_ptr_variadic;
        // Phase C: propagate fn_ptr params from typedef if not set by declarator.
        if (ts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
            auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
            if (tdit != typedef_fn_ptr_info_.end()) {
                d->fn_ptr_params = tdit->second.params;
                d->n_fn_ptr_params = tdit->second.n_params;
                d->fn_ptr_variadic = tdit->second.variadic;
            }
        }
        if (vname) var_types_[vname] = ts;  // for typeof(var) resolution
        decls.push_back(d);
    } while (match(TokenKind::Comma));

    match(TokenKind::Semi);

    if (decls.empty()) return make_node(NK_EMPTY, ln);
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

    if (is_cpp_mode() && check(TokenKind::KwNamespace)) {
        consume();
        QualifiedNameRef ns_name;
        bool has_name = false;
        if (check(TokenKind::Identifier)) {
            ns_name = parse_qualified_name(false);
            has_name = true;
        }
        expect(TokenKind::LBrace);

        int context_id = current_namespace_context_id();
        if (has_name) {
            for (const std::string& segment : ns_name.qualifier_segments) {
                context_id = ensure_named_namespace_context(context_id, segment);
            }
            context_id = ensure_named_namespace_context(context_id, ns_name.base_name);
        } else {
            context_id = create_anonymous_namespace_context(context_id);
        }
        push_namespace_context(context_id);

        std::vector<Node*> items;
        while (!at_end() && !check(TokenKind::RBrace)) {
            Node* item = parse_top_level();
            if (item) items.push_back(item);
        }
        expect(TokenKind::RBrace);
        match(TokenKind::Semi);
        pop_namespace_context();

        if (items.empty()) return make_node(NK_EMPTY, ln);
        if (items.size() == 1) return items[0];
        Node* blk = make_node(NK_BLOCK, ln);
        blk->n_children = (int)items.size();
        blk->children = arena_.alloc_array<Node*>(blk->n_children);
        for (int i = 0; i < blk->n_children; ++i) blk->children[i] = items[i];
        return blk;
    }

    if (is_cpp_mode() && check(TokenKind::KwUsing)) {
        consume();
        const int using_context_id = current_namespace_context_id();
        if (match(TokenKind::KwNamespace)) {
            if (!check(TokenKind::Identifier))
                throw std::runtime_error("expected namespace name after 'using namespace'");
            QualifiedNameRef target_name = parse_qualified_name(true);
            int target_context = resolve_namespace_name(target_name);
            if (target_context < 0) {
                throw std::runtime_error("unknown namespace in using-directive");
            }
            using_namespace_contexts_[using_context_id].push_back(target_context);
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }

        if (!check(TokenKind::Identifier))
            throw std::runtime_error("expected identifier after 'using'");

        std::string first_name = cur().lexeme;
        consume();

        if (match(TokenKind::Assign)) {
            TypeSpec alias_ts = parse_type_name();
            const std::string qualified = canonical_name_in_context(using_context_id, first_name);
            typedefs_.insert(qualified);
            user_typedefs_.insert(qualified);
            typedef_types_[qualified] = alias_ts;
            if (using_context_id == 0) {
                typedefs_.insert(first_name);
                user_typedefs_.insert(first_name);
                typedef_types_[first_name] = alias_ts;
            }
            match(TokenKind::Semi);
            return make_node(NK_EMPTY, ln);
        }

        std::string target = first_name;
        while (match(TokenKind::ColonColon)) {
            if (!check(TokenKind::Identifier))
                throw std::runtime_error("expected identifier after '::'");
            target += "::";
            target += cur().lexeme;
            consume();
        }

        const size_t last_sep = target.rfind("::");
        const std::string imported_name =
            (last_sep == std::string::npos) ? target : target.substr(last_sep + 2);
        auto td_it = typedef_types_.find(target);
        if (td_it != typedef_types_.end()) {
            const std::string imported_key =
                canonical_name_in_context(using_context_id, imported_name);
            typedefs_.insert(imported_key);
            user_typedefs_.insert(imported_key);
            typedef_types_[imported_key] = td_it->second;
            if (using_context_id == 0) {
                typedefs_.insert(imported_name);
                user_typedefs_.insert(imported_name);
                typedef_types_[imported_name] = td_it->second;
            }
        } else {
            using_value_aliases_[using_context_id][imported_name] = target;
        }
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    if (is_cpp_mode() && check(TokenKind::KwTemplate)) {
        consume();
        expect(TokenKind::Less);

        // Explicit template specialization: template<> RetType name<Args>(...) { ... }
        if (check(TokenKind::Greater)) {
            consume();  // consume >
            bool saved_spec = parsing_explicit_specialization_;
            parsing_explicit_specialization_ = true;
            Node* spec = parse_top_level();
            parsing_explicit_specialization_ = saved_spec;
            if (spec) spec->is_explicit_specialization = true;
            return spec;
        }

        std::vector<const char*> template_params;
        std::vector<bool> template_param_nttp;  // true if non-type template param
        std::vector<bool> template_param_has_default;
        std::vector<TypeSpec> template_param_default_types;
        std::vector<long long> template_param_default_values;
        while (!at_end() && !check(TokenKind::Greater)) {
            if ((check(TokenKind::Identifier) && cur().lexeme == "typename") ||
                check(TokenKind::KwClass)) {
                // Type template parameter.
                consume();
                const char* pname = nullptr;
                if (check(TokenKind::Identifier)) {
                    pname = arena_.strdup(cur().lexeme);
                    consume();
                } else {
                    pname = make_anon_template_param_name(arena_, false, template_params.size());
                }
                template_params.push_back(pname);
                template_param_nttp.push_back(false);
                // Check for default type argument: = type
                if (match(TokenKind::Assign)) {
                    TypeSpec def_ts = parse_type_name();
                    template_param_has_default.push_back(true);
                    template_param_default_types.push_back(def_ts);
                    template_param_default_values.push_back(0);
                } else {
                    template_param_has_default.push_back(false);
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_param_default_types.push_back(dummy);
                    template_param_default_values.push_back(0);
                }
            } else if (is_type_kw(cur().kind)) {
                // Non-type template parameter (NTTP): e.g. int N, unsigned N.
                // Consume the type specifier tokens, then the parameter name.
                while (!at_end() && is_type_kw(cur().kind)) consume();
                const char* pname = nullptr;
                if (check(TokenKind::Identifier)) {
                    pname = arena_.strdup(cur().lexeme);
                    consume();
                } else {
                    pname = make_anon_template_param_name(arena_, true, template_params.size());
                }
                template_params.push_back(pname);
                template_param_nttp.push_back(true);
                // Check for default NTTP value: = expr
                if (match(TokenKind::Assign)) {
                    long long sign = 1;
                    if (match(TokenKind::Minus)) sign = -1;
                    if (check(TokenKind::IntLit)) {
                        long long val = parse_int_lexeme(cur().lexeme.c_str());
                        consume();
                        template_param_has_default.push_back(true);
                        TypeSpec dummy{};
                        dummy.array_size = -1;
                        dummy.inner_rank = -1;
                        template_param_default_types.push_back(dummy);
                        template_param_default_values.push_back(val * sign);
                    } else {
                        // Complex default expression — skip for now
                        int depth = 0;
                        while (!at_end()) {
                            if (check(TokenKind::Less)) ++depth;
                            else if (check(TokenKind::Greater)) {
                                if (depth == 0) break;
                                --depth;
                            } else if (check(TokenKind::Comma) && depth == 0) {
                                break;
                            }
                            consume();
                        }
                        template_param_has_default.push_back(false);
                        TypeSpec dummy{};
                        dummy.array_size = -1;
                        dummy.inner_rank = -1;
                        template_param_default_types.push_back(dummy);
                        template_param_default_values.push_back(0);
                    }
                } else {
                    template_param_has_default.push_back(false);
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_param_default_types.push_back(dummy);
                    template_param_default_values.push_back(0);
                }
            } else {
                throw std::runtime_error("expected template parameter name");
            }
            if (!match(TokenKind::Comma)) break;
        }
        expect(TokenKind::Greater);

        std::vector<std::string> injected_names;
        for (size_t i = 0; i < template_params.size(); ++i) {
            const char* pname = template_params[i];
            if (!pname || !pname[0]) continue;
            if (template_param_nttp[i]) continue;  // NTTP: don't inject as typedef
            injected_names.emplace_back(pname);
            typedefs_.insert(pname);
            TypeSpec param_ts{};
            param_ts.array_size = -1;
            param_ts.array_rank = 0;
            param_ts.base = TB_TYPEDEF;
            param_ts.tag = pname;
            typedef_types_[pname] = param_ts;
        }

        size_t struct_defs_before = struct_defs_.size();
        Node* templated = parse_top_level();
        for (const std::string& pname : injected_names) {
            typedefs_.erase(pname);
            typedef_types_.erase(pname);
        }

        auto attach_template_params = [&](Node* n) {
            if (!n || template_params.empty()) return;
            n->n_template_params = (int)template_params.size();
            n->template_param_names = arena_.alloc_array<const char*>(n->n_template_params);
            n->template_param_is_nttp = arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_has_default = arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_default_types = arena_.alloc_array<TypeSpec>(n->n_template_params);
            n->template_param_default_values = arena_.alloc_array<long long>(n->n_template_params);
            for (int i = 0; i < n->n_template_params; ++i) {
                n->template_param_names[i] = template_params[i];
                n->template_param_is_nttp[i] = template_param_nttp[i];
                n->template_param_has_default[i] = template_param_has_default[i];
                n->template_param_default_types[i] = template_param_default_types[i];
                n->template_param_default_values[i] = template_param_default_values[i];
            }
        };

        if (templated && templated->kind == NK_BLOCK) {
            for (int i = 0; i < templated->n_children; ++i) {
                attach_template_params(templated->children[i]);
            }
        } else {
            attach_template_params(templated);
        }
        // Template struct definitions: struct Pair { ... } was parsed inside the
        // recursive parse_top_level() call and stored in struct_defs_, while
        // `templated` is NK_EMPTY (struct-only declaration).  Find the struct
        // def and attach template params to it.  Only consider structs that were
        // added during THIS template parse (not pre-existing instantiated structs).
        if (struct_defs_.size() > struct_defs_before && !template_params.empty()) {
            Node* last_sd = struct_defs_.back();
            if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                last_sd->n_template_params == 0) {
                attach_template_params(last_sd);
                template_struct_defs_[last_sd->name] = last_sd;
                // In C++ mode, register the template struct name as a typedef
                // so it's recognized as a type start for Pair<int> syntax.
                typedefs_.insert(last_sd->name);
                TypeSpec struct_ts{};
                struct_ts.array_size = -1;
                struct_ts.inner_rank = -1;
                struct_ts.base = TB_STRUCT;
                struct_ts.tag = last_sd->name;
                typedef_types_[last_sd->name] = struct_ts;
                // If inside a namespace, also register ns::Name so
                // qualified references like std::vector<int> work.
                const std::string qn =
                    canonical_name_in_context(current_namespace_context_id(), last_sd->name);
                typedefs_.insert(qn);
                typedef_types_[qn] = struct_ts;
                template_struct_defs_[qn] = last_sd;
            }
        }
        return templated;
    }

    // Handle #pragma pack tokens
    if (check(TokenKind::PragmaPack)) {
        handle_pragma_pack(cur().lexeme);
        consume();
        return make_node(NK_EMPTY, ln);
    }

    // Handle #pragma weak tokens
    if (check(TokenKind::PragmaWeak)) {
        auto* node = make_node(NK_PRAGMA_WEAK, ln);
        node->name = arena_.strdup(cur().lexeme);
        consume();
        return node;
    }

    // Handle #pragma GCC visibility push/pop tokens
    if (check(TokenKind::PragmaGccVisibility)) {
        handle_pragma_gcc_visibility(cur().lexeme);
        consume();
        return make_node(NK_EMPTY, ln);
    }

    // Don't skip_attributes() here — type-affecting attributes like
    // __attribute__((vector_size(N))) must flow to parse_base_type().
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
    bool is_constexpr = false;
    bool is_consteval = false;
    const char* linkage_spec = nullptr;

    // Consume storage class specifiers
    while (true) {
        if (match(TokenKind::KwStatic))    { is_static  = true; }
        else if (match(TokenKind::KwExtern))   {
            is_extern  = true;
            if (const char* spec = maybe_parse_linkage_spec(this)) linkage_spec = spec;
        }
        else if (match(TokenKind::KwTypedef))  { is_typedef = true; }
        else if (match(TokenKind::KwInline))   { is_inline  = true; }
        else if (is_cpp_mode() && match(TokenKind::KwConstexpr)) { is_constexpr = true; }
        else if (is_cpp_mode() && match(TokenKind::KwConsteval)) { is_consteval = true; }
        else if (match(TokenKind::KwExtension)) {}
        else if (match(TokenKind::KwNoreturn))  {}
        else break;
    }
    // Don't skip_attributes() here — let parse_base_type() handle them so
    // type-affecting attributes like vector_size are captured.

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
        parse_attributes(&base_ts);
        // Re-scan for storage class specifiers that appear after the base type
        // (e.g. `struct a {...} static g = ...` — GCC allows storage class mid-decl).
        while (true) {
            if (match(TokenKind::KwStatic))    { is_static  = true; }
            else if (match(TokenKind::KwExtern))   {
                is_extern  = true;
                if (const char* spec = maybe_parse_linkage_spec(this)) linkage_spec = spec;
            }
            else if (match(TokenKind::KwTypedef))  { is_typedef = true; }
            else if (match(TokenKind::KwInline))   { is_inline  = true; }
            else if (is_cpp_mode() && match(TokenKind::KwConstexpr)) { is_constexpr = true; }
            else if (is_cpp_mode() && match(TokenKind::KwConsteval)) { is_consteval = true; }
            else if (match(TokenKind::KwExtension)) {}
            else if (match(TokenKind::KwNoreturn))  {}
            else break;
        }
        parse_attributes(&base_ts);
    }
    // C++: consteval and constexpr cannot both appear on the same declaration.
    if (is_consteval && is_constexpr) {
        throw std::runtime_error(
            std::string("error: 'consteval' and 'constexpr' cannot be combined (line ")
            + std::to_string(ln) + ")");
    }

    // Phase C: save the return-type typedef name for fn_ptr propagation to NK_FUNCTION.
    const std::string ret_typedef_name = last_resolved_typedef_;

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (ts.tpl_struct_origin) return false;  // pending template struct — resolved at HIR level
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
        Node** td_fn_ptr_params = nullptr;
        int td_n_fn_ptr_params = 0;
        bool td_fn_ptr_variadic = false;
        parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic);
        if (tdname) {
            const char* scoped_tdname = qualify_name_arena(tdname);
            auto it = typedef_types_.find(tdname);
            if (user_typedefs_.count(tdname) && it != typedef_types_.end() &&
                !types_compatible_p(it->second, ts_copy, typedef_types_))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            typedefs_.insert(tdname);
            user_typedefs_.insert(tdname);
            typedef_types_[tdname] = ts_copy;
            if (scoped_tdname != tdname) {
                typedefs_.insert(scoped_tdname);
                typedef_types_[scoped_tdname] = ts_copy;
            }
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic))
                typedef_fn_ptr_info_[tdname] = {td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
        }
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic);
            if (tdn2) {
                const char* scoped_tdn2 = qualify_name_arena(tdn2);
                auto it = typedef_types_.find(tdn2);
                if (user_typedefs_.count(tdn2) && it != typedef_types_.end() &&
                    !types_compatible_p(it->second, ts2, typedef_types_))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                typedefs_.insert(tdn2);
                user_typedefs_.insert(tdn2);
                typedef_types_[tdn2] = ts2;
                if (scoped_tdn2 != tdn2) {
                    typedefs_.insert(scoped_tdn2);
                    typedef_types_[scoped_tdn2] = ts2;
                }
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic))
                    typedef_fn_ptr_info_[tdn2] = {td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
            }
        }
        match(TokenKind::Semi);
        // Preserve enum definitions in typedef declarations so global
        // enumerators are visible after `typedef enum { ... } Name;`.
        if (base_ts.base == TB_ENUM && last_enum_def_) return last_enum_def_;
        return make_node(NK_EMPTY, ln);
    }

    // Peek to disambiguate: struct/union/enum declaration only (no declarator)
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        if (base_ts.base == TB_ENUM && last_enum_def_) {
            return last_enum_def_;
        }
        return make_node(NK_EMPTY, ln);
    }

    // Parse declarator (name + pointer stars + maybe function params)
    TypeSpec ts = base_ts;
    // Preserve typedef array dims: apply_decl_dims prepends declarator dims to base typedef dims.
    ts.array_size_expr = nullptr;
    const char* decl_name = nullptr;
    Node** decl_fn_ptr_params = nullptr;
    int decl_n_fn_ptr_params = 0;
    bool decl_fn_ptr_variadic = false;
    Node** decl_ret_fn_ptr_params = nullptr;
    int decl_n_ret_fn_ptr_params = 0;
    bool decl_ret_fn_ptr_variadic = false;
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
        // Detect nested fn_ptr: (* (*name(params))(ret_params)) or function-returning-fptr: (* f1(params))
        if (check(TokenKind::LParen) && check2(TokenKind::Star)) {
            // Nested fn_ptr: (* (*name(fn_params))(inner_ret_params) )(outer_ret_params)
            // Recurse: consume '(' '*' to enter the inner level.
            consume();  // (
            consume();  // *
            ts.ptr_level++;
            while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
            // Skip qualifiers/nullability
            while (is_qualifier(cur().kind)) consume();
            while (check(TokenKind::Identifier)) {
                const std::string& ql = cur().lexeme;
                if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                    ql == "__nullable" || ql == "__nonnull") { consume(); continue; }
                break;
            }
            if (check(TokenKind::Identifier)) {
                decl_name = arena_.strdup(cur().lexeme);
                consume();
            }
            // Function's own params (pick_outer(int which))
            if (decl_name && check(TokenKind::LParen)) {
                fn_returning_fptr = true;
                consume();
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
                expect(TokenKind::RParen);
            }
            expect(TokenKind::RParen);  // close inner (*name(params))
            // Parse inner return fn_ptr params: (int)
            if (check(TokenKind::LParen)) {
                std::vector<Node*> inner_ret_params;
                bool inner_ret_variadic = false;
                consume();
                if (!check(TokenKind::RParen)) {
                    while (!at_end()) {
                        if (check(TokenKind::Ellipsis)) {
                            inner_ret_variadic = true; consume(); break;
                        }
                        if (check(TokenKind::RParen)) break;
                        if (!is_type_start()) { consume(); if (match(TokenKind::Comma)) continue; break; }
                        Node* p = parse_param();
                        if (p) inner_ret_params.push_back(p);
                        if (!match(TokenKind::Comma)) break;
                    }
                }
                expect(TokenKind::RParen);
                ts.is_fn_ptr = true;
                // For fn-returning-fptr: these are the inner return params (the first level of return fn_ptr).
                // Store as fn_ptr_params for now; outer params (below) become ret_fn_ptr_params.
                if (fn_returning_fptr) {
                    decl_n_fn_ptr_params = static_cast<int>(inner_ret_params.size());
                    decl_fn_ptr_variadic = inner_ret_variadic;
                    if (!inner_ret_params.empty()) {
                        decl_fn_ptr_params = arena_.alloc_array<Node*>(decl_n_fn_ptr_params);
                        for (int i = 0; i < decl_n_fn_ptr_params; ++i)
                            decl_fn_ptr_params[i] = inner_ret_params[i];
                    }
                }
            }
        } else if (check(TokenKind::LParen)) {
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
        if (check(TokenKind::LParen)) {
            std::vector<Node*> pointed_fn_params;
            bool pointed_fn_variadic = false;
            consume();  // (
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) {
                        pointed_fn_variadic = true;
                        consume();
                        break;
                    }
                    if (check(TokenKind::RParen)) break;
                    if (!is_type_start()) {
                        consume();
                        if (match(TokenKind::Comma)) continue;
                        break;
                    }
                    Node* p = parse_param();
                    if (p) pointed_fn_params.push_back(p);
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);
            ts.is_fn_ptr = true; // return type is a function pointer
            if (!fn_returning_fptr) {
                decl_n_fn_ptr_params = static_cast<int>(pointed_fn_params.size());
                decl_fn_ptr_variadic = pointed_fn_variadic;
                if (!pointed_fn_params.empty()) {
                    decl_fn_ptr_params = arena_.alloc_array<Node*>(decl_n_fn_ptr_params);
                    for (int i = 0; i < decl_n_fn_ptr_params; ++i) {
                        decl_fn_ptr_params[i] = pointed_fn_params[i];
                    }
                }
            }
        }
        if (!fn_returning_fptr) is_fptr_global = true;
    } else {
    parse_declarator(ts, &decl_name, &decl_fn_ptr_params, &decl_n_fn_ptr_params,
                     &decl_fn_ptr_variadic, &decl_ret_fn_ptr_params,
                     &decl_n_ret_fn_ptr_params, &decl_ret_fn_ptr_variadic);
    if (is_incomplete_object_type(ts) && !check(TokenKind::LParen))
        throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
    }

    // Explicit template specialization: parse <type_args> after function name.
    // e.g. template<> int add<int>(int a, int b) { ... }
    //                          ^^^^^ captured here
    std::vector<TypeSpec> spec_arg_types;
    std::vector<bool> spec_arg_is_value;
    std::vector<long long> spec_arg_values;
    if (parsing_explicit_specialization_ && decl_name && check(TokenKind::Less)) {
        consume();  // <
        while (!at_end() && !check(TokenKind::Greater)) {
            // Try to parse as type first (int, long, char, etc.)
            if (is_type_start()) {
                TypeSpec arg_ts = parse_type_name();
                spec_arg_types.push_back(arg_ts);
                spec_arg_is_value.push_back(false);
                spec_arg_values.push_back(0);
            } else if (check(TokenKind::IntLit) || check(TokenKind::CharLit) ||
                       (is_cpp_mode() &&
                        (check(TokenKind::KwTrue) || check(TokenKind::KwFalse))) ||
                       (check(TokenKind::Minus) && pos_ + 1 < (int)tokens_.size() &&
                        (tokens_[pos_ + 1].kind == TokenKind::IntLit ||
                         tokens_[pos_ + 1].kind == TokenKind::CharLit))) {
                // NTTP value
                long long sign = 1;
                if (match(TokenKind::Minus)) sign = -1;
                long long val = parse_int_lexeme(cur().lexeme.c_str());
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
                TypeSpec dummy{};
                dummy.array_size = -1;
                dummy.inner_rank = -1;
                spec_arg_types.push_back(dummy);
                spec_arg_is_value.push_back(true);
                spec_arg_values.push_back(val * sign);
            } else {
                break;
            }
            if (!match(TokenKind::Comma)) break;
        }
        expect(TokenKind::Greater);
    }

    parse_attributes(&ts);
    skip_asm();
    parse_attributes(&ts);

    if (!decl_name) {
        // No name: just a type declaration; skip to ;
        match(TokenKind::Semi);
        return make_node(NK_EMPTY, ln);
    }

    const char* scoped_decl_name = qualify_name_arena(decl_name);

    // Handle function-returning-fptr: int (* f1(a, b))(c, d) { body }
    // Params were already parsed into fptr_fn_params; now look for { body }.
    if (fn_returning_fptr && decl_name) {
        parse_attributes(&ts); skip_asm(); parse_attributes(&ts);
        if (check(TokenKind::LBrace)) {
            bool saved_top = parsing_top_level_context_;
            parsing_top_level_context_ = false;
            Node* body = parse_block();
            parsing_top_level_context_ = saved_top;
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = scoped_decl_name;
            apply_decl_namespace(fn, current_namespace_context_id(), decl_name);
            fn->variadic  = fptr_fn_variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->is_constexpr = is_constexpr;
            fn->is_consteval = is_consteval;
            fn->linkage_spec = linkage_spec;
            fn->visibility = visibility_;
            fn->body      = body;
            fn->n_params  = (int)fptr_fn_params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
            }
            // Phase C: propagate return type fn_ptr params.
            fn->ret_fn_ptr_params   = decl_fn_ptr_params;
            fn->n_ret_fn_ptr_params = decl_n_fn_ptr_params;
            fn->ret_fn_ptr_variadic = decl_fn_ptr_variadic;
            known_fn_names_.insert(scoped_decl_name);
            return fn;
        }
        // Function declaration (no body): consume semicolon and return
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = scoped_decl_name;
        apply_decl_namespace(fn, current_namespace_context_id(), decl_name);
        fn->variadic  = fptr_fn_variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->is_constexpr = is_constexpr;
        fn->is_consteval = is_consteval;
        fn->linkage_spec = linkage_spec;
        fn->visibility = visibility_;
        fn->body      = nullptr;
        fn->n_params  = (int)fptr_fn_params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
        }
        // Phase C: propagate return type fn_ptr params.
        fn->ret_fn_ptr_params   = decl_fn_ptr_params;
        fn->n_ret_fn_ptr_params = decl_n_fn_ptr_params;
        fn->ret_fn_ptr_variadic = decl_fn_ptr_variadic;
        known_fn_names_.insert(scoped_decl_name);
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
        parse_attributes(&ts);
        skip_asm();
        parse_attributes(&ts);

        // K&R style parameter declarations before body:
        //   f(a, b) int a; short *b; { ... }
        // Bind declared types back to identifier list order.
        if (!knr_param_names.empty()) {
            std::unordered_map<std::string, Node*> knr_param_decl_map;
            while (is_type_start() && !check(TokenKind::LBrace)) {
                TypeSpec knr_base = parse_base_type();
                parse_attributes(&knr_base);
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

        // Helper: attach explicit specialization args to NK_FUNCTION.
        auto attach_spec_args = [&](Node* fn) {
            if (spec_arg_types.empty()) return;
            int n = (int)spec_arg_types.size();
            fn->n_template_args = n;
            fn->template_arg_types = arena_.alloc_array<TypeSpec>(n);
            fn->template_arg_is_value = arena_.alloc_array<bool>(n);
            fn->template_arg_values = arena_.alloc_array<long long>(n);
            fn->template_arg_nttp_names = arena_.alloc_array<const char*>(n);
            for (int i = 0; i < n; ++i) {
                fn->template_arg_types[i] = spec_arg_types[i];
                fn->template_arg_is_value[i] = spec_arg_is_value[i];
                fn->template_arg_values[i] = spec_arg_values[i];
                fn->template_arg_nttp_names[i] = nullptr;
            }
        };

        // Phase C: helper lambda to propagate return type fn_ptr params to NK_FUNCTION.
        auto propagate_ret_fn_ptr = [&](Node* fn) {
            if (ts.is_fn_ptr && !ret_typedef_name.empty()) {
                auto tdit = typedef_fn_ptr_info_.find(ret_typedef_name);
                if (tdit != typedef_fn_ptr_info_.end()) {
                    fn->ret_fn_ptr_params   = tdit->second.params;
                    fn->n_ret_fn_ptr_params = tdit->second.n_params;
                    fn->ret_fn_ptr_variadic = tdit->second.variadic;
                }
            }
        };

        if (check(TokenKind::LBrace)) {
            // Function definition
            bool saved_top = parsing_top_level_context_;
            parsing_top_level_context_ = false;
            Node* body = parse_block();
            parsing_top_level_context_ = saved_top;
            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = scoped_decl_name;
            apply_decl_namespace(fn, current_namespace_context_id(), decl_name);
            fn->variadic  = variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->is_constexpr = is_constexpr;
            fn->is_consteval = is_consteval;
            fn->linkage_spec = linkage_spec;
            fn->visibility = visibility_;
            fn->body      = body;
            fn->n_params  = (int)params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }
            propagate_ret_fn_ptr(fn);
            attach_spec_args(fn);
            known_fn_names_.insert(scoped_decl_name);
            return fn;
        }

        // Function declaration only
        match(TokenKind::Semi);
        Node* fn = make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = scoped_decl_name;
        apply_decl_namespace(fn, current_namespace_context_id(), decl_name);
        fn->variadic  = variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->is_constexpr = is_constexpr;
        fn->is_consteval = is_consteval;
        fn->linkage_spec = linkage_spec;
        fn->visibility = visibility_;
        fn->body      = nullptr;  // declaration only
        fn->n_params  = (int)params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
        }
        propagate_ret_fn_ptr(fn);
        attach_spec_args(fn);
        known_fn_names_.insert(scoped_decl_name);
        return fn;
    }

    // Global variable (possibly with initializer and multiple declarators)
    std::vector<Node*> gvars;

    auto make_gvar = [&](TypeSpec gts, const char* gname, const char* source_name, Node* ginit,
                         Node** fn_ptr_params, int n_fn_ptr_params,
                         bool fn_ptr_variadic,
                         Node** ret_fn_ptr_params = nullptr,
                         int n_ret_fn_ptr_params = 0,
                         bool ret_fn_ptr_variadic = false) {
        // C++ consteval is only valid on functions, not variable declarations.
        if (is_consteval) {
            throw std::runtime_error(
                std::string("error: 'consteval' can only be applied to a function declaration (line ")
                + std::to_string(ln) + ")");
        }
        Node* gv = make_node(NK_GLOBAL_VAR, ln);
        gv->type      = gts;
        if (is_constexpr) gv->type.is_const = true;
        gv->name      = gname;
        apply_decl_namespace(gv, current_namespace_context_id(), source_name);
        gv->init      = ginit;
        gv->is_static = is_static;
        gv->is_extern = is_extern;
        gv->is_constexpr = is_constexpr;
        gv->is_consteval = false;
        gv->linkage_spec = linkage_spec;
        gv->visibility = visibility_;
        gv->fn_ptr_params = fn_ptr_params;
        gv->n_fn_ptr_params = n_fn_ptr_params;
        gv->fn_ptr_variadic = fn_ptr_variadic;
        gv->ret_fn_ptr_params = ret_fn_ptr_params;
        gv->n_ret_fn_ptr_params = n_ret_fn_ptr_params;
        gv->ret_fn_ptr_variadic = ret_fn_ptr_variadic;
        // Phase C: propagate fn_ptr params from typedef if not set by declarator.
        if (gts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
            auto tdit = typedef_fn_ptr_info_.find(last_resolved_typedef_);
            if (tdit != typedef_fn_ptr_info_.end()) {
                gv->fn_ptr_params = tdit->second.params;
                gv->n_fn_ptr_params = tdit->second.n_params;
                gv->fn_ptr_variadic = tdit->second.variadic;
            }
        }
        if (gname) var_types_[gname] = gts;  // for typeof(var) resolution
        return gv;
    };

    Node* first_init = nullptr;
    if (match(TokenKind::Assign) ||
        (is_cpp_mode() && check(TokenKind::LBrace))) {
        first_init = parse_initializer();
    }
    gvars.push_back(make_gvar(ts, scoped_decl_name, decl_name, first_init, decl_fn_ptr_params,
                              decl_n_fn_ptr_params, decl_fn_ptr_variadic,
                              decl_ret_fn_ptr_params, decl_n_ret_fn_ptr_params,
                              decl_ret_fn_ptr_variadic));

    while (match(TokenKind::Comma)) {
        TypeSpec ts2 = base_ts;
        // Preserve typedef ptr_level and array dims for each additional declarator.
        ts2.array_size_expr = nullptr;
        const char* n2 = nullptr;
        Node** fn_ptr_params2 = nullptr;
        int n_fn_ptr_params2 = 0;
        bool fn_ptr_variadic2 = false;
        parse_declarator(ts2, &n2, &fn_ptr_params2, &n_fn_ptr_params2, &fn_ptr_variadic2);
        skip_attributes();
        skip_asm();
        Node* init2 = nullptr;
        if (match(TokenKind::Assign) ||
            (is_cpp_mode() && check(TokenKind::LBrace))) init2 = parse_initializer();
        if (n2) {
            const char* scoped_n2 = qualify_name_arena(n2);
            gvars.push_back(make_gvar(ts2, scoped_n2, n2, init2,
                                          fn_ptr_params2, n_fn_ptr_params2,
                                          fn_ptr_variadic2));
        }
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

}  // namespace c4c

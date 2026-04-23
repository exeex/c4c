#include "parser.hpp"

#include <climits>
#include <sstream>
#include <cstring>
#include <stdexcept>

namespace c4c {

static void finalize_pending_operator_name(std::string& name, size_t param_count) {
    if (name.find("operator_star_pending") != std::string::npos) {
        name.replace(name.find("operator_star_pending"),
                     sizeof("operator_star_pending") - 1,
                     param_count == 0 ? "operator_deref" : "operator_mul");
    }
    if (name.find("operator_amp_pending") != std::string::npos) {
        name.replace(name.find("operator_amp_pending"),
                     sizeof("operator_amp_pending") - 1,
                     param_count == 0 ? "operator_addr" : "operator_bitand");
    }
    if (name.find("operator_inc_pending") != std::string::npos) {
        name.replace(name.find("operator_inc_pending"),
                     sizeof("operator_inc_pending") - 1,
                     param_count == 0 ? "operator_preinc" : "operator_postinc");
    }
    if (name.find("operator_dec_pending") != std::string::npos) {
        name.replace(name.find("operator_dec_pending"),
                     sizeof("operator_dec_pending") - 1,
                     param_count == 0 ? "operator_predec" : "operator_postdec");
    }
}

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

static bool template_param_expr_continues_after_greater(TokenKind k) {
    return k == TokenKind::ColonColon || k == TokenKind::PipePipe ||
           k == TokenKind::AmpAmp || k == TokenKind::Pipe ||
           k == TokenKind::Amp || k == TokenKind::Caret ||
           k == TokenKind::Plus || k == TokenKind::Minus ||
           k == TokenKind::Star || k == TokenKind::Slash ||
           k == TokenKind::Percent || k == TokenKind::EqualEqual ||
           k == TokenKind::BangEqual || k == TokenKind::LessEqual ||
           k == TokenKind::GreaterEqual || k == TokenKind::Question;
}

static void skip_template_param_default_expr(Parser& parser,
                                             bool track_brackets_and_braces,
                                             int* out_start_pos = nullptr) {
    if (out_start_pos) *out_start_pos = parser.core_input_state_.pos;

    auto is_expr_continuation = [&](int p) -> bool {
        if (p >= static_cast<int>(parser.core_input_state_.tokens.size())) return false;
        return template_param_expr_continues_after_greater(
            parser.core_input_state_.tokens[p].kind);
    };

    int depth = 0;
    while (!parser.at_end()) {
        if (parser.check(TokenKind::Less) || parser.check(TokenKind::LParen) ||
            (track_brackets_and_braces &&
             (parser.check(TokenKind::LBracket) || parser.check(TokenKind::LBrace)))) {
            ++depth;
        } else if (parser.check(TokenKind::Greater)) {
            if (depth == 0) {
                if (is_expr_continuation(parser.core_input_state_.pos + 1)) {
                    parser.consume();
                    continue;
                }
                break;
            }
            --depth;
        } else if (parser.check(TokenKind::RParen) ||
                   (track_brackets_and_braces &&
                    (parser.check(TokenKind::RBracket) || parser.check(TokenKind::RBrace)))) {
            if (depth > 0) --depth;
        } else if (parser.check(TokenKind::GreaterGreater)) {
            if (depth == 0) {
                if (is_expr_continuation(parser.core_input_state_.pos + 1)) {
                    parser.consume();
                    continue;
                }
                break;
            }
            if (depth == 1) {
                if (is_expr_continuation(parser.core_input_state_.pos + 1)) {
                    depth = 0;
                    parser.consume();
                    continue;
                }
                parser.parse_greater_than_in_template_list(false);
                break;
            }
            depth -= 2;
        } else if (parser.check(TokenKind::Comma) && depth == 0) {
            break;
        }
        parser.consume();
    }
}

namespace {

const char* maybe_parse_linkage_spec(Parser* parser) {
    if (!parser->is_cpp_mode() || !parser->check(TokenKind::StrLit)) return nullptr;
    if (parser->token_spelling(parser->cur()) == "\"C\"") {
        parser->consume();
        return "C";
    }
    if (parser->token_spelling(parser->cur()) == "\"C++\"") {
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

bool is_internal_typedef_name(const char* name) {
    return name && name[0] == '_' && name[1] == '_';
}

bool is_same_visible_type_name(Parser& parser, TextId lhs_text_id,
                               std::string_view lhs, TextId rhs_text_id,
                               std::string_view rhs) {
    if (lhs.empty() || rhs.empty()) return false;
    if (lhs == rhs) return true;
    return parser.resolve_visible_type_name(lhs_text_id, lhs) ==
           parser.resolve_visible_type_name(rhs_text_id, rhs);
}

bool is_cpp20_requires_clause_record_decl_boundary(TokenKind kind) {
    switch (kind) {
        case TokenKind::KwStruct:
        case TokenKind::KwClass:
        case TokenKind::KwUnion:
        case TokenKind::KwEnum:
            return true;
        default:
            return false;
    }
}

bool is_cpp20_requires_clause_decl_boundary(Parser& parser) {
    if (parser.at_end()) return true;
    TokenKind kind = parser.cur().kind;
    if (kind == TokenKind::Identifier) {
        if (parser.core_input_state_.pos > 0 &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos - 1].kind ==
                TokenKind::ColonColon) {
            return false;
        }
        if (parser.core_input_state_.pos + 1 <
                static_cast<int>(parser.core_input_state_.tokens.size()) &&
            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                TokenKind::Less) {
            return false;
        }
        return true;
    }
    if (kind == TokenKind::KwOperator ||
        kind == TokenKind::KwConstexpr || kind == TokenKind::KwConsteval ||
        kind == TokenKind::KwExplicit || kind == TokenKind::KwInline ||
        kind == TokenKind::KwStatic || kind == TokenKind::KwExtern ||
        kind == TokenKind::KwMutable || kind == TokenKind::KwVirtual ||
        kind == TokenKind::KwFriend || kind == TokenKind::KwTypedef ||
        kind == TokenKind::KwUsing || kind == TokenKind::KwConcept ||
        kind == TokenKind::KwStaticAssert || kind == TokenKind::KwStruct ||
        kind == TokenKind::KwClass || kind == TokenKind::KwUnion ||
        kind == TokenKind::KwEnum || kind == TokenKind::KwTypename ||
        kind == TokenKind::KwAuto || kind == TokenKind::KwAlignas) {
        return true;
    }
    if (is_type_kw(kind) || is_qualifier(kind) || is_storage_class(kind)) {
        return true;
    }
    return false;
}

bool try_skip_cpp_concept_declaration(Parser& parser) {
    if (!parser.is_cpp_mode() || !parser.check(TokenKind::KwConcept)) {
        return false;
    }

    parser.consume();  // concept
    if (!parser.check(TokenKind::Identifier)) {
        throw std::runtime_error("expected concept name");
    }
    const TextId concept_name_text_id =
        parser.parser_text_id_for_token(parser.cur().text_id,
                                        parser.token_spelling(parser.cur()));
    const std::string concept_name = std::string(parser.token_spelling(parser.cur()));
    parser.consume();  // concept identifier
    parser.expect(TokenKind::Assign);

    // Reuse the existing expression / requires-expression parser for the
    // constraint-expression on the right-hand side of `concept Name = ...;`.
    (void)parser.parse_expr();
    parser.expect(TokenKind::Semi);
    if (concept_name_text_id != kInvalidText) {
        parser.binding_state_.concept_name_text_ids.insert(concept_name_text_id);
    }
    parser.register_concept_name_in_context(
        parser.current_namespace_context_id(), concept_name_text_id, concept_name);
    const std::string qualified = parser.bridge_name_in_context(
        parser.current_namespace_context_id(), concept_name_text_id, concept_name);
    if (qualified != concept_name) {
        parser.binding_state_.concept_names.insert(qualified);
    }
    return true;
}

bool is_top_level_decl_recovery_boundary(TokenKind kind) {
    switch (kind) {
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
        case TokenKind::KwClass:
        case TokenKind::KwUnion:
        case TokenKind::KwEnum:
        case TokenKind::KwConst:
        case TokenKind::KwVolatile:
        case TokenKind::KwStatic:
        case TokenKind::KwExtern:
        case TokenKind::KwInline:
        case TokenKind::KwConstexpr:
        case TokenKind::KwConsteval:
        case TokenKind::KwTypedef:
        case TokenKind::KwAuto:
        case TokenKind::KwTemplate:
        case TokenKind::KwConcept:
        case TokenKind::KwNamespace:
        case TokenKind::KwUsing:
        case TokenKind::KwStaticAssert:
        case TokenKind::KwAsm:
        case TokenKind::PragmaPack:
        case TokenKind::PragmaWeak:
        case TokenKind::PragmaGccVisibility:
            return true;
        default:
            return false;
    }
}

bool recover_top_level_decl_terminator_or_boundary(Parser& parser, int recovery_line) {
    while (!parser.at_end() && !parser.check(TokenKind::Semi)) {
        if (parser.cur().line != recovery_line &&
            is_top_level_decl_recovery_boundary(parser.cur().kind)) {
            return false;
        }
        parser.consume();
    }
    if (parser.check(TokenKind::Semi)) {
        parser.consume();
        return true;
    }
    return false;
}

bool recover_top_level_using_alias_or_boundary(Parser& parser, int recovery_line) {
    int depth = 0;
    bool consumed_type_tokens = false;
    while (!parser.at_end()) {
        if (depth == 0 && parser.check(TokenKind::Semi)) {
            parser.consume();
            return true;
        }
        if (depth == 0 && consumed_type_tokens &&
            parser.cur().line != recovery_line &&
            is_top_level_decl_recovery_boundary(parser.cur().kind)) {
            return false;
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
    return false;
}

bool using_alias_consumed_following_declaration(Parser& parser,
                                                int alias_type_pos,
                                                int alias_type_end_pos,
                                                int recovery_line) {
    if (alias_type_end_pos <= alias_type_pos)
        return false;

    const int last_token_pos = alias_type_end_pos - 1;
    if (last_token_pos < alias_type_pos ||
        parser.core_input_state_.tokens[last_token_pos].kind !=
            TokenKind::Identifier ||
        parser.core_input_state_.tokens[last_token_pos].line == recovery_line) {
        return false;
    }

    int depth = 0;
    const int candidate_line = parser.core_input_state_.tokens[last_token_pos].line;
    for (int i = alias_type_pos; i < last_token_pos; ++i) {
        TokenKind kind = parser.core_input_state_.tokens[i].kind;
        if (kind == TokenKind::Less || kind == TokenKind::LParen ||
            kind == TokenKind::LBracket) {
            ++depth;
            continue;
        }
        if ((kind == TokenKind::Greater || kind == TokenKind::RParen ||
             kind == TokenKind::RBracket) &&
            depth > 0) {
            --depth;
            continue;
        }
        if (kind == TokenKind::GreaterGreater && depth > 0) {
            depth -= std::min(depth, 2);
            continue;
        }
        if (depth == 0 &&
            i > alias_type_pos &&
            parser.core_input_state_.tokens[i - 1].kind == TokenKind::ColonColon) {
            continue;
        }
        if (depth == 0 &&
            parser.core_input_state_.tokens[i].line == candidate_line &&
            is_top_level_decl_recovery_boundary(kind)) {
            return true;
        }
    }
    return false;
}

bool skip_top_level_asm_or_recover(Parser& parser) {
    if (!parser.check(TokenKind::LParen)) return false;

    parser.consume();  // (
    int depth = 1;
    while (!parser.at_end() && depth > 0) {
        if (depth == 1 &&
            is_top_level_decl_recovery_boundary(parser.cur().kind)) {
            return false;
        }
        if (parser.check(TokenKind::LParen)) {
            ++depth;
            parser.consume();
            continue;
        }
        if (parser.check(TokenKind::RParen)) {
            --depth;
            parser.consume();
            continue;
        }
        parser.consume();
    }
    return depth == 0;
}

bool template_owner_can_defer_static_assert(const Node* owner) {
    return owner && owner->n_template_params > 0;
}

bool skip_cpp20_constraint_atom(Parser& parser) {
    if (parser.at_end()) return false;

    if (parser.check(TokenKind::KwRequires)) {
        parser.consume();
        if (parser.check(TokenKind::LParen)) parser.skip_paren_group();
        if (!parser.check(TokenKind::LBrace))
            return false;
        parser.skip_brace_group();
        return true;
    }

    if (parser.check(TokenKind::LParen)) {
        parser.skip_paren_group();
        return true;
    }

    int angle_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    bool consumed = false;
    while (!parser.at_end()) {
        if (consumed && angle_depth == 0 && paren_depth == 0 &&
            bracket_depth == 0) {
            if (parser.check(TokenKind::AmpAmp) ||
                parser.check(TokenKind::PipePipe) ||
                parser.check(TokenKind::Semi) ||
                parser.check(TokenKind::Assign) ||
                parser.check(TokenKind::Comma) ||
                parser.check(TokenKind::LBrace) ||
                parser.check(TokenKind::Colon) ||
                is_cpp20_requires_clause_decl_boundary(parser)) {
                break;
            }
        }

        if (parser.check(TokenKind::Less)) {
            ++angle_depth;
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::Greater)) {
            if (angle_depth == 0) break;
            --angle_depth;
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::GreaterGreater)) {
            if (angle_depth == 0) break;
            angle_depth -= std::min(angle_depth, 2);
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::LParen)) {
            ++paren_depth;
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::RParen)) {
            if (paren_depth == 0) break;
            --paren_depth;
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::LBracket)) {
            ++bracket_depth;
            parser.consume();
            consumed = true;
            continue;
        }
        if (parser.check(TokenKind::RBracket)) {
            if (bracket_depth == 0) break;
            --bracket_depth;
            parser.consume();
            consumed = true;
            continue;
        }

        parser.consume();
        consumed = true;
    }
    return consumed;
}

void parse_optional_cpp20_requires_clause(Parser& parser) {
    if (!parser.is_cpp_mode() || !parser.check(TokenKind::KwRequires)) {
        return;
    }

    parser.consume();  // requires
    bool consumed_constraint = false;
    while (!parser.at_end()) {
        if (!skip_cpp20_constraint_atom(parser))
            break;
        consumed_constraint = true;
        if (parser.check(TokenKind::AmpAmp) ||
            parser.check(TokenKind::PipePipe)) {
            parser.consume();
            continue;
        }
        break;
    }

    if (!consumed_constraint) {
        if (is_cpp20_requires_clause_record_decl_boundary(parser.cur().kind)) {
            return;
        }
        throw std::runtime_error("expected constraint-expression after requires");
    }
}

void parse_optional_cpp20_trailing_requires_clause(Parser& parser) {
    if (!parser.is_cpp_mode() || !parser.check(TokenKind::KwRequires)) {
        return;
    }

    parser.consume();  // requires
    bool consumed_constraint = false;
    while (!parser.at_end()) {
        if (!skip_cpp20_constraint_atom(parser))
            break;
        consumed_constraint = true;
        if (parser.check(TokenKind::AmpAmp) ||
            parser.check(TokenKind::PipePipe)) {
            parser.consume();
            continue;
        }
        break;
    }

    if (!consumed_constraint) {
        throw std::runtime_error("expected constraint-expression after requires");
    }
}

}  // namespace

Node* Parser::parse_local_decl() {
    ParseContextGuard trace(this, __func__);
    int ln = cur().line;
    auto skip_cpp11_attrs_only = [&]() {
        while (check(TokenKind::LBracket) &&
               core_input_state_.pos + 1 <
                   static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
            consume();
            consume();
            int depth = 1;
            while (!at_end() && depth > 0) {
                if (check(TokenKind::LBracket) &&
                    core_input_state_.pos + 1 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::LBracket) {
                    consume();
                    consume();
                    ++depth;
                    continue;
                }
                if (check(TokenKind::RBracket) &&
                    core_input_state_.pos + 1 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::RBracket) {
                    consume();
                    consume();
                    --depth;
                    continue;
                }
                consume();
            }
        }
    };
    skip_cpp11_attrs_only();

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
        if (is_cpp_mode() && !active_context_state_.current_struct_tag.empty()) {
            if (is_same_visible_type_name(
                    *this, active_context_state_.current_struct_tag_text_id,
                    current_struct_tag_text(), find_parser_text_id(ts.tag),
                    ts.tag))
                return false;
        }
        auto it = definition_state_.struct_tag_def_map.find(ts.tag);
        if (it == definition_state_.struct_tag_def_map.end()) return true;
        Node* def = it->second;
        return !def || def->n_fields < 0;
    };

    if (is_typedef) {
        // Simple typedef: typedef base_type name[...];
        // Also handles typedef base_type (*fn_name)(...);
        const char* tdname = nullptr;
        TextId tdname_text_id = kInvalidText;
        TypeSpec ts_copy = base_ts;
        Node** td_fn_ptr_params = nullptr;
        int td_n_fn_ptr_params = 0;
        bool td_fn_ptr_variadic = false;
        parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic, nullptr,
                         nullptr, nullptr, nullptr, &tdname_text_id);
        if (tdname) {
            if (!is_cpp_mode() && !is_internal_typedef_name(tdname) &&
                has_conflicting_user_typedef_binding(tdname, ts_copy))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            register_typedef_binding(tdname, ts_copy,
                                     !is_internal_typedef_name(tdname));
            // Phase C: store fn_ptr param info for typedef'd function pointers.
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
                const TextId typedef_name_id =
                    tdname_text_id != kInvalidText
                        ? tdname_text_id
                        : parser_text_id_for_token(kInvalidText, tdname);
                if (typedef_name_id != kInvalidText) {
                    binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                        td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
                }
            }
        }
        // multiple declarators in typedef
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            TextId tdn2_text_id = kInvalidText;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic,
                             nullptr, nullptr, nullptr, nullptr,
                             &tdn2_text_id);
            if (tdn2) {
                if (!is_cpp_mode() && !is_internal_typedef_name(tdn2) &&
                    has_conflicting_user_typedef_binding(tdn2, ts2))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                register_typedef_binding(tdn2, ts2,
                                         !is_internal_typedef_name(tdn2));
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                    const TextId typedef_name_id =
                        tdn2_text_id != kInvalidText
                            ? tdn2_text_id
                            : parser_text_id_for_token(kInvalidText, tdn2);
                    if (typedef_name_id != kInvalidText) {
                        binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                            td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
                    }
                }
            }
        }
        match(TokenKind::Semi);
        // Keep enum definition nodes for `typedef enum { ... } T;` so
        // downstream semantic passes can bind enumerator constants.
        if (base_ts.base == TB_ENUM && definition_state_.last_enum_def)
            return definition_state_.last_enum_def;
        return make_node(NK_EMPTY, ln);
    }

    // declaration-only tag type in local scope: `enum/struct/union ... ;`
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        if (base_ts.base == TB_ENUM && definition_state_.last_enum_def) {
            return definition_state_.last_enum_def;
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
                         nullptr,
                         &ret_fn_ptr_params, &n_ret_fn_ptr_params, &ret_fn_ptr_variadic);
        // C++ constructor invocation: `Type var(args)` where Type is a struct.
        // In C mode this is a K&R function-type suffix that we skip.
        bool is_kr_fn_decl = false;
        bool is_ctor_init = false;
        std::vector<Node*> ctor_args;
        if (check(TokenKind::LParen)) {
            bool parsed_as_function_decl = false;
            if (is_cpp_mode() && vname) {
                bool single_value_arg = false;
                if (core_input_state_.pos + 2 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::Identifier &&
                    core_input_state_.tokens[core_input_state_.pos + 2].kind ==
                        TokenKind::RParen) {
                    const std::string arg_name = std::string(token_spelling(
                        core_input_state_.tokens[core_input_state_.pos + 1]));
                    const std::string resolved_type_name =
                        resolve_visible_type_name(arg_name);
                    const bool arg_is_type =
                        is_typedef_name(arg_name) ||
                        has_visible_typedef_type(arg_name) ||
                        definition_state_.struct_tag_def_map.count(arg_name) >
                            0 ||
                        definition_state_.struct_tag_def_map.count(
                            resolved_type_name) > 0;
                    single_value_arg = !arg_is_type;
                }
                auto can_use_lite_ctor_init_probe = [&]() -> bool {
                    if (core_input_state_.pos + 1 >=
                        static_cast<int>(core_input_state_.tokens.size())) {
                        return false;
                    }

                    switch (core_input_state_.tokens[core_input_state_.pos + 1].kind) {
                        case TokenKind::IntLit:
                        case TokenKind::FloatLit:
                        case TokenKind::CharLit:
                        case TokenKind::StrLit:
                        case TokenKind::KwTrue:
                        case TokenKind::KwFalse:
                        case TokenKind::KwNullptr:
                        case TokenKind::KwSizeof:
                        case TokenKind::KwAlignof:
                        case TokenKind::Plus:
                        case TokenKind::Minus:
                        case TokenKind::Bang:
                        case TokenKind::Tilde:
                        case TokenKind::Amp:
                        case TokenKind::Star:
                        case TokenKind::LParen:
                            return true;
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
                        case TokenKind::RParen:
                            return false;
                        case TokenKind::Identifier: {
                            const std::string arg_name = std::string(token_spelling(
                                core_input_state_.tokens[core_input_state_.pos + 1]));
                            const std::string resolved_type_name =
                                resolve_visible_type_name(arg_name);
                            const bool arg_is_type =
                                is_template_scope_type_param(arg_name) ||
                                is_typedef_name(arg_name) ||
                                has_visible_typedef_type(arg_name) ||
                                definition_state_.struct_tag_def_map.count(
                                    arg_name) > 0 ||
                                definition_state_.struct_tag_def_map.count(
                                    resolved_type_name) > 0;
                            if (arg_is_type) return false;
                            if (single_value_arg) return true;
                            if (core_input_state_.pos + 2 <
                                    static_cast<int>(core_input_state_.tokens.size()) &&
                                (core_input_state_.tokens[core_input_state_.pos + 2].kind ==
                                     TokenKind::ColonColon ||
                                 core_input_state_.tokens[core_input_state_.pos + 2].kind ==
                                     TokenKind::Less)) {
                                return false;
                            }
                            return true;
                        }
                        default:
                            return false;
                    }
                };

                auto probe_ctor_vs_function_decl = [&]() -> bool {
                    std::vector<Node*> probe_params;
                    std::vector<const char*> probe_knr_names;
                    bool probe_variadic = false;
                    parse_top_level_parameter_list(
                        &probe_params, &probe_knr_names, &probe_variadic);
                    if (single_value_arg ||
                        (!check(TokenKind::Semi) && !check(TokenKind::Comma))) {
                        return false;
                    }
                    return !(is_cpp_mode() && !probe_knr_names.empty());
                };

                try {
                    if (can_use_lite_ctor_init_probe()) {
                        TentativeParseGuardLite guard(*this);
                        parsed_as_function_decl = probe_ctor_vs_function_decl();
                    } else {
                        TentativeParseGuard guard(*this);
                        parsed_as_function_decl = probe_ctor_vs_function_decl();
                    }
                } catch (const std::exception&) {
                }
            }
            if (is_cpp_mode() && vname && !parsed_as_function_decl) {
                // C++ direct-initialization: parse constructor-style arguments.
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

        if (is_incomplete_object_type(ts)) {
            throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
        }

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
            if (const FnPtrTypedefInfo* info = find_current_typedef_fn_ptr_info()) {
                d->fn_ptr_params = info->params;
                d->n_fn_ptr_params = info->n_params;
                d->fn_ptr_variadic = info->variadic;
            }
        }
        if (vname && !active_context_state_.suppress_local_var_bindings) {
            register_var_type_binding(vname, ts);
        }
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

void Parser::parse_top_level_parameter_list(
    std::vector<Node*>* out_params,
    std::vector<const char*>* out_knr_param_names,
    bool* out_variadic) {
    ParseContextGuard trace(this, __func__);
    if (out_params) out_params->clear();
    if (out_knr_param_names) out_knr_param_names->clear();
    if (out_variadic) *out_variadic = false;

    expect(TokenKind::LParen);
    if (!check(TokenKind::RParen)) {
        while (!at_end()) {
            if (check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                consume();
                break;
            }
            if (check(TokenKind::RParen)) break;
            if (!is_type_start() && !can_start_parameter_type() &&
                check(TokenKind::Identifier)) {
                if (out_knr_param_names)
                    out_knr_param_names->push_back(
                        arena_.strdup(std::string(token_spelling(cur()))));
                consume();
                if (match(TokenKind::Comma)) continue;
                break;
            }
            if (!can_start_parameter_type()) break;
            Node* p = parse_param();
            if (p && out_params) out_params->push_back(p);
            if (check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                consume();
                break;
            }
            if (!match(TokenKind::Comma)) break;
        }
    }
    expect(TokenKind::RParen);
}

Node* Parser::parse_static_assert_declaration() {
    const int ln = cur().line;
    consume();  // static_assert / _Static_assert
    expect(TokenKind::LParen);
    Node* cond = parse_assign_expr();
    Node* msg = nullptr;
    if (match(TokenKind::Comma)) {
        msg = parse_assign_expr();
    }
    expect(TokenKind::RParen);
    match(TokenKind::Semi);
    Node* sa = make_node(NK_STATIC_ASSERT, ln);
    sa->left = cond;
    sa->right = msg;
    return sa;
}

Node* Parser::parse_top_level() {
    ParseContextGuard trace(this, __func__);
    struct TopLevelFlagGuard {
        bool& ref;
        bool old;
        explicit TopLevelFlagGuard(bool& r) : ref(r), old(r) { ref = true; }
        ~TopLevelFlagGuard() { ref = old; }
    } top_level_guard(active_context_state_.parsing_top_level_context);

    int ln = cur().line;
    if (at_end()) return nullptr;
    auto skip_cpp11_attrs_only = [&]() -> bool {
        bool consumed_any = false;
        while (check(TokenKind::LBracket) &&
               core_input_state_.pos + 1 <
                   static_cast<int>(core_input_state_.tokens.size()) &&
               core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
            consumed_any = true;
            consume();
            consume();
            int depth = 1;
            while (!at_end() && depth > 0) {
                if (check(TokenKind::LBracket) &&
                    core_input_state_.pos + 1 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::LBracket) {
                    consume();
                    consume();
                    ++depth;
                    continue;
                }
                if (check(TokenKind::RBracket) &&
                    core_input_state_.pos + 1 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::RBracket) {
                    consume();
                    consume();
                    --depth;
                    continue;
                }
                consume();
            }
        }
        return consumed_any;
    };
    if (skip_cpp11_attrs_only() && check(TokenKind::Semi)) {
        consume();
        return nullptr;
    }

    bool is_inline_namespace = false;
    if (is_cpp_mode() && check(TokenKind::KwInline) && peek(1).kind == TokenKind::KwNamespace) {
        is_inline_namespace = true;
        consume();
    }
    if (is_cpp_mode() && check(TokenKind::KwNamespace)) {
        consume();
        QualifiedNameRef ns_name;
        bool has_name = false;
        if (check(TokenKind::Identifier)) {
            ns_name = parse_qualified_name(false);
            has_name = true;
        }
        skip_attributes();
        expect(TokenKind::LBrace);

        int context_id = current_namespace_context_id();
        if (has_name) {
            for (size_t i = 0; i < ns_name.qualifier_segments.size(); ++i) {
                const TextId segment_text_id =
                    i < ns_name.qualifier_text_ids.size()
                        ? ns_name.qualifier_text_ids[i]
                        : kInvalidText;
                context_id = ensure_named_namespace_context(
                    context_id, segment_text_id, ns_name.qualifier_segments[i]);
            }
            context_id = ensure_named_namespace_context(
                context_id, ns_name.base_text_id, ns_name.base_name);
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

        if (items.empty()) return nullptr;
        if (items.size() == 1) return items[0];
        Node* blk = make_node(NK_BLOCK, ln);
        blk->n_children = (int)items.size();
        blk->children = arena_.alloc_array<Node*>(blk->n_children);
        for (int i = 0; i < blk->n_children; ++i) blk->children[i] = items[i];
        return blk;
    }
    if (is_inline_namespace) {
        // We only consumed 'inline' for the `inline namespace` form above.
        throw std::runtime_error("expected namespace after inline");
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
            namespace_state_.using_namespace_contexts[using_context_id].push_back(target_context);
            recover_top_level_decl_terminator_or_boundary(*this, ln);
            return nullptr;
        }

        if (!check(TokenKind::Identifier) && !check(TokenKind::ColonColon))
            throw std::runtime_error("expected identifier after 'using'");

        QualifiedNameRef target_name;
        if (match(TokenKind::ColonColon)) {
            target_name.is_global_qualified = true;
            if (!check(TokenKind::Identifier))
                throw std::runtime_error("expected identifier after '::'");
            target_name.base_name = std::string(token_spelling(cur()));
            target_name.base_text_id =
                parser_text_id_for_token(cur().text_id, target_name.base_name);
            consume();
        } else {
            std::string first_name = std::string(token_spelling(cur()));
            consume();

            if (match(TokenKind::Assign)) {
                const int alias_type_pos = pos_;
                TypeSpec alias_ts{};
                try {
                    alias_ts = parse_type_name();
                    if (!alias_ts.tpl_struct_origin &&
                        alias_type_pos < core_input_state_.pos &&
                        core_input_state_.tokens[alias_type_pos].kind ==
                            TokenKind::KwTypename) {
                        auto skip_balanced_template_args = [&](int* pos) -> bool {
                            if (!pos ||
                                *pos < 0 ||
                                *pos >=
                                    static_cast<int>(core_input_state_.tokens.size()) ||
                                core_input_state_.tokens[*pos].kind != TokenKind::Less) {
                                return false;
                            }
                            int depth = 0;
                            while (*pos < static_cast<int>(core_input_state_.tokens.size())) {
                                const TokenKind kind = core_input_state_.tokens[*pos].kind;
                                if (kind == TokenKind::Less) {
                                    ++depth;
                                    ++(*pos);
                                    continue;
                                }
                                if (kind == TokenKind::Greater) {
                                    --depth;
                                    ++(*pos);
                                    if (depth <= 0) return true;
                                    continue;
                                }
                                if (kind == TokenKind::GreaterGreater) {
                                    depth -= 2;
                                    ++(*pos);
                                    if (depth <= 0) return true;
                                    continue;
                                }
                                ++(*pos);
                            }
                            return false;
                        };
                        auto join_token_lexemes = [&](int start, int end) -> std::string {
                            std::string out;
                            for (int i = start; i < end; ++i) {
                                out += token_spelling(core_input_state_.tokens[i]);
                            }
                            return out;
                        };
                        auto split_template_arg_token_ranges =
                            [&](int start, int end) -> std::vector<std::pair<int, int>> {
                            std::vector<std::pair<int, int>> ranges;
                            int arg_start = start;
                            int angle_depth = 0;
                            int paren_depth = 0;
                            int bracket_depth = 0;
                            int brace_depth = 0;
                            for (int i = start; i < end; ++i) {
                                switch (core_input_state_.tokens[i].kind) {
                                    case TokenKind::Less: ++angle_depth; break;
                                    case TokenKind::Greater:
                                        if (angle_depth > 0) --angle_depth;
                                        break;
                                    case TokenKind::GreaterGreater:
                                        angle_depth = std::max(0, angle_depth - 2);
                                        break;
                                    case TokenKind::LParen: ++paren_depth; break;
                                    case TokenKind::RParen:
                                        if (paren_depth > 0) --paren_depth;
                                        break;
                                    case TokenKind::LBracket: ++bracket_depth; break;
                                    case TokenKind::RBracket:
                                        if (bracket_depth > 0) --bracket_depth;
                                        break;
                                    case TokenKind::LBrace: ++brace_depth; break;
                                    case TokenKind::RBrace:
                                        if (brace_depth > 0) --brace_depth;
                                        break;
                                    case TokenKind::Comma:
                                        if (angle_depth == 0 && paren_depth == 0 &&
                                            bracket_depth == 0 && brace_depth == 0) {
                                            ranges.push_back({arg_start, i});
                                            arg_start = i + 1;
                                        }
                                        break;
                                    default:
                                        break;
                                }
                            }
                            if (arg_start < end) ranges.push_back({arg_start, end});
                            return ranges;
                        };
                        std::function<std::string(int, int)> encode_template_arg_ref_tokens =
                            [&](int start, int end) -> std::string {
                            while (start < end &&
                                   (core_input_state_.tokens[start].kind ==
                                        TokenKind::KwTypename ||
                                    core_input_state_.tokens[start].kind ==
                                        TokenKind::KwClass)) {
                                ++start;
                            }
                            if (start >= end) return {};

                            for (int i = start; i < end; ++i) {
                                if (core_input_state_.tokens[i].kind != TokenKind::Less) continue;
                                int probe = i;
                                if (!skip_balanced_template_args(&probe) || probe > end) {
                                    break;
                                }

                                std::string owner = join_token_lexemes(start, i);
                                std::string inner_refs;
                                const auto inner_ranges =
                                    split_template_arg_token_ranges(i + 1, probe - 1);
                                for (const auto& [arg_start, arg_end] : inner_ranges) {
                                    const std::string part =
                                        encode_template_arg_ref_tokens(arg_start, arg_end);
                                    if (part.empty()) continue;
                                    if (!inner_refs.empty()) inner_refs += ",";
                                    inner_refs += part;
                                }
                                return "@" + owner + ":" + inner_refs;
                            }

                            if (core_input_state_.tokens[start].kind == TokenKind::IntLit ||
                                core_input_state_.tokens[start].kind == TokenKind::FloatLit ||
                                core_input_state_.tokens[start].kind == TokenKind::CharLit ||
                                core_input_state_.tokens[start].kind == TokenKind::KwTrue ||
                                core_input_state_.tokens[start].kind == TokenKind::KwFalse) {
                                return join_token_lexemes(start, end);
                            }

                            std::string prefix;
                            while (start < end &&
                                   (core_input_state_.tokens[start].kind == TokenKind::KwConst ||
                                    core_input_state_.tokens[start].kind ==
                                        TokenKind::KwVolatile)) {
                                if (core_input_state_.tokens[start].kind == TokenKind::KwConst)
                                    prefix += "const_";
                                if (core_input_state_.tokens[start].kind ==
                                    TokenKind::KwVolatile)
                                    prefix += "volatile_";
                                ++start;
                            }

                            std::string base;
                            while (start < end &&
                                   (core_input_state_.tokens[start].kind ==
                                        TokenKind::Identifier ||
                                    core_input_state_.tokens[start].kind ==
                                        TokenKind::ColonColon)) {
                                base += token_spelling(core_input_state_.tokens[start]);
                                ++start;
                            }
                            if (base.empty()) return join_token_lexemes(start, end);

                            std::string suffix;
                            for (int i = start; i < end; ++i) {
                                if (core_input_state_.tokens[i].kind == TokenKind::Star)
                                    suffix += "_ptr";
                                if (core_input_state_.tokens[i].kind == TokenKind::Amp)
                                    suffix += "_ref";
                                if (core_input_state_.tokens[i].kind == TokenKind::AmpAmp)
                                    suffix += "_rref";
                            }
                            return prefix + base + suffix;
                        };

                        int probe = alias_type_pos + 1;  // skip typename
                        bool owner_global_qualified = false;
                        if (probe < core_input_state_.pos &&
                            core_input_state_.tokens[probe].kind ==
                                TokenKind::ColonColon) {
                            owner_global_qualified = true;
                            ++probe;
                        }
                        std::vector<std::string> qualifier_segments;
                        std::vector<TextId> qualifier_text_ids;
                        int owner_template_arg_start = -1;
                        int owner_template_arg_end = -1;
                        std::string member_name;
                        while (probe < core_input_state_.pos) {
                            if (core_input_state_.tokens[probe].kind != TokenKind::Identifier)
                                break;
                            const Token& segment_token = core_input_state_.tokens[probe];
                            const std::string segment =
                                std::string(token_spelling(segment_token));
                            const TextId segment_text_id =
                                parser_text_id_for_token(segment_token.text_id, segment);
                            ++probe;
                            if (probe < core_input_state_.pos &&
                                core_input_state_.tokens[probe].kind == TokenKind::Less) {
                                owner_template_arg_start = probe;
                                if (!skip_balanced_template_args(&probe)) break;
                                owner_template_arg_end = probe;
                            }
                            if (probe < core_input_state_.pos &&
                                core_input_state_.tokens[probe].kind ==
                                    TokenKind::ColonColon) {
                                ++probe;
                                if (probe < core_input_state_.pos &&
                                    core_input_state_.tokens[probe].kind ==
                                        TokenKind::KwTemplate) {
                                    ++probe;
                                }
                                if (probe < core_input_state_.pos &&
                                    core_input_state_.tokens[probe].kind ==
                                        TokenKind::Identifier) {
                                    qualifier_segments.push_back(segment);
                                    qualifier_text_ids.push_back(segment_text_id);
                                    continue;
                                }
                            }
                            member_name = segment;
                            break;
                        }

                        if (!qualifier_segments.empty() &&
                            owner_template_arg_start >= 0 &&
                            owner_template_arg_end > owner_template_arg_start &&
                            !member_name.empty()) {
                            QualifiedNameRef owner_qn;
                            owner_qn.is_global_qualified = owner_global_qualified;
                            owner_qn.qualifier_segments.assign(
                                qualifier_segments.begin(),
                                qualifier_segments.end() - 1);
                            owner_qn.qualifier_text_ids.assign(
                                qualifier_text_ids.begin(),
                                qualifier_text_ids.end() - 1);
                            owner_qn.base_name = qualifier_segments.back();
                            owner_qn.base_text_id = qualifier_text_ids.back();

                            std::string owner_name =
                                resolve_qualified_type_name(owner_qn);
                            if (owner_name.empty()) {
                                owner_name = resolve_visible_type_name(
                                    owner_qn.base_text_id, owner_qn.base_name);
                            }

                            std::string owner_arg_refs;
                            const auto owner_ranges = split_template_arg_token_ranges(
                                owner_template_arg_start + 1,
                                owner_template_arg_end - 1);
                            for (const auto& [arg_start, arg_end] : owner_ranges) {
                                const std::string ref =
                                    encode_template_arg_ref_tokens(arg_start, arg_end);
                                if (ref.empty()) continue;
                                if (!owner_arg_refs.empty()) owner_arg_refs += ",";
                                owner_arg_refs += ref;
                            }

                            alias_ts = {};
                            alias_ts.array_size = -1;
                            alias_ts.inner_rank = -1;
                            alias_ts.base = TB_STRUCT;
                            alias_ts.tag = arena_.strdup(owner_name.c_str());
                            alias_ts.tpl_struct_origin = alias_ts.tag;
                            if (!owner_arg_refs.empty()) {
                                alias_ts.tpl_struct_args.data =
                                    arena_.alloc_array<TemplateArgRef>(1);
                                alias_ts.tpl_struct_args.size = 1;
                                alias_ts.tpl_struct_args.data[0].kind =
                                    TemplateArgKind::Type;
                                alias_ts.tpl_struct_args.data[0].type = {};
                                alias_ts.tpl_struct_args.data[0].value = 0;
                                alias_ts.tpl_struct_args.data[0].debug_text =
                                    arena_.strdup(owner_arg_refs.c_str());
                            }
                            alias_ts.deferred_member_type_name =
                                arena_.strdup(member_name.c_str());
                        }
                    }
                } catch (const std::exception&) {
                    pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(*this, ln);
                    return nullptr;
                }
                if (using_alias_consumed_following_declaration(*this,
                                                               alias_type_pos,
                                                               pos_,
                                                               ln)) {
                    pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(*this, ln);
                    return nullptr;
                }
                if (!match(TokenKind::Semi)) {
                    pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(*this, ln);
                    return nullptr;
                }
                const TextId alias_name_text_id =
                    parser_text_id_for_token(kInvalidText, first_name);
                const std::string qualified = bridge_name_in_context(
                    using_context_id, alias_name_text_id, first_name);
                register_structured_typedef_binding_in_context(
                    using_context_id, alias_name_text_id, first_name, alias_ts);
                register_typedef_binding(qualified, alias_ts, true);
                if (using_context_id == 0) {
                    register_typedef_binding(first_name, alias_ts, true);
                }
                set_last_using_alias_name(qualified);
                return nullptr;
            }
            target_name.base_name = std::move(first_name);
            target_name.base_text_id =
                parser_text_id_for_token(kInvalidText, target_name.base_name);
        }
        while (match(TokenKind::ColonColon)) {
            if (!check(TokenKind::Identifier))
                throw std::runtime_error("expected identifier after '::'");
            target_name.qualifier_segments.push_back(target_name.base_name);
            target_name.qualifier_text_ids.push_back(target_name.base_text_id);
            target_name.base_name = std::string(token_spelling(cur()));
            target_name.base_text_id =
                parser_text_id_for_token(cur().text_id, target_name.base_name);
            consume();
        }

        const std::string imported_name =
            std::string(parser_text(target_name.base_text_id, target_name.base_name));
        const std::string imported_type_name =
            resolve_qualified_type_name(target_name);
        if (!imported_type_name.empty()) {
            if (const TypeSpec* imported_typedef =
                    find_typedef_type(imported_type_name)) {
                const std::string imported_key = bridge_name_in_context(
                    using_context_id, target_name.base_text_id, imported_name);
                register_structured_typedef_binding_in_context(
                    using_context_id, target_name.base_text_id, imported_name,
                    *imported_typedef);
                register_typedef_binding(imported_key, *imported_typedef, true);
                if (using_context_id == 0) {
                    register_typedef_binding(imported_name, *imported_typedef, true);
                }
                recover_top_level_decl_terminator_or_boundary(*this, ln);
                return nullptr;
            }
        }

        const QualifiedNameKey imported_value_key = qualified_name_key(target_name);
        const std::string imported_key = bridge_name_in_context(
            using_context_id, target_name.base_text_id, imported_name);
        std::string imported_value_name;
        if (shared_lookup_state_.token_texts) {
            imported_value_name = render_qualified_name(
                imported_value_key, shared_lookup_state_.parser_name_paths,
                *shared_lookup_state_.token_texts);
        }
        const TypeSpec* imported_var = nullptr;
        if (!imported_value_name.empty()) {
            imported_var = find_var_type(imported_value_name);
        }
        if (!imported_var) {
            imported_value_name = resolve_qualified_value_name(target_name);
            if (imported_value_name.empty()) {
                imported_value_name = qualified_name_text(target_name);
            }
            if (imported_value_name.empty()) {
                imported_value_name = imported_key;
                if (imported_value_name.rfind("::", 0) == 0) {
                    imported_value_name.erase(0, 2);
                }
            }
            imported_var = find_var_type(imported_value_name);
        }
        {
            if (imported_var) register_var_type_binding(imported_key, *imported_var);
            if (has_known_fn_name(imported_value_key)) {
                register_known_fn_name(known_fn_name_key_in_context(
                    using_context_id, target_name.base_text_id, imported_name));
            }
            namespace_state_.using_value_aliases[using_context_id]
                                               [target_name.base_text_id] = {
                imported_value_key, imported_value_name};
        }
        recover_top_level_decl_terminator_or_boundary(*this, ln);
        return nullptr;
    }

    if (is_cpp_mode() && check(TokenKind::KwTemplate)) {
        consume();
        expect(TokenKind::Less);

        // Explicit template specialization: template<> RetType name<Args>(...) { ... }
        if (check(TokenKind::Greater)) {
            consume();  // consume >
            bool saved_spec =
                active_context_state_.parsing_explicit_specialization;
            active_context_state_.parsing_explicit_specialization = true;
            size_t struct_defs_before = definition_state_.struct_defs.size();
            Node* spec = parse_top_level();
            active_context_state_.parsing_explicit_specialization = saved_spec;
            if (definition_state_.struct_defs.size() > struct_defs_before) {
                Node* last_sd = definition_state_.struct_defs.back();
                if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                    last_sd->template_origin_name && last_sd->n_template_args > 0) {
                    register_template_struct_specialization(
                        last_sd->template_origin_name, last_sd);
                }
            }
            if (spec) spec->is_explicit_specialization = true;
            return spec;
        }

        TemplateDeclarationPreludeGuard template_prelude_guard(this);
        std::vector<const char*> template_params;
        std::vector<bool> template_param_nttp;  // true if non-type template param
        std::vector<bool> template_param_is_pack;
        std::vector<bool> template_param_has_default;
        std::vector<TypeSpec> template_param_default_types;
        std::vector<long long> template_param_default_values;
        std::vector<const char*> template_param_default_exprs;
        // Deferred NTTP default expression tokens per parameter index.
        std::unordered_map<int, std::vector<Token>> deferred_nttp_defaults;
        auto push_type_template_param =
            [&](const char* pname, bool is_pack = false,
                bool has_default = false, const TypeSpec* default_ts = nullptr) {
                if (!pname || !pname[0]) {
                    pname = make_anon_template_param_name(arena_, false, template_params.size());
                }
                if (pname && pname[0]) {
                    register_synthesized_typedef_binding(pname);
                    template_prelude_guard.injected_type_params.emplace_back(pname);
                }
                template_params.push_back(pname);
                template_param_nttp.push_back(false);
                template_param_is_pack.push_back(is_pack);
                if (has_default && default_ts) {
                    template_param_has_default.push_back(true);
                    template_param_default_types.push_back(*default_ts);
                } else {
                    template_param_has_default.push_back(false);
                    TypeSpec dummy{};
                    dummy.array_size = -1;
                    dummy.inner_rank = -1;
                    template_param_default_types.push_back(dummy);
                }
                template_param_default_values.push_back(0);
                template_param_default_exprs.push_back(nullptr);
            };
        auto push_nttp_no_default = [&]() {
            template_param_has_default.push_back(false);
            TypeSpec dummy{};
            dummy.array_size = -1;
            dummy.inner_rank = -1;
            template_param_default_types.push_back(dummy);
            template_param_default_values.push_back(0);
            template_param_default_exprs.push_back(nullptr);
        };
        auto push_nttp_param = [&](const char* pname, bool is_pack = false) {
            if (!pname || !pname[0]) {
                pname = make_anon_template_param_name(arena_, true, template_params.size());
            }
            template_params.push_back(pname);
            template_param_nttp.push_back(true);
            template_param_is_pack.push_back(is_pack);
            return pname;
        };
        auto push_nttp_default_value = [&](long long value) {
            template_param_has_default.push_back(true);
            TypeSpec dummy{};
            dummy.array_size = -1;
            dummy.inner_rank = -1;
            template_param_default_types.push_back(dummy);
            template_param_default_values.push_back(value);
            template_param_default_exprs.push_back(nullptr);
        };
        auto push_nttp_deferred_default =
            [&](int param_idx, int start_pos, long long fallback_value = LLONG_MIN) {
                std::vector<Token> saved_toks(tokens_.begin() + start_pos,
                                             tokens_.begin() + pos_);
                TypeSpec dummy{};
                dummy.array_size = -1;
                dummy.inner_rank = -1;
                template_param_default_types.push_back(dummy);
                template_param_default_values.push_back(fallback_value);
                if (!saved_toks.empty()) {
                    deferred_nttp_defaults[param_idx] = std::move(saved_toks);
                    template_param_has_default.push_back(true);
                    std::ostringstream expr;
                    for (size_t tok_i = 0; tok_i < deferred_nttp_defaults[param_idx].size(); ++tok_i) {
                        if (tok_i > 0) expr << ' ';
                        expr << token_spelling(deferred_nttp_defaults[param_idx][tok_i]);
                    }
                    template_param_default_exprs.push_back(arena_.strdup(expr.str()));
                } else {
                    template_param_has_default.push_back(false);
                    template_param_default_exprs.push_back(nullptr);
                }
            };
        auto parse_template_param_pack_and_name = [&]() {
            bool is_pack = false;
            if (check(TokenKind::Ellipsis)) { consume(); is_pack = true; }
            const char* pname = nullptr;
            if (check(TokenKind::Identifier)) {
                pname = arena_.strdup(std::string(token_spelling(cur())));
                consume();
            }
            return std::pair<bool, const char*>(is_pack, pname);
        };
        auto try_consume_typedef_nttp_type_head = [&]() -> bool {
            if (is_cpp_mode()) {
                TentativeParseGuard guard(*this);
                std::string head_name;
                if (consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &head_name, nullptr) &&
                    is_concept_name(head_name)) {
                    return false;
                }
            }
            return consume_template_parameter_type_start(/*allow_typename_keyword=*/true);
        };
        while (!at_end() && !check(TokenKind::Greater)) {
            if (check(TokenKind::KwTemplate)) {
                // Template-template parameter:
                //   template<typename...> class Op
                // For the current frontend, treat the declared name as a
                // dependent type parameter and only consume the nested
                // template parameter clause plus any default argument.
                consume();  // template
                expect(TokenKind::Less);
                int nested_depth = 1;
                while (!at_end() && nested_depth > 0) {
                    if (check(TokenKind::Less)) {
                        ++nested_depth;
                        consume();
                        continue;
                    }
                    if (check_template_close()) {
                        match_template_close();
                        --nested_depth;
                        if (nested_depth == 0) break;
                        continue;
                    }
                    consume();
                }
                if (!check(TokenKind::KwTypename) &&
                    !check(TokenKind::KwClass)) {
                    throw std::runtime_error("expected template parameter name");
                }
                consume();  // typename/class
                auto [is_pack, pname] = parse_template_param_pack_and_name();
                if (match(TokenKind::Assign)) {
                    skip_template_param_default_expr(*this, true);
                }
                push_type_template_param(pname, is_pack);
            } else if (check(TokenKind::KwTypename) ||
                check(TokenKind::KwClass)) {
                if (classify_typename_template_parameter() ==
                    TypenameTemplateParamKind::TypedNonTypeParameter) {
                    TypeSpec param_ts = parse_type_name();
                    (void)param_ts;
                    while (check(TokenKind::Star) || is_qualifier(cur().kind)) consume();
                    auto [is_pack, pname] = parse_template_param_pack_and_name();
                    push_nttp_param(pname, is_pack);
                    if (match(TokenKind::Assign)) {
                        skip_template_param_default_expr(*this, false);
                    }
                    push_nttp_no_default();
                    continue;
                }
                // Type template parameter (possibly variadic: typename... Ts).
                consume();
                auto [is_pack, pname] = parse_template_param_pack_and_name();
                // Check for default type argument: = type
                if (match(TokenKind::Assign)) {
                    TypeSpec def_ts = parse_type_name();
                    push_type_template_param(pname, is_pack, true, &def_ts);
                } else {
                    push_type_template_param(pname, is_pack);
                }
            } else if (is_type_kw(cur().kind)) {
                // Non-type template parameter (NTTP): e.g. int N, unsigned N, bool... Bn.
                // Consume the type specifier tokens, then the parameter name.
                while (!at_end() && is_type_kw(cur().kind)) consume();
                auto [is_pack, pname] = parse_template_param_pack_and_name();
                push_nttp_param(pname, is_pack);
                // Check for default NTTP value: = expr
                if (match(TokenKind::Assign)) {
                    long long sign = 1;
                    if (match(TokenKind::Minus)) sign = -1;
                    if (check(TokenKind::KwTrue) || check(TokenKind::KwFalse)) {
                        long long val = check(TokenKind::KwTrue) ? 1 : 0;
                        consume();
                        push_nttp_default_value(val);
                    } else if (check(TokenKind::IntLit)) {
                        long long val =
                            parse_int_lexeme(std::string(token_spelling(cur())).c_str());
                        consume();
                        push_nttp_default_value(val * sign);
                    } else {
                        // Complex default expression — skip balanced tokens.
                        // Save the tokens for deferred evaluation during instantiation.
                        // A > at depth 0 closes the template param list UNLESS
                        // followed by :: or a binary operator (the expression continues).
                        int start_pos = pos_;
                        skip_template_param_default_expr(*this, false, &start_pos);
                        // Save tokens for deferred evaluation (LLONG_MIN sentinel).
                        int param_idx = static_cast<int>(template_params.size()) - 1;
                        push_nttp_deferred_default(param_idx, start_pos);
                    }
                } else {
                    push_nttp_no_default();
                }
            } else if (try_consume_typedef_nttp_type_head()) {
                // Non-type template parameter with typedef type
                // (e.g. size_t N, std::size_t N).
                // Skip pointer stars or qualifiers.
                while (check(TokenKind::Star) || is_qualifier(cur().kind)) consume();
                auto [is_pack, pname] = parse_template_param_pack_and_name();
                push_nttp_param(pname, is_pack);
                // Check for default NTTP value: = expr
                if (match(TokenKind::Assign)) {
                    skip_template_param_default_expr(*this, false);
                }
                push_nttp_no_default();
            } else if (is_cpp_mode()) {
                TentativeParseGuard guard(*this);
                std::string constraint_name;
                if (consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &constraint_name, nullptr) &&
                    (check(TokenKind::Ellipsis) || check(TokenKind::Identifier))) {
                    // C++20 constrained type parameter:
                    //   template<Constraint T>
                    //   template<ns::Constraint T = int>
                    auto [is_pack, pname] = parse_template_param_pack_and_name();
                    if (match(TokenKind::Assign)) {
                        TypeSpec def_ts = parse_type_name();
                        push_type_template_param(pname, is_pack, true, &def_ts);
                    } else {
                        push_type_template_param(pname, is_pack);
                    }
                    guard.commit();
                } else {
                    // guard restores pos_ on scope exit
                    throw std::runtime_error("expected template parameter name");
                }
            } else {
                throw std::runtime_error("expected template parameter name");
            }
            if (!match(TokenKind::Comma)) break;
        }
        expect_template_close();
        parse_optional_cpp20_requires_clause(*this);

        // Push template scope so struct body / member parsing can consult it.
        {
            std::vector<TemplateScopeParam> scope_params;
            for (size_t i = 0; i < template_params.size(); ++i) {
                TemplateScopeParam p;
                p.name_text_id = parser_text_id_for_token(kInvalidText,
                                                          template_params[i]);
                p.name = template_params[i];
                p.is_nttp = template_param_nttp[i];
                scope_params.push_back(p);
            }
            push_template_scope(TemplateScopeKind::FreeFunctionTemplate, scope_params);
            template_prelude_guard.pushed_template_scope = true;
        }

        size_t struct_defs_before = definition_state_.struct_defs.size();
        clear_last_using_alias_name();
        Node* templated = parse_top_level();
        // If parse_top_level() registered a using-alias, record alias template
        // info so that later Name<args> can rebuild the aliased template struct.
        const std::string_view alias_name = last_using_alias_name_text();
        if (!alias_name.empty() && !template_params.empty()) {
            if (const TypeSpec* aliased_type =
                    find_visible_typedef_type(alias_name)) {
                ParserAliasTemplateInfo ati;
                for (size_t i = 0; i < template_params.size(); ++i) {
                    ati.param_names.push_back(template_params[i]);
                    ati.param_is_nttp.push_back(template_param_nttp[i]);
                    ati.param_is_pack.push_back(template_param_is_pack[i]);
                    ati.param_has_default.push_back(template_param_has_default[i]);
                    ati.param_default_types.push_back(template_param_default_types[i]);
                    ati.param_default_values.push_back(template_param_default_values[i]);
                }
                ati.aliased_type = *aliased_type;
                template_state_.alias_template_info[std::string(alias_name)] =
                    std::move(ati);
            }
            clear_last_using_alias_name();
        }
        auto attach_template_params = [&](Node* n) {
            if (!n || template_params.empty()) return;
            n->n_template_params = (int)template_params.size();
            n->template_param_names = arena_.alloc_array<const char*>(n->n_template_params);
            n->template_param_is_nttp = arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_is_pack = arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_has_default = arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_default_types = arena_.alloc_array<TypeSpec>(n->n_template_params);
            n->template_param_default_values = arena_.alloc_array<long long>(n->n_template_params);
            n->template_param_default_exprs = arena_.alloc_array<const char*>(n->n_template_params);
            for (int i = 0; i < n->n_template_params; ++i) {
                n->template_param_names[i] = template_params[i];
                n->template_param_is_nttp[i] = template_param_nttp[i];
                n->template_param_is_pack[i] = template_param_is_pack[i];
                n->template_param_has_default[i] = template_param_has_default[i];
                n->template_param_default_types[i] = template_param_default_types[i];
                n->template_param_default_values[i] = template_param_default_values[i];
                n->template_param_default_exprs[i] = template_param_default_exprs[i];
            }
            // Store deferred NTTP default tokens keyed by node name.
            if (n->name && !deferred_nttp_defaults.empty()) {
                for (auto& [idx, toks] : deferred_nttp_defaults) {
                    std::string key = std::string(n->name) + ":" + std::to_string(idx);
                    template_state_.nttp_default_expr_tokens[key] =
                        std::move(toks);
                }
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
        // recursive parse_top_level() call and stored in
        // definition_state_.struct_defs, while
        // `templated` is NK_EMPTY (struct-only declaration).  Find the struct
        // def and attach template params to it.  Only consider structs that were
        // added during THIS template parse (not pre-existing instantiated structs).
        if (definition_state_.struct_defs.size() > struct_defs_before) {
            Node* last_sd = definition_state_.struct_defs.back();
            if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                last_sd->template_origin_name && last_sd->n_template_args > 0) {
                if (!template_params.empty() && last_sd->n_template_params == 0)
                    attach_template_params(last_sd);
                register_template_struct_specialization(
                    last_sd->template_origin_name, last_sd);
            } else if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                       !template_params.empty()) {
                if (last_sd->n_template_params == 0)
                    attach_template_params(last_sd);
                register_template_struct_primary(last_sd->name, last_sd);
                // In C++ mode, register the template struct name as a typedef
                // so it's recognized as a type start for Pair<int> syntax.
                register_tag_type_binding(last_sd->name, TB_STRUCT,
                                          last_sd->name);
                // If inside a namespace, also register ns::Name so
                // qualified references like std::vector<int> work.
                const std::string qn = bridge_name_in_context(
                    current_namespace_context_id(), find_parser_text_id(last_sd->name),
                    last_sd->name);
                register_tag_type_binding(qn, TB_STRUCT, last_sd->name);
                register_template_struct_primary(qn, last_sd);
            }
        }
        return templated;
    }

    if (try_skip_cpp_concept_declaration(*this)) {
        return nullptr;
    }

    // Handle #pragma pack tokens
    if (check(TokenKind::PragmaPack)) {
        handle_pragma_pack(cur().text_id == kInvalidText
                               ? std::string()
                               : std::string(token_spelling(cur())));
        consume();
        return nullptr;
    }

    // Handle #pragma weak tokens
    if (check(TokenKind::PragmaWeak)) {
        auto* node = make_node(NK_PRAGMA_WEAK, ln);
        node->name = arena_.strdup(std::string(token_spelling(cur())));
        consume();
        return node;
    }

    // Handle #pragma GCC visibility push/pop tokens
    if (check(TokenKind::PragmaGccVisibility)) {
        handle_pragma_gcc_visibility(std::string(token_spelling(cur())));
        consume();
        return nullptr;
    }

    if (check(TokenKind::PragmaExec)) {
        auto* node = make_node(NK_PRAGMA_EXEC, ln);
        node->name = arena_.strdup(std::string(token_spelling(cur())));
        handle_pragma_exec(std::string(token_spelling(cur())));
        consume();
        return node;
    }

    // Don't skip_attributes() here — type-affecting attributes like
    // __attribute__((vector_size(N))) must flow to parse_base_type().
    if (at_end()) return nullptr;

    // Handle top-level asm
    if (check(TokenKind::KwAsm)) {
        consume();
        if (check(TokenKind::LParen)) {
            const bool closed = skip_top_level_asm_or_recover(*this);
            if (closed) match(TokenKind::Semi);
            return nullptr;
        }
        match(TokenKind::Semi);
        return nullptr;
    }

    // _Static_assert
    if (check(TokenKind::KwStaticAssert)) {
        return parse_static_assert_declaration();
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
    if (is_cpp_mode() && linkage_spec && check(TokenKind::LBrace)) {
        consume();
        std::vector<Node*> items;
        while (!at_end() && !check(TokenKind::RBrace)) {
            Node* item = parse_top_level();
            if (item) items.push_back(item);
        }
        expect(TokenKind::RBrace);
        match(TokenKind::Semi);
        if (items.empty()) return nullptr;
        if (items.size() == 1) return items[0];
        Node* blk = make_node(NK_BLOCK, ln);
        blk->n_children = static_cast<int>(items.size());
        blk->children = arena_.alloc_array<Node*>(blk->n_children);
        for (int i = 0; i < blk->n_children; ++i) blk->children[i] = items[i];
        return blk;
    }
    // Don't skip_attributes() here — let parse_base_type() handle them so
    // type-affecting attributes like vector_size are captured.

    // C++ out-of-class constructor / conversion-operator definition/declaration:
    //   Type::Type(params) { ... }
    //   Type::operator bool() const { ... }
    // There is no explicit return type, so handle these before parse_base_type().
    if (is_cpp_mode() && !is_typedef) {
        // NOTE: saved_special_member_pos is captured by the lambdas below;
        // converting to TentativeParseGuard would require refactoring all
        // lambda captures, so manual pos_ save/restore is intentionally kept here.
        const int saved_special_member_pos = pos_;
        auto consume_optional_template_id = [&]() -> bool {
            if (!check(TokenKind::Less)) return true;
            int depth = 1;
            consume();
            while (!at_end() && depth > 0) {
                if (check(TokenKind::Less)) {
                    ++depth;
                    consume();
                    continue;
                }
                if (check_template_close()) {
                    --depth;
                    if (!match_template_close()) return false;
                    continue;
                }
                consume();
            }
            return depth == 0;
        };

        auto probe_skip_optional_template_id = [&](int* probe_pos) -> bool {
            if (!probe_pos) return false;
            if (*probe_pos >= static_cast<int>(core_input_state_.tokens.size()) ||
                core_input_state_.tokens[*probe_pos].kind != TokenKind::Less) {
                return true;
            }
            int depth = 1;
            ++(*probe_pos);
            while (*probe_pos < static_cast<int>(core_input_state_.tokens.size()) &&
                   depth > 0) {
                const TokenKind kind = core_input_state_.tokens[*probe_pos].kind;
                if (kind == TokenKind::Less) {
                    ++depth;
                    ++(*probe_pos);
                    continue;
                }
                if (kind == TokenKind::Greater) {
                    --depth;
                    ++(*probe_pos);
                    continue;
                }
                if (kind == TokenKind::GreaterGreater) {
                    depth -= 2;
                    ++(*probe_pos);
                    continue;
                }
                if (kind == TokenKind::GreaterEqual) {
                    --depth;
                    ++(*probe_pos);
                    continue;
                }
                if (kind == TokenKind::GreaterGreaterAssign) {
                    depth -= 2;
                    ++(*probe_pos);
                    continue;
                }
                ++(*probe_pos);
            }
            return depth <= 0;
        };

        auto probe_special_member_owner =
            [&](std::string* out_owner, std::string* out_base_name,
                bool* out_is_operator, bool* out_is_ctor) -> bool {
                if (out_owner) out_owner->clear();
                if (out_base_name) out_base_name->clear();
                if (out_is_operator) *out_is_operator = false;
                if (out_is_ctor) *out_is_ctor = false;

                int probe = saved_special_member_pos;
                if (probe < static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[probe].kind == TokenKind::ColonColon) {
                    ++probe;
                }
                if (probe >= static_cast<int>(core_input_state_.tokens.size()) ||
                    core_input_state_.tokens[probe].kind != TokenKind::Identifier) {
                    return false;
                }

                std::string owner;
                while (true) {
                    const std::string seg =
                        std::string(token_spelling(core_input_state_.tokens[probe]));
                    ++probe;
                    if (!probe_skip_optional_template_id(&probe)) {
                        return false;
                    }

                    if (probe + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
                        core_input_state_.tokens[probe].kind == TokenKind::ColonColon &&
                        core_input_state_.tokens[probe + 1].kind ==
                            TokenKind::KwOperator) {
                        if (!owner.empty()) owner += "::";
                        owner += seg;
                        if (out_owner) *out_owner = owner;
                        if (out_base_name) *out_base_name = seg;
                        if (out_is_operator) *out_is_operator = true;
                        return true;
                    }

                    if (probe + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
                        core_input_state_.tokens[probe].kind == TokenKind::ColonColon &&
                        core_input_state_.tokens[probe + 1].kind ==
                            TokenKind::Identifier) {
                        const std::string next_name =
                            std::string(
                                token_spelling(core_input_state_.tokens[probe + 1]));
                        int terminal_probe = probe + 2;
                        if (next_name == seg &&
                            terminal_probe <
                                static_cast<int>(core_input_state_.tokens.size()) &&
                            core_input_state_.tokens[terminal_probe].kind ==
                                TokenKind::LParen) {
                            if (!owner.empty()) owner += "::";
                            owner += seg;
                            if (out_owner) *out_owner = owner;
                            if (out_base_name) *out_base_name = seg;
                            if (out_is_ctor) *out_is_ctor = true;
                            return true;
                        }

                        if (!owner.empty()) owner += "::";
                        owner += seg;
                        probe += 2;
                        continue;
                    }

                    return false;
                }
            };

        auto consume_special_member_owner =
            [&](bool stop_before_operator, bool stop_before_ctor) -> bool {
                pos_ = saved_special_member_pos;
                if (check(TokenKind::ColonColon)) consume();
                if (!check(TokenKind::Identifier)) return false;

                while (true) {
                    const std::string seg = std::string(token_spelling(cur()));
                    consume();
                    if (!consume_optional_template_id()) return false;

                    if (stop_before_operator &&
                        check(TokenKind::ColonColon) &&
                        peek(1).kind == TokenKind::KwOperator) {
                        return true;
                    }

                    if (stop_before_ctor &&
                        check(TokenKind::ColonColon) &&
                        peek(1).kind == TokenKind::Identifier &&
                        token_spelling(peek(1)) == seg &&
                        core_input_state_.pos + 2 <
                            static_cast<int>(core_input_state_.tokens.size()) &&
                        core_input_state_.tokens[core_input_state_.pos + 2].kind ==
                            TokenKind::LParen) {
                        return true;
                    }

                    if (check(TokenKind::ColonColon) && peek(1).kind == TokenKind::Identifier) {
                        consume();
                        continue;
                    }

                    return false;
                }
            };

        std::string qualified_owner;
        std::string owner_base_name;
        bool looks_like_qualified_operator = false;
        bool looks_like_qualified_ctor = false;
        probe_special_member_owner(&qualified_owner, &owner_base_name,
                                   &looks_like_qualified_operator,
                                   &looks_like_qualified_ctor);

        if (looks_like_qualified_operator) {
            if (!consume_special_member_owner(/*stop_before_operator=*/true,
                                              /*stop_before_ctor=*/false)) {
                pos_ = saved_special_member_pos;
            }
            expect(TokenKind::ColonColon);
            expect(TokenKind::KwOperator);

            // Enter owner scope for out-of-class operator definition.
            std::string saved_tag_op = active_context_state_.current_struct_tag;
            set_current_struct_tag(qualified_owner);
            if (!template_state_.template_scope_stack.empty() &&
                template_state_.template_scope_stack.back().kind ==
                    TemplateScopeKind::FreeFunctionTemplate &&
                (find_template_struct_primary(qualified_owner) ||
                 template_state_.template_struct_defs.count(qualified_owner))) {
                template_state_.template_scope_stack.back().kind =
                    TemplateScopeKind::EnclosingClass;
                template_state_.template_scope_stack.back().owner_struct_tag =
                    qualified_owner;
            }

            TypeSpec conv_ts = parse_base_type();
            parse_attributes(&conv_ts);
            while (check(TokenKind::Star)) {
                consume();
                conv_ts.ptr_level++;
            }
            if (check(TokenKind::AmpAmp)) {
                consume();
                conv_ts.is_rvalue_ref = true;
            } else if (check(TokenKind::Amp)) {
                consume();
                conv_ts.is_lvalue_ref = true;
            }

            std::string qualified_op_name = qualified_owner;
            if (!qualified_op_name.empty()) qualified_op_name += "::";
            if (conv_ts.base == TB_BOOL && conv_ts.ptr_level == 0 &&
                !conv_ts.is_lvalue_ref && !conv_ts.is_rvalue_ref) {
                qualified_op_name += "operator_bool";
            } else {
                qualified_op_name += "operator_conv_";
                append_type_mangled_suffix_local(qualified_op_name, conv_ts);
            }

            expect(TokenKind::LParen);
            std::vector<Node*> params;
            bool variadic = false;
            if (!check(TokenKind::RParen)) {
                while (!at_end()) {
                    if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                    if (check(TokenKind::RParen)) break;
                    if (!can_start_parameter_type()) break;
                    Node* p = parse_param();
                    if (p) params.push_back(p);
                    if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                    if (!match(TokenKind::Comma)) break;
                }
            }
            expect(TokenKind::RParen);

            bool is_const_method = false;
            bool is_lvalue_ref_method = false;
            bool is_rvalue_ref_method = false;
            while (true) {
                if (match(TokenKind::KwConst)) {
                    is_const_method = true;
                } else if (match(TokenKind::KwConstexpr)) {
                    is_constexpr = true;
                } else if (match(TokenKind::KwConsteval)) {
                    is_consteval = true;
                } else {
                    break;
                }
            }
            if (match(TokenKind::AmpAmp)) {
                is_rvalue_ref_method = true;
            } else if (match(TokenKind::Amp)) {
                is_lvalue_ref_method = true;
            }
            skip_exception_spec();
            parse_optional_cpp20_trailing_requires_clause(*this);

            Node* fn = make_node(NK_FUNCTION, ln);
            fn->type = conv_ts;
            fn->name = arena_.strdup(qualified_op_name.c_str());
            fn->variadic = variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->is_constexpr = is_constexpr;
            fn->is_consteval = is_consteval;
            fn->is_const_method = is_const_method;
            fn->is_lvalue_ref_method = is_lvalue_ref_method;
            fn->is_rvalue_ref_method = is_rvalue_ref_method;
            fn->linkage_spec = linkage_spec;
            fn->visibility = pragma_state_.visibility;
            fn->execution_domain = pragma_state_.execution_domain;
            fn->n_params = static_cast<int>(params.size());
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }

            if (check(TokenKind::LBrace)) {
                bool saved_top = active_context_state_.parsing_top_level_context;
                active_context_state_.parsing_top_level_context = false;
                fn->body = parse_block();
                active_context_state_.parsing_top_level_context = saved_top;
            } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                       core_input_state_.pos + 1 <
                           static_cast<int>(core_input_state_.tokens.size()) &&
                       core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                           TokenKind::KwDelete) {
                consume();
                consume();
                fn->is_deleted = true;
                match(TokenKind::Semi);
            } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                       core_input_state_.pos + 1 <
                           static_cast<int>(core_input_state_.tokens.size()) &&
                       core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                           TokenKind::KwDefault) {
                consume();
                consume();
                fn->is_defaulted = true;
                match(TokenKind::Semi);
            } else {
                match(TokenKind::Semi);
            }
            register_known_fn_name(qualified_op_name);
            set_current_struct_tag(saved_tag_op);
            return fn;
        }

        if (looks_like_qualified_ctor) {
            if (!consume_special_member_owner(/*stop_before_operator=*/false,
                                              /*stop_before_ctor=*/true)) {
                pos_ = saved_special_member_pos;
            }
            expect(TokenKind::ColonColon);
            const std::string ctor_name = std::string(token_spelling(cur()));
            consume();
            if (check(TokenKind::LParen)) {
                std::string qualified_ctor_name = qualified_owner;
                if (!qualified_ctor_name.empty()) qualified_ctor_name += "::";
                qualified_ctor_name += ctor_name;

                // Enter owner scope for out-of-class constructor definition.
                std::string saved_tag_ctor =
                    active_context_state_.current_struct_tag;
                set_current_struct_tag(qualified_owner);
                if (!template_state_.template_scope_stack.empty() &&
                    template_state_.template_scope_stack.back().kind ==
                        TemplateScopeKind::FreeFunctionTemplate &&
                    (find_template_struct_primary(qualified_owner) ||
                     template_state_.template_struct_defs.count(qualified_owner))) {
                    template_state_.template_scope_stack.back().kind =
                        TemplateScopeKind::EnclosingClass;
                    template_state_.template_scope_stack.back().owner_struct_tag =
                        qualified_owner;
                }

                consume();  // (
                std::vector<Node*> params;
                bool variadic = false;
                if (!check(TokenKind::RParen)) {
                    while (!at_end()) {
                        if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                        if (check(TokenKind::RParen)) break;
                        if (!can_start_parameter_type()) break;
                        Node* p = parse_param();
                        if (p) params.push_back(p);
                        if (check(TokenKind::Ellipsis)) { variadic = true; consume(); break; }
                        if (!match(TokenKind::Comma)) break;
                    }
                }
                expect(TokenKind::RParen);
                skip_exception_spec();
                parse_optional_cpp20_trailing_requires_clause(*this);

                std::vector<const char*> init_names;
                std::vector<std::vector<Node*>> init_args_list;
                if (check(TokenKind::Colon)) {
                    consume(); // :
                    while (!at_end() && !check(TokenKind::LBrace)) {
                        if (!check(TokenKind::Identifier)) break;
                        const char* mem_name =
                            arena_.strdup(std::string(token_spelling(cur())));
                        consume();
                        expect(TokenKind::LParen);
                        std::vector<Node*> args;
                        if (!check(TokenKind::RParen)) {
                            while (true) {
                                Node* arg = parse_assign_expr();
                                if (arg) args.push_back(arg);
                                if (!match(TokenKind::Comma)) break;
                            }
                        }
                        expect(TokenKind::RParen);
                        init_names.push_back(mem_name);
                        init_args_list.push_back(std::move(args));
                        if (!match(TokenKind::Comma)) break;
                    }
                }

                Node* fn = make_node(NK_FUNCTION, ln);
                fn->type.base = TB_VOID;
                fn->name = arena_.strdup(qualified_ctor_name.c_str());
                fn->variadic = variadic;
                fn->is_static = is_static;
                fn->is_extern = is_extern;
                fn->is_inline = is_inline;
                fn->is_constexpr = is_constexpr;
                fn->is_consteval = is_consteval;
                fn->is_constructor = true;
                fn->linkage_spec = linkage_spec;
                fn->visibility = pragma_state_.visibility;
                fn->execution_domain = pragma_state_.execution_domain;
                fn->n_params = static_cast<int>(params.size());
                if (fn->n_params > 0) {
                    fn->params = arena_.alloc_array<Node*>(fn->n_params);
                    for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
                }
                if (!init_names.empty()) {
                    fn->n_ctor_inits = static_cast<int>(init_names.size());
                    fn->ctor_init_names = arena_.alloc_array<const char*>(fn->n_ctor_inits);
                    fn->ctor_init_args = arena_.alloc_array<Node**>(fn->n_ctor_inits);
                    fn->ctor_init_nargs = arena_.alloc_array<int>(fn->n_ctor_inits);
                    for (int i = 0; i < fn->n_ctor_inits; ++i) {
                        fn->ctor_init_names[i] = init_names[i];
                        fn->ctor_init_nargs[i] = static_cast<int>(init_args_list[i].size());
                        if (fn->ctor_init_nargs[i] > 0) {
                            fn->ctor_init_args[i] = arena_.alloc_array<Node*>(fn->ctor_init_nargs[i]);
                            for (int j = 0; j < fn->ctor_init_nargs[i]; ++j)
                                fn->ctor_init_args[i][j] = init_args_list[i][j];
                        } else {
                            fn->ctor_init_args[i] = nullptr;
                        }
                    }
                }

                if (check(TokenKind::LBrace)) {
                    bool saved_top =
                        active_context_state_.parsing_top_level_context;
                    active_context_state_.parsing_top_level_context = false;
                    fn->body = parse_block();
                    active_context_state_.parsing_top_level_context = saved_top;
                } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                           core_input_state_.pos + 1 <
                               static_cast<int>(core_input_state_.tokens.size()) &&
                           core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                               TokenKind::KwDelete) {
                    consume();
                    consume();
                    fn->is_deleted = true;
                    match(TokenKind::Semi);
                } else if (is_cpp_mode() && check(TokenKind::Assign) &&
                           core_input_state_.pos + 1 <
                               static_cast<int>(core_input_state_.tokens.size()) &&
                           core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                               TokenKind::KwDefault) {
                    consume();
                    consume();
                    fn->is_defaulted = true;
                    match(TokenKind::Semi);
                } else {
                    match(TokenKind::Semi);
                }
                register_known_fn_name(qualified_ctor_name);
                set_current_struct_tag(saved_tag_ctor);
                return fn;
            }
        }
        pos_ = saved_special_member_pos;
    }

    TypeSpec base_ts{};
    if (!is_type_start()) {
        if (is_typedef) {
            if (check(TokenKind::Identifier)) {
                const std::string spelled = std::string(token_spelling(cur()));
                if (is_template_scope_type_param(spelled)) {
                    base_ts.array_size = -1;
                    base_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) base_ts.array_dims[i] = -1;
                    base_ts.base = TB_TYPEDEF;
                    base_ts.tag = arena_.strdup(spelled.c_str());
                    consume();
                    goto top_level_base_ready;
                }
                if (const TypeSpec* typedef_type =
                        find_visible_typedef_type(spelled)) {
                    base_ts = *typedef_type;
                    consume();
                    goto top_level_base_ready;
                }
            }
            std::string nm = check(TokenKind::Identifier)
                                 ? std::string(token_spelling(cur()))
                                 : "<non-identifier>";
            throw std::runtime_error(std::string("typedef uses unknown base type: ") + nm);
        }
        // K&R/old-style implicit-int function at file scope:
        // e.g. `main() { ... }` or `static foo() { ... }`.
        // Treat as `int name(...)` instead of discarding the whole definition.
        bool maybe_implicit_int_fn =
            check(TokenKind::Identifier) && peek_next_is(TokenKind::LParen);
        if (!maybe_implicit_int_fn) {
            const int recovery_line = !at_end() ? cur().line : ln;
            while (!at_end() && !check(TokenKind::Semi)) {
                if (cur().line != recovery_line &&
                    is_top_level_decl_recovery_boundary(cur().kind)) {
                    goto top_level_decl_recovery_done;
                }
                consume();
            }
            match(TokenKind::Semi);
top_level_decl_recovery_done:
            // Unsupported top-level recovery is bookkeeping-only once we stop
            // at a declaration boundary or terminator, so do not materialize a
            // synthetic Empty node in the item stream.
            return nullptr;
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
top_level_base_ready:
    // C++: consteval and constexpr cannot both appear on the same declaration.
    if (is_consteval && is_constexpr) {
        throw std::runtime_error(
            std::string("error: 'consteval' and 'constexpr' cannot be combined (line ")
            + std::to_string(ln) + ")");
    }

    // Phase C: save the return-type typedef name for fn_ptr propagation to NK_FUNCTION.
    const TextId ret_typedef_name_id =
        active_context_state_.last_resolved_typedef_text_id;

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (ts.tpl_struct_origin) return false;  // pending template struct — resolved at HIR level
        if (!ts.tag) return true;
        if (is_cpp_mode() && !active_context_state_.current_struct_tag.empty()) {
            if (is_same_visible_type_name(
                    *this, active_context_state_.current_struct_tag_text_id,
                    current_struct_tag_text(), find_parser_text_id(ts.tag),
                    ts.tag))
                return false;
        }
        auto it = definition_state_.struct_tag_def_map.find(ts.tag);
        if (it == definition_state_.struct_tag_def_map.end()) return true;
        Node* def = it->second;
        return !def || def->n_fields < 0;
    };
    if (is_typedef) {
        // Local typedef
        const char* tdname = nullptr;
        TextId tdname_text_id = kInvalidText;
        TypeSpec ts_copy = base_ts;
        Node** td_fn_ptr_params = nullptr;
        int td_n_fn_ptr_params = 0;
        bool td_fn_ptr_variadic = false;
        parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic, nullptr,
                         nullptr, nullptr, nullptr, &tdname_text_id);
        if (tdname) {
            const char* scoped_tdname =
                qualify_name_arena(tdname_text_id, tdname);
            if (!is_cpp_mode() && !is_internal_typedef_name(tdname) &&
                has_conflicting_user_typedef_binding(tdname, ts_copy))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            register_typedef_binding(tdname, ts_copy,
                                     !is_internal_typedef_name(tdname));
            if (scoped_tdname != tdname) {
                register_typedef_binding(scoped_tdname, ts_copy, false);
            }
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
                const TextId typedef_name_id =
                    tdname_text_id != kInvalidText
                        ? tdname_text_id
                        : parser_text_id_for_token(kInvalidText, tdname);
                if (typedef_name_id != kInvalidText) {
                    binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                        td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
                }
            }
        }
        while (match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            TextId tdn2_text_id = kInvalidText;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic,
                             nullptr, nullptr, nullptr, nullptr,
                             &tdn2_text_id);
            if (tdn2) {
                const char* scoped_tdn2 =
                    qualify_name_arena(tdn2_text_id, tdn2);
                if (!is_cpp_mode() && !is_internal_typedef_name(tdn2) &&
                    has_conflicting_user_typedef_binding(tdn2, ts2))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                register_typedef_binding(tdn2, ts2,
                                         !is_internal_typedef_name(tdn2));
                if (scoped_tdn2 != tdn2) {
                    register_typedef_binding(scoped_tdn2, ts2, false);
                }
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                    const TextId typedef_name_id =
                        tdn2_text_id != kInvalidText
                            ? tdn2_text_id
                            : parser_text_id_for_token(kInvalidText, tdn2);
                    if (typedef_name_id != kInvalidText) {
                        binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                            td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
                    }
                }
            }
        }
        match(TokenKind::Semi);
        // Top-level typedef declarations are bookkeeping-only once the alias
        // tables have been updated, so drop them from the program item stream.
        return nullptr;
    }

    // Peek to disambiguate: struct/union/enum declaration only (no declarator)
    if (check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        consume();  // consume ;
        return nullptr;
    }

    // Parse declarator (name + pointer stars + maybe function params)
    TypeSpec ts = base_ts;
    const int decl_suffix_start = pos_;
    // Preserve typedef array dims: apply_decl_dims prepends declarator dims to base typedef dims.
    ts.array_size_expr = nullptr;
    const char* decl_name = nullptr;
    TextId decl_name_text_id = kInvalidText;
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
    if (check(TokenKind::LParen) && peek_next_is(TokenKind::Star)) {
        consume();  // (
        consume();  // *
        ts.ptr_level++;
        while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
        // Skip qualifiers after * (e.g. `double (* const a[])`)
        while (is_qualifier(cur().kind)) consume();
        // Skip nullability annotations: (* _Nullable name) or (* _Nonnull name)
        while (check(TokenKind::Identifier)) {
            const std::string ql = std::string(token_spelling(cur()));
            if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                ql == "__nullable" || ql == "__nonnull")
                consume();
            else break;
        }
        skip_attributes();
        if (check(TokenKind::Identifier)) {
            decl_name_text_id =
                parser_text_id_for_token(cur().text_id, token_spelling(cur()));
            decl_name = arena_.strdup(std::string(token_spelling(cur())));
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
        if (check(TokenKind::LParen) && peek_next_is(TokenKind::Star)) {
            // Nested fn_ptr: (* (*name(fn_params))(inner_ret_params) )(outer_ret_params)
            // Recurse: consume '(' '*' to enter the inner level.
            consume();  // (
            consume();  // *
            ts.ptr_level++;
            while (check(TokenKind::Star)) { consume(); ts.ptr_level++; }
            // Skip qualifiers/nullability
            while (is_qualifier(cur().kind)) consume();
            while (check(TokenKind::Identifier)) {
                const std::string ql = std::string(token_spelling(cur()));
                if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                    ql == "__nullable" || ql == "__nonnull") { consume(); continue; }
                break;
            }
            if (check(TokenKind::Identifier)) {
                decl_name_text_id = parser_text_id_for_token(
                    cur().text_id, token_spelling(cur()));
                decl_name = arena_.strdup(std::string(token_spelling(cur())));
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
                        if (!can_start_parameter_type()) {
                            consume();
                            if (match(TokenKind::Comma)) continue;
                            break;
                        }
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
                        if (!can_start_parameter_type()) {
                            consume();
                            if (match(TokenKind::Comma)) continue;
                            break;
                        }
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
                    if (!can_start_parameter_type()) {
                        consume();
                        if (match(TokenKind::Comma)) continue;
                        break;
                    }
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
                    if (!can_start_parameter_type()) {
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
                     &decl_fn_ptr_variadic, nullptr, &decl_ret_fn_ptr_params,
                     &decl_n_ret_fn_ptr_params, &decl_ret_fn_ptr_variadic,
                     &decl_name_text_id);
    if (is_incomplete_object_type(ts) && !check(TokenKind::LParen)) {
        throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
    }
    }

    // Owner-aware scope: if the declarator produced a qualified name like
    // "vector::set_capacity", detect the owner struct and re-enter its scope
    // so member typedefs are visible during parameter/body parsing.
    // We save/restore current_struct_tag_ via a scope guard so all exit paths
    // are covered.
    std::string qualified_owner_tag;
    auto enter_owner_scope = [&]() {
        if (!is_cpp_mode() || !decl_name) return;
        std::string dn(decl_name);
        auto sep = dn.rfind("::");
        if (sep == std::string::npos || sep == 0) return;
        qualified_owner_tag = dn.substr(0, sep);
        set_current_struct_tag(qualified_owner_tag);
        // If the owner is a known template struct and we have an active
        // FreeFunctionTemplate scope, relabel it as EnclosingClass.
        if (!template_state_.template_scope_stack.empty() &&
            template_state_.template_scope_stack.back().kind ==
                TemplateScopeKind::FreeFunctionTemplate) {
            if (find_template_struct_primary(qualified_owner_tag) ||
                template_state_.template_struct_defs.count(qualified_owner_tag)) {
                template_state_.template_scope_stack.back().kind =
                    TemplateScopeKind::EnclosingClass;
                template_state_.template_scope_stack.back().owner_struct_tag =
                    qualified_owner_tag;
            }
        }
    };
    std::string saved_struct_tag_for_qualified =
        active_context_state_.current_struct_tag;
    enter_owner_scope();
    // Helper to restore struct tag before returning from this function.
    auto restore_owner_scope = [&]() {
        if (!qualified_owner_tag.empty())
            set_current_struct_tag(saved_struct_tag_for_qualified);
    };

    // Explicit template specialization: parse <type_args> after function name.
    // e.g. template<> int add<int>(int a, int b) { ... }
    //                          ^^^^^ captured here
    std::vector<TypeSpec> spec_arg_types;
    std::vector<bool> spec_arg_is_value;
    std::vector<long long> spec_arg_values;
    if (active_context_state_.parsing_explicit_specialization && decl_name &&
        check(TokenKind::Less)) {
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
                       (check(TokenKind::Minus) &&
                        core_input_state_.pos + 1 <
                            static_cast<int>(core_input_state_.tokens.size()) &&
                        (core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                             TokenKind::IntLit ||
                         core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                             TokenKind::CharLit))) {
                // NTTP value
                long long sign = 1;
                if (match(TokenKind::Minus)) sign = -1;
                long long val =
                    parse_int_lexeme(std::string(token_spelling(cur())).c_str());
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
                    val = parse_int_lexeme(std::string(token_spelling(cur())).c_str());
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
    skip_attributes();

    if (!decl_name) {
        auto tail_is_attr_or_asm_only = [&](int begin, int end) -> bool {
            int i = begin;
            auto skip_parens = [&](int start) -> int {
                if (start >= end ||
                    core_input_state_.tokens[start].kind != TokenKind::LParen) {
                    return -1;
                }
                int depth = 0;
                int j = start;
                while (j < end) {
                    if (core_input_state_.tokens[j].kind == TokenKind::LParen) ++depth;
                    else if (core_input_state_.tokens[j].kind == TokenKind::RParen &&
                             --depth == 0)
                        return j + 1;
                    ++j;
                }
                return -1;
            };
            auto skip_cpp11_attrs = [&](int start) -> int {
                if (start + 1 >= end ||
                    core_input_state_.tokens[start].kind != TokenKind::LBracket ||
                    core_input_state_.tokens[start + 1].kind != TokenKind::LBracket) {
                    return -1;
                }
                int depth = 1;
                int j = start + 2;
                while (j < end) {
                    if (j + 1 < end &&
                        core_input_state_.tokens[j].kind == TokenKind::LBracket &&
                        core_input_state_.tokens[j + 1].kind == TokenKind::LBracket) {
                        ++depth;
                        j += 2;
                        continue;
                    }
                    if (j + 1 < end &&
                        core_input_state_.tokens[j].kind == TokenKind::RBracket &&
                        core_input_state_.tokens[j + 1].kind == TokenKind::RBracket) {
                        --depth;
                        j += 2;
                        if (depth == 0) return j;
                        continue;
                    }
                    ++j;
                }
                return -1;
            };

            while (i < end) {
                if (core_input_state_.tokens[i].kind == TokenKind::KwAttribute) {
                    ++i;
                    i = skip_parens(i);
                    if (i < 0 || i >= end ||
                        core_input_state_.tokens[i].kind != TokenKind::LParen) {
                        return false;
                    }
                    i = skip_parens(i);
                    if (i < 0) return false;
                    continue;
                }
                if (core_input_state_.tokens[i].kind == TokenKind::LBracket) {
                    i = skip_cpp11_attrs(i);
                    if (i < 0) return false;
                    continue;
                }
                if (core_input_state_.tokens[i].kind == TokenKind::KwAsm) {
                    ++i;
                    while (i < end &&
                           (core_input_state_.tokens[i].kind == TokenKind::KwVolatile ||
                            core_input_state_.tokens[i].kind == TokenKind::KwInline ||
                            core_input_state_.tokens[i].kind == TokenKind::KwGoto)) {
                        ++i;
                    }
                    i = skip_parens(i);
                    if (i < 0) return false;
                    continue;
                }
                if (core_input_state_.tokens[i].kind == TokenKind::KwExtension ||
                    core_input_state_.tokens[i].kind == TokenKind::KwNoreturn) {
                    ++i;
                    continue;
                }
                return false;
            }
            return true;
        };

        // Drop valid top-level structure-only declarations once their real
        // struct/enum definition has already been recorded, even when trailing
        // attributes or asm labels route them through the declarator fallback.
        const bool is_structure_only_decl =
            check(TokenKind::Semi) &&
            (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
             base_ts.base == TB_ENUM) &&
            tail_is_attr_or_asm_only(decl_suffix_start, pos_);
        match(TokenKind::Semi);
        restore_owner_scope();
        if (is_structure_only_decl) return nullptr;
        // No name: just a type declaration; keep the explicit discard node for
        // non-structure-only fallback shapes so recovery remains visible.
        return make_node(NK_EMPTY, ln);
    }

    const char* scoped_decl_name =
        qualify_name_arena(decl_name_text_id, decl_name);

    // Handle function-returning-fptr: int (* f1(a, b))(c, d) { body }
    // Params were already parsed into fptr_fn_params; now look for { body }.
    if (fn_returning_fptr && decl_name) {
        parse_attributes(&ts); skip_asm(); parse_attributes(&ts); skip_attributes();
        parse_optional_cpp20_trailing_requires_clause(*this);
        if (check(TokenKind::LBrace)) {
            bool saved_top = active_context_state_.parsing_top_level_context;
            active_context_state_.parsing_top_level_context = false;
            Node* body = parse_block();
            active_context_state_.parsing_top_level_context = saved_top;
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
            fn->visibility = pragma_state_.visibility;
            fn->execution_domain = pragma_state_.execution_domain;
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
            register_known_fn_name(scoped_decl_name);
            restore_owner_scope();
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
        fn->visibility = pragma_state_.visibility;
        fn->execution_domain = pragma_state_.execution_domain;
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
        register_known_fn_name(scoped_decl_name);
        restore_owner_scope();
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
        std::vector<Node*> params;
        std::vector<const char*> knr_param_names;
        bool variadic = false;
        parse_top_level_parameter_list(&params, &knr_param_names, &variadic);
        if (decl_name) {
            std::string adjusted_name(decl_name);
            finalize_pending_operator_name(adjusted_name, params.size());
            decl_name = arena_.strdup(adjusted_name.c_str());
            decl_name_text_id = kInvalidText;
            scoped_decl_name = qualify_name_arena(decl_name_text_id, decl_name);
        }
        parse_attributes(&ts);
        skip_asm();
        parse_attributes(&ts);
        skip_attributes();
        bool is_const_method = false;
        bool is_lvalue_ref_method = false;
        bool is_rvalue_ref_method = false;
        while (is_cpp_mode() && decl_name && std::string(decl_name).find("::") != std::string::npos) {
            if (match(TokenKind::KwConst)) {
                is_const_method = true;
                continue;
            }
            if (match(TokenKind::KwConstexpr)) {
                is_constexpr = true;
                continue;
            }
            if (match(TokenKind::KwConsteval)) {
                is_consteval = true;
                continue;
            }
            break;
        }
        if (is_cpp_mode() && decl_name && std::string(decl_name).find("::") != std::string::npos) {
            if (match(TokenKind::AmpAmp)) {
                is_rvalue_ref_method = true;
            } else if (match(TokenKind::Amp)) {
                is_lvalue_ref_method = true;
            }
        }
        skip_exception_spec();
        if (is_cpp_mode() && match(TokenKind::Arrow)) {
            ts = parse_type_name();
            parse_attributes(&ts);
        }
        parse_optional_cpp20_trailing_requires_clause(*this);

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
            fn->template_arg_exprs = arena_.alloc_array<Node*>(n);
            for (int i = 0; i < n; ++i) {
                fn->template_arg_types[i] = spec_arg_types[i];
                fn->template_arg_is_value[i] = spec_arg_is_value[i];
                fn->template_arg_values[i] = spec_arg_values[i];
                fn->template_arg_nttp_names[i] = nullptr;
                fn->template_arg_exprs[i] = nullptr;
            }
        };

        // Phase C: helper lambda to propagate return type fn_ptr params to NK_FUNCTION.
        auto propagate_ret_fn_ptr = [&](Node* fn) {
            if (ts.is_fn_ptr) {
                if (const FnPtrTypedefInfo* info =
                        find_typedef_fn_ptr_info(ret_typedef_name_id)) {
                    fn->ret_fn_ptr_params   = info->params;
                    fn->n_ret_fn_ptr_params = info->n_params;
                    fn->ret_fn_ptr_variadic = info->variadic;
                }
            }
        };

        if (check(TokenKind::LBrace)) {
            // Function definition
            bool saved_top = active_context_state_.parsing_top_level_context;
            active_context_state_.parsing_top_level_context = false;
            Node* body = parse_block();
            active_context_state_.parsing_top_level_context = saved_top;
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
            fn->is_const_method = is_const_method;
            fn->is_lvalue_ref_method = is_lvalue_ref_method;
            fn->is_rvalue_ref_method = is_rvalue_ref_method;
            fn->linkage_spec = linkage_spec;
            fn->visibility = pragma_state_.visibility;
            fn->execution_domain = pragma_state_.execution_domain;
            fn->body      = body;
            fn->n_params  = (int)params.size();
            if (fn->n_params > 0) {
                fn->params = arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }
            propagate_ret_fn_ptr(fn);
            attach_spec_args(fn);
            register_known_fn_name(scoped_decl_name);
            restore_owner_scope();
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
        fn->is_const_method = is_const_method;
        fn->is_lvalue_ref_method = is_lvalue_ref_method;
        fn->is_rvalue_ref_method = is_rvalue_ref_method;
        fn->linkage_spec = linkage_spec;
        fn->visibility = pragma_state_.visibility;
        fn->execution_domain = pragma_state_.execution_domain;
        fn->body      = nullptr;  // declaration only
        fn->n_params  = (int)params.size();
        if (fn->n_params > 0) {
            fn->params = arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
        }
        propagate_ret_fn_ptr(fn);
        attach_spec_args(fn);
        register_known_fn_name(scoped_decl_name);
        restore_owner_scope();
        return fn;
    }

    // Global variable (possibly with initializer and multiple declarators)
    std::vector<Node*> gvars;

    auto make_gvar = [&](TypeSpec gts, const char* gname, const char* source_name,
                         TextId source_name_text_id, Node* ginit,
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
        gv->visibility = pragma_state_.visibility;
        gv->execution_domain = pragma_state_.execution_domain;
        gv->fn_ptr_params = fn_ptr_params;
        gv->n_fn_ptr_params = n_fn_ptr_params;
        gv->fn_ptr_variadic = fn_ptr_variadic;
        gv->ret_fn_ptr_params = ret_fn_ptr_params;
        gv->n_ret_fn_ptr_params = n_ret_fn_ptr_params;
        gv->ret_fn_ptr_variadic = ret_fn_ptr_variadic;
        // Phase C: propagate fn_ptr params from typedef if not set by declarator.
        if (gts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
            if (const FnPtrTypedefInfo* info = find_current_typedef_fn_ptr_info()) {
                gv->fn_ptr_params = info->params;
                gv->n_fn_ptr_params = info->n_params;
                gv->fn_ptr_variadic = info->variadic;
            }
        }
        if (gname) register_var_type_binding(gname, gts);
        if (gname && ginit &&
            (gv->type.is_const || gv->is_constexpr) &&
            !gv->type.is_lvalue_ref && !gv->type.is_rvalue_ref &&
            gv->type.ptr_level == 0 && gv->type.array_rank == 0) {
            long long cv = 0;
            if (eval_const_int(ginit, &cv, &definition_state_.struct_tag_def_map,
                               &binding_state_.const_int_bindings)) {
                const TextId binding_name_text_id =
                    source_name_text_id != kInvalidText
                        ? source_name_text_id
                        : parser_text_id_for_token(
                              kInvalidText, source_name ? source_name : "");
                if (binding_name_text_id != kInvalidText) {
                    binding_state_.const_int_bindings[binding_name_text_id] = cv;
                }
            }
        }
        return gv;
    };

    Node* first_init = nullptr;
    if (match(TokenKind::Assign) ||
        (is_cpp_mode() && check(TokenKind::LBrace))) {
        first_init = parse_initializer();
    }
    gvars.push_back(make_gvar(ts, scoped_decl_name, decl_name, decl_name_text_id,
                              first_init, decl_fn_ptr_params,
                              decl_n_fn_ptr_params, decl_fn_ptr_variadic,
                              decl_ret_fn_ptr_params, decl_n_ret_fn_ptr_params,
                              decl_ret_fn_ptr_variadic));

    while (match(TokenKind::Comma)) {
        TypeSpec ts2 = base_ts;
        // Preserve typedef ptr_level and array dims for each additional declarator.
        ts2.array_size_expr = nullptr;
        const char* n2 = nullptr;
        TextId n2_text_id = kInvalidText;
        Node** fn_ptr_params2 = nullptr;
        int n_fn_ptr_params2 = 0;
        bool fn_ptr_variadic2 = false;
        parse_declarator(ts2, &n2, &fn_ptr_params2, &n_fn_ptr_params2,
                         &fn_ptr_variadic2, nullptr, nullptr, nullptr, nullptr,
                         &n2_text_id);
        skip_attributes();
        skip_asm();
        Node* init2 = nullptr;
        if (match(TokenKind::Assign) ||
            (is_cpp_mode() && check(TokenKind::LBrace))) init2 = parse_initializer();
        if (n2) {
            const char* scoped_n2 = qualify_name_arena(n2_text_id, n2);
            gvars.push_back(make_gvar(ts2, scoped_n2, n2, n2_text_id, init2,
                                          fn_ptr_params2, n_fn_ptr_params2,
                                          fn_ptr_variadic2));
        }
    }
    match(TokenKind::Semi);

    restore_owner_scope();
    if (gvars.size() == 1) return gvars[0];

    // Multiple global vars — wrap in a block
    Node* blk = make_node(NK_BLOCK, ln);
    blk->n_children = (int)gvars.size();
    blk->children   = arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = gvars[i];
    return blk;
}

}  // namespace c4c

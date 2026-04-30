#include "parser_impl.hpp"
#include "lexer.hpp"

#include <climits>
#include <sstream>
#include <cstring>
#include <stdexcept>

#define finalize_pending_operator_name finalize_pending_operator_name_types_helpers
#define is_cpp20_requires_clause_decl_boundary \
    is_cpp20_requires_clause_decl_boundary_types_helpers
#define skip_cpp20_constraint_atom skip_cpp20_constraint_atom_types_helpers
#define parse_optional_cpp20_requires_clause parse_optional_cpp20_requires_clause_types_helpers
#define parse_optional_cpp20_trailing_requires_clause \
    parse_optional_cpp20_trailing_requires_clause_types_helpers
#include "types/types_helpers.hpp"
#undef finalize_pending_operator_name
#undef is_cpp20_requires_clause_decl_boundary
#undef skip_cpp20_constraint_atom
#undef parse_optional_cpp20_requires_clause
#undef parse_optional_cpp20_trailing_requires_clause

namespace c4c {

struct ParserFunctionParamScopeGuard {
    Parser* parser = nullptr;
    bool active = false;

    ParserFunctionParamScopeGuard(Parser& p, const std::vector<Node*>& params)
        : parser(&p) {
        if (params.empty()) return;
        parser->push_local_binding_scope();
        active = true;
        for (Node* param : params) {
            if (!param || !param->name || !param->name[0]) continue;
            parser->register_var_type_binding(
                parser->parser_text_id_for_token(kInvalidText, param->name),
                param->type);
        }
    }

    ~ParserFunctionParamScopeGuard() {
        if (active) parser->pop_local_binding_scope();
    }
};

static void restore_current_struct_tag(Parser& parser, TextId text_id,
                                       const std::string& fallback) {
    const std::string_view tag = parser.parser_text(text_id, fallback);
    if (tag.empty()) {
        parser.clear_current_struct_tag();
        return;
    }
    parser.set_current_struct_tag(tag);
}

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

static QualifiedNameKey alias_template_member_owner_key(
    Parser& parser, const Parser::QualifiedNameRef& owner_qn) {
    if (Node* primary = parser.find_template_struct_primary(owner_qn)) {
        int context_id = primary->namespace_context_id >= 0
                             ? primary->namespace_context_id
                             : parser.current_namespace_context_id();
        TextId name_text_id = primary->unqualified_text_id;
        if (name_text_id == kInvalidText && primary->unqualified_name) {
            name_text_id = parser.parser_text_id_for_token(
                kInvalidText, primary->unqualified_name);
        }
        return parser.alias_template_key_in_context(context_id, name_text_id);
    }

    const int context_id =
        owner_qn.qualifier_segments.empty()
            ? (owner_qn.is_global_qualified ? 0 : parser.current_namespace_context_id())
            : parser.resolve_namespace_context(owner_qn);
    if (context_id < 0) return {};
    return parser.alias_template_key_in_context(context_id, owner_qn.base_text_id);
}

static ParserAliasTemplateMemberTypedefInfo
try_parse_alias_template_member_typedef_info(Parser& parser) {
    ParserAliasTemplateMemberTypedefInfo info;
    Parser::TentativeParseGuard guard(parser);

    if (!parser.match(TokenKind::KwTypename)) return info;

    Parser::QualifiedNameRef owner_qn;
    if (!parser.consume_qualified_type_spelling_with_typename(
            /*require_typename=*/false,
            /*allow_global=*/true,
            /*consume_final_template_args=*/false,
            nullptr,
            &owner_qn)) {
        return info;
    }
    if (owner_qn.base_text_id == kInvalidText || !parser.check(TokenKind::Less)) {
        return info;
    }

    std::vector<Parser::TemplateArgParseResult> owner_args;
    if (!parser.parse_template_argument_list(&owner_args)) return info;
    if (!parser.match(TokenKind::ColonColon)) return info;
    if (parser.check(TokenKind::KwTemplate)) parser.consume();
    if (!parser.check(TokenKind::Identifier)) return info;

    const std::string_view member_name = parser.token_spelling(parser.cur());
    const TextId member_text_id =
        parser.parser_text_id_for_token(parser.cur().text_id, member_name);
    parser.consume();
    if (member_text_id == kInvalidText) return info;

    const QualifiedNameKey owner_key =
        alias_template_member_owner_key(parser, owner_qn);
    if (owner_key.base_text_id == kInvalidText) return info;

    info.valid = true;
    info.owner_key = owner_key;
    info.owner_args = std::move(owner_args);
    info.member_text_id = member_text_id;
    return info;
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
    const Parser::VisibleNameResult lhs_type =
        parser.resolve_visible_type(lhs_text_id, lhs);
    const Parser::VisibleNameResult rhs_type =
        parser.resolve_visible_type(rhs_text_id, rhs);
    if (!lhs_type || !rhs_type) return false;
    if (lhs_type.key.base_text_id != kInvalidText &&
        rhs_type.key.base_text_id != kInvalidText) {
        return lhs_type.key == rhs_type.key;
    }
    return lhs_type.base_text_id != kInvalidText &&
           rhs_type.base_text_id != kInvalidText &&
           lhs_type.base_text_id == rhs_type.base_text_id &&
           lhs_type.context_id == rhs_type.context_id;
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
    (void)parse_expr(parser);
    parser.expect(TokenKind::Semi);
    if (concept_name_text_id != kInvalidText) {
        parser.binding_state_.concept_name_text_ids.insert(concept_name_text_id);
    }
    parser.register_concept_name_in_context(
        parser.current_namespace_context_id(), concept_name_text_id);
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


Node* parse_local_decl(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    int ln = parser.cur().line;
    auto skip_cpp11_attrs_only = [&]() {
        while (parser.check(TokenKind::LBracket) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
            parser.consume();
            parser.consume();
            int depth = 1;
            while (!parser.at_end() && depth > 0) {
                if (parser.check(TokenKind::LBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::LBracket) {
                    parser.consume();
                    parser.consume();
                    ++depth;
                    continue;
                }
                if (parser.check(TokenKind::RBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::RBracket) {
                    parser.consume();
                    parser.consume();
                    --depth;
                    continue;
                }
                parser.consume();
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
        if (parser.match(TokenKind::KwStatic))  { is_static = true; }
        else if (parser.match(TokenKind::KwExtern))  {
            is_extern = true;
            if (const char* spec = maybe_parse_linkage_spec(&parser)) linkage_spec = spec;
        }
        else if (parser.match(TokenKind::KwTypedef)) { is_typedef = true; }
        else if (parser.match(TokenKind::KwRegister)) {}
        else if (parser.match(TokenKind::KwInline))   {}
        else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) { is_constexpr = true; }
        else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConsteval)) { is_consteval = true; }
        else if (parser.match(TokenKind::KwExtension)) {}
        else break;
    }

    // C++ consteval is only valid on functions, not local variable declarations.
    if (is_consteval) {
        throw std::runtime_error(
            std::string("error: 'consteval' can only be applied to a function declaration (line ")
            + std::to_string(ln) + ")");
    }

    TypeSpec base_ts = parser.parse_base_type();
    parser.parse_attributes(&base_ts);

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (ts.tpl_struct_origin) return false;  // pending template struct — resolved at HIR level
        if (ts.tag && parser.is_cpp_mode() &&
            !parser.active_context_state_.current_struct_tag.empty()) {
            if (is_same_visible_type_name(
                    parser, parser.active_context_state_.current_struct_tag_text_id,
                    parser.current_struct_tag_text(), parser.find_parser_text_id(ts.tag),
                    ts.tag))
                return false;
        }
        Node* def = resolve_record_type_spec(
            ts, &parser.definition_state_.struct_tag_def_map);
        if (!def) return true;
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
        parser.parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic, nullptr,
                         nullptr, nullptr, nullptr, &tdname_text_id);
        if (tdname) {
            if (!parser.is_cpp_mode() && !is_internal_typedef_name(tdname) &&
                parser.has_conflicting_user_typedef_binding(tdname_text_id, ts_copy))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            parser.register_typedef_binding(
                tdname_text_id, ts_copy,
                !is_internal_typedef_name(tdname));
            // Phase C: store fn_ptr param info for typedef'd function pointers.
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
                const TextId typedef_name_id =
                    tdname_text_id != kInvalidText
                        ? tdname_text_id
                        : parser.parser_text_id_for_token(kInvalidText, tdname);
                if (typedef_name_id != kInvalidText) {
                    parser.binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                        td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
                }
            }
        }
        // multiple declarators in typedef
        while (parser.match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            TextId tdn2_text_id = kInvalidText;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parser.parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic,
                             nullptr, nullptr, nullptr, nullptr,
                             &tdn2_text_id);
            if (tdn2) {
                if (!parser.is_cpp_mode() && !is_internal_typedef_name(tdn2) &&
                    parser.has_conflicting_user_typedef_binding(tdn2_text_id, ts2))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                parser.register_typedef_binding(
                    tdn2_text_id, ts2,
                    !is_internal_typedef_name(tdn2));
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                    const TextId typedef_name_id =
                        tdn2_text_id != kInvalidText
                            ? tdn2_text_id
                            : parser.parser_text_id_for_token(kInvalidText, tdn2);
                    if (typedef_name_id != kInvalidText) {
                        parser.binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                            td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
                    }
                }
            }
        }
        parser.match(TokenKind::Semi);
        // Keep enum definition nodes for `typedef enum { ... } T;` so
        // downstream semantic passes can bind enumerator constants.
        if (base_ts.base == TB_ENUM && parser.definition_state_.last_enum_def)
            return parser.definition_state_.last_enum_def;
        return parser.make_node(NK_EMPTY, ln);
    }

    // declaration-only tag type in local scope: `enum/struct/union ... ;`
    if (parser.check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        parser.consume();  // consume ;
        if (base_ts.base == TB_ENUM && parser.definition_state_.last_enum_def) {
            return parser.definition_state_.last_enum_def;
        }
        return parser.make_node(NK_EMPTY, ln);
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
        TextId vname_text_id = kInvalidText;
        parser.parse_declarator(ts, &vname, &fn_ptr_params, &n_fn_ptr_params, &fn_ptr_variadic,
                         nullptr,
                         &ret_fn_ptr_params, &n_ret_fn_ptr_params, &ret_fn_ptr_variadic,
                         &vname_text_id);
        // C++ constructor invocation: `Type var(args)` where Type is a struct.
        // In C mode this is a K&R function-type suffix that we skip.
        bool is_kr_fn_decl = false;
        bool is_ctor_init = false;
        std::vector<Node*> ctor_args;
        if (parser.check(TokenKind::LParen)) {
            bool parsed_as_function_decl = false;
            if (parser.is_cpp_mode() && vname) {
                enum class CtorInitVisibleHeadHandoff {
                    None,
                    PreferCtorInit,
                    PreferFunctionDecl,
                };

                bool single_value_arg = false;
                if (parser.core_input_state_.pos + 2 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::Identifier &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                        TokenKind::RParen) {
                    const Token& arg_tok = parser.core_input_state_.tokens[parser.core_input_state_.pos + 1];
                    const TextId arg_text_id = arg_tok.text_id;
                    const std::string arg_name = std::string(parser.token_spelling(arg_tok));
                    const bool arg_is_type =
                        is_known_simple_visible_type_head(parser, arg_text_id, arg_name);
                    single_value_arg = !arg_is_type;
                }
                // Keep zero-arg identifier calls on the expression side when
                // the callee head is not already known as a type. This avoids
                // misreading direct-init such as `bool flag(IsPositive());` as
                // a function declaration while preserving the classic vexing
                // parse for known type heads.
                if (!single_value_arg && parser.is_cpp_mode() &&
                    parser.core_input_state_.pos + 4 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::Identifier &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                        TokenKind::LParen &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 3].kind ==
                        TokenKind::RParen &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 4].kind ==
                        TokenKind::RParen) {
                    const Token& arg_tok =
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1];
                    const std::string arg_name =
                        std::string(parser.token_spelling(arg_tok));
                    if (!is_known_simple_visible_type_head(parser, arg_tok.text_id,
                                                           arg_name)) {
                        single_value_arg = true;
                    }
                }
                auto can_use_lite_ctor_init_probe = [&]() -> bool {
                    if (parser.core_input_state_.pos + 1 >=
                        static_cast<int>(parser.core_input_state_.tokens.size())) {
                        return false;
                    }

                    switch (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind) {
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
                        case TokenKind::RParen:
                            return false;
                        case TokenKind::Identifier: {
                            const Token& arg_tok =
                                parser.core_input_state_.tokens[parser.core_input_state_.pos + 1];
                            const TextId arg_text_id = arg_tok.text_id;
                            const std::string arg_name = std::string(parser.token_spelling(arg_tok));
                            const bool arg_is_type =
                                is_known_simple_visible_type_head(parser, arg_text_id, arg_name);
                            if (arg_is_type) return false;
                            if (single_value_arg) return true;
                            if (parser.classify_visible_value_or_type_head(
                                    parser.core_input_state_.pos + 1) > 0) {
                                return true;
                            }
                            // If the first argument already has unresolved
                            // parameter-type shape, use the heavy tentative
                            // parse so any parser-side scope/bookkeeping
                            // mutations roll back cleanly on a ctor-init path.
                            if (parser.looks_like_unresolved_identifier_type_head(
                                    parser.core_input_state_.pos + 1)) {
                                return false;
                            }
                            if (parser.looks_like_unresolved_parenthesized_parameter_type_head(
                                    parser.core_input_state_.pos + 1)) {
                                return false;
                            }
                            if (parser.core_input_state_.pos + 2 <
                                    static_cast<int>(parser.core_input_state_.tokens.size()) &&
                                (parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                                     TokenKind::ColonColon ||
                                 parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                                     TokenKind::Less)) {
                                return false;
                            }
                            return true;
                        }
                        case TokenKind::ColonColon:
                            return parser.classify_visible_value_or_type_head(
                                       parser.core_input_state_.pos + 1) > 0;
                        default:
                            return false;
                    }
                };

                auto classify_ctor_init_visible_head_handoff =
                    [&]() -> CtorInitVisibleHeadHandoff {
                    const int visible_head_kind =
                        parser.classify_visible_value_or_type_head(
                            parser.core_input_state_.pos + 1);
                    if (visible_head_kind > 0) {
                        return CtorInitVisibleHeadHandoff::PreferCtorInit;
                    }
                    if (visible_head_kind < 0) {
                        return CtorInitVisibleHeadHandoff::PreferFunctionDecl;
                    }
                    return CtorInitVisibleHeadHandoff::None;
                };

                auto probe_ctor_vs_function_decl = [&]() -> bool {
                    if (parser.core_input_state_.pos + 3 <
                            static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                            TokenKind::Identifier &&
                        !is_known_simple_visible_type_head(
                            parser,
                            parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].text_id,
                            parser.token_spelling(
                                parser.core_input_state_.tokens[parser.core_input_state_.pos + 1])) &&
                        (parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                             TokenKind::Amp ||
                         parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                             TokenKind::AmpAmp ||
                         parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                             TokenKind::Star) &&
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 3].kind ==
                            TokenKind::Identifier) {
                        return false;
                    }
                    if (single_value_arg) {
                        return false;
                    }
                    const CtorInitVisibleHeadHandoff visible_head_handoff =
                        classify_ctor_init_visible_head_handoff();
                    if (visible_head_handoff ==
                        CtorInitVisibleHeadHandoff::PreferCtorInit) {
                        return false;
                    }
                    if (visible_head_handoff ==
                        CtorInitVisibleHeadHandoff::PreferFunctionDecl) {
                        return true;
                    }
                    if (parser.core_input_state_.pos + 2 <
                            static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                            TokenKind::Identifier &&
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                            TokenKind::Identifier &&
                        parser.looks_like_unresolved_identifier_type_head(
                            parser.core_input_state_.pos + 1)) {
                        return true;
                    }

                    std::vector<Node*> probe_params;
                    std::vector<const char*> probe_knr_names;
                    bool probe_variadic = false;
                    parse_top_level_parameter_list(
                        parser, &probe_params, &probe_knr_names,
                        &probe_variadic);
                    if (single_value_arg ||
                        (!parser.check(TokenKind::Semi) && !parser.check(TokenKind::Comma))) {
                        return false;
                    }
                    return !(parser.is_cpp_mode() && !probe_knr_names.empty());
                };

                try {
                    if (can_use_lite_ctor_init_probe()) {
                        Parser::TentativeParseGuardLite guard(parser);
                        parsed_as_function_decl = probe_ctor_vs_function_decl();
                    } else {
                        Parser::TentativeParseGuard guard(parser);
                        parsed_as_function_decl = probe_ctor_vs_function_decl();
                    }
                } catch (const std::exception&) {
                }
            }
            if (parser.is_cpp_mode() && vname && !parsed_as_function_decl) {
                // C++ direct-initialization: parse constructor-style arguments.
                parser.consume();  // '('
                if (!parser.check(TokenKind::RParen)) {
                    while (!parser.at_end()) {
                        if (parser.check(TokenKind::RParen)) break;
                        Node* arg = parse_assign_expr(parser);
                        if (arg) ctor_args.push_back(arg);
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }
                parser.expect(TokenKind::RParen);
                is_ctor_init = true;
            } else {
                // K&R-style function-type suffix: `float fx ()` in local decls.
                parser.skip_paren_group();
                is_kr_fn_decl = true;
            }
        }
        parser.skip_attributes();
        parser.skip_asm();
        parser.skip_attributes();

        if (!vname) {
            // Abstract declarator in statement position — skip
            parser.match(TokenKind::Semi);
            return parser.make_node(NK_EMPTY, ln);
        }

        if (is_kr_fn_decl) continue;  // K&R fn decl: no local variable

        if (is_incomplete_object_type(ts)) {
            throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
        }

        Node* init_node = nullptr;
        if (!is_ctor_init) {
            if (parser.match(TokenKind::Assign) ||
                (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) {
                init_node = parse_initializer(parser);
            }
        }

        Node* d = parser.make_node(NK_DECL, ln);
        d->type      = ts;
        if (is_constexpr) d->type.is_const = true;
        d->name      = vname;
        d->unqualified_name = vname;
        d->unqualified_text_id =
            parser.parser_text_id_for_token(vname_text_id, vname);
        d->init      = init_node;
        d->is_ctor_init = is_ctor_init;
        if (is_ctor_init && !ctor_args.empty()) {
            d->n_children = (int)ctor_args.size();
            d->children = parser.arena_.alloc_array<Node*>(d->n_children);
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
            if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                d->fn_ptr_params = info->params;
                d->n_fn_ptr_params = info->n_params;
                d->fn_ptr_variadic = info->variadic;
            }
        }
        if (vname && !parser.active_context_state_.suppress_local_var_bindings) {
            parser.register_var_type_binding(d->unqualified_text_id, ts);
        }
        decls.push_back(d);
    } while (parser.match(TokenKind::Comma));

    parser.match(TokenKind::Semi);

    if (decls.empty()) return parser.make_node(NK_EMPTY, ln);
    if (decls.size() == 1) return decls[0];

    // Multiple declarators in one declaration statement (e.g. `int a, b;`):
    // wrap as a synthetic block so statement shape stays single-node.
    // Mark it via ival=1 so IR emission can keep the surrounding scope.
    Node* blk = parser.make_node(NK_BLOCK, ln);
    blk->ival = 1;
    blk->n_children = (int)decls.size();
    blk->children   = parser.arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = decls[i];
    return blk;
}

// ── top-level parsing ─────────────────────────────────────────────────────────

void parse_top_level_parameter_list(
    Parser& parser,
    std::vector<Node*>* out_params,
    std::vector<const char*>* out_knr_param_names,
    bool* out_variadic) {
    Parser::ParseContextGuard trace(&parser, __func__);
    if (out_params) out_params->clear();
    if (out_knr_param_names) out_knr_param_names->clear();
    if (out_variadic) *out_variadic = false;

    parser.expect(TokenKind::LParen);
    if (!parser.check(TokenKind::RParen)) {
        while (!parser.at_end()) {
            if (parser.check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                parser.consume();
                break;
            }
            if (parser.check(TokenKind::RParen)) break;
            if (!parser.is_type_start() && !parser.can_start_parameter_type() &&
                parser.check(TokenKind::Identifier)) {
                if (out_knr_param_names)
                    out_knr_param_names->push_back(
                        parser.arena_.strdup(std::string(parser.token_spelling(parser.cur()))));
                parser.consume();
                if (parser.match(TokenKind::Comma)) continue;
                break;
            }
            if (!parser.can_start_parameter_type()) break;
            Node* p = parse_param(parser);
            if (p && out_params) out_params->push_back(p);
            if (parser.check(TokenKind::Ellipsis)) {
                if (out_variadic) *out_variadic = true;
                parser.consume();
                break;
            }
            if (!parser.match(TokenKind::Comma)) break;
        }
    }
    parser.expect(TokenKind::RParen);
}

Node* parse_static_assert_declaration(Parser& parser) {
    const int ln = parser.cur().line;
    parser.consume();  // static_assert / _Static_assert
    parser.expect(TokenKind::LParen);
    Node* cond = parse_assign_expr(parser);
    Node* msg = nullptr;
    if (parser.match(TokenKind::Comma)) {
        msg = parse_assign_expr(parser);
    }
    parser.expect(TokenKind::RParen);
    parser.match(TokenKind::Semi);
    Node* sa = parser.make_node(NK_STATIC_ASSERT, ln);
    sa->left = cond;
    sa->right = msg;
    return sa;
}

Node* parse_top_level(Parser& parser) {
    Parser::ParseContextGuard trace(&parser, __func__);
    struct TopLevelFlagGuard {
        bool& ref;
        bool old;
        explicit TopLevelFlagGuard(bool& r) : ref(r), old(r) { ref = true; }
        ~TopLevelFlagGuard() { ref = old; }
    } top_level_guard(parser.active_context_state_.parsing_top_level_context);

    int ln = parser.cur().line;
    if (parser.at_end()) return nullptr;
    auto skip_cpp11_attrs_only = [&]() -> bool {
        bool consumed_any = false;
        while (parser.check(TokenKind::LBracket) &&
               parser.core_input_state_.pos + 1 <
                   static_cast<int>(parser.core_input_state_.tokens.size()) &&
               parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                   TokenKind::LBracket) {
            consumed_any = true;
            parser.consume();
            parser.consume();
            int depth = 1;
            while (!parser.at_end() && depth > 0) {
                if (parser.check(TokenKind::LBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::LBracket) {
                    parser.consume();
                    parser.consume();
                    ++depth;
                    continue;
                }
                if (parser.check(TokenKind::RBracket) &&
                    parser.core_input_state_.pos + 1 <
                        static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                        TokenKind::RBracket) {
                    parser.consume();
                    parser.consume();
                    --depth;
                    continue;
                }
                parser.consume();
            }
        }
        return consumed_any;
    };
    if (skip_cpp11_attrs_only() && parser.check(TokenKind::Semi)) {
        parser.consume();
        return nullptr;
    }

    bool is_inline_namespace = false;
    if (parser.is_cpp_mode() && parser.check(TokenKind::KwInline) && parser.peek(1).kind == TokenKind::KwNamespace) {
        is_inline_namespace = true;
        parser.consume();
    }
    if (parser.is_cpp_mode() && parser.check(TokenKind::KwNamespace)) {
        parser.consume();
        Parser::QualifiedNameRef ns_name;
        bool has_name = false;
        if (parser.check(TokenKind::Identifier)) {
            ns_name = parser.parse_qualified_name(false);
            has_name = true;
        }
        parser.skip_attributes();
        parser.expect(TokenKind::LBrace);

        int context_id = parser.current_namespace_context_id();
        if (has_name) {
            for (size_t i = 0; i < ns_name.qualifier_segments.size(); ++i) {
                const TextId segment_text_id =
                    i < ns_name.qualifier_text_ids.size()
                        ? ns_name.qualifier_text_ids[i]
                        : kInvalidText;
                context_id = parser.ensure_named_namespace_context(
                    context_id, segment_text_id, ns_name.qualifier_segments[i]);
            }
            context_id = parser.ensure_named_namespace_context(
                context_id, ns_name.base_text_id, ns_name.base_name);
        } else {
            context_id = parser.create_anonymous_namespace_context(context_id);
        }
        parser.push_namespace_context(context_id);

        std::vector<Node*> items;
        while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
            Node* item = parse_top_level(parser);
            if (item) items.push_back(item);
        }
        parser.expect(TokenKind::RBrace);
        parser.match(TokenKind::Semi);
        parser.pop_namespace_context();

        if (items.empty()) return nullptr;
        if (items.size() == 1) return items[0];
        Node* blk = parser.make_node(NK_BLOCK, ln);
        blk->n_children = (int)items.size();
        blk->children = parser.arena_.alloc_array<Node*>(blk->n_children);
        for (int i = 0; i < blk->n_children; ++i) blk->children[i] = items[i];
        return blk;
    }
    if (is_inline_namespace) {
        // We only consumed 'inline' for the `inline namespace` form above.
        throw std::runtime_error("expected namespace after inline");
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::KwUsing)) {
        parser.consume();
        const int using_context_id = parser.current_namespace_context_id();
        if (parser.match(TokenKind::KwNamespace)) {
            if (!parser.check(TokenKind::Identifier))
                throw std::runtime_error("expected namespace name after 'using namespace'");
            Parser::QualifiedNameRef target_name = parser.parse_qualified_name(true);
            int target_context = parser.resolve_namespace_name(target_name);
            if (target_context < 0) {
                throw std::runtime_error("unknown namespace in using-directive");
            }
            parser.namespace_state_.using_namespace_contexts[using_context_id].push_back(target_context);
            recover_top_level_decl_terminator_or_boundary(parser, ln);
            return nullptr;
        }

        if (!parser.check(TokenKind::Identifier) && !parser.check(TokenKind::ColonColon))
            throw std::runtime_error("expected identifier after 'using'");

        Parser::QualifiedNameRef target_name;
        if (parser.match(TokenKind::ColonColon)) {
            target_name.is_global_qualified = true;
            if (!parser.check(TokenKind::Identifier))
                throw std::runtime_error("expected identifier after '::'");
            target_name.base_name = std::string(parser.token_spelling(parser.cur()));
            target_name.base_text_id =
                parser.parser_text_id_for_token(parser.cur().text_id, target_name.base_name);
            parser.consume();
        } else {
            std::string first_name = std::string(parser.token_spelling(parser.cur()));
            parser.consume();

            if (parser.match(TokenKind::Assign)) {
                const int alias_type_pos = parser.pos_;
                const ParserAliasTemplateMemberTypedefInfo member_typedef_info =
                    try_parse_alias_template_member_typedef_info(parser);
                TypeSpec alias_ts{};
                try {
                    alias_ts = parser.parse_type_name();
                    if (!alias_ts.tpl_struct_origin &&
                        alias_type_pos < parser.core_input_state_.pos &&
                        parser.core_input_state_.tokens[alias_type_pos].kind ==
                            TokenKind::KwTypename) {
                        auto skip_balanced_template_args = [&](int* pos) -> bool {
                            if (!pos ||
                                *pos < 0 ||
                                *pos >=
                                    static_cast<int>(parser.core_input_state_.tokens.size()) ||
                                parser.core_input_state_.tokens[*pos].kind != TokenKind::Less) {
                                return false;
                            }
                            int depth = 0;
                            while (*pos < static_cast<int>(parser.core_input_state_.tokens.size())) {
                                const TokenKind kind = parser.core_input_state_.tokens[*pos].kind;
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
                                out += parser.token_spelling(parser.core_input_state_.tokens[i]);
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
                                switch (parser.core_input_state_.tokens[i].kind) {
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
                                   (parser.core_input_state_.tokens[start].kind ==
                                        TokenKind::KwTypename ||
                                    parser.core_input_state_.tokens[start].kind ==
                                        TokenKind::KwClass)) {
                                ++start;
                            }
                            if (start >= end) return {};

                            for (int i = start; i < end; ++i) {
                                if (parser.core_input_state_.tokens[i].kind != TokenKind::Less) continue;
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

                            if (parser.core_input_state_.tokens[start].kind == TokenKind::IntLit ||
                                parser.core_input_state_.tokens[start].kind == TokenKind::FloatLit ||
                                parser.core_input_state_.tokens[start].kind == TokenKind::CharLit ||
                                parser.core_input_state_.tokens[start].kind == TokenKind::KwTrue ||
                                parser.core_input_state_.tokens[start].kind == TokenKind::KwFalse) {
                                return join_token_lexemes(start, end);
                            }

                            std::string prefix;
                            while (start < end &&
                                   (parser.core_input_state_.tokens[start].kind == TokenKind::KwConst ||
                                    parser.core_input_state_.tokens[start].kind ==
                                        TokenKind::KwVolatile)) {
                                if (parser.core_input_state_.tokens[start].kind == TokenKind::KwConst)
                                    prefix += "const_";
                                if (parser.core_input_state_.tokens[start].kind ==
                                    TokenKind::KwVolatile)
                                    prefix += "volatile_";
                                ++start;
                            }

                            std::string base;
                            while (start < end &&
                                   (parser.core_input_state_.tokens[start].kind ==
                                        TokenKind::Identifier ||
                                    parser.core_input_state_.tokens[start].kind ==
                                        TokenKind::ColonColon)) {
                                base += parser.token_spelling(parser.core_input_state_.tokens[start]);
                                ++start;
                            }
                            if (base.empty()) return join_token_lexemes(start, end);

                            std::string suffix;
                            for (int i = start; i < end; ++i) {
                                if (parser.core_input_state_.tokens[i].kind == TokenKind::Star)
                                    suffix += "_ptr";
                                if (parser.core_input_state_.tokens[i].kind == TokenKind::Amp)
                                    suffix += "_ref";
                                if (parser.core_input_state_.tokens[i].kind == TokenKind::AmpAmp)
                                    suffix += "_rref";
                            }
                            return prefix + base + suffix;
                        };

                        int probe = alias_type_pos + 1;  // skip typename
                        bool owner_global_qualified = false;
                        if (probe < parser.core_input_state_.pos &&
                            parser.core_input_state_.tokens[probe].kind ==
                                TokenKind::ColonColon) {
                            owner_global_qualified = true;
                            ++probe;
                        }
                        std::vector<std::string> qualifier_segments;
                        std::vector<TextId> qualifier_text_ids;
                        int owner_template_arg_start = -1;
                        int owner_template_arg_end = -1;
                        std::string member_name;
                        while (probe < parser.core_input_state_.pos) {
                            if (parser.core_input_state_.tokens[probe].kind != TokenKind::Identifier)
                                break;
                            const Token& segment_token = parser.core_input_state_.tokens[probe];
                            const std::string segment =
                                std::string(parser.token_spelling(segment_token));
                            const TextId segment_text_id =
                                parser.parser_text_id_for_token(segment_token.text_id, segment);
                            ++probe;
                            if (probe < parser.core_input_state_.pos &&
                                parser.core_input_state_.tokens[probe].kind == TokenKind::Less) {
                                owner_template_arg_start = probe;
                                if (!skip_balanced_template_args(&probe)) break;
                                owner_template_arg_end = probe;
                            }
                            if (probe < parser.core_input_state_.pos &&
                                parser.core_input_state_.tokens[probe].kind ==
                                    TokenKind::ColonColon) {
                                ++probe;
                                if (probe < parser.core_input_state_.pos &&
                                    parser.core_input_state_.tokens[probe].kind ==
                                        TokenKind::KwTemplate) {
                                    ++probe;
                                }
                                if (probe < parser.core_input_state_.pos &&
                                    parser.core_input_state_.tokens[probe].kind ==
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
                            Parser::QualifiedNameRef owner_qn;
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
                                resolve_qualified_known_type_name(parser, owner_qn);

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
                            alias_ts.tag = parser.arena_.strdup(owner_name.c_str());
                            alias_ts.tpl_struct_origin = alias_ts.tag;
                            if (!owner_arg_refs.empty()) {
                                alias_ts.tpl_struct_args.data =
                                    parser.arena_.alloc_array<TemplateArgRef>(1);
                                alias_ts.tpl_struct_args.size = 1;
                                alias_ts.tpl_struct_args.data[0].kind =
                                    TemplateArgKind::Type;
                                alias_ts.tpl_struct_args.data[0].type = {};
                                alias_ts.tpl_struct_args.data[0].value = 0;
                                alias_ts.tpl_struct_args.data[0].debug_text =
                                    parser.arena_.strdup(owner_arg_refs.c_str());
                            }
                            alias_ts.deferred_member_type_name =
                                parser.arena_.strdup(member_name.c_str());
                        }
                    }
                } catch (const std::exception&) {
                    parser.pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(parser, ln);
                    return nullptr;
                }
                if (using_alias_consumed_following_declaration(parser,
                                                               alias_type_pos,
                                                               parser.pos_,
                                                               ln)) {
                    parser.pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(parser, ln);
                    return nullptr;
                }
                if (!parser.match(TokenKind::Semi)) {
                    parser.pos_ = alias_type_pos;
                    recover_top_level_using_alias_or_boundary(parser, ln);
                    return nullptr;
                }
                const TextId alias_name_text_id =
                    parser.parser_text_id_for_token(kInvalidText, first_name);
                const QualifiedNameKey alias_key = parser.alias_template_key_in_context(
                    using_context_id, alias_name_text_id);
                const std::string qualified = parser.bridge_name_in_context(
                    using_context_id, alias_name_text_id, first_name);
                parser.register_structured_typedef_binding_in_context(
                    using_context_id, alias_name_text_id, alias_ts);
                parser.register_typedef_binding(
                    parser.parser_text_id_for_token(kInvalidText, qualified),
                    alias_ts, true);
                if (using_context_id == 0) {
                    parser.register_typedef_binding(alias_name_text_id, alias_ts, true);
                }
                parser.set_last_using_alias_name(alias_key);
                parser.active_context_state_.last_using_alias_member_typedef =
                    member_typedef_info;
                return nullptr;
            }
            target_name.base_name = std::move(first_name);
            target_name.base_text_id =
                parser.parser_text_id_for_token(kInvalidText, target_name.base_name);
        }
        while (parser.match(TokenKind::ColonColon)) {
            if (!parser.check(TokenKind::Identifier))
                throw std::runtime_error("expected identifier after '::'");
            target_name.qualifier_segments.push_back(target_name.base_name);
            target_name.qualifier_text_ids.push_back(target_name.base_text_id);
            target_name.base_name = std::string(parser.token_spelling(parser.cur()));
            target_name.base_text_id =
                parser.parser_text_id_for_token(parser.cur().text_id, target_name.base_name);
            parser.consume();
        }

        const std::string imported_name =
            std::string(parser.parser_text(target_name.base_text_id, target_name.base_name));
        const Parser::VisibleNameResult imported_type =
            parser.resolve_qualified_type(target_name);
        if (imported_type) {
            const std::string imported_type_name =
                parser.visible_name_spelling(imported_type);
            const TypeSpec* imported_typedef =
                parser.find_typedef_type(imported_type.key);
            if (imported_typedef) {
                const std::string imported_key = parser.bridge_name_in_context(
                    using_context_id, target_name.base_text_id, imported_name);
                parser.register_structured_typedef_binding_in_context(
                    using_context_id, target_name.base_text_id, *imported_typedef);
                parser.register_typedef_binding(
                    parser.parser_text_id_for_token(kInvalidText, imported_key),
                    *imported_typedef, true);
                if (using_context_id == 0) {
                    parser.register_typedef_binding(target_name.base_text_id, *imported_typedef, true);
                }
                recover_top_level_decl_terminator_or_boundary(parser, ln);
                return nullptr;
            }
        }

        const QualifiedNameKey imported_value_key = parser.qualified_name_key(target_name);
        const std::string imported_key = parser.bridge_name_in_context(
            using_context_id, target_name.base_text_id, imported_name);
        std::string imported_value_name;
        if (parser.shared_lookup_state_.token_texts) {
            imported_value_name = render_qualified_name(
                imported_value_key, parser.shared_lookup_state_.parser_name_paths,
                *parser.shared_lookup_state_.token_texts);
        }
        const TypeSpec* imported_var =
            parser.find_var_type(imported_value_key);
        QualifiedNameKey imported_alias_key = imported_value_key;
        bool imported_known_fn = parser.has_known_fn_name(imported_value_key);
        if (!imported_var || !imported_known_fn) {
            const Parser::VisibleNameResult imported_value =
                parser.resolve_qualified_value(target_name);
            const std::string resolved_value_name =
                parser.visible_name_spelling(imported_value);
            if (imported_value) {
                if (!imported_var) {
                    const TypeSpec* resolved_var =
                        parser.find_var_type(imported_value.key);
                    if (resolved_var) {
                        imported_var = resolved_var;
                        imported_alias_key = imported_value.key;
                    }
                }
                if (!imported_known_fn &&
                    parser.has_known_fn_name(imported_value.key)) {
                    imported_known_fn = true;
                    imported_alias_key = imported_value.key;
                }
                if (!resolved_value_name.empty()) {
                    imported_value_name = resolved_value_name;
                }
            }
            if (imported_value_name.empty()) {
                imported_value_name = parser.qualified_name_text(target_name);
            }
            if (imported_value_name.empty()) {
                imported_value_name = imported_key;
                if (imported_value_name.rfind("::", 0) == 0) {
                    imported_value_name.erase(0, 2);
                }
            }
            if (!imported_var && !imported_value_name.empty()) {
                imported_var = parser.find_var_type(parser.find_parser_text_id(imported_value_name));
            }
        }
        {
            if (imported_var) {
                parser.register_var_type_binding(
                    parser.parser_text_id_for_token(kInvalidText, imported_key),
                    *imported_var);
            }
            if (imported_known_fn) {
                parser.register_known_fn_name_in_context(
                    using_context_id, target_name.base_text_id);
            }
            parser.namespace_state_.using_value_aliases[using_context_id]
                                               [target_name.base_text_id] = {
                imported_alias_key, imported_value_name};
        }
        recover_top_level_decl_terminator_or_boundary(parser, ln);
        return nullptr;
    }

    if (parser.is_cpp_mode() && parser.check(TokenKind::KwTemplate)) {
        parser.consume();
        parser.expect(TokenKind::Less);

        // Explicit template specialization: template<> RetType name<Args>(...) { ... }
        if (parser.check(TokenKind::Greater)) {
            parser.consume();  // consume >
            bool saved_spec =
                parser.active_context_state_.parsing_explicit_specialization;
            parser.active_context_state_.parsing_explicit_specialization = true;
            size_t struct_defs_before = parser.definition_state_.struct_defs.size();
            Node* spec = parse_top_level(parser);
            parser.active_context_state_.parsing_explicit_specialization = saved_spec;
            if (parser.definition_state_.struct_defs.size() > struct_defs_before) {
                Node* last_sd = parser.definition_state_.struct_defs.back();
                if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                    last_sd->template_origin_name && last_sd->n_template_args > 0) {
                    parser.register_template_struct_specialization(
                        parser.current_namespace_context_id(),
                        parser.parser_text_id_for_token(
                            kInvalidText, last_sd->template_origin_name),
                        last_sd);
                }
            }
            if (spec) spec->is_explicit_specialization = true;
            return spec;
        }

        Parser::TemplateDeclarationPreludeGuard template_prelude_guard(&parser);
        std::vector<const char*> template_params;
        std::vector<TextId> template_param_name_text_ids;
        std::vector<bool> template_param_nttp;  // true if non-type template param
        std::vector<bool> template_param_is_pack;
        std::vector<bool> template_param_has_default;
        std::vector<TypeSpec> template_param_default_types;
        std::vector<long long> template_param_default_values;
        std::vector<const char*> template_param_default_exprs;
        // Deferred NTTP default expression tokens per parameter index.
        std::unordered_map<int, std::vector<Token>> deferred_nttp_defaults;
        auto push_type_template_param =
            [&](const char* pname, TextId pname_text_id = kInvalidText,
                bool is_pack = false,
                bool has_default = false, const TypeSpec* default_ts = nullptr) {
                TextId ast_pname_text_id = pname_text_id;
                if (!pname || !pname[0]) {
                    pname = make_anon_template_param_name(parser.arena_, false, template_params.size());
                    ast_pname_text_id = kInvalidText;
                }
                if (pname && pname[0]) {
                    const TextId binding_text_id =
                        ast_pname_text_id != kInvalidText
                            ? ast_pname_text_id
                            : parser.parser_text_id_for_token(kInvalidText, pname);
                    parser.register_synthesized_typedef_binding(binding_text_id);
                    template_prelude_guard.injected_type_params.push_back(
                        {binding_text_id, pname});
                }
                template_params.push_back(pname);
                template_param_name_text_ids.push_back(ast_pname_text_id);
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
        auto push_nttp_param =
            [&](const char* pname, TextId pname_text_id = kInvalidText,
                bool is_pack = false) {
            TextId ast_pname_text_id = pname_text_id;
            if (!pname || !pname[0]) {
                pname = make_anon_template_param_name(parser.arena_, true, template_params.size());
                ast_pname_text_id = kInvalidText;
            }
            template_params.push_back(pname);
            template_param_name_text_ids.push_back(ast_pname_text_id);
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
                std::vector<Token> saved_toks(parser.tokens_.begin() + start_pos,
                                             parser.tokens_.begin() + parser.pos_);
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
                        expr << parser.token_spelling(deferred_nttp_defaults[param_idx][tok_i]);
                    }
                    template_param_default_exprs.push_back(parser.arena_.strdup(expr.str()));
                } else {
                    template_param_has_default.push_back(false);
                    template_param_default_exprs.push_back(nullptr);
                }
            };
        struct ParsedTemplateParamName {
            bool is_pack;
            const char* name;
            TextId name_text_id;
        };
        auto parse_template_param_pack_and_name = [&]() {
            bool is_pack = false;
            if (parser.check(TokenKind::Ellipsis)) { parser.consume(); is_pack = true; }
            const char* pname = nullptr;
            TextId pname_text_id = kInvalidText;
            if (parser.check(TokenKind::Identifier)) {
                const std::string spelling =
                    std::string(parser.token_spelling(parser.cur()));
                pname_text_id =
                    parser.parser_text_id_for_token(parser.cur().text_id, spelling);
                pname = parser.arena_.strdup(spelling);
                parser.consume();
            }
            return ParsedTemplateParamName{is_pack, pname, pname_text_id};
        };
        auto try_consume_typedef_nttp_type_head = [&]() -> bool {
            if (parser.is_cpp_mode()) {
                Parser::TentativeParseGuard guard(parser);
                std::string head_name;
                if (parser.consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &head_name, nullptr) &&
                    parser.is_concept_name(head_name)) {
                    return false;
                }
            }
            return parser.consume_template_parameter_type_start(/*allow_typename_keyword=*/true);
        };
        while (!parser.at_end() && !parser.check(TokenKind::Greater)) {
            if (parser.check(TokenKind::KwTemplate)) {
                // Template-template parameter:
                //   template<typename...> class Op
                // For the current frontend, treat the declared name as a
                // dependent type parameter and only consume the nested
                // template parameter clause plus any default argument.
                parser.consume();  // template
                parser.expect(TokenKind::Less);
                int nested_depth = 1;
                while (!parser.at_end() && nested_depth > 0) {
                    if (parser.check(TokenKind::Less)) {
                        ++nested_depth;
                        parser.consume();
                        continue;
                    }
                    if (parser.check_template_close()) {
                        parser.match_template_close();
                        --nested_depth;
                        if (nested_depth == 0) break;
                        continue;
                    }
                    parser.consume();
                }
                if (!parser.check(TokenKind::KwTypename) &&
                    !parser.check(TokenKind::KwClass)) {
                    throw std::runtime_error("expected template parameter name");
                }
                parser.consume();  // typename/class
                auto param_name = parse_template_param_pack_and_name();
                if (parser.match(TokenKind::Assign)) {
                    skip_template_param_default_expr(parser, true);
                }
                push_type_template_param(param_name.name,
                                         param_name.name_text_id,
                                         param_name.is_pack);
            } else if (parser.check(TokenKind::KwTypename) ||
                parser.check(TokenKind::KwClass)) {
                if (parser.classify_typename_template_parameter() ==
                    Parser::TypenameTemplateParamKind::TypedNonTypeParameter) {
                    TypeSpec param_ts = parser.parse_type_name();
                    (void)param_ts;
                    while (parser.check(TokenKind::Star) || is_qualifier(parser.cur().kind)) parser.consume();
                    auto param_name = parse_template_param_pack_and_name();
                    push_nttp_param(param_name.name,
                                    param_name.name_text_id,
                                    param_name.is_pack);
                    if (parser.match(TokenKind::Assign)) {
                        skip_template_param_default_expr(parser, false);
                    }
                    push_nttp_no_default();
                    continue;
                }
                // Type template parameter (possibly variadic: typename... Ts).
                parser.consume();
                auto param_name = parse_template_param_pack_and_name();
                // Check for default type argument: = type
                if (parser.match(TokenKind::Assign)) {
                    TypeSpec def_ts = parser.parse_type_name();
                    push_type_template_param(param_name.name,
                                             param_name.name_text_id,
                                             param_name.is_pack, true, &def_ts);
                } else {
                    push_type_template_param(param_name.name,
                                             param_name.name_text_id,
                                             param_name.is_pack);
                }
            } else if (is_type_kw(parser.cur().kind)) {
                // Non-type template parameter (NTTP): e.g. int N, unsigned N, bool... Bn.
                // Consume the type specifier tokens, then the parameter name.
                while (!parser.at_end() && is_type_kw(parser.cur().kind)) parser.consume();
                auto param_name = parse_template_param_pack_and_name();
                push_nttp_param(param_name.name,
                                param_name.name_text_id,
                                param_name.is_pack);
                // Check for default NTTP value: = expr
                if (parser.match(TokenKind::Assign)) {
                    long long sign = 1;
                    if (parser.match(TokenKind::Minus)) sign = -1;
                    if (parser.check(TokenKind::KwTrue) || parser.check(TokenKind::KwFalse)) {
                        long long val = parser.check(TokenKind::KwTrue) ? 1 : 0;
                        parser.consume();
                        push_nttp_default_value(val);
                    } else if (parser.check(TokenKind::IntLit)) {
                        long long val =
                            parse_int_lexeme(std::string(parser.token_spelling(parser.cur())).c_str());
                        parser.consume();
                        push_nttp_default_value(val * sign);
                    } else {
                        // Complex default expression — skip balanced tokens.
                        // Save the tokens for deferred evaluation during instantiation.
                        // A > at depth 0 closes the template param list UNLESS
                        // followed by :: or a binary operator (the expression continues).
                        int start_pos = parser.pos_;
                        skip_template_param_default_expr(parser, false, &start_pos);
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
                while (parser.check(TokenKind::Star) || is_qualifier(parser.cur().kind)) parser.consume();
                auto param_name = parse_template_param_pack_and_name();
                push_nttp_param(param_name.name,
                                param_name.name_text_id,
                                param_name.is_pack);
                // Check for default NTTP value: = expr
                if (parser.match(TokenKind::Assign)) {
                    skip_template_param_default_expr(parser, false);
                }
                push_nttp_no_default();
            } else if (parser.is_cpp_mode()) {
                Parser::TentativeParseGuard guard(parser);
                std::string constraint_name;
                if (parser.consume_qualified_type_spelling(
                        /*allow_global=*/true,
                        /*consume_final_template_args=*/true,
                        &constraint_name, nullptr) &&
                    (parser.check(TokenKind::Ellipsis) || parser.check(TokenKind::Identifier))) {
                    // C++20 constrained type parameter:
                    //   template<Constraint T>
                    //   template<ns::Constraint T = int>
                    auto param_name = parse_template_param_pack_and_name();
                    if (parser.match(TokenKind::Assign)) {
                        TypeSpec def_ts = parser.parse_type_name();
                        push_type_template_param(param_name.name,
                                                 param_name.name_text_id,
                                                 param_name.is_pack, true, &def_ts);
                    } else {
                        push_type_template_param(param_name.name,
                                                 param_name.name_text_id,
                                                 param_name.is_pack);
                    }
                    guard.commit();
                } else {
                    // guard restores parser.pos_ on scope exit
                    throw std::runtime_error("expected template parameter name");
                }
            } else {
                throw std::runtime_error("expected template parameter name");
            }
            if (!parser.match(TokenKind::Comma)) break;
        }
        parser.expect_template_close();
        parse_optional_cpp20_requires_clause(parser);

        // Push template scope so struct body / member parsing can consult it.
        {
            std::vector<Parser::TemplateScopeParam> scope_params;
            for (size_t i = 0; i < template_params.size(); ++i) {
                Parser::TemplateScopeParam p;
                p.name_text_id =
                    template_param_name_text_ids[i] != kInvalidText
                        ? template_param_name_text_ids[i]
                        : parser.parser_text_id_for_token(kInvalidText,
                                                          template_params[i]);
                p.name = template_params[i];
                p.is_nttp = template_param_nttp[i];
                scope_params.push_back(p);
            }
            parser.push_template_scope(Parser::TemplateScopeKind::FreeFunctionTemplate, scope_params);
            template_prelude_guard.pushed_template_scope = true;
        }

        size_t struct_defs_before = parser.definition_state_.struct_defs.size();
        parser.clear_last_using_alias_name();
        Node* templated = parse_top_level(parser);
        // If parse_top_level registered a using-alias, record alias template
        // info so that later Name<args> can rebuild the aliased template struct.
        const QualifiedNameKey alias_key = parser.active_context_state_.last_using_alias_key;
        if (alias_key.base_text_id != kInvalidText && !template_params.empty()) {
            if (const TypeSpec* aliased_type =
                    parser.find_structured_typedef_type(alias_key)) {
                ParserAliasTemplateInfo ati;
                for (size_t i = 0; i < template_params.size(); ++i) {
                    ati.param_names.push_back(template_params[i]);
                    ati.param_name_text_ids.push_back(
                        template_param_name_text_ids[i]);
                    ati.param_is_nttp.push_back(template_param_nttp[i]);
                    ati.param_is_pack.push_back(template_param_is_pack[i]);
                    ati.param_has_default.push_back(template_param_has_default[i]);
                    ati.param_default_types.push_back(template_param_default_types[i]);
                    ati.param_default_values.push_back(template_param_default_values[i]);
                }
                ati.aliased_type = *aliased_type;
                ati.member_typedef =
                    parser.active_context_state_.last_using_alias_member_typedef;
                parser.template_state_.alias_template_info[alias_key] = std::move(ati);
            }
            parser.clear_last_using_alias_name();
        }
        auto attach_template_params = [&](Node* n) {
            if (!n || template_params.empty()) return;
            n->n_template_params = (int)template_params.size();
            n->template_param_names = parser.arena_.alloc_array<const char*>(n->n_template_params);
            n->template_param_name_text_ids = parser.arena_.alloc_array<TextId>(n->n_template_params);
            n->template_param_is_nttp = parser.arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_is_pack = parser.arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_has_default = parser.arena_.alloc_array<bool>(n->n_template_params);
            n->template_param_default_types = parser.arena_.alloc_array<TypeSpec>(n->n_template_params);
            n->template_param_default_values = parser.arena_.alloc_array<long long>(n->n_template_params);
            n->template_param_default_exprs = parser.arena_.alloc_array<const char*>(n->n_template_params);
            for (int i = 0; i < n->n_template_params; ++i) {
                n->template_param_names[i] = template_params[i];
                n->template_param_name_text_ids[i] = template_param_name_text_ids[i];
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
                    const char* key_name =
                        (n->unqualified_name && n->unqualified_name[0])
                            ? n->unqualified_name
                            : n->name;
                    const TextId key_text_id =
                        n->unqualified_text_id != kInvalidText
                            ? n->unqualified_text_id
                            : parser.parser_text_id_for_token(kInvalidText,
                                                              key_name);
                    const int key_context_id =
                        n->namespace_context_id >= 0
                            ? n->namespace_context_id
                            : parser.current_namespace_context_id();
                    parser.cache_nttp_default_expr_tokens(
                        parser.alias_template_key_in_context(
                            key_context_id, key_text_id),
                        idx, std::move(toks));
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
        // recursive parse_top_level call and stored in
        // parser.definition_state_.struct_defs, while
        // `templated` is NK_EMPTY (struct-only declaration).  Find the struct
        // def and attach template params to it.  Only consider structs that were
        // added during THIS template parse (not pre-existing instantiated structs).
        if (parser.definition_state_.struct_defs.size() > struct_defs_before) {
            Node* last_sd = parser.definition_state_.struct_defs.back();
            if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                last_sd->template_origin_name && last_sd->n_template_args > 0) {
                if (!template_params.empty() && last_sd->n_template_params == 0)
                    attach_template_params(last_sd);
                parser.register_template_struct_specialization(
                    parser.current_namespace_context_id(),
                    parser.parser_text_id_for_token(
                        kInvalidText, last_sd->template_origin_name),
                    last_sd);
            } else if (last_sd && last_sd->kind == NK_STRUCT_DEF &&
                       !template_params.empty()) {
                if (last_sd->n_template_params == 0)
                    attach_template_params(last_sd);
                parser.register_template_struct_primary(
                    parser.current_namespace_context_id(),
                    parser.parser_text_id_for_token(kInvalidText, last_sd->name),
                    last_sd);
                // In C++ mode, register the template struct name as a typedef
                // so it's recognized as a type start for Pair<int> syntax.
                parser.register_tag_type_binding(last_sd->name, TB_STRUCT,
                                          last_sd->name);
                // If inside a namespace, also register ns::Name so
                // qualified references like std::vector<int> work.
                const std::string qn = parser.bridge_name_in_context(
                    parser.current_namespace_context_id(), parser.find_parser_text_id(last_sd->name),
                    last_sd->name);
                parser.register_tag_type_binding(qn, TB_STRUCT, last_sd->name);
            }
        }
        return templated;
    }

    if (try_skip_cpp_concept_declaration(parser)) {
        return nullptr;
    }

    // Handle #pragma pack tokens
    if (parser.check(TokenKind::PragmaPack)) {
        handle_pragma_pack(parser, parser.cur().text_id == kInvalidText
                                      ? std::string()
                                      : std::string(parser.token_spelling(parser.cur())));
        parser.consume();
        return nullptr;
    }

    // Handle #pragma weak tokens
    if (parser.check(TokenKind::PragmaWeak)) {
        auto* node = parser.make_node(NK_PRAGMA_WEAK, ln);
        node->name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        parser.consume();
        return node;
    }

    // Handle #pragma GCC visibility push/pop tokens
    if (parser.check(TokenKind::PragmaGccVisibility)) {
        handle_pragma_gcc_visibility(parser, std::string(parser.token_spelling(parser.cur())));
        parser.consume();
        return nullptr;
    }

    if (parser.check(TokenKind::PragmaExec)) {
        auto* node = parser.make_node(NK_PRAGMA_EXEC, ln);
        node->name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
        handle_pragma_exec(parser, std::string(parser.token_spelling(parser.cur())));
        parser.consume();
        return node;
    }

    // Don't parser.skip_attributes() here — type-affecting attributes like
    // __attribute__((vector_size(N))) must flow to parser.parse_base_type().
    if (parser.at_end()) return nullptr;

    // Handle top-level asm
    if (parser.check(TokenKind::KwAsm)) {
        parser.consume();
        if (parser.check(TokenKind::LParen)) {
            const bool closed = skip_top_level_asm_or_recover(parser);
            if (closed) parser.match(TokenKind::Semi);
            return nullptr;
        }
        parser.match(TokenKind::Semi);
        return nullptr;
    }

    // _Static_assert
    if (parser.check(TokenKind::KwStaticAssert)) {
        return parse_static_assert_declaration(parser);
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
        if (parser.match(TokenKind::KwStatic))    { is_static  = true; }
        else if (parser.match(TokenKind::KwExtern))   {
            is_extern  = true;
            if (const char* spec = maybe_parse_linkage_spec(&parser)) linkage_spec = spec;
        }
        else if (parser.match(TokenKind::KwTypedef))  { is_typedef = true; }
        else if (parser.match(TokenKind::KwInline))   { is_inline  = true; }
        else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) { is_constexpr = true; }
        else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConsteval)) { is_consteval = true; }
        else if (parser.match(TokenKind::KwExtension)) {}
        else if (parser.match(TokenKind::KwNoreturn))  {}
        else break;
    }
    if (parser.is_cpp_mode() && linkage_spec && parser.check(TokenKind::LBrace)) {
        parser.consume();
        std::vector<Node*> items;
        while (!parser.at_end() && !parser.check(TokenKind::RBrace)) {
            Node* item = parse_top_level(parser);
            if (item) items.push_back(item);
        }
        parser.expect(TokenKind::RBrace);
        parser.match(TokenKind::Semi);
        if (items.empty()) return nullptr;
        if (items.size() == 1) return items[0];
        Node* blk = parser.make_node(NK_BLOCK, ln);
        blk->n_children = static_cast<int>(items.size());
        blk->children = parser.arena_.alloc_array<Node*>(blk->n_children);
        for (int i = 0; i < blk->n_children; ++i) blk->children[i] = items[i];
        return blk;
    }
    // Don't parser.skip_attributes() here — let parser.parse_base_type() handle them so
    // type-affecting attributes like vector_size are captured.

    // C++ out-of-class constructor / conversion-operator definition/declaration:
    //   Type::Type(params) { ... }
    //   Type::operator bool() const { ... }
    // There is no explicit return type, so handle these before parser.parse_base_type().
    if (parser.is_cpp_mode() && !is_typedef) {
        // NOTE: saved_special_member_pos is captured by the lambdas below;
        // converting to Parser::TentativeParseGuard would require refactoring all
        // lambda captures, so manual parser.pos_ save/restore is intentionally kept here.
        const int saved_special_member_pos = parser.pos_;
        auto consume_optional_template_id = [&]() -> bool {
            if (!parser.check(TokenKind::Less)) return true;
            int depth = 1;
            parser.consume();
            while (!parser.at_end() && depth > 0) {
                if (parser.check(TokenKind::Less)) {
                    ++depth;
                    parser.consume();
                    continue;
                }
                if (parser.check_template_close()) {
                    --depth;
                    if (!parser.match_template_close()) return false;
                    continue;
                }
                parser.consume();
            }
            return depth == 0;
        };

        auto probe_skip_optional_template_id = [&](int* probe_pos) -> bool {
            if (!probe_pos) return false;
            if (*probe_pos >= static_cast<int>(parser.core_input_state_.tokens.size()) ||
                parser.core_input_state_.tokens[*probe_pos].kind != TokenKind::Less) {
                return true;
            }
            int depth = 1;
            ++(*probe_pos);
            while (*probe_pos < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                   depth > 0) {
                const TokenKind kind = parser.core_input_state_.tokens[*probe_pos].kind;
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
                if (probe < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                    parser.core_input_state_.tokens[probe].kind == TokenKind::ColonColon) {
                    ++probe;
                }
                if (probe >= static_cast<int>(parser.core_input_state_.tokens.size()) ||
                    parser.core_input_state_.tokens[probe].kind != TokenKind::Identifier) {
                    return false;
                }

                std::string owner;
                while (true) {
                    const std::string seg =
                        std::string(parser.token_spelling(parser.core_input_state_.tokens[probe]));
                    ++probe;
                    if (!probe_skip_optional_template_id(&probe)) {
                        return false;
                    }

                    if (probe + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        parser.core_input_state_.tokens[probe].kind == TokenKind::ColonColon &&
                        parser.core_input_state_.tokens[probe + 1].kind ==
                            TokenKind::KwOperator) {
                        if (!owner.empty()) owner += "::";
                        owner += seg;
                        if (out_owner) *out_owner = owner;
                        if (out_base_name) *out_base_name = seg;
                        if (out_is_operator) *out_is_operator = true;
                        return true;
                    }

                    if (probe + 1 < static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        parser.core_input_state_.tokens[probe].kind == TokenKind::ColonColon &&
                        parser.core_input_state_.tokens[probe + 1].kind ==
                            TokenKind::Identifier) {
                        const std::string next_name =
                            std::string(
                                parser.token_spelling(parser.core_input_state_.tokens[probe + 1]));
                        int terminal_probe = probe + 2;
                        if (next_name == seg &&
                            terminal_probe <
                                static_cast<int>(parser.core_input_state_.tokens.size()) &&
                            parser.core_input_state_.tokens[terminal_probe].kind ==
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
            [&](bool stop_before_operator, bool stop_before_ctor,
                std::vector<TextId>* out_owner_text_ids = nullptr,
                bool* out_is_global_qualified = nullptr) -> bool {
                parser.pos_ = saved_special_member_pos;
                if (out_owner_text_ids) out_owner_text_ids->clear();
                if (out_is_global_qualified) *out_is_global_qualified = false;
                if (parser.check(TokenKind::ColonColon)) {
                    if (out_is_global_qualified) *out_is_global_qualified = true;
                    parser.consume();
                }
                if (!parser.check(TokenKind::Identifier)) return false;

                while (true) {
                    const std::string seg = std::string(parser.token_spelling(parser.cur()));
                    const TextId seg_text_id =
                        parser.parser_text_id_for_token(parser.cur().text_id, seg);
                    parser.consume();
                    if (!consume_optional_template_id()) return false;
                    if (out_owner_text_ids && seg_text_id != kInvalidText) {
                        out_owner_text_ids->push_back(seg_text_id);
                    }

                    if (stop_before_operator &&
                        parser.check(TokenKind::ColonColon) &&
                        parser.peek(1).kind == TokenKind::KwOperator) {
                        return true;
                    }

                    if (stop_before_ctor &&
                        parser.check(TokenKind::ColonColon) &&
                        parser.peek(1).kind == TokenKind::Identifier &&
                        parser.token_spelling(parser.peek(1)) == seg &&
                        parser.core_input_state_.pos + 2 <
                            static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        parser.core_input_state_.tokens[parser.core_input_state_.pos + 2].kind ==
                            TokenKind::LParen) {
                        return true;
                    }

                    if (parser.check(TokenKind::ColonColon) && parser.peek(1).kind == TokenKind::Identifier) {
                        parser.consume();
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
            std::vector<TextId> qualified_owner_text_ids;
            bool qualified_owner_is_global = false;
            if (!consume_special_member_owner(/*stop_before_operator=*/true,
                                              /*stop_before_ctor=*/false,
                                              &qualified_owner_text_ids,
                                              &qualified_owner_is_global)) {
                parser.pos_ = saved_special_member_pos;
            }
            parser.expect(TokenKind::ColonColon);
            parser.expect(TokenKind::KwOperator);

            // Enter owner scope for out-of-class operator definition.
            const TextId saved_tag_op_text_id =
                parser.active_context_state_.current_struct_tag_text_id;
            const std::string saved_tag_op_fallback =
                parser.active_context_state_.current_struct_tag;
            parser.set_current_struct_tag(qualified_owner);
            if (!parser.template_state_.template_scope_stack.empty() &&
                parser.template_state_.template_scope_stack.back().kind ==
                    Parser::TemplateScopeKind::FreeFunctionTemplate &&
                parser.find_template_struct_primary(
                    parser.current_namespace_context_id(),
                    parser.parser_text_id_for_token(kInvalidText,
                                                    qualified_owner))) {
                parser.template_state_.template_scope_stack.back().kind =
                    Parser::TemplateScopeKind::EnclosingClass;
                parser.template_state_.template_scope_stack.back().owner_struct_tag =
                    qualified_owner;
            }

            TypeSpec conv_ts = parser.parse_base_type();
            parser.parse_attributes(&conv_ts);
            while (parser.check(TokenKind::Star)) {
                parser.consume();
                conv_ts.ptr_level++;
            }
            if (parser.check(TokenKind::AmpAmp)) {
                parser.consume();
                conv_ts.is_rvalue_ref = true;
            } else if (parser.check(TokenKind::Amp)) {
                parser.consume();
                conv_ts.is_lvalue_ref = true;
            }

            std::string qualified_op_name = qualified_owner;
            if (!qualified_op_name.empty()) qualified_op_name += "::";
            std::string operator_base_name;
            if (conv_ts.base == TB_BOOL && conv_ts.ptr_level == 0 &&
                !conv_ts.is_lvalue_ref && !conv_ts.is_rvalue_ref) {
                operator_base_name = "operator_bool";
            } else {
                operator_base_name = "operator_conv_";
                append_type_mangled_suffix_local(operator_base_name, conv_ts);
            }
            qualified_op_name += operator_base_name;

            parser.expect(TokenKind::LParen);
            std::vector<Node*> params;
            bool variadic = false;
            if (!parser.check(TokenKind::RParen)) {
                while (!parser.at_end()) {
                    if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                    if (parser.check(TokenKind::RParen)) break;
                    if (!parser.can_start_parameter_type()) break;
                    Node* p = parse_param(parser);
                    if (p) params.push_back(p);
                    if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);

            bool is_const_method = false;
            bool is_lvalue_ref_method = false;
            bool is_rvalue_ref_method = false;
            while (true) {
                if (parser.match(TokenKind::KwConst)) {
                    is_const_method = true;
                } else if (parser.match(TokenKind::KwConstexpr)) {
                    is_constexpr = true;
                } else if (parser.match(TokenKind::KwConsteval)) {
                    is_consteval = true;
                } else {
                    break;
                }
            }
            if (parser.match(TokenKind::AmpAmp)) {
                is_rvalue_ref_method = true;
            } else if (parser.match(TokenKind::Amp)) {
                is_lvalue_ref_method = true;
            }
            parser.skip_exception_spec();
            parse_optional_cpp20_trailing_requires_clause(parser);

            Node* fn = parser.make_node(NK_FUNCTION, ln);
            fn->type = conv_ts;
            fn->name = parser.arena_.strdup(qualified_op_name.c_str());
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
            fn->visibility = parser.pragma_state_.visibility;
            fn->execution_domain = parser.pragma_state_.execution_domain;
            fn->n_params = static_cast<int>(params.size());
            if (fn->n_params > 0) {
                fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }

            if (parser.check(TokenKind::LBrace)) {
                ParserFunctionParamScopeGuard param_scope(parser, params);
                bool saved_top = parser.active_context_state_.parsing_top_level_context;
                parser.active_context_state_.parsing_top_level_context = false;
                fn->body = parse_block(parser);
                parser.active_context_state_.parsing_top_level_context = saved_top;
            } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                       parser.core_input_state_.pos + 1 <
                           static_cast<int>(parser.core_input_state_.tokens.size()) &&
                       parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                           TokenKind::KwDelete) {
                parser.consume();
                parser.consume();
                fn->is_deleted = true;
                parser.match(TokenKind::Semi);
            } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                       parser.core_input_state_.pos + 1 <
                           static_cast<int>(parser.core_input_state_.tokens.size()) &&
                       parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                           TokenKind::KwDefault) {
                parser.consume();
                parser.consume();
                fn->is_defaulted = true;
                parser.match(TokenKind::Semi);
            } else {
                parser.match(TokenKind::Semi);
            }
            const TextId operator_base_text_id =
                parser.parser_text_id_for_token(kInvalidText, operator_base_name);
            if (operator_base_text_id != kInvalidText) {
                QualifiedNameKey operator_key;
                operator_key.context_id = 0;
                operator_key.is_global_qualified = qualified_owner_is_global;
                operator_key.base_text_id = operator_base_text_id;
                if (!qualified_owner_text_ids.empty()) {
                    operator_key.qualifier_path_id =
                        parser.shared_lookup_state_.parser_name_paths.intern(
                            qualified_owner_text_ids);
                }
                parser.register_known_fn_name(operator_key);
            }
            restore_current_struct_tag(parser, saved_tag_op_text_id,
                                       saved_tag_op_fallback);
            return fn;
        }

        if (looks_like_qualified_ctor) {
            std::vector<TextId> qualified_owner_text_ids;
            bool qualified_owner_is_global = false;
            if (!consume_special_member_owner(/*stop_before_operator=*/false,
                                              /*stop_before_ctor=*/true,
                                              &qualified_owner_text_ids,
                                              &qualified_owner_is_global)) {
                parser.pos_ = saved_special_member_pos;
            }
            parser.expect(TokenKind::ColonColon);
            const std::string ctor_name = std::string(parser.token_spelling(parser.cur()));
            const TextId ctor_name_text_id =
                parser.parser_text_id_for_token(parser.cur().text_id, ctor_name);
            parser.consume();
            if (parser.check(TokenKind::LParen)) {
                std::string qualified_ctor_name = qualified_owner;
                if (!qualified_ctor_name.empty()) qualified_ctor_name += "::";
                qualified_ctor_name += ctor_name;

                // Enter owner scope for out-of-class constructor definition.
                const TextId saved_tag_ctor_text_id =
                    parser.active_context_state_.current_struct_tag_text_id;
                const std::string saved_tag_ctor_fallback =
                    parser.active_context_state_.current_struct_tag;
                parser.set_current_struct_tag(qualified_owner);
                if (!parser.template_state_.template_scope_stack.empty() &&
                    parser.template_state_.template_scope_stack.back().kind ==
                        Parser::TemplateScopeKind::FreeFunctionTemplate &&
                    parser.find_template_struct_primary(
                        parser.current_namespace_context_id(),
                        parser.parser_text_id_for_token(kInvalidText,
                                                        qualified_owner))) {
                    parser.template_state_.template_scope_stack.back().kind =
                        Parser::TemplateScopeKind::EnclosingClass;
                    parser.template_state_.template_scope_stack.back().owner_struct_tag =
                        qualified_owner;
                }

                parser.consume();  // (
                std::vector<Node*> params;
                bool variadic = false;
                if (!parser.check(TokenKind::RParen)) {
                    while (!parser.at_end()) {
                        if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                        if (parser.check(TokenKind::RParen)) break;
                        if (!parser.can_start_parameter_type()) break;
                        Node* p = parse_param(parser);
                        if (p) params.push_back(p);
                        if (parser.check(TokenKind::Ellipsis)) { variadic = true; parser.consume(); break; }
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }
                parser.expect(TokenKind::RParen);
                parser.skip_exception_spec();
                parse_optional_cpp20_trailing_requires_clause(parser);

                std::vector<const char*> init_names;
                std::vector<std::vector<Node*>> init_args_list;
                if (parser.check(TokenKind::Colon)) {
                    parser.consume(); // :
                    while (!parser.at_end() && !parser.check(TokenKind::LBrace)) {
                        if (!parser.check(TokenKind::Identifier)) break;
                        const char* mem_name =
                            parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                        parser.consume();
                        parser.expect(TokenKind::LParen);
                        std::vector<Node*> args;
                        if (!parser.check(TokenKind::RParen)) {
                            while (true) {
                                Node* arg = parse_assign_expr(parser);
                                if (arg) args.push_back(arg);
                                if (!parser.match(TokenKind::Comma)) break;
                            }
                        }
                        parser.expect(TokenKind::RParen);
                        init_names.push_back(mem_name);
                        init_args_list.push_back(std::move(args));
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }

                Node* fn = parser.make_node(NK_FUNCTION, ln);
                fn->type.base = TB_VOID;
                fn->name = parser.arena_.strdup(qualified_ctor_name.c_str());
                fn->variadic = variadic;
                fn->is_static = is_static;
                fn->is_extern = is_extern;
                fn->is_inline = is_inline;
                fn->is_constexpr = is_constexpr;
                fn->is_consteval = is_consteval;
                fn->is_constructor = true;
                fn->linkage_spec = linkage_spec;
                fn->visibility = parser.pragma_state_.visibility;
                fn->execution_domain = parser.pragma_state_.execution_domain;
                fn->n_params = static_cast<int>(params.size());
                if (fn->n_params > 0) {
                    fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
                    for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
                }
                if (!init_names.empty()) {
                    fn->n_ctor_inits = static_cast<int>(init_names.size());
                    fn->ctor_init_names = parser.arena_.alloc_array<const char*>(fn->n_ctor_inits);
                    fn->ctor_init_args = parser.arena_.alloc_array<Node**>(fn->n_ctor_inits);
                    fn->ctor_init_nargs = parser.arena_.alloc_array<int>(fn->n_ctor_inits);
                    for (int i = 0; i < fn->n_ctor_inits; ++i) {
                        fn->ctor_init_names[i] = init_names[i];
                        fn->ctor_init_nargs[i] = static_cast<int>(init_args_list[i].size());
                        if (fn->ctor_init_nargs[i] > 0) {
                            fn->ctor_init_args[i] = parser.arena_.alloc_array<Node*>(fn->ctor_init_nargs[i]);
                            for (int j = 0; j < fn->ctor_init_nargs[i]; ++j)
                                fn->ctor_init_args[i][j] = init_args_list[i][j];
                        } else {
                            fn->ctor_init_args[i] = nullptr;
                        }
                    }
                }

                if (parser.check(TokenKind::LBrace)) {
                    ParserFunctionParamScopeGuard param_scope(parser, params);
                    bool saved_top =
                        parser.active_context_state_.parsing_top_level_context;
                    parser.active_context_state_.parsing_top_level_context = false;
                    fn->body = parse_block(parser);
                    parser.active_context_state_.parsing_top_level_context = saved_top;
                } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                           parser.core_input_state_.pos + 1 <
                               static_cast<int>(parser.core_input_state_.tokens.size()) &&
                           parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                               TokenKind::KwDelete) {
                    parser.consume();
                    parser.consume();
                    fn->is_deleted = true;
                    parser.match(TokenKind::Semi);
                } else if (parser.is_cpp_mode() && parser.check(TokenKind::Assign) &&
                           parser.core_input_state_.pos + 1 <
                               static_cast<int>(parser.core_input_state_.tokens.size()) &&
                           parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                               TokenKind::KwDefault) {
                    parser.consume();
                    parser.consume();
                    fn->is_defaulted = true;
                    parser.match(TokenKind::Semi);
                } else {
                    parser.match(TokenKind::Semi);
                }
                if (ctor_name_text_id != kInvalidText) {
                    QualifiedNameKey ctor_key;
                    ctor_key.context_id = 0;
                    ctor_key.is_global_qualified = qualified_owner_is_global;
                    ctor_key.base_text_id = ctor_name_text_id;
                    if (!qualified_owner_text_ids.empty()) {
                        ctor_key.qualifier_path_id =
                            parser.shared_lookup_state_.parser_name_paths.intern(
                                qualified_owner_text_ids);
                    }
                    parser.register_known_fn_name(ctor_key);
                }
                restore_current_struct_tag(parser, saved_tag_ctor_text_id,
                                           saved_tag_ctor_fallback);
                return fn;
            }
        }
        parser.pos_ = saved_special_member_pos;
    }

    TypeSpec base_ts{};
    if (!parser.is_type_start()) {
        if (is_typedef) {
            if (parser.check(TokenKind::Identifier)) {
                const TextId name_text_id = parser.cur().text_id;
                const std::string spelled = std::string(parser.token_spelling(parser.cur()));
                if (parser.is_template_scope_type_param(name_text_id)) {
                    base_ts.array_size = -1;
                    base_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) base_ts.array_dims[i] = -1;
                    base_ts.base = TB_TYPEDEF;
                    base_ts.tag = parser.arena_.strdup(spelled.c_str());
                    base_ts.tag_text_id = name_text_id;
                    parser.consume();
                    goto top_level_base_ready;
                }
                if (const TypeSpec* typedef_type =
                        parser.find_visible_typedef_type(parser.cur().text_id)) {
                    base_ts = *typedef_type;
                    parser.consume();
                    goto top_level_base_ready;
                }
                if (parser.is_cpp_mode() &&
                    parser.looks_like_unresolved_identifier_type_head(
                        parser.core_input_state_.pos)) {
                    base_ts.array_size = -1;
                    base_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) base_ts.array_dims[i] = -1;
                    base_ts.base = TB_TYPEDEF;
                    base_ts.tag = parser.arena_.strdup(spelled.c_str());
                    base_ts.tag_text_id = name_text_id;
                    parser.consume();
                    goto top_level_base_ready;
                }
            }
            std::string nm = parser.check(TokenKind::Identifier)
                                 ? std::string(parser.token_spelling(parser.cur()))
                                 : "<non-identifier>";
            throw std::runtime_error(std::string("typedef uses unknown base type: ") + nm);
        }
        // K&R/old-style implicit-int function at file scope:
        // e.g. `main() { ... }` or `static foo() { ... }`.
        // Treat as `int name(...)` instead of discarding the whole definition.
        bool maybe_implicit_int_fn =
            parser.check(TokenKind::Identifier) && parser.peek_next_is(TokenKind::LParen);
        if (!maybe_implicit_int_fn) {
            const int recovery_line = !parser.at_end() ? parser.cur().line : ln;
            while (!parser.at_end() && !parser.check(TokenKind::Semi)) {
                if (parser.cur().line != recovery_line &&
                    is_top_level_decl_recovery_boundary(parser.cur().kind)) {
                    goto top_level_decl_recovery_done;
                }
                parser.consume();
            }
            parser.match(TokenKind::Semi);
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
        base_ts = parser.parse_base_type();
        parser.parse_attributes(&base_ts);
        // Re-scan for storage class specifiers that appear after the base type
        // (e.g. `struct a {...} static g = ...` — GCC allows storage class mid-decl).
        while (true) {
            if (parser.match(TokenKind::KwStatic))    { is_static  = true; }
            else if (parser.match(TokenKind::KwExtern))   {
                is_extern  = true;
                if (const char* spec = maybe_parse_linkage_spec(&parser)) linkage_spec = spec;
            }
            else if (parser.match(TokenKind::KwTypedef))  { is_typedef = true; }
            else if (parser.match(TokenKind::KwInline))   { is_inline  = true; }
            else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConstexpr)) { is_constexpr = true; }
            else if (parser.is_cpp_mode() && parser.match(TokenKind::KwConsteval)) { is_consteval = true; }
            else if (parser.match(TokenKind::KwExtension)) {}
            else if (parser.match(TokenKind::KwNoreturn))  {}
            else break;
        }
        parser.parse_attributes(&base_ts);
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
        parser.active_context_state_.last_resolved_typedef_text_id;

    auto is_incomplete_object_type = [&](const TypeSpec& ts) -> bool {
        if (ts.ptr_level > 0) return false;
        if (ts.base != TB_STRUCT && ts.base != TB_UNION) return false;
        if (ts.tpl_struct_origin) return false;  // pending template struct — resolved at HIR level
        if (ts.tag && parser.is_cpp_mode() &&
            !parser.active_context_state_.current_struct_tag.empty()) {
            if (is_same_visible_type_name(
                    parser, parser.active_context_state_.current_struct_tag_text_id,
                    parser.current_struct_tag_text(), parser.find_parser_text_id(ts.tag),
                    ts.tag))
                return false;
        }
        Node* def = resolve_record_type_spec(
            ts, &parser.definition_state_.struct_tag_def_map);
        if (!def) return true;
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
        parser.parse_declarator(ts_copy, &tdname, &td_fn_ptr_params,
                         &td_n_fn_ptr_params, &td_fn_ptr_variadic, nullptr,
                         nullptr, nullptr, nullptr, &tdname_text_id);
        if (tdname) {
            const char* scoped_tdname =
                parser.qualify_name_arena(tdname_text_id, tdname);
            if (!parser.is_cpp_mode() && !is_internal_typedef_name(tdname) &&
                parser.has_conflicting_user_typedef_binding(tdname_text_id, ts_copy))
                throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdname);
            parser.register_typedef_binding(
                tdname_text_id, ts_copy,
                !is_internal_typedef_name(tdname));
            if (parser.is_cpp_mode() && !is_internal_typedef_name(tdname)) {
                parser.register_structured_typedef_binding_in_context(
                    parser.current_namespace_context_id(), tdname_text_id,
                    ts_copy);
            }
            if (scoped_tdname != tdname) {
                parser.register_typedef_binding(
                    parser.parser_text_id_for_token(kInvalidText, scoped_tdname),
                    ts_copy, false);
            }
            if (ts_copy.is_fn_ptr && (td_n_fn_ptr_params > 0 || td_fn_ptr_variadic)) {
                const TextId typedef_name_id =
                    tdname_text_id != kInvalidText
                        ? tdname_text_id
                        : parser.parser_text_id_for_token(kInvalidText, tdname);
                if (typedef_name_id != kInvalidText) {
                    parser.binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                        td_fn_ptr_params, td_n_fn_ptr_params, td_fn_ptr_variadic};
                }
            }
        }
        while (parser.match(TokenKind::Comma)) {
            TypeSpec ts2 = base_ts;
            const char* tdn2 = nullptr;
            TextId tdn2_text_id = kInvalidText;
            Node** td2_fn_ptr_params = nullptr;
            int td2_n_fn_ptr_params = 0;
            bool td2_fn_ptr_variadic = false;
            parser.parse_declarator(ts2, &tdn2, &td2_fn_ptr_params,
                             &td2_n_fn_ptr_params, &td2_fn_ptr_variadic,
                             nullptr, nullptr, nullptr, nullptr,
                             &tdn2_text_id);
            if (tdn2) {
                const char* scoped_tdn2 =
                    parser.qualify_name_arena(tdn2_text_id, tdn2);
                if (!parser.is_cpp_mode() && !is_internal_typedef_name(tdn2) &&
                    parser.has_conflicting_user_typedef_binding(tdn2_text_id, ts2))
                    throw std::runtime_error(std::string("conflicting typedef redefinition: ") + tdn2);
                parser.register_typedef_binding(
                    tdn2_text_id, ts2,
                    !is_internal_typedef_name(tdn2));
                if (parser.is_cpp_mode() && !is_internal_typedef_name(tdn2)) {
                    parser.register_structured_typedef_binding_in_context(
                        parser.current_namespace_context_id(), tdn2_text_id,
                        ts2);
                }
                if (scoped_tdn2 != tdn2) {
                    parser.register_typedef_binding(
                        parser.parser_text_id_for_token(kInvalidText, scoped_tdn2),
                        ts2, false);
                }
                if (ts2.is_fn_ptr && (td2_n_fn_ptr_params > 0 || td2_fn_ptr_variadic)) {
                    const TextId typedef_name_id =
                        tdn2_text_id != kInvalidText
                            ? tdn2_text_id
                            : parser.parser_text_id_for_token(kInvalidText, tdn2);
                    if (typedef_name_id != kInvalidText) {
                        parser.binding_state_.typedef_fn_ptr_info[typedef_name_id] = {
                            td2_fn_ptr_params, td2_n_fn_ptr_params, td2_fn_ptr_variadic};
                    }
                }
            }
        }
        parser.match(TokenKind::Semi);
        // Top-level typedef declarations are bookkeeping-only once the alias
        // tables have been updated, so drop them from the program item stream.
        return nullptr;
    }

    // Peek to disambiguate: struct/union/enum declaration only (no declarator)
    if (parser.check(TokenKind::Semi) &&
        (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
         base_ts.base == TB_ENUM)) {
        parser.consume();  // consume ;
        return nullptr;
    }

    // Parse declarator (name + pointer stars + maybe function params)
    TypeSpec ts = base_ts;
    const int decl_suffix_start = parser.pos_;
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
    if (parser.check(TokenKind::LParen) && parser.peek_next_is(TokenKind::Star)) {
        parser.consume();  // (
        parser.consume();  // *
        ts.ptr_level++;
        while (parser.check(TokenKind::Star)) { parser.consume(); ts.ptr_level++; }
        // Skip qualifiers after * (e.g. `double (* const a[])`)
        while (is_qualifier(parser.cur().kind)) parser.consume();
        // Skip nullability annotations: (* _Nullable name) or (* _Nonnull name)
        while (parser.check(TokenKind::Identifier)) {
            const std::string ql = std::string(parser.token_spelling(parser.cur()));
            if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                ql == "__nullable" || ql == "__nonnull")
                parser.consume();
            else break;
        }
        parser.skip_attributes();
        if (parser.check(TokenKind::Identifier)) {
            decl_name_text_id =
                parser.parser_text_id_for_token(parser.cur().text_id, parser.token_spelling(parser.cur()));
            decl_name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
            parser.consume();
        }
        // Handle array-of-function-pointer: void (*func2[6])(void)
        while (parser.check(TokenKind::LBracket)) {
            parser.consume();  // [
            long long dim = -2;
            if (!parser.check(TokenKind::RBracket)) {
                Node* sz = parse_expr(parser);
                if (sz) { long long v; if (eval_const_int(sz, &v)) dim = v; }
            }
            parser.expect(TokenKind::RBracket);
            if (ts.array_rank < 8) ts.array_dims[ts.array_rank++] = dim;
            if (ts.array_rank == 1) ts.array_size = ts.array_dims[0];
        }
        // Detect nested fn_ptr: (* (*name(params))(ret_params)) or function-returning-fptr: (* f1(params))
        if (parser.check(TokenKind::LParen) && parser.peek_next_is(TokenKind::Star)) {
            // Nested fn_ptr: (* (*name(fn_params))(inner_ret_params) )(outer_ret_params)
            // Recurse: consume '(' '*' to enter the inner level.
            parser.consume();  // (
            parser.consume();  // *
            ts.ptr_level++;
            while (parser.check(TokenKind::Star)) { parser.consume(); ts.ptr_level++; }
            // Skip qualifiers/nullability
            while (is_qualifier(parser.cur().kind)) parser.consume();
            while (parser.check(TokenKind::Identifier)) {
                const std::string ql = std::string(parser.token_spelling(parser.cur()));
                if (ql == "_Nullable" || ql == "_Nonnull" || ql == "_Null_unspecified" ||
                    ql == "__nullable" || ql == "__nonnull") { parser.consume(); continue; }
                break;
            }
            if (parser.check(TokenKind::Identifier)) {
                decl_name_text_id = parser.parser_text_id_for_token(
                    parser.cur().text_id, parser.token_spelling(parser.cur()));
                decl_name = parser.arena_.strdup(std::string(parser.token_spelling(parser.cur())));
                parser.consume();
            }
            // Function's own params (pick_outer(int which))
            if (decl_name && parser.check(TokenKind::LParen)) {
                fn_returning_fptr = true;
                parser.consume();
                if (!parser.check(TokenKind::RParen)) {
                    while (!parser.at_end()) {
                        if (parser.check(TokenKind::Ellipsis)) {
                            fptr_fn_variadic = true; parser.consume(); break;
                        }
                        if (parser.check(TokenKind::RParen)) break;
                        if (!parser.can_start_parameter_type()) {
                            parser.consume();
                            if (parser.match(TokenKind::Comma)) continue;
                            break;
                        }
                        Node* p = parse_param(parser);
                        if (p) fptr_fn_params.push_back(p);
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }
                parser.expect(TokenKind::RParen);
            }
            parser.expect(TokenKind::RParen);  // close inner (*name(params))
            // Parse inner return fn_ptr params: (int)
            if (parser.check(TokenKind::LParen)) {
                std::vector<Node*> inner_ret_params;
                bool inner_ret_variadic = false;
                parser.consume();
                if (!parser.check(TokenKind::RParen)) {
                    while (!parser.at_end()) {
                        if (parser.check(TokenKind::Ellipsis)) {
                            inner_ret_variadic = true; parser.consume(); break;
                        }
                        if (parser.check(TokenKind::RParen)) break;
                        if (!parser.can_start_parameter_type()) {
                            parser.consume();
                            if (parser.match(TokenKind::Comma)) continue;
                            break;
                        }
                        Node* p = parse_param(parser);
                        if (p) inner_ret_params.push_back(p);
                        if (!parser.match(TokenKind::Comma)) break;
                    }
                }
                parser.expect(TokenKind::RParen);
                ts.is_fn_ptr = true;
                // For fn-returning-fptr: these are the inner return params (the first level of return fn_ptr).
                // Store as fn_ptr_params for now; outer params (below) become ret_fn_ptr_params.
                if (fn_returning_fptr) {
                    decl_n_fn_ptr_params = static_cast<int>(inner_ret_params.size());
                    decl_fn_ptr_variadic = inner_ret_variadic;
                    if (!inner_ret_params.empty()) {
                        decl_fn_ptr_params = parser.arena_.alloc_array<Node*>(decl_n_fn_ptr_params);
                        for (int i = 0; i < decl_n_fn_ptr_params; ++i)
                            decl_fn_ptr_params[i] = inner_ret_params[i];
                    }
                }
            }
        } else if (parser.check(TokenKind::LParen)) {
            fn_returning_fptr = true;
            // Parse the function's own parameter list properly
            parser.consume();  // consume '('
            if (!parser.check(TokenKind::RParen)) {
                while (!parser.at_end()) {
                    if (parser.check(TokenKind::Ellipsis)) {
                        fptr_fn_variadic = true; parser.consume(); break;
                    }
                    if (parser.check(TokenKind::RParen)) break;
                    if (!parser.can_start_parameter_type()) {
                        parser.consume();
                        if (parser.match(TokenKind::Comma)) continue;
                        break;
                    }
                    Node* p = parse_param(parser);
                    if (p) fptr_fn_params.push_back(p);
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);  // close function's own params
        }
        parser.expect(TokenKind::RParen);  // close (*name...)
        if (parser.check(TokenKind::LParen)) {
            std::vector<Node*> pointed_fn_params;
            bool pointed_fn_variadic = false;
            parser.consume();  // (
            if (!parser.check(TokenKind::RParen)) {
                while (!parser.at_end()) {
                    if (parser.check(TokenKind::Ellipsis)) {
                        pointed_fn_variadic = true;
                        parser.consume();
                        break;
                    }
                    if (parser.check(TokenKind::RParen)) break;
                    if (!parser.can_start_parameter_type()) {
                        parser.consume();
                        if (parser.match(TokenKind::Comma)) continue;
                        break;
                    }
                    Node* p = parse_param(parser);
                    if (p) pointed_fn_params.push_back(p);
                    if (!parser.match(TokenKind::Comma)) break;
                }
            }
            parser.expect(TokenKind::RParen);
            ts.is_fn_ptr = true; // return type is a function pointer
            if (!fn_returning_fptr) {
                decl_n_fn_ptr_params = static_cast<int>(pointed_fn_params.size());
                decl_fn_ptr_variadic = pointed_fn_variadic;
                if (!pointed_fn_params.empty()) {
                    decl_fn_ptr_params = parser.arena_.alloc_array<Node*>(decl_n_fn_ptr_params);
                    for (int i = 0; i < decl_n_fn_ptr_params; ++i) {
                        decl_fn_ptr_params[i] = pointed_fn_params[i];
                    }
                }
            }
        }
        if (!fn_returning_fptr) is_fptr_global = true;
    } else {
    parser.parse_declarator(ts, &decl_name, &decl_fn_ptr_params, &decl_n_fn_ptr_params,
                     &decl_fn_ptr_variadic, nullptr, &decl_ret_fn_ptr_params,
                     &decl_n_ret_fn_ptr_params, &decl_ret_fn_ptr_variadic,
                     &decl_name_text_id);
    if (is_incomplete_object_type(ts) && !parser.check(TokenKind::LParen)) {
        throw std::runtime_error(std::string("object has incomplete type: ") + (ts.tag ? ts.tag : "<anonymous>"));
    }
    }

    // Owner-aware scope: if the declarator produced a qualified name like
    // "vector::set_capacity", detect the owner struct and re-enter its scope
    // so member typedefs are visible during parameter/body parsing.
    // We save/restore current_struct_tag via TextId plus fallback spelling so
    // all exit paths keep structured context metadata intact.
    std::string qualified_owner_tag;
    auto enter_owner_scope = [&]() {
        if (!parser.is_cpp_mode() || !decl_name) return;
        std::string dn(decl_name);
        auto sep = dn.rfind("::");
        if (sep == std::string::npos || sep == 0) return;
        qualified_owner_tag = dn.substr(0, sep);
        parser.set_current_struct_tag(qualified_owner_tag);
        // If the owner is a known template struct and we have an active
        // FreeFunctionTemplate scope, relabel it as EnclosingClass.
        if (!parser.template_state_.template_scope_stack.empty() &&
            parser.template_state_.template_scope_stack.back().kind ==
                Parser::TemplateScopeKind::FreeFunctionTemplate) {
            if (parser.find_template_struct_primary(
                    parser.current_namespace_context_id(),
                    parser.parser_text_id_for_token(kInvalidText,
                                                    qualified_owner_tag))) {
                parser.template_state_.template_scope_stack.back().kind =
                    Parser::TemplateScopeKind::EnclosingClass;
                parser.template_state_.template_scope_stack.back().owner_struct_tag =
                    qualified_owner_tag;
            }
        }
    };
    const TextId saved_struct_tag_text_id_for_qualified =
        parser.active_context_state_.current_struct_tag_text_id;
    const std::string saved_struct_tag_fallback_for_qualified =
        parser.active_context_state_.current_struct_tag;
    enter_owner_scope();
    // Helper to restore struct tag before returning from this function.
    auto restore_owner_scope = [&]() {
        if (!qualified_owner_tag.empty())
            restore_current_struct_tag(
                parser, saved_struct_tag_text_id_for_qualified,
                saved_struct_tag_fallback_for_qualified);
    };

    // Explicit template specialization: parse <type_args> after function name.
    // e.g. template<> int add<int>(int a, int b) { ... }
    //                          ^^^^^ captured here
    std::vector<TypeSpec> spec_arg_types;
    std::vector<bool> spec_arg_is_value;
    std::vector<long long> spec_arg_values;
    if (parser.active_context_state_.parsing_explicit_specialization && decl_name &&
        parser.check(TokenKind::Less)) {
        parser.consume();  // <
        while (!parser.at_end() && !parser.check(TokenKind::Greater)) {
            // Try to parse as type first (int, long, char, etc.)
            if (parser.is_type_start()) {
                TypeSpec arg_ts = parser.parse_type_name();
                spec_arg_types.push_back(arg_ts);
                spec_arg_is_value.push_back(false);
                spec_arg_values.push_back(0);
            } else if (parser.check(TokenKind::IntLit) || parser.check(TokenKind::CharLit) ||
                       (parser.is_cpp_mode() &&
                        (parser.check(TokenKind::KwTrue) || parser.check(TokenKind::KwFalse))) ||
                       (parser.check(TokenKind::Minus) &&
                        parser.core_input_state_.pos + 1 <
                            static_cast<int>(parser.core_input_state_.tokens.size()) &&
                        (parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                             TokenKind::IntLit ||
                         parser.core_input_state_.tokens[parser.core_input_state_.pos + 1].kind ==
                             TokenKind::CharLit))) {
                // NTTP value
                long long sign = 1;
                if (parser.match(TokenKind::Minus)) sign = -1;
                long long val =
                    parse_int_lexeme(std::string(parser.token_spelling(parser.cur())).c_str());
                if (parser.check(TokenKind::KwTrue)) {
                    val = 1;
                    parser.consume();
                } else if (parser.check(TokenKind::KwFalse)) {
                    val = 0;
                    parser.consume();
                } else if (parser.check(TokenKind::CharLit)) {
                    Node* lit = parse_primary(parser);
                    val = lit ? lit->ival : 0;
                } else {
                    val = parse_int_lexeme(std::string(parser.token_spelling(parser.cur())).c_str());
                    parser.consume();
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
            if (!parser.match(TokenKind::Comma)) break;
        }
        parser.expect(TokenKind::Greater);
    }

    parser.parse_attributes(&ts);
    parser.skip_asm();
    parser.parse_attributes(&ts);
    parser.skip_attributes();

    if (!decl_name) {
        auto tail_is_attr_or_asm_only = [&](int begin, int end) -> bool {
            int i = begin;
            auto skip_parens = [&](int start) -> int {
                if (start >= end ||
                    parser.core_input_state_.tokens[start].kind != TokenKind::LParen) {
                    return -1;
                }
                int depth = 0;
                int j = start;
                while (j < end) {
                    if (parser.core_input_state_.tokens[j].kind == TokenKind::LParen) ++depth;
                    else if (parser.core_input_state_.tokens[j].kind == TokenKind::RParen &&
                             --depth == 0)
                        return j + 1;
                    ++j;
                }
                return -1;
            };
            auto skip_cpp11_attrs = [&](int start) -> int {
                if (start + 1 >= end ||
                    parser.core_input_state_.tokens[start].kind != TokenKind::LBracket ||
                    parser.core_input_state_.tokens[start + 1].kind != TokenKind::LBracket) {
                    return -1;
                }
                int depth = 1;
                int j = start + 2;
                while (j < end) {
                    if (j + 1 < end &&
                        parser.core_input_state_.tokens[j].kind == TokenKind::LBracket &&
                        parser.core_input_state_.tokens[j + 1].kind == TokenKind::LBracket) {
                        ++depth;
                        j += 2;
                        continue;
                    }
                    if (j + 1 < end &&
                        parser.core_input_state_.tokens[j].kind == TokenKind::RBracket &&
                        parser.core_input_state_.tokens[j + 1].kind == TokenKind::RBracket) {
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
                if (parser.core_input_state_.tokens[i].kind == TokenKind::KwAttribute) {
                    ++i;
                    i = skip_parens(i);
                    if (i < 0 || i >= end ||
                        parser.core_input_state_.tokens[i].kind != TokenKind::LParen) {
                        return false;
                    }
                    i = skip_parens(i);
                    if (i < 0) return false;
                    continue;
                }
                if (parser.core_input_state_.tokens[i].kind == TokenKind::LBracket) {
                    i = skip_cpp11_attrs(i);
                    if (i < 0) return false;
                    continue;
                }
                if (parser.core_input_state_.tokens[i].kind == TokenKind::KwAsm) {
                    ++i;
                    while (i < end &&
                           (parser.core_input_state_.tokens[i].kind == TokenKind::KwVolatile ||
                            parser.core_input_state_.tokens[i].kind == TokenKind::KwInline ||
                            parser.core_input_state_.tokens[i].kind == TokenKind::KwGoto)) {
                        ++i;
                    }
                    i = skip_parens(i);
                    if (i < 0) return false;
                    continue;
                }
                if (parser.core_input_state_.tokens[i].kind == TokenKind::KwExtension ||
                    parser.core_input_state_.tokens[i].kind == TokenKind::KwNoreturn) {
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
            parser.check(TokenKind::Semi) &&
            (base_ts.base == TB_STRUCT || base_ts.base == TB_UNION ||
             base_ts.base == TB_ENUM) &&
            tail_is_attr_or_asm_only(decl_suffix_start, parser.pos_);
        parser.match(TokenKind::Semi);
        restore_owner_scope();
        if (is_structure_only_decl) return nullptr;
        // No name: just a type declaration; keep the explicit discard node for
        // non-structure-only fallback shapes so recovery remains visible.
        return parser.make_node(NK_EMPTY, ln);
    }

    const char* scoped_decl_name =
        parser.qualify_name_arena(decl_name_text_id, decl_name);
    auto register_decl_known_fn_name = [&]() {
        if (decl_name) {
            const TextId known_fn_text_id =
                parser.parser_text_id_for_token(decl_name_text_id, decl_name);
            parser.register_known_fn_name_in_context(
                parser.current_namespace_context_id(), known_fn_text_id);
        }
    };

    // Handle function-returning-fptr: int (* f1(a, b))(c, d) { body }
    // Params were already parsed into fptr_fn_params; now look for { body }.
    if (fn_returning_fptr && decl_name) {
        parser.parse_attributes(&ts); parser.skip_asm(); parser.parse_attributes(&ts); parser.skip_attributes();
        parse_optional_cpp20_trailing_requires_clause(parser);
        if (parser.check(TokenKind::LBrace)) {
            ParserFunctionParamScopeGuard param_scope(parser, fptr_fn_params);
            bool saved_top = parser.active_context_state_.parsing_top_level_context;
            parser.active_context_state_.parsing_top_level_context = false;
            Node* body = parse_block(parser);
            parser.active_context_state_.parsing_top_level_context = saved_top;
            Node* fn = parser.make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = scoped_decl_name;
            parser.apply_decl_namespace(fn, parser.current_namespace_context_id(), decl_name);
            fn->variadic  = fptr_fn_variadic;
            fn->is_static = is_static;
            fn->is_extern = is_extern;
            fn->is_inline = is_inline;
            fn->is_constexpr = is_constexpr;
            fn->is_consteval = is_consteval;
            fn->linkage_spec = linkage_spec;
            fn->visibility = parser.pragma_state_.visibility;
            fn->execution_domain = parser.pragma_state_.execution_domain;
            fn->body      = body;
            fn->n_params  = (int)fptr_fn_params.size();
            if (fn->n_params > 0) {
                fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
            }
            // Phase C: propagate return type fn_ptr params.
            fn->ret_fn_ptr_params   = decl_fn_ptr_params;
            fn->n_ret_fn_ptr_params = decl_n_fn_ptr_params;
            fn->ret_fn_ptr_variadic = decl_fn_ptr_variadic;
            register_decl_known_fn_name();
            restore_owner_scope();
            return fn;
        }
        // Function declaration (no body): consume semicolon and return
        parser.match(TokenKind::Semi);
        Node* fn = parser.make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = scoped_decl_name;
        parser.apply_decl_namespace(fn, parser.current_namespace_context_id(), decl_name);
        fn->variadic  = fptr_fn_variadic;
        fn->is_static = is_static;
        fn->is_extern = is_extern;
        fn->is_inline = is_inline;
        fn->is_constexpr = is_constexpr;
        fn->is_consteval = is_consteval;
        fn->linkage_spec = linkage_spec;
        fn->visibility = parser.pragma_state_.visibility;
        fn->execution_domain = parser.pragma_state_.execution_domain;
        fn->body      = nullptr;
        fn->n_params  = (int)fptr_fn_params.size();
        if (fn->n_params > 0) {
            fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = fptr_fn_params[i];
        }
        // Phase C: propagate return type fn_ptr params.
        fn->ret_fn_ptr_params   = decl_fn_ptr_params;
        fn->n_ret_fn_ptr_params = decl_n_fn_ptr_params;
        fn->ret_fn_ptr_variadic = decl_fn_ptr_variadic;
        register_decl_known_fn_name();
        restore_owner_scope();
        return fn;
    }

    // Check for function definition: name followed by ( params ) {
    // We need to figure out if we're looking at a function or a variable.
    // The key distinction: if we have ( params ) { body }, it's a function.
    // We might have already consumed the function params in parser.parse_declarator.
    // Re-parser.check: if the next token is { or if the type's base indicates function.

    // Actually, after parser.parse_declarator, if we encounter '(' next, it means
    // the declarator name was just the name and the function params are next.
    // OR: the declarator already handled the suffix.
    // Let's handle it here.

    if (!is_fptr_global && parser.check(TokenKind::LParen)) {
        // Function declaration or definition
        std::vector<Node*> params;
        std::vector<const char*> knr_param_names;
        bool variadic = false;
        parse_top_level_parameter_list(
            parser, &params, &knr_param_names, &variadic);
        if (decl_name) {
            std::string adjusted_name(decl_name);
            finalize_pending_operator_name(adjusted_name, params.size());
            decl_name = parser.arena_.strdup(adjusted_name.c_str());
            decl_name_text_id = kInvalidText;
            scoped_decl_name = parser.qualify_name_arena(decl_name_text_id, decl_name);
        }
        parser.parse_attributes(&ts);
        parser.skip_asm();
        parser.parse_attributes(&ts);
        parser.skip_attributes();
        bool is_const_method = false;
        bool is_lvalue_ref_method = false;
        bool is_rvalue_ref_method = false;
        while (parser.is_cpp_mode() && decl_name && std::string(decl_name).find("::") != std::string::npos) {
            if (parser.match(TokenKind::KwConst)) {
                is_const_method = true;
                continue;
            }
            if (parser.match(TokenKind::KwConstexpr)) {
                is_constexpr = true;
                continue;
            }
            if (parser.match(TokenKind::KwConsteval)) {
                is_consteval = true;
                continue;
            }
            break;
        }
        if (parser.is_cpp_mode() && decl_name && std::string(decl_name).find("::") != std::string::npos) {
            if (parser.match(TokenKind::AmpAmp)) {
                is_rvalue_ref_method = true;
            } else if (parser.match(TokenKind::Amp)) {
                is_lvalue_ref_method = true;
            }
        }
        parser.skip_exception_spec();
        if (parser.is_cpp_mode() && parser.match(TokenKind::Arrow)) {
            ts = parser.parse_type_name();
            parser.parse_attributes(&ts);
        }
        parse_optional_cpp20_trailing_requires_clause(parser);

        // K&R style parameter declarations before body:
        //   f(a, b) int a; short *b; { ... }
        // Bind declared types back to identifier list order.
        if (!knr_param_names.empty()) {
            std::vector<TextId> knr_param_name_text_ids;
            knr_param_name_text_ids.reserve(knr_param_names.size());
            for (const char* pnm : knr_param_names) {
                knr_param_name_text_ids.push_back(
                    parser.parser_text_id_for_token(kInvalidText,
                                                    pnm ? pnm : ""));
            }
            std::unordered_map<TextId, Node*> knr_param_decl_map;
            while (parser.is_type_start() && !parser.check(TokenKind::LBrace)) {
                TypeSpec knr_base = parser.parse_base_type();
                parser.parse_attributes(&knr_base);
                do {
                    TypeSpec knr_ts = knr_base;
                    knr_ts.array_size = -1;
                    knr_ts.array_rank = 0;
                    for (int i = 0; i < 8; ++i) knr_ts.array_dims[i] = -1;
                    knr_ts.is_ptr_to_array = false;
                    knr_ts.array_size_expr = nullptr;
                    const char* knr_name = nullptr;
                    TextId knr_name_text_id = kInvalidText;
                    parser.parse_declarator(
                        knr_ts, &knr_name, nullptr, nullptr, nullptr, nullptr,
                        nullptr, nullptr, nullptr, &knr_name_text_id);
                    if (!knr_name) continue;
                    if (knr_name_text_id == kInvalidText) {
                        knr_name_text_id =
                            parser.parser_text_id_for_token(kInvalidText,
                                                            knr_name);
                    }
                    if (knr_name_text_id == kInvalidText) continue;
                    bool listed = false;
                    for (TextId pnm_text_id : knr_param_name_text_ids) {
                        if (pnm_text_id == knr_name_text_id) { listed = true; break; }
                    }
                    if (!listed) continue;
                    Node* p = parser.make_node(NK_DECL, ln);
                    p->name = knr_name;
                    p->type = knr_ts;
                    knr_param_decl_map[knr_name_text_id] = p;
                } while (parser.match(TokenKind::Comma));
                parser.match(TokenKind::Semi);
            }
            params.clear();
            for (size_t i = 0; i < knr_param_names.size(); ++i) {
                const char* pnm = knr_param_names[i];
                const TextId pnm_text_id = knr_param_name_text_ids[i];
                auto it = knr_param_decl_map.find(pnm_text_id);
                if (it != knr_param_decl_map.end()) {
                    params.push_back(it->second);
                    continue;
                }
                // Old-style fallback: undeclared parameter defaults to int.
                Node* p = parser.make_node(NK_DECL, ln);
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
            while (parser.is_type_start() && !parser.check(TokenKind::LBrace)) {
                parser.skip_until(TokenKind::Semi);
            }
        }

        // Helper: attach explicit specialization args to NK_FUNCTION.
        auto attach_spec_args = [&](Node* fn) {
            if (spec_arg_types.empty()) return;
            int n = (int)spec_arg_types.size();
            fn->n_template_args = n;
            fn->template_arg_types = parser.arena_.alloc_array<TypeSpec>(n);
            fn->template_arg_is_value = parser.arena_.alloc_array<bool>(n);
            fn->template_arg_values = parser.arena_.alloc_array<long long>(n);
            fn->template_arg_nttp_names = parser.arena_.alloc_array<const char*>(n);
            fn->template_arg_exprs = parser.arena_.alloc_array<Node*>(n);
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
                if (const Parser::FnPtrTypedefInfo* info =
                        parser.find_typedef_fn_ptr_info(ret_typedef_name_id)) {
                    fn->ret_fn_ptr_params   = info->params;
                    fn->n_ret_fn_ptr_params = info->n_params;
                    fn->ret_fn_ptr_variadic = info->variadic;
                }
            }
        };

        if (parser.check(TokenKind::LBrace)) {
            ParserFunctionParamScopeGuard param_scope(parser, params);
            // Function definition
            bool saved_top = parser.active_context_state_.parsing_top_level_context;
            parser.active_context_state_.parsing_top_level_context = false;
            Node* body = parse_block(parser);
            parser.active_context_state_.parsing_top_level_context = saved_top;
            Node* fn = parser.make_node(NK_FUNCTION, ln);
            fn->type      = ts;
            fn->name      = scoped_decl_name;
            parser.apply_decl_namespace(fn, parser.current_namespace_context_id(), decl_name);
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
            fn->visibility = parser.pragma_state_.visibility;
            fn->execution_domain = parser.pragma_state_.execution_domain;
            fn->body      = body;
            fn->n_params  = (int)params.size();
            if (fn->n_params > 0) {
                fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
                for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
            }
            propagate_ret_fn_ptr(fn);
            attach_spec_args(fn);
            register_decl_known_fn_name();
            restore_owner_scope();
            return fn;
        }

        // Function declaration only
        parser.match(TokenKind::Semi);
        Node* fn = parser.make_node(NK_FUNCTION, ln);
        fn->type      = ts;
        fn->name      = scoped_decl_name;
        parser.apply_decl_namespace(fn, parser.current_namespace_context_id(), decl_name);
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
        fn->visibility = parser.pragma_state_.visibility;
        fn->execution_domain = parser.pragma_state_.execution_domain;
        fn->body      = nullptr;  // declaration only
        fn->n_params  = (int)params.size();
        if (fn->n_params > 0) {
            fn->params = parser.arena_.alloc_array<Node*>(fn->n_params);
            for (int i = 0; i < fn->n_params; ++i) fn->params[i] = params[i];
        }
        propagate_ret_fn_ptr(fn);
        attach_spec_args(fn);
        register_decl_known_fn_name();
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
        Node* gv = parser.make_node(NK_GLOBAL_VAR, ln);
        gv->type      = gts;
        if (is_constexpr) gv->type.is_const = true;
        gv->name      = gname;
        parser.apply_decl_namespace(gv, parser.current_namespace_context_id(), source_name);
        gv->init      = ginit;
        gv->is_static = is_static;
        gv->is_extern = is_extern;
        gv->is_constexpr = is_constexpr;
        gv->is_consteval = false;
        gv->linkage_spec = linkage_spec;
        gv->visibility = parser.pragma_state_.visibility;
        gv->execution_domain = parser.pragma_state_.execution_domain;
        gv->fn_ptr_params = fn_ptr_params;
        gv->n_fn_ptr_params = n_fn_ptr_params;
        gv->fn_ptr_variadic = fn_ptr_variadic;
        gv->ret_fn_ptr_params = ret_fn_ptr_params;
        gv->n_ret_fn_ptr_params = n_ret_fn_ptr_params;
        gv->ret_fn_ptr_variadic = ret_fn_ptr_variadic;
        // Phase C: propagate fn_ptr params from typedef if not set by declarator.
        if (gts.is_fn_ptr && n_fn_ptr_params == 0 && !fn_ptr_variadic) {
            if (const Parser::FnPtrTypedefInfo* info = parser.find_current_typedef_fn_ptr_info()) {
                gv->fn_ptr_params = info->params;
                gv->n_fn_ptr_params = info->n_params;
                gv->fn_ptr_variadic = info->variadic;
            }
        }
        if (source_name && gname && std::strcmp(gname, source_name) == 0) {
            parser.register_var_type_binding(source_name_text_id, gts);
        } else if (gname) {
            parser.register_var_type_binding(
                parser.parser_text_id_for_token(kInvalidText, gname), gts);
        }
        if (gname && ginit &&
            (gv->type.is_const || gv->is_constexpr) &&
            !gv->type.is_lvalue_ref && !gv->type.is_rvalue_ref &&
            gv->type.ptr_level == 0 && gv->type.array_rank == 0) {
            long long cv = 0;
            if (eval_const_int(ginit, &cv, &parser.definition_state_.struct_tag_def_map,
                               &parser.binding_state_.const_int_bindings)) {
                const TextId binding_name_text_id =
                    source_name_text_id != kInvalidText
                        ? source_name_text_id
                        : parser.parser_text_id_for_token(
                              kInvalidText, source_name ? source_name : "");
                if (binding_name_text_id != kInvalidText) {
                    parser.binding_state_.const_int_bindings[binding_name_text_id] = cv;
                }
            }
        }
        return gv;
    };

    Node* first_init = nullptr;
    if (parser.match(TokenKind::Assign) ||
        (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) {
        first_init = parse_initializer(parser);
    }
    gvars.push_back(make_gvar(ts, scoped_decl_name, decl_name, decl_name_text_id,
                              first_init, decl_fn_ptr_params,
                              decl_n_fn_ptr_params, decl_fn_ptr_variadic,
                              decl_ret_fn_ptr_params, decl_n_ret_fn_ptr_params,
                              decl_ret_fn_ptr_variadic));

    while (parser.match(TokenKind::Comma)) {
        TypeSpec ts2 = base_ts;
        // Preserve typedef ptr_level and array dims for each additional declarator.
        ts2.array_size_expr = nullptr;
        const char* n2 = nullptr;
        TextId n2_text_id = kInvalidText;
        Node** fn_ptr_params2 = nullptr;
        int n_fn_ptr_params2 = 0;
        bool fn_ptr_variadic2 = false;
        parser.parse_declarator(ts2, &n2, &fn_ptr_params2, &n_fn_ptr_params2,
                         &fn_ptr_variadic2, nullptr, nullptr, nullptr, nullptr,
                         &n2_text_id);
        parser.skip_attributes();
        parser.skip_asm();
        Node* init2 = nullptr;
        if (parser.match(TokenKind::Assign) ||
            (parser.is_cpp_mode() && parser.check(TokenKind::LBrace))) init2 = parse_initializer(parser);
        if (n2) {
            const char* scoped_n2 = parser.qualify_name_arena(n2_text_id, n2);
            gvars.push_back(make_gvar(ts2, scoped_n2, n2, n2_text_id, init2,
                                          fn_ptr_params2, n_fn_ptr_params2,
                                          fn_ptr_variadic2));
        }
    }
    parser.match(TokenKind::Semi);

    restore_owner_scope();
    if (gvars.size() == 1) return gvars[0];

    // Multiple global vars — wrap in a block
    Node* blk = parser.make_node(NK_BLOCK, ln);
    blk->n_children = (int)gvars.size();
    blk->children   = parser.arena_.alloc_array<Node*>(blk->n_children);
    for (int i = 0; i < blk->n_children; ++i) blk->children[i] = gvars[i];
    return blk;
}


}  // namespace c4c

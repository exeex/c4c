// types_helpers.hpp — shared anonymous-namespace and static helper code
// included by parser implementation translation units. Each translation unit
// gets its own copy (internal linkage), which is intentional.

#pragma once

// NOTE: This file is meant to be #included inside a .cpp that has already
// included parser.hpp, lexer.hpp and the standard library headers needed below.

#include <cstring>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace c4c {

namespace {

bool skip_cpp20_constraint_atom(Parser& parser);

using ParsedTemplateArg = Parser::TemplateArgParseResult;

std::string resolve_qualified_known_type_name(
    const Parser& parser,
    const Parser::QualifiedNameRef& qn);

bool match_floatn_keyword_base(std::string_view name, TypeBase* out_base) {
    TypeBase base = TB_INT;
    if (name == "_Float16" || name == "_Float32") {
        base = TB_FLOAT;
    } else if (name == "_Float64" || name == "_Float32x") {
        base = TB_DOUBLE;
    } else if (name == "__float128" || name == "_Float128" || name == "_Float64x" ||
               name == "_Float128x") {
        base = TB_LONGDOUBLE;
    } else {
        return false;
    }
    if (out_base) *out_base = base;
    return true;
}

bool skip_balanced_template_arg_tokens(const std::vector<Token>& tokens, int* pos) {
    if (!pos || *pos < 0 || *pos >= static_cast<int>(tokens.size()) ||
        tokens[*pos].kind != TokenKind::Less) {
        return false;
    }

    int depth = 0;
    while (*pos < static_cast<int>(tokens.size())) {
        const TokenKind kind = tokens[*pos].kind;
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
}

bool token_can_follow_value_like_template_arg(TokenKind kind) {
    switch (kind) {
        case TokenKind::Comma:
        case TokenKind::Greater:
        case TokenKind::GreaterGreater:
        case TokenKind::RParen:
        case TokenKind::RBracket:
        case TokenKind::RBrace:
        case TokenKind::Semi:
        case TokenKind::Ellipsis:
        case TokenKind::Question:
        case TokenKind::Colon:
        case TokenKind::Assign:
        case TokenKind::EqualEqual:
        case TokenKind::BangEqual:
        case TokenKind::Less:
        case TokenKind::LessEqual:
        case TokenKind::GreaterEqual:
        case TokenKind::Plus:
        case TokenKind::Minus:
        case TokenKind::Star:
        case TokenKind::Slash:
        case TokenKind::Percent:
        case TokenKind::Amp:
        case TokenKind::AmpAmp:
        case TokenKind::Pipe:
        case TokenKind::PipePipe:
        case TokenKind::Caret:
        case TokenKind::Dot:
        case TokenKind::Arrow:
            return true;
        default:
            return false;
    }
}

std::string visible_type_head_name(const Parser& parser,
                                   TextId name_text_id,
                                   std::string_view name) {
    if (const TypeSpec* visible_type =
            parser.find_visible_typedef_type(name_text_id)) {
        if (visible_type->tag && visible_type->tag[0]) {
            return visible_type->tag;
        }
    }
    const Parser::VisibleNameResult resolved =
        parser.resolve_visible_type(name_text_id);
    return resolved ? parser.visible_name_spelling(resolved) : std::string(name);
}

std::string visible_type_head_name(const Parser& parser,
                                   std::string_view name) {
    return visible_type_head_name(parser, parser.find_parser_text_id(name), name);
}

Node* record_definition_in_context_by_text_id(const Parser& parser,
                                              int context_id,
                                              TextId name_text_id);

Node* type_spec_structured_record_definition(const Parser& parser,
                                             const TypeSpec* type) {
    if (!type) return nullptr;
    TypeSpec resolved = parser.resolve_struct_like_typedef_type(*type);
    if (Node* record = resolve_record_type_spec(resolved, nullptr)) {
        return record;
    }
    const TextId record_text_id =
        resolved.tag_text_id != kInvalidText
            ? resolved.tag_text_id
            : parser.find_parser_text_id(resolved.tag ? resolved.tag : "");
    if (record_text_id == kInvalidText) return nullptr;
    const int context_id = resolved.namespace_context_id >= 0
                               ? resolved.namespace_context_id
                               : parser.current_namespace_context_id();
    if (Node* record =
            record_definition_in_context_by_text_id(parser, context_id,
                                                    record_text_id)) {
        return record;
    }
    if (context_id != 0) {
        return record_definition_in_context_by_text_id(parser, 0,
                                                       record_text_id);
    }
    return nullptr;
}

bool type_spec_has_structured_record_definition(const Parser& parser,
                                                const TypeSpec* type) {
    return type_spec_structured_record_definition(parser, type) != nullptr;
}

bool visible_type_head_has_structured_record_definition(
    const Parser& parser, TextId name_text_id, std::string_view name) {
    return type_spec_has_structured_record_definition(
        parser, parser.find_visible_typedef_type(name_text_id));
}

bool visible_type_result_has_structured_record_definition(
    const Parser& parser, const Parser::VisibleNameResult& result) {
    if (!result) return false;
    return type_spec_has_structured_record_definition(
        parser, parser.find_structured_typedef_type(result.key));
}

Node* qualified_type_structured_record_definition(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    const Parser::VisibleNameResult result = parser.resolve_qualified_type(qn);
    return type_spec_structured_record_definition(
        parser, parser.find_structured_typedef_type(result.key));
}

bool qualified_type_has_structured_record_definition(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    return qualified_type_structured_record_definition(parser, qn) != nullptr;
}

Node* record_definition_in_context_by_text_id(const Parser& parser,
                                              int context_id,
                                              TextId name_text_id) {
    if (context_id < 0 || name_text_id == kInvalidText) return nullptr;

    for (const auto& entry : parser.definition_state_.struct_tag_def_map) {
        Node* record = entry.second;
        if (!record || record->kind != NK_STRUCT_DEF) continue;
        if (record->namespace_context_id != context_id) continue;
        if (record->unqualified_text_id == name_text_id) return record;
    }
    return nullptr;
}

Node* qualified_record_definition_in_context(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    if (qn.base_text_id == kInvalidText) return nullptr;
    const int context_id =
        qn.qualifier_segments.empty()
            ? (qn.is_global_qualified ? 0 : parser.current_namespace_context_id())
            : parser.resolve_namespace_context(qn);
    return record_definition_in_context_by_text_id(parser, context_id,
                                                   qn.base_text_id);
}

Node* qualified_type_record_definition_from_structured_authority(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    if (qn.base_text_id == kInvalidText) return nullptr;

    if (Node* record = qualified_record_definition_in_context(parser, qn)) {
        return record;
    }

    const bool is_qualified = !qn.qualifier_segments.empty() ||
                              qn.is_global_qualified;
    if (!is_qualified) {
        if (const TypeSpec* current_member_type =
                parser.find_typedef_type(
                    parser.current_record_member_name_key(qn.base_text_id))) {
            if (Node* record =
                    type_spec_structured_record_definition(
                        parser, current_member_type)) {
                return record;
            }
        }
        if (!parser.active_context_state_.current_struct_tag.empty()) {
            TextId current_record_text_id =
                parser.active_context_state_.current_struct_tag_text_id;
            if (current_record_text_id == kInvalidText) {
                current_record_text_id =
                    parser.find_parser_text_id(parser.current_struct_tag_text());
            }
            if (Node* current_record = record_definition_in_context_by_text_id(
                    parser, parser.current_namespace_context_id(),
                    current_record_text_id)) {
                if (current_record->n_member_typedefs > 0 &&
                    current_record->member_typedef_names &&
                    current_record->member_typedef_types) {
                    const std::string_view member_name =
                        parser.parser_text(qn.base_text_id, qn.base_name);
                    for (int i = 0; i < current_record->n_member_typedefs;
                         ++i) {
                        const char* candidate =
                            current_record->member_typedef_names[i];
                        if (!candidate || member_name != candidate) continue;
                        return type_spec_structured_record_definition(
                            parser,
                            &current_record->member_typedef_types[i]);
                    }
                }
            }
        }
        return type_spec_structured_record_definition(
            parser, parser.find_visible_typedef_type(qn.base_text_id));
    }

    const Parser::VisibleNameResult resolved_type =
        parser.resolve_qualified_type(qn);
    if (!resolved_type) return nullptr;
    return type_spec_structured_record_definition(
        parser, parser.find_structured_typedef_type(resolved_type.key));
}

bool qualified_type_owner_has_structured_authority(
    const Parser& parser, const Parser::QualifiedNameRef& owner_qn) {
    if (owner_qn.base_text_id == kInvalidText) return false;

    if (owner_qn.qualifier_segments.empty() &&
        !owner_qn.is_global_qualified &&
        parser.has_visible_typedef_type(owner_qn.base_text_id)) {
        return true;
    }

    if (parser.has_template_struct_primary(owner_qn) ||
        parser.find_template_struct_specializations(owner_qn) != nullptr ||
        qualified_record_definition_in_context(parser, owner_qn)) {
        return true;
    }

    const Parser::VisibleNameResult resolved_type =
        parser.resolve_qualified_type(owner_qn);
    return resolved_type &&
           parser.find_structured_typedef_type(resolved_type.key) != nullptr;
}

Node* qualified_enum_definition_in_context(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    if (qn.base_text_id == kInvalidText) return nullptr;
    const int context_id =
        qn.qualifier_segments.empty()
            ? (qn.is_global_qualified ? 0 : parser.current_namespace_context_id())
            : parser.resolve_namespace_context(qn);
    if (context_id < 0) return nullptr;

    for (Node* def : parser.definition_state_.struct_defs) {
        if (!def || def->kind != NK_ENUM_DEF) continue;
        if (def->namespace_context_id != context_id) continue;
        if (def->unqualified_text_id == qn.base_text_id) return def;
    }
    return nullptr;
}

bool qualified_name_from_text(const Parser& parser, std::string_view text,
                              Parser::QualifiedNameRef* out) {
    if (!out || text.empty()) return false;

    Parser::QualifiedNameRef qn;
    size_t start = 0;
    if (text.rfind("::", 0) == 0) {
        qn.is_global_qualified = true;
        start = 2;
    }
    if (start >= text.size()) return false;

    while (start < text.size()) {
        const size_t sep = text.find("::", start);
        const std::string segment =
            sep == std::string::npos ? std::string(text.substr(start))
                                     : std::string(text.substr(start, sep - start));
        if (segment.empty()) return false;
        const TextId segment_text_id =
            parser.parser_text_id_for_token(kInvalidText, segment);
        if (sep == std::string::npos) {
            qn.base_name = segment;
            qn.base_text_id = segment_text_id;
            *out = std::move(qn);
            return segment_text_id != kInvalidText;
        }
        qn.qualifier_segments.push_back(segment);
        qn.qualifier_text_ids.push_back(segment_text_id);
        start = sep + 2;
        if (start >= text.size()) return false;
    }
    return false;
}

Parser::QualifiedNameRef qualified_owner_name(const Parser& parser,
                                              const Parser::QualifiedNameRef& qn) {
    Parser::QualifiedNameRef owner_qn;
    if (qn.qualifier_segments.empty()) return owner_qn;

    owner_qn.is_global_qualified = qn.is_global_qualified;
    owner_qn.qualifier_segments.assign(qn.qualifier_segments.begin(),
                                       qn.qualifier_segments.end() - 1);
    if (!qn.qualifier_text_ids.empty()) {
        const size_t owner_qualifier_count =
            qn.qualifier_segments.size() > 0 ? qn.qualifier_segments.size() - 1 : 0;
        const size_t copy_count =
            owner_qualifier_count < qn.qualifier_text_ids.size()
                ? owner_qualifier_count
                : qn.qualifier_text_ids.size();
        owner_qn.qualifier_text_ids.assign(
            qn.qualifier_text_ids.begin(),
            qn.qualifier_text_ids.begin() + copy_count);
    }

    owner_qn.base_name = qn.qualifier_segments.back();
    if (qn.qualifier_text_ids.size() >= qn.qualifier_segments.size()) {
        owner_qn.base_text_id = qn.qualifier_text_ids[qn.qualifier_segments.size() - 1];
    } else {
        owner_qn.base_text_id =
            parser.parser_text_id_for_token(kInvalidText, owner_qn.base_name);
    }
    return owner_qn;
}

const TypeSpec* record_member_typedef_type_from_structured_authority(
    const Parser& parser, const Parser::QualifiedNameRef& qn) {
    if (qn.qualifier_segments.empty() || qn.base_text_id == kInvalidText) {
        return nullptr;
    }

    const Parser::QualifiedNameRef owner_qn = qualified_owner_name(parser, qn);
    const Node* owner =
        qualified_type_record_definition_from_structured_authority(parser,
                                                                   owner_qn);
    if (!owner || owner->n_member_typedefs <= 0 ||
        !owner->member_typedef_names || !owner->member_typedef_types) {
        return nullptr;
    }

    for (int i = 0; i < owner->n_member_typedefs; ++i) {
        const char* candidate = owner->member_typedef_names[i];
        if (!candidate) continue;
        if (parser.find_parser_text_id(candidate) == qn.base_text_id) {
            return &owner->member_typedef_types[i];
        }
    }
    return nullptr;
}

std::string resolve_qualified_owner_type_name(
    const Parser& parser,
    const Parser::QualifiedNameRef& qn) {
    if (qn.qualifier_segments.empty()) return {};

    const Parser::QualifiedNameRef owner_qn = qualified_owner_name(parser, qn);
    const std::string owner_name =
        resolve_qualified_known_type_name(parser, owner_qn);
    if (!owner_name.empty()) return owner_name;

    return std::string(parser.parser_text(owner_qn.base_text_id, owner_qn.base_name));
}

bool split_qualified_member_type_name(
    const Parser& parser, std::string_view text,
    Parser::QualifiedNameRef* owner_qn, std::string* member_name) {
    Parser::QualifiedNameRef qn;
    if (!qualified_name_from_text(parser, text, &qn) ||
        qn.qualifier_segments.empty()) {
        return false;
    }
    if (owner_qn) *owner_qn = qualified_owner_name(parser, qn);
    if (member_name) {
        *member_name = std::string(parser.parser_text(qn.base_text_id, qn.base_name));
    }
    return true;
}

void append_qualified_name_tokens(Parser& parser,
                                  std::vector<Token>* out,
                                  const Token& seed,
                                  const std::string& name) {
    if (!out || name.empty()) return;
    size_t start = 0;
    while (start < name.size()) {
        size_t sep = name.find("::", start);
        const std::string segment = sep == std::string::npos
            ? name.substr(start)
            : name.substr(start, sep - start);
        Token name_tok =
            parser.make_injected_token(seed, TokenKind::Identifier, segment);
        out->push_back(name_tok);
        if (sep == std::string::npos) break;
        Token cc_tok =
            parser.make_injected_token(seed, TokenKind::ColonColon, "::");
        out->push_back(cc_tok);
        start = sep + 2;
    }
}

void append_typespec_reparse_tokens(Parser& parser,
                                    std::vector<Token>* out,
                                    const Token& seed,
                                    const TypeSpec& ts) {
    if (!out) return;
    auto emit = [&](TokenKind kind, const char* lexeme) {
        Token tok = parser.make_injected_token(seed, kind, lexeme);
        out->push_back(tok);
    };

    if (ts.is_const) emit(TokenKind::KwConst, "const");
    if (ts.is_volatile) emit(TokenKind::KwVolatile, "volatile");

    bool emitted_head = false;
    if (ts.tag && ts.tag[0]) {
        append_qualified_name_tokens(parser, out, seed, ts.tag);
        emitted_head = true;
    } else {
        switch (ts.base) {
            case TB_VOID: emit(TokenKind::KwVoid, "void"); emitted_head = true; break;
            case TB_BOOL: emit(TokenKind::KwBool, "bool"); emitted_head = true; break;
            case TB_CHAR: emit(TokenKind::KwChar, "char"); emitted_head = true; break;
            case TB_SCHAR:
                emit(TokenKind::KwSigned, "signed");
                emit(TokenKind::KwChar, "char");
                emitted_head = true;
                break;
            case TB_UCHAR:
                emit(TokenKind::KwUnsigned, "unsigned");
                emit(TokenKind::KwChar, "char");
                emitted_head = true;
                break;
            case TB_SHORT: emit(TokenKind::KwShort, "short"); emitted_head = true; break;
            case TB_USHORT:
                emit(TokenKind::KwUnsigned, "unsigned");
                emit(TokenKind::KwShort, "short");
                emitted_head = true;
                break;
            case TB_INT: emit(TokenKind::KwInt, "int"); emitted_head = true; break;
            case TB_UINT:
                emit(TokenKind::KwUnsigned, "unsigned");
                emit(TokenKind::KwInt, "int");
                emitted_head = true;
                break;
            case TB_LONG: emit(TokenKind::KwLong, "long"); emitted_head = true; break;
            case TB_ULONG:
                emit(TokenKind::KwUnsigned, "unsigned");
                emit(TokenKind::KwLong, "long");
                emitted_head = true;
                break;
            case TB_LONGLONG:
                emit(TokenKind::KwLong, "long");
                emit(TokenKind::KwLong, "long");
                emitted_head = true;
                break;
            case TB_ULONGLONG:
                emit(TokenKind::KwUnsigned, "unsigned");
                emit(TokenKind::KwLong, "long");
                emit(TokenKind::KwLong, "long");
                emitted_head = true;
                break;
            case TB_FLOAT: emit(TokenKind::KwFloat, "float"); emitted_head = true; break;
            case TB_DOUBLE: emit(TokenKind::KwDouble, "double"); emitted_head = true; break;
            default:
                break;
        }
    }

    if (!emitted_head) {
        out->push_back(
            parser.make_injected_token(seed, TokenKind::Identifier, "int"));
        return;
    }

    for (int i = 0; i < ts.ptr_level; ++i) emit(TokenKind::Star, "*");
    if (ts.is_lvalue_ref) emit(TokenKind::Amp, "&");
    if (ts.is_rvalue_ref) emit(TokenKind::AmpAmp, "&&");
}

bool instantiate_template_struct_via_injected_parse(
    Parser& parser,
    const std::string& template_name,
    const std::vector<ParsedTemplateArg>& args,
    int line,
    const char* debug_reason = nullptr,
    TypeSpec* out_resolved = nullptr) {
    std::vector<Token> inject_toks;
    Token t{};
    t.line = line;
    t.column = 0;
    append_qualified_name_tokens(parser, &inject_toks, t, template_name);
    inject_toks.push_back(parser.make_injected_token(t, TokenKind::Less, "<"));
    for (int ai = 0; ai < static_cast<int>(args.size()); ++ai) {
        if (ai > 0) {
            inject_toks.push_back(
                parser.make_injected_token(t, TokenKind::Comma, ","));
        }
        if (args[ai].is_value) {
            if (args[ai].value == 0) {
                inject_toks.push_back(
                    parser.make_injected_token(t, TokenKind::KwFalse, "false"));
            } else if (args[ai].value == 1) {
                inject_toks.push_back(
                    parser.make_injected_token(t, TokenKind::KwTrue, "true"));
            } else {
                inject_toks.push_back(parser.make_injected_token(
                    t, TokenKind::IntLit, std::to_string(args[ai].value)));
            }
        } else {
            append_typespec_reparse_tokens(parser, &inject_toks, t,
                                           args[ai].type);
        }
    }
    inject_toks.push_back(parser.make_injected_token(t, TokenKind::Greater, ">"));
    inject_toks.push_back(parser.make_injected_token(t, TokenKind::Semi, ";"));

    return parser.parse_injected_base_type(std::move(inject_toks), debug_reason,
                                           out_resolved);
}

bool is_known_simple_type_head(const Parser& parser, TextId name_text_id,
                               std::string_view name) {
    if (match_floatn_keyword_base(name, nullptr)) return true;
    if (parser.is_template_scope_type_param(name_text_id)) return true;
    if (parser.is_typedef_name(name_text_id)) return true;
    if (parser.has_visible_typedef_type(name_text_id)) return true;
    const std::string resolved =
        visible_type_head_name(parser, name_text_id, name);
    if (visible_type_head_has_structured_record_definition(parser, name_text_id,
                                                           name)) {
        return true;
    }
    if (record_definition_in_context_by_text_id(
            parser, parser.current_namespace_context_id(), name_text_id)) {
        return true;
    }
    return parser.has_template_struct_primary(
               parser.current_namespace_context_id(), name_text_id) ||
           (!resolved.empty() &&
            parser.has_template_struct_primary(
                parser.current_namespace_context_id(),
                parser.parser_text_id_for_token(c4c::kInvalidText,
                                                resolved)));
}

bool is_known_simple_visible_type_head(const Parser& parser,
                                       TextId name_text_id,
                                       std::string_view name) {
    return parser.is_typedef_name(name_text_id) ||
           parser.is_template_scope_type_param(name_text_id) ||
           parser.has_visible_typedef_type(name_text_id);
}

bool is_visible_alias_template_head(const Parser& parser, TextId name_text_id) {
    if (name_text_id == kInvalidText) return false;
    if (parser.find_alias_template_info_in_context(
            parser.current_namespace_context_id(), name_text_id)) {
        return true;
    }

    const Parser::VisibleNameResult visible_type =
        parser.resolve_visible_type(name_text_id);
    return visible_type && parser.find_alias_template_info(visible_type.key);
}

bool starts_with_value_like_template_expr(const Parser& parser,
                                          const std::vector<Token>& tokens,
                                          int pos) {
    if (pos < 0 || pos >= static_cast<int>(tokens.size())) return false;

    const int start_pos = pos;
    if (tokens[pos].kind == TokenKind::ColonColon) ++pos;
    if (pos >= static_cast<int>(tokens.size()) ||
        tokens[pos].kind != TokenKind::Identifier) {
        return false;
    }

    const auto looks_like_variable_template = [](const std::string& name) {
        return name.size() >= 2 &&
               name.compare(name.size() - 2, 2, "_v") == 0;
    };

    bool saw_scope = tokens[start_pos].kind == TokenKind::ColonColon;
    const Token& first_identifier = tokens[pos];
    const TextId first_identifier_text_id =
        parser.parser_text_id_for_token(
            first_identifier.text_id, parser.token_spelling(first_identifier));
    if (pos + 1 < static_cast<int>(tokens.size()) &&
        tokens[pos + 1].kind == TokenKind::Less &&
        is_visible_alias_template_head(parser, first_identifier_text_id)) {
        return false;
    }
    const bool first_identifier_is_known_type =
        is_known_simple_type_head(parser, first_identifier_text_id,
                                  parser.token_spelling(first_identifier));

    while (pos < static_cast<int>(tokens.size())) {
        const std::string current_name =
            std::string(parser.token_spelling(tokens[pos]));
        ++pos;  // identifier
        bool saw_template_args = false;
        if (pos < static_cast<int>(tokens.size()) &&
            tokens[pos].kind == TokenKind::Less) {
            saw_template_args = true;
            if (!skip_balanced_template_arg_tokens(tokens, &pos)) {
                return false;
            }
        }

        if (pos < static_cast<int>(tokens.size()) &&
            tokens[pos].kind == TokenKind::LParen) {
            const int declarator_probe = pos + 1;
            if (declarator_probe < static_cast<int>(tokens.size())) {
                const TokenKind declarator_kind = tokens[declarator_probe].kind;
                if (declarator_kind == TokenKind::Star ||
                    declarator_kind == TokenKind::Caret ||
                    declarator_kind == TokenKind::Amp ||
                    declarator_kind == TokenKind::AmpAmp) {
                    return false;
                }
            }
            return saw_scope || saw_template_args || !first_identifier_is_known_type;
        }

        if (pos >= static_cast<int>(tokens.size())) {
            return saw_template_args &&
                   looks_like_variable_template(current_name);
        }

        if (tokens[pos].kind != TokenKind::ColonColon) {
            return saw_template_args &&
                   looks_like_variable_template(current_name) &&
                   token_can_follow_value_like_template_arg(tokens[pos].kind);
        }

        ++pos;  // ::
        saw_scope = true;
        if (pos < static_cast<int>(tokens.size()) &&
            tokens[pos].kind == TokenKind::KwTemplate) {
            ++pos;
        }
        if (pos >= static_cast<int>(tokens.size()) ||
            tokens[pos].kind != TokenKind::Identifier) {
            return false;
        }

        if (parser.token_spelling(tokens[pos]) == "value") {
            ++pos;
            if (pos >= static_cast<int>(tokens.size())) return true;
            return token_can_follow_value_like_template_arg(tokens[pos].kind);
        }
    }

    return false;
}

struct QualifiedTypeProbe {
    bool has_resolved_typedef = false;
    int namespace_context_id = -1;
    Node* record_def = nullptr;
    const TypeSpec* record_member_typedef_type = nullptr;
    std::string resolved_typedef_name;
    std::string spelled_name;
};

std::string qualified_name_text(const Parser& parser,
                                const Parser::QualifiedNameRef& qn,
                                bool include_global_prefix = true) {
    return parser.qualified_name_text(qn, include_global_prefix);
}

std::string resolve_qualified_typedef_name(const Parser& parser,
                                           const Parser::QualifiedNameRef& qn) {
    const bool is_qualified = !qn.qualifier_segments.empty() ||
                              qn.is_global_qualified;
    const std::string_view base_name =
        parser.parser_text(qn.base_text_id, qn.base_name);
    if (is_qualified) {
        const Parser::VisibleNameResult resolved_type =
            parser.resolve_qualified_type(qn);
        std::string resolved = parser.visible_name_spelling(resolved_type);
        if (resolved_type && !resolved.empty()) return resolved;
    } else {
        return visible_type_head_name(parser, qn.base_text_id, base_name);
    }

    const Parser::VisibleNameResult visible_type =
        parser.resolve_visible_type(qn.base_text_id);
    std::string resolved = parser.visible_name_spelling(visible_type);
    if (visible_type && parser.has_visible_typedef_type(qn.base_text_id))
        return resolved;
    if (!qn.qualifier_segments.empty() || qn.is_global_qualified) {
        return {};
    }
    return {};
}

std::string resolve_qualified_known_type_name(
    const Parser& parser,
    const Parser::QualifiedNameRef& qn) {
    std::string resolved = resolve_qualified_typedef_name(parser, qn);
    if (!resolved.empty()) return resolved;

    const bool is_qualified = !qn.qualifier_segments.empty() ||
                              qn.is_global_qualified;
    if (is_qualified) {
        const Parser::VisibleNameResult resolved_type =
            parser.resolve_qualified_type(qn);
        resolved = parser.visible_name_spelling(resolved_type);
        if (!resolved.empty() &&
            (visible_type_result_has_structured_record_definition(
                 parser, resolved_type) ||
             parser.has_template_struct_primary(qn) ||
             qualified_record_definition_in_context(parser, qn))) {
            return resolved;
        }
    } else {
        resolved = std::string(parser.parser_text(qn.base_text_id, qn.base_name));
        if (!resolved.empty() &&
            (visible_type_head_has_structured_record_definition(
                 parser, qn.base_text_id,
                 parser.parser_text(qn.base_text_id, qn.base_name)) ||
             record_definition_in_context_by_text_id(
                 parser, parser.current_namespace_context_id(),
                 qn.base_text_id) ||
             parser.has_template_struct_primary(
                 parser.current_namespace_context_id(), qn.base_text_id))) {
            return resolved;
        }
    }

    const Parser::VisibleNameResult visible_type =
        parser.resolve_visible_type(qn.base_text_id);
    resolved = parser.visible_name_spelling(visible_type);
    if (parser.has_template_struct_primary(
            parser.current_namespace_context_id(),
            parser.parser_text_id_for_token(c4c::kInvalidText, resolved)) ||
        visible_type_result_has_structured_record_definition(parser,
                                                             visible_type)) {
        return resolved;
    }
    return {};
}

QualifiedTypeProbe probe_qualified_type(const Parser& parser,
                                        const Parser::QualifiedNameRef& qn) {
    QualifiedTypeProbe probe;
    const bool is_qualified = !qn.qualifier_segments.empty() ||
                              qn.is_global_qualified;

    if (is_qualified) {
        probe.spelled_name = qualified_name_text(parser, qn);
        probe.namespace_context_id = parser.resolve_namespace_context(qn);

        if (const TypeSpec* member_typedef =
                record_member_typedef_type_from_structured_authority(parser,
                                                                     qn)) {
            probe.has_resolved_typedef = true;
            probe.record_member_typedef_type = member_typedef;
            probe.resolved_typedef_name = probe.spelled_name;
            return probe;
        }

        const Parser::VisibleNameResult resolved_type =
            parser.resolve_qualified_type(qn);
        const std::string resolved =
            parser.visible_name_spelling(resolved_type);
        if (resolved_type &&
            (parser.find_typedef_type(resolved_type.key) ||
             (resolved_type.source != Parser::VisibleNameSource::Fallback &&
              visible_type_result_has_structured_record_definition(
                  parser, resolved_type)))) {
            probe.has_resolved_typedef = true;
            probe.resolved_typedef_name = resolved;
            return probe;
        }

        if (Node* record = qualified_record_definition_in_context(parser, qn)) {
            probe.has_resolved_typedef = true;
            probe.record_def = record;
            probe.resolved_typedef_name =
                record->name && record->name[0]
                    ? std::string(record->name)
                    : qualified_name_text(parser, qn,
                                          /*include_global_prefix=*/false);
            return probe;
        }

        if (Node* enum_def = qualified_enum_definition_in_context(parser, qn)) {
            probe.has_resolved_typedef = true;
            probe.resolved_typedef_name =
                enum_def->name && enum_def->name[0]
                    ? std::string(enum_def->name)
                    : qualified_name_text(parser, qn,
                                          /*include_global_prefix=*/false);
            return probe;
        }

        if (parser.has_template_struct_primary(qn) ||
            parser.find_template_struct_specializations(qn) != nullptr) {
            probe.has_resolved_typedef = true;
            probe.resolved_typedef_name = probe.spelled_name;
        }
        return probe;
    }

    probe.resolved_typedef_name = resolve_qualified_known_type_name(parser, qn);
    if (parser.has_visible_typedef_type(qn.base_text_id)) {
        probe.has_resolved_typedef = true;
        return probe;
    }
    if (parser.has_template_struct_primary(
            parser.current_namespace_context_id(),
            parser.parser_text_id_for_token(c4c::kInvalidText,
                                            probe.resolved_typedef_name)) ||
        qualified_type_has_structured_record_definition(parser, qn) ||
        qualified_record_definition_in_context(parser, qn)) {
        probe.has_resolved_typedef = true;
        return probe;
    }

    return probe;
}

bool has_qualified_type_parse_fallback(const QualifiedTypeProbe& probe) {
    return probe.has_resolved_typedef;
}

bool starts_parenthesized_member_pointer_declarator(const Parser& parser,
                                                    int after_pos) {
    if (!parser.token_kind_at(after_pos, TokenKind::LParen)) {
        return false;
    }

    int probe = after_pos + 1;
    auto skip_attributes = [&]() {
        while (parser.token_kind_at(probe, TokenKind::KwAttribute)) {
            ++probe;
            for (int paren = 0; parser.token_kind_at(probe, TokenKind::LParen);) {
                ++paren;
                ++probe;
                while (probe < parser.token_count() && paren > 0) {
                    if (parser.token_kind_at(probe, TokenKind::LParen)) {
                        ++paren;
                    } else if (parser.token_kind_at(probe, TokenKind::RParen)) {
                        --paren;
                    }
                    ++probe;
                }
            }
        }
    };

    skip_attributes();
    if (parser.token_kind_at(probe, TokenKind::KwTypename)) {
        ++probe;
    }
    if (parser.token_kind_at(probe, TokenKind::ColonColon)) {
        ++probe;
    }
    if (!parser.token_kind_at(probe, TokenKind::Identifier)) {
        return false;
    }

    ++probe;
    while (parser.token_kind_at(probe, TokenKind::ColonColon) &&
           parser.token_kind_at(probe + 1, TokenKind::Identifier)) {
        probe += 2;
    }

    return parser.token_kind_at(probe, TokenKind::ColonColon) &&
           parser.token_kind_at(probe + 1, TokenKind::Star);
}

bool starts_qualified_member_pointer_type_id(const Parser& parser,
                                             int start_pos) {
    if (start_pos < 0 || start_pos >= parser.token_count()) {
        return false;
    }

    int probe = start_pos;
    bool saw_qualified = false;
    if (parser.token_kind_at(probe, TokenKind::ColonColon)) {
        saw_qualified = true;
        ++probe;
    }
    if (!parser.token_kind_at(probe, TokenKind::Identifier)) {
        return false;
    }

    ++probe;
    while (parser.token_kind_at(probe, TokenKind::ColonColon) &&
           parser.token_kind_at(probe + 1, TokenKind::Identifier)) {
        saw_qualified = true;
        probe += 2;
    }

    return saw_qualified &&
           starts_parenthesized_member_pointer_declarator(parser, probe);
}

bool can_start_qualified_type_declaration(const Parser& parser,
                                          const QualifiedTypeProbe& probe,
                                          int after_pos,
                                          TokenKind trailing_kind) {
    (void)parser;
    (void)after_pos;
    (void)trailing_kind;
    if (probe.has_resolved_typedef) return true;
    return false;
}

std::function<void(const char*)> make_record_field_duplicate_checker(
    Parser* parser,
    std::unordered_set<TextId>* field_names_seen) {
    return [parser, field_names_seen](const char* fname) {
        if (!fname || !field_names_seen)
            return;
        const std::string_view name(fname);
        if (name.empty())
            return;
        TextId name_text_id =
            parser ? parser->find_parser_text_id(name) : kInvalidText;
        if (name_text_id == kInvalidText && parser)
            name_text_id = parser->parser_text_id_for_token(kInvalidText, name);
        if (name_text_id == kInvalidText)
            return;
        if (field_names_seen->count(name_text_id)) {
            if (!parser || parser->is_cpp_mode())
                return;
            throw std::runtime_error(std::string("duplicate field name: ") +
                                     std::string(name));
        }
        field_names_seen->insert(name_text_id);
    };
}

bool parse_alignas_specifier(Parser* parser, TypeSpec* ts, int line) {
    if (!parser->check(TokenKind::KwAlignas)) return false;
    parser->consume();
    if (!parser->check(TokenKind::LParen)) return true;
    parser->consume();
    long long align_val = 0;
    bool have_align = false;
    if (!parser->check(TokenKind::RParen)) {
        if (parser->is_type_start()) {
            TypeSpec align_ts = parser->parse_type_name();
            Node* align_node = parser->make_node(NK_ALIGNOF_TYPE, line);
            align_node->type = align_ts;
            have_align =
                parser->eval_const_int_with_parser_tables(align_node, &align_val);
        } else {
            Node* align_expr = parse_assign_expr(*parser);
            have_align =
                parser->eval_const_int_with_parser_tables(align_expr, &align_val);
        }
    }
    parser->expect(TokenKind::RParen);
    if (ts && have_align && align_val > ts->align_bytes)
        ts->align_bytes = static_cast<int>(align_val);
    return true;
}

void consume_optional_cpp_member_virt_specifier_seq(Parser* parser) {
    if (!parser || !parser->is_cpp_mode())
        return;

    while (true) {
        if (parser->match(TokenKind::KwOverride))
            continue;
        if (parser->match(TokenKind::KwFinal))
            continue;
        break;
    }
}

void consume_optional_cpp_explicit_specifier(Parser* parser) {
    if (!parser || !parser->is_cpp_mode() || !parser->check(TokenKind::KwExplicit))
        return;

    parser->consume();
    if (parser->check(TokenKind::LParen))
        parser->skip_paren_group();
}

bool is_type_template_param(const Node* tpl_def, const char* name) {
    if (!tpl_def || !name) return false;
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
        if (!tpl_def->template_param_is_nttp[i] &&
            tpl_def->template_param_names[i] &&
            std::strcmp(tpl_def->template_param_names[i], name) == 0)
            return true;
    }
    return false;
}

bool is_value_template_param(const Node* tpl_def, const char* name) {
    if (!tpl_def || !name) return false;
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
        if (tpl_def->template_param_is_nttp[i] &&
            tpl_def->template_param_names[i] &&
            std::strcmp(tpl_def->template_param_names[i], name) == 0)
            return true;
    }
    return false;
}

TextId template_param_text_id(const Node* tpl_def, int param_index,
                              const Parser& parser) {
    if (!tpl_def || param_index < 0 ||
        param_index >= tpl_def->n_template_params ||
        !tpl_def->template_param_names) {
        return kInvalidText;
    }
    if (tpl_def->template_param_name_text_ids &&
        tpl_def->template_param_name_text_ids[param_index] != kInvalidText) {
        return tpl_def->template_param_name_text_ids[param_index];
    }
    const char* name = tpl_def->template_param_names[param_index];
    return name ? parser.parser_text_id_for_token(kInvalidText, name)
                : kInvalidText;
}

int find_template_param_index(const Node* tpl_def, TextId name_text_id,
                              const char* fallback_name,
                              const Parser& parser) {
    if (!tpl_def || !tpl_def->template_param_names) return -1;
    if (name_text_id == kInvalidText && fallback_name && fallback_name[0]) {
        name_text_id = parser.parser_text_id_for_token(kInvalidText,
                                                       fallback_name);
    }
    if (name_text_id != kInvalidText) {
        for (int i = 0; i < tpl_def->n_template_params; ++i) {
            if (template_param_text_id(tpl_def, i, parser) == name_text_id)
                return i;
        }
        return -1;
    }
    if (!fallback_name || !fallback_name[0]) return -1;
    for (int i = 0; i < tpl_def->n_template_params; ++i) {
        const char* param_name = tpl_def->template_param_names[i];
        if (param_name && std::strcmp(param_name, fallback_name) == 0)
            return i;
    }
    return -1;
}

TextId type_template_param_text_id(const Node* tpl_def, const char* name,
                                   const Parser& parser) {
    const int index = find_template_param_index(tpl_def, kInvalidText, name,
                                                parser);
    if (index < 0 || !tpl_def->template_param_is_nttp ||
        tpl_def->template_param_is_nttp[index]) {
        return kInvalidText;
    }
    return template_param_text_id(tpl_def, index, parser);
}

TextId value_template_param_text_id(const Node* tpl_def, const char* name,
                                    const Parser& parser) {
    const int index = find_template_param_index(tpl_def, kInvalidText, name,
                                                parser);
    if (index < 0 || !tpl_def->template_param_is_nttp ||
        !tpl_def->template_param_is_nttp[index]) {
        return kInvalidText;
    }
    return template_param_text_id(tpl_def, index, parser);
}

TypeSpec strip_pattern_qualifiers(TypeSpec ts, const TypeSpec& pattern) {
    if (pattern.is_const) ts.is_const = false;
    if (pattern.is_volatile) ts.is_volatile = false;
    ts.ptr_level -= pattern.ptr_level;
    if (ts.ptr_level < 0) ts.ptr_level = 0;
    if (pattern.is_lvalue_ref) ts.is_lvalue_ref = false;
    if (pattern.is_rvalue_ref) ts.is_rvalue_ref = false;
    if (pattern.array_rank > 0) {
        ts.array_rank -= pattern.array_rank;
        if (ts.array_rank < 0) ts.array_rank = 0;
        ts.array_size = ts.array_rank > 0 ? ts.array_dims[0] : -1;
    }
    return ts;
}

bool match_type_pattern(const TypeSpec& pattern_raw, const TypeSpec& actual_raw,
                        const Node* tpl_def, const Parser& parser,
                        std::unordered_map<TextId, TypeSpec>* type_bindings) {
    TypeSpec pattern = parser.resolve_typedef_type_chain(pattern_raw);
    TypeSpec actual = parser.resolve_typedef_type_chain(actual_raw);
    if (pattern.is_const && !actual.is_const) return false;
    if (pattern.is_volatile && !actual.is_volatile) return false;
    if (pattern.ptr_level > actual.ptr_level) return false;
    if (pattern.is_lvalue_ref && !actual.is_lvalue_ref) return false;
    if (pattern.is_rvalue_ref && !actual.is_rvalue_ref) return false;
    if (pattern.array_rank > actual.array_rank) return false;

    if (pattern.base == TB_TYPEDEF && pattern.tag &&
        type_template_param_text_id(tpl_def, pattern.tag, parser) != kInvalidText) {
        const TextId param_text_id =
            type_template_param_text_id(tpl_def, pattern.tag, parser);
        TypeSpec bound = strip_pattern_qualifiers(actual, pattern);
        auto it = type_bindings->find(param_text_id);
        if (it == type_bindings->end()) {
            (*type_bindings)[param_text_id] = bound;
            return true;
        }
        return parser.are_types_compatible(it->second, bound);
    }

    return parser.are_types_compatible(pattern, actual);
}

int specialization_match_score(const Node* spec) {
    if (!spec) return -1;
    if (spec->n_template_params == 0) return 100000;
    int score = 0;
    for (int i = 0; i < spec->n_template_args; ++i) {
        if (spec->template_arg_is_value && spec->template_arg_is_value[i]) {
            if (!(spec->template_arg_nttp_names && spec->template_arg_nttp_names[i])) score += 8;
            continue;
        }
        const TypeSpec& ts = spec->template_arg_types[i];
        if (ts.base != TB_TYPEDEF || !spec->template_param_names) score += 8;
        else if (!is_type_template_param(spec, ts.tag)) score += 4;
        if (ts.is_const || ts.is_volatile || ts.ptr_level > 0 ||
            ts.is_lvalue_ref || ts.is_rvalue_ref || ts.array_rank > 0)
            score += 4;
    }
    return score;
}

std::string canonical_template_struct_type_key(const TypeSpec& ts) {
    std::function<std::string(const TemplateArgRef&)> canonical_arg_key =
        [&](const TemplateArgRef& arg) -> std::string {
            if (arg.kind == TemplateArgKind::Value) {
                return std::string("v:") + std::to_string(arg.value);
            }
            std::string type_key = canonical_template_struct_type_key(arg.type);
            if (!type_key.empty() && type_key != "unknown") {
                return std::string("t:") + type_key;
            }
            return arg.debug_text && arg.debug_text[0] ? arg.debug_text : "t:?";
        };
    std::string out;
    if (ts.is_const) out += "const_";
    if (ts.is_volatile) out += "volatile_";
    switch (ts.base) {
        case TB_VOID: out += "void"; break;
        case TB_BOOL: out += "bool"; break;
        case TB_CHAR: out += "char"; break;
        case TB_SCHAR: out += "schar"; break;
        case TB_UCHAR: out += "uchar"; break;
        case TB_SHORT: out += "short"; break;
        case TB_USHORT: out += "ushort"; break;
        case TB_INT: out += "int"; break;
        case TB_UINT: out += "uint"; break;
        case TB_LONG: out += "long"; break;
        case TB_ULONG: out += "ulong"; break;
        case TB_LONGLONG: out += "llong"; break;
        case TB_ULONGLONG: out += "ullong"; break;
        case TB_FLOAT: out += "float"; break;
        case TB_DOUBLE: out += "double"; break;
        case TB_LONGDOUBLE: out += "ldouble"; break;
        case TB_INT128: out += "i128"; break;
        case TB_UINT128: out += "u128"; break;
        case TB_STRUCT:
            out += ts.tag ? std::string("struct.") + ts.tag : "struct.?";
            break;
        case TB_UNION:
            out += ts.tag ? std::string("union.") + ts.tag : "union.?";
            break;
        case TB_ENUM:
            out += ts.tag ? std::string("enum.") + ts.tag : "enum.?";
            break;
        case TB_TYPEDEF:
            if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
                out += std::string("pending.") + ts.tpl_struct_origin;
                out += "<";
                if (ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0) {
                    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
                        if (i > 0) out += ",";
                        out += canonical_arg_key(ts.tpl_struct_args.data[i]);
                    }
                }
                out += ">";
            } else {
                out += ts.tag ? std::string("typedef.") + ts.tag : "typedef.?";
            }
            break;
        default:
            if (ts.tpl_struct_origin && ts.tpl_struct_origin[0]) {
                out += std::string("pending.") + ts.tpl_struct_origin;
                out += "<";
                if (ts.tpl_struct_args.data && ts.tpl_struct_args.size > 0) {
                    for (int i = 0; i < ts.tpl_struct_args.size; ++i) {
                        if (i > 0) out += ",";
                        out += canonical_arg_key(ts.tpl_struct_args.data[i]);
                    }
                }
                out += ">";
            } else {
                out += "unknown";
            }
            break;
    }
    for (int i = 0; i < ts.ptr_level; ++i) out += "*";
    if (ts.is_lvalue_ref) out += "&";
    if (ts.is_rvalue_ref) out += "&&";
    return out;
}

ParserTemplateState::TemplateInstantiationKey::Argument
make_template_instantiation_argument_key(const ParsedTemplateArg& arg) {
    ParserTemplateState::TemplateInstantiationKey::Argument key;
    key.is_value = arg.is_value;
    if (arg.is_value) {
        if (!arg.captured_expr_tokens.empty()) {
            key.canonical_key = "$tokens:";
            for (const Token& tok : arg.captured_expr_tokens) {
                key.canonical_key += std::to_string(static_cast<int>(tok.kind));
                key.canonical_key += "#";
                key.canonical_key += std::to_string(tok.text_id);
                key.canonical_key += ";";
            }
        } else if (arg.nttp_name && arg.nttp_name[0] &&
            std::strncmp(arg.nttp_name, "$expr:", 6) == 0) {
            key.canonical_key = arg.nttp_name;
        } else {
            key.canonical_key = std::to_string(arg.value);
        }
    } else {
        key.canonical_key = canonical_template_struct_type_key(arg.type);
    }
    return key;
}

std::vector<ParserTemplateState::TemplateInstantiationKey::Argument>
make_template_instantiation_argument_keys(
    const std::vector<ParsedTemplateArg>& concrete_args) {
    std::vector<ParserTemplateState::TemplateInstantiationKey::Argument> keys;
    keys.reserve(concrete_args.size());
    for (const ParsedTemplateArg& arg : concrete_args) {
        keys.push_back(make_template_instantiation_argument_key(arg));
    }
    return keys;
}

const Node* select_template_struct_pattern(
    const std::vector<ParsedTemplateArg>& actual_args,
    const Node* primary_tpl,
    const std::vector<Node*>* specialization_patterns,
    const Parser& parser,
    std::vector<std::pair<std::string, TypeSpec>>* out_type_bindings,
    std::vector<std::pair<std::string, long long>>* out_nttp_bindings) {
    const Node* best = primary_tpl;
    int best_score = -1;

    auto try_candidate = [&](const Node* cand) {
        if (!cand) return;
        if (cand->n_template_args != static_cast<int>(actual_args.size())) return;
        std::unordered_map<TextId, TypeSpec> type_bindings_map;
        std::unordered_map<TextId, long long> value_bindings_map;
        for (int i = 0; i < cand->n_template_args; ++i) {
            const ParsedTemplateArg& actual = actual_args[i];
            const bool pattern_is_value =
                cand->template_arg_is_value && cand->template_arg_is_value[i];
            if (pattern_is_value != actual.is_value) return;
            if (pattern_is_value) {
                const char* pname = cand->template_arg_nttp_names ?
                    cand->template_arg_nttp_names[i] : nullptr;
                const TextId param_text_id =
                    value_template_param_text_id(cand, pname, parser);
                if (param_text_id != kInvalidText) {
                    auto it = value_bindings_map.find(param_text_id);
                    if (it == value_bindings_map.end())
                        value_bindings_map[param_text_id] = actual.value;
                    else if (it->second != actual.value) return;
                } else {
                    if (cand->template_arg_values[i] != actual.value) return;
                }
            } else {
                if (!match_type_pattern(cand->template_arg_types[i], actual.type,
                                        cand, parser, &type_bindings_map))
                    return;
            }
        }

        for (int i = 0; i < cand->n_template_params; ++i) {
            const char* pname = cand->template_param_names[i];
            if (!pname) continue;
            const TextId param_text_id = template_param_text_id(cand, i, parser);
            const bool has_default =
                cand->template_param_has_default &&
                cand->template_param_has_default[i];
            if (cand->template_param_is_nttp[i]) {
                if (!has_default &&
                    (param_text_id == kInvalidText ||
                     value_bindings_map.count(param_text_id) == 0))
                    return;
            } else {
                if (!has_default &&
                    (param_text_id == kInvalidText ||
                     type_bindings_map.count(param_text_id) == 0))
                    return;
            }
        }

        int score = specialization_match_score(cand);
        if (score <= best_score) return;
        best = cand;
        best_score = score;
        out_type_bindings->clear();
        out_nttp_bindings->clear();
        for (int i = 0; i < cand->n_template_params; ++i) {
            const char* pname = cand->template_param_names[i];
            if (!pname) continue;
            const TextId param_text_id = template_param_text_id(cand, i, parser);
            if (cand->template_param_is_nttp[i]) {
                auto it = param_text_id != kInvalidText
                    ? value_bindings_map.find(param_text_id)
                    : value_bindings_map.end();
                if (it != value_bindings_map.end()) {
                    out_nttp_bindings->push_back({pname, it->second});
                } else if (cand->template_param_has_default &&
                           cand->template_param_has_default[i]) {
                    out_nttp_bindings->push_back({pname, cand->template_param_default_values[i]});
                }
            } else {
                auto it = param_text_id != kInvalidText
                    ? type_bindings_map.find(param_text_id)
                    : type_bindings_map.end();
                if (it != type_bindings_map.end()) {
                    out_type_bindings->push_back({pname, it->second});
                } else if (cand->template_param_has_default &&
                           cand->template_param_has_default[i]) {
                    out_type_bindings->push_back({pname, cand->template_param_default_types[i]});
                }
            }
        }
    };

    if (specialization_patterns) {
        for (const Node* cand : *specialization_patterns) try_candidate(cand);
    }

    if (best != primary_tpl && best) return best;

    out_type_bindings->clear();
    out_nttp_bindings->clear();
    if (!primary_tpl) return nullptr;
    int arg_idx = 0;
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
        const char* param_name = primary_tpl->template_param_names[pi];
        const bool is_pack =
            primary_tpl->template_param_is_pack &&
            primary_tpl->template_param_is_pack[pi];
        if (is_pack) {
            while (arg_idx < static_cast<int>(actual_args.size())) {
                if (primary_tpl->template_param_is_nttp[pi]) {
                    if (!actual_args[arg_idx].is_value) return nullptr;
                    out_nttp_bindings->push_back({param_name, actual_args[arg_idx].value});
                } else {
                    if (actual_args[arg_idx].is_value) return nullptr;
                    out_type_bindings->push_back({param_name, actual_args[arg_idx].type});
                }
                ++arg_idx;
            }
            continue;
        }
        if (arg_idx < static_cast<int>(actual_args.size())) {
            if (primary_tpl->template_param_is_nttp[pi]) {
                if (!actual_args[arg_idx].is_value) return nullptr;
                out_nttp_bindings->push_back({param_name, actual_args[arg_idx].value});
            } else {
                if (actual_args[arg_idx].is_value) return nullptr;
                out_type_bindings->push_back({param_name, actual_args[arg_idx].type});
            }
            ++arg_idx;
            continue;
        }
        if (primary_tpl->template_param_has_default &&
            primary_tpl->template_param_has_default[pi]) {
            if (primary_tpl->template_param_is_nttp[pi]) {
                out_nttp_bindings->push_back(
                    {param_name, primary_tpl->template_param_default_values[pi]});
            } else {
                out_type_bindings->push_back(
                    {param_name, primary_tpl->template_param_default_types[pi]});
            }
            continue;
        }
        return nullptr;
    }
    if (arg_idx != static_cast<int>(actual_args.size())) {
        return nullptr;
    }
    return primary_tpl;
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

bool is_cpp20_requires_clause_decl_boundary(Parser& parser) {
    if (parser.at_end()) return true;
    TokenKind kind = parser.cur().kind;
    if (kind == TokenKind::Identifier) {
        const int current_pos = parser.current_token_index();
        if (current_pos > 0 &&
            parser.token_kind_at(current_pos - 1, TokenKind::ColonColon)) {
            return false;
        }
        if (parser.token_kind_at(current_pos + 1, TokenKind::Less)) {
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

bool looks_like_record_member_decl_boundary(Parser& parser) {
    if (parser.at_end()) {
        return false;
    }
    const TokenKind start_kind = parser.cur().kind;
    if (!(start_kind == TokenKind::KwTemplate ||
          start_kind == TokenKind::KwFriend ||
          start_kind == TokenKind::KwUsing ||
          start_kind == TokenKind::KwStaticAssert ||
          start_kind == TokenKind::KwConstexpr ||
          start_kind == TokenKind::KwConsteval ||
          start_kind == TokenKind::KwExplicit ||
          start_kind == TokenKind::KwInline ||
          start_kind == TokenKind::KwStatic ||
          start_kind == TokenKind::KwExtern ||
          start_kind == TokenKind::KwMutable ||
          start_kind == TokenKind::KwVirtual ||
          start_kind == TokenKind::KwTypedef ||
          start_kind == TokenKind::KwConcept ||
          start_kind == TokenKind::KwStruct ||
          start_kind == TokenKind::KwClass ||
          start_kind == TokenKind::KwUnion ||
          start_kind == TokenKind::KwEnum ||
          start_kind == TokenKind::KwTypename ||
          start_kind == TokenKind::KwAuto ||
          start_kind == TokenKind::KwAlignas ||
          is_type_kw(start_kind) || is_qualifier(start_kind) ||
          is_storage_class(start_kind))) {
        return false;
    }

    int angle_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;
    bool saw_identifier = false;
    for (int i = parser.current_token_index(); i < parser.token_count(); ++i) {
        const TokenKind kind = parser.token_kind(i);
        if (kind == TokenKind::Identifier) {
            saw_identifier = true;
        }

        if (kind == TokenKind::Less) {
            ++angle_depth;
            continue;
        }
        if (kind == TokenKind::Greater && angle_depth > 0) {
            --angle_depth;
            continue;
        }
        if (kind == TokenKind::GreaterGreater && angle_depth > 0) {
            angle_depth -= std::min(angle_depth, 2);
            continue;
        }
        if (kind == TokenKind::LParen) {
            if (angle_depth == 0 && bracket_depth == 0 && paren_depth == 0 &&
                saw_identifier) {
                return true;
            }
            ++paren_depth;
            continue;
        }
        if (kind == TokenKind::RParen) {
            if (paren_depth == 0) return false;
            --paren_depth;
            continue;
        }
        if (kind == TokenKind::LBracket) {
            ++bracket_depth;
            continue;
        }
        if (kind == TokenKind::RBracket) {
            if (bracket_depth == 0) return false;
            --bracket_depth;
            continue;
        }
        if (kind == TokenKind::LBrace) {
            ++brace_depth;
            continue;
        }
        if (kind == TokenKind::RBrace) {
            if (brace_depth > 0) {
                --brace_depth;
                continue;
            }
            return false;
        }

        if (angle_depth != 0 || paren_depth != 0 || bracket_depth != 0 ||
            brace_depth != 0) {
            continue;
        }

        if (saw_identifier && kind == TokenKind::Semi) {
            for (int j = i + 1; j < parser.token_count(); ++j) {
                const TokenKind next_kind = parser.token_kind(j);
                if (next_kind == TokenKind::RBrace) {
                    return true;
                }
                if (next_kind != TokenKind::Semi) {
                    break;
                }
            }
        }
        if (kind == TokenKind::AmpAmp || kind == TokenKind::PipePipe ||
            kind == TokenKind::Comma) {
            return false;
        }
    }

    return false;
}

bool is_record_member_recovery_boundary(Parser& parser,
                                        int member_start_pos) {
    if (!parser.is_cpp_mode() || parser.at_end() ||
        member_start_pos < 0 ||
        member_start_pos >= parser.token_count() ||
        parser.current_token_index() <= member_start_pos) {
        return false;
    }

    return looks_like_record_member_decl_boundary(parser);
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

    if (is_cpp20_requires_clause_decl_boundary(parser) &&
        !parser.check(TokenKind::Identifier)) {
        return false;
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
        throw std::runtime_error("expected constraint-expression after requires");
    }
}

}  // namespace

// ── static file-scope helpers (used by multiple sections) ────────────────────

static void append_type_mangled_suffix(std::string& out, const TypeSpec& ts);
static std::string capture_template_arg_expr_text(
    const Parser& parser, const std::vector<Token>& toks, int start, int end);
static bool parse_builtin_typespec_text(const std::string& text, TypeSpec* out);
static int find_template_arg_expr_end(const std::vector<Token>& tokens, int start_pos);

static bool parse_builtin_typespec_text(const std::string& text, TypeSpec* out) {
    if (!out) return false;
    auto trim_local = [](std::string s0) {
        size_t start = 0;
        while (start < s0.size() &&
               std::isspace(static_cast<unsigned char>(s0[start]))) {
            ++start;
        }
        size_t end = s0.size();
        while (end > start &&
               std::isspace(static_cast<unsigned char>(s0[end - 1]))) {
            --end;
        }
        return s0.substr(start, end - start);
    };
    std::string s = trim_local(text);
    if (s.empty()) return false;

    TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;

    auto strip_prefix = [&](const char* prefix, bool* flag) {
        const std::string p(prefix);
        if (s.rfind(p, 0) == 0) {
            *flag = true;
            s = trim_local(s.substr(p.size()));
            return true;
        }
        return false;
    };

    bool progress = true;
    while (progress) {
        progress = false;
        progress |= strip_prefix("const ", &ts.is_const);
        progress |= strip_prefix("volatile ", &ts.is_volatile);
    }

    if (s == "void") ts.base = TB_VOID;
    else if (s == "bool") ts.base = TB_BOOL;
    else if (s == "char") ts.base = TB_CHAR;
    else if (s == "signed char") ts.base = TB_SCHAR;
    else if (s == "unsigned char") ts.base = TB_UCHAR;
    else if (s == "short") ts.base = TB_SHORT;
    else if (s == "unsigned short") ts.base = TB_USHORT;
    else if (s == "int") ts.base = TB_INT;
    else if (s == "unsigned" || s == "unsigned int") ts.base = TB_UINT;
    else if (s == "long") ts.base = TB_LONG;
    else if (s == "unsigned long") ts.base = TB_ULONG;
    else if (s == "long long") ts.base = TB_LONGLONG;
    else if (s == "unsigned long long") ts.base = TB_ULONGLONG;
    else if (s == "float") ts.base = TB_FLOAT;
    else if (s == "double") ts.base = TB_DOUBLE;
    else return false;

    *out = ts;
    return true;
}

// Reverse of append_type_mangled_suffix: convert mangled suffix back to TypeSpec.
static bool parse_mangled_type_suffix(const std::string& text, TypeSpec* out) {
    if (!out) return false;
    TypeSpec ts{};
    ts.array_size = -1;
    ts.inner_rank = -1;
    std::string base_text = text;
    while (true) {
        if (base_text.rfind("const_", 0) == 0) {
            ts.is_const = true;
            base_text = base_text.substr(6);
            continue;
        }
        if (base_text.rfind("volatile_", 0) == 0) {
            ts.is_volatile = true;
            base_text = base_text.substr(9);
            continue;
        }
        break;
    }
    if (base_text == "int") ts.base = TB_INT;
    else if (base_text == "uint") ts.base = TB_UINT;
    else if (base_text == "char") ts.base = TB_CHAR;
    else if (base_text == "schar") ts.base = TB_SCHAR;
    else if (base_text == "uchar") ts.base = TB_UCHAR;
    else if (base_text == "short") ts.base = TB_SHORT;
    else if (base_text == "ushort") ts.base = TB_USHORT;
    else if (base_text == "long") ts.base = TB_LONG;
    else if (base_text == "ulong") ts.base = TB_ULONG;
    else if (base_text == "llong") ts.base = TB_LONGLONG;
    else if (base_text == "ullong") ts.base = TB_ULONGLONG;
    else if (base_text == "float") ts.base = TB_FLOAT;
    else if (base_text == "double") ts.base = TB_DOUBLE;
    else if (base_text == "ldouble") ts.base = TB_LONGDOUBLE;
    else if (base_text == "void") ts.base = TB_VOID;
    else if (base_text == "bool") ts.base = TB_BOOL;
    else if (base_text == "i128") ts.base = TB_INT128;
    else if (base_text == "u128") ts.base = TB_UINT128;
    else return false;
    *out = ts;
    return true;
}

static void append_type_mangled_suffix(std::string& out, const TypeSpec& ts) {
    if (ts.is_const) out += "const_";
    if (ts.is_volatile) out += "volatile_";
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

static std::string capture_template_arg_expr_text(
    const Parser& parser, const std::vector<Token>& tokens, int start_pos,
    int end_pos) {
    std::string text;
    for (int i = start_pos; i < end_pos && i < static_cast<int>(tokens.size()); ++i)
        text += parser.token_spelling(tokens[i]);
    return text;
}

static int find_template_arg_expr_end(const std::vector<Token>& tokens, int start_pos) {
    auto can_open_nested_template = [&](int idx) -> bool {
        if (idx <= start_pos || idx >= static_cast<int>(tokens.size())) return false;
        const TokenKind prev = tokens[idx - 1].kind;
        switch (prev) {
            case TokenKind::Identifier:
            case TokenKind::ColonColon:
            case TokenKind::Greater:
            case TokenKind::GreaterGreater:
                return true;
            default:
                return false;
        }
    };
    int scan_pos = start_pos;
    int angle_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;
    while (scan_pos < static_cast<int>(tokens.size())) {
        const TokenKind tk = tokens[scan_pos].kind;
        if (tk == TokenKind::LParen) ++paren_depth;
        else if (tk == TokenKind::RParen) {
            if (paren_depth > 0) --paren_depth;
        } else if (tk == TokenKind::LBracket) ++bracket_depth;
        else if (tk == TokenKind::RBracket) {
            if (bracket_depth > 0) --bracket_depth;
        } else if (tk == TokenKind::LBrace) ++brace_depth;
        else if (tk == TokenKind::RBrace) {
            if (brace_depth > 0) --brace_depth;
            else break;  // unbalanced } — not inside a template argument
        } else if (paren_depth == 0 && bracket_depth == 0 && brace_depth == 0) {
            // Semicolons and unbalanced closing braces cannot appear inside
            // a template argument expression — stop scanning so the caller
            // treats the tentative template parse as failed.
            if (tk == TokenKind::Semi) break;
            if (tk == TokenKind::Comma && angle_depth == 0) break;
            if (tk == TokenKind::Greater || tk == TokenKind::GreaterGreater) {
                if (angle_depth == 0) break;
                angle_depth -= std::min(angle_depth,
                                        tk == TokenKind::GreaterGreater ? 2 : 1);
            } else if (tk == TokenKind::Less && can_open_nested_template(scan_pos)) {
                ++angle_depth;
            }
        }
        ++scan_pos;
    }
    return scan_pos;
}

static std::vector<Token> lex_template_expr_text(const std::string& text,
                                                 SourceProfile source_profile) {
    Lexer lexer(text, lex_profile_from(source_profile));
    std::vector<Token> toks = lexer.scan_all();
    if (!toks.empty() && toks.back().kind == TokenKind::EndOfFile) toks.pop_back();
    return toks;
}

static const char* extra_operator_mangled_name(TokenKind kind) {
    switch (kind) {
        case TokenKind::AmpAmp: return "operator_and";
        case TokenKind::PipePipe: return "operator_or";
        case TokenKind::Bang: return "operator_not";
        case TokenKind::Tilde: return "operator_bitnot";
        case TokenKind::MinusMinus: return "operator_dec_pending";
        case TokenKind::PlusPlus: return "operator_inc_pending";
        case TokenKind::Slash: return "operator_div";
        case TokenKind::Percent: return "operator_mod";
        case TokenKind::Amp: return "operator_bitand";
        case TokenKind::Pipe: return "operator_bitor";
        case TokenKind::Caret: return "operator_bitxor";
        case TokenKind::LessLess: return "operator_shl";
        case TokenKind::GreaterGreater: return "operator_shr";
        case TokenKind::PlusAssign: return "operator_plus_assign";
        case TokenKind::MinusAssign: return "operator_minus_assign";
        case TokenKind::StarAssign: return "operator_mul_assign";
        case TokenKind::SlashAssign: return "operator_div_assign";
        case TokenKind::PercentAssign: return "operator_mod_assign";
        case TokenKind::AmpAssign: return "operator_and_assign";
        case TokenKind::PipeAssign: return "operator_or_assign";
        case TokenKind::CaretAssign: return "operator_xor_assign";
        case TokenKind::LessLessAssign: return "operator_shl_assign";
        case TokenKind::GreaterGreaterAssign: return "operator_shr_assign";
        case TokenKind::Spaceship: return "operator_spaceship";
        default: return nullptr;
    }
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

static void finalize_vector_type(TypeSpec& ts) {
    if (!ts.is_vector || ts.vector_bytes <= 0) return;
    long long elem_sz = 4;
    switch (ts.base) {
        case TB_CHAR: case TB_SCHAR: case TB_UCHAR: elem_sz = 1; break;
        case TB_SHORT: case TB_USHORT: elem_sz = 2; break;
        case TB_FLOAT: case TB_INT: case TB_UINT: elem_sz = 4; break;
        case TB_DOUBLE: case TB_LONG: case TB_ULONG:
        case TB_LONGLONG: case TB_ULONGLONG: elem_sz = 8; break;
        default: elem_sz = 4; break;
    }
    ts.vector_lanes = ts.vector_bytes / elem_sz;
}

}  // namespace c4c

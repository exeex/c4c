// base.cpp — base type parsing, skip helpers, parse_base_type
#include "../parser_impl.hpp"
#include "lexer.hpp"

#include <algorithm>
#include <climits>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "types_helpers.hpp"
#include "sema/type_utils.hpp"

namespace c4c {

static std::vector<std::string> split_template_arg_ref_text(
    const std::string& refs) {
    std::vector<std::string> parts;
    if (refs.empty()) return parts;
    size_t start = 0;
    int angle_depth = 0;
    int paren_depth = 0;
    int bracket_depth = 0;
    int brace_depth = 0;
    for (size_t i = 0; i < refs.size(); ++i) {
        switch (refs[i]) {
            case '<':
                ++angle_depth;
                break;
            case '>':
                if (angle_depth > 0) --angle_depth;
                break;
            case '(':
                ++paren_depth;
                break;
            case ')':
                if (paren_depth > 0) --paren_depth;
                break;
            case '[':
                ++bracket_depth;
                break;
            case ']':
                if (bracket_depth > 0) --bracket_depth;
                break;
            case '{':
                ++brace_depth;
                break;
            case '}':
                if (brace_depth > 0) --brace_depth;
                break;
            case ',':
                if (angle_depth == 0 && paren_depth == 0 &&
                    bracket_depth == 0 && brace_depth == 0) {
                    parts.push_back(refs.substr(start, i - start));
                    start = i + 1;
                }
                break;
            default:
                break;
        }
    }
    parts.push_back(refs.substr(start));
    return parts;
}

static QualifiedNameKey template_instantiation_name_key_for_direct_emit(
    Parser& parser,
    const Node* primary_tpl,
    std::string_view fallback_name) {
    if (!primary_tpl) return {};
    int context_id = primary_tpl->namespace_context_id >= 0
                         ? primary_tpl->namespace_context_id
                         : parser.current_namespace_context_id();
    const char* name = primary_tpl->unqualified_name;
    TextId name_text_id = primary_tpl->unqualified_text_id;
    if ((!name || !name[0]) && primary_tpl->name) {
        name = std::strrchr(primary_tpl->name, ':');
        name = name ? name + 1 : primary_tpl->name;
    }
    if (!name || !name[0]) {
        name = fallback_name.data();
    }
    if (name_text_id == kInvalidText && name && name[0]) {
        name_text_id = parser.parser_text_id_for_token(kInvalidText, name);
    }
    return parser.alias_template_key_in_context(context_id, name_text_id);
}

static ParserNttpBindingMetadata nttp_binding_metadata_for_template_param(
    Parser& parser,
    const Node* primary_tpl,
    int param_idx,
    const char* fallback_name,
    long long value) {
    ParserNttpBindingMetadata binding{};
    binding.name = fallback_name;
    binding.value = value;
    if (!primary_tpl || param_idx < 0 ||
        param_idx >= primary_tpl->n_template_params) {
        return binding;
    }
    if (primary_tpl->template_param_names &&
        primary_tpl->template_param_names[param_idx]) {
        binding.name = primary_tpl->template_param_names[param_idx];
    }
    if (primary_tpl->template_param_name_text_ids) {
        binding.name_text_id =
            primary_tpl->template_param_name_text_ids[param_idx];
    }
    if (binding.name_text_id == kInvalidText && binding.name &&
        binding.name[0]) {
        binding.name_text_id =
            parser.parser_text_id_for_token(kInvalidText, binding.name);
    }
    const int context_id = primary_tpl->namespace_context_id >= 0
                               ? primary_tpl->namespace_context_id
                               : parser.current_namespace_context_id();
    binding.name_key =
        parser.alias_template_key_in_context(context_id, binding.name_text_id);
    return binding;
}

static std::vector<ParserNttpBindingMetadata>
nttp_binding_metadata_for_template_params(
    Parser& parser,
    const Node* primary_tpl,
    const std::vector<std::pair<std::string, long long>>& nttp_bindings) {
    std::vector<ParserNttpBindingMetadata> bindings;
    if (!primary_tpl || !primary_tpl->template_param_is_nttp) return bindings;
    bindings.reserve(nttp_bindings.size());
    for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
        if (!primary_tpl->template_param_is_nttp[pi]) continue;
        const char* name = primary_tpl->template_param_names
                               ? primary_tpl->template_param_names[pi]
                               : nullptr;
        if (!name || !name[0]) continue;
        for (const auto& [binding_name, binding_value] : nttp_bindings) {
            if (binding_name == name) {
                bindings.push_back(nttp_binding_metadata_for_template_param(
                    parser, primary_tpl, pi, name, binding_value));
                break;
            }
        }
    }
    return bindings;
}

static bool has_template_instantiation_dedup_key_for_direct_emit(
    Parser& parser,
    const ParserTemplateState::TemplateInstantiationKey& structured_key) {
    if (structured_key.template_key.base_text_id == kInvalidText) {
        return false;
    }
    return parser.template_state_.instantiated_template_struct_keys_by_key.count(
               structured_key) > 0;
}

static void mark_template_instantiation_dedup_key_for_direct_emit(
    Parser& parser,
    const ParserTemplateState::TemplateInstantiationKey& structured_key) {
    if (structured_key.template_key.base_text_id != kInvalidText) {
        parser.template_state_.instantiated_template_struct_keys_by_key.insert(
            structured_key);
    }
}

bool Parser::is_type_start() const {
    TokenKind k = cur().kind;
    if (k == TokenKind::LBracket &&
        core_input_state_.pos + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::LBracket) {
        return true;
    }
    if (is_type_kw(k)) return true;
    if (is_qualifier(k)) return true;
    if (is_storage_class(k)) return true;
    if (k == TokenKind::KwConstexpr || k == TokenKind::KwConsteval || k == TokenKind::KwExplicit) return true;
    if (k == TokenKind::KwAttribute) return true;  // __attribute__((x)) type cast
    if (k == TokenKind::KwAlignas) return true;
    if (k == TokenKind::KwStaticAssert) return false;
    if (k == TokenKind::KwTypename) return true;
    if (k == TokenKind::Identifier) {
        const TextId name_text_id = cur().text_id;
        const std::string name(token_spelling(cur()));
        if (is_concept_name(name_text_id)) return false;
        if (find_local_visible_typedef_type(name_text_id)) return true;
        if (starts_with_value_like_template_expr(*this, core_input_state_.tokens,
                                                 core_input_state_.pos)) return false;
        if (match_floatn_keyword_base(name, nullptr)) return true;
        if (is_known_simple_visible_type_head(*this, name_text_id, name)) return true;
        // C++ fallback: identifier followed by < is likely a template type if
        // the name is registered as a template struct, or if we're inside a
        // struct body where namespace-scoped template names may not resolve.
        if (is_cpp_mode() &&
            core_input_state_.pos + 1 <
                static_cast<int>(core_input_state_.tokens.size()) &&
            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                TokenKind::Less &&
            (find_template_struct_primary(current_namespace_context_id(),
                                          name_text_id) ||
             find_template_struct_primary(
                 current_namespace_context_id(),
                 parser_text_id_for_token(
                     kInvalidText, visible_type_head_name(*this, name))) ||
             !current_struct_tag_text().empty())) return true;
        // Keep declaration probes aligned with parse_base_type(): even when
        // template-type registration was lost, `Identifier<...>` should still
        // be allowed to enter type parsing so the unresolved-template fallback
        // can recover declarations such as `holder<T> value`.
        if (is_cpp_mode() &&
            core_input_state_.pos + 1 <
                static_cast<int>(core_input_state_.tokens.size()) &&
            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                TokenKind::Less) return true;
    }
    if (k == TokenKind::ColonColon) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, true)) {
            const QualifiedTypeProbe probe = probe_qualified_type(*this, qn);
            const int after_pos =
                core_input_state_.pos + (qn.is_global_qualified ? 1 : 0) +
                2 * static_cast<int>(qn.qualifier_segments.size()) + 1;
            const TokenKind trailing_kind =
                after_pos < static_cast<int>(core_input_state_.tokens.size())
                    ? core_input_state_.tokens[after_pos].kind
                    : TokenKind::EndOfFile;
            if (trailing_kind == TokenKind::LParen &&
                starts_parenthesized_member_pointer_declarator(*this, after_pos)) {
                return true;
            }
            if (can_start_qualified_type_declaration(*this, probe, after_pos,
                                                     trailing_kind))
                return true;
        }
    }
    // C++ qualified type: StructName::TypedefName or ns::ns2::Type
    if (k == TokenKind::Identifier &&
        core_input_state_.pos + 2 < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::ColonColon &&
        core_input_state_.tokens[core_input_state_.pos + 2].kind == TokenKind::Identifier) {
        QualifiedNameRef qn;
        if (peek_qualified_name(&qn, false)) {
            const QualifiedTypeProbe probe = probe_qualified_type(*this, qn);
            const int after_pos =
                core_input_state_.pos + 1 +
                2 * static_cast<int>(qn.qualifier_segments.size());
            const TokenKind trailing_kind =
                after_pos < static_cast<int>(core_input_state_.tokens.size())
                    ? core_input_state_.tokens[after_pos].kind
                    : TokenKind::EndOfFile;
            if (trailing_kind == TokenKind::LParen &&
                starts_parenthesized_member_pointer_declarator(*this, after_pos)) {
                return true;
            }
            if (can_start_qualified_type_declaration(*this, probe, after_pos,
                                                     trailing_kind))
                return true;
        }
    }
    return false;
}

bool Parser::can_start_parameter_type() const {
    if (is_type_start()) return true;
    if (!is_cpp_mode()) return false;

    if (looks_like_unresolved_identifier_type_head(core_input_state_.pos)) return true;
    if (looks_like_unresolved_parenthesized_parameter_type_head(
            core_input_state_.pos)) {
        return true;
    }

    if (check(TokenKind::Identifier) &&
        core_input_state_.pos + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::Identifier) {
        return true;
    }

    if (check(TokenKind::Identifier) &&
        core_input_state_.pos + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
        (core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::Amp ||
         core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::AmpAmp)) {
        return true;
    }

    if (check(TokenKind::Identifier) &&
        core_input_state_.pos + 1 < static_cast<int>(core_input_state_.tokens.size()) &&
        core_input_state_.tokens[core_input_state_.pos + 1].kind == TokenKind::Less) {
        return true;
    }

    if (!(check(TokenKind::ColonColon) ||
          (check(TokenKind::Identifier) &&
           core_input_state_.pos + 1 <
               static_cast<int>(core_input_state_.tokens.size()) &&
           core_input_state_.tokens[core_input_state_.pos + 1].kind ==
               TokenKind::ColonColon))) {
        return false;
    }

    QualifiedNameRef qn;
    if (!peek_qualified_name(&qn, true)) return false;

    const int after_pos =
        core_input_state_.pos + (qn.is_global_qualified ? 1 : 0) +
        2 * static_cast<int>(qn.qualifier_segments.size()) + 1;
    return after_pos < static_cast<int>(core_input_state_.tokens.size()) &&
           (core_input_state_.tokens[after_pos].kind == TokenKind::Less ||
            (core_input_state_.tokens[after_pos].kind == TokenKind::LParen &&
             starts_parenthesized_member_pointer_declarator(*this, after_pos)));
}

int Parser::classify_visible_value_or_type_head(int pos, int* after_pos) {
    if (after_pos) *after_pos = pos;
    if (pos < 0 || pos >= static_cast<int>(core_input_state_.tokens.size())) {
        return 0;
    }

    const TokenKind first_kind = core_input_state_.tokens[pos].kind;
    if (first_kind != TokenKind::Identifier &&
        first_kind != TokenKind::ColonColon) {
        return 0;
    }

    const int saved_pos = core_input_state_.pos;
    core_input_state_.pos = pos;

    QualifiedNameRef qn;
    const bool parsed_qn = peek_qualified_name(&qn,
                                               first_kind == TokenKind::ColonColon);
    core_input_state_.pos = saved_pos;
    if (!parsed_qn || (!qn.is_global_qualified &&
                       qn.qualifier_segments.empty())) {
        if (first_kind != TokenKind::Identifier) return 0;
        if (after_pos) *after_pos = pos + 1;

        const Token& head_tok = core_input_state_.tokens[pos];
        const std::string head_name = std::string(token_spelling(head_tok));
        if (find_visible_var_type(head_tok.text_id)) return 1;

        const QualifiedNameKey direct_known_fn_key =
            known_fn_name_key_in_context(current_namespace_context_id(), head_tok.text_id);
        if (direct_known_fn_key.base_text_id != kInvalidText) {
            if (has_known_fn_name(direct_known_fn_key)) return 1;
        }

        const QualifiedNameKey current_member_key =
            current_record_member_name_key(head_tok.text_id);
        if (current_member_key.base_text_id != kInvalidText) {
            if (has_known_fn_name(current_member_key)) return 1;
        }

        const VisibleNameResult resolved_value =
            resolve_visible_value(head_tok.text_id);
        if (resolved_value) {
            if (resolved_value.key.base_text_id != kInvalidText) {
                if (has_known_fn_name(resolved_value.key)) return 1;
            }
        }

        if (is_known_simple_visible_type_head(*this, head_tok.text_id,
                                              head_name)) {
            return -1;
        }
        if (resolve_visible_type(head_tok.text_id)) return -1;
        return 0;
    }

    int after_name_pos = pos;
    if (qn.is_global_qualified) ++after_name_pos;
    after_name_pos += 1 + 2 * static_cast<int>(qn.qualifier_segments.size());
    if (after_pos) *after_pos = after_name_pos;

    if (resolve_qualified_value(qn)) return 1;
    if (resolve_qualified_type(qn)) return -1;
    // Declaration-side probes keep unresolved qualified heads on the type side
    // unless structured value lookup proves they are expression-like.
    return -1;
}

int Parser::classify_visible_value_or_type_starter(int pos, int* after_pos) {
    int after_name_pos = pos;
    const int head_kind =
        classify_visible_value_or_type_head(pos, &after_name_pos);
    if (after_pos) *after_pos = after_name_pos;
    if (head_kind == 0) return 0;

    const TokenKind first_kind = core_input_state_.tokens[pos].kind;
    const bool has_qualified_head =
        first_kind == TokenKind::ColonColon || after_name_pos != pos + 1;
    if (!has_qualified_head) return head_kind;

    if (after_name_pos >= static_cast<int>(core_input_state_.tokens.size())) {
        return 0;
    }

    const TokenKind tail_kind = core_input_state_.tokens[after_name_pos].kind;
    if (tail_kind != TokenKind::LParen &&
        !(tail_kind == TokenKind::Less &&
          starts_with_value_like_template_expr(
              *this, core_input_state_.tokens, pos))) {
        return 0;
    }

    return head_kind;
}

bool Parser::looks_like_unresolved_identifier_type_head(int pos) const {
    if (!is_cpp_mode()) return false;
    if (pos < 0 || pos >= static_cast<int>(core_input_state_.tokens.size())) return false;
    if (core_input_state_.tokens[pos].kind != TokenKind::Identifier) return false;
    if (is_concept_name(core_input_state_.tokens[pos].text_id)) return false;

    const int next = pos + 1;
    if (next >= static_cast<int>(core_input_state_.tokens.size())) return false;

    switch (core_input_state_.tokens[next].kind) {
        case TokenKind::Identifier:
        case TokenKind::Amp:
        case TokenKind::AmpAmp:
        case TokenKind::Star:
        case TokenKind::Less:
        case TokenKind::Comma:
        case TokenKind::RParen:
        case TokenKind::Ellipsis:
            return true;
        default:
            break;
    }

    return is_qualifier(core_input_state_.tokens[next].kind);
}

bool Parser::looks_like_unresolved_parenthesized_parameter_type_head(int pos) const {
    if (!is_cpp_mode()) return false;
    if (pos < 0 || pos >= static_cast<int>(core_input_state_.tokens.size())) return false;
    if (core_input_state_.tokens[pos].kind != TokenKind::Identifier) return false;

    const Token& head_tok = core_input_state_.tokens[pos];
    const TextId head_text_id = head_tok.text_id;
    const std::string head_name = std::string(token_spelling(head_tok));
    if (is_concept_name(head_text_id)) return false;
    if (is_known_simple_visible_type_head(*this, head_text_id, head_name)) return false;
    if (find_visible_var_type(head_text_id)) return false;

    const int lparen = pos + 1;
    if (lparen >= static_cast<int>(core_input_state_.tokens.size()) ||
        core_input_state_.tokens[lparen].kind != TokenKind::LParen) {
        return false;
    }

    int inner = lparen + 1;
    if (inner >= static_cast<int>(core_input_state_.tokens.size())) return false;

    int grouped_depth = 0;
    while (inner < static_cast<int>(core_input_state_.tokens.size()) &&
           core_input_state_.tokens[inner].kind == TokenKind::LParen) {
        ++grouped_depth;
        ++inner;
    }

    while (inner < static_cast<int>(core_input_state_.tokens.size()) &&
           (core_input_state_.tokens[inner].kind == TokenKind::Star ||
            core_input_state_.tokens[inner].kind == TokenKind::Amp ||
            core_input_state_.tokens[inner].kind == TokenKind::AmpAmp)) {
        ++inner;
    }

    if (inner >= static_cast<int>(core_input_state_.tokens.size()) ||
        core_input_state_.tokens[inner].kind != TokenKind::Identifier) {
        return false;
    }
    ++inner;

    for (int i = 0; i < grouped_depth; ++i) {
        if (inner >= static_cast<int>(core_input_state_.tokens.size()) ||
            core_input_state_.tokens[inner].kind != TokenKind::RParen) {
            return false;
        }
        ++inner;
    }

    return inner < static_cast<int>(core_input_state_.tokens.size()) &&
           core_input_state_.tokens[inner].kind == TokenKind::RParen;
}

// ── skip helpers ─────────────────────────────────────────────────────────────

void Parser::skip_paren_group() {
    expect(TokenKind::LParen);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LParen)) ++depth;
        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_brace_group() {
    expect(TokenKind::LBrace);
    int depth = 1;
    while (!at_end() && depth > 0) {
        if (check(TokenKind::LBrace)) ++depth;
        else if (check(TokenKind::RBrace)) { if (--depth == 0) { consume(); return; } }
        consume();
    }
}

void Parser::skip_attributes() {
    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (check(TokenKind::LParen)) skip_paren_group();
        if (check(TokenKind::LParen)) skip_paren_group();  // double-paren
    }
    // C++11 [[attribute]] syntax
    while (check(TokenKind::LBracket) &&
           core_input_state_.pos + 1 <
               static_cast<int>(core_input_state_.tokens.size()) &&
           core_input_state_.tokens[core_input_state_.pos + 1].kind ==
               TokenKind::LBracket) {
        consume(); consume(); // [[
        int depth = 1;
        while (core_input_state_.pos <
                   static_cast<int>(core_input_state_.tokens.size()) &&
               depth > 0) {
            if (check(TokenKind::LBracket)) ++depth;
            else if (check(TokenKind::RBracket)) --depth;
            if (depth > 0) consume();
        }
        if (check(TokenKind::RBracket)) consume(); // first ]
        if (check(TokenKind::RBracket)) consume(); // second ]
    }
    while (check(TokenKind::KwExtension)) {
        consume();
    }
    while (check(TokenKind::KwNoreturn)) {
        consume();
    }
    while (check(TokenKind::KwNoexcept) ||
           (check(TokenKind::Identifier) && token_spelling(cur()) == "noexcept")) {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

void Parser::skip_exception_spec() {
    // noexcept / noexcept(expr)
    while (check(TokenKind::KwNoexcept) ||
           (check(TokenKind::Identifier) && token_spelling(cur()) == "noexcept")) {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
    // throw() / throw(type-list)
    if (check(TokenKind::Identifier) && token_spelling(cur()) == "throw") {
        consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

void Parser::parse_attributes(TypeSpec* ts) {
    auto apply_aligned_attr = [&](long long align) {
        if (!ts || align <= 0) return;
        if (align > ts->align_bytes) ts->align_bytes = static_cast<int>(align);
    };

    auto apply_vector_size_attr = [&](long long bytes) {
        if (!ts || bytes <= 0) return;
        ts->is_vector = true;
        ts->vector_bytes = bytes;
        // Leave vector_lanes=0; finalize_vector() computes it once the
        // final base type is known.
        ts->vector_lanes = 0;
    };

    while (check(TokenKind::KwAttribute)) {
        consume();  // __attribute__
        if (!check(TokenKind::LParen)) continue;
        consume();  // (
        if (!check(TokenKind::LParen)) {
            skip_paren_group();
            continue;
        }
        consume();  // (

        while (!at_end() && !check(TokenKind::RParen)) {
            if (!check(TokenKind::Identifier)) {
                consume();
                continue;
            }

            const std::string attr_name(token_spelling(cur()));
            consume();

            if (attr_name == "aligned" || attr_name == "__aligned__") {
                long long align = 16;  // GCC bare aligned => target max useful alignment.
                if (check(TokenKind::LParen)) {
                    consume();
                    if (!check(TokenKind::RParen)) {
                        Node* align_expr = parse_assign_expr(*this);
                        long long align_val = 0;
                        if (align_expr &&
                            eval_const_int(
                                align_expr, &align_val,
                                &definition_state_.struct_tag_def_map,
                                &binding_state_.const_int_bindings) &&
                            align_val > 0) {
                            align = align_val;
                        }
                    }
                    expect(TokenKind::RParen);
                }
                apply_aligned_attr(align);
            } else if (attr_name == "packed" || attr_name == "__packed__") {
                if (ts) ts->is_packed = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else if (attr_name == "vector_size" || attr_name == "__vector_size__") {
                if (check(TokenKind::LParen)) {
                    consume();
                    Node* sz_expr = parse_assign_expr(*this);
                    long long sz_val = 0;
                    eval_const_int(sz_expr, &sz_val,
                                   &definition_state_.struct_tag_def_map,
                                   &binding_state_.const_int_bindings);
                    expect(TokenKind::RParen);
                    apply_vector_size_attr(sz_val);
                }
            } else if (attr_name == "noinline" || attr_name == "__noinline__") {
                if (ts) ts->is_noinline = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else if (attr_name == "always_inline" || attr_name == "__always_inline__") {
                if (ts) ts->is_always_inline = true;
                if (check(TokenKind::LParen)) skip_paren_group();
            } else {
                if (check(TokenKind::LParen)) skip_paren_group();
            }

            if (check(TokenKind::Comma)) consume();
        }

        expect(TokenKind::RParen);
        expect(TokenKind::RParen);
    }

    while (check(TokenKind::KwExtension)) consume();
    while (check(TokenKind::KwNoreturn)) consume();

    // If vector_size was set and the base type is already known, compute lanes.
    if (ts) finalize_vector_type(*ts);
}

void Parser::skip_asm() {
    if (check(TokenKind::KwAsm)) {
        consume();
        // Skip optional qualifiers: volatile, __volatile__, goto, inline
        while (check(TokenKind::KwVolatile) || check(TokenKind::KwInline) ||
               check(TokenKind::KwGoto))
            consume();
        if (check(TokenKind::LParen)) skip_paren_group();
    }
}

// ── type parsing ──────────────────────────────────────────────────────────────

TypeSpec Parser::parse_base_type() {
    definition_state_.last_enum_def = nullptr;
    TypeSpec ts{};
    ts.array_size = -1;
    ts.array_rank = 0;
    ts.inner_rank = -1;
    for (int i = 0; i < 8; ++i) ts.array_dims[i] = -1;
    ts.align_bytes = 0;
    ts.vector_lanes = 0;
    ts.vector_bytes = 0;
    ts.base = TB_INT;  // default

    auto set_template_arg_debug_refs =
        [&](TypeSpec* target, const std::vector<std::string>& refs) {
            if (!target) return;
            target->tpl_struct_args.data = nullptr;
            target->tpl_struct_args.size = 0;
            if (refs.empty()) return;
            target->tpl_struct_args.data =
                arena_.alloc_array<TemplateArgRef>(refs.size());
            target->tpl_struct_args.size = static_cast<int>(refs.size());
            for (int i = 0; i < target->tpl_struct_args.size; ++i) {
                target->tpl_struct_args.data[i].kind = TemplateArgKind::Type;
                target->tpl_struct_args.data[i].type = {};
                target->tpl_struct_args.data[i].value = 0;
                target->tpl_struct_args.data[i].nttp_text_id = kInvalidText;
                target->tpl_struct_args.data[i].debug_text =
                    arena_.strdup(refs[i].c_str());
            }
        };
    auto zero_value_arg_ref_uses_debug_fallback =
        [](const TemplateArgRef& arg) -> bool {
            if (arg.kind != TemplateArgKind::Value || arg.value != 0 ||
                !arg.debug_text || !arg.debug_text[0]) {
                return false;
            }
            if (arg.type.array_size_expr) return false;
            char* end = nullptr;
            (void)std::strtoll(arg.debug_text, &end, 10);
            return !(end && *end == '\0');
        };
    auto unstructured_type_arg_ref_uses_debug_fallback =
        [](const TemplateArgRef& arg) -> bool {
            if (arg.kind != TemplateArgKind::Type || !arg.debug_text ||
                !arg.debug_text[0]) {
                return false;
            }
            return !arg.type.tag && !arg.type.record_def &&
                   !arg.type.tpl_struct_origin &&
                   !arg.type.deferred_member_type_name &&
                   arg.type.base == TB_VOID;
        };
    auto render_template_arg_ref = [&](const ParsedTemplateArg& arg)
        -> std::string {
        if (arg.is_value) {
            if (arg.nttp_name && arg.nttp_name[0]) return arg.nttp_name;
            return std::to_string(arg.value);
        }
        if (arg.type.tpl_struct_origin) {
            std::string ref = "@";
            ref += arg.type.tpl_struct_origin;
            ref += ":";
            if (arg.type.tpl_struct_args.data && arg.type.tpl_struct_args.size > 0) {
                for (int i = 0; i < arg.type.tpl_struct_args.size; ++i) {
                    if (i > 0) ref += ",";
                    const TemplateArgRef& nested = arg.type.tpl_struct_args.data[i];
                    if (nested.kind == TemplateArgKind::Value) {
                        if (nested.type.array_size_expr && nested.debug_text &&
                            nested.debug_text[0]) {
                            ref += nested.debug_text;
                        } else if (zero_value_arg_ref_uses_debug_fallback(nested)) {
                            ref += nested.debug_text;
                        } else {
                            ref += std::to_string(nested.value);
                        }
                    } else if (nested.kind == TemplateArgKind::Type) {
                        std::string nested_mangled;
                        append_type_mangled_suffix(nested_mangled, nested.type);
                        if (unstructured_type_arg_ref_uses_debug_fallback(nested)) {
                            ref += nested.debug_text;
                        } else if (nested_mangled.empty() && nested.type.tag) {
                            ref += nested.type.tag;
                        } else if (!nested_mangled.empty()) {
                            ref += nested_mangled;
                        } else if (nested.debug_text && nested.debug_text[0]) {
                            ref += nested.debug_text;
                        }
                    } else if (nested.debug_text && nested.debug_text[0]) {
                        ref += nested.debug_text;
                    }
                }
            }
            if (arg.type.deferred_member_type_name &&
                arg.type.deferred_member_type_name[0]) {
                ref += "$";
                ref += arg.type.deferred_member_type_name;
            }
            return ref;
        }
        std::string mangled;
        append_type_mangled_suffix(mangled, arg.type);
        if (mangled.empty() && arg.type.tag) return arg.type.tag;
        return mangled;
    };
    auto set_template_arg_refs_from_parsed_args =
        [&](TypeSpec* target, const std::vector<ParsedTemplateArg>& args) {
            if (!target) return;
            target->tpl_struct_args.data = nullptr;
            target->tpl_struct_args.size = 0;
            if (args.empty()) return;
            target->tpl_struct_args.data =
                arena_.alloc_array<TemplateArgRef>(args.size());
            target->tpl_struct_args.size = static_cast<int>(args.size());
            for (int i = 0; i < target->tpl_struct_args.size; ++i) {
                const ParsedTemplateArg& arg = args[i];
                TemplateArgRef& out = target->tpl_struct_args.data[i];
                out.kind = arg.is_value ? TemplateArgKind::Value
                                        : TemplateArgKind::Type;
                out.type = arg.is_value ? TypeSpec{} : arg.type;
                if (arg.is_value) {
                    out.type.array_size = -1;
                    out.type.inner_rank = -1;
                    out.type.array_size_expr = arg.expr;
                }
                out.value = arg.is_value ? arg.value : 0;
                out.nttp_text_id =
                    arg.is_value ? arg.nttp_text_id : kInvalidText;
                const std::string debug_ref = render_template_arg_ref(arg);
                out.debug_text = debug_ref.empty()
                    ? nullptr
                    : arena_.strdup(debug_ref.c_str());
            }
        };
    auto parsed_args_have_simple_structured_carrier =
        [](const std::vector<ParsedTemplateArg>& args) -> bool {
        if (args.empty()) return false;
        for (const ParsedTemplateArg& arg : args) {
            if (arg.is_value) return true;
            if (arg.type.tpl_struct_origin ||
                (arg.type.tpl_struct_args.data &&
                 arg.type.tpl_struct_args.size > 0)) {
                return false;
            }
        }
        return true;
    };
    auto structured_nttp_expr_carrier =
        [](const ParsedTemplateArg& arg) -> Node* {
        if (arg.expr) return arg.expr;
        return arg.type.array_size_expr;
    };
    auto parsed_nttp_arg_has_structured_carrier =
        [&](const ParsedTemplateArg& arg) -> bool {
        return structured_nttp_expr_carrier(arg) ||
               arg.nttp_text_id != kInvalidText;
    };
    auto set_template_arg_debug_refs_text =
        [&](TypeSpec* target, const std::string& refs_text) {
            if (!target) return;
            if (refs_text.empty()) {
                set_template_arg_debug_refs(target, std::vector<std::string>{});
                return;
            }
            set_template_arg_debug_refs(target,
                                        split_template_arg_ref_text(refs_text));
        };
    auto template_arg_refs_text = [&](const TypeSpec& spec) -> std::string {
        std::string refs;
        if (!spec.tpl_struct_args.data || spec.tpl_struct_args.size <= 0) return refs;
        for (int i = 0; i < spec.tpl_struct_args.size; ++i) {
            if (i > 0) refs += ",";
            const TemplateArgRef& arg = spec.tpl_struct_args.data[i];
            if (arg.kind == TemplateArgKind::Value) {
                if (arg.type.array_size_expr && arg.debug_text &&
                    arg.debug_text[0]) {
                    refs += arg.debug_text;
                } else if (zero_value_arg_ref_uses_debug_fallback(arg)) {
                    refs += arg.debug_text;
                } else {
                    refs += std::to_string(arg.value);
                }
            } else if (arg.kind == TemplateArgKind::Type) {
                std::string mangled;
                append_type_mangled_suffix(mangled, arg.type);
                if (unstructured_type_arg_ref_uses_debug_fallback(arg))
                    refs += arg.debug_text;
                else if (mangled.empty() && arg.type.tag) refs += arg.type.tag;
                else if (!mangled.empty()) refs += mangled;
                else if (arg.debug_text && arg.debug_text[0]) refs += arg.debug_text;
            } else if (arg.debug_text && arg.debug_text[0]) {
                refs += arg.debug_text;
            }
        }
        return refs;
    };
    auto deferred_member_lookup_name = [&](const TypeSpec& spec) -> std::string {
        if (spec.deferred_member_type_text_id != kInvalidText) {
            std::string name(parser_text(spec.deferred_member_type_text_id, {}));
            if (!name.empty()) return name;
        }
        return spec.deferred_member_type_name ? spec.deferred_member_type_name : "";
    };

    std::function<bool(const TypeSpec&, const std::string&, TextId, TypeSpec*)>
        lookup_struct_member_typedef_recursive_for_type;
    lookup_struct_member_typedef_recursive_for_type =
            [&](const TypeSpec& owner_ts, const std::string& member,
                TextId member_text_id, TypeSpec* out) -> bool {
                if (member.empty() || !out) return false;
                auto type_spec_base_text_id =
                    [&](const TypeSpec& ts) -> TextId {
                        if (ts.tag_text_id != kInvalidText) return ts.tag_text_id;
                        if (!ts.tag || !ts.tag[0]) return kInvalidText;
                        const std::string_view tag_text(ts.tag);
                        if (tag_text.find("::") != std::string_view::npos)
                            return kInvalidText;
                        return find_parser_text_id(tag_text);
                    };
                auto type_spec_qualified_name_ref =
                    [&](const TypeSpec& ts, QualifiedNameRef* out) -> bool {
                        if (!out) return false;
                        const TextId base_text_id = type_spec_base_text_id(ts);
                        if (base_text_id == kInvalidText) return false;

                        QualifiedNameRef qn;
                        qn.base_text_id = base_text_id;
                        qn.is_global_qualified = ts.is_global_qualified;
                        if (ts.n_qualifier_segments > 0) {
                            qn.qualifier_segments.reserve(ts.n_qualifier_segments);
                            qn.qualifier_text_ids.reserve(ts.n_qualifier_segments);
                            for (int i = 0; i < ts.n_qualifier_segments; ++i) {
                                const char* segment =
                                    ts.qualifier_segments ? ts.qualifier_segments[i]
                                                          : nullptr;
                                qn.qualifier_segments.push_back(
                                    segment ? std::string(segment) : std::string{});
                                TextId segment_text_id =
                                    ts.qualifier_text_ids ? ts.qualifier_text_ids[i]
                                                          : kInvalidText;
                                if (segment_text_id == kInvalidText && segment &&
                                    segment[0]) {
                                    segment_text_id = find_parser_text_id(segment);
                                }
                                qn.qualifier_text_ids.push_back(segment_text_id);
                            }
                        }
                        *out = std::move(qn);
                        return true;
                    };
                auto lookup_typedef_for_type =
                    [&](const TypeSpec& ts) -> const TypeSpec* {
                        const TextId base_text_id = type_spec_base_text_id(ts);
                        if (base_text_id == kInvalidText) return nullptr;
                        if (ts.n_qualifier_segments > 0 || ts.is_global_qualified) {
                            QualifiedNameRef qn;
                            if (type_spec_qualified_name_ref(ts, &qn)) {
                                if (const TypeSpec* structured =
                                        find_typedef_type(qualified_name_key(qn))) {
                                    return structured;
                                }
                            }
                        }
                        if (ts.namespace_context_id >= 0) {
                            if (const TypeSpec* structured = find_typedef_type(
                                    struct_typedef_key_in_context(
                                        ts.namespace_context_id, base_text_id))) {
                                return structured;
                            }
                        }
                        return find_visible_typedef_type(base_text_id);
                    };
                auto resolve_struct_like = [&](TypeSpec ts) -> TypeSpec {
                    ts = resolve_typedef_type_chain(ts);
                    if (ts.base == TB_TYPEDEF && ts.tag) {
                        if (const TypeSpec* nested = lookup_typedef_for_type(ts)) {
                            ts = *nested;
                        }
                    }
                    return ts;
                };
                std::function<bool(const Node*, TextId, TypeSpec*)>
                    resolve_record_member_typedef_sidecar;
                auto try_node_member_typedefs = [&](const Node* sdef) -> bool {
                    if (!sdef || sdef->n_member_typedefs <= 0) return false;
                    for (int i = 0; i < sdef->n_member_typedefs; ++i) {
                        const char* name = sdef->member_typedef_names[i];
                        const bool name_matches =
                            member_text_id != kInvalidText
                                ? find_parser_text_id(name ? name : "") == member_text_id
                                : (name && member == name);
                        if (name_matches) {
                            TextId alias_text_id = member_text_id;
                            if (alias_text_id == kInvalidText && name)
                                alias_text_id = find_parser_text_id(name);
                            if (resolve_record_member_typedef_sidecar &&
                                alias_text_id != kInvalidText &&
                                resolve_record_member_typedef_sidecar(
                                    sdef, alias_text_id, out)) {
                                return true;
                            }
                            *out = sdef->member_typedef_types[i];
                            return true;
                        }
                    }
                    return false;
                };
                auto structured_record_for_type = [&](const TypeSpec& ts) -> const Node* {
                    QualifiedNameRef qn;
                    if (type_spec_qualified_name_ref(ts, &qn) &&
                        (ts.n_qualifier_segments > 0 || ts.is_global_qualified)) {
                        return qualified_type_record_definition_from_structured_authority(
                            *this, qn);
                    }
                    if (!ts.tag || !ts.tag[0]) return nullptr;
                    return record_definition_in_context_by_text_id(
                        *this, current_namespace_context_id(),
                        type_spec_base_text_id(ts));
                };
                auto apply_template_bindings =
                    [&](TypeSpec member_ts,
                        const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
                        const std::vector<std::pair<std::string, long long>>& nttp_bindings,
                        bool* substituted_type = nullptr) {
                        if (substituted_type) *substituted_type = false;
                        for (const auto& [pname, pts] : type_bindings) {
                            if (member_ts.base == TB_TYPEDEF && member_ts.tag &&
                                std::string(member_ts.tag) == pname) {
                                const bool outer_lref = member_ts.is_lvalue_ref;
                                const bool outer_rref = member_ts.is_rvalue_ref;
                                const bool outer_const = member_ts.is_const;
                                const bool outer_volatile = member_ts.is_volatile;
                                member_ts = pts;
                                member_ts.is_lvalue_ref =
                                    member_ts.is_lvalue_ref || outer_lref;
                                member_ts.is_rvalue_ref =
                                    !member_ts.is_lvalue_ref &&
                                    (member_ts.is_rvalue_ref || outer_rref);
                                member_ts.is_const = member_ts.is_const || outer_const;
                                member_ts.is_volatile =
                                    member_ts.is_volatile || outer_volatile;
                                if (substituted_type) *substituted_type = true;
                                break;
                            }
                        }
                        if (member_ts.array_size_expr &&
                            member_ts.array_size_expr->kind == NK_VAR &&
                            member_ts.array_size_expr->name) {
                            for (const auto& [pname, value] : nttp_bindings) {
                                if (std::string(member_ts.array_size_expr->name) == pname) {
                                    if (member_ts.array_rank > 0) {
                                        member_ts.array_dims[0] = value;
                                        member_ts.array_size = value;
                                    }
                                    member_ts.array_size_expr = nullptr;
                                    break;
                                }
                            }
                        }
                        return member_ts;
                    };
                auto record_member_key_for_node =
                    [&](const Node* owner,
                        TextId alias_text_id) -> QualifiedNameKey {
                        if (!owner || alias_text_id == kInvalidText) return {};
                        const int context_id =
                            owner->namespace_context_id >= 0
                                ? owner->namespace_context_id
                                : current_namespace_context_id();
                        TextId record_text_id = kInvalidText;
                        if (owner->template_origin_name &&
                            owner->template_origin_name[0]) {
                            record_text_id = parser_text_id_for_token(
                                kInvalidText, owner->template_origin_name);
                        }
                        if (record_text_id == kInvalidText)
                            record_text_id = owner->unqualified_text_id;
                        if (record_text_id == kInvalidText &&
                            owner->unqualified_name) {
                            record_text_id = parser_text_id_for_token(
                                kInvalidText, owner->unqualified_name);
                        }
                        if (record_text_id == kInvalidText && owner->name) {
                            record_text_id = parser_text_id_for_token(
                                kInvalidText, owner->name);
                        }
                        if (record_text_id == kInvalidText) return {};
                        return record_member_typedef_key_in_context(
                            context_id, record_text_id, alias_text_id);
                    };
                auto template_primary_for_record =
                    [&](const Node* owner) -> const Node* {
                        if (!owner) return nullptr;
                        if (owner->template_origin_name &&
                            owner->template_origin_name[0]) {
                            return find_template_struct_primary(
                                owner->namespace_context_id >= 0
                                    ? owner->namespace_context_id
                                    : current_namespace_context_id(),
                                parser_text_id_for_token(
                                    kInvalidText, owner->template_origin_name));
                        }
                        if (is_primary_template_struct_def(owner)) return owner;
                        return nullptr;
                    };
                auto record_actual_args =
                    [&](const Node* owner,
                        std::vector<ParsedTemplateArg>* out_args) -> bool {
                        if (!owner || !out_args || owner->n_template_args <= 0)
                            return false;
                        out_args->clear();
                        out_args->reserve(owner->n_template_args);
                        for (int i = 0; i < owner->n_template_args; ++i) {
                            ParsedTemplateArg arg;
                            arg.is_value = owner->template_arg_is_value &&
                                           owner->template_arg_is_value[i];
                            if (arg.is_value) {
                                arg.value = owner->template_arg_values
                                                ? owner->template_arg_values[i]
                                                : 0;
                                arg.nttp_name =
                                    owner->template_arg_nttp_names
                                        ? owner->template_arg_nttp_names[i]
                                        : nullptr;
                                arg.nttp_text_id =
                                    owner->template_arg_nttp_text_ids
                                        ? owner->template_arg_nttp_text_ids[i]
                                        : kInvalidText;
                            } else if (owner->template_arg_types) {
                                arg.type = owner->template_arg_types[i];
                            }
                            out_args->push_back(arg);
                        }
                        return !out_args->empty();
                    };
                auto substitute_record_carrier_arg =
                    [&](const Node* record_primary,
                        const std::vector<ParsedTemplateArg>& actual_args,
                        const ParsedTemplateArg& carrier_arg,
                        ParsedTemplateArg* out_arg) -> bool {
                        if (!out_arg) return false;
                        *out_arg = carrier_arg;
                        if (!record_primary ||
                            record_primary->n_template_params <= 0 ||
                            actual_args.empty()) {
                            return true;
                        }
                        auto param_index = [&](TextId text_id) -> int {
                            if (text_id == kInvalidText) return -1;
                            for (int pi = 0;
                                 pi < record_primary->n_template_params;
                                 ++pi) {
                                const char* pname =
                                    record_primary->template_param_names
                                        ? record_primary->template_param_names[pi]
                                        : nullptr;
                                if (!pname) continue;
                                if (parser_text_id_for_token(kInvalidText,
                                                             pname) == text_id) {
                                    return pi;
                                }
                            }
                            return -1;
                        };
                        if (carrier_arg.is_value) {
                            if (!(carrier_arg.nttp_name &&
                                  carrier_arg.nttp_name[0])) {
                                return true;
                            }
                            const int pi = param_index(parser_text_id_for_token(
                                kInvalidText, carrier_arg.nttp_name));
                            if (pi < 0 ||
                                pi >= static_cast<int>(actual_args.size())) {
                                return true;
                            }
                            if (!actual_args[pi].is_value) return false;
                            *out_arg = actual_args[pi];
                            return true;
                        }
                        const TypeSpec& carrier_type = carrier_arg.type;
                        if (!(carrier_type.base == TB_TYPEDEF &&
                              carrier_type.tag &&
                              carrier_type.tag[0])) {
                            return true;
                        }
                        TextId type_text_id = carrier_type.tag_text_id;
                        if (type_text_id == kInvalidText) {
                            type_text_id = parser_text_id_for_token(
                                kInvalidText, carrier_type.tag);
                        }
                        const int pi = param_index(type_text_id);
                        if (pi < 0 ||
                            pi >= static_cast<int>(actual_args.size())) {
                            return true;
                        }
                        if (actual_args[pi].is_value) return false;
                        *out_arg = actual_args[pi];
                        return true;
                    };
                resolve_record_member_typedef_sidecar =
                    [&](const Node* record_owner,
                        TextId alias_text_id,
                        TypeSpec* resolved_out) -> bool {
                        if (!record_owner || alias_text_id == kInvalidText ||
                            !resolved_out) {
                            return false;
                        }
                        const QualifiedNameKey alias_key =
                            record_member_key_for_node(record_owner,
                                                       alias_text_id);
                        const ParserAliasTemplateMemberTypedefInfo* info =
                            find_record_member_typedef_info(alias_key);
                        if (!info || !info->valid ||
                            info->owner_key.base_text_id == kInvalidText ||
                            info->member_text_id == kInvalidText) {
                            return false;
                        }
                        const Node* rhs_primary =
                            find_template_struct_primary(info->owner_key);
                        if (!rhs_primary) return false;

                        std::vector<ParsedTemplateArg> record_args;
                        (void)record_actual_args(record_owner, &record_args);
                        const Node* record_primary =
                            template_primary_for_record(record_owner);
                        std::vector<ParsedTemplateArg> actual_rhs_args;
                        actual_rhs_args.reserve(info->owner_args.size());
                        for (const ParsedTemplateArg& carrier_arg :
                             info->owner_args) {
                            ParsedTemplateArg actual_arg;
                            if (!substitute_record_carrier_arg(
                                    record_primary, record_args, carrier_arg,
                                    &actual_arg)) {
                                return false;
                            }
                            actual_rhs_args.push_back(actual_arg);
                        }

                        const ParserTemplateState::TemplateInstantiationKey
                            concrete_owner{
                                info->owner_key,
                                make_template_instantiation_argument_keys(
                                    actual_rhs_args)};
                        if (const TypeSpec* structured_member =
                                find_template_instantiation_member_typedef_type(
                                    concrete_owner, info->member_text_id)) {
                            *resolved_out = *structured_member;
                            return true;
                        }

                        std::vector<std::pair<std::string, TypeSpec>>
                            type_bindings;
                        std::vector<std::pair<std::string, long long>>
                            nttp_bindings;
                        const std::vector<Node*>* specializations =
                            find_template_struct_specializations(
                                info->owner_key);
                        const Node* selected =
                            select_template_struct_pattern_for_args(
                                actual_rhs_args, rhs_primary, specializations,
                                &type_bindings, &nttp_bindings);
                        if (!selected || selected->n_member_typedefs <= 0)
                            return false;
                        for (int mi = 0; mi < selected->n_member_typedefs;
                             ++mi) {
                            const char* member =
                                selected->member_typedef_names[mi];
                            if (!member || !member[0]) continue;
                            const TextId selected_member_text_id =
                                parser_text_id_for_token(kInvalidText, member);
                            if (selected_member_text_id !=
                                info->member_text_id) {
                                continue;
                            }
                            bool substituted = false;
                            *resolved_out = apply_template_bindings(
                                selected->member_typedef_types[mi],
                                type_bindings, nttp_bindings, &substituted);
                            (void)substituted;
                            return true;
                        }
                        return false;
                    };
                auto try_selected_specialization_member_typedefs =
                    [&](const Node* owner) -> bool {
                        if (!owner || owner->n_template_args <= 0) return false;
                        const Node* primary_tpl = nullptr;
                        if (owner->template_origin_name && owner->template_origin_name[0]) {
                            primary_tpl = find_template_struct_primary(
                                current_namespace_context_id(),
                                parser_text_id_for_token(
                                    kInvalidText,
                                    owner->template_origin_name));
                        } else if (is_primary_template_struct_def(owner)) {
                            primary_tpl = owner;
                        } else if (owner->name && owner->name[0]) {
                            primary_tpl = find_template_struct_primary(
                                current_namespace_context_id(),
                                parser_text_id_for_token(kInvalidText,
                                                         owner->name));
                        }
                        if (!primary_tpl) return false;

                        std::vector<ParsedTemplateArg> actual_args;
                        actual_args.reserve(owner->n_template_args);
                        for (int i = 0; i < owner->n_template_args; ++i) {
                            ParsedTemplateArg arg;
                            arg.is_value =
                                owner->template_arg_is_value &&
                                owner->template_arg_is_value[i];
                            if (arg.is_value) {
                                arg.value = owner->template_arg_values[i];
                            } else if (owner->template_arg_types) {
                                arg.type = owner->template_arg_types[i];
                            }
                            actual_args.push_back(arg);
                        }

                        std::vector<std::pair<std::string, TypeSpec>> type_bindings;
                        std::vector<std::pair<std::string, long long>> nttp_bindings;
                        const auto* specializations =
                            find_template_struct_specializations(primary_tpl);
                        const Node* selected = select_template_struct_pattern_for_args(
                            actual_args, primary_tpl, specializations,
                            &type_bindings, &nttp_bindings);
                        if (selected && selected->template_arg_types &&
                            selected->template_arg_is_value) {
                            for (int ai = 0;
                                 ai < selected->n_template_args &&
                                 ai < static_cast<int>(actual_args.size());
                                 ++ai) {
                                if (selected->template_arg_is_value[ai] ||
                                    actual_args[ai].is_value) {
                                    continue;
                                }
                                const TypeSpec pattern_arg =
                                    selected->template_arg_types[ai];
                                if (!(pattern_arg.base == TB_TYPEDEF &&
                                      pattern_arg.tag &&
                                      pattern_arg.tag[0])) {
                                    continue;
                                }
                                bool is_selected_param = false;
                                for (int pi = 0; pi < selected->n_template_params; ++pi) {
                                    const char* param_name =
                                        selected->template_param_names
                                            ? selected->template_param_names[pi]
                                            : nullptr;
                                    if (param_name &&
                                        std::string(pattern_arg.tag) == param_name) {
                                        is_selected_param = true;
                                        break;
                                    }
                                }
                                if (!is_selected_param) continue;
                                bool already_bound = false;
                                for (const auto& [bound_name, _] : type_bindings) {
                                    if (bound_name == pattern_arg.tag) {
                                        already_bound = true;
                                        break;
                                    }
                                }
                                if (!already_bound) {
                                    type_bindings.push_back(
                                        {pattern_arg.tag, actual_args[ai].type});
                                }
                            }
                        }
                        if (!selected || selected->n_member_typedefs <= 0) return false;
                        for (int i = 0; i < selected->n_member_typedefs; ++i) {
                            const char* name = selected->member_typedef_names[i];
                            const bool name_matches =
                                member_text_id != kInvalidText
                                    ? find_parser_text_id(name ? name : "") ==
                                          member_text_id
                                    : (name && member == name);
                            if (!name_matches) continue;
                            TypeSpec selected_member_ts =
                                selected->member_typedef_types[i];
                            bool substituted_type = false;
                            *out = apply_template_bindings(
                                selected_member_ts,
                                type_bindings, nttp_bindings, &substituted_type);
                            const bool is_type_member =
                                member_text_id != kInvalidText
                                    ? find_parser_text_id("type") == member_text_id
                                    : member == "type";
                            if (!substituted_type && is_type_member &&
                                type_bindings.size() == 1) {
                                *out = type_bindings.front().second;
                            }
                            return true;
                        }
                        return false;
                };
                auto try_template_instantiation_member_typedef =
                    [&](const Node* owner) -> bool {
                        if (!owner || !owner->template_origin_name ||
                            !owner->template_origin_name[0] ||
                            owner->n_template_args <= 0) {
                            return false;
                        }
                        if (member_text_id == kInvalidText) return false;
                        const Node* primary_tpl = find_template_struct_primary(
                            owner->namespace_context_id >= 0
                                ? owner->namespace_context_id
                                : current_namespace_context_id(),
                            parser_text_id_for_token(
                                kInvalidText, owner->template_origin_name));
                        if (!primary_tpl) return false;
                        std::vector<ParsedTemplateArg> actual_args;
                        actual_args.reserve(owner->n_template_args);
                        for (int i = 0; i < owner->n_template_args; ++i) {
                            ParsedTemplateArg arg;
                            arg.is_value = owner->template_arg_is_value &&
                                           owner->template_arg_is_value[i];
                            if (arg.is_value) {
                                arg.value = owner->template_arg_values
                                                ? owner->template_arg_values[i]
                                                : 0;
                                arg.nttp_name =
                                    owner->template_arg_nttp_names
                                        ? owner->template_arg_nttp_names[i]
                                        : nullptr;
                                arg.nttp_text_id =
                                    owner->template_arg_nttp_text_ids
                                        ? owner->template_arg_nttp_text_ids[i]
                                        : kInvalidText;
                            } else if (owner->template_arg_types) {
                                arg.type = owner->template_arg_types[i];
                            }
                            actual_args.push_back(arg);
                        }
                        const ParserTemplateState::TemplateInstantiationKey
                            concrete_owner{
                                template_instantiation_name_key_for_direct_emit(
                                    *this, primary_tpl,
                                    owner->template_origin_name),
                                make_template_instantiation_argument_keys(
                                    actual_args)};
                        if (const TypeSpec* structured_member =
                                find_template_instantiation_member_typedef_type(
                                    concrete_owner, member_text_id)) {
                            *out = *structured_member;
                            return true;
                        }
                        return false;
                    };
                TypeSpec resolved = resolve_struct_like(owner_ts);
                if (!resolved.record_def) {
                    if (const TypeSpec* typedef_type =
                            lookup_typedef_for_type(owner_ts)) {
                        resolved = resolve_struct_like(*typedef_type);
                    }
                }
                const Node* sdef = resolve_record_type_spec(resolved, nullptr);
                if (!sdef) sdef = structured_record_for_type(resolved);
                if (!sdef) {
                    if (const TypeSpec* typedef_type =
                            lookup_typedef_for_type(owner_ts)) {
                        resolved = resolve_struct_like(*typedef_type);
                        sdef = resolve_record_type_spec(resolved, nullptr);
                        if (!sdef) sdef = structured_record_for_type(resolved);
                    }
                }
                if (!sdef) return false;
                if (try_template_instantiation_member_typedef(sdef))
                    return true;
                if (try_selected_specialization_member_typedefs(sdef))
                    return true;
                if (try_node_member_typedefs(sdef))
                    return true;
                for (int bi = 0; bi < sdef->n_bases; ++bi) {
                    TypeSpec base_ts = resolve_struct_like(sdef->base_types[bi]);
                    if (lookup_struct_member_typedef_recursive_for_type(
                            base_ts, member, member_text_id, out))
                        return true;
                }
                return false;
            };
    bool has_signed   = false;
    bool has_unsigned = false;
    bool has_short    = false;
    int  long_count   = 0;
    bool has_int_kw   = false;
    bool has_char     = false;
    bool has_void     = false;
    bool has_float    = false;
    bool has_double   = false;
    bool has_bool     = false;
    bool has_complex  = false;
    bool has_struct   = false;
    bool has_union    = false;
    bool has_enum     = false;
    bool has_typedef  = false;
    bool done         = false;
    bool base_set     = false;  // true when ts.base was set directly (KwBuiltin, KwInt128, etc.)
    auto parse_builtin_transform_type = [&](TypeSpec* out) -> bool {
        if (!out || !check(TokenKind::Identifier)) return false;
        if (token_spelling(cur()) != "__underlying_type") return false;

        consume();
        expect(TokenKind::LParen);
        // Keep transform-type builtins parseable in declaration contexts even
        // when semantic lowering still treats them conservatively.
        *out = parse_type_name();
        expect(TokenKind::RParen);
        return true;
    };
    auto infer_typeof_like_operand_type = [&](TypeSpec* out, bool strip_qual) -> bool {
        if (!out) return false;
        if (check(TokenKind::LParen)) {
            const int saved_pos = pos_;
            consume();
            if (is_type_start()) {
                bool save_c = out->is_const, save_v = out->is_volatile;
                *out = parse_type_name();
                if (check(TokenKind::RParen)) {
                    consume();
                    if (strip_qual) {
                        out->is_const = false;
                        out->is_volatile = false;
                    }
                    out->is_const |= save_c;
                    out->is_volatile |= save_v;
                    return true;
                }
            }
            pos_ = saved_pos;
        }
        if (check(TokenKind::KwNullptr) ||
            (check(TokenKind::Identifier) && token_spelling(cur()) == "nullptr")) {
            out->base = TB_VOID;
            out->ptr_level = 1;
            consume();
            return true;
        }
        if (is_type_start()) {
            bool save_c = out->is_const, save_v = out->is_volatile;
            *out = parse_type_name();
            if (strip_qual) {
                out->is_const = false;
                out->is_volatile = false;
            }
            out->is_const |= save_c;
            out->is_volatile |= save_v;
            return true;
        }
        if (check(TokenKind::Identifier)) {
            const TextId id_text_id = cur().text_id;
            std::string id(token_spelling(cur()));
            consume();
            if (const TypeSpec* var_type =
                    find_visible_var_type(id_text_id)) {
                *out = *var_type;
            } else {
                out->base = TB_INT;  // enum constants and unknowns → int
            }
            if (strip_qual) {
                out->is_const = false;
                out->is_volatile = false;
            }
            return true;
        }
        if (check(TokenKind::FloatLit)) {
            const std::string lex(token_spelling(cur()));
            const bool is_f16 = lex.find("f16") != std::string::npos ||
                                lex.find("F16") != std::string::npos;
            const bool is_f64 = lex.find("f64") != std::string::npos ||
                                lex.find("F64") != std::string::npos;
            const bool is_f128 = lex.find("f128") != std::string::npos ||
                                 lex.find("F128") != std::string::npos;
            const bool is_bf16 = lex.find("bf16") != std::string::npos ||
                                 lex.find("BF16") != std::string::npos;
            const bool has_fixed_width_suffix = is_f16 || is_f64 || is_f128 || is_bf16;
            const bool is_f32 = lex.find("f32") != std::string::npos ||
                                lex.find("F32") != std::string::npos ||
                                (!has_fixed_width_suffix &&
                                 (lex.find('f') != std::string::npos ||
                                  lex.find('F') != std::string::npos));
            const bool is_ldbl = lex.find('l') != std::string::npos ||
                                 lex.find('L') != std::string::npos;
            out->base = (is_f16 || is_f32 || is_bf16) ? TB_FLOAT
                      : (is_f128 || is_ldbl) ? TB_LONGDOUBLE
                      : is_f64 ? TB_DOUBLE
                               : TB_DOUBLE;
            consume();
            return true;
        }
        if (check(TokenKind::IntLit) || check(TokenKind::CharLit) ||
            (is_cpp_mode() && (check(TokenKind::KwTrue) || check(TokenKind::KwFalse)))) {
            out->base = check(TokenKind::IntLit) || check(TokenKind::CharLit) ? TB_INT : TB_BOOL;
            consume();
            return true;
        }
        if (check(TokenKind::StrLit)) {
            out->base = TB_CHAR;
            out->ptr_level = 1;
            consume();
            return true;
        }
        return false;
    };
    while (!done && !at_end()) {
        TokenKind k = cur().kind;
        switch (k) {
            // Qualifiers / storage class — skip but record const
            case TokenKind::KwConst:    ts.is_const    = true; consume(); break;
            case TokenKind::KwVolatile: ts.is_volatile = true; consume(); break;
            case TokenKind::KwRestrict:
            case TokenKind::KwAtomic:
            case TokenKind::KwStatic:
            case TokenKind::KwExtern:
            case TokenKind::KwRegister:
            case TokenKind::KwInline:
            case TokenKind::KwExtension:
            case TokenKind::KwNoreturn:
            case TokenKind::KwThreadLocal:
            case TokenKind::KwMutable:
            case TokenKind::KwConstexpr:
            case TokenKind::KwConsteval:
            case TokenKind::KwExplicit:
                consume(); break;
            case TokenKind::KwAuto:
                if (is_cpp_mode() && !base_set) {
                    ts.base = TB_AUTO;
                    base_set = true;
                    consume(); done = true; break;
                }
                consume(); break;  // ignore auto storage (C mode)

            // Signed / unsigned modifiers
            case TokenKind::KwSigned:   has_signed   = true; consume(); break;
            case TokenKind::KwUnsigned: has_unsigned = true; consume(); break;

            // Base type keywords
            case TokenKind::KwVoid:   has_void   = true; consume(); break;
            case TokenKind::KwChar:   has_char   = true; consume(); break;
            case TokenKind::KwShort:  has_short  = true; consume(); break;
            case TokenKind::KwInt:    has_int_kw = true; consume(); break;
            case TokenKind::KwLong:   long_count++;      consume(); break;
            case TokenKind::KwFloat:  has_float  = true; consume(); break;
            case TokenKind::KwDouble: has_double = true; consume(); break;
            case TokenKind::KwBool:   has_bool   = true; consume(); break;
            case TokenKind::KwComplex: has_complex = true; consume(); break;

            case TokenKind::KwInt128:  ts.base = has_unsigned ? TB_UINT128 : TB_INT128;
                                       base_set = true; consume(); done = true; break;
            case TokenKind::KwUInt128: ts.base = TB_UINT128; base_set = true; consume(); done = true; break;

            case TokenKind::KwBuiltin:
                ts.base = TB_VA_LIST;
                ts.tag  = arena_.strdup("__va_list");
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwAutoType:
                // __auto_type: treat as int for now
                ts.base = TB_INT;
                base_set = true;
                consume(); done = true; break;

            case TokenKind::KwStruct:
            case TokenKind::KwClass:
                has_struct = true;
                consume();
                done = true;
                break;

            case TokenKind::KwUnion: has_union = true; consume(); done = true; break;
            case TokenKind::KwEnum:  has_enum  = true; consume(); done = true; break;

            case TokenKind::KwTypeofUnqual:
            case TokenKind::KwTypeof: {
                // typeof(type) or typeof(expr)
                bool strip_qual = (cur().kind == TokenKind::KwTypeofUnqual);
                consume();  // consume typeof / typeof_unqual
                expect(TokenKind::LParen);  // consume (
                if (infer_typeof_like_operand_type(&ts, strip_qual)) {
                    // Skip remaining tokens (handles expressions like `i + 1`) to closing )
                    int depth = 0;
                    while (!at_end() && !(check(TokenKind::RParen) && depth == 0)) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) --depth;
                        consume();
                    }
                    expect(TokenKind::RParen);
                } else {
                    // Complex expression: skip to balanced ) returning int
                    int depth = 1;
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) { if (--depth == 0) { consume(); break; } }
                        consume();
                    }
                    ts.base = TB_INT;
                    if (strip_qual) {
                        ts.is_const = false;
                        ts.is_volatile = false;
                    }
                }
                base_set = true;
                done = true; break;
            }

            case TokenKind::KwDecltype: {
                consume();  // consume decltype
                expect(TokenKind::LParen);
                if (infer_typeof_like_operand_type(&ts, /*strip_qual=*/false)) {
                    int depth = 0;
                    while (!at_end() && !(check(TokenKind::RParen) && depth == 0)) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) --depth;
                        consume();
                    }
                    expect(TokenKind::RParen);
                } else {
                    // C++ mode or unknown content: skip balanced parens.
                    // decltype expressions can be arbitrarily complex
                    // (qualified calls, template args, commas, etc.) so
                    // just skip to the closing paren and assume int.
                    int depth = 1;
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) ++depth;
                        else if (check(TokenKind::RParen)) {
                            if (--depth == 0) { consume(); break; }
                        }
                        consume();
                    }
                    ts.base = TB_INT;
                }
                base_set = true;
                done = true; break;
            }

            case TokenKind::KwStaticAssert: {
                // _Static_assert(expr, msg); - skip
                consume();
                skip_paren_group();
                match(TokenKind::Semi);
                done = true; break;
            }

            case TokenKind::ColonColon:
                if (!is_cpp_mode()) {
                    done = true;
                    break;
                }
                [[fallthrough]];

            case TokenKind::KwTypename:
            case TokenKind::Identifier:
                {
                    const TextId name_text_id = cur().text_id;
                    const std::string_view name = token_spelling(cur());
                    if (parse_builtin_transform_type(&ts)) {
                        base_set = true;
                        done = true;
                        break;
                    }
                    TypeBase floatn_base;
                    if (match_floatn_keyword_base(name, &floatn_base)) {
                        ts.base = floatn_base;
                        base_set = true;
                        consume();
                        done = true;
                        break;
                    }
                    if (is_cpp_mode()) {
                        const bool already_have_base =
                            has_signed || has_unsigned || has_short || long_count > 0 ||
                            has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                            has_struct || has_union || has_enum || base_set;
                        if (k == TokenKind::Identifier && !already_have_base &&
                            is_template_scope_type_param(name_text_id)) {
                            has_typedef = true;
                            ts.base = TB_TYPEDEF;
                            ts.tag = arena_.strdup(std::string(name).c_str());
                            ts.tag_text_id = name_text_id;
                            consume();
                            done = true;
                            break;
                        }
                        const bool has_following_scope =
                            core_input_state_.pos + 1 <
                                static_cast<int>(core_input_state_.tokens.size()) &&
                            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                                TokenKind::ColonColon;
                        const TypeSpec* visible_head_type =
                            (k == TokenKind::Identifier && has_following_scope)
                                ? find_visible_typedef_type(name_text_id)
                                : nullptr;
                        const bool typed_record_owner_scope =
                            visible_head_type && visible_head_type->record_def;
                        const bool simple_unqualified_known_type_head =
                            k == TokenKind::Identifier &&
                            (!has_following_scope || typed_record_owner_scope) &&
                            is_known_simple_visible_type_head(*this,
                                                              name_text_id,
                                                              name);
                        if (!simple_unqualified_known_type_head &&
                            try_parse_cpp_scoped_base_type(*this,
                                                           already_have_base,
                                                           &ts)) {
                            if (ts.base == TB_TYPEDEF || ts.tag) {
                                has_typedef = true;
                            } else {
                                base_set = true;
                            }
                            done = true;
                            break;
                        }
                    }
                    if (k == TokenKind::KwTypename) {
                        done = true;
                        break;
                    }
                    if (is_known_simple_visible_type_head(*this, cur().text_id,
                                                          name)) {
                        // If we've already seen concrete type specifiers/modifiers,
                        // this identifier is the declarator name (e.g. `int s;` even
                        // when `s` is also a typedef name in outer scope).
                        bool already_have_base =
                            has_signed || has_unsigned || has_short || long_count > 0 ||
                            has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                            has_struct || has_union || has_enum || base_set;
                        // C++ pointer-to-member declarators start with a class name
                        // after the pointee type: `R C::*member`. In that shape the
                        // class name belongs to the declarator, not the base type.
                        if (is_cpp_mode() &&
                            core_input_state_.pos + 2 <
                                static_cast<int>(core_input_state_.tokens.size()) &&
                            core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                                TokenKind::ColonColon &&
                            core_input_state_.tokens[core_input_state_.pos + 2].kind ==
                                TokenKind::Star) {
                            done = true;
                            break;
                        }
                        if (!already_have_base) {
                            has_typedef = true;
                            const VisibleNameResult visible_type =
                                resolve_visible_type(name_text_id);
                            const std::string resolved =
                                visible_type ? visible_name_spelling(visible_type)
                                             : std::string(name);
                            ts.tag = arena_.strdup(resolved.c_str());
                            ts.tag_text_id = visible_type && visible_type.base_text_id != kInvalidText
                                                 ? visible_type.base_text_id
                                                 : name_text_id;
                            consume();
                        }
                        done = true;
                    } else if (is_cpp_mode() &&
                               !(has_signed || has_unsigned || has_short || long_count > 0 ||
                                 has_int_kw || has_char || has_void || has_float || has_double || has_bool ||
                                 has_struct || has_union || has_enum || base_set) &&
                               looks_like_unresolved_identifier_type_head(core_input_state_.pos)) {
                        // C++ unresolved simple type in a declarator or parameter:
                        // treat identifier spellings such as `Box value`,
                        // `Box& value`, `Box* value`, `Box const& value`, and
                        // unnamed forms like `true_type)` as a type head even if
                        // typedef or injected-class-name registration was lost.
                        has_typedef = true;
                        const std::string spelled(name);
                        ts.tag = arena_.strdup(spelled.c_str());
                        ts.tag_text_id = name_text_id;
                        consume();
                        done = true;
                    } else {
                        done = true;  // end of type; identifier is the declarator name
                    }
                }
                break;

            // __attribute__ may appear within type specs (e.g., packed struct)
            case TokenKind::KwAttribute:
                parse_attributes(&ts);
                break;

            case TokenKind::KwAlignas: {
                parse_alignas_specifier(this, &ts, cur().line);
                break;
            }

            default:
                done = true; break;
        }
    }

    // Handle struct/union/enum after switch (needs extra parsing)
    if (has_struct) {
        Node* sd = parse_struct_or_union(*this, false);
        ts.base = TB_STRUCT;
        ts.tag  = sd ? sd->name : nullptr;
        ts.record_def = (sd && sd->n_fields >= 0) ? sd : nullptr;
        // In C++ mode, 'struct Pair<int>' should trigger template struct instantiation
        // just like 'Pair<int>' does via the typedef path. If the tag matches a known
        // template struct and '<' follows, fall through to the template instantiation
        // code below instead of returning immediately.
        if (!(is_cpp_mode() && ts.tag &&
              find_template_struct_primary(current_namespace_context_id(),
                                           parser_text_id_for_token(
                                               kInvalidText, ts.tag)) &&
              check(TokenKind::Less))) {
            return ts;
        }
        // Fall through to template struct instantiation below
    }
    if (has_union) {
        Node* sd = parse_struct_or_union(*this, true);
        ts.base = TB_UNION;
        ts.tag  = sd ? sd->name : nullptr;
        ts.record_def = (sd && sd->n_fields >= 0) ? sd : nullptr;
        return ts;
    }
    if (has_enum) {
        Node* ed = parse_enum(*this);
        definition_state_.last_enum_def = ed;
        if (ed && ed->name) {
            if (const TypeSpec* typedef_type = find_visible_typedef_type(find_parser_text_id(ed->name))) {
                ts = *typedef_type;
            }
        }
        ts.base = TB_ENUM;
        ts.tag  = ed ? ed->name : nullptr;
        return ts;
    }

    // If base was set directly (KwBuiltin, KwInt128, etc.), skip combined-specifier resolution.
    if (base_set) {
        finalize_vector_type(ts);
        return ts;
    }

    // Template struct instantiation for 'struct Pair<int>' syntax (has_struct fall-through).
    // ts.base is already TB_STRUCT and ts.tag is the template name.
    if (has_struct && is_cpp_mode() && ts.tag &&
        find_template_struct_primary(current_namespace_context_id(),
                                     parser_text_id_for_token(kInvalidText,
                                                              ts.tag)) &&
        check(TokenKind::Less)) {
        // Reuse the typedef-path template instantiation by setting has_typedef and
        // preparing ts as if the typedef had been resolved.
        has_typedef = true;
        has_struct = false;
    }

    // Resolve combined specifiers
    if (has_typedef) {
        // Try to resolve the typedef to its underlying TypeSpec
        const char* tname = ts.tag;
        if (tname) {
            const bool is_unqualified_typedef =
                std::strstr(tname, "::") == nullptr;
            const TextId tname_text_id =
                is_unqualified_typedef && ts.tag_text_id != kInvalidText
                    ? ts.tag_text_id
                    : parser_text_id_for_token(kInvalidText, tname);
            const TypeSpec* typedef_type =
                is_unqualified_typedef ? find_visible_typedef_type(tname_text_id)
                                       : find_typedef_type(tname_text_id);
            if (typedef_type) {
                // Resolve: use the stored TypeSpec, preserving qualifiers from this context
                bool save_const = ts.is_const, save_vol = ts.is_volatile;
                const bool has_qualified_type_metadata =
                    ts.is_global_qualified || ts.namespace_context_id >= 0 ||
                    ts.n_qualifier_segments > 0 ||
                    ts.tag_text_id != kInvalidText;
                const TextId save_tag_text_id = ts.tag_text_id;
                const bool save_is_global_qualified = ts.is_global_qualified;
                const int save_namespace_context_id = ts.namespace_context_id;
                const int save_n_qualifier_segments = ts.n_qualifier_segments;
                const char** save_qualifier_segments = ts.qualifier_segments;
                TextId* save_qualifier_text_ids = ts.qualifier_text_ids;
                ts = *typedef_type;
                if (has_qualified_type_metadata) {
                    ts.tag_text_id = save_tag_text_id;
                    ts.is_global_qualified = save_is_global_qualified;
                    ts.namespace_context_id = save_namespace_context_id;
                    ts.n_qualifier_segments = save_n_qualifier_segments;
                    ts.qualifier_segments = save_qualifier_segments;
                    ts.qualifier_text_ids = save_qualifier_text_ids;
                }
                ts.is_const   |= save_const;
                ts.is_volatile |= save_vol;
                // Phase C: remember the typedef name for fn_ptr param propagation.
                set_last_resolved_typedef(tname_text_id, tname);
                // Alias template application: e.g. bool_constant<expr> → integral_constant<bool, expr>
                if (is_cpp_mode() && check(TokenKind::Less)) {
                    auto alias_template_key_from_saved_type_metadata =
                        [&]() -> QualifiedNameKey {
                        TextId alias_text_id =
                            save_tag_text_id != kInvalidText
                                ? save_tag_text_id
                                : active_context_state_
                                      .last_resolved_typedef_text_id;
                        if (alias_text_id == kInvalidText) return {};

                        if (save_namespace_context_id >= 0) {
                            return alias_template_key_in_context(
                                save_namespace_context_id, alias_text_id);
                        }

                        if (save_is_global_qualified ||
                            save_n_qualifier_segments > 0) {
                            QualifiedNameRef alias_qn;
                            alias_qn.base_text_id = alias_text_id;
                            alias_qn.is_global_qualified =
                                save_is_global_qualified;
                            alias_qn.qualifier_segments.reserve(
                                save_n_qualifier_segments);
                            alias_qn.qualifier_text_ids.reserve(
                                save_n_qualifier_segments);
                            for (int qi = 0;
                                 qi < save_n_qualifier_segments; ++qi) {
                                const char* segment =
                                    save_qualifier_segments
                                        ? save_qualifier_segments[qi]
                                        : nullptr;
                                alias_qn.qualifier_segments.push_back(
                                    segment ? std::string(segment)
                                            : std::string{});
                                TextId segment_text_id =
                                    save_qualifier_text_ids
                                        ? save_qualifier_text_ids[qi]
                                        : kInvalidText;
                                if (segment_text_id == kInvalidText && segment &&
                                    segment[0]) {
                                    segment_text_id =
                                        find_parser_text_id(segment);
                                }
                                alias_qn.qualifier_text_ids.push_back(
                                    segment_text_id);
                            }
                            const int context_id =
                                resolve_namespace_context(alias_qn);
                            if (context_id >= 0) {
                                return alias_template_key_in_context(
                                    context_id, alias_text_id);
                            }
                        }

                        return alias_template_key_in_context(
                            current_namespace_context_id(), alias_text_id);
                    };
                    const QualifiedNameKey alias_key =
                        alias_template_key_from_saved_type_metadata();
                    const ParserAliasTemplateInfo* ati =
                        find_alias_template_info(alias_key);
                    if (ati) {
                        TentativeParseGuard alias_guard(*this);
                        std::vector<TemplateArgParseResult> alias_args;
                        const size_t alias_param_count = std::max({
                            ati->param_name_text_ids.size(),
                            ati->param_is_nttp.size(),
                            ati->param_is_pack.size(),
                            ati->param_has_default.size(),
                            ati->param_default_types.size(),
                            ati->param_default_values.size(),
                            ati->param_names.size(),
                        });
                        auto alias_args_match = [&](const std::vector<TemplateArgParseResult>& args)
                            -> bool {
                            size_t ai = 0;
                            for (size_t pi = 0; pi < alias_param_count; ++pi) {
                                const bool is_pack =
                                    pi < ati->param_is_pack.size() && ati->param_is_pack[pi];
                                const bool expects_value =
                                    pi < ati->param_is_nttp.size() && ati->param_is_nttp[pi];
                                if (is_pack) {
                                    while (ai < args.size()) {
                                        if (args[ai].is_value != expects_value) return false;
                                        ++ai;
                                    }
                                    return true;
                                }
                                if (ai >= args.size()) {
                                    const bool has_default =
                                        pi < ati->param_has_default.size() &&
                                        ati->param_has_default[pi];
                                    if (!has_default) return false;
                                    continue;
                                }
                                if (args[ai].is_value != expects_value) return false;
                                ++ai;
                            }
                            return ai == args.size();
                        };
                        if (!parse_template_argument_list(
                                &alias_args,
                                nullptr,
                                &ati->param_is_nttp) ||
                            !alias_args_match(alias_args)) {
                            // alias_guard restores pos_ on scope exit
                        } else {
                            bool alias_parse_ok = true;
                            std::vector<TemplateArgParseResult> resolved_alias_args =
                                alias_args;
                            for (size_t pi = resolved_alias_args.size();
                                 pi < alias_param_count; ++pi) {
                                const bool is_pack =
                                    pi < ati->param_is_pack.size() && ati->param_is_pack[pi];
                                if (is_pack) break;
                                const bool has_default =
                                    pi < ati->param_has_default.size() &&
                                    ati->param_has_default[pi];
                                if (!has_default) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                TemplateArgParseResult default_arg;
                                default_arg.is_value =
                                    pi < ati->param_is_nttp.size() &&
                                    ati->param_is_nttp[pi];
                                if (default_arg.is_value) {
                                    if (pi >= ati->param_default_values.size() ||
                                        ati->param_default_values[pi] == LLONG_MIN) {
                                        alias_parse_ok = false;
                                        break;
                                    }
                                    default_arg.value = ati->param_default_values[pi];
                                } else {
                                    if (pi >= ati->param_default_types.size()) {
                                        alias_parse_ok = false;
                                        break;
                                    }
                                    default_arg.type = ati->param_default_types[pi];
                                }
                                resolved_alias_args.push_back(default_arg);
                            }
                            auto alias_param_text_id =
                                [&](size_t param_index) -> TextId {
                                    if (param_index <
                                            ati->param_name_text_ids.size() &&
                                        ati->param_name_text_ids[param_index] !=
                                            kInvalidText) {
                                        return ati->param_name_text_ids[param_index];
                                    }
                                    const char* pname =
                                        param_index < ati->param_names.size()
                                            ? ati->param_names[param_index]
                                            : nullptr;
                                    return pname && pname[0]
                                               ? parser_text_id_for_token(
                                                     kInvalidText, pname)
                                               : kInvalidText;
                                };
                            auto alias_param_ref_text_id =
                                [&](std::string_view ref) -> TextId {
                                    if (ref.empty()) return kInvalidText;
                                    TextId ref_text_id = find_parser_text_id(ref);
                                    if (ref_text_id != kInvalidText) {
                                        return ref_text_id;
                                    }
                                    for (size_t pi = 0;
                                         pi < alias_param_count; ++pi) {
                                        if (pi < ati->param_name_text_ids.size() &&
                                            ati->param_name_text_ids[pi] !=
                                                kInvalidText) {
                                            continue;
                                        }
                                        const char* pname = ati->param_names[pi];
                                        if (pname && ref == pname) {
                                            return alias_param_text_id(pi);
                                        }
                                    }
                                    return kInvalidText;
                                };
                            size_t bound_arg_index = 0;
                            for (size_t pi = 0; pi < alias_param_count; ++pi) {
                                const TextId param_text_id =
                                    alias_param_text_id(pi);
                                if (param_text_id == kInvalidText) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                const bool is_pack =
                                    pi < ati->param_is_pack.size() && ati->param_is_pack[pi];
                                const bool expects_value =
                                    pi < ati->param_is_nttp.size() && ati->param_is_nttp[pi];
                                if (is_pack) {
                                    while (bound_arg_index < resolved_alias_args.size()) {
                                        if (resolved_alias_args[bound_arg_index].is_value !=
                                            expects_value) {
                                            alias_parse_ok = false;
                                            break;
                                        }
                                        ++bound_arg_index;
                                    }
                                    if (!alias_parse_ok) break;
                                    continue;
                                }
                                if (bound_arg_index >= resolved_alias_args.size()) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                if (resolved_alias_args[bound_arg_index].is_value !=
                                    expects_value) {
                                    alias_parse_ok = false;
                                    break;
                                }
                                ++bound_arg_index;
                            }
                            if (alias_parse_ok &&
                                bound_arg_index != resolved_alias_args.size()) {
                                alias_parse_ok = false;
                            }
                            if (!alias_parse_ok) {
                                // alias_guard restores pos_ on scope exit
                            } else {
                            // Substitute alias params in the aliased type.
                                ts = ati->aliased_type;
                                auto split_template_arg_refs =
                                    [&](const std::string& refs)
                                    -> std::vector<std::string> {
                                        std::vector<std::string> parts;
                                        if (refs.empty()) return parts;
                                        size_t start = 0;
                                        int angle_depth = 0;
                                        int paren_depth = 0;
                                        int bracket_depth = 0;
                                        int brace_depth = 0;
                                        for (size_t i = 0; i < refs.size(); ++i) {
                                            const char ch = refs[i];
                                            switch (ch) {
                                                case '<': ++angle_depth; break;
                                                case '>':
                                                    if (angle_depth > 0) --angle_depth;
                                                    break;
                                                case '(':
                                                    ++paren_depth;
                                                    break;
                                                case ')':
                                                    if (paren_depth > 0) --paren_depth;
                                                    break;
                                                case '[':
                                                    ++bracket_depth;
                                                    break;
                                                case ']':
                                                    if (bracket_depth > 0) --bracket_depth;
                                                    break;
                                                case '{':
                                                    ++brace_depth;
                                                    break;
                                                case '}':
                                                    if (brace_depth > 0) --brace_depth;
                                                    break;
                                                case ',':
                                                    if (angle_depth == 0 &&
                                                        paren_depth == 0 &&
                                                        bracket_depth == 0 &&
                                                        brace_depth == 0) {
                                                        parts.push_back(
                                                            refs.substr(start, i - start));
                                                        start = i + 1;
                                                    }
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                        parts.push_back(refs.substr(start));
                                        return parts;
                                    };
                                    auto alias_param_index_for_text_id =
                                        [&](TextId param_text_id) -> size_t {
                                            if (param_text_id == kInvalidText) {
                                                return alias_param_count;
                                            }
                                            for (size_t pi = 0;
                                                 pi < alias_param_count; ++pi) {
                                                if (alias_param_text_id(pi) ==
                                                    param_text_id) {
                                                    return pi;
                                                }
                                            }
                                            return alias_param_count;
                                        };
                                    auto substitute_carrier_arg =
                                        [&](const ParsedTemplateArg& carrier_arg,
                                            ParsedTemplateArg* out_arg) -> bool {
                                            if (!out_arg) return false;
                                            *out_arg = carrier_arg;
                                            if (carrier_arg.is_value) {
                                                if (!carrier_arg.nttp_name ||
                                                    !carrier_arg.nttp_name[0]) {
                                                    return true;
                                                }
                                                const TextId nttp_text_id =
                                                    alias_param_ref_text_id(
                                                        carrier_arg.nttp_name);
                                                const size_t pi =
                                                    alias_param_index_for_text_id(
                                                        nttp_text_id);
                                                if (pi >= resolved_alias_args.size() ||
                                                    pi >= alias_param_count) {
                                                    return true;
                                                }
                                                if (!resolved_alias_args[pi].is_value) {
                                                    return false;
                                                }
                                                *out_arg = resolved_alias_args[pi];
                                                return true;
                                            }
                                            const TypeSpec& carrier_type =
                                                carrier_arg.type;
                                            if (carrier_type.base != TB_TYPEDEF ||
                                                !carrier_type.tag ||
                                                !carrier_type.tag[0]) {
                                                return true;
                                            }
                                            const TextId type_text_id =
                                                alias_param_ref_text_id(
                                                    carrier_type.tag);
                                            const size_t pi =
                                                alias_param_index_for_text_id(
                                                    type_text_id);
                                            if (pi >= resolved_alias_args.size() ||
                                                pi >= alias_param_count) {
                                                return true;
                                            }
                                            if (resolved_alias_args[pi].is_value) {
                                                return false;
                                            }
                                            *out_arg = resolved_alias_args[pi];
                                            return true;
                                        };
                                    auto template_arg_ref_from_parsed_arg =
                                        [&](const ParsedTemplateArg& arg)
                                        -> TemplateArgRef {
                                        TemplateArgRef out{};
                                        out.kind = arg.is_value
                                                       ? TemplateArgKind::Value
                                                       : TemplateArgKind::Type;
                                        out.type = arg.is_value ? TypeSpec{} : arg.type;
                                        if (arg.is_value) {
                                            out.type.array_size = -1;
                                            out.type.inner_rank = -1;
                                            out.type.array_size_expr = arg.expr;
                                        }
                                        out.value = arg.is_value ? arg.value : 0;
                                        out.nttp_text_id =
                                            arg.is_value ? arg.nttp_text_id
                                                         : kInvalidText;
                                        const std::string debug_ref =
                                            render_template_arg_ref(arg);
                                        out.debug_text =
                                            debug_ref.empty()
                                                ? nullptr
                                                : arena_.strdup(debug_ref.c_str());
                                        return out;
                                    };
                                    std::function<bool(TemplateArgRef*, int)>
                                        substitute_template_arg_ref_structured =
                                            [&](TemplateArgRef* ref,
                                                int depth) -> bool {
                                        if (!ref) return false;
                                        if (depth > 64) return true;
                                        if (ref->kind == TemplateArgKind::Value) {
                                            const TextId value_text_id =
                                                ref->nttp_text_id;
                                            const size_t pi =
                                                alias_param_index_for_text_id(
                                                    value_text_id);
                                            if (pi >= resolved_alias_args.size() ||
                                                pi >= alias_param_count) {
                                                return true;
                                            }
                                            if (!resolved_alias_args[pi].is_value) {
                                                return false;
                                            }
                                            *ref = template_arg_ref_from_parsed_arg(
                                                resolved_alias_args[pi]);
                                            return true;
                                        }

                                        TypeSpec& ref_type = ref->type;
                                        if (ref_type.base == TB_TYPEDEF &&
                                            ref_type.tag && ref_type.tag[0]) {
                                            const TextId type_text_id =
                                                alias_param_ref_text_id(ref_type.tag);
                                            const size_t pi =
                                                alias_param_index_for_text_id(
                                                    type_text_id);
                                            if (pi < resolved_alias_args.size() &&
                                                pi < alias_param_count) {
                                                if (resolved_alias_args[pi].is_value) {
                                                    return false;
                                                }
                                                *ref = template_arg_ref_from_parsed_arg(
                                                    resolved_alias_args[pi]);
                                                return true;
                                            }
                                        }

                                        if (ref_type.tpl_struct_args.data &&
                                            ref_type.tpl_struct_args.size > 0) {
                                            for (int ai = 0;
                                                 ai < ref_type.tpl_struct_args.size;
                                                ++ai) {
                                                if (!substitute_template_arg_ref_structured(
                                                        &ref_type.tpl_struct_args.data[ai],
                                                        depth + 1)) {
                                                    return false;
                                                }
                                            }
                                        }
                                        return true;
                                    };
                                    auto substitute_template_arg_refs_structured =
                                        [&](TypeSpec* target) -> bool {
                                        if (!target || !target->tpl_struct_args.data ||
                                            target->tpl_struct_args.size <= 0) {
                                            return true;
                                        }
                                        for (int ai = 0;
                                             ai < target->tpl_struct_args.size; ++ai) {
                                            if (!substitute_template_arg_ref_structured(
                                                    &target->tpl_struct_args.data[ai],
                                                    0)) {
                                                return false;
                                            }
                                        }
                                        return true;
                                    };
                                    auto materialize_template_origin_record_def =
                                        [&](TypeSpec* target) -> bool {
                                        if (!target ||
                                            target->record_def ||
                                            target->tpl_struct_origin_key
                                                    .base_text_id ==
                                                kInvalidText ||
                                            !target->tpl_struct_args.data ||
                                            target->tpl_struct_args.size <= 0) {
                                            return false;
                                        }
                                        const Node* primary =
                                            find_template_struct_primary(
                                                target->tpl_struct_origin_key);
                                        if (!primary) return false;

                                        std::vector<ParsedTemplateArg> args;
                                        args.reserve(
                                            target->tpl_struct_args.size);
                                        for (int ai = 0;
                                             ai < target->tpl_struct_args.size;
                                             ++ai) {
                                            const TemplateArgRef& src =
                                                target->tpl_struct_args.data[ai];
                                            ParsedTemplateArg arg{};
                                            if (src.kind ==
                                                TemplateArgKind::Value) {
                                                if (zero_value_arg_ref_uses_debug_fallback(
                                                        src)) {
                                                    return false;
                                                }
                                                arg.is_value = true;
                                                arg.value = src.value;
                                                arg.expr =
                                                    src.type.array_size_expr;
                                                if (arg.expr) {
                                                    return false;
                                                }
                                                arg.nttp_text_id =
                                                    src.nttp_text_id;
                                            } else {
                                                if (unstructured_type_arg_ref_uses_debug_fallback(
                                                        src)) {
                                                    return false;
                                                }
                                                if (src.type.tpl_struct_origin ||
                                                    (src.type.tpl_struct_args
                                                         .data &&
                                                     src.type.tpl_struct_args
                                                             .size > 0)) {
                                                    return false;
                                                }
                                                arg.is_value = false;
                                                arg.type = src.type;
                                            }
                                            args.push_back(arg);
                                        }

                                        const char* origin =
                                            primary->name && primary->name[0]
                                                ? primary->name
                                            : primary->template_origin_name &&
                                                    primary
                                                        ->template_origin_name[0]
                                                ? primary->template_origin_name
                                            : primary->unqualified_name &&
                                                    primary->unqualified_name[0]
                                                ? primary->unqualified_name
                                                : target->tpl_struct_origin;
                                        const char* saved_member_name =
                                            target->deferred_member_type_name;
                                        const TextId saved_member_text_id =
                                            target->deferred_member_type_text_id;
                                        if (!origin || !origin[0]) {
                                            return false;
                                        }
                                        TypeSpec resolved = *target;
                                        std::string mangled;
                                        if (!ensure_template_struct_instantiated_from_args(
                                                origin, primary, args,
                                                cur().line, &mangled,
                                                "alias_template_origin_key_record_def",
                                                &resolved)) {
                                            return false;
                                        }
                                        if (!resolved.record_def) {
                                            if (Node* record =
                                                    resolve_record_type_spec(
                                                        resolved, nullptr)) {
                                                resolved.record_def = record;
                                            }
                                        }
                                        if (!resolved.record_def) return false;
                                        resolved.deferred_member_type_name =
                                            saved_member_name;
                                        resolved.deferred_member_type_text_id =
                                            saved_member_text_id;
                                        *target = resolved;
                                        return true;
                                    };
                                    auto type_mentions_template_scope_param =
                                        [&](const TypeSpec& type) -> bool {
                                        std::function<bool(const TypeSpec&)> type_mentions_dep_params =
                                            [&](const TypeSpec& inner) -> bool {
                                                if (inner.base == TB_TYPEDEF &&
                                                    inner.tag &&
                                                    inner.tag[0] &&
                                                    is_template_scope_type_param(
                                                        alias_param_ref_text_id(
                                                            inner.tag))) {
                                                    return true;
                                                }
                                                if (!inner.tpl_struct_args.data ||
                                                    inner.tpl_struct_args.size <= 0) {
                                                    return false;
                                                }
                                                for (int ai = 0;
                                                     ai < inner.tpl_struct_args.size; ++ai) {
                                                    const TemplateArgRef& arg =
                                                        inner.tpl_struct_args.data[ai];
                                                    if (arg.kind == TemplateArgKind::Type &&
                                                        type_mentions_dep_params(arg.type)) {
                                                        return true;
                                                    }
                                                }
                                                return false;
                                            };
                                            return type_mentions_dep_params(type);
                                        };
                                    auto apply_alias_template_bindings =
                                        [&](TypeSpec member_ts,
                                            const std::vector<std::pair<std::string, TypeSpec>>& type_bindings,
                                            const std::vector<std::pair<std::string, long long>>& nttp_bindings,
                                            bool* substituted_type = nullptr) {
                                            if (substituted_type) {
                                                *substituted_type = false;
                                            }
                                            for (const auto& [pname, pts] :
                                                 type_bindings) {
                                                if (member_ts.base == TB_TYPEDEF &&
                                                    member_ts.tag &&
                                                    std::string(member_ts.tag) ==
                                                        pname) {
                                                    const int outer_ptr_level =
                                                        member_ts.ptr_level;
                                                    const bool outer_lref =
                                                        member_ts.is_lvalue_ref;
                                                    const bool outer_rref =
                                                        member_ts.is_rvalue_ref;
                                                    const bool outer_const =
                                                        member_ts.is_const;
                                                    const bool outer_volatile =
                                                        member_ts.is_volatile;
                                                    member_ts = pts;
                                                    member_ts.ptr_level +=
                                                        outer_ptr_level;
                                                    member_ts.is_lvalue_ref =
                                                        member_ts.is_lvalue_ref ||
                                                        outer_lref;
                                                    member_ts.is_rvalue_ref =
                                                        !member_ts.is_lvalue_ref &&
                                                        (member_ts.is_rvalue_ref ||
                                                         outer_rref);
                                                    member_ts.is_const =
                                                        member_ts.is_const ||
                                                        outer_const;
                                                    member_ts.is_volatile =
                                                        member_ts.is_volatile ||
                                                        outer_volatile;
                                                    if (substituted_type) {
                                                        *substituted_type = true;
                                                    }
                                                    break;
                                                }
                                            }
                                            if (member_ts.array_size_expr &&
                                                member_ts.array_size_expr->kind ==
                                                    NK_VAR &&
                                                member_ts.array_size_expr->name) {
                                                for (const auto& [pname, value] :
                                                     nttp_bindings) {
                                                    if (std::string(
                                                            member_ts.array_size_expr
                                                                ->name) == pname) {
                                                        if (member_ts.array_rank > 0) {
                                                            member_ts.array_dims[0] =
                                                                value;
                                                            member_ts.array_size =
                                                                value;
                                                        }
                                                        member_ts.array_size_expr =
                                                            nullptr;
                                                        break;
                                                    }
                                                }
                                            }
                                            return member_ts;
                                        };
                                    auto resolve_structured_alias_member_type =
                                        [&]() -> bool {
                                            if (!ati->member_typedef.valid ||
                                                ati->member_typedef.owner_key
                                                        .base_text_id ==
                                                    kInvalidText ||
                                                ati->member_typedef.member_text_id ==
                                                    kInvalidText) {
                                                return false;
                                            }
                                            std::vector<ParsedTemplateArg>
                                                actual_args;
                                            actual_args.reserve(
                                                ati->member_typedef.owner_args
                                                    .size());
                                            bool has_dependent_arg = false;
                                            for (const ParsedTemplateArg& carrier_arg :
                                                 ati->member_typedef.owner_args) {
                                                ParsedTemplateArg actual{};
                                                if (!substitute_carrier_arg(
                                                        carrier_arg, &actual)) {
                                                    return false;
                                                }
                                                if (!actual.is_value &&
                                                    type_mentions_template_scope_param(
                                                        actual.type)) {
                                                    has_dependent_arg = true;
                                                }
                                                actual_args.push_back(actual);
                                            }
                                            const Node* primary_tpl =
                                                find_template_struct_primary(
                                                    ati->member_typedef.owner_key);
                                            if (!primary_tpl) {
                                                const ParserAliasTemplateInfo*
                                                    owner_alias =
                                                        find_alias_template_info(
                                                            ati->member_typedef
                                                                .owner_key);
                                                if (!owner_alias ||
                                                    !owner_alias->member_typedef
                                                         .valid) {
                                                    return false;
                                                }

                                                const size_t owner_param_count =
                                                    std::max({
                                                        owner_alias
                                                            ->param_name_text_ids
                                                            .size(),
                                                        owner_alias->param_is_nttp
                                                            .size(),
                                                        owner_alias->param_is_pack
                                                            .size(),
                                                        owner_alias
                                                            ->param_has_default
                                                            .size(),
                                                        owner_alias
                                                            ->param_default_types
                                                            .size(),
                                                        owner_alias
                                                            ->param_default_values
                                                            .size(),
                                                        owner_alias->param_names
                                                            .size(),
                                                    });
                                                auto owner_alias_param_text_id =
                                                    [&](size_t param_index)
                                                    -> TextId {
                                                    if (param_index <
                                                            owner_alias
                                                                ->param_name_text_ids
                                                                .size() &&
                                                        owner_alias
                                                                ->param_name_text_ids
                                                                    [param_index] !=
                                                            kInvalidText) {
                                                        return owner_alias
                                                            ->param_name_text_ids
                                                                [param_index];
                                                    }
                                                    const char* pname =
                                                        param_index <
                                                                owner_alias
                                                                    ->param_names
                                                                    .size()
                                                            ? owner_alias
                                                                  ->param_names
                                                                      [param_index]
                                                            : nullptr;
                                                    return pname && pname[0]
                                                               ? parser_text_id_for_token(
                                                                     kInvalidText,
                                                                     pname)
                                                               : kInvalidText;
                                                };
                                                auto owner_alias_param_ref_text_id =
                                                    [&](std::string_view ref)
                                                    -> TextId {
                                                    if (ref.empty())
                                                        return kInvalidText;
                                                    TextId ref_text_id =
                                                        find_parser_text_id(ref);
                                                    if (ref_text_id !=
                                                        kInvalidText) {
                                                        return ref_text_id;
                                                    }
                                                    for (size_t pi = 0;
                                                         pi < owner_param_count;
                                                         ++pi) {
                                                        if (pi <
                                                                owner_alias
                                                                    ->param_name_text_ids
                                                                    .size() &&
                                                            owner_alias
                                                                    ->param_name_text_ids
                                                                        [pi] !=
                                                                kInvalidText) {
                                                            continue;
                                                        }
                                                        const char* pname =
                                                            pi < owner_alias
                                                                     ->param_names
                                                                     .size()
                                                                ? owner_alias
                                                                      ->param_names
                                                                          [pi]
                                                                : nullptr;
                                                        if (pname &&
                                                            ref == pname) {
                                                            return owner_alias_param_text_id(
                                                                pi);
                                                        }
                                                    }
                                                    return kInvalidText;
                                                };
                                                auto owner_alias_param_index_for_text_id =
                                                    [&](TextId param_text_id)
                                                    -> size_t {
                                                    if (param_text_id ==
                                                        kInvalidText) {
                                                        return owner_param_count;
                                                    }
                                                    for (size_t pi = 0;
                                                         pi < owner_param_count;
                                                         ++pi) {
                                                        if (owner_alias_param_text_id(
                                                                pi) ==
                                                            param_text_id) {
                                                            return pi;
                                                        }
                                                    }
                                                    return owner_param_count;
                                                };
                                                auto substitute_owner_carrier_arg =
                                                    [&](const ParsedTemplateArg&
                                                            carrier_arg,
                                                        ParsedTemplateArg*
                                                            out_arg) -> bool {
                                                    if (!out_arg) return false;
                                                    *out_arg = carrier_arg;
                                                    if (carrier_arg.is_value) {
                                                        if (!carrier_arg
                                                                 .nttp_name ||
                                                            !carrier_arg
                                                                 .nttp_name[0]) {
                                                            return true;
                                                        }
                                                        const TextId nttp_text_id =
                                                            owner_alias_param_ref_text_id(
                                                                carrier_arg
                                                                    .nttp_name);
                                                        const size_t pi =
                                                            owner_alias_param_index_for_text_id(
                                                                nttp_text_id);
                                                        if (pi >=
                                                                actual_args.size() ||
                                                            pi >=
                                                                owner_param_count) {
                                                            return true;
                                                        }
                                                        if (!actual_args[pi]
                                                                 .is_value) {
                                                            return false;
                                                        }
                                                        *out_arg =
                                                            actual_args[pi];
                                                        return true;
                                                    }
                                                    const TypeSpec& carrier_type =
                                                        carrier_arg.type;
                                                    if (carrier_type.base !=
                                                            TB_TYPEDEF ||
                                                        !carrier_type.tag ||
                                                        !carrier_type.tag[0]) {
                                                        return true;
                                                    }
                                                    const TextId type_text_id =
                                                        owner_alias_param_ref_text_id(
                                                            carrier_type.tag);
                                                    const size_t pi =
                                                        owner_alias_param_index_for_text_id(
                                                            type_text_id);
                                                    if (pi >=
                                                            actual_args.size() ||
                                                        pi >=
                                                            owner_param_count) {
                                                        return true;
                                                    }
                                                    if (actual_args[pi]
                                                            .is_value) {
                                                        return false;
                                                    }
                                                    *out_arg = actual_args[pi];
                                                    return true;
                                                };

                                                std::vector<ParsedTemplateArg>
                                                    owner_actual_args;
                                                owner_actual_args.reserve(
                                                    owner_alias->member_typedef
                                                        .owner_args.size());
                                                for (const ParsedTemplateArg&
                                                         carrier_arg :
                                                     owner_alias->member_typedef
                                                         .owner_args) {
                                                    ParsedTemplateArg actual{};
                                                    if (!substitute_owner_carrier_arg(
                                                            carrier_arg,
                                                            &actual)) {
                                                        return false;
                                                    }
                                                    owner_actual_args.push_back(
                                                        actual);
                                                }

                                                TypeSpec owner_ts{};
                                                const ParserTemplateState::
                                                    TemplateInstantiationKey
                                                        concrete_alias_owner{
                                                            owner_alias
                                                                ->member_typedef
                                                                .owner_key,
                                                            make_template_instantiation_argument_keys(
                                                                owner_actual_args)};
                                                if (const TypeSpec*
                                                        structured_owner =
                                                            find_template_instantiation_member_typedef_type(
                                                                concrete_alias_owner,
                                                                owner_alias
                                                                    ->member_typedef
                                                                    .member_text_id)) {
                                                    owner_ts = *structured_owner;
                                                } else {
                                                    const Node*
                                                        owner_primary_tpl =
                                                            find_template_struct_primary(
                                                                owner_alias
                                                                    ->member_typedef
                                                                    .owner_key);
                                                    if (!owner_primary_tpl) {
                                                        return false;
                                                    }
                                                    std::vector<std::pair<std::string, TypeSpec>>
                                                        owner_type_bindings;
                                                    std::vector<std::pair<std::string, long long>>
                                                        owner_nttp_bindings;
                                                    const std::vector<Node*>*
                                                        owner_specializations =
                                                            find_template_struct_specializations(
                                                                owner_alias
                                                                    ->member_typedef
                                                                    .owner_key);
                                                    const Node* owner_selected =
                                                        select_template_struct_pattern_for_args(
                                                            owner_actual_args,
                                                            owner_primary_tpl,
                                                            owner_specializations,
                                                            &owner_type_bindings,
                                                            &owner_nttp_bindings);
                                                    if (!owner_selected ||
                                                        owner_selected
                                                                ->n_member_typedefs <=
                                                            0) {
                                                        return false;
                                                    }
                                                    for (int mi = 0;
                                                         mi < owner_selected
                                                                  ->n_member_typedefs;
                                                         ++mi) {
                                                        const char* member =
                                                            owner_selected
                                                                ->member_typedef_names
                                                                    [mi];
                                                        if (!member ||
                                                            !member[0]) {
                                                            continue;
                                                        }
                                                        const TextId
                                                            selected_member_text_id =
                                                                parser_text_id_for_token(
                                                                    kInvalidText,
                                                                    member);
                                                        if (selected_member_text_id !=
                                                            owner_alias
                                                                ->member_typedef
                                                                .member_text_id) {
                                                            continue;
                                                        }
                                                        bool substituted = false;
                                                        owner_ts =
                                                            apply_alias_template_bindings(
                                                                owner_selected
                                                                    ->member_typedef_types
                                                                        [mi],
                                                                owner_type_bindings,
                                                                owner_nttp_bindings,
                                                                &substituted);
                                                        (void)substituted;
                                                        break;
                                                    }
                                                    if (!((owner_ts.tag &&
                                                           owner_ts.tag[0]) ||
                                                          owner_ts.record_def ||
                                                          owner_ts
                                                              .tpl_struct_origin)) {
                                                        return false;
                                                    }
                                                }

                                                const std::string member_name(
                                                    parser_text(
                                                        ati->member_typedef
                                                            .member_text_id,
                                                        {}));
                                                if (member_name.empty()) {
                                                    return false;
                                                }
                                                if (lookup_struct_member_typedef_recursive_for_type(
                                                        owner_ts,
                                                        member_name,
                                                        ati->member_typedef
                                                            .member_text_id,
                                                        &ts)) {
                                                    return true;
                                                }
                                                return false;
                                            }
                                            if (has_dependent_arg) {
                                                ParserAliasTemplateMemberTypedefInfo
                                                    dependent_carrier;
                                                dependent_carrier.valid = true;
                                                dependent_carrier.owner_key =
                                                    ati->member_typedef.owner_key;
                                                dependent_carrier.owner_args =
                                                    actual_args;
                                                dependent_carrier.member_text_id =
                                                    ati->member_typedef
                                                        .member_text_id;
                                                active_context_state_
                                                    .last_using_alias_member_typedef =
                                                    std::move(dependent_carrier);
                                                return true;
                                            }

                                            const ParserTemplateState::
                                                TemplateInstantiationKey
                                                    concrete_owner{
                                                        ati->member_typedef
                                                            .owner_key,
                                                        make_template_instantiation_argument_keys(
                                                            actual_args)};
                                            if (const TypeSpec* structured_member =
                                                    find_template_instantiation_member_typedef_type(
                                                        concrete_owner,
                                                        ati->member_typedef
                                                            .member_text_id)) {
                                                ts = *structured_member;
                                                return true;
                                            }

                                            std::vector<std::pair<std::string, TypeSpec>>
                                                type_bindings;
                                            std::vector<std::pair<std::string, long long>>
                                                nttp_bindings;
                                            const std::vector<Node*>* specializations =
                                                find_template_struct_specializations(
                                                    ati->member_typedef.owner_key);
                                            const Node* selected =
                                                select_template_struct_pattern_for_args(
                                                    actual_args, primary_tpl,
                                                    specializations,
                                                    &type_bindings,
                                                    &nttp_bindings);
                                            if (!selected ||
                                                selected->n_member_typedefs <= 0) {
                                                return false;
                                            }
                                            for (int mi = 0;
                                                 mi < selected->n_member_typedefs;
                                                 ++mi) {
                                                const char* member =
                                                    selected
                                                        ->member_typedef_names[mi];
                                                if (!member || !member[0]) continue;
                                                const TextId selected_member_text_id =
                                                    parser_text_id_for_token(
                                                        kInvalidText, member);
                                                if (selected_member_text_id !=
                                                    ati->member_typedef
                                                        .member_text_id) {
                                                    continue;
                                                }
                                                bool substituted = false;
                                                ts = apply_alias_template_bindings(
                                                    selected
                                                        ->member_typedef_types[mi],
                                                    type_bindings, nttp_bindings,
                                                    &substituted);
                                                (void)substituted;
                                                return true;
                                            }
                                            return false;
                                        };
                                bool resolved_alias_member = false;
                                resolved_alias_member =
                                    resolve_structured_alias_member_type();
                                if (!resolved_alias_member) {
                                    for (size_t ai = 0;
                                         ai < resolved_alias_args.size() &&
                                         ai < alias_param_count; ++ai) {
                                        if (ai < ati->param_is_nttp.size() &&
                                            ati->param_is_nttp[ai]) {
                                            continue;
                                        }
                                        const TextId tag_text_id =
                                            ts.tag ? alias_param_ref_text_id(ts.tag)
                                                   : kInvalidText;
                                        if (ts.base == TB_TYPEDEF && ts.tag &&
                                            tag_text_id != kInvalidText &&
                                            tag_text_id ==
                                                alias_param_text_id(ai)) {
                                            const bool outer_lref = ts.is_lvalue_ref;
                                            const bool outer_rref = ts.is_rvalue_ref;
                                            const bool outer_const = ts.is_const;
                                            const bool outer_volatile = ts.is_volatile;
                                            ts = resolved_alias_args[ai].type;
                                            ts.is_lvalue_ref = ts.is_lvalue_ref || outer_lref;
                                            ts.is_rvalue_ref =
                                                !ts.is_lvalue_ref &&
                                                (ts.is_rvalue_ref || outer_rref);
                                            ts.is_const = ts.is_const || outer_const;
                                            ts.is_volatile = ts.is_volatile || outer_volatile;
                                            break;
                                        }
                                    }
                                }
                                if (ts.tpl_struct_args.size > 0) {
                                    if (!substitute_template_arg_refs_structured(&ts)) {
                                        alias_parse_ok = false;
                                    } else {
                                        materialize_template_origin_record_def(
                                            &ts);
                                    }
                                    // Update tag (mangled name) to reflect substituted args.
                                    if (alias_parse_ok && ts.tpl_struct_origin) {
                                        const std::string new_refs =
                                            template_arg_refs_text(ts);
                                        std::string new_tag = ts.tpl_struct_origin;
                                        for (const auto& p2 :
                                             split_template_arg_refs(new_refs)) {
                                        new_tag += "_";
                                        new_tag += p2;
                                        }
                                        ts.tag = arena_.strdup(new_tag.c_str());
                                    }
                                }
                                if (!alias_parse_ok) {
                                    // alias_guard restores pos_ on scope exit
                                } else {
                                ts.is_const   |= save_const;
                                ts.is_volatile |= save_vol;
                                alias_guard.commit();
                                return ts;
                                }
                            }
                    }
                }
            } else if (is_cpp_mode() &&
                       find_template_struct_primary(current_namespace_context_id(),
                                                    active_context_state_
                                                        .last_resolved_typedef_text_id)) {
                // Qualified names such as `ns::Template<T>` may resolve directly
                // to a template primary without first passing through typedef
                // registration. Reuse the existing template-struct
                // instantiation/member-suffix path below.
                ts.base = TB_STRUCT;
            }
            if (is_cpp_mode() && ts.tag && ts.tag[0] &&
                check(TokenKind::ColonColon) &&
                core_input_state_.pos + 1 <
                    static_cast<int>(core_input_state_.tokens.size()) &&
                core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                    TokenKind::Identifier) {
                std::string member(
                    token_spelling(core_input_state_.tokens[core_input_state_.pos + 1]));
                const TextId member_text_id =
                    core_input_state_.tokens[core_input_state_.pos + 1].text_id;
                TypeSpec resolved{};
                if (lookup_struct_member_typedef_recursive_for_type(
                        ts, member, member_text_id, &resolved)) {
                    consume();  // ::
                    consume();  // member
                    bool save_const = ts.is_const, save_vol = ts.is_volatile;
                    ts = resolved;
                    ts.is_const |= save_const;
                    ts.is_volatile |= save_vol;
                    return ts;
                } else if (ts.tpl_struct_origin || (ts.tag && ts.tag[0])) {
                    consume();  // ::
                    consume();  // member
                    ts.deferred_member_type_name = arena_.strdup(member.c_str());
                    ts.deferred_member_type_text_id = member_text_id;
                    return ts;
                }
            }
            // Dependent template-template parameter application:
                // template<template<typename...> class Op> using X = Op<int>;
                // We only need to consume the argument list and keep the
                // dependent name as a placeholder type for later stages.
                if (is_cpp_mode() && tname &&
                    is_template_scope_type_param(tname_text_id) &&
                    check(TokenKind::Less)) {
                    std::vector<TemplateArgParseResult> ignored_args;
                    if (parse_template_argument_list(&ignored_args)) {
                        ts.base = TB_TYPEDEF;
                        ts.tag = tname;
                        ts.tag_text_id = tname_text_id;
                        ts.array_size = -1;
                        ts.array_rank = 0;
                        return ts;
                    }
                }
                // Template struct instantiation: Pair<int> or Array<int, 4> in type context.
                if (is_cpp_mode() && ts.base == TB_STRUCT && ts.tag &&
                    find_template_struct_primary(
                        current_namespace_context_id(),
                        parser_text_id_for_token(kInvalidText, ts.tag)) &&
                    check(TokenKind::Less)) {
                    std::string tpl_name = ts.tag;
                    const Node* primary_tpl = find_template_struct_primary(
                        current_namespace_context_id(),
                        parser_text_id_for_token(kInvalidText, tpl_name));
                    std::vector<ParsedTemplateArg> actual_args;
                    if (!parse_template_argument_list(&actual_args, primary_tpl)) return ts;
                    for (auto& arg : actual_args) {
                        if (arg.is_value) continue;
                        TypeSpec& arg_ts = arg.type;
                        const std::string deferred_member_name =
                            deferred_member_lookup_name(arg_ts);
                        if (deferred_member_name.empty() ||
                            !arg_ts.tag || !arg_ts.tag[0]) {
                            continue;
                        }
                        TypeSpec resolved_member{};
                        const TextId deferred_member_text_id =
                            arg_ts.deferred_member_type_text_id;
                        if (!lookup_struct_member_typedef_recursive_for_type(
                                arg_ts,
                                deferred_member_name,
                                deferred_member_text_id,
                                &resolved_member)) {
                            continue;
                        }
                        resolved_member.deferred_member_type_name = nullptr;
                        resolved_member.deferred_member_type_text_id = kInvalidText;
                        arg_ts = resolved_member;
                    }
                    bool has_pack_param = false;
                    if (primary_tpl && primary_tpl->template_param_is_pack) {
                        for (int pi = 0; pi < primary_tpl->n_template_params; ++pi) {
                            if (primary_tpl->template_param_is_pack[pi]) {
                                has_pack_param = true;
                                break;
                            }
                        }
                    }
                    if (primary_tpl && has_pack_param) {
                        std::string arg_refs;
                        for (const auto& arg : actual_args) {
                            if (!arg_refs.empty()) arg_refs += ",";
                            if (arg.is_value) {
                                if (arg.nttp_name && arg.nttp_name[0]) arg_refs += arg.nttp_name;
                                else arg_refs += std::to_string(arg.value);
                                } else if (arg.type.tpl_struct_origin) {
                                arg_refs += "@";
                                arg_refs += arg.type.tpl_struct_origin;
                                arg_refs += ":";
                                arg_refs += template_arg_refs_text(arg.type);
                                if (arg.type.deferred_member_type_name &&
                                    arg.type.deferred_member_type_name[0]) {
                                    arg_refs += "$";
                                    arg_refs += arg.type.deferred_member_type_name;
                                }
                            } else if (arg.type.tag) {
                                arg_refs += arg.type.tag;
                            } else {
                                std::string type_name;
                                append_type_mangled_suffix(type_name, arg.type);
                                arg_refs += type_name.empty() ? "?" : type_name;
                            }
                        }
                        ts.tpl_struct_origin = arena_.strdup(tpl_name.c_str());
                        ts.tpl_struct_origin_key =
                            template_instantiation_name_key_for_direct_emit(
                                *this, primary_tpl, tpl_name);
                        if (parsed_args_have_simple_structured_carrier(actual_args))
                            set_template_arg_refs_from_parsed_args(&ts, actual_args);
                        else
                            set_template_arg_debug_refs_text(&ts, arg_refs);
                        ts.tag = arena_.strdup(tpl_name.c_str());
                        return ts;
                    }
                    // Fill in deferred NTTP defaults before pattern selection
                    // so specialization matching has the complete arg list.
                    if (primary_tpl) {
                        const QualifiedNameKey primary_template_key =
                            template_instantiation_name_key_for_direct_emit(
                                *this, primary_tpl, tpl_name);
                        // Build preliminary type bindings from explicit args
                        std::vector<std::pair<std::string, TypeSpec>> prelim_tb;
                        std::vector<std::pair<std::string, long long>> prelim_nb;
                        std::vector<ParserNttpBindingMetadata> prelim_nb_meta;
                        auto prelim_type_for_type =
                            [&](TypeSpec type) -> TypeSpec {
                            if (type.base == TB_TYPEDEF && type.tag &&
                                type.tag[0]) {
                                for (const auto& [pname, pts] : prelim_tb) {
                                    if (pname == type.tag) return pts;
                                }
                            }
                            return type;
                        };
                        auto prelim_nttp_value_for_ref =
                            [&](const Node* node, long long* out) -> bool {
                            if (!node || !out) return false;
                            const TextId ref_text_id =
                                node->unqualified_text_id;
                            if (ref_text_id != kInvalidText) {
                                for (const ParserNttpBindingMetadata& meta :
                                     prelim_nb_meta) {
                                    if (meta.name_text_id == ref_text_id ||
                                        meta.name_key.base_text_id ==
                                            ref_text_id) {
                                        *out = meta.value;
                                        return true;
                                    }
                                }
                                return false;
                            }
                            if (!node->name) return false;
                            for (const auto& [pname, value] : prelim_nb) {
                                if (pname == node->name) {
                                    *out = value;
                                    return true;
                                }
                            }
                            return false;
                        };
                        auto prelim_type_is_unsigned_integer =
                            [&](TypeSpec type) -> bool {
                            type = prelim_type_for_type(type);
                            switch (type.base) {
                                case TB_UCHAR:
                                case TB_USHORT:
                                case TB_UINT:
                                case TB_ULONG:
                                case TB_ULONGLONG:
                                case TB_UINT128:
                                    return true;
                                default:
                                    return false;
                            }
                        };
                        auto prelim_functional_cast_type =
                            [&](const Node* node, TypeSpec* out_type) -> bool {
                            if (!node || node->kind != NK_CALL || !node->left ||
                                node->left->kind != NK_VAR ||
                                !node->left->name || !out_type) {
                                return false;
                            }
                            TypeSpec candidate{};
                            candidate.base = TB_TYPEDEF;
                            candidate.tag = node->left->name;
                            candidate.tag_text_id =
                                node->left->unqualified_text_id;
                            candidate.array_size = -1;
                            candidate.inner_rank = -1;
                            *out_type = prelim_type_for_type(candidate);
                            return out_type->base != TB_TYPEDEF ||
                                   out_type->record_def ||
                                   out_type->tag != candidate.tag;
                        };
                        std::function<bool(const Node*)>
                            prelim_expr_has_unsigned_type =
                                [&](const Node* node) -> bool {
                            if (!node) return false;
                            if (node->kind == NK_CAST ||
                                node->kind == NK_SIZEOF_TYPE) {
                                return prelim_type_is_unsigned_integer(
                                    node->type);
                            }
                            TypeSpec cast_type{};
                            if (prelim_functional_cast_type(node,
                                                            &cast_type)) {
                                return prelim_type_is_unsigned_integer(
                                    cast_type);
                            }
                            return false;
                        };
                        std::function<bool(const Node*, long long*)>
                            eval_prelim_structured_nttp_expr =
                                [&](const Node* node,
                                    long long* out) -> bool {
                            if (!node || !out) return false;
                            if (node->kind == NK_INT_LIT ||
                                node->kind == NK_CHAR_LIT) {
                                *out = node->ival;
                                return true;
                            }
                            if (node->kind == NK_VAR)
                                return prelim_nttp_value_for_ref(node, out);
                            if (node->kind == NK_CAST && node->left)
                                return eval_prelim_structured_nttp_expr(
                                    node->left, out);
                            if (node->kind == NK_CALL &&
                                node->n_children == 1 && node->children) {
                                TypeSpec cast_type{};
                                if (prelim_functional_cast_type(node,
                                                                &cast_type)) {
                                    return eval_prelim_structured_nttp_expr(
                                        node->children[0], out);
                                }
                            }
                            if (node->kind == NK_SIZEOF_TYPE) {
                                const TypeSpec sized =
                                    prelim_type_for_type(node->type);
                                if (sized.base == TB_TYPEDEF && sized.tag)
                                    return false;
                                *out = sizeof_type_spec(sized);
                                return true;
                            }
                            if (node->kind == NK_UNARY && node->op &&
                                node->left) {
                                long long v = 0;
                                if (!eval_prelim_structured_nttp_expr(
                                        node->left, &v)) {
                                    return false;
                                }
                                if (std::strcmp(node->op, "-") == 0) {
                                    *out = -v;
                                    return true;
                                }
                                if (std::strcmp(node->op, "+") == 0) {
                                    *out = v;
                                    return true;
                                }
                                if (std::strcmp(node->op, "!") == 0) {
                                    *out = !v;
                                    return true;
                                }
                                if (std::strcmp(node->op, "~") == 0) {
                                    *out = ~v;
                                    return true;
                                }
                                return false;
                            }
                            if (node->kind == NK_BINOP && node->op &&
                                node->left && node->right) {
                                long long l = 0;
                                long long r = 0;
                                if (!eval_prelim_structured_nttp_expr(
                                        node->left, &l) ||
                                    !eval_prelim_structured_nttp_expr(
                                        node->right, &r)) {
                                    return false;
                                }
                                if (std::strcmp(node->op, "+") == 0) {
                                    *out = l + r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "-") == 0) {
                                    *out = l - r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "*") == 0) {
                                    *out = l * r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "/") == 0 &&
                                    r != 0) {
                                    *out = l / r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "%") == 0 &&
                                    r != 0) {
                                    *out = l % r;
                                    return true;
                                }
                                const bool compare_unsigned =
                                    prelim_expr_has_unsigned_type(node->left) ||
                                    prelim_expr_has_unsigned_type(node->right);
                                if (std::strcmp(node->op, "<") == 0) {
                                    *out = compare_unsigned
                                               ? static_cast<unsigned long long>(
                                                     l) <
                                                     static_cast<
                                                         unsigned long long>(r)
                                               : l < r;
                                    return true;
                                }
                                if (std::strcmp(node->op, ">") == 0) {
                                    *out = compare_unsigned
                                               ? static_cast<unsigned long long>(
                                                     l) >
                                                     static_cast<
                                                         unsigned long long>(r)
                                               : l > r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "<=") == 0) {
                                    *out = compare_unsigned
                                               ? static_cast<unsigned long long>(
                                                     l) <=
                                                     static_cast<
                                                         unsigned long long>(r)
                                               : l <= r;
                                    return true;
                                }
                                if (std::strcmp(node->op, ">=") == 0) {
                                    *out = compare_unsigned
                                               ? static_cast<unsigned long long>(
                                                     l) >=
                                                     static_cast<
                                                         unsigned long long>(r)
                                               : l >= r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "==") == 0) {
                                    *out = l == r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "!=") == 0) {
                                    *out = l != r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "&&") == 0) {
                                    *out = (l && r) ? 1 : 0;
                                    return true;
                                }
                                if (std::strcmp(node->op, "||") == 0) {
                                    *out = (l || r) ? 1 : 0;
                                    return true;
                                }
                                if (std::strcmp(node->op, "&") == 0) {
                                    *out = l & r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "|") == 0) {
                                    *out = l | r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "^") == 0) {
                                    *out = l ^ r;
                                    return true;
                                }
                                if (std::strcmp(node->op, "<<") == 0) {
                                    *out = l << r;
                                    return true;
                                }
                                if (std::strcmp(node->op, ">>") == 0) {
                                    *out = l >> r;
                                    return true;
                                }
                                return false;
                            }
                            if (node->kind == NK_TERNARY && node->cond &&
                                node->then_ && node->else_) {
                                long long c = 0;
                                if (!eval_prelim_structured_nttp_expr(
                                        node->cond, &c)) {
                                    return false;
                                }
                                return eval_prelim_structured_nttp_expr(
                                    c ? node->then_ : node->else_, out);
                            }
                            return false;
                        };
                        for (int pi = 0; pi < static_cast<int>(actual_args.size()) &&
                                         pi < primary_tpl->n_template_params; ++pi) {
                            const char* pn = primary_tpl->template_param_names[pi];
                            if (primary_tpl->template_param_is_nttp[pi]) {
                                if (actual_args[pi].is_value) {
                                    long long ev = 0;
                                    Node* structured_expr =
                                        structured_nttp_expr_carrier(
                                            actual_args[pi]);
                                    if (structured_expr &&
                                        eval_prelim_structured_nttp_expr(
                                            structured_expr, &ev)) {
                                        actual_args[pi].value = ev;
                                        actual_args[pi].expr = nullptr;
                                        actual_args[pi].type.array_size_expr =
                                            nullptr;
                                        actual_args[pi].nttp_name = nullptr;
                                    } else if (actual_args[pi].nttp_text_id !=
                                               kInvalidText) {
                                        Node ref{};
                                        ref.kind = NK_VAR;
                                        ref.unqualified_text_id =
                                            actual_args[pi].nttp_text_id;
                                        if (prelim_nttp_value_for_ref(&ref,
                                                                     &ev)) {
                                            actual_args[pi].value = ev;
                                            actual_args[pi].nttp_name =
                                                nullptr;
                                        }
                                    } else if (!parsed_nttp_arg_has_structured_carrier(
                                                   actual_args[pi]) &&
                                               actual_args[pi].nttp_name &&
                                               std::strncmp(
                                                   actual_args[pi].nttp_name,
                                                   "$expr:", 6) == 0) {
                                        std::vector<Token> expr_toks = lex_template_expr_text(
                                            actual_args[pi].nttp_name + 6,
                                            core_input_state_.source_profile);
                                        if (eval_deferred_nttp_expr_tokens(
                                                tpl_name, expr_toks,
                                                prelim_tb, prelim_nb, &ev,
                                                &prelim_nb_meta)) {
                                            actual_args[pi].value = ev;
                                            actual_args[pi].nttp_name =
                                                nullptr;
                                        }
                                    }
                                    prelim_nb.push_back({pn, actual_args[pi].value});
                                    prelim_nb_meta.push_back(
                                        nttp_binding_metadata_for_template_param(
                                            *this, primary_tpl, pi, pn,
                                            actual_args[pi].value));
                                }
                            } else {
                                if (!actual_args[pi].is_value)
                                    prelim_tb.push_back({pn, actual_args[pi].type});
                            }
                        }
                        // Evaluate deferred defaults for missing args
                        int sz = static_cast<int>(actual_args.size());
                        while (sz < primary_tpl->n_template_params) {
                            if (!primary_tpl->template_param_has_default ||
                                !primary_tpl->template_param_has_default[sz]) break;
                            ParsedTemplateArg da;
                            da.is_value = primary_tpl->template_param_is_nttp[sz];
                            if (da.is_value && primary_tpl->template_param_default_values[sz] == LLONG_MIN) {
                                long long ev = 0;
                                if (eval_deferred_nttp_default(
                                        primary_template_key, sz,
                                        prelim_tb, prelim_nb, &ev,
                                        &prelim_nb_meta)) {
                                    da.value = ev;
                                    actual_args.push_back(da);
                                } else {
                                    // Missing structured default tokens leave
                                    // this instantiation deferred; do not
                                    // re-lex the compatibility display text.
                                    break;
                                }
                            } else if (da.is_value) {
                                da.value = primary_tpl->template_param_default_values[sz];
                                actual_args.push_back(da);
                            } else {
                                da.type = primary_tpl->template_param_default_types[sz];
                                actual_args.push_back(da);
                            }
                            if (da.is_value) {
                                prelim_nb.push_back(
                                    {primary_tpl->template_param_names[sz],
                                     da.value});
                                prelim_nb_meta.push_back(
                                    nttp_binding_metadata_for_template_param(
                                        *this, primary_tpl, sz,
                                        primary_tpl->template_param_names[sz],
                                        da.value));
                            } else if (!da.is_value) {
                                prelim_tb.push_back(
                                    {primary_tpl->template_param_names[sz],
                                     da.type});
                            }
                            ++sz;
                        }
                    }
                    std::vector<std::pair<std::string, TypeSpec>> type_bindings;
                    std::vector<std::pair<std::string, long long>> nttp_bindings;
                    const auto* specialization_patterns =
                        find_template_struct_specializations(primary_tpl);
                    const Node* tpl_def = select_template_struct_pattern_for_args(
                        actual_args, primary_tpl, specialization_patterns,
                        &type_bindings, &nttp_bindings);
                    if (!tpl_def) return ts;
                    std::vector<ParserNttpBindingMetadata> nttp_binding_meta =
                        nttp_binding_metadata_for_template_params(
                            *this, primary_tpl, nttp_bindings);
                    std::vector<ParsedTemplateArg> concrete_args = actual_args;
                    while (primary_tpl && static_cast<int>(concrete_args.size()) < primary_tpl->n_template_params) {
                        int i = static_cast<int>(concrete_args.size());
                        if (!primary_tpl->template_param_has_default[i]) break;
                        ParsedTemplateArg arg;
                        arg.is_value = primary_tpl->template_param_is_nttp[i];
                        if (arg.is_value) {
                            if (primary_tpl->template_param_default_values[i] == LLONG_MIN) {
                                // Deferred NTTP default — evaluate with current bindings
                                long long eval_val = 0;
                                const QualifiedNameKey primary_template_key =
                                    template_instantiation_name_key_for_direct_emit(
                                        *this, primary_tpl, tpl_name);
                                if (eval_deferred_nttp_default(
                                        primary_template_key, i,
                                        type_bindings, nttp_bindings,
                                        &eval_val, &nttp_binding_meta)) {
                                    arg.value = eval_val;
                                } else {
                                    // Missing structured default tokens leave
                                    // this instantiation deferred; do not
                                    // re-lex the compatibility display text.
                                    break;
                                }
                            } else {
                                arg.value = primary_tpl->template_param_default_values[i];
                            }
                        } else {
                            arg.type = primary_tpl->template_param_default_types[i];
                        }
                        concrete_args.push_back(arg);
                        if (arg.is_value) {
                            nttp_bindings.push_back(
                                {primary_tpl->template_param_names[i],
                                 arg.value});
                            nttp_binding_meta.push_back(
                                nttp_binding_metadata_for_template_param(
                                    *this, primary_tpl, i,
                                    primary_tpl->template_param_names[i],
                                    arg.value));
                        } else {
                            type_bindings.push_back(
                                {primary_tpl->template_param_names[i],
                                 arg.type});
                        }
                    }
                    const ParserTemplateState::TemplateInstantiationKey
                        structured_emitted_instance_key{
                            template_instantiation_name_key_for_direct_emit(
                                *this, primary_tpl, tpl_name),
                            make_template_instantiation_argument_keys(
                                concrete_args)};

                    // Build mangled name from the concrete family arguments.
                    const std::string family_name =
                        tpl_def->template_origin_name ? tpl_def->template_origin_name : tpl_name;
                    std::string mangled = build_template_struct_mangled_name(
                        tpl_name, primary_tpl, tpl_def, concrete_args);
                    const size_t mangled_sep = mangled.rfind("::");
                    const std::string mangled_unqualified =
                        mangled_sep == std::string::npos
                            ? mangled
                            : mangled.substr(mangled_sep + 2);
                    const TextId mangled_text_id =
                        parser_text_id_for_token(kInvalidText,
                                                 mangled_unqualified);
                    const int mangled_namespace_context =
                        primary_tpl && primary_tpl->namespace_context_id >= 0
                            ? primary_tpl->namespace_context_id
                            : current_namespace_context_id();
                    // Check if any type arg is an unresolved template param
                    // or a pending template struct (e.g. Pair<T> inside Box<Pair<T>>).
                    // If so, defer instantiation to HIR template function lowering.
                    bool has_unresolved_type_arg = false;
                    for (const auto& [pn, pts] : type_bindings) {
                        if (pts.base == TB_TYPEDEF || pts.tpl_struct_origin) {
                            has_unresolved_type_arg = true;
                            break;
                        }
                    }
                    bool has_deferred_nttp_arg = false;
                    for (const auto& arg : concrete_args) {
                        if (arg.is_value && arg.nttp_name && arg.nttp_name[0]) {
                            has_deferred_nttp_arg = true;
                            break;
                        }
                    }
                    // Also check: if concrete_args is incomplete (fewer than
                    // primary_tpl->n_template_params) because a deferred NTTP
                    // default could not be evaluated, treat as deferred.
                    bool has_incomplete_nttp_default = false;
                    if (primary_tpl &&
                        static_cast<int>(concrete_args.size()) < primary_tpl->n_template_params) {
                        has_incomplete_nttp_default = true;
                    }

                    if (has_unresolved_type_arg || has_deferred_nttp_arg || has_incomplete_nttp_default) {
                        // Build arg_refs: comma-separated list of arg values
                        // in template param order. Type args use the TypeSpec tag
                        // (e.g., "T"), NTTP args use the numeric value.
                        std::string arg_refs;
                        int ati = 0, ani = 0;
                        for (int pi = 0; pi < tpl_def->n_template_params; ++pi) {
                            if (tpl_def->template_param_is_nttp[pi]) {
                                const bool have_arg = pi < static_cast<int>(concrete_args.size());
                                const ParsedTemplateArg* carg =
                                    have_arg ? &concrete_args[pi] : nullptr;
                                bool have_value = ani < (int)nttp_bindings.size();
                                long long bound_value = have_value ? nttp_bindings[ani].second : 0;
                                const bool missing_default =
                                    (!have_arg &&
                                     (!have_value ||
                                      (bound_value == LLONG_MIN &&
                                       tpl_def->template_param_has_default &&
                                       tpl_def->template_param_has_default[pi])));
                                if (missing_default) break;
                                if (!arg_refs.empty()) arg_refs += ",";
                                if (carg && carg->nttp_name && carg->nttp_name[0]) {
                                    arg_refs += carg->nttp_name;
                                } else {
                                    arg_refs += std::to_string(bound_value);
                                }
                                ++ani;
                            } else {
                                if (ati < (int)type_bindings.size()) {
                                    if (!arg_refs.empty()) arg_refs += ",";
                                    const TypeSpec& ats = type_bindings[ati++].second;
                                    if (ats.tpl_struct_origin) {
                                        // Nested pending template struct: encode as @origin:args
                                        arg_refs += "@";
                                        arg_refs += ats.tpl_struct_origin;
                                        arg_refs += ":";
                                        arg_refs += template_arg_refs_text(ats);
                                        if (ats.deferred_member_type_name &&
                                            ats.deferred_member_type_name[0]) {
                                            arg_refs += "$";
                                            arg_refs += ats.deferred_member_type_name;
                                        }
                                    } else if (ats.tag) {
                                        arg_refs += ats.tag;
                                    } else {
                                        // Builtin type (bool, int, etc.) — encode as mangled name.
                                        std::string type_name;
                                        append_type_mangled_suffix(type_name, ats);
                                        arg_refs += type_name.empty() ? "?" : type_name;
                                    }
                                } else {
                                    if (!arg_refs.empty()) arg_refs += ",";
                                    arg_refs += "?";
                                }
                            }
                        }
                        ts.tpl_struct_origin = arena_.strdup(family_name.c_str());
                        ts.tpl_struct_origin_key =
                            structured_emitted_instance_key.template_key;
                        if (parsed_args_have_simple_structured_carrier(concrete_args))
                            set_template_arg_refs_from_parsed_args(&ts, concrete_args);
                        else
                            set_template_arg_debug_refs_text(&ts, arg_refs);
                        ts.tag = arena_.strdup(mangled.c_str());
                        ts.tag_text_id = mangled_text_id;
                        ts.namespace_context_id = mangled_namespace_context;
                        return ts;
                    }

                    Node* instantiated_record = nullptr;
                    if (!has_template_instantiation_dedup_key_for_direct_emit(
                            *this, structured_emitted_instance_key)) {
                        mark_template_instantiation_dedup_key_for_direct_emit(
                            *this, structured_emitted_instance_key);
                        // Create a concrete NK_STRUCT_DEF with substituted field types
                        Node* inst = make_node(NK_STRUCT_DEF, tpl_def->line);
                        inst->name = arena_.strdup(mangled.c_str());
                        inst->unqualified_name =
                            arena_.strdup(mangled_unqualified.c_str());
                        inst->unqualified_text_id = mangled_text_id;
                        inst->namespace_context_id = mangled_namespace_context;
                        inst->template_origin_name = arena_.strdup(family_name.c_str());
                        inst->is_union = tpl_def->is_union;
                        inst->pack_align = tpl_def->pack_align;
                        inst->struct_align = tpl_def->struct_align;
                        inst->n_bases = tpl_def->n_bases;
                        if (inst->n_bases > 0) {
                            inst->base_types = arena_.alloc_array<TypeSpec>(inst->n_bases);
                            for (int bi = 0; bi < inst->n_bases; ++bi) {
                                inst->base_types[bi] = tpl_def->base_types[bi];
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (inst->base_types[bi].base == TB_TYPEDEF &&
                                        inst->base_types[bi].tag &&
                                        std::string(inst->base_types[bi].tag) == pname) {
                                        inst->base_types[bi].base = pts.base;
                                        inst->base_types[bi].tag = pts.tag;
                                        inst->base_types[bi].record_def = pts.record_def;
                                        inst->base_types[bi].ptr_level += pts.ptr_level;
                                        inst->base_types[bi].is_const |= pts.is_const;
                                        inst->base_types[bi].is_volatile |= pts.is_volatile;
                                    }
                                }
                                // Resolve pending template base types: e.g. is_const<T> → is_const<const int>
                                if (inst->base_types[bi].tpl_struct_origin) {
                                    std::string origin =
                                        inst->base_types[bi].tpl_struct_origin;
                                    auto find_instantiated_base_primary =
                                        [&]() -> const Node* {
                                        if (inst->base_types[bi]
                                                .tpl_struct_origin_key
                                                .base_text_id != kInvalidText) {
                                            return find_template_struct_primary(
                                                inst->base_types[bi]
                                                    .tpl_struct_origin_key);
                                        }
                                        return nullptr;
                                    };
                                    auto type_mentions_bound_param =
                                        [&](const TypeSpec& candidate) -> bool {
                                            if (candidate.tag && candidate.tag[0] &&
                                                (candidate.base == TB_TYPEDEF ||
                                                 (!candidate.record_def &&
                                                  !candidate.tpl_struct_origin))) {
                                                for (const auto& [pname2, _] : type_bindings) {
                                                    if (std::string(candidate.tag) == pname2) {
                                                        return true;
                                                    }
                                                }
                                            }
                                            return false;
                                    };
                                    auto bound_type_for_type = [&](TypeSpec type) {
                                        if (type.base == TB_TYPEDEF &&
                                            type.tag && type.tag[0]) {
                                            for (const auto& [pname2, pts] :
                                                 type_bindings) {
                                                if (std::string(type.tag) == pname2) {
                                                    return pts;
                                                }
                                            }
                                        }
                                        return type;
                                    };
                                    auto bound_type_for_param_name =
                                        [&](const char* param_name,
                                            TypeSpec* out) -> bool {
                                        if (!param_name || !param_name[0] || !out)
                                            return false;
                                        for (const auto& [pname2, pts] :
                                             type_bindings) {
                                            if (pname2 == param_name) {
                                                *out = pts;
                                                return true;
                                            }
                                        }
                                        if (!primary_tpl ||
                                            !primary_tpl->template_param_names) {
                                            return false;
                                        }
                                        for (int pi = 0;
                                             pi < primary_tpl->n_template_params &&
                                             pi < static_cast<int>(
                                                      concrete_args.size());
                                             ++pi) {
                                            if (primary_tpl->template_param_is_nttp &&
                                                primary_tpl
                                                    ->template_param_is_nttp[pi]) {
                                                continue;
                                            }
                                            const char* outer_name =
                                                primary_tpl
                                                    ->template_param_names[pi];
                                            if (!outer_name ||
                                                std::strcmp(param_name,
                                                            outer_name) != 0 ||
                                                concrete_args[pi].is_value) {
                                                continue;
                                            }
                                            *out = concrete_args[pi].type;
                                            return true;
                                        }
                                        return false;
                                    };
                                    auto substitute_bound_type_arg =
                                        [&](TypeSpec type) -> TypeSpec {
                                        if (!(type.tag && type.tag[0] &&
                                              (type.base == TB_TYPEDEF ||
                                               (!type.record_def &&
                                                !type.tpl_struct_origin)))) {
                                            return type;
                                        }
                                        TypeSpec bound_type{};
                                        if (bound_type_for_param_name(
                                                type.tag, &bound_type)) {
                                            const int outer_ptr_level =
                                                type.ptr_level;
                                            const bool outer_lref =
                                                type.is_lvalue_ref;
                                            const bool outer_rref =
                                                type.is_rvalue_ref;
                                            const bool outer_const =
                                                type.is_const;
                                            const bool outer_volatile =
                                                type.is_volatile;
                                            type = bound_type;
                                            type.ptr_level += outer_ptr_level;
                                            type.is_lvalue_ref =
                                                type.is_lvalue_ref || outer_lref;
                                            type.is_rvalue_ref =
                                                !type.is_lvalue_ref &&
                                                (type.is_rvalue_ref || outer_rref);
                                            type.is_const =
                                                type.is_const || outer_const;
                                            type.is_volatile =
                                                type.is_volatile || outer_volatile;
                                            return type;
                                        }
                                        return type;
                                    };
                                    auto nttp_value_for_ref =
                                        [&](const Node* node, long long* out)
                                        -> bool {
                                        if (!node || !out) return false;
                                        const TextId ref_text_id =
                                            node->unqualified_text_id;
                                        if (ref_text_id != kInvalidText) {
                                            for (const auto& meta :
                                                 nttp_binding_meta) {
                                                if (meta.name_text_id ==
                                                    ref_text_id) {
                                                    *out = meta.value;
                                                    return true;
                                                }
                                            }
                                            return false;
                                        }
                                        if (!node->name) return false;
                                        for (const auto& [pname2, value] :
                                             nttp_bindings) {
                                            if (std::string(node->name) ==
                                                pname2) {
                                                *out = value;
                                                return true;
                                            }
                                        }
                                        return false;
                                    };
                                    auto type_is_unsigned_integer =
                                        [&](TypeSpec type) -> bool {
                                        type = bound_type_for_type(type);
                                        switch (type.base) {
                                            case TB_UCHAR:
                                            case TB_USHORT:
                                            case TB_UINT:
                                            case TB_ULONG:
                                            case TB_ULONGLONG:
                                            case TB_UINT128:
                                                return true;
                                            default:
                                                return false;
                                        }
                                    };
                                    auto functional_cast_type =
                                        [&](const Node* node,
                                            TypeSpec* out_type) -> bool {
                                        if (!node || node->kind != NK_CALL ||
                                            !node->left ||
                                            node->left->kind != NK_VAR ||
                                            !node->left->name || !out_type) {
                                            return false;
                                        }
                                        TypeSpec candidate{};
                                        candidate.base = TB_TYPEDEF;
                                        candidate.tag = node->left->name;
                                        candidate.tag_text_id =
                                            node->left->unqualified_text_id;
                                        candidate.array_size = -1;
                                        candidate.inner_rank = -1;
                                        *out_type = bound_type_for_type(candidate);
                                        return out_type->base != TB_TYPEDEF ||
                                               out_type->record_def ||
                                               out_type->tag != candidate.tag;
                                    };
                                    std::function<bool(const Node*)>
                                        expr_has_unsigned_type =
                                            [&](const Node* node) -> bool {
                                        if (!node) return false;
                                        if (node->kind == NK_CAST ||
                                            node->kind == NK_SIZEOF_TYPE) {
                                            return type_is_unsigned_integer(node->type);
                                        }
                                        TypeSpec cast_type{};
                                        if (functional_cast_type(node,
                                                                 &cast_type)) {
                                            return type_is_unsigned_integer(
                                                cast_type);
                                        }
                                        return false;
                                    };
                                    std::function<bool(const Node*, long long*)>
                                        eval_structured_nttp_expr =
                                            [&](const Node* node,
                                                long long* out) -> bool {
                                        if (!node || !out) return false;
                                        if (node->kind == NK_INT_LIT ||
                                            node->kind == NK_CHAR_LIT) {
                                            *out = node->ival;
                                            return true;
                                        }
                                        if (node->kind == NK_VAR) {
                                            return nttp_value_for_ref(node, out);
                                        }
                                        if (node->kind == NK_CAST && node->left) {
                                            return eval_structured_nttp_expr(
                                                node->left, out);
                                        }
                                        if (node->kind == NK_CALL &&
                                            node->n_children == 1 &&
                                            node->children) {
                                            TypeSpec cast_type{};
                                            if (functional_cast_type(node,
                                                                     &cast_type)) {
                                                return eval_structured_nttp_expr(
                                                    node->children[0], out);
                                            }
                                        }
                                        if (node->kind == NK_SIZEOF_TYPE) {
                                            const TypeSpec sized =
                                                bound_type_for_type(node->type);
                                            if (sized.base == TB_TYPEDEF &&
                                                sized.tag) {
                                                return false;
                                            }
                                            *out = sizeof_type_spec(sized);
                                            return true;
                                        }
                                        if (node->kind == NK_UNARY && node->op &&
                                            node->left) {
                                            long long v = 0;
                                            if (!eval_structured_nttp_expr(
                                                    node->left, &v)) {
                                                return false;
                                            }
                                            if (std::strcmp(node->op, "-") == 0) {
                                                *out = -v;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "+") == 0) {
                                                *out = v;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "!") == 0) {
                                                *out = !v;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "~") == 0) {
                                                *out = ~v;
                                                return true;
                                            }
                                            return false;
                                        }
                                        if (node->kind == NK_BINOP && node->op &&
                                            node->left && node->right) {
                                            long long l = 0;
                                            long long r = 0;
                                            if (!eval_structured_nttp_expr(
                                                    node->left, &l) ||
                                                !eval_structured_nttp_expr(
                                                    node->right, &r)) {
                                                return false;
                                            }
                                            if (std::strcmp(node->op, "+") == 0) {
                                                *out = l + r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "-") == 0) {
                                                *out = l - r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "*") == 0) {
                                                *out = l * r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "/") == 0 &&
                                                r != 0) {
                                                *out = l / r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "%") == 0 &&
                                                r != 0) {
                                                *out = l % r;
                                                return true;
                                            }
                                            const bool compare_unsigned =
                                                expr_has_unsigned_type(node->left) ||
                                                expr_has_unsigned_type(node->right);
                                            if (std::strcmp(node->op, "<") == 0) {
                                                *out = compare_unsigned
                                                           ? static_cast<unsigned long long>(l) <
                                                                 static_cast<unsigned long long>(r)
                                                           : l < r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, ">") == 0) {
                                                *out = compare_unsigned
                                                           ? static_cast<unsigned long long>(l) >
                                                                 static_cast<unsigned long long>(r)
                                                           : l > r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "<=") == 0) {
                                                *out = compare_unsigned
                                                           ? static_cast<unsigned long long>(l) <=
                                                                 static_cast<unsigned long long>(r)
                                                           : l <= r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, ">=") == 0) {
                                                *out = compare_unsigned
                                                           ? static_cast<unsigned long long>(l) >=
                                                                 static_cast<unsigned long long>(r)
                                                           : l >= r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "==") == 0) {
                                                *out = l == r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "!=") == 0) {
                                                *out = l != r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "&&") == 0) {
                                                *out = (l && r) ? 1 : 0;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "||") == 0) {
                                                *out = (l || r) ? 1 : 0;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "&") == 0) {
                                                *out = l & r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "|") == 0) {
                                                *out = l | r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "^") == 0) {
                                                *out = l ^ r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, "<<") == 0) {
                                                *out = l << r;
                                                return true;
                                            }
                                            if (std::strcmp(node->op, ">>") == 0) {
                                                *out = l >> r;
                                                return true;
                                            }
                                            return false;
                                        }
                                        if (node->kind == NK_TERNARY &&
                                            node->cond && node->then_ &&
                                            node->else_) {
                                            long long c = 0;
                                            if (!eval_structured_nttp_expr(
                                                    node->cond, &c)) {
                                                return false;
                                            }
                                            return eval_structured_nttp_expr(
                                                c ? node->then_ : node->else_,
                                                out);
                                        }
                                        return false;
                                    };
                                    const Node* base_primary =
                                        find_instantiated_base_primary();
                                    if (base_primary) {
                                        if (base_primary->template_origin_name &&
                                            base_primary
                                                ->template_origin_name[0]) {
                                            origin =
                                                base_primary->template_origin_name;
                                        } else if (base_primary
                                                       ->unqualified_name &&
                                                   base_primary
                                                       ->unqualified_name[0]) {
                                            origin =
                                                base_primary->unqualified_name;
                                        } else if (base_primary->name &&
                                                   base_primary->name[0]) {
                                            const char* unqualified =
                                                std::strrchr(base_primary->name,
                                                            ':');
                                            origin = unqualified
                                                         ? unqualified + 1
                                                         : base_primary->name;
                                        }
                                    }
                                    const char* saved_deferred_member_name =
                                        inst->base_types[bi]
                                            .deferred_member_type_name;
                                    const TextId saved_deferred_member_text_id =
                                        inst->base_types[bi]
                                            .deferred_member_type_text_id;
                                    auto restore_deferred_member_lookup =
                                        [&]() {
                                        if (saved_deferred_member_name &&
                                            saved_deferred_member_name[0]) {
                                            inst->base_types[bi]
                                                .deferred_member_type_name =
                                                saved_deferred_member_name;
                                        }
                                        if (saved_deferred_member_text_id !=
                                            kInvalidText) {
                                            inst->base_types[bi]
                                                .deferred_member_type_text_id =
                                                saved_deferred_member_text_id;
                                        }
                                    };
                                    auto resolve_deferred_member_base =
                                        [&]() -> bool {
                                        restore_deferred_member_lookup();
                                        const std::string deferred_member_name =
                                            deferred_member_lookup_name(
                                                inst->base_types[bi]);
                                        if (deferred_member_name.empty() ||
                                            !inst->base_types[bi].tag ||
                                            !inst->base_types[bi].tag[0]) {
                                            return false;
                                        }
                                        TypeSpec resolved_member{};
                                        const TextId deferred_member_text_id =
                                            inst->base_types[bi]
                                                .deferred_member_type_text_id;
                                        if (!lookup_struct_member_typedef_recursive_for_type(
                                                inst->base_types[bi],
                                                deferred_member_name,
                                                deferred_member_text_id,
                                                &resolved_member)) {
                                            return false;
                                        }
                                        resolved_member.deferred_member_type_name =
                                            nullptr;
                                        resolved_member.deferred_member_type_text_id =
                                            kInvalidText;
                                        if (resolved_member.tpl_struct_origin &&
                                            !resolved_member.record_def &&
                                            resolved_member.tpl_struct_args.data &&
                                            resolved_member.tpl_struct_args.size > 0) {
                                            const Node* member_primary = nullptr;
                                            if (resolved_member
                                                    .tpl_struct_origin_key
                                                    .base_text_id !=
                                                kInvalidText) {
                                                member_primary =
                                                    find_template_struct_primary(
                                                        resolved_member
                                                            .tpl_struct_origin_key);
                                            }
                                            if (member_primary) {
                                                std::vector<ParsedTemplateArg>
                                                    member_args;
                                                bool can_use_member_args = true;
                                                for (int ai = 0;
                                                     ai < resolved_member
                                                              .tpl_struct_args
                                                              .size;
                                                     ++ai) {
                                                    const TemplateArgRef& src_arg =
                                                        resolved_member
                                                            .tpl_struct_args
                                                            .data[ai];
                                                    ParsedTemplateArg member_arg{};
                                                    if (src_arg.kind ==
                                                        TemplateArgKind::Value) {
                                                        if (zero_value_arg_ref_uses_debug_fallback(
                                                                src_arg)) {
                                                            can_use_member_args =
                                                                false;
                                                            break;
                                                        }
                                                        member_arg.is_value = true;
                                                        member_arg.value =
                                                            src_arg.value;
                                                        member_arg.expr =
                                                            src_arg.type
                                                                .array_size_expr;
                                                        if (member_arg.expr) {
                                                            long long expr_value = 0;
                                                            if (!eval_structured_nttp_expr(
                                                                    member_arg.expr,
                                                                    &expr_value)) {
                                                                can_use_member_args =
                                                                    false;
                                                                break;
                                                            }
                                                            member_arg.value =
                                                                expr_value;
                                                            member_arg.expr =
                                                                nullptr;
                                                        }
                                                    } else {
                                                        if (unstructured_type_arg_ref_uses_debug_fallback(
                                                                src_arg)) {
                                                            can_use_member_args =
                                                                false;
                                                            break;
                                                        }
                                                        member_arg.is_value = false;
                                                        member_arg.type =
                                                            substitute_bound_type_arg(
                                                                src_arg.type);
                                                        if (type_mentions_bound_param(
                                                                member_arg.type)) {
                                                            can_use_member_args =
                                                                false;
                                                            break;
                                                        }
                                                    }
                                                    member_args.push_back(
                                                        member_arg);
                                                }
                                                if (can_use_member_args) {
                                                    std::string member_mangled;
                                                    if (ensure_template_struct_instantiated_from_args(
                                                            resolved_member
                                                                .tpl_struct_origin,
                                                            member_primary,
                                                            member_args,
                                                            tpl_def->line,
                                                            &member_mangled,
                                                            "member_typedef_base_instantiation",
                                                            &resolved_member)) {
                                                        if (!resolved_member
                                                                 .record_def) {
                                                            if (Node* member_def =
                                                                    resolve_record_type_spec(
                                                                        resolved_member,
                                                                        nullptr)) {
                                                                resolved_member
                                                                    .record_def =
                                                                    member_def;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        inst->base_types[bi] = resolved_member;
                                        return true;
                                    };
                                    auto materialize_structured_base_record =
                                        [&](const std::string& base_mangled,
                                            const Node* primary,
                                            const std::vector<ParsedTemplateArg>& args)
                                            -> Node* {
                                        if (!primary || base_mangled.empty()) {
                                            return nullptr;
                                        }
                                        auto it =
                                            definition_state_.struct_tag_def_map.find(
                                                base_mangled);
                                        if (it ==
                                            definition_state_.struct_tag_def_map.end()) {
                                            return nullptr;
                                        }
                                        Node* candidate = it->second;
                                        if (!candidate ||
                                            candidate->kind != NK_STRUCT_DEF) {
                                            return nullptr;
                                        }
                                        const char* expected_origin =
                                            primary->template_origin_name &&
                                                    primary->template_origin_name[0]
                                                ? primary->template_origin_name
                                                : primary->name;
                                        const char* candidate_origin =
                                            candidate->template_origin_name &&
                                                    candidate->template_origin_name[0]
                                                ? candidate->template_origin_name
                                                : candidate->name;
                                        if (!expected_origin || !candidate_origin ||
                                            std::strcmp(expected_origin,
                                                        candidate_origin) != 0) {
                                            return nullptr;
                                        }
                                        if (candidate->n_template_args <
                                                static_cast<int>(args.size()) ||
                                            !candidate->template_arg_is_value) {
                                            return nullptr;
                                        }
                                        for (int ai = 0;
                                             ai < static_cast<int>(args.size());
                                             ++ai) {
                                            const ParsedTemplateArg& arg = args[ai];
                                            if (candidate->template_arg_is_value[ai] !=
                                                arg.is_value) {
                                                return nullptr;
                                            }
                                            if (arg.is_value) {
                                                if (!candidate->template_arg_values ||
                                                    candidate
                                                            ->template_arg_values[ai] !=
                                                        arg.value) {
                                                    return nullptr;
                                                }
                                                continue;
                                            }
                                            if (!candidate->template_arg_types ||
                                                !type_binding_values_equivalent(
                                                    candidate
                                                        ->template_arg_types[ai],
                                                    arg.type)) {
                                                return nullptr;
                                            }
                                        }
                                        return candidate;
                                    };
                                    auto absorb_instantiated_base_result =
                                        [&](const TypeSpec& produced,
                                            const std::string& base_mangled)
                                            -> Node* {
                                        Node* base_def = produced.record_def;
                                        if (!base_def) {
                                            base_def = resolve_record_type_spec(
                                                produced, nullptr);
                                        }
                                        if (!base_def) return nullptr;
                                        const char* saved_member_name =
                                            inst->base_types[bi]
                                                .deferred_member_type_name;
                                        const TextId saved_member_text_id =
                                            inst->base_types[bi]
                                                .deferred_member_type_text_id;
                                        inst->base_types[bi] = produced;
                                        if (!inst->base_types[bi].tag) {
                                            inst->base_types[bi] = TypeSpec{};
                                            inst->base_types[bi].array_size = -1;
                                            inst->base_types[bi].inner_rank = -1;
                                            inst->base_types[bi].base = TB_STRUCT;
                                            inst->base_types[bi].tag =
                                                arena_.strdup(
                                                    (produced.tag && produced.tag[0])
                                                        ? produced.tag
                                                        : base_mangled.c_str());
                                        }
                                        if (saved_member_name &&
                                            saved_member_name[0]) {
                                            inst->base_types[bi]
                                                .deferred_member_type_name =
                                                saved_member_name;
                                        }
                                        if (saved_member_text_id !=
                                            kInvalidText) {
                                            inst->base_types[bi]
                                                .deferred_member_type_text_id =
                                                saved_member_text_id;
                                        }
                                        inst->base_types[bi].record_def =
                                            base_def;
                                        return base_def;
                                    };
                                    if (inst->base_types[bi].tpl_struct_args.data &&
                                        inst->base_types[bi].tpl_struct_args.size > 0 &&
                                        base_primary) {
                                        const bool base_has_structured_carrier =
                                            inst->base_types[bi].record_def ||
                                            inst->base_types[bi]
                                                    .tpl_struct_origin_key
                                                    .base_text_id != kInvalidText ||
                                            (inst->base_types[bi]
                                                 .tpl_struct_args.data &&
                                             inst->base_types[bi]
                                                     .tpl_struct_args.size > 0);
                                        bool can_use_typed_args = true;
                                        std::vector<ParsedTemplateArg> base_args;
                                        std::vector<bool> base_arg_uses_default;
                                        base_args.reserve(
                                            inst->base_types[bi].tpl_struct_args.size);
                                        base_arg_uses_default.reserve(
                                            inst->base_types[bi].tpl_struct_args.size);
                                        for (int ai = 0;
                                             ai < inst->base_types[bi].tpl_struct_args.size;
                                             ++ai) {
                                            const TemplateArgRef& src_arg =
                                                inst->base_types[bi].tpl_struct_args.data[ai];
                                            ParsedTemplateArg base_arg{};
                                            bool uses_defaulted_nttp = false;
                                            const bool base_param_is_nttp =
                                                ai < base_primary->n_template_params &&
                                                base_primary->template_param_is_nttp[ai];
                                            const bool base_param_has_deferred_default =
                                                base_param_is_nttp &&
                                                base_primary->template_param_has_default &&
                                                base_primary
                                                    ->template_param_has_default[ai] &&
                                                    base_primary
                                                        ->template_param_default_values[ai] ==
                                                    LLONG_MIN;
                                            if (src_arg.kind == TemplateArgKind::Value) {
                                                const bool value_is_unresolved_default =
                                                    base_param_has_deferred_default &&
                                                    src_arg.value == 0 &&
                                                    src_arg.nttp_text_id == kInvalidText &&
                                                    !src_arg.type.array_size_expr;
                                                if (value_is_unresolved_default) {
                                                    base_arg.is_value = true;
                                                    base_arg.value = 0;
                                                    uses_defaulted_nttp = true;
                                                    base_args.push_back(base_arg);
                                                    base_arg_uses_default.push_back(
                                                        uses_defaulted_nttp);
                                                    continue;
                                                }
                                                if (zero_value_arg_ref_uses_debug_fallback(
                                                        src_arg)) {
                                                    can_use_typed_args = false;
                                                    break;
                                                }
                                                base_arg.is_value = true;
                                                base_arg.value = src_arg.value;
                                                base_arg.expr =
                                                    src_arg.type.array_size_expr;
                                                if (base_arg.expr) {
                                                    long long expr_value = 0;
                                                    if (!eval_structured_nttp_expr(
                                                            base_arg.expr,
                                                            &expr_value)) {
                                                        can_use_typed_args = false;
                                                        break;
                                                    }
                                                    base_arg.value = expr_value;
                                                    base_arg.expr = nullptr;
                                                }
                                            } else {
                                                if (unstructured_type_arg_ref_uses_debug_fallback(
                                                        src_arg)) {
                                                    if (base_param_has_deferred_default) {
                                                        base_arg.is_value = true;
                                                        base_arg.value = 0;
                                                        uses_defaulted_nttp = true;
                                                        base_args.push_back(
                                                            base_arg);
                                                        base_arg_uses_default.push_back(
                                                            uses_defaulted_nttp);
                                                        continue;
                                                    }
                                                    if (base_param_is_nttp) {
                                                        can_use_typed_args = false;
                                                        break;
                                                    }
                                                    const char* base_param_name =
                                                        ai < base_primary
                                                                 ->n_template_params
                                                            ? base_primary
                                                                  ->template_param_names[ai]
                                                            : nullptr;
                                                    bool found_bound_type = false;
                                                    if (base_param_name) {
                                                        TypeSpec bound_type{};
                                                        if (bound_type_for_param_name(
                                                                base_param_name,
                                                                &bound_type)) {
                                                            base_arg.is_value =
                                                                false;
                                                            base_arg.type =
                                                                bound_type;
                                                            found_bound_type =
                                                                true;
                                                        }
                                                    }
                                                    if (!found_bound_type) {
                                                        can_use_typed_args = false;
                                                        break;
                                                    }
                                                } else {
                                                    base_arg.is_value = false;
                                                    base_arg.type =
                                                        substitute_bound_type_arg(
                                                            src_arg.type);
                                                }
                                                if (type_mentions_bound_param(base_arg.type)) {
                                                    can_use_typed_args = false;
                                                    break;
                                                }
                                            }
                                            base_args.push_back(base_arg);
                                            base_arg_uses_default.push_back(
                                                uses_defaulted_nttp);
                                        }
                                        if (can_use_typed_args) {
                                            std::vector<std::pair<std::string, TypeSpec>>
                                                base_prelim_tb;
                                            std::vector<std::pair<std::string, long long>>
                                                base_prelim_nb;
                                            std::vector<ParserNttpBindingMetadata>
                                                base_prelim_nb_meta;
                                            for (int pi = 0;
                                                 pi < static_cast<int>(base_args.size()) &&
                                                 pi < base_primary->n_template_params; ++pi) {
                                                const char* pn =
                                                    base_primary->template_param_names[pi];
                                                if (base_primary->template_param_is_nttp[pi]) {
                                                    if (base_args[pi].is_value) {
                                                        const bool uses_defaulted_nttp =
                                                            pi < static_cast<int>(
                                                                     base_arg_uses_default
                                                                         .size()) &&
                                                            base_arg_uses_default[pi];
                                                        if (uses_defaulted_nttp) {
                                                            long long ev = 0;
                                                            const QualifiedNameKey
                                                                base_template_key =
                                                                    template_instantiation_name_key_for_direct_emit(
                                                                        *this,
                                                                        base_primary,
                                                                        origin);
                                                            bool evaluated_default =
                                                                eval_deferred_nttp_default(
                                                                    base_template_key,
                                                                    pi,
                                                                    base_prelim_tb,
                                                                    base_prelim_nb,
                                                                    &ev,
                                                                    &base_prelim_nb_meta);
                                                            if (!evaluated_default) {
                                                                can_use_typed_args =
                                                                    false;
                                                                break;
                                                            }
                                                            base_args[pi].value = ev;
                                                        }
                                                        base_prelim_nb.push_back(
                                                            {pn, base_args[pi].value});
                                                        base_prelim_nb_meta.push_back(
                                                            nttp_binding_metadata_for_template_param(
                                                                *this, base_primary,
                                                                pi, pn,
                                                                base_args[pi].value));
                                                    }
                                                } else if (!base_args[pi].is_value) {
                                                    base_prelim_tb.push_back(
                                                        {pn, base_args[pi].type});
                                                }
                                            }
                                            int bsz = static_cast<int>(base_args.size());
                                            while (bsz < base_primary->n_template_params) {
                                                if (!base_primary->template_param_has_default ||
                                                    !base_primary->template_param_has_default[bsz]) {
                                                    break;
                                                }
                                                ParsedTemplateArg da{};
                                                da.is_value =
                                                    base_primary->template_param_is_nttp[bsz];
                                                if (da.is_value &&
                                                    base_primary->template_param_default_values[bsz] ==
                                                        LLONG_MIN) {
                                                    long long ev = 0;
                                                    const QualifiedNameKey
                                                        base_template_key =
                                                            template_instantiation_name_key_for_direct_emit(
                                                                *this, base_primary,
                                                                origin);
                                                    if (eval_deferred_nttp_default(
                                                            base_template_key,
                                                            bsz,
                                                            base_prelim_tb, base_prelim_nb,
                                                            &ev,
                                                            &base_prelim_nb_meta)) {
                                                        da.value = ev;
                                                    } else {
                                                        can_use_typed_args = false;
                                                        break;
                                                    }
                                                } else if (da.is_value) {
                                                    da.value =
                                                        base_primary->template_param_default_values[bsz];
                                                } else {
                                                    da.type =
                                                        base_primary->template_param_default_types[bsz];
                                                }
                                                base_args.push_back(da);
                                                if (da.is_value) {
                                                    base_prelim_nb.push_back(
                                                        {base_primary->template_param_names[bsz],
                                                         da.value});
                                                    base_prelim_nb_meta.push_back(
                                                        nttp_binding_metadata_for_template_param(
                                                            *this, base_primary,
                                                            bsz,
                                                            base_primary->template_param_names[bsz],
                                                            da.value));
                                                } else {
                                                    base_prelim_tb.push_back(
                                                        {base_primary->template_param_names[bsz],
                                                         da.type});
                                                }
                                                ++bsz;
                                            }
                                            if (can_use_typed_args) {
                                                std::string base_mangled;
                                                TypeSpec produced_base{};
                                                if (ensure_template_struct_instantiated_from_args(
                                                        origin, base_primary, base_args,
                                                        tpl_def->line, &base_mangled,
                                                        "template_base_instantiation",
                                                        &produced_base)) {
                                                    Node* base_def =
                                                        absorb_instantiated_base_result(
                                                            produced_base,
                                                            base_mangled);
                                                    if (!base_def &&
                                                        base_has_structured_carrier) {
                                                        base_def =
                                                            materialize_structured_base_record(
                                                                base_mangled,
                                                                base_primary,
                                                                base_args);
                                                    } else if (!base_def) {
                                                        // Rendered-tag map lookup is a
                                                        // no-carrier compatibility
                                                        // fallback only. Structured
                                                        // parser/Sema carriers must
                                                        // not rediscover records by
                                                        // rendered spelling here.
                                                        auto base_def_it =
                                                            definition_state_.struct_tag_def_map.find(
                                                                base_mangled);
                                                        if (base_def_it !=
                                                            definition_state_
                                                                .struct_tag_def_map.end()) {
                                                            base_def = base_def_it->second;
                                                        }
                                                    }
                                                    if (base_def) {
                                                        inst->base_types[bi].record_def = base_def;
                                                    }
                                                    if (resolve_deferred_member_base()) {
                                                        continue;
                                                    }
                                                    restore_deferred_member_lookup();
                                                    continue;
                                                }
                                            }
                                        }
                                    }
                                    const bool base_has_structured_arg_carrier =
                                        inst->base_types[bi].tpl_struct_args.data &&
                                        inst->base_types[bi].tpl_struct_args.size > 0;
                                    if (base_primary && base_has_structured_arg_carrier) {
                                        restore_deferred_member_lookup();
                                        continue;
                                    }
                                    base_primary = find_instantiated_base_primary();
                                    if (base_primary) {
                                        const bool base_has_structured_carrier =
                                            inst->base_types[bi].record_def ||
                                            inst->base_types[bi]
                                                    .tpl_struct_origin_key
                                                    .base_text_id != kInvalidText ||
                                            base_has_structured_arg_carrier;
                                        // No explicit carrier remains. The only
                                        // parser/Sema-owned case here is a
                                        // default-only base such as Base<>.
                                        std::vector<ParsedTemplateArg> base_args;
                                        std::vector<std::pair<std::string, TypeSpec>>
                                            base_prelim_tb;
                                        std::vector<std::pair<std::string, long long>>
                                            base_prelim_nb;
                                        std::vector<ParserNttpBindingMetadata>
                                            base_prelim_nb_meta;
                                        bool can_use_default_only_args = true;
                                        int bsz = 0;
                                        while (bsz < base_primary->n_template_params) {
                                            if (!base_primary->template_param_has_default ||
                                                !base_primary->template_param_has_default[bsz]) {
                                                can_use_default_only_args = false;
                                                break;
                                            }
                                            ParsedTemplateArg da{};
                                            da.is_value =
                                                base_primary->template_param_is_nttp[bsz];
                                            if (da.is_value &&
                                                base_primary->template_param_default_values[bsz] ==
                                                    LLONG_MIN) {
                                                long long ev = 0;
                                                const QualifiedNameKey base_template_key =
                                                    template_instantiation_name_key_for_direct_emit(
                                                        *this, base_primary, origin);
                                                if (!eval_deferred_nttp_default(
                                                        base_template_key, bsz,
                                                        base_prelim_tb, base_prelim_nb,
                                                        &ev, &base_prelim_nb_meta)) {
                                                    can_use_default_only_args = false;
                                                    break;
                                                }
                                                da.value = ev;
                                            } else if (da.is_value) {
                                                da.value =
                                                    base_primary
                                                        ->template_param_default_values[bsz];
                                            } else {
                                                da.type =
                                                    base_primary
                                                        ->template_param_default_types[bsz];
                                            }
                                            base_args.push_back(da);
                                            if (da.is_value) {
                                                base_prelim_nb.push_back(
                                                    {base_primary
                                                         ->template_param_names[bsz],
                                                     da.value});
                                                base_prelim_nb_meta.push_back(
                                                    nttp_binding_metadata_for_template_param(
                                                        *this, base_primary, bsz,
                                                        base_primary
                                                            ->template_param_names[bsz],
                                                        da.value));
                                            } else {
                                                base_prelim_tb.push_back(
                                                    {base_primary
                                                         ->template_param_names[bsz],
                                                     da.type});
                                            }
                                            ++bsz;
                                        }
                                        if (can_use_default_only_args) {
                                            std::string base_mangled;
                                            TypeSpec produced_base{};
                                            if (ensure_template_struct_instantiated_from_args(
                                                    origin, base_primary, base_args,
                                                    tpl_def->line, &base_mangled,
                                                    "template_base_instantiation",
                                                    &produced_base)) {
                                                Node* base_def =
                                                    absorb_instantiated_base_result(
                                                        produced_base,
                                                        base_mangled);
                                                if (!base_def &&
                                                    base_has_structured_carrier) {
                                                    base_def =
                                                        materialize_structured_base_record(
                                                            base_mangled,
                                                            base_primary,
                                                            base_args);
                                                } else if (!base_def) {
                                                    // Rendered-tag map lookup is a
                                                    // no-carrier compatibility
                                                    // fallback only. Structured
                                                    // parser/Sema carriers must
                                                    // not rediscover records by
                                                    // rendered spelling here.
                                                    auto base_def_it =
                                                        definition_state_
                                                            .struct_tag_def_map
                                                            .find(base_mangled);
                                                    if (base_def_it !=
                                                        definition_state_
                                                            .struct_tag_def_map.end()) {
                                                        base_def =
                                                            base_def_it->second;
                                                    }
                                                }
                                                if (base_def) {
                                                    restore_deferred_member_lookup();
                                                    inst->base_types[bi].record_def =
                                                        base_def;
                                                }
                                            } else if (!base_has_structured_carrier) {
                                                if (auto base_def_it =
                                                           definition_state_
                                                               .struct_tag_def_map
                                                               .find(base_mangled);
                                                       base_def_it !=
                                                       definition_state_
                                                           .struct_tag_def_map.end()) {
                                                    // Rendered-tag map lookup is the
                                                    // only compatibility path left when
                                                    // instantiation failed and no
                                                    // structured TypeSpec carrier is
                                                    // available.
                                                    inst->base_types[bi] = TypeSpec{};
                                                    inst->base_types[bi].array_size = -1;
                                                    inst->base_types[bi].inner_rank = -1;
                                                    inst->base_types[bi].base = TB_STRUCT;
                                                    inst->base_types[bi].tag =
                                                        arena_.strdup(
                                                            base_mangled.c_str());
                                                    inst->base_types[bi].record_def =
                                                        base_def_it->second;
                                                    restore_deferred_member_lookup();
                                                }
                                            }
                                        }
                                    }
                                }
                                const std::string deferred_member_name =
                                    deferred_member_lookup_name(
                                        inst->base_types[bi]);
                                if (!deferred_member_name.empty() &&
                                    inst->base_types[bi].tag &&
                                    inst->base_types[bi].tag[0]) {
                                    TypeSpec resolved_member{};
                                    const TextId deferred_member_text_id =
                                        inst->base_types[bi]
                                            .deferred_member_type_text_id;
                                    if (lookup_struct_member_typedef_recursive_for_type(
                                            inst->base_types[bi],
                                            deferred_member_name,
                                            deferred_member_text_id,
                                            &resolved_member)) {
                                        resolved_member.deferred_member_type_name = nullptr;
                                        resolved_member.deferred_member_type_text_id =
                                            kInvalidText;
                                        inst->base_types[bi] = resolved_member;
                                    }
                                }
                            }
                        }
                        // Store template bindings so HIR can resolve pending types
                        // in method bodies and signatures.
                        {
                            int n = tpl_def->n_template_params;
                            inst->n_template_params = n;
                            inst->template_param_names = tpl_def->template_param_names;
                            inst->template_param_name_text_ids =
                                tpl_def->template_param_name_text_ids;
                            inst->template_param_is_nttp = tpl_def->template_param_is_nttp;
                            inst->n_template_args = n;
                            inst->template_arg_types = arena_.alloc_array<TypeSpec>(n);
                            inst->template_arg_is_value = arena_.alloc_array<bool>(n);
                            inst->template_arg_values = arena_.alloc_array<long long>(n);
                            inst->template_arg_nttp_names = arena_.alloc_array<const char*>(n);
                            inst->template_arg_nttp_text_ids = arena_.alloc_array<TextId>(n);
                            inst->template_arg_exprs = arena_.alloc_array<Node*>(n);
                            int tti = 0, nni = 0;
                            for (int pi = 0; pi < n; ++pi) {
                                if (tpl_def->template_param_is_nttp[pi]) {
                                    inst->template_arg_is_value[pi] = true;
                                    inst->template_arg_values[pi] =
                                        nni < (int)nttp_bindings.size() ? nttp_bindings[nni++].second : 0;
                                    inst->template_arg_nttp_names[pi] = nullptr;
                                    inst->template_arg_nttp_text_ids[pi] = kInvalidText;
                                    inst->template_arg_exprs[pi] = nullptr;
                                    inst->template_arg_types[pi] = {};
                                } else {
                                    inst->template_arg_is_value[pi] = false;
                                    inst->template_arg_types[pi] =
                                        tti < (int)type_bindings.size() ? type_bindings[tti++].second : TypeSpec{};
                                    inst->template_arg_values[pi] = 0;
                                    inst->template_arg_nttp_names[pi] = nullptr;
                                    inst->template_arg_nttp_text_ids[pi] = kInvalidText;
                                    inst->template_arg_exprs[pi] = nullptr;
                                }
                            }
                        }
                        // Clone member typedefs with type substitution and register
                        // them under the concrete instantiation scope so later
                        // `Template<Args>::member` lookup does not need to fall
                        // back to the template family name.
                        inst->n_member_typedefs = tpl_def->n_member_typedefs;
                        if (inst->n_member_typedefs > 0) {
                            inst->member_typedef_names =
                                arena_.alloc_array<const char*>(inst->n_member_typedefs);
                            inst->member_typedef_types =
                                arena_.alloc_array<TypeSpec>(inst->n_member_typedefs);
                            for (int ti = 0; ti < inst->n_member_typedefs; ++ti) {
                                inst->member_typedef_names[ti] =
                                    tpl_def->member_typedef_names[ti];
                                TypeSpec member_ts = tpl_def->member_typedef_types[ti];
                                if (tpl_def->template_arg_types &&
                                    tpl_def->template_arg_is_value) {
                                    for (int ai = 0;
                                         ai < tpl_def->n_template_args &&
                                         ai < static_cast<int>(actual_args.size());
                                         ++ai) {
                                        if (tpl_def->template_arg_is_value[ai] ||
                                            actual_args[ai].is_value) {
                                            continue;
                                        }
                                        const TypeSpec pattern_arg =
                                            tpl_def->template_arg_types[ai];
                                        if (!(member_ts.base == TB_TYPEDEF &&
                                              member_ts.tag &&
                                              pattern_arg.base == TB_TYPEDEF &&
                                              pattern_arg.tag &&
                                              std::string(member_ts.tag) ==
                                                  pattern_arg.tag)) {
                                            continue;
                                        }
                                        member_ts = actual_args[ai].type;
                                        break;
                                    }
                                }
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (member_ts.base == TB_TYPEDEF && member_ts.tag &&
                                        std::string(member_ts.tag) == pname) {
                                        const bool outer_lref = member_ts.is_lvalue_ref;
                                        const bool outer_rref = member_ts.is_rvalue_ref;
                                        member_ts.base = pts.base;
                                        member_ts.tag = pts.tag;
                                        member_ts.record_def = pts.record_def;
                                        member_ts.ptr_level += pts.ptr_level;
                                        member_ts.is_lvalue_ref =
                                            pts.is_lvalue_ref || outer_lref;
                                        member_ts.is_rvalue_ref =
                                            !member_ts.is_lvalue_ref &&
                                            (pts.is_rvalue_ref || outer_rref);
                                        member_ts.is_const |= pts.is_const;
                                        member_ts.is_volatile |= pts.is_volatile;
                                    }
                                }
                                if (member_ts.array_size_expr &&
                                    member_ts.array_size_expr->kind == NK_VAR &&
                                    member_ts.array_size_expr->name) {
                                    for (const auto& [npname, nval] : nttp_bindings) {
                                        if (std::string(member_ts.array_size_expr->name) == npname) {
                                            if (member_ts.array_rank > 0) {
                                                member_ts.array_dims[0] = nval;
                                                member_ts.array_size = nval;
                                            }
                                            member_ts.array_size_expr = nullptr;
                                            break;
                                        }
                                    }
                                }
                                inst->member_typedef_types[ti] = member_ts;
                                if (inst->member_typedef_names[ti] &&
                                    inst->member_typedef_names[ti][0]) {
                                    const TextId member_text_id =
                                        parser_text_id_for_token(
                                            kInvalidText,
                                            inst->member_typedef_names[ti]);
                                    register_template_instantiation_member_typedef_binding(
                                        structured_emitted_instance_key,
                                        member_text_id, member_ts);
                                }
                            }
                        }
                        // Clone fields with type and NTTP substitution
                        int num_fields = tpl_def->n_fields > 0 ? tpl_def->n_fields : 0;
                        inst->n_fields = num_fields;
                        inst->fields = arena_.alloc_array<Node*>(num_fields);
                        for (int fi = 0; fi < num_fields; ++fi) {
                            const Node* orig_f = tpl_def->fields[fi];
                            Node* new_f = make_node(NK_DECL, orig_f->line);
                            new_f->name = orig_f->name;
                            new_f->unqualified_name =
                                orig_f->unqualified_name ? orig_f->unqualified_name
                                                         : orig_f->name;
                            new_f->unqualified_text_id =
                                orig_f->unqualified_text_id != kInvalidText
                                    ? orig_f->unqualified_text_id
                                    : parser_text_id_for_token(
                                          kInvalidText,
                                          new_f->unqualified_name
                                              ? new_f->unqualified_name
                                              : "");
                            new_f->namespace_context_id =
                                inst->namespace_context_id;
                            new_f->type = orig_f->type;
                            new_f->ival = orig_f->ival;
                            new_f->is_anon_field = orig_f->is_anon_field;
                            new_f->is_static = orig_f->is_static;
                            new_f->is_constexpr = orig_f->is_constexpr;
                            new_f->init = orig_f->init;
                            // Substitute template type parameters in field type
                            for (const auto& [pname, pts] : type_bindings) {
                                if (new_f->type.base == TB_TYPEDEF && new_f->type.tag &&
                                    std::string(new_f->type.tag) == pname) {
                                    new_f->type.base = pts.base;
                                    new_f->type.tag = pts.tag;
                                    new_f->type.record_def = pts.record_def;
                                    new_f->type.ptr_level += pts.ptr_level;
                                    new_f->type.is_const |= pts.is_const;
                                    new_f->type.is_volatile |= pts.is_volatile;
                                }
                            }
                            // Substitute NTTP values in array dimensions
                            if (new_f->type.array_size_expr) {
                                Node* ase = new_f->type.array_size_expr;
                                if (ase->kind == NK_VAR && ase->name) {
                                    for (const auto& [npname, nval] : nttp_bindings) {
                                        if (std::string(ase->name) == npname) {
                                            // Replace the first array dim with the NTTP value
                                            if (new_f->type.array_rank > 0) {
                                                new_f->type.array_dims[0] = nval;
                                                new_f->type.array_size = nval;
                                            }
                                            new_f->type.array_size_expr = nullptr;
                                            break;
                                        }
                                    }
                                }
                            }
                            inst->fields[fi] = new_f;
                        }
                        // Clone methods with type substitution
                        int num_methods = tpl_def->n_children > 0 ? tpl_def->n_children : 0;
                        inst->n_children = num_methods;
                        if (num_methods > 0) {
                            inst->children = arena_.alloc_array<Node*>(num_methods);
                            for (int mi = 0; mi < num_methods; ++mi) {
                                const Node* orig_m = tpl_def->children[mi];
                                if (!orig_m || orig_m->kind != NK_FUNCTION) {
                                    inst->children[mi] = nullptr;
                                    continue;
                                }
                                Node* new_m = make_node(NK_FUNCTION, orig_m->line);
                                new_m->name = orig_m->name;
                                new_m->unqualified_name = orig_m->unqualified_name;
                                new_m->template_origin_name = orig_m->template_origin_name;
                                new_m->execution_domain = orig_m->execution_domain;
                                new_m->variadic = orig_m->variadic;
                                new_m->is_static = orig_m->is_static;
                                new_m->is_inline = orig_m->is_inline;
                                new_m->is_constexpr = orig_m->is_constexpr;
                                new_m->is_consteval = orig_m->is_consteval;
                                new_m->is_const_method = orig_m->is_const_method;
                                new_m->is_lvalue_ref_method = orig_m->is_lvalue_ref_method;
                                new_m->is_rvalue_ref_method = orig_m->is_rvalue_ref_method;
                                new_m->is_constructor = orig_m->is_constructor;
                                new_m->is_destructor = orig_m->is_destructor;
                                new_m->is_deleted = orig_m->is_deleted;
                                new_m->is_defaulted = orig_m->is_defaulted;
                                // Substitute template type params in return type
                                new_m->type = orig_m->type;
                                for (const auto& [pname, pts] : type_bindings) {
                                    if (new_m->type.base == TB_TYPEDEF && new_m->type.tag &&
                                        std::string(new_m->type.tag) == pname) {
                                        new_m->type.base = pts.base;
                                        new_m->type.tag = pts.tag;
                                        new_m->type.record_def = pts.record_def;
                                        new_m->type.ptr_level += pts.ptr_level;
                                    }
                                }
                                // Clone params with type substitution
                                new_m->n_params = orig_m->n_params;
                                if (new_m->n_params > 0) {
                                    new_m->params = arena_.alloc_array<Node*>(new_m->n_params);
                                    for (int pi = 0; pi < new_m->n_params; ++pi) {
                                        const Node* orig_p = orig_m->params[pi];
                                        Node* new_p = make_node(NK_DECL, orig_p->line);
                                        new_p->name = orig_p->name;
                                        new_p->type = orig_p->type;
                                        for (const auto& [pname, pts] : type_bindings) {
                                            if (new_p->type.base == TB_TYPEDEF && new_p->type.tag &&
                                                std::string(new_p->type.tag) == pname) {
                                                new_p->type.base = pts.base;
                                                new_p->type.tag = pts.tag;
                                                new_p->type.record_def = pts.record_def;
                                                new_p->type.ptr_level += pts.ptr_level;
                                            }
                                        }
                                        new_m->params[pi] = new_p;
                                    }
                                }
                                new_m->n_ctor_inits = orig_m->n_ctor_inits;
                                new_m->ctor_init_names = orig_m->ctor_init_names;
                                new_m->ctor_init_args = orig_m->ctor_init_args;
                                new_m->ctor_init_nargs = orig_m->ctor_init_nargs;
                                // Body is shared (not deep-cloned) — HIR lowering
                                // handles field access via this->field resolution.
                                new_m->body = orig_m->body;
                                inst->children[mi] = new_m;
                            }
                        }
                        definition_state_.struct_defs.push_back(inst);
                        definition_state_.struct_tag_def_map[mangled] = inst;
                        definition_state_.defined_struct_tags.insert(mangled);
                        instantiated_record = inst;
                    } else {
                        auto existing_inst =
                            definition_state_.struct_tag_def_map.find(mangled);
                        if (existing_inst !=
                            definition_state_.struct_tag_def_map.end()) {
                            instantiated_record = existing_inst->second;
                        }
                    }
                    ts.tag = arena_.strdup(mangled.c_str());
                    ts.tag_text_id = mangled_text_id;
                    ts.namespace_context_id = mangled_namespace_context;
                    ts.record_def = instantiated_record;
                }
                // C++ template using alias: typedef resolved but the resolved
                // type is not a primary template struct (e.g. using A = S<T>;
                // then A<int>).  Skip the <...> since the typedef already
                // encodes the target type.
                if (is_cpp_mode() && check(TokenKind::Less)) {
                    int depth = 1;
                    int paren_depth = 0;
                    consume();  // <
                    while (!at_end() && depth > 0) {
                        if (check(TokenKind::LParen)) { ++paren_depth; consume(); continue; }
                        if (check(TokenKind::RParen)) { if (paren_depth > 0) --paren_depth; consume(); continue; }
                        // Only count <> as template brackets when not inside parens
                        if (paren_depth == 0 && check(TokenKind::Less)) ++depth;
                        else if (paren_depth == 0 && check_template_close()) {
                            --depth;
                            if (depth > 0) { match_template_close(); continue; }
                            break;
                        }
                        consume();
                    }
                    if (check_template_close()) match_template_close();
                }
                // Handle TemplateStruct<Args>::member suffix (e.g. bool_constant<true>::type).
                // Resolve the member as a typedef within the instantiated struct.
                if (is_cpp_mode() && check(TokenKind::ColonColon) &&
                    core_input_state_.pos + 1 <
                        static_cast<int>(core_input_state_.tokens.size()) &&
                    core_input_state_.tokens[core_input_state_.pos + 1].kind ==
                        TokenKind::Identifier) {
                    std::string member(token_spelling(core_input_state_.tokens[core_input_state_.pos + 1]));
                    const TextId member_text_id =
                        core_input_state_.tokens[core_input_state_.pos + 1].text_id;
                    const bool should_preserve_deferred_template_member =
                        ts.tpl_struct_origin && ts.tpl_struct_args.size > 0 &&
                        member == "type";
                    if (should_preserve_deferred_template_member) {
                        consume(); // ::
                        consume(); // member
                        ts.deferred_member_type_name = arena_.strdup(member.c_str());
                        ts.deferred_member_type_text_id = member_text_id;
                        return ts;
                    }
                    TypeSpec resolved{};
                    if (lookup_struct_member_typedef_recursive_for_type(
                            ts, member, member_text_id, &resolved)) {
                        consume(); // ::
                        consume(); // member
                        // The typedef resolves to a type — for `typedef bool_constant type;` inside
                        // bool_constant<true>, `type` refers back to bool_constant<true> itself.
                        // The resolved typedef's tag might be the template name, not the instantiation.
                        // If the resolved type is the same template origin, use the instantiation tag.
                        if (resolved.base == TB_STRUCT && resolved.tag) {
                            const Node* inst_def =
                                resolve_record_type_spec(ts, nullptr);
                            if (!inst_def && ts.tag) {
                                // Rendered-tag map lookup is a compatibility
                                // fallback for instantiated owners without
                                // TypeSpec::record_def.
                                auto inst_it =
                                    definition_state_.struct_tag_def_map.find(ts.tag);
                                if (inst_it !=
                                        definition_state_.struct_tag_def_map.end()) {
                                    inst_def = inst_it->second;
                                }
                            }
                            if (inst_def &&
                                inst_def->template_origin_name &&
                                std::string(resolved.tag) == inst_def->template_origin_name) {
                                resolved.tag = ts.tag;
                            }
                        }
                        bool save_const = ts.is_const, save_vol = ts.is_volatile;
                        ts = resolved;
                        ts.is_const |= save_const;
                        ts.is_volatile |= save_vol;
                    } else if (ts.tpl_struct_origin || (ts.tag && ts.tag[0])) {
                        consume(); // ::
                        consume(); // member
                        ts.deferred_member_type_name = arena_.strdup(member.c_str());
                        ts.deferred_member_type_text_id = member_text_id;
                    }
                }
                return ts;
            }
        }
        if (ts.record_def &&
            (ts.base == TB_STRUCT || ts.base == TB_UNION)) {
            return ts;
        }
        ts.base = TB_TYPEDEF;
        // tag already set above
        // In C++ mode, skip unresolved template arguments (e.g. reverse_iterator<Iterator1>)
        if (is_cpp_mode() && check(TokenKind::Less)) {
            int depth = 1;
            consume();  // <
            while (!at_end() && depth > 0) {
                if (check(TokenKind::Less)) ++depth;
                else if (check_template_close()) {
                    --depth;
                    if (depth > 0) { match_template_close(); continue; }
                    break;
                }
                consume();
            }
            if (check_template_close()) match_template_close();
        }
    } else if (has_void) {
        ts.base = TB_VOID;
    } else if (has_bool) {
        ts.base = TB_BOOL;
    } else if (has_float) {
        ts.base = has_complex ? TB_COMPLEX_FLOAT : TB_FLOAT;
    } else if (has_double) {
        if (has_complex) ts.base = long_count > 0 ? TB_COMPLEX_LONGDOUBLE : TB_COMPLEX_DOUBLE;
        else ts.base = long_count > 0 ? TB_LONGDOUBLE : TB_DOUBLE;
    } else if (has_char) {
        if (has_complex)
            ts.base = has_unsigned ? TB_COMPLEX_UCHAR : TB_COMPLEX_SCHAR;
        else
            ts.base = has_unsigned ? TB_UCHAR : (has_signed ? TB_SCHAR : TB_CHAR);
    } else if (has_short) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_USHORT : TB_COMPLEX_SHORT)
                              : (has_unsigned ? TB_USHORT : TB_SHORT);
    } else if (long_count >= 2) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONGLONG : TB_COMPLEX_LONGLONG)
                              : (has_unsigned ? TB_ULONGLONG : TB_LONGLONG);
    } else if (long_count == 1) {
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_ULONG : TB_COMPLEX_LONG)
                              : (has_unsigned ? TB_ULONG : TB_LONG);
    } else if (has_complex && !has_int_kw && !has_signed && !has_unsigned) {
        ts.base = TB_COMPLEX_DOUBLE;  // __complex__ with no type = double _Complex
    } else {
        // plain int (possibly unsigned)
        ts.base = has_complex ? (has_unsigned ? TB_COMPLEX_UINT : TB_COMPLEX_INT)
                              : (has_unsigned ? TB_UINT : TB_INT);
    }

    finalize_vector_type(ts);
    return ts;
}

}  // namespace c4c

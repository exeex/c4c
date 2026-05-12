#pragma once

// Narrow public parser-support helpers used outside the parser implementation.

#include "ast.hpp"

#include <string>
#include <unordered_map>

namespace c4c {

long long sizeof_base(TypeBase b);
long long sizeof_type_spec(const TypeSpec& ts);
long long alignof_type_spec(const TypeSpec& ts);

// Ordinary parser-support record resolution uses only direct record_def
// metadata. Callers that still need the parser-local rendered tag mirror must
// opt into the compatibility entry point below.
Node* resolve_record_type_spec(const TypeSpec& ts);

// Explicit compatibility bridge for parser-local record probes that still
// receive rendered record maps. Prefer direct record_def and structured record
// metadata; remove this API once those callers carry record_def or structured
// record keys.
Node* resolve_record_type_spec_with_parser_tag_map_compatibility(
    const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>*
        parser_tag_map_compatibility);

// Parser-owned constant evaluation should pass named constants through the
// TextId table. For record layout queries, TypeSpec carries only provisional
// parser output: record kind through TypeBase, tag TextId, namespace context,
// qualifier TextId sequence, source spelling/location on the owning Node, and
// declaration/reference role from AST context. The separate
// resolve_record_type_spec_with_parser_tag_map_compatibility API keeps the
// existing parser-local compatibility bridge alive for non-layout parser
// probes and declaration checks. Constant-layout evaluation uses record_def for
// structured records; Sema owns final record identity and completion.
bool eval_const_int(Node* n, long long* out,
                    const std::unordered_map<std::string, Node*>*
                        parser_record_layout_compatibility_tag_map = nullptr,
                    const std::unordered_map<TextId, long long>* structured_named_consts =
                        nullptr);
// Explicit compatibility bridge for legacy/HIR template paths that only carry
// rendered NTTP/constant names. If parser structured named-constant metadata is
// supplied, it is authoritative and misses fail closed before rendered lookup.
// New parser-owned callers should use the TextId surface above, and ordinary
// overload resolution should not select this path.
bool eval_const_int_with_rendered_named_const_compatibility(
    Node* n, long long* out,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map,
    const std::unordered_map<std::string, long long>* compatibility_named_consts,
    const std::unordered_map<TextId, long long>* structured_named_consts =
        nullptr);

// Parser-owned typedef resolution should pass typedef names through the
// TextId table. A miss in this structured domain is authoritative and must not
// recover through rendered fallback names.
TypeSpec resolve_typedef_chain(TypeSpec ts,
                               const std::unordered_map<TextId, TypeSpec>& typedefs);
bool types_compatible_p(TypeSpec a, TypeSpec b,
                        const std::unordered_map<TextId, TypeSpec>& typedefs);

}  // namespace c4c

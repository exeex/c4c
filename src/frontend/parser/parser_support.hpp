#pragma once

// Narrow public parser-support helpers used outside the parser implementation.

#include "ast.hpp"

#include <string>
#include <unordered_map>

namespace c4c {

long long sizeof_base(TypeBase b);
long long sizeof_type_spec(const TypeSpec& ts);
long long alignof_type_spec(const TypeSpec& ts);
Node* resolve_record_type_spec(
    const TypeSpec& ts,
    const std::unordered_map<std::string, Node*>* compatibility_tag_map);

// Parser-owned constant evaluation should pass named constants through the
// TextId table. TypeSpec::record_def is the record-layout authority; rendered
// struct tags remain a tag-only compatibility bridge for sizeof/alignof/
// offsetof callers that still carry final spelling without typed record
// identity.
bool eval_const_int(Node* n, long long* out,
                    const std::unordered_map<std::string, Node*>*
                        compatibility_tag_map = nullptr,
                    const std::unordered_map<TextId, long long>* structured_named_consts =
                        nullptr);
// Compatibility bridge for legacy/HIR proof paths that only carry rendered
// constant names. New parser-owned callers should use the TextId overload.
bool eval_const_int(Node* n, long long* out,
                    const std::unordered_map<std::string, Node*>*
                        compatibility_tag_map,
                    const std::unordered_map<std::string, long long>* compatibility_named_consts);

TypeSpec resolve_typedef_chain(TypeSpec ts,
                               const std::unordered_map<std::string, TypeSpec>& tmap);
bool types_compatible_p(TypeSpec a, TypeSpec b,
                        const std::unordered_map<std::string, TypeSpec>& tmap);

}  // namespace c4c

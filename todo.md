# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Split Remaining Parser Semantic Lookup From Text Spelling

## Just Finished

Step 3 packet completed, including the follow-up focused unit test:
`select_template_struct_pattern()` no longer uses local
`std::unordered_map<std::string, ...>` scratch maps as the authority for
template specialization type/value parameter bindings.

The local matching path now resolves template parameters to parser `TextId`
identity from `Node::template_param_name_text_ids`, with a parser text table
fallback only when existing AST payloads lack a valid text id. Rendered
parameter names remain only for the existing public output vectors and
diagnostic/compatibility spelling.

Added `test_parser_template_specialization_binding_prefers_param_text_id()` to
`tests/frontend/frontend_parser_tests.cpp`; it directly selects a specialization
whose rendered template parameter name is stale while
`template_param_name_text_ids` still points at the real parameter identity.

## Suggested Next

Step 3 remains active.

Next bounded Step 3 packet: convert the alias-template substitution scratch
state in `src/frontend/parser/impl/types/base.cpp` away from rendered spelling
as semantic binding authority. Scope this to the `parse_type_name()` alias
template path around `find_alias_template_info()`, the local
`std::unordered_map<std::string, std::string> subst`,
`substitute_template_arg_refs()`, and the related transformed-owner argument
builder. Use `ParserAliasTemplateInfo::param_name_text_ids` as the primary
parameter identity for substitution bindings, with rendered names retained only
for existing argument-ref spelling, parsing compatibility, and fallback when a
stored text id is invalid.

Suggested focused proof: `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R
'^(frontend_parser_tests|cpp_positive_sema_(template_specialization_member_typedef_trait_parse_cpp|template_specialization_typedef_chain_parse_cpp|template_specialization_visible_typedef_chain_parse_cpp|template_struct_specialization_parse_cpp|template_struct_specialization_runtime_cpp|template_bool_specialization_parse_cpp|specialization_identity_cpp|template_forward_pick_specialization_identity_cpp|eastl_slice6_template_defaults_and_refqual_cpp))$'
> test_after.log 2>&1`

## Watchouts

`match_type_pattern()` still receives a `TypeSpec` whose typedef tag spelling is
the only available payload for pattern references; the conversion resolves that
spelling back to parser `TextId` before touching binding maps. Avoid widening
this into `parser_state.hpp`, template rendered mirrors, public binding vector
APIs, `defined_struct_tags`, or `struct_tag_def_map`.

For the next alias-template packet, do not change public support helper
signatures, `ParserAliasTemplateInfo` lookup keys, template rendered mirror
maps, `nttp_default_expr_tokens`, `defined_struct_tags`, or
`struct_tag_def_map`. The local string-to-string substitution helper may still
need to render substituted argument refs because `TypeSpec` and existing alias
argument refs carry spelling; the semantic binding map should stop using those
rendered parameter names as its primary key.

The focused unit test includes `impl/types/types_helpers.hpp` directly, so that
header now explicitly includes `<stdexcept>` instead of relying on including
translation units to provide it.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_(template_specialization_member_typedef_trait_parse_cpp|template_specialization_typedef_chain_parse_cpp|template_specialization_visible_typedef_chain_parse_cpp|template_struct_specialization_parse_cpp|template_struct_specialization_runtime_cpp|template_bool_specialization_parse_cpp|specialization_identity_cpp|template_forward_pick_specialization_identity_cpp|eastl_slice6_template_defaults_and_refqual_cpp))$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser and
template specialization subset output: 10 tests passed, 0 failed.

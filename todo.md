# Current Packet

Status: Active
Source Idea Path: ideas/open/123_parser_legacy_string_lookup_removal_convergence.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Demote Compatibility String Helpers And Tests

## Just Finished

Step 3 packet completed: alias-template substitution in
`parse_type_name()` no longer uses a local
`std::unordered_map<std::string, std::string>` as the semantic authority for
template parameter substitution.

The local alias-template scratch binding map is now keyed by parser `TextId`
from `ParserAliasTemplateInfo::param_name_text_ids`, with rendered parameter
names used only as the fallback path when stored text ids are invalid and for
existing rendered argument-ref output. The transformed owner argument path and
direct aliased-parameter replacement now resolve references through the same
`TextId` lookup before touching substitution bindings.

Added `test_parser_alias_template_substitution_prefers_param_text_id()` to
`tests/frontend/frontend_parser_tests.cpp`; it parses an alias template whose
rendered parameter name is stale while `param_name_text_ids` still carries the
real parameter identity.

## Suggested Next

Step 3 is exhausted for the current runbook. Remaining direct string-keyed
parser surfaces now classify as compatibility/final-spelling mirrors, public
support-helper boundaries, or already-paired structured-key caches rather than
precise bounded Step 3 semantic scratch packets.

First bounded Step 4 packet: demote the record-layout compatibility helper
surface around `resolve_record_type_spec()`, `eval_const_int()` struct-map
parameters, and the frontend parser record-layout tests. Keep typed
`TypeSpec::record_def` authority unchanged, but rename or document the retained
rendered tag map path as tag-only compatibility/final-spelling fallback in the
header, helper parameter names/comments, and test names/assertions.

## Watchouts

Alias-template argument references still render through existing string debug
refs because the surrounding `TypeSpec` payload and public compatibility
surfaces carry spelling. This packet only moved the local binding authority to
`TextId`; it did not change those compatibility payloads.

The fallback path intentionally interns rendered parameter names only when an
alias template info entry lacks a valid stored parameter `TextId`.

## Proof

Proof command run exactly as delegated:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_(template_specialization_member_typedef_trait_parse_cpp|template_specialization_typedef_chain_parse_cpp|template_specialization_visible_typedef_chain_parse_cpp|template_struct_specialization_parse_cpp|template_struct_specialization_runtime_cpp|template_bool_specialization_parse_cpp|specialization_identity_cpp|template_forward_pick_specialization_identity_cpp|eastl_slice6_template_defaults_and_refqual_cpp))$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the delegated frontend parser and
template specialization subset output: 10 tests passed, 0 failed.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the parent Step 6 qualified alias-template type parser boundary.

The structured `try_parse_qualified_base_type` alias-template branch now
projects `ParserAliasTemplateInfo::aliased_type` when the alias is a direct
type alias rather than a member-typedef carrier. This keeps semantic authority
on the parser alias-template registry and fixes qualified plain aliases such
as `ns::void_t<>` without accepting unknown namespace-qualified template ids
from spelling alone.

The delegated proof improved from the packet baseline of 0/13 passing to 7/13
passing. Newly green in this packet:

- `cpp_eastl_integer_sequence_parse_recipe`
- `cpp_eastl_type_traits_parse_recipe`
- `cpp_eastl_utility_parse_recipe`
- `cpp_eastl_vector_parse_recipe`
- `cpp_eastl_memory_uses_allocator_parse_recipe`
- `eastl_cpp_external_utility_frontend_basic_cpp`
- `cpp_parse_template_alias_empty_pack_default_arg_dump`

Remaining failures in the delegated subset:

- `cpp_qualified_template_call_template_arg_perf` still times out.
- `cpp_parse_record_base_variable_template_value_arg_dump` still misses the
  expected `has_unique_object_representations_T_int` specialization dump.
- `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_spelling_stack`,
  `cpp_parser_debug_tentative_template_arg_lifecycle`, and
  `cpp_parser_debug_tentative_cli_only` still fail their expected parser-debug
  stack substrings.

## Suggested Next

Recommended next Step 6 implementation packet: repair the remaining parser
qualified/template debug/perf/value-template cluster. Start with the value
template argument specialization dump for
`has_unique_object_representations<int>` and the parser-debug stack cases,
then rerun the same 13-test proof.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The namespace-qualified unknown-template guard was checked separately with
  `frontend_parser_tests`, which passed.
- The qualified alias-template fix depends on a real
  `ParserAliasTemplateInfo` lookup; unknown namespace-qualified template ids
  still roll back instead of being accepted from spelling.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Parent Step 6 focused parser proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$' > test_after.log 2>&1`

Result: failed but improved, 7/13 passing with 6 remaining failures.

Proof log: `test_after.log`.

Additional guard check:
`ctest --test-dir build --output-on-failure -R '^frontend_parser_tests$'`

Result: passed, 1/1.

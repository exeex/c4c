# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the parent Step 6 parser perf boundary for call-like alias-template
member expressions used as non-type template arguments.

`parse_next_template_argument` now has a structured fast carrier for
`Alias<Ts...>::template member<Us...>()`-shaped value arguments. The carrier is
an `NK_CALL` over `NK_MEMBER`/`NK_VAR` with parsed template-argument metadata on
the owner and selected member; it does not raw-capture the token stream and it
leaves enclosing `>>` handling to the existing template closer logic.

The supporting parser work keeps rollback changes narrow: simple builtin/type
template arguments and a few cursor-only lookahead probes now use lite
tentative guards, while the broader parser disambiguation paths continue to
use full rollback. The focused perf case now passes in the delegated subset.

The refreshed delegated proof improved from 11/13 to 12/13 passing. The only
remaining failure is:

- `cpp_parse_record_base_variable_template_value_arg_dump` still misses the
  expected `has_unique_object_representations_T_int` specialization dump.

## Suggested Next

Recommended next Step 6 implementation packet: repair the variable-template
value-argument record-base specialization seam. The first visible boundary is
that `eastl::has_unique_object_representations<int>::value` is parsed/lowered
as `Var(eastl::has_unique_object_representations::value) specialize<1>`
without causing the concrete `has_unique_object_representations_T_int` record
specialization to appear in the parse dump.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The namespace-qualified unknown-template guard was checked separately with
  `frontend_parser_tests`, which passed.
- The prior qualified alias-template fix remains green in this proof; preserve
  the structured `ParserAliasTemplateInfo` authority and the unknown
  namespace-qualified template-id guard.
- The `Owner<Args>::member` repair only succeeds after structured template
  owner lookup plus selected-member typedef resolution; unknown member names
  still roll back.
- The prior perf guard's repeated full tentative-expression parsing in
  qualified template-call default arguments is fixed by the structured carrier
  in this packet; preserve the focused carrier shape rather than returning to
  raw token capture.
- Rollback-mode changes in this packet are limited to the new structured
  carrier, simple type-argument fast paths, and cursor-only lookahead probes.
  Do not broaden lite rollback without a matching parser-state audit.
- The parse-only value-template dump still lowers
  `eastl::has_unique_object_representations<int>::value` as
  `Var(eastl::has_unique_object_representations::value) specialize<1>` without
  emitting the concrete `has_unique_object_representations_T_int` record.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Step 6 focused parser proof after the structured NTTP-expression carrier:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$' > test_after.log 2>&1`

Result: failed, 12/13 passing. `cpp_qualified_template_call_template_arg_perf`
passes in 4.40 sec; only
`cpp_parse_record_base_variable_template_value_arg_dump` remains failing.

Proof log: `test_after.log`.

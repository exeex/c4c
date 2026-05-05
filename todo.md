# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Blocked the parent Step 6 attempt to repair the final two focused parser
subset failures.

The local perf probe found that avoiding full tentative snapshots for
qualified call-like template arguments can make
`cpp_qualified_template_call_template_arg_perf` pass, but the only explored
source route was not commit-ready: early value-expression token capture for
alias/variable-template calls destabilized the EASTL recipe path, and broader
lite-guard conversion allowed speculative parser state to escape rollback.
Those non-ready source edits were reverted.

The refreshed delegated proof is back at the clean packet baseline: 11/13
passing with the same two remaining failures:

- `cpp_qualified_template_call_template_arg_perf` still times out.
- `cpp_parse_record_base_variable_template_value_arg_dump` still misses the
  expected `has_unique_object_representations_T_int` specialization dump.

## Suggested Next

Recommended next Step 6 implementation packet: implement a structured
non-type template argument expression carrier for call-like alias-template
member expressions. The first bad boundary is
`try_parse_template_non_type_arg`: expressions such as
`Alias<Ts...>::template member<Us...>()` fall through to full expression
parsing inside template-argument disambiguation. The next attempt should
consume that structured expression shape without raw token capture, without
swallowing `>>` needed by enclosing template lists, and without broad
rollback-mode changes.

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
- The perf guard still shows quadratic behavior from repeated full
  tentative-expression parsing in qualified template-call default arguments.
- A rejected local probe made the perf case pass by token-capturing
  value-like `Alias<Ts...>::template f<Us...>()` arguments, but it regressed
  EASTL because direct variable-template arguments closed by `>>` need
  structured closer splitting, not raw token capture.
- Another rejected probe converted broad parser lookahead guards to lite
  rollback; that can leak speculative parser symbol state and caused EASTL
  recipe failures/kills. Keep rollback-mode changes narrow and prove each one
  against EASTL/debug cases.
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

Parent Step 6 focused parser proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$' > test_after.log 2>&1`

Result: failed, 11/13 passing with 2 remaining failures. This packet did not
improve the baseline and is blocked with no source changes retained.

Proof log: `test_after.log`.

Additional rejected local probe:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^cpp_qualified_template_call_template_arg_perf$'`

Result: passed only while non-ready parser guard/capture edits were present;
the edits were reverted because the delegated subset regressed.

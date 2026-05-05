# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the parent Step 6 qualified template-owner member typedef handoff.

The structured qualified-template parser now keeps `Owner<Args>::member` in
the type-specifier path when `member` resolves to a record member typedef on
the selected template owner. It selects the owner template pattern from the
structured parsed arguments, applies type-parameter bindings to the member
typedef, and returns the resolved `TypeSpec` before declarator parsing. This
prevents `Owner<Args>::type value(` from being split into a standalone
`Owner<Args>` type item followed by a separate `value(` parse.

The delegated proof improved from the packet baseline of 8/13 passing to 10/13
passing. Newly green in this packet:

- `cpp_parser_debug_tentative_template_arg_lifecycle`
- `cpp_parser_debug_tentative_cli_only`

Remaining failures in the delegated subset:

- `cpp_qualified_template_call_template_arg_perf` still times out.
- `cpp_parse_record_base_variable_template_value_arg_dump` still misses the
  expected `has_unique_object_representations_T_int` specialization dump.
- `cpp_parser_debug_qualified_type_spelling_stack` still fails because the
  repaired semantic path exposes the deeper nested `ns::Box<int>` probe in the
  retained summary stack; the expected substring is now too shallow for the
  full structured path.

## Suggested Next

Recommended next Step 6 implementation packet: repair one of the remaining
semantic parser boundaries in the same subset. The clearest next seams are the
quadratic qualified template-call default-argument parse path
(`cpp_qualified_template_call_template_arg_perf`) and the missing concrete
specialization for `Trait<T>::value` after a variable-template value argument
in a record base clause
(`cpp_parse_record_base_variable_template_value_arg_dump`). Treat the remaining
parser-debug spelling-stack mismatch as a diagnostic-summary normalization
follow-up after the semantic parser seams are smaller.

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
- The perf guard shows quadratic behavior: local timing was roughly 1000
  generated declarations in 1.9s and 2000 in 7.3s, with 5000 timing out.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Parent Step 6 focused parser proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$' > test_after.log 2>&1`

Result: failed but improved, 10/13 passing with 3 remaining failures.

Proof log: `test_after.log`.

Additional guard check:
`ctest --test-dir build --output-on-failure -R '^frontend_parser_tests$'`

Result: passed, 1/1.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the Step 6 parser/HIR structured metadata regression in functional
cast preliminary evaluation for explicit NTTP template arguments.

The parser now evaluates a direct functional-cast explicit NTTP argument with
the pre-template outer preliminary bindings plus only missing earlier
parameters from the same primary template. That lets a structured expression
such as `Box<unsigned, T(7)>` resolve `T` through the primary template
parameter text-id/owner metadata rather than through stale rendered parameter
spelling, while preserving the existing isolation from unrelated current
template bindings and avoiding eager trait-base owner instantiation.

The delegated two-test proof improved from the supervisor baseline of 1/2 to
2/2 passing. The focused 13-test parser subset is green again after narrowing
the prior-parameter rebuild, and the required spot checks for the
namespace-qualified parser guard plus the two prior positive-Sema regressions
all pass.

## Suggested Next

Recommended next Step 6 packet: supervisor-side broad/full validation for the
parent TypeSpec-tag deletion route. This metadata slice is isolated to the
parser preliminary NTTP evaluator and the delegated proof is green.

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
- This packet did not add rollback/lite-guard changes.
- Variable-template value-argument evaluation now depends on structured
  template-global registration plus owner-aware template-parameter metadata;
  preserve owner/index checks so nested templates with the same parameter name
  do not bind to the wrong scope.
- Explicit NTTP argument preliminary evaluation must not see arbitrary current
  template bindings. The local same-primary rebuild is intentionally limited to
  direct functional-cast carriers and skips text ids already visible in the
  saved outer bindings, so unrelated trait expressions such as struct-base
  owner checks remain deferred instead of becoming parser-owned placeholders.
- Concrete owner materialization is limited to structured, concrete template
  owner arguments. Dependent owner arguments, including `sizeof...(pack)`,
  still remain deferred for later specialization.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Step 6 delegated parser/HIR metadata proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^(cpp_hir_parser_type_base_prelim_eval_structured_metadata|cpp_qualified_template_call_template_arg_perf)$' > test_after.log 2>&1`

Result: passed, 2/2. `cpp_hir_parser_type_base_prelim_eval_structured_metadata`
now passes, and `cpp_qualified_template_call_template_arg_perf` remains green
at 4.33 sec.

Proof log: `test_after.log`.

Focused parser subset confirmation:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$'`

Result: passed, 13/13. The previously regressed
`eastl_cpp_external_utility_frontend_basic_cpp` is green.

Required spot checks:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_inherited_static_member_lookup_runtime_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp)$'`

Result: passed, 3/3.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the final parent Step 6 focused parser failure for variable-template
value arguments in record-base specialization metadata.

Parser template-global primaries are now registered with structured
namespace/name identity, and the parser-side structured NTTP evaluator can
resolve variable-template references through their initializer metadata when
the value argument is an AST carrier rather than captured text.

The expression handoff for `Template<Args>::member` now preserves parsed
qualified owner metadata without falling back to parser-only placeholders.
Qualified injected instantiations re-enter the primary template's structured
namespace context, and dependent value-expression owner arguments stay deferred.
This lets `eastl::has_unique_object_representations<int>::value` emit the real
concrete `has_unique_object_representations_T_int` record in the parse dump
without regressing `sizeof...(pack)` value arguments.

The refreshed delegated focused proof improved from the accepted 12/13
baseline to 13/13 passing, and the requested broader `^cpp_positive_sema_`
validation is green at 884/884.

## Suggested Next

Recommended next Step 6 packet: supervisor-side full-suite validation for the
parent TypeSpec-tag deletion route. The focused parser subset and
`^cpp_positive_sema_` are green; use full validation to identify the next
remaining non-parser cluster.

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
- Concrete owner materialization is limited to structured, concrete template
  owner arguments. Dependent owner arguments, including `sizeof...(pack)`,
  still remain deferred for later specialization.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Step 6 focused parser proof after the variable-template value-argument
metadata repair:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_qualified_template_call_template_arg_perf|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$' > test_after.log 2>&1`

Result: passed, 13/13. `cpp_parse_record_base_variable_template_value_arg_dump`
now passes, and `cpp_qualified_template_call_template_arg_perf` remains green
at 4.68 sec.

Proof log: `test_after.log`.

Broader positive-Sema confirmation after repairing the supervisor-reported
regressions:
`ctest --test-dir build -j --output-on-failure -R '^cpp_positive_sema_'`

Result: passed, 884/884. The previously regressed
`cpp_positive_sema_inherited_static_member_lookup_runtime_cpp` and
`cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp`
are green, and `cpp_positive_sema_variadic_template_arg_sizeof_pack_parse_cpp`
remains green with dependent value-expression owner arguments deferred.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the final Step 6 full-suite timeout in
`cpp_qualified_template_call_template_arg_perf`.

The parser now rejects non-call-like template arguments before entering the
structured alias-template member-call NTTP carrier path, so ordinary template
arguments do not pay alias lookup or expression-fallback costs. The structured
carrier for `Alias<Ts...>::template member<Us...>()` now uses the parsed
`NK_CALL`/`NK_MEMBER` tree directly instead of also building a rendered
`$expr:` fallback string.

Template non-type parsing now treats leading `!` value-template expressions as
clearly value-like and captures them through the existing bounded token carrier
without a failed type/expression detour. The remaining speculative probes on
this hot path use lite rollback where they only need cursor-level state.

The member-template prelude parser also skips balanced defaults for unnamed
type template parameters. Those parameters do not create a structured prelude
binding, so their defaults are not retained metadata; consuming them
syntactically avoids reparsing thousands of `typename enable_if<...>::type`
defaults while preserving named/defaulted parameter carriers.

The exact full-suite proof improved from the supervisor baseline of 3022/3023
with only the perf timeout to 3023/3023. In the full-suite run,
`cpp_qualified_template_call_template_arg_perf` passed in 2.06s.

## Suggested Next

Recommended next Step 6 packet: supervisor-side regression review and commit
for the completed TypeSpec-tag deletion validation route, since the delegated
full-suite proof is now green.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- The alias-template member-call NTTP path remains structured: parsed owner
  and member template arguments attach to the call/member nodes, and semantic
  identity comes from those carriers, not from `$expr:` text.
- The prelude default skip is intentionally limited to unnamed type template
  parameters, because named parameters need their structured default metadata
  recorded for later parser/HIR handoff.
- The bounded token carrier is still used for non-call-like value-template
  expressions such as leading `!` trait expressions; avoid raw token capture
  for call-like alias-template member expressions.
- Keep rollback/lite-guard changes narrow. The main type-argument parser still
  uses the heavy guard where parser-debug lifecycle tests expect it.

## Proof

Acceptance proof:
`cmake --build build && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`

Result: passed, 3023/3023.

Proof log: `test_after.log`.

Focused perf check:
`ctest --test-dir build --output-on-failure -R '^cpp_qualified_template_call_template_arg_perf$'`

Result: passed, 1/1, 1.77s isolated.

Parser/EASTL/debug spot check:
`ctest --test-dir build -j --output-on-failure -R '^(cpp_eastl_integer_sequence_parse_recipe|cpp_eastl_type_traits_parse_recipe|cpp_eastl_utility_parse_recipe|cpp_eastl_vector_parse_recipe|cpp_eastl_memory_uses_allocator_parse_recipe|eastl_cpp_external_utility_frontend_basic_cpp|cpp_parse_record_base_variable_template_value_arg_dump|cpp_parse_template_alias_empty_pack_default_arg_dump|cpp_parser_debug_qualified_type_template_arg_stack|cpp_parser_debug_qualified_type_spelling_stack|cpp_parser_debug_tentative_template_arg_lifecycle|cpp_parser_debug_tentative_cli_only)$'`

Result: passed, 12/12.

Prior metadata/runtime spot check:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_prelim_eval_structured_metadata|c_testsuite_src_00019_c|llvm_gcc_c_torture_src_20040709_1_c|llvm_gcc_c_torture_src_pr40022_c|llvm_gcc_c_torture_src_pr23324_c|llvm_gcc_c_torture_src_20020402_3_c|llvm_gcc_c_torture_src_pr41463_c)$'`

Result: passed, 8/8.

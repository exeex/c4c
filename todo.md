# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/declarator.cpp` deferred template-owner member
guard cluster that started at the deletion-probe residual near line 661.
`parse_dependent_typename_specifier` now accepts local deferred-owner handoff
through `tpl_struct_origin_key`, `record_def`, `tag_text_id`, or an existing
`deferred_member_type_text_id` before falling back to explicitly legacy rendered
owner spelling. The packet added
`cpp_hir_parser_declarator_deferred_owner_structured_metadata`, which proves
stale rendered template-owner spelling does not replace structured owner/member
identity for a deferred `typename Owner<int>::type` handoff.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/base.cpp:3858`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Classify each deletion-probe failure as parser/HIR-owned,
  compatibility/display/final spelling, or downstream metadata gap.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- The deletion probe log for this packet is
  `/tmp/c4c_typespec_tag_deletion_probe_step4_declarator_deferred_owner.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 105 of 105
delegated tests, including the new
`cpp_hir_parser_declarator_deferred_owner_structured_metadata` test, the
existing parser type-base structured metadata tests, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=104 failed=0 total=104`
before and `passed=105 failed=0 total=105` after. There are no new failing
tests; the pass-count increase over the current baseline is the new parser
declarator deferred-owner structured-metadata test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_declarator_deferred_owner.log`.
The first emitted residual is now
`src/frontend/parser/impl/types/base.cpp:3858`, outside this packet's owned
files, so the targeted deferred template-owner member guard cluster around the
prior `declarator.cpp:661` residual is no longer first.

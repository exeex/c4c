# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/support.cpp` residual family that started at the
enum `sizeof(type)` dependency checks. Enum dependency classification now uses
`tag_text_id`/`template_param_text_id` before field-detected rendered fallback,
record layout lookup uses `record_def`/`tag_text_id` before no-metadata
compatibility tag-map lookup, and the remaining support helper rendered-tag
uses are explicit field-detected fallbacks. The packet added
`cpp_hir_parser_support_residual_structured_metadata` so the delegated
`cpp_hir_.*` proof observes one additional passing test.

## Suggested Next

Continue Step 4 with the next supervisor-selected residual family. The current
deletion probe first emits in `src/frontend/parser/impl/types/base.cpp` around
line 714, with broad parser type-base rendered-tag residuals following.

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
- `resolve_typedef_chain` in `support.cpp` still has only a rendered-string
  `tmap` signature, so this packet limited that site to field-detected
  compatibility access without inventing a new structured map contract.
- The deletion probe log for this packet is
  `/tmp/c4c_typespec_tag_deletion_probe_step4_support_residual.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 98 of 98
delegated tests, including the new
`cpp_hir_parser_support_residual_structured_metadata` test and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=97 failed=0 total=97`
before and `passed=98 failed=0 total=98` after. There are no new failing tests;
the pass-count increase is the new parser support residual test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --build --preset default` wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_support_residual.log`. The first
emitted residual is now `src/frontend/parser/impl/types/base.cpp:714`, and the
targeted `support.cpp` residual family is no longer first.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/base.cpp` struct/union/enum producer branch and
the adjacent `struct Pair<int>` template fall-through checks. Record producers
now assign `record_def`/`tag_text_id` metadata before compatibility spelling,
enum producers preserve `tag_text_id` and fixed-underlying metadata where
available, and rendered tag writes/reads in this branch are isolated behind
field-detected final-spelling compatibility helpers. The packet added
`cpp_hir_parser_type_base_record_enum_producer_structured_metadata` so the
delegated `cpp_hir_.*` proof observes one additional passing test.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits in the later
`src/frontend/parser/impl/types/base.cpp` template/member substitution cluster
around line 2673.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_record_enum_producers.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 102 of 102
delegated tests, including the new
`cpp_hir_parser_type_base_record_enum_producer_structured_metadata` test,
`cpp_hir_parser_type_base_producer_structured_metadata`, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=101 failed=0 total=101`
before and `passed=102 failed=0 total=102` after. There are no new failing
tests; the pass-count increase is the new parser type-base record/enum
producer test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_record_enum_producers.log`. The
first emitted residual is now `src/frontend/parser/impl/types/base.cpp:2673`,
so the targeted struct/union/enum producer branch around line 2240 is no
longer first.

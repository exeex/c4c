# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/base.cpp` pending-template materialization
subcluster that started at the deletion-probe residual near line 4484. Pending
argument-ref rendering now uses structured type-argument metadata before the
field-detected no-metadata rendered fallback, and direct instantiated-base
type-parameter substitution now binds through template parameter `TextId`
metadata before legacy rendered names. The packet added
`cpp_hir_parser_type_base_pending_base_substitution_structured_metadata`, which
proves `Derived<int>` materializes its pending `Base<T>` base as `Base<int>`
even when the rendered owner parameter spelling is stale and the structured
`template_param_text_id` remains authoritative.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/base.cpp:5828`, in the later member/field
substitution families that this packet was told not to touch.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_base_pending_base.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 107 of 107
delegated tests, including the new
`cpp_hir_parser_type_base_pending_base_substitution_structured_metadata` test,
the existing parser type-base structured metadata tests, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=106 failed=0 total=106`
before and `passed=107 failed=0 total=107` after. There are no new failing
tests; the pass-count increase over the current baseline is the new parser
type-base pending-base substitution structured-metadata test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_base_pending_base.log`. The first emitted
`base.cpp` residual is now
`src/frontend/parser/impl/types/base.cpp:5828`, outside this packet's owned
local pending-template materialization block, so the targeted residual around
the prior `base.cpp:4484` cluster is no longer first.

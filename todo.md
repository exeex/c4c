# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/base.cpp` instantiated-base fallback and
adjacent deferred-member base-resolution gate around the deletion-probe
residuals at lines 5828 and 5842. The fallback now writes rendered tags only
through the field-detected no-metadata compatibility helper, and deferred
member base resolution is gated by structured identity metadata before any
legacy rendered tag compatibility. The packet added
`cpp_hir_parser_type_base_instantiated_deferred_member_structured_metadata`,
which proves `Derived<int>` resolves a deferred `Owner<T>::type` base through
structured owner/member metadata after the legacy owner tag is cleared and the
rendered member name is stale.

## Suggested Next

Continue Step 4 with the next supervisor-selected parser type-base residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/base.cpp:5942`, in the later member typedef,
field, and method substitution families that this packet was told not to touch.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_base5828.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 108 of 108
delegated tests, including the new
`cpp_hir_parser_type_base_instantiated_deferred_member_structured_metadata`
test, the existing parser type-base structured metadata tests, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=107 failed=0 total=107`
before and `passed=108 failed=0 total=108` after. There are no new failing
tests; the pass-count increase over the current baseline is the new parser
type-base instantiated deferred-member structured-metadata test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway copy
of the working tree wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_base5828.log`. The first emitted
`base.cpp` residual is now
`src/frontend/parser/impl/types/base.cpp:5942`, outside this packet's owned
local instantiated-base fallback and deferred-member gate, so the targeted
`base.cpp:5828`/`5842` cluster is no longer first.

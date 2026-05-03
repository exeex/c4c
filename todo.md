# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/sema/consteval.cpp:182`, `src/frontend/sema/consteval.cpp:352`,
and `src/frontend/sema/type_utils.cpp:500` deletion-probe residuals.
Consteval type binding and record-layout lookups now use structured metadata
before explicit no-metadata rendered compatibility, and type-utils rendered-name
compatibility is deletion-safe and gated behind absence of structured name
metadata. A focused sema/HIR test now covers consteval record layout
owner-metadata precedence, metadata-miss rejection of stale rendered tags, and
type-binding rendered-name compatibility only when no structured metadata is
present.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe residual
family. The current deletion probe no longer emits the
`src/frontend/sema/consteval.cpp:182`, `src/frontend/sema/consteval.cpp:352`,
or `src/frontend/sema/type_utils.cpp:500` residuals; both owned sema files
compiled during the probe. The first emitted residual boundary is now outside
this packet's ownership at `src/frontend/sema/validate.cpp:418`.

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
- Keep rendered compatibility writes explicit and deletion-probe-safe; the
  positive ctor-init runtime case still needs the instantiated rendered tag as
  a compatibility payload for downstream member-expression lowering.
- The deletion probe log for this packet is
  `/tmp/c4c_typespec_tag_deletion_probe_step4_consteval_type_utils.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 111 of 111
delegated tests, including
`cpp_hir_sema_consteval_type_utils_structured_metadata`,
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: command exited 0. Guard passed with `passed=110 failed=0 total=110`
before and `passed=111 failed=0 total=111` after. There are no new failing
tests; the pass-count increase is the new
`cpp_hir_sema_consteval_type_utils_structured_metadata` test.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --build --preset default` wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_consteval_type_utils.log`. The
targeted `src/frontend/sema/consteval.cpp:182`,
`src/frontend/sema/consteval.cpp:352`, and
`src/frontend/sema/type_utils.cpp:500` residuals are cleared; `consteval.cpp`
and `type_utils.cpp` rebuilt during the probe. The current first emitted
residual is `src/frontend/sema/validate.cpp:418`, outside this packet's owned
files. `ast.hpp` was restored after the probe, and the delegated proof was
rerun successfully afterward.

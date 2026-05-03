# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/sema/validate.cpp:418` deletion-probe residual family. Validation
function type compatibility, pointer/record type compatibility, record
completeness, structured record key lookup, injected `this`, dependent
reference parameters, and typedef/cast placeholder checks now prefer
`record_def`, text/namespace metadata, structured keys, template parameter text
metadata, and explicit no-metadata rendered compatibility instead of direct
`TypeSpec::tag` access.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe residual
family. The current deletion probe no longer emits any
`src/frontend/sema/validate.cpp` residuals; `validate.cpp` rebuilt during the
probe. The first emitted residual boundary is now outside this packet's
ownership at `src/shared/llvm_helpers.hpp:444`.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_validate.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 111 of 111
delegated tests, including
`cpp_hir_sema_canonical_symbol_structured_metadata`,
`cpp_hir_sema_consteval_type_utils_structured_metadata`,
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: command exited 0. Guard passed with `passed=111 failed=0 total=111`
before and `passed=111 failed=0 total=111` after. There are no new failing
tests; the pass count is unchanged because this packet migrated existing
covered validate routes without adding a new testcase.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --build --preset default` wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_validate.log`. The targeted
`src/frontend/sema/validate.cpp:418` residual family is cleared; no
`validate.cpp` residuals are emitted, and the current first emitted residual is
`src/shared/llvm_helpers.hpp:444`, outside this packet's owned files.
`ast.hpp` was restored after the probe, and the delegated proof was rerun
successfully afterward.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/template.cpp:230` deletion-probe residual
cluster. Template instantiation result TypeSpecs now carry `record_def`,
`tag_text_id`, and namespace context as semantic metadata, with only a
deletion-safe display spelling setter retaining final compatibility text.
`decode_type_ref_text` now rebuilds prefixed record/enum references through
`tag_text_id`, and deferred visible typedef template-reference recovery uses
typedef/record structured metadata before falling back to deletion-safe display
compatibility.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe residual
family. The current deletion probe no longer emits the `template.cpp:230`,
`249`, `255`, `329`, or `742-743` cluster; the first emitted residual boundary
is now outside this packet's ownership at
`src/frontend/sema/canonical_symbol.cpp:146`.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_template.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 109 of 109
delegated tests, including `cpp_hir_template_recover_identity_metadata`,
`cpp_hir_template_realize_struct_metadata`,
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: command exited 0. Guard passed with `passed=109 failed=0 total=109`
before and `passed=109 failed=0 total=109` after. There are no new failing
tests; the pass count is unchanged because this packet migrated existing
covered template metadata routes without adding a new testcase.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --build --preset default` wrote
`/tmp/c4c_typespec_tag_deletion_probe_step4_template.log`. The targeted
`src/frontend/parser/impl/types/template.cpp:230`, `249`, `255`, `329`, and
`742-743` residuals are cleared; the current first emitted residual is
`src/frontend/sema/canonical_symbol.cpp:146`, outside this packet's owned
files. `ast.hpp` was restored after the probe, and a normal
`cmake --build --preset default` passed.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the targeted
`src/frontend/parser/impl/types/struct.cpp:847` deletion-probe residual
cluster. Record member alias typedef fallbacks, nested record member fields,
nested enum member fields, record conversion-operator typedef fallback, and
concrete record typedef bindings now use `tag_text_id`, `record_def`,
namespace/qualifier metadata, or deletion-safe display compatibility helpers
instead of directly requiring `TypeSpec::tag`.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe residual
family. The current deletion probe first emits outside this packet's ownership
at `src/frontend/parser/impl/types/template.cpp:230`; `struct.cpp` compiles
cleanly in the temporary `TypeSpec::tag` deletion build before the build stops.

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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_struct.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(eastl_cpp_external_utility_frontend_basic_cpp|frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 109 of 109
delegated tests, including the parser structured metadata tests,
`cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp`, and
`eastl_cpp_external_utility_frontend_basic_cpp`. `test_after.log` is the
canonical proof log.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: command exited 0. Guard passed with `passed=109 failed=0 total=109`
before and `passed=109 failed=0 total=109` after. There are no new failing
tests; the pass count is unchanged because this packet migrated existing
covered struct/record producer routes without adding a new testcase.

Deletion probe:

Temporarily removing `TypeSpec::tag` and running
`cmake --preset default && cmake --build --preset default` in a throwaway
worktree wrote `/tmp/c4c_typespec_tag_deletion_probe_step4_struct.log`. The
targeted `src/frontend/parser/impl/types/struct.cpp:847`, `951`, `984`,
`1037`, `1429`, and `2487` residuals are cleared; the current first emitted
residual is `src/frontend/parser/impl/types/template.cpp:230`, outside this
packet's owned files.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries repaired the reviewer-blocking
callable structured-miss fallback in
`src/frontend/hir/hir_functions.cpp::find_struct_def_for_callable_type`.

The callable helper now makes the complete structured owner-key lookup
authoritative: a structured hit returns the HIR struct definition, and a complete
structured miss returns `nullptr` before rendered `TypeSpec::tag` compatibility
lookup can run. Focused `frontend_hir_lookup_tests` coverage now proves the
callable zero-sized-return normalizer rejects a stale rendered zero-sized struct
after a complete structured owner-key miss.

## Suggested Next

Continue Step 4 with the next bounded frontend/HIR deletion-probe cluster from
the remaining inventory. The callable helper blocker in `hir_functions.cpp` is
cleared; remaining probe inventory still includes parser-owned semantic
producers/consumers and first-failure HIR clusters in
`src/frontend/hir/impl/compile_time/compile_time_engine.hpp`,
`src/frontend/hir/hir_types.cpp`, `src/frontend/hir/hir_ir.hpp`,
`src/frontend/hir/hir_lowering_core.cpp`,
`src/frontend/hir/impl/expr/builtin.cpp`, and
`src/frontend/hir/hir_build.cpp`.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The callable zero-sized-return path should keep rendered `TypeSpec::tag`
  compatibility only when no complete structured owner metadata exists.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Do not create downstream follow-up ideas until a probe reaches LIR/BIR/backend
  failures after frontend/HIR compile blockers are cleared.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not create new follow-up ideas for parser/HIR work that still belongs in
  this runbook.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 and `test_after.log` was preserved as the canonical
executor proof log. The build passed; CTest ran 73 HIR tests and all passed.
`test_before.log` was left untouched until supervisor regression comparison.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with before 73/73 and after 73/73. Supervisor then rolled
`test_after.log` forward to `test_before.log` for the next packet.

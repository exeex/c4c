# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the bounded
`src/frontend/hir/impl/expr/expr.cpp::is_ast_lvalue` cast lvalue family.

`is_ast_lvalue` now treats `TypeSpec::template_param_text_id` plus
`FunctionCtx::tpl_bindings_by_text` as authoritative when present. It no longer
falls through to rendered `TypeSpec::tag` after a structured template-param
binding miss. The remaining direct `TypeSpec::tag` read in `expr.cpp` is
centralized behind `compatibility_lvalue_template_binding_tag` and is reached
only for no-TextId TypeSpecs. Focused `frontend_hir_lookup_tests` coverage now
proves TextId-backed lvalue cast binding wins over stale rendered tags and that
structured misses reject stale rendered-tag bindings.

## Suggested Next

Continue Step 4 with the next bounded frontend/HIR deletion-probe cluster, likely
`src/frontend/hir/impl/expr/call.cpp` template-argument and call-helper tag
reads, keeping structured template-param, record, and value-argument metadata
authoritative where present.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- `is_ast_lvalue` is static and has no `Lowerer::module_` access, so this slice
  intentionally uses `tpl_bindings_by_text` for structured hits instead of a
  TextTable-derived string bridge.
- There is no record-owner lookup in `is_ast_lvalue`; record metadata remains a
  boundary for the later call/object/operator/type clusters.
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

Result: command exited 0 and `test_after.log` was preserved. The build passed;
CTest ran 73 HIR tests and all passed.

Regression guard:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result: passed with before 73/73 and after 73/73.

Controlled deletion probe:

1. Temporarily removed `TypeSpec::tag` from
   `src/frontend/parser/ast.hpp`.
2. Ran `cmake --build --preset default`; result failed as expected during
   frontend/HIR compilation. The probe did not report
   `src/frontend/hir/impl/expr/expr.cpp`, confirming the `is_ast_lvalue`
   compile blocker is cleared.
3. Restored `src/frontend/parser/ast.hpp` exactly to the pre-probe state.
4. Ran `cmake --build --preset default`; result passed.
5. Ran `git diff --check`; result passed.

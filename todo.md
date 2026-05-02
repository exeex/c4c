# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries repaired the
`src/frontend/hir/impl/stmt/decl.cpp` route drift from
`review/step4_post_decl_checkpoint_review.md`. Aggregate direct-assignment
compatibility now refuses rendered-spelling fallback when either side has
structured owner identity and structured owner resolution misses, covering the
array element and initializer-list aggregate direct-assignment decisions.

Focused coverage in `tests/frontend/frontend_hir_lookup_tests.cpp` builds stale
rendered tags with unresolved structured record metadata and proves those tags
do not decide the local aggregate direct-assignment paths.

## Suggested Next

Return Step 4 to the next deletion-probe blocker in
`src/frontend/hir/impl/stmt/range_for.cpp`, with the same rule: replace direct
`TypeSpec::tag` reads with structured owner/text metadata and do not let
rendered spelling become a semantic fallback after a complete structured miss.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- Step 4 is a probe, not the final field removal. Do not commit a broken
  deletion build.
- Classify each failure as parser/HIR-owned, compatibility/display/final
  spelling, or downstream metadata gap instead of fixing broad clusters inside
  the probe packet.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Treat any `TypeSpec::tag` deletion build as temporary until Step 5.
- `decl.cpp` is semantically cleared for the reviewed direct aggregate
  structured-miss fallback, but the local rendered-spelling helper still exists
  for compatibility/display-only callers without structured identity.
- The inherited-base aggregate cast TypeSpec now preserves final spelling with a
  deletion-safe write helper and copies `tag_text_id`/namespace metadata from
  the resolved base layout when available.
- The current deletion probe first reports direct `TypeSpec::tag` reads in
  `src/frontend/hir/impl/stmt/range_for.cpp`, followed in the same failed build
  by `src/frontend/hir/impl/stmt/stmt.cpp` and
  `src/frontend/hir/impl/templates/deduction.cpp`. The same probe also reaches
  later `src/frontend/hir/impl/templates/global.cpp` errors after those
  statement/template-deduction clusters.
- Non-canonical deletion probe artifacts for recent packets include
  `/tmp/c4c_typespec_tag_deletion_probe_step4_call_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_next.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_object_new_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_delete_expr.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_operator.log`,
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl.log`, and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl_direct_agg.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit, and
`test_after.log` was preserved as the canonical executor proof log. The build
passed, and CTest passed 72 of 72 delegated tests.

Focused local coverage:

`cmake --build --preset default --target frontend_hir_lookup_tests`

`ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`

Result: both commands exited 0. This focused test binary is the active preset
home for the new stale-rendered direct aggregate regression.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_decl_direct_agg.log 2>&1`, and
restored the temporary edit. The probe moved past
`src/frontend/hir/impl/stmt/decl.cpp`. The first residual errors are direct
`TypeSpec::tag` reads in `src/frontend/hir/impl/stmt/range_for.cpp`, with
adjacent same-build residuals in `src/frontend/hir/impl/stmt/stmt.cpp` and
`src/frontend/hir/impl/templates/deduction.cpp`; later output also reaches
`src/frontend/hir/impl/templates/global.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.

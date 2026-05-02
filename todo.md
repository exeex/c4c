# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries cleared the
`src/frontend/hir/impl/stmt/decl.cpp` deletion-probe blocker. Local-declaration
struct-owner resolution, aggregate direct-assignment comparison, initializer-list
field/member owner lookup, and inherited-base aggregate lvalue construction now
route through structured owner metadata, `tag_text_id`, and deletion-safe
final-spelling compatibility helpers instead of direct `TypeSpec::tag` reads or
writes. `decl.cpp` no longer has deletion-probe-blocking direct
`TypeSpec::tag` reads or writes outside SFINAE compatibility bridges.

## Suggested Next

Continue Step 4 by taking the next deletion-probe blocker in
`src/frontend/hir/impl/stmt/range_for.cpp`, keeping it scoped to range-for
statement owner/member lookup and leaving the larger `stmt.cpp` and
`templates/deduction.cpp` residual clusters for separate packets unless a shared
statement-owner helper is required.

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
- `decl.cpp` now keeps legacy spelling access behind local deletion-safe
  compatibility helpers. Do not use those helpers as a semantic owner shortcut
  when `record_def`, template origin/args, or complete `tag_text_id` metadata is
  available.
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
  `/tmp/c4c_typespec_tag_deletion_probe_step4_operator.log`, and
  `/tmp/c4c_typespec_tag_deletion_probe_step4_decl.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0 after restoring the temporary deletion-probe edit, and
`test_after.log` was preserved as the canonical executor proof log. The build
passed, and CTest passed 72 of 72 delegated tests.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_decl.log 2>&1`, and restored
the temporary edit. The probe moved past
`src/frontend/hir/impl/stmt/decl.cpp`. The first residual errors are direct
`TypeSpec::tag` reads in `src/frontend/hir/impl/stmt/range_for.cpp`, with
adjacent same-build residuals in `src/frontend/hir/impl/stmt/stmt.cpp` and
`src/frontend/hir/impl/templates/deduction.cpp`; later output also reaches
`src/frontend/hir/impl/templates/global.cpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.

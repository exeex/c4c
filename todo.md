# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries moved
`src/frontend/hir/impl/stmt/decl.cpp` past direct deletion-probe blocking
`TypeSpec::tag` reads/writes, but the reviewer found the route is not fully
cleared semantically. Aggregate direct-assignment paths can still fall through
to rendered compatibility spelling after both structured owner resolutions miss:
array element aggregate comparison around `decl.cpp:780-801` and
initializer-list aggregate comparison around `decl.cpp:936-959`.

Review absorbed from `review/step4_post_decl_checkpoint_review.md`: the
structured-owner-miss fallback is route drift from the source idea because once
record-def/template metadata is classified as structured identity, a complete
structured miss must not recover semantic success through rendered spelling
equality from `tag_text_id` or legacy `TypeSpec::tag`.

## Suggested Next

Continue Step 4 with a narrow `src/frontend/hir/impl/stmt/decl.cpp`
repair/test packet before returning to `src/frontend/hir/impl/stmt/range_for.cpp`.
The packet should block rendered-spelling compatibility fallback when both sides
have complete structured metadata but owner resolution fails, and add focused
coverage for stale rendered spelling in the direct aggregate comparison paths.
If that cannot stay local to `decl.cpp`, stop and request an explicit split
before resuming the `range_for.cpp` deletion-probe blocker.

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
- `decl.cpp` is not semantically cleared yet: deletion-safe compatibility
  helpers must not become semantic owner shortcuts after complete structured
  metadata exists and structured owner lookup fails.
- The next executor packet should constrain the local compatibility route
  instead of moving to `range_for.cpp` first.
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

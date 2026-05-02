# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
`resolve_struct_member_typedef_if_ready` readiness guard in
`src/frontend/hir/impl/templates/type_resolution.cpp` away from direct
`TypeSpec::tag` reads. The guard now discovers owner readiness through
structured `record_def`, structured `tag_text_id` owner metadata, template
origin state, or explicit no-metadata legacy rendered-tag compatibility. The
non-template owner lookup now uses the same structured owner tag when available
and blocks stale rendered-tag recovery when a structured `tag_text_id` carrier
is present but misses. Added focused coverage in
`cpp_hir_member_typedef_readiness_structured_metadata` proving structured
`tag_text_id` owner metadata beats a stale rendered tag and a structured miss
does not recover through the stale rendered tag.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. The
current probe's first emitted errors are still in
`src/frontend/hir/impl/templates/type_resolution.cpp`, now downstream of the
readiness guard in the member-typedef binding and nested-owner fallback paths.
Keep those type-resolution residuals split from the parser/core residuals
unless the supervisor routes them together.

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
- `resolve_typedef_type_chain` is now semantically cleared for structured
  typedef-chain lookup. Its remaining rendered-tag fallback is compatibility
  only, disabled whenever structured typedef lookup identity is present, and
  uses a field-detection helper so the Step 4 deletion probe classifies later
  residuals first.
- `resolve_struct_like_typedef_type` is now semantically cleared for
  structured typedef lookup. Its remaining rendered-tag fallback is
  compatibility only, disabled whenever structured typedef lookup identity is
  present, and uses a field-detection helper so the Step 4 deletion probe
  classifies later residuals first.
- `resolve_struct_member_typedef_if_ready` readiness is now semantically
  cleared for structured owner metadata. Its no-metadata rendered-tag fallback
  is explicit and field-detection guarded, and a structured `tag_text_id` miss
  no longer falls back to a stale rendered tag in the non-template owner path.
- Deletion probe residuals after this packet no longer include the targeted
  readiness guard around former
  `src/frontend/hir/impl/templates/type_resolution.cpp:438`. Same-build
  residuals begin later in the same file at current probe lines 496/502
  (`resolved_member.tag` template-binding fallback), then 588 and 619/622
  (nested deferred-member/template-owner rendered-tag checks). The first
  remaining parser/core residual is current
  `src/frontend/parser/impl/core.cpp:1073`, with later parser/core residuals
  around current lines 1077-1081, 1167, 1183, and 1872-1882.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_hir_readiness.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 85 of 85 delegated
tests after this packet, increasing the focused subset with new
`cpp_hir_member_typedef_readiness_structured_metadata` coverage for the
migrated HIR readiness guard and non-template owner lookup. `test_after.log`
is the canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_hir_readiness.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`resolve_struct_member_typedef_if_ready` readiness-guard direct
`TypeSpec::tag` blocker around former line 438. The first emitted errors are
now later in `src/frontend/hir/impl/templates/type_resolution.cpp` around
current probe lines 496/502, followed by 588 and 619/622; parser/core
residuals still include current `src/frontend/parser/impl/core.cpp:1073`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.

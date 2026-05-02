# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
`resolve_struct_like_typedef_type` lookup in
`src/frontend/parser/impl/core.cpp` away from direct `TypeSpec::tag` reads.
The helper now resolves struct-like typedefs from structured `TypeSpec`
metadata first and keeps the rendered-tag compatibility path explicit, gated
to no structured typedef lookup carrier, and hidden behind a field-detection
helper so a `TypeSpec::tag` deletion probe moves past this blocker. Added
focused coverage in
`cpp_hir_parser_core_struct_like_typedef_structured_metadata` proving
`tag_text_id` metadata wins over stale rendered tags and a
structured-metadata miss does not fall back to a matching rendered tag.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. The
current probe's first emitted errors are downstream in
`src/frontend/hir/impl/templates/type_resolution.cpp`. The first remaining
parser/core deletion-probe blocker is now in `are_types_compatible` around
current `src/frontend/parser/impl/core.cpp:1073`, followed by parser
type-helper residuals in `src/frontend/parser/impl/types/types_helpers.hpp`.
Keep parser/core residuals, parser type-helper residuals, and downstream HIR
template type-resolution residuals split unless the supervisor routes them
together.

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
- Deletion probe residuals after this packet no longer include the targeted
  `src/frontend/parser/impl/core.cpp:1014` direct read. Same-build residuals
  begin in `src/frontend/hir/impl/templates/type_resolution.cpp:438`; the first
  remaining parser/core residual is current `src/frontend/parser/impl/core.cpp:1073`,
  with later parser/core residuals around current lines 1077-1081, 1167, 1183,
  and 1872-1882.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_struct_like_typedef.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 83 of 83 delegated
tests before this packet and 84 of 84 delegated tests after this packet,
including new
`cpp_hir_parser_core_struct_like_typedef_structured_metadata` coverage for the
migrated parser/core struct-like typedef path. `test_after.log` is the
canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_struct_like_typedef.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`resolve_struct_like_typedef_type` direct `TypeSpec::tag` blocker around
former line 1014. The first emitted errors are now downstream in
`src/frontend/hir/impl/templates/type_resolution.cpp:438`, and the first
remaining parser/core residual blocker is current
`src/frontend/parser/impl/core.cpp:1073`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
`resolve_typedef_type_chain` lookup in `src/frontend/parser/impl/core.cpp` away
from direct `TypeSpec::tag` reads. The helper now resolves typedef chains from
structured `TypeSpec` metadata first and keeps the rendered-tag compatibility
path explicit, gated to no structured typedef lookup carrier, and hidden behind
a field-detection helper so a `TypeSpec::tag` deletion probe moves past this
blocker. Added focused coverage in
`cpp_hir_parser_core_typedef_chain_structured_metadata` proving `tag_text_id`
metadata wins over stale rendered tags and a structured-metadata miss does not
fall back to a matching rendered tag.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. The
current probe's first emitted parser/core blocker is now the adjacent
`resolve_struct_like_typedef_type` rendered-tag compatibility path in
`src/frontend/parser/impl/core.cpp` around current line 1014. Keep later
parser/core residuals, parser type-helper residuals, and downstream HIR
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
- Deletion probe residuals after this packet begin at
  `src/frontend/parser/impl/core.cpp:1014`, with same-build parser/core
  residuals around current lines 1023, 1071, 1075-1079, 1165, 1181, and
  1870-1880. Same-build downstream residuals remain in
  `src/frontend/hir/impl/templates/type_resolution.cpp` and
  `src/frontend/parser/impl/types/types_helpers.hpp`.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_parser_core_typedef_chain.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 83 of 83 delegated
tests, including new
`cpp_hir_parser_core_typedef_chain_structured_metadata` coverage for the
migrated parser/core typedef-chain path. `test_after.log` is the canonical
proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_parser_core_typedef_chain.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`resolve_typedef_type_chain` direct `TypeSpec::tag` blocker around former line
967. The first emitted parser/core residual blocker is now
`src/frontend/parser/impl/core.cpp:1014`; same-build residuals include later
parser/core direct reads around current lines 1023, 1071, 1075-1079, 1165,
1181, and 1870-1880, plus downstream residuals in
`src/frontend/hir/impl/templates/type_resolution.cpp` and
`src/frontend/parser/impl/types/types_helpers.hpp`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above is green after reverting the temporary edit.

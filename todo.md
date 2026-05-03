# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
parser/core producer assignments in `register_synthesized_typedef_binding`,
`register_tag_type_binding`, and builtin `__true_type`/`__false_type` setup
around current `src/frontend/parser/impl/core.cpp:1187`, `1206`, `1893`, and
`1904`. These producers now write structured `tag_text_id` identity first, and
their rendered `TypeSpec::tag` payload is assigned only through an explicit
field-detected compatibility helper. Added focused parser-core coverage in
`cpp_hir_parser_core_producer_structured_metadata`.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. The
current first emitted residuals are parser type-helper display/key/mangling
uses in `src/frontend/parser/impl/types/types_helpers.hpp` around current
probe lines 117, 145, 467, 1168, 1211-1231, and 1943-1946.

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
- `resolve_struct_member_typedef_if_ready` origin member-typedef
  template-binding is now semantically cleared for structured template
  parameter metadata. Its no-metadata rendered-tag fallback is explicit through
  `find_template_typedef_binding`, and structured owner mismatch does not fall
  back to stale rendered names.
- `resolve_struct_member_typedef_if_ready` nested deferred-member readiness is
  now semantically cleared for structured owner/deferred-member metadata. Its
  no-metadata rendered-tag fallback is explicit and field-detection guarded,
  and nested record-def metadata can recurse without any rendered tag.
- `resolve_struct_member_typedef_if_ready` template-owner realized lookup is
  now semantically cleared for structured owner metadata. Its no-metadata
  rendered-tag fallback is explicit and field-detection guarded, and realized
  template owners can resolve inherited member typedefs through the HIR struct
  `tag_text_id`/owner index path.
- `recover_template_struct_identity_from_tag` is now semantically cleared for
  structured template/record owner metadata. Its no-metadata rendered-tag
  fallback is explicit and field-detection guarded.
- `resolves_to_record_ctor_type` is now semantically cleared for structured
  record-constructor metadata. Its no-metadata rendered-tag fallback is
  explicit and field-detection guarded.
- `register_synthesized_typedef_binding`, `register_tag_type_binding`, and
  builtin `__true_type`/`__false_type` setup are now semantically cleared as
  parser/core producers. `tag_text_id` carries the typedef/tag identity, and
  rendered spelling assignment is explicit field-detected compatibility only.
- Deletion probe residuals from this packet no longer include the targeted
  direct `TypeSpec::tag` producer assignments around former
  `src/frontend/parser/impl/core.cpp:1169`, `1185`, `1874`, or `1884`.
  Current same-build deletion-probe residuals first emit in
  `src/frontend/parser/impl/types/types_helpers.hpp` around probe lines 117,
  145, 467, 1168, 1211-1231, and 1943-1946. Later residuals include
  `src/frontend/parser/impl/declarations.cpp` display/final-spelling uses and
  downstream HIR template value-arg uses that were outside this packet.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_parser_core_producers.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 91 of 91
delegated tests, increasing the focused subset with new
`cpp_hir_parser_core_producer_structured_metadata` coverage for the migrated
producer metadata. `test_after.log` is the canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_parser_core_producers.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
parser/core producer assignments around former
`src/frontend/parser/impl/core.cpp:1169`, `1185`, `1874`, and `1884`.

Result: command exited 1 as expected for the controlled deletion probe. The
first emitted errors are now in
`src/frontend/parser/impl/types/types_helpers.hpp` around probe lines 117, 145,
467, 1168, 1211-1231, and 1943-1946. The normal delegated proof above was
rerun green after restoring the temporary edit.

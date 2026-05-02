# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
`recover_template_struct_identity_from_tag` recovery path in
`src/frontend/hir/impl/templates/value_args.cpp` away from requiring direct
`TypeSpec::tag` access before semantic recovery. The helper now tries complete
structured template-origin key metadata, `record_def` owner metadata,
`tag_text_id` owner-key/text-table metadata, and owner indexes first. The
rendered `tag` path remains only as an explicit field-detected compatibility
fallback when no structured owner metadata is available. Added focused HIR
coverage in `cpp_hir_template_recover_identity_structured_metadata` for
recovering template identity from record metadata without a rendered tag.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. If
the supervisor keeps the packet in HIR `value_args.cpp`, the next visible
residual family is in `resolve_member_lookup_owner_tag` and adjacent template
argument recovery paths around current probe lines 388/403/416/438. The
current first emitted deletion-probe errors are parser/core residuals, so keep
parser/core work split unless explicitly routed.

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
- Deletion probe residuals from this packet no longer include the targeted
  direct `TypeSpec::tag` recovery guard around former
  `src/frontend/hir/impl/templates/value_args.cpp:175`. Current same-build
  deletion-probe residuals first emit in
  `src/frontend/parser/impl/core.cpp:1073`, with parser/core residuals around
  current lines 1077-1081, 1167, 1183, and 1872-1882. Later HIR residuals in
  this same probe include `src/frontend/hir/impl/templates/value_args.cpp`
  around current probe lines 388, 403, 416, 438, 468, 474, 492, and 495.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_hir_value_args_recover.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 89 of 89 delegated
tests after this packet, increasing the focused subset with new
`cpp_hir_template_recover_identity_structured_metadata` coverage for the
migrated recovery helper. `test_after.log` is the canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_hir_value_args_recover.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`recover_template_struct_identity_from_tag` direct rendered-tag dependency
around former `src/frontend/hir/impl/templates/value_args.cpp:175`.

Result: command exited 1 as expected for the controlled deletion probe. The
first emitted errors are now in `src/frontend/parser/impl/core.cpp:1073`, with
later HIR residuals in `src/frontend/hir/impl/templates/value_args.cpp` around
current probe lines 388/403/416/438. The normal delegated proof above was
rerun green after restoring the temporary edit.

# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Probe Field Removal And Split Boundaries

## Just Finished

Step 4 - Probe Field Removal And Split Boundaries migrated the targeted
`resolve_struct_member_typedef_if_ready` template-owner fallback in
`src/frontend/hir/impl/templates/type_resolution.cpp` away from direct
`owner_ts.tag` lookup. Realized template owners now resolve member typedefs
through structured owner metadata first, including the realized HIR struct
`tag_text_id` and template-instance owner index, with only explicit
no-metadata legacy compatibility left behind. The same helper now carries the
rendered owner key into base-chain lookup so realized template-owner metadata
can find inherited member typedefs without consulting `TypeSpec::tag`. Added
focused coverage in
`cpp_hir_member_typedef_realized_owner_structured_metadata`.

## Suggested Next

Continue Step 4 with the next supervisor-selected deletion-probe blocker. The
current probe no longer reports the targeted
`resolve_struct_member_typedef_if_ready` `owner_ts.tag` template-owner
fallback; first emitted errors now begin in out-of-scope
`src/frontend/hir/impl/templates/value_args.cpp`, followed by parser/core
residuals. Keep those split unless the supervisor routes them together.

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
- Deletion probe residuals from the previous packet no longer include the
  targeted readiness guard around former
  `src/frontend/hir/impl/templates/type_resolution.cpp:438`. Same-build
  residuals from this packet no longer include the targeted
  `resolved_member.tag` template-binding fallback around current pre-change
  lines 496/502 or the targeted nested deferred-member readiness check around
  current pre-change line 574. Same-build residuals from this packet no longer
  include the targeted `owner_ts.tag` template-owner fallback around current
  pre-change lines 618/621. Current same-build residuals now begin in
  out-of-scope `src/frontend/hir/impl/templates/value_args.cpp` around current
  probe line 175. The first remaining parser/core residual is current
  `src/frontend/parser/impl/core.cpp:1073`, with later parser/core residuals
  around current lines 1077-1081, 1167, 1183, and 1872-1882.
- This packet added
  `/tmp/c4c_typespec_tag_deletion_probe_step4_hir_owner_ts.log`.

## Proof

Executor proof:

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|cpp_positive_sema_ctor_init_piecewise_delegating_template_runtime_cpp|frontend_hir_tests|cpp_hir_.*)$"' > test_after.log 2>&1`

Result: command exited 0. The build passed, and CTest passed 88 of 88 delegated
tests after this packet, increasing the focused subset with new
`cpp_hir_member_typedef_realized_owner_structured_metadata` coverage for the
migrated template-owner fallback. `test_after.log` is the canonical proof log.

Deletion probe:

Temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp`, ran
`bash -lc 'cmake --build --preset default' >
/tmp/c4c_typespec_tag_deletion_probe_step4_hir_owner_ts.log 2>&1`,
and restored the temporary edit. The probe moved past the targeted
`resolve_struct_member_typedef_if_ready` template-owner `owner_ts.tag`
fallback around current pre-change lines 618/621. The first emitted errors are
now in `src/frontend/hir/impl/templates/value_args.cpp` around current probe
line 175; parser/core residuals still include current
`src/frontend/parser/impl/core.cpp:1073`.

Result: command exited 1 as expected for the controlled deletion probe, and the
normal build proof above was rerun green after reverting the temporary edit.

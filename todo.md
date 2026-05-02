# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate HIR Type And Record Consumers

## Just Finished

Step 3 - Migrate HIR Type And Record Consumers migrated inherited static
member evaluation in
`src/frontend/hir/impl/templates/templates.cpp::eval_struct_static_member_value_hir`.
For base-class traversal, a base `TypeSpec` carrying `record_def` now recurses
through that structured AST owner before rendered `base_ts.tag`, and a
structured base miss no longer falls through to a stale rendered base tag.
Rendered `base_ts.tag`/`struct_defs` lookup remains explicit compatibility for
base TypeSpecs without `record_def`.

Focused coverage in `frontend_hir_lookup_tests` proves inherited static member
evaluation returns the value from the structured `record_def` base despite a
stale rendered base tag, and proves a structured base miss does not fall back
to a stale rendered base that defines the member.

## Suggested Next

Continue Step 3 with another bounded HIR `TypeSpec::tag` consumer where
structured owner metadata is already present. A good next packet is a narrow
template/type route that still indexes by rendered `tag`, such as a
`src/frontend/hir/impl/templates/value_args.cpp` caller that can pass
`record_def`, `tag_text_id`, owner keys, or template metadata instead of only
rendered owner spelling, or a remaining HIR `hir_types.cpp` semantic equality
or typedef-to-struct route with existing structured carriers.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic field.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads.
- The deferred member typedef resolver still intentionally uses rendered
  `resolve_struct_member_typedef_type(tag, ...)` for no-metadata compatibility
  and for concrete realized template-origin base traversal after origin
  materialization.
- Function specialization TypeSpec argument matching still intentionally uses
  rendered `tag` comparison when both sides lack structured nominal metadata.
  Template parameter name lookup remains keyed by rendered parameter names in
  `TypeBindings`; migrating that boundary needs a separate structured template
  parameter binding packet.
- Repeated template argument deduction consistency now compares structured
  TypeSpec nominal identity, but the `TypeBindings` map keys remain rendered
  template parameter names by packet contract. Pointer-depth deduction still
  compares only the pre-existing shape fields and does not use nominal tag
  identity in this packet.
- Canonical template struct primary lookup now blocks stale rendered origin
  fallback when `record_def` has a complete owner key but no structured entry.
  Rendered primary-name fallback remains for declarations without owner-key
  metadata. Template struct specialization lookup by primary owner remains a
  separate rendered-fallback surface for a later packet if needed.
- Inherited static member evaluation now uses `base_ts.record_def` before
  rendered `base_ts.tag` and blocks stale rendered fallback after a structured
  base miss. The function still has no HIR owner-key map, so `tag_text_id` and
  `HirRecordOwnerKey` lookup are residual surfaces for a later helper or API
  packet if needed.
- The default preset used for this packet does not register
  `frontend_hir_tests`; focused coverage for this route was therefore added to
  `frontend_hir_lookup_tests`, which the delegated regex compiles and runs.
- Do not weaken tests, mark supported cases unsupported, or add named-case
  shortcuts.
- Keep downstream LIR/BIR/backend carrier gaps as separate follow-up ideas
  instead of broadening this runbook.
- Do not attempt `TypeSpec::tag` field deletion in Step 3; removal belongs to
  the later deletion/probe and removal steps.
- Treat a `TypeSpec::tag` deletion build as a temporary probe only until the
  runbook reaches the removal step.

## Proof

Step 3 delegated proof passed with:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_.*template.*|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

Result: build passed and 60/60 selected tests passed. Proof log:
`test_after.log`. `git diff --check` passed.

# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Structured HIR Record-Layout Handoff

## Just Finished

Plan-owner review accepted Step 2 - Migrate HIR NTTP And Consteval Handoff as
complete enough for this runbook. The latest executor packet migrated the
remaining metadata-backed HIR static constexpr struct/member NTTP const eval
route to prefer a `NttpTextBindings` mirror before rendered compatibility, and
classified residual `NttpBindings` uses as compatibility payloads,
mangling/spec-key payloads, synthetic pack-key compatibility, or deferred
expression/debug-text boundaries.

Step 3 - Add Structured HIR Record-Layout Handoff is now the active execution
step.

## Suggested Next

Start Step 3 with one narrow structured HIR record-layout handoff: trace the
current `ConstEvalEnv::struct_defs` setup from HIR, add or thread a structured
record-owner carrier using existing `HirRecordOwnerKey` /
`struct_def_owner_index` metadata where available, and keep rendered
`TypeSpec::tag` / `struct_defs` lookup only as explicit no-metadata
compatibility.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.
- Do not count mangled/link-visible strings or HIR dump strings as cleanup
  targets unless they are used to recover semantic identity.
- Record-layout cleanup is a separate boundary from NTTP cleanup: HIR has
  `HirRecordOwnerKey`/`struct_def_owner_index`, but Sema consteval does not yet
  receive a structured layout map.
- Residual rendered `NttpBindings` uses should not be removed mechanically:
  several are ABI/display/cache payloads, pack synthetic-key compatibility, or
  deferred expression/debug-text boundaries without a complete structured
  carrier.
- Step 3 may require a minimal HIR-owned env bridge to Sema consteval because
  `ConstEvalEnv::struct_defs` is currently rendered-keyed. Do not widen into
  parser/Sema cleanup beyond the bridge needed to consume HIR record-owner
  metadata.
- Tests should prove structured record-layout lookup wins when rendered record
  spelling drifts; do not rely only on unchanged happy-path `sizeof` /
  `alignof` cases.

## Proof

Step 2 delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

No new validation was run for this lifecycle-only Step 2 review and Step 3
pointer update.

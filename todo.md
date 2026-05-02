# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Record Boundaries And Validate

## Just Finished

Plan-owner review accepted Step 5 - Preserve Display And Link Text Boundaries
as complete enough for this runbook. Step 5 classified display, diagnostic,
dump, final spelling, link-visible, mangling, generated-name, and compatibility
strings touched by Steps 2-4 as payloads rather than semantic authority.
Focused preservation coverage now verifies a structured-selected global keeps
its `LinkNameId` / link-visible payload.

Step 6 - Record Boundaries And Validate is now the active execution step.

## Suggested Next

Start Step 6 by auditing the remaining boundaries recorded in `todo.md` and
the final diff. Create separate `ideas/open/` follow-up files only for true
missing upstream/downstream metadata initiatives, then run supervisor-selected
broader validation and record the final proof.

## Watchouts

- Do not edit parser/Sema, LIR, BIR, or backend code except to record a
  boundary or bridge a HIR-owned blocker required by the active plan.
- Do not weaken tests, mark supported routes unsupported, or add named-case
  shortcuts.
- Preserve diagnostics, dumps, mangling, ABI/link-visible text, and final
  spelling as payloads while removing rendered strings as semantic authority.
- Do not count mangled/link-visible strings or HIR dump strings as cleanup
  targets unless they are used to recover semantic identity.
- Record-layout cleanup is a separate boundary from NTTP cleanup. Sema
  consteval, HIR builtin layout queries, global aggregate-init normalization,
  and implicit-this member recovery now receive/use owner metadata, but several
  HIR lowering/layout consumers still query rendered `Module::struct_defs`
  directly.
- Step 3 residual direct `struct_defs` consumers are classified boundaries, not
  Step 4 targets by default. Step 4 should focus on legacy declaration,
  function/global/template/member lookup authority unless a rendered
  `struct_defs` route is also acting as normal semantic lookup with an existing
  structured carrier.
- Residual rendered `NttpBindings` uses should not be removed mechanically:
  several are ABI/display/cache payloads, pack synthetic-key compatibility, or
  deferred expression/debug-text boundaries without a complete structured
  carrier.
- `ConstEvalEnv::struct_defs` remains rendered-keyed by design in this packet;
  `struct_def_owner_index` only selects the authoritative rendered layout entry
  when `TypeSpec` metadata can form a complete `HirRecordOwnerKey`.
- Template-instantiation record-owner keys need specialization identity; this
  packet covers declaration-owner layout lookup only. Do not infer that all
  template-record layout paths are structured yet.
- Do not mechanically replace `struct_defs` lookups in dump/codegen/template
  materialization paths: those often consume rendered storage names or generated
  specialization names as compatibility payloads, not semantic lookup authority.
- Step 5 is an audit/preservation step, not a license to remove every remaining
  string. Link-visible names, mangled names, generated specialization names,
  HIR dumps, diagnostics, and explicit no-metadata compatibility routes should
  remain strings when they are payloads rather than lookup authority.
- Preserve `LegacyRendered` tests for missing or incomplete metadata; Step 4
  moved metadata-backed callers to central structured-first resolver APIs
  without deleting compatibility fallback.
- Step 6 should decide which residual boundaries are acceptable compatibility
  for this runbook versus separate open ideas. Do not hide true missing carrier
  work only in `todo.md` if it needs durable follow-up.
- Step 6 validation should be broader than the narrow executor subset if the
  supervisor treats this as a runbook milestone.

## Proof

Step 5 focused pre-proof passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$'`.

Step 5 delegated proof passed and wrote `test_after.log`:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_hir_.*|frontend_parser_lookup_authority_tests|cpp_positive_sema_.*deferred_nttp.*|cpp_positive_sema_.*consteval.*)$' | tee test_after.log`.

No new validation was run for this lifecycle-only Step 5 review and Step 6
pointer update.

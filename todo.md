# Current Packet

Status: Active
Source Idea Path: ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Record Boundaries And Validate

## Just Finished

Step 6 - Record Boundaries And Validate completed the boundary audit before
final validation.

Accepted compatibility for this runbook:

- Rendered `NttpBindings` compatibility maps remain for older callers,
  no-metadata fallback, ABI/cache/display payloads, and generated
  specialization naming. Metadata-backed NTTP/consteval routes migrated in
  this runbook use TextId/structured mirrors where producer metadata exists.
- Pack synthetic keys such as `Ts#0`, `pack_binding_name`, pack counting, and
  generated pack materialization loops remain compatibility payloads. There is
  no current per-pack-element TextId carrier, and creating one is outside the
  HIR resweep's covered metadata-backed routes.
- Deferred `$expr:` NTTP environment pairs remain rendered/vectorized
  compatibility input for the deferred expression representation. They are not
  treated as structured TextId-backed lookup authority in this runbook.
- Rendered `ConstEvalEnv::struct_defs` storage remains as compatibility
  storage. Metadata-backed record-layout routes added by Step 3 use HIR
  `HirRecordOwnerKey` / `struct_def_owner_index` to select the authoritative
  rendered layout entry when owner metadata is complete.
- Generated, mangled, final-spelling, ABI/link-visible, diagnostic, dump, and
  debug strings remain payloads. Step 5 coverage verifies a structured-selected
  global keeps the selected declaration's `LinkNameId` payload.
- Central legacy rendered fallback for missing or incomplete metadata remains
  explicitly named compatibility. Step 4 converted covered metadata-backed
  local extern/global/prototype/ordinary variable routes to central
  structured-first resolvers.

Durable follow-up status:

- No new `ideas/open/` file was created in this audit.
- Existing `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
  already owns the broader `TypeSpec::tag` removal and type/record metadata
  migration work that outlives idea 140, including rendered type/record storage
  limits and template-record specialization identity fallout exposed when
  `TypeSpec::tag` stops acting as cross-stage semantic identity.
- Pack-element TextId carriers and deferred `$expr:` structured carriers are
  not blockers for closing this runbook because current migrated routes either
  have structured carriers or are explicitly classified no-metadata/generated
  compatibility. They should become separate ideas only if a future plan makes
  pack/deferred expression identity migration an explicit goal.

## Suggested Next

Run supervisor-selected broader validation for Step 6, then decide whether the
active runbook and source idea are ready for close review.

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
- The current audit created no new follow-up ideas. If validation or review
  finds a real untracked metadata blocker, record it under `ideas/open/` before
  closure rather than expanding Step 6 into implementation.

## Proof

Supervisor-run Step 6 full-suite validation passed and wrote `test_after.log`:
`ctest --test-dir build -j --output-on-failure | tee test_after.log`.

Supervisor-run regression guard against `test_baseline.log` passed with
before=2987, after=2987, and no new failures.

# HIR Legacy String Lookup Metadata Resweep

Status: Closed
Created: 2026-05-01
Restored: 2026-05-01
Closed: 2026-05-02

Parent Ideas:
- `ideas/closed/124_hir_legacy_string_lookup_removal_convergence.md`
- `ideas/closed/136_hir_structured_record_template_lookup_authority_cleanup.md`
- `ideas/open/139_parser_sema_rendered_string_lookup_removal.md`

## Goal

Run a focused HIR cleanup pass for rendered-string lookup authority and
metadata propagation after the parser/Sema rendered lookup removal work exposes
cross-module carrier gaps.

HIR should use declaration ids, `TextId`, namespace qualifiers,
`ModuleDeclLookupKey`, `HirRecordOwnerKey`, `LinkNameId`, and structured
record/template/NTTP carriers as lookup authority where available. Rendered
strings should remain diagnostics, dumps, final spelling, mangling payload, or
explicit no-metadata compatibility fallback only.

## Why This Idea Exists

The active parser/Sema runbook must not absorb HIR carrier migration. In
particular, deleting the rendered-name
`eval_const_int(..., const std::unordered_map<std::string, long long>*)`
compatibility overload is blocked while HIR still passes `NttpBindings` as a
string-keyed map. The same boundary now covers Sema
`lookup_record_layout`: `ConstEvalEnv` exposes rendered-keyed HIR
`struct_defs`, while structured record-layout lookup authority lives on
`hir::Module` through `struct_def_owner_index` and
`find_struct_def_by_owner_structured(...)`.

This idea preserves that blocker as explicit open lifecycle state so idea 139
can keep parser-owned const-int cleanup on `TextId` or structured maps without
silently expanding into `src/frontend/hir`.

## Scope

- Re-inventory `src/frontend/hir` for `std::string` keyed lookup maps,
  rendered-name fallback paths, and parity-only structured mirrors.
- Include HIR `fallback`, `legacy`, and `compatibility` named semantic routes
  surfaced during idea 139 cleanup, including
  `find_function_by_name_legacy`, `find_global_by_name_legacy`,
  `has_legacy_mangled_entry`, `ModuleDeclLookupAuthority::LegacyRendered`,
  `declaration_fallback`, rendered-name compatibility indexes, and
  `fallback_tag` / `fallback_member` style recovery.
- Trace parser/Sema metadata through HIR lowering for records, methods,
  templates, static members, consteval, globals, function declarations, and
  NTTP bindings.
- Migrate HIR `NttpBindings` and any equivalent consteval metadata handoff to
  structured or `TextId` keyed carriers where parser/Sema already provides the
  source identity.
- Add or migrate a structured HIR record-layout carrier for consteval layout
  lookup so Sema does not have to resolve `TypeSpec::tag` through rendered
  `ConstEvalEnv::struct_defs` keys when record owner metadata is available.
- Convert or demote HIR lookup paths where structured keys already exist.
- Preserve rendered names for diagnostics, dumps, mangling, link-visible final
  spelling, and explicit no-metadata compatibility input.
- Add tests where HIR receives both structured metadata and drifted rendered
  spelling and must prefer the structured carrier.
- Split missing upstream parser/Sema metadata or downstream LIR/BIR carrier
  needs into separate `ideas/open/` files.

## Deferred From Idea 139

The active parser/Sema plan may delete parser/Sema fallback routes only when
the required metadata is owned by parser/Sema. The following blockers should
move here instead of widening idea 139:

- HIR rendered declaration lookup and lookup authority parity, including
  `find_function_by_name_legacy`, `find_global_by_name_legacy`, and
  `ModuleDeclLookupAuthority::LegacyRendered`.
- HIR mangled/rendered materialization compatibility, including
  `has_legacy_mangled_entry` and rendered-name compatibility indexes that
  still decide semantic identity.
- HIR fallback recovery surfaces such as `declaration_fallback`,
  `fallback_tag`, and `fallback_member` when they recover owner/member/type
  identity from rendered spelling.
- HIR consteval and NTTP handoff through string-keyed `NttpBindings`, which
  blocks deleting the parser/Sema rendered-name
  `eval_const_int(..., const std::unordered_map<std::string, long long>*)`
  compatibility overload.
- HIR record-layout handoff into Sema consteval: `lookup_record_layout`
  currently receives rendered-keyed `ConstEvalEnv::struct_defs` but no
  structured HIR record owner/index or equivalent layout map, which blocks
  deleting `env.struct_defs->find(ts.tag)`.
- Any parser/Sema route whose only remaining blocker is a HIR producer or
  consumer contract rather than missing parser/Sema metadata.

## Out Of Scope

- Parser/Sema implementation cleanup except as a blocker recorded in a new
  idea.
- LIR/BIR/backend implementation cleanup except as a downstream metadata bridge
  follow-up.
- Removing final link names, dumps, diagnostics, or ABI-visible text.
- Named-case shortcuts or unsupported expectation downgrades.

## Acceptance Criteria

- Covered HIR semantic lookup paths are structured-primary.
- HIR no longer relies on rendered strings to recover metadata that is already
  present in parser/Sema or HIR carriers.
- HIR `NttpBindings` no longer requires the parser/Sema
  `eval_const_int(..., const std::unordered_map<std::string, long long>*)`
  compatibility overload for metadata-backed routes, or any remaining
  no-metadata compatibility path is explicitly classified.
- Sema consteval record-layout lookup can use a structured HIR record-layout
  carrier when record owner metadata is available, or any retained
  rendered-keyed `ConstEvalEnv::struct_defs` path is explicitly classified as
  no-metadata compatibility.
- Remaining HIR string surfaces are classified as display, diagnostics, dump,
  final spelling, compatibility, local scratch, or unresolved metadata
  boundary.
- Cross-module metadata blockers are represented as open ideas.

## Completion Notes

Closed after the active runbook completed all six steps:

- HIR rendered lookup surfaces were classified before behavior changes.
- Metadata-backed NTTP/consteval handoff routes gained TextId/structured
  carriers while rendered `NttpBindings` remains explicit compatibility.
- Metadata-backed record-layout lookup can use HIR owner/index authority where
  owner metadata is available; rendered `ConstEvalEnv::struct_defs` storage is
  classified compatibility.
- Covered local extern/global/prototype/ordinary variable lookup routes moved
  to central structured-first resolvers, with legacy rendered fallback retained
  for missing or incomplete metadata.
- Display, diagnostics, dumps, final spelling, link-visible text, mangling, and
  generated-name strings were preserved as payloads rather than lookup
  authority.
- Step 6 boundary audit found no new durable follow-up idea needed for this
  runbook. Existing `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`
  owns the broader `TypeSpec::tag` removal and type/record metadata migration
  work that outlives this idea.

Close validation:

- Full suite passed in `test_after.log`: 2987/2987 tests.
- Regression guard against `test_baseline.log` passed with before=2987,
  after=2987, and no new failures.

## Reviewer Reject Signals

- The route claims HIR metadata cleanup while leaving `NttpBindings` semantic
  lookup dependent on `std::unordered_map<std::string, long long>` for
  metadata-backed NTTP routes.
- The change deletes or narrows the parser/Sema `eval_const_int` compatibility
  overload without first migrating the HIR caller contract or proving an
  equivalent structured carrier.
- The route claims `lookup_record_layout` cleanup while leaving metadata-backed
  record layout lookup dependent on rendered `TypeSpec::tag` and rendered
  `ConstEvalEnv::struct_defs` keys.
- A rendered-name fallback is renamed, wrapped, or reclassified as structured
  progress while still deciding HIR semantic identity after structured metadata
  exists.
- A `legacy`, `fallback`, or `compatibility` named HIR semantic route surfaced
  from idea 139 remains in the normal metadata-backed path without being
  deleted, structurally replaced, or explicitly classified as no-metadata
  compatibility.
- Tests are weakened, marked unsupported, or limited to one named fixture
  instead of covering same-feature structured-vs-rendered disagreement.
- The implementation expands into LIR, BIR, or backend cleanup instead of
  recording those downstream carrier gaps as separate open ideas.

# HIR Legacy String Lookup Metadata Resweep

Status: Open
Created: 2026-05-01
Restored: 2026-05-01

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
string-keyed map.

This idea preserves that blocker as explicit open lifecycle state so idea 139
can keep parser-owned const-int cleanup on `TextId` or structured maps without
silently expanding into `src/frontend/hir`.

## Scope

- Re-inventory `src/frontend/hir` for `std::string` keyed lookup maps,
  rendered-name fallback paths, and parity-only structured mirrors.
- Trace parser/Sema metadata through HIR lowering for records, methods,
  templates, static members, consteval, globals, function declarations, and
  NTTP bindings.
- Migrate HIR `NttpBindings` and any equivalent consteval metadata handoff to
  structured or `TextId` keyed carriers where parser/Sema already provides the
  source identity.
- Convert or demote HIR lookup paths where structured keys already exist.
- Preserve rendered names for diagnostics, dumps, mangling, link-visible final
  spelling, and explicit no-metadata compatibility input.
- Add tests where HIR receives both structured metadata and drifted rendered
  spelling and must prefer the structured carrier.
- Split missing upstream parser/Sema metadata or downstream LIR/BIR carrier
  needs into separate `ideas/open/` files.

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
- Remaining HIR string surfaces are classified as display, diagnostics, dump,
  final spelling, compatibility, local scratch, or unresolved metadata
  boundary.
- Cross-module metadata blockers are represented as open ideas.

## Reviewer Reject Signals

- The route claims HIR metadata cleanup while leaving `NttpBindings` semantic
  lookup dependent on `std::unordered_map<std::string, long long>` for
  metadata-backed NTTP routes.
- The change deletes or narrows the parser/Sema `eval_const_int` compatibility
  overload without first migrating the HIR caller contract or proving an
  equivalent structured carrier.
- A rendered-name fallback is renamed, wrapped, or reclassified as structured
  progress while still deciding HIR semantic identity after structured metadata
  exists.
- Tests are weakened, marked unsupported, or limited to one named fixture
  instead of covering same-feature structured-vs-rendered disagreement.
- The implementation expands into LIR, BIR, or backend cleanup instead of
  recording those downstream carrier gaps as separate open ideas.

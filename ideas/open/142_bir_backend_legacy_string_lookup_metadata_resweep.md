# BIR Backend Legacy String Lookup Metadata Resweep

Status: Open
Created: 2026-04-30

Parent Ideas:
- [126_bir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/126_bir_legacy_string_lookup_removal_convergence.md)
- [138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md](/workspaces/c4c/ideas/closed/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md)
- [141_lir_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/141_lir_legacy_string_lookup_metadata_resweep.md)

## Goal

Run a second BIR/backend cleanup and readiness pass for legacy rendered-string
lookup authority after the first BIR convergence and aggregate layout bridge
cleanup.

BIR and backend consumers should use `TextId`, `LinkNameId`, block label IDs,
structured layout entries, typed records, prepared metadata, or route-local
semantic keys where available instead of raw rendered strings.

This pass should also decide how ready the `semantic BIR -> prepared backend
facts -> MIR/aarch64 consumer` route is for a follow-on backend cleanup or
rebuild. The goal is not to rewrite AArch64 in this idea; the goal is to
separate what can be cleaned inside 142 from the larger BIR/prealloc/MIR
contracts that need their own ideas.

## Why This Idea Exists

The remaining backend half of the compiler still contains many legitimate
strings: final assembly text, local labels, diagnostics, dumps, selector input,
and route-local names. The resweep should separate those from actual semantic
lookup authority, especially at the LIR-to-BIR and BIR-to-MIR/backend metadata
boundaries.

This idea is the BIR/backend leg of the second 123-to-126 style cleanup
sequence.

Earlier BIR ideas were closed when their own scopes converged, not because the
entire BIR/backend boundary was proven complete. In particular:

- 126 closed the covered BIR string lookup route while explicitly leaving
  unresolved producer-boundary work to follow-up ideas.
- 138 closed aggregate layout text-bridge cleanup with explicit structured
  selection/fallback reporting, but did not claim that every BIR/prealloc/MIR
  consumer contract was ready for target backend rebuild work.

This idea exists to revisit those closure boundaries with the specific question:
what remains before backend work can safely proceed through prepared facts into
`mir/aarch64` rather than target-local rediscovery?

## Read First

- [126_bir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/126_bir_legacy_string_lookup_removal_convergence.md)
- [138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md](/workspaces/c4c/ideas/closed/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md)
- Any follow-up ideas referenced by those closure notes, especially unresolved
  producer-boundary or aggregate-layout metadata blockers.
- `src/backend/README.md`
- `src/backend/bir/`
- `src/backend/prealloc/`
- `src/backend/mir/aarch64/`

## Working Model

Backend cleanup should flow through:

```text
semantic BIR
  -> prepared backend facts
  -> MIR/aarch64 target consumer
```

`mir/aarch64` should consume prepared facts instead of reopening canonical BIR
or reconstructing stack layout, ABI homes, liveness, local slots, addressing
provenance, aggregate layout, or symbol identity from raw strings.

## Scope

- Re-inventory `src/backend/bir`, `src/backend/prealloc`, and target backend
  lookup paths for `std::string` maps, raw-name matching, and fallback routes.
- Re-read closure notes from prior BIR/backend ideas and classify deferred
  observations as either still resolved, in-scope for 142, or requiring a new
  follow-up idea.
- Trace LIR/BIR metadata into backend lowering for calls, globals, aggregate
  layouts, block labels, local slots, pointer provenance, prepared handoff, and
  target routing.
- Audit the `semantic BIR -> prepared backend facts -> mir/aarch64` boundary
  for facts that AArch64 currently rediscovers locally instead of receiving
  from BIR/prealloc.
- Prefer structured IDs, typed BIR records, prepared metadata, and route-local
  semantic keys over raw rendered strings where the data exists.
- Keep raw strings for final assembly text, diagnostics, dumps, selectors,
  compatibility input, and route-local temporary handles when classified.
- Add focused BIR/backend tests where drifted raw spelling must not override
  structured metadata.
- Split missing LIR producer metadata, BIR/prealloc fact gaps, or target
  consumer contract gaps into separate open ideas instead of silently expanding
  142.
- Produce a backend-readiness inventory that says which future backend work can
  start from existing BIR/prealloc contracts and which work needs additional
  contract ideas first.

## Out Of Scope

- Parser, Sema, HIR, or LIR cleanup except as separately opened metadata
  blockers.
- Removing final emitted assembly, dump text, diagnostics, or selector syntax.
- Rewriting target backend architecture beyond lookup-authority cleanup and
  readiness classification.
- Rebuilding `bir -> mir/aarch64` or `prealloc -> mir/aarch64` directly inside
  this idea, except for tiny proof patches needed to validate a metadata
  cleanup.
- Testcase overfit or expectation downgrades.

## Acceptance Criteria

- Covered BIR/backend semantic lookup paths are structured-primary.
- Remaining backend string maps and lookups are classified by role and cannot
  silently override structured metadata where present.
- Focused backend tests cover same-feature structured-vs-rendered disagreement
  cases.
- Prior BIR/backend closure notes have been reviewed, and deferred observations
  are either closed as already resolved, handled inside 142, or recorded as new
  `ideas/open/` follow-ups.
- The BIR/prealloc/MIR boundary has a written readiness inventory covering
  calls, globals, aggregate layout, block labels, local slots, pointer
  provenance, stack layout, ABI homes, liveness/regalloc facts, inline asm, and
  target routing.
- Any cross-module metadata blockers, missing prepared facts, or AArch64
  consumer contract gaps are recorded under `ideas/open/`.
- The result clearly answers whether follow-on backend work can proceed through
  `semantic BIR -> prepared backend facts -> mir/aarch64`, and names the next
  backend idea(s) if not.

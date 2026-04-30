# LIR Legacy String Lookup Metadata Resweep

Status: Open
Created: 2026-04-30

Parent Ideas:
- [125_lir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/125_lir_legacy_string_lookup_removal_convergence.md)
- [138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md](/workspaces/c4c/ideas/closed/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md)
- [140_hir_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/140_hir_legacy_string_lookup_metadata_resweep.md)

## Goal

Run a second LIR cleanup pass for rendered-string lookup authority and metadata
handoff into BIR/backend. LIR should carry `TextId`, `LinkNameId`,
`StructNameId`, structured type refs, function metadata, and aggregate layout
identity across module boundaries instead of forcing downstream consumers to
rediscover identity from rendered spellings.

## Why This Idea Exists

The first LIR convergence closed with some producer-boundary fallbacks
classified. The later aggregate layout cleanup removed one large
type-declaration text bridge, but a full resweep should verify direct calls,
externs, globals, type refs, signature references, and layout handoff metadata
still survive into BIR/backend.

This idea is the LIR leg of the second 123-to-126 style cleanup sequence.

## Scope

- Re-inventory `src/codegen/lir` for `std::string` keyed lookup maps,
  rendered-name matching, signature-text scans, and compatibility fallback
  routes.
- Trace HIR-to-LIR metadata for functions, extern declarations, globals,
  structured type refs, aggregate layouts, pointer/global references, and
  signature references.
- Prefer existing semantic IDs and structured records over rendered strings
  where producer data exists.
- Keep rendered strings for final LLVM spelling, diagnostics, dumps, ABI/link
  spelling, and explicit raw/hand-built LIR compatibility.
- Add focused LIR and downstream tests where drifted rendered spelling must not
  override structured metadata.
- Split missing HIR producer data or BIR/backend consumer metadata gaps into
  new open ideas.

## Out Of Scope

- Parser/Sema/HIR implementation cleanup except as separately opened blockers.
- BIR/backend consumer rewrites unless they are necessary narrow proof for LIR
  metadata handoff.
- Removing final emitted LLVM text or ABI-visible names.
- Broad type-ref redesign unrelated to lookup authority.

## Acceptance Criteria

- Covered LIR lookup and handoff paths are structured-primary.
- LIR exposes enough metadata for covered BIR/backend consumers to avoid
  rendered-string rediscovery.
- Remaining LIR string surfaces are classified as final spelling, display,
  diagnostics, dump, compatibility, or unresolved producer/consumer boundary.
- Any unresolved cross-module blocker is recorded under `ideas/open/`.


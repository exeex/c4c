# BIR Backend Legacy String Lookup Metadata Resweep

Status: Open
Created: 2026-04-30

Parent Ideas:
- [126_bir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/126_bir_legacy_string_lookup_removal_convergence.md)
- [138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md](/workspaces/c4c/ideas/closed/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md)
- [141_lir_legacy_string_lookup_metadata_resweep.md](/workspaces/c4c/ideas/open/141_lir_legacy_string_lookup_metadata_resweep.md)

## Goal

Run a second BIR/backend cleanup pass for legacy rendered-string lookup
authority after the first BIR convergence and aggregate layout bridge cleanup.
BIR and backend consumers should use `TextId`, `LinkNameId`, block label IDs,
structured layout entries, typed records, prepared metadata, or route-local
semantic keys where available instead of raw rendered strings.

## Why This Idea Exists

The remaining backend half of the compiler still contains many legitimate
strings: final assembly text, local labels, diagnostics, dumps, selector input,
and route-local names. The resweep should separate those from actual semantic
lookup authority, especially at the LIR-to-BIR and BIR-to-MIR/backend metadata
boundaries.

This idea is the BIR/backend leg of the second 123-to-126 style cleanup
sequence.

## Scope

- Re-inventory `src/backend/bir`, `src/backend/prealloc`, and target backend
  lookup paths for `std::string` maps, raw-name matching, and fallback routes.
- Trace LIR/BIR metadata into backend lowering for calls, globals, aggregate
  layouts, block labels, local slots, pointer provenance, prepared handoff, and
  target routing.
- Prefer structured IDs, typed BIR records, prepared metadata, and route-local
  semantic keys over raw rendered strings where the data exists.
- Keep raw strings for final assembly text, diagnostics, dumps, selectors,
  compatibility input, and route-local temporary handles when classified.
- Add focused BIR/backend tests where drifted raw spelling must not override
  structured metadata.
- Split missing LIR producer metadata or backend consumer contract gaps into
  separate open ideas.

## Out Of Scope

- Parser, Sema, HIR, or LIR cleanup except as separately opened metadata
  blockers.
- Removing final emitted assembly, dump text, diagnostics, or selector syntax.
- Rewriting target backend architecture beyond lookup-authority cleanup.
- Testcase overfit or expectation downgrades.

## Acceptance Criteria

- Covered BIR/backend semantic lookup paths are structured-primary.
- Remaining backend string maps and lookups are classified by role and cannot
  silently override structured metadata where present.
- Focused backend tests cover same-feature structured-vs-rendered disagreement
  cases.
- Cross-module metadata blockers are recorded under `ideas/open/`.


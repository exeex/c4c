# Cross IR String Authority Audit And Followup Queue

Status: Open
Created: 2026-04-29

Parent Ideas:
- [123_parser_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/123_parser_legacy_string_lookup_removal_convergence.md)
- [124_hir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/124_hir_legacy_string_lookup_removal_convergence.md)
- [125_lir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/125_lir_legacy_string_lookup_removal_convergence.md)
- [126_bir_legacy_string_lookup_removal_convergence.md](/workspaces/c4c/ideas/closed/126_bir_legacy_string_lookup_removal_convergence.md)

## Goal

Create a cross-IR inventory of `std::string` / rendered-spelling uses that still
act as semantic identity, and split concrete cleanup work into separate ideas.

## Why This Idea Exists

The intended cross-IR identity path should be carried by structured tables and
ids such as `TextTable`, `LinkNameTable`, `StructNameTable`,
`MemberSymbolTable`, `BlockLabelTable`, and related structured lookup keys.
`std::string` remains appropriate for generated names, mangling, anonymous labels
or section labels, diagnostics, dumps, and final emitted text. It is suspicious
when used as the only authority for a cross-IR lookup or producer/consumer
handoff.

Ideas 123-126 removed many known legacy lookup paths. This audit owns the
remaining classification pass and creates narrower follow-up ideas for any
unreasonable string authority that remains.

## Scope

- Search Parser, Sema, HIR, LIR, BIR, and backend handoff surfaces for
  `std::string`, raw `const char*`, rendered text fields, and string-keyed maps
  used near IR boundaries.
- Classify each relevant use as:
  - structured identity carrier already exists
  - legitimate generated/display/final-spelling text
  - compatibility fallback with structured primary path
  - suspicious string authority requiring cleanup
- Treat mangling, anonymous-name generation, section labels, diagnostics, and
  dump/final text as legitimate only when they do not become cross-IR semantic
  authority.
- Write one or more follow-up ideas for suspicious cases instead of expanding
  this audit into implementation work.

## Out Of Scope

- Bulk replacing all `std::string` use.
- Removing final emitted text fields.
- Changing mangling, anonymous label generation, diagnostics, or dump behavior.
- Fixing every suspicious path inside this audit.

## Acceptance Criteria

- The repo has an explicit inventory or review note for cross-IR string
  authority risk.
- Every suspicious cross-stage string authority found by the audit has a
  concrete follow-up idea under `ideas/open/`.
- Legitimate `std::string` use is documented as generated/display/final text,
  not cross-IR identity.
- No code behavior changes are made by the audit itself unless a separate
  implementation idea is activated.

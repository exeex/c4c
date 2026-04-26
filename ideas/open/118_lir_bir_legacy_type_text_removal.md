# LIR BIR Legacy Type Text Removal

Status: Open
Created: 2026-04-26
Last Updated: 2026-04-26

Parent Ideas:
- [116_bir_layout_dual_path_coverage_and_dump_guard.md](/workspaces/c4c/ideas/open/116_bir_layout_dual_path_coverage_and_dump_guard.md)
- [117_bir_printer_structured_render_context.md](/workspaces/c4c/ideas/open/117_bir_printer_structured_render_context.md)

## Goal

Remove legacy LIR/BIR type-text authority only after the BIR layout dual path
and BIR printer structured render context are proven. Legacy text may remain
as final spelling where explicitly required, but it must no longer be active
layout, lowering, or dump-render authority for the covered paths.

## Why This Idea Exists

Ideas 112 through 115 split backend layout blockers from HIR-to-LIR layout
authority and introduced structured mirrors. The next risk is over-removing:
`LirModule::type_decls`, raw `LirTypeRef` text, function signature strings,
extern return strings, global LLVM type strings, and BIR dump text all still
serve different roles.

This idea is deliberately last in the sequence. It should only start after:

- idea 116 proves structured BIR layout coverage and dump guards under the dual
  path
- idea 117 gives BIR printing a structured render route

## Scope

Candidate legacy surfaces include, only after direct proof:

- `LirModule::type_decls` as active BIR layout authority
- `TypeDeclMap` construction from legacy declaration text where structured
  layout is complete
- fallback-only aggregate layout parsing paths that are no longer needed
- BIR printer fallback string fields that idea 117 marked as removable
- proof-only parity shadows whose covered paths no longer need legacy text

Out of scope unless a runbook explicitly proves readiness:

- broad `LirTypeRef` equality and identity redesign
- final emitted LLVM spelling that still needs text for output compatibility
- `src/backend/mir/` migration or preservation
- testcase expectation downgrades or narrow named-case shortcuts

## Execution Rules

- Do not begin this idea while idea 116 or idea 117 remains incomplete.
- Remove legacy authority incrementally by semantic family, not by deleting all
  string fields at once.
- Every removed fallback must have a structured-only proof.
- Preserve `--dump-bir`, prepared-BIR, and backend output unless an intentional
  output contract change is explicitly accepted.
- Treat missing structured coverage as a blocker, not as permission to rewrite
  expectations.
- Keep final spelling strings only where they are printer/output data, not
  semantic lookup or layout authority.

## Acceptance Criteria

- Covered BIR layout paths no longer require `LirModule::type_decls` or
  legacy declaration body parsing as active authority.
- Covered BIR printer paths render through structured context without legacy
  fallback.
- Legacy fallback removal is backed by structured-only tests and existing
  full-suite or targeted regression proof.
- Remaining legacy text fields are classified as final spelling,
  type-ref-authority work, planned-rebuild residue, or explicitly deferred
  compatibility surfaces.
- No testcase-overfit expectation rewrite is used to claim progress.

## Deferred

- Any discovered independent structured identity or type-ref redesign should
  become a separate idea under `ideas/open/` rather than expanding this removal
  route silently.

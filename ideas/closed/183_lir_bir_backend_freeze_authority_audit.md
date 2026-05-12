# LIR/BIR Backend Freeze Authority Audit

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

Depends On:
- `ideas/closed/182_type_identity_migration_closure_gate.md`

## Goal

Audit the LIR/BIR boundary before restarting backend work and produce a freeze
ledger for the remaining identity-authority surfaces.

The audit should classify signature, symbol, global, type/layout, memory
provenance, and route-local name handling so the next backend implementation
work does not reopen semantic string authority by accident.

## Why This Idea Exists

The parser, sema, HIR, LinkNameId, and type-identity waves have converged far
enough to form a stable frontend/midend boundary. The remaining risk is that
LIR/BIR/backend lowering still contains compatibility strings that are valid
for raw or no-metadata imports, but dangerous if treated as ordinary semantic
authority for generated metadata-rich paths.

Before backend restart, these LIR/BIR surfaces need to be classified and split
into concrete follow-up work.

## In Scope

- Inventory `src/codegen/lir`, `src/backend/bir`, and `src/backend/prealloc`
  identity surfaces relevant to backend restart.
- Classify retained strings as display/output, diagnostics, route-local
  handles, no-metadata compatibility, or semantic authority that still needs
  replacement.
- Pay special attention to direct-call signatures, direct callee identity,
  global/type declaration compatibility tables, aggregate layout facts, memory
  provenance global handles, and prealloc route-local names.
- Identify which surfaces should be fixed before backend restart and which can
  remain fenced compatibility.
- Create or confirm follow-up ideas for each required freeze blocker.

## Out Of Scope

- Implementing the follow-up fixes in the audit slice.
- Restarting x86 or target backend implementation.
- Removing every string from LIR/BIR printers, diagnostics, or route-local SSA
  bookkeeping.
- Treating hand-authored raw LIR/BIR fixtures as metadata-rich generated input.

## Acceptance Criteria

- A freeze ledger documents each inspected LIR/BIR identity surface and its
  authority classification.
- Any unclassified semantic string authority is converted into a concrete
  follow-up idea.
- Direct-call signature metadata, global/type declaration compatibility,
  direct symbol identity, memory provenance global handles, and prealloc
  route-local names are explicitly addressed.
- The audit does not create an active `plan.md`/`todo.md` unless this idea is
  activated through the normal lifecycle.

## Closure Notes

- The audit ledger classified direct-call signatures, global/type declaration
  compatibility tables, direct symbol identity, memory provenance global
  handles, prealloc route-local names, and the final freeze gate.
- Required freeze blockers are covered by open follow-up ideas 184-188.
- This closure is audit completion only; backend freeze remains open until
  `ideas/open/188_lir_bir_freeze_closure_gate.md` closes after ideas 184-187.

## Reviewer Reject Signals

- The audit treats all strings as defects without separating display and
  route-local handles.
- The audit accepts generated metadata-rich paths falling back to rendered
  signature or symbol text without calling it out.
- The audit starts backend rewrite work instead of producing a freeze map.
- Follow-up work is described only in chat and not captured as durable ideas.

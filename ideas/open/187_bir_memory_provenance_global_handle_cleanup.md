# BIR Memory Provenance Global Handle Cleanup

Status: Open
Created: 2026-05-12

Depends On:
- `ideas/closed/185_lir_to_bir_global_typedecl_compatibility_fence.md`
- `ideas/closed/186_bir_direct_symbol_identity_validation_closure.md`

## Goal

Clean up the BIR memory/provenance global-handle tables so global identity is
`LinkNameId`-backed where possible and raw global spelling remains a
route-local or compatibility bridge.

## Why This Idea Exists

The LIR-to-BIR memory/provenance layer still has many string-keyed maps for
global pointers, global addresses, local slots, pointer aliases, dynamic arrays,
and aggregate/scalar access tracking. Many of these names are legitimate
route-local handles, but global provenance can accidentally become a second
semantic symbol table if final spelling is used as identity after LinkNameId
metadata exists.

Before backend restart, this boundary should be reduced or explicitly fenced.

## In Scope

- Audit memory/provenance maps under `src/backend/bir/lir_to_bir/memory`.
- Identify which maps are route-local SSA/slot handles and which represent
  global symbol identity.
- Convert one or more selected global-provenance maps or keys to carry
  `LinkNameId` authority, or add fail-closed checks when id metadata is
  missing/stale.
- Keep local slot, local SSA, and raw-import compatibility strings where they
  are truly route-local.
- Add focused tests for global pointer/address provenance with matching and
  mismatched symbol metadata.

## Out Of Scope

- Rewriting all memory lowering or pointer analysis.
- Removing route-local local variable, slot, or temporary names.
- Changing final BIR dump spelling.
- Broad backend alias analysis redesign.

## Acceptance Criteria

- The selected memory/provenance global path no longer uses global spelling as
  ordinary semantic identity when `LinkNameId` is available.
- Route-local strings are explicitly distinguished from global symbol identity.
- Tests cover structured success and stale/missing global-id rejection or
  documented compatibility behavior.
- Validation includes targeted BIR memory/provenance coverage.

## Reviewer Reject Signals

- The fix treats every string-keyed local map as semantic symbol authority.
- Global metadata mismatches are accepted because final spellings match.
- Tests only check simple local slots and do not exercise global provenance.
- The slice grows into unrelated memory lowering rewrites.

# BIR Memory Provenance Global Handle Cleanup

Status: Closed
Created: 2026-05-12
Closed: 2026-05-12

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

## Closure Notes

- Step 1 inventory classified memory/provenance handles and selected the
  addressed-global pointer provenance path for hardening.
- Step 2 keyed `AddressedGlobalPointerSlots` and
  `AddressedGlobalPointerValueSlots` by `LinkNameId`, preserving route-local
  and compatibility string paths.
- Step 3 added focused backend coverage for structured success and stale,
  missing, or mismatched global-id rejection on the selected path.
- Step 4 validated the backend handoff with
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Idea 188 still owns the final LIR/BIR freeze gate; this closure does not
  accept or replace that broader milestone.
- The rejected `test_baseline.new.log` full-suite candidate had unrelated
  failures and was not accepted as a baseline refresh for this idea.

## Reviewer Reject Signals

- The fix treats every string-keyed local map as semantic symbol authority.
- Global metadata mismatches are accepted because final spellings match.
- Tests only check simple local slots and do not exercise global provenance.
- The slice grows into unrelated memory lowering rewrites.

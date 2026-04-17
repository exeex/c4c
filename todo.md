Status: Active
Source Idea: ideas/open/13_x86_backend_pure_bir_handoff_and_legacy_guardrails.md
Source Plan: plan.md

# Current Packet

## Just Finished

Activated the x86 pure-BIR handoff idea into `plan.md` and reset canonical
execution state for the new active runbook.

## Suggested Next

Start Step 1 by identifying the actual x86 backend entry boundary in
`src/backend/`, the exact mixed LIR/BIR seams that still own route selection,
and the narrowest honest proof test for the active x86 handoff.

## Watchouts

- do not preserve the mixed route as a fallback just because it currently
  works; the idea is about deleting or hard-retiring the wrong boundary
- keep x86 handoff work focused on contract ownership, not broad emission
  cleanup outside the source idea
- prefer backend-published ABI/location facts over new x86-local shadow
  contracts

## Proof

Lifecycle activation only. No implementation proof run yet.

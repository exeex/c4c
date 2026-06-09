Status: Active
Source Idea Path: ideas/open/138_call_plan_lookup_ownership_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current call lookup ownership and dependencies

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and `todo.md` from
`ideas/open/138_call_plan_lookup_ownership_cleanup.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inspect current call lookup ownership and
dependencies, then record the intended call-owned declaration boundary and any
dependency-cycle constraints before making implementation edits.

## Watchouts

- Keep the first implementation packet inspection-focused until the ownership
  move is clear.
- Do not change ABI classification, call lowering semantics, preservation
  behavior, or AArch64 register handling.
- Do not replace prepared call lookups with local scans in AArch64 consumers.

## Proof

Lifecycle-only activation. No build or test proof required yet.

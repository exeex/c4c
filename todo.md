Status: Active
Source Idea Path: ideas/open/140_edge_copy_facade_split.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect current edge-copy facade ownership and dependencies

# Current Packet

## Just Finished

Lifecycle activation created the active runbook from
`ideas/open/140_edge_copy_facade_split.md`.

## Suggested Next

Execute Step 1 from `plan.md`: inspect current edge-copy facade ownership and
dependencies, then record declaration locations, definition locations,
construction wiring, consumer include constraints, and the exact Step 2
split/rename boundary in this file.

## Watchouts

- Do not edit implementation files during Step 1.
- Do not change control-flow semantics, out-of-SSA parallel-copy generation,
  dispatch lowering, memory lowering, or predecessor/successor transfer
  identity.
- Do not replace prepared edge facts with predecessor rescans, BIR rediscovery,
  local value-location scans, or named-case shortcuts.
- Keep reusable edge publication and move-bundle facts available to non-AArch64
  backends.

## Proof

Step 1 is inspection-only; no build proof is required until the first
code-changing packet.

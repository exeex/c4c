Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Aggregate Transport Decisions

# Current Packet

## Just Finished

Lifecycle activation created this execution scratchpad for Step 1 of
`plan.md`.

## Suggested Next

Inventory aggregate transport decision points in
`src/backend/mir/aarch64/codegen/instruction.cpp`,
`src/backend/mir/aarch64/codegen/memory.cpp`, and relevant shared
aggregate/value-home/addressing surfaces. Record stack-backed,
register-backed, and mixed or lane-sensitive paths when present before any code
changes.

## Watchouts

- Do not implement AArch64 aggregate copy cleanup before naming the shared
  authority contract.
- Do not treat ordinary load/store opcode choice or address spelling as shared
  aggregate transport authority.
- Do not weaken aggregate testcase expectations or add named-case shortcuts.
- Keep general memory address cleanup out of this route.

## Proof

Lifecycle-only activation; no build or test run required.

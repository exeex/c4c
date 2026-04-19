# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce Frame And Addressing Facts In Shared Prepare
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed the next Step 2 packet by teaching shared prepare to publish
per-instruction pointer-indirect addressing during stack layout:
`src/backend/prealloc/stack_layout.cpp` now records `PreparedMemoryAccess`
entries for `LoadLocalInst`, `StoreLocalInst`, `LoadGlobalInst`, and
`StoreGlobalInst` lanes that carry pointer-value addresses, and
`tests/backend/backend_prepare_stack_layout_test.cpp` proves those records
alongside the earlier direct frame-slot and symbol-backed producer coverage.

## Suggested Next

Begin Step 3.1 by teaching the prepared x86 frame-layout consumer to use the
published prepared frame size and alignment facts for prologue/epilogue
emission instead of recomputing frame layout locally.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Shared prepare now publishes frame metrics plus direct frame-slot,
  symbol-backed, and pointer-indirect access records for the covered load/store
  lanes.
- X86 still consumes private frame/address reconstruction until later Step 3
  packets land.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' | tee test_after.log`
for this Step 2 pointer-indirect packet; the focused stack-layout subset
passed and `test_after.log` is the canonical proof log for the packet.

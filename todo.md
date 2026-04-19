# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce Frame And Addressing Facts In Shared Prepare
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed the next Step 2 packet by teaching shared prepare to publish
per-instruction direct symbol-backed addressing during stack layout:
`src/backend/prealloc/stack_layout.cpp` now records `PreparedMemoryAccess`
entries for direct `LoadGlobalInst` and `StoreGlobalInst` lanes that resolve
to canonical global-symbol or string-constant bases, and
`tests/backend/backend_prepare_stack_layout_test.cpp` proves those records
alongside the surviving direct frame-slot store/load facts.

## Suggested Next

Execute the next Step 2 packet by classifying pointer-indirect load/store
addressing into prepared memory-access records in shared prepare, so the later
x86 consumer can stop recovering pointer-based address provenance locally.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Shared prepare now publishes frame metrics plus direct frame-slot and direct
  symbol-backed access records, but pointer-indirect classification remains
  open Step 2 work.
- X86 still consumes private frame/address reconstruction until later Step 3
  packets land.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' | tee test_after.log`
for this Step 2 direct symbol-backed packet; the focused stack-layout subset
passed and `test_after.log` is the canonical proof log for the packet.

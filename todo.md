# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce Frame And Addressing Facts In Shared Prepare
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the first Step 2 bootstrap packet by teaching shared prepare to
publish per-function prepared-addressing frame facts during stack layout:
`src/backend/prealloc/stack_layout.cpp` now records each function's canonical
frame size/alignment in `PreparedBirModule::addressing`, and
`tests/backend/backend_prepare_stack_layout_test.cpp` proves that the producer
path exposes those frame metrics while leaving per-instruction accesses empty
for later Step 2 packets.

## Suggested Next

Execute the next Step 2 packet by populating per-instruction prepared
addressing records for direct frame-slot loads/stores in shared prepare,
reusing the new per-function addressing container instead of adding x86-local
slot or offset reconstruction.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Shared prepare now publishes frame metrics, but `PreparedAddressingFunction`
  still has no per-instruction access records; direct frame-slot, symbol, and
  pointer-indirect classification remain open Step 2 work.
- X86 still consumes private frame/address reconstruction until later Step 3
  packets land.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' | tee test_after.log`
for this Step 2 bootstrap packet; the focused stack-layout subset passed and
`test_after.log` is the canonical proof log for the packet.

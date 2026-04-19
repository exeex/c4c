# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Lock The Prepared Addressing Contract
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed the first Step 1 packet by defining the initial prepared
frame/addressing contract in `src/backend/prealloc/prealloc.hpp`: added
consumer-oriented prepared addressing records, base-kind naming, function and
memory-access lookup helpers, and the `PreparedBirModule::addressing` surface;
`tests/backend/backend_prepare_stack_layout_test.cpp` now exercises that
header-only contract with a focused lookup activation.

## Suggested Next

Execute the next Step 1 packet or Step 2 handoff by beginning the shared
producer path in `src/backend/prealloc/legalize.cpp`: populate function-level
frame size/alignment and per-instruction prepared addressing records for the
existing stack-slot and direct-memory routes now that the contract surface
exists.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- The current packet only locked the contract surface; shared prepare still
  needs to populate `PreparedBirModule::addressing`, and x86 still consumes
  private frame/address reconstruction until later steps land.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_prepare_stack_layout$' | tee test_after.log`
for this Step 1 packet; the focused stack-layout subset passed and
`test_after.log` is the canonical proof log for the packet.

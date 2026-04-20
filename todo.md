# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed a Step 3.2 packet in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` that moves the
bounded same-module helper `StoreGlobalInst` x86 consumer onto prepared
symbol-address lookups instead of spelling RIP globals directly from
`store->global_name`.

## Suggested Next

Continue Step 3.2 by identifying the next public, admitted x86 symbol-backed
consumer that still derives symbol identity locally in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`, then move that
lane onto prepared-address data with proof that stays inside an already
supported route.

## Watchouts

- Do not reopen closed idea 60 value-home or move-bundle work while touching
  address consumers.
- Keep frame size, slot identity, and address provenance in shared prepare,
  not x86-local slot-name or suffix reconstruction.
- The bounded same-module helper path is only observable through the existing
  public multi-defined route contract; adding a new non-trivial helper test
  shape can invalidate that route instead of proving the consumer change.
- The delegated backend subset still has the same pre-existing failing cases as
  `test_before.log`: tests 32, 33, 74, and 75.
- Do not silently activate idea 59 instruction-selection work from this plan.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'` with output in `test_after.log`; the rerun
matched `test_before.log`'s failing set (32, 33, 74, 75) and the focused
`ctest --test-dir build --output-on-failure -R '^backend_x86_handoff_boundary$'`
check passed after rebuild.

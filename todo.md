# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed another Step 3.2 packet in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` by moving the
single-block `i16`/`i64` subtract-return direct frame-slot lane onto
authoritative prepared frame-slot accesses. The helper now matches its short
and long carriers by prepared frame-slot identity instead of raw `slot_name`
equality or local-slot-layout fallback. `tests/backend/backend_x86_handoff_boundary_local_i16_guard_test.cpp`
now drifts every raw short/long slot carrier after prepare to prove the
covered x86 return route still follows canonical prepared frame-slot
addressing data.

## Suggested Next

Continue Step 3.2 by moving another remaining single-block direct frame-slot
return/helper lane, likely `render_prepared_minimal_local_slot_return_if_supported`,
onto authoritative prepared frame-slot matching with drift coverage that proves
raw slot-name carriers no longer matter after prepare.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Covered guard-family direct frame-slot plus same-module global load/store
  lanes now honor prepared addressing even when raw slot/global carriers drift,
  but Step 3.2 is not exhausted yet.
- Keep Step 3.2 focused on direct frame-slot, global-symbol, and
  string-constant access consumption. Pointer-indirect memory operands belong
  to the later Step 3.3 cleanup packet.
- The `i16`/`i64` subtract-return lane now expects prepared direct frame-slot
  accesses at entry indices `0`, `1`, `2`, `3`, `7`, and `8`; if those
  prepared records disappear, the helper correctly rejects the route instead of
  reopening raw slot-carrier matching.
- Some single-block local-return and addressed-local fallback paths still rely
  on older local reconstruction because the current consumer coverage is wider
  in guard-family lanes than in the remaining bounded renderers.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the current packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.2 direct frame-slot subtract-return packet; the focused x86
handoff subset passed and `test_after.log` is the canonical proof log for the
packet.

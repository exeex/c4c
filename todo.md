# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 3 / 10
# Current Packet

## Just Finished

Completed another Step 3.2 packet in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` by moving the
single-block minimal local-slot return lane onto authoritative prepared
frame-slot accesses before the constant-folded fallback path. The helper now
rejects missing prepared direct frame-slot records instead of rebuilding stack
operands from raw `slot_name` carriers, and the single-block return dispatch
prefers that prepared consumer when the route is available.
`tests/backend/backend_x86_handoff_boundary_local_i32_guard_test.cpp` now adds
dedicated local-return coverage that proves the route still emits the
canonical stack load/store sequence and rejects drifted raw carriers once the
prepared frame-slot access records are removed.

## Suggested Next

Continue Step 3.2 by moving another remaining direct frame or symbol-backed
single-block helper lane onto authoritative prepared addressing, likely one of
the bounded same-module global or string-backed memory consumers that still
accept local reconstruction when prepared access metadata goes missing.

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
- The `i16`/`i64` subtract-return lane still expects prepared direct
  frame-slot accesses at entry indices `0`, `1`, `2`, `3`, `7`, and `8`; if
  those prepared records disappear, the helper correctly rejects the route
  instead of reopening raw slot-carrier matching.
- The minimal single-block local-slot return lane now also rejects missing
  prepared frame-slot accesses instead of falling back to raw `slot_name`
  layout recovery, and dispatch reaches that prepared consumer before the
  constant-folded route can erase the addressing ownership boundary.
- Some addressed-local and symbol-backed bounded helper paths still rely on
  older reconstruction because the current consumer coverage is wider in
  direct frame-slot lanes than in the remaining residual renderers.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the current packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.2 minimal local-slot return packet; the focused x86 handoff
subset passed and `test_after.log` is the canonical proof log for the packet.

# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Completed another Step 3.2 packet in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` by moving the
minimal local-slot guard helper's direct same-module global load/store lane
onto authoritative prepared symbol accesses. The helper now requires prepared
`GlobalSymbol` access records for the covered `LoadGlobalInst` and
`StoreGlobalInst` paths instead of rebuilding symbol operands from raw
`global_name` and byte-offset carriers when prepared metadata goes missing.
`tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp` now adds
guard coverage proving the same-module global load/store lane still emits the
canonical symbol-backed sequence when raw carriers drift and rejects the route
once the prepared symbol access records are removed.

## Suggested Next

Continue Step 3.2 by moving another remaining direct symbol-backed helper lane
onto authoritative prepared addressing, likely one of the string-backed
single-block or bounded helper consumers that still accept local reconstruction
when prepared symbol access metadata goes missing.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Covered guard-family direct frame-slot plus same-module global load/store
  lanes now require authoritative prepared addressing rather than raw
  slot/global carriers, but Step 3.2 is not exhausted yet.
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
- The minimal local-slot guard helper now also rejects missing prepared
  same-module global symbol accesses instead of reopening raw `global_name`
  or byte-offset reconstruction for that lane.
- Some addressed-local and string-backed bounded helper paths still rely on
  older reconstruction because the current consumer coverage is wider in
  direct frame-slot and same-module global lanes than in the remaining
  residual renderers.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the current packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.2 same-module global guard/helper packet; the focused x86
handoff subset passed and `test_after.log` is the canonical proof log for the
packet.

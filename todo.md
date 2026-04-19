# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed another Step 3.2 packet in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` by extending the
guard-chain same-module symbol consumer to accept offset `StoreGlobalInst`
lanes and to keep resolved prepared symbol-plus-offset facts authoritative when
spelling fallback memory operands. `tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp`
now covers a same-module offset-store guard route and drifts both raw
store/load global carriers after prepare to prove the covered x86 route still
follows authoritative prepared addressing data.

## Suggested Next

Continue Step 3.2 by moving the remaining direct frame-slot consumers in the
single-block local-return renderers onto authoritative prepared frame-slot
lookups, with drift coverage that proves those routes no longer depend on raw
slot-name carriers after prepare.

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
- Some single-block local-return and addressed-local fallback paths still rely
  on older local reconstruction because the current consumer coverage is wider
  in guard-family lanes than in the remaining bounded renderers.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the current packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.2 offset-store direct-symbol packet; the focused x86 handoff
subset passed and `test_after.log` is the canonical proof log for the packet.

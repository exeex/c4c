# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2
Current Step Title: Direct Frame And Symbol Access Consumption
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the first Step 3.2 packet by threading prepared stack-layout data
into the covered x86 local-slot renderers and preferring prepared memory-access
facts for direct frame-slot and same-module global guard lanes:
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`,
`src/backend/mir/x86/codegen/x86_codegen.hpp`, and
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` now use prepared
frame-slot ids plus canonical stack-slot offsets for covered local loads and
stores, and use prepared symbol-backed accesses for covered same-module global
loads and stores before falling back to older local reconstruction paths.
`tests/backend/backend_x86_handoff_boundary_local_i32_guard_test.cpp` and
`tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp` now drift
raw slot/global carriers after prepare and prove that the covered x86 routes
still follow authoritative prepared addressing data.

## Suggested Next

Continue Step 3.2 by extending prepared-address consumption across the
remaining direct frame-slot and symbol-backed renderers, especially the
single-block local return paths and any residual same-module store lanes that
still rely on raw slot/global carriers when prepared accesses are missing.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Covered guard-family direct frame-slot and same-module global lanes now honor
  prepared addressing even when raw slot/global carriers drift, but Step 3.2 is
  not exhausted yet.
- Keep Step 3.2 focused on direct frame-slot, global-symbol, and
  string-constant access consumption. Pointer-indirect memory operands belong
  to the later Step 3.3 cleanup packet.
- Some addressed-local fallback paths still rely on older local reconstruction
  because the current prepared access producer does not yet publish every
  local-slot-address carrier as a direct frame-slot access.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the current packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.2 direct-frame/symbol packet; the focused x86 handoff subset
passed and `test_after.log` is the canonical proof log for the packet.

# Execution State

Status: Active
Source Idea Path: ideas/open/61_stack_frame_and_addressing_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Frame Layout Consumer Migration
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Completed the first Step 3.1 packet by threading prepared frame facts through
the x86 prepared-module entry points and using them for covered prologue and
epilogue stack adjustment:
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`,
`src/backend/mir/x86/codegen/x86_codegen.hpp`, and
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` now feed prepared
frame size and alignment facts into the covered local-slot renderers, while
`tests/backend/backend_x86_handoff_boundary_local_i32_guard_test.cpp` proves
that stack adjustment follows authoritative prepared frame facts instead of
falling back to local frame-size recomputation.

## Suggested Next

Continue with Step 3.2 by teaching the x86 prepared local-slot consumer to use
prepared addressing lookups for direct frame-slot and symbol-backed memory
operands, so covered loads and stores stop recovering slot or symbol operands
locally.

## Watchouts

- Keep activation scope on idea 61 only. Do not reopen closed idea 58 or
  activate blocked idea 59 through the runbook text.
- Treat value-home and move-bundle ownership as idea 60 scope, not something
  to absorb into this plan.
- Shared prepare now publishes the frame and addressing facts Step 3 needs, and
  covered x86 stack adjustment consumes prepared frame size/alignment, but
  direct frame-slot and symbol-backed memory operand rendering is still local.
- Keep Step 3.2 focused on direct frame-slot, global-symbol, and
  string-constant access consumption. Pointer-indirect memory operands belong
  to the later Step 3.3 cleanup packet.
- Prefer prepared frame/address data over x86-local slot-name, suffix, or
  offset reconstruction whenever the Step 3 packet is covering that lane.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 3.1 frame-layout packet; the focused x86 handoff subset passed
and `test_after.log` is the canonical proof log for the packet.

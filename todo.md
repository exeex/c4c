# Execution State

Status: Active
Source Idea Path: ideas/open/60_scalar_expression_and_terminator_selection_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Idea-60 Ownership And Confirm The Next Scalar Seam
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Step 1 rechecked the clean `c_testsuite_x86_backend_src_00204_c` route and
corrected the stale blocker note in this file. The failing `match` function
does enter `render_prepared_local_slot_guard_chain_if_supported`, but the
first real x86 matcher miss is now `logic.end.8` instruction `0`, where
`render_prepared_cast_inst_if_supported` rejects the cast on the clean route.
That disproves the earlier param-bearing single-join countdown-reservation
theory and keeps ownership in idea 60 without code changes.

## Suggested Next

Build the next Step 2.1 packet around the clean-route cast seam in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`. The target
should be a generic repair for the `logic.end.8` cast path inside
`render_prepared_local_slot_guard_chain_if_supported`, not another dispatcher
or countdown-shape relaxation in `prepared_module_emit.cpp`.

## Watchouts

- Do not revive the rejected dispatcher relaxation in
  `prepared_module_emit.cpp`. Clean-route probing showed `match` has `3`
  branch conditions and `1` join transfer, so the stale single-join countdown
  theory was the wrong route diagnosis.
- The clean `logic.end.8` blocker is still a cast seam inside the local-slot
  renderer. `render_prepared_cast_inst_if_supported` currently requires the
  `SExt I8->I32` operand to match the live same-block `current_i8_name`
  carrier, so any repair must stay generic and avoid testcase-shaped fast
  paths.
- Keep the packet in idea 60. The route remains inside x86 structural dispatch
  after the authoritative prepared handoff and does not push ownership back to
  idea 61.

## Proof

Ran the delegated proof command `cmake --build --preset default && ctest
--test-dir build -j --output-on-failure -R
'^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00204_c)$'`.
`backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00204_c` still failed with the same idea-60
scalar restriction on the clean tree. Supporting investigation used temporary
local-slot diagnostics during a direct `./build/c4cll --codegen asm --target
x86_64-unknown-linux-gnu tests/c/external/c-testsuite/src/00204.c -o
/tmp/00204_match_probe.s` probe, which identified the first clean-route miss as
`match` block `logic.end.8` instruction `0` (`kind=cast`). The temporary probe
was reverted before the canonical proof rerun. Canonical proof log:
`test_after.log`.

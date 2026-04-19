# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Organize prepared_module_emit.cpp For Prepared Control-Flow Consumption
Plan Review Counter: 7 / 10
# Current Packet

## Just Finished

Completed another Step 4 organization packet by extracting the bounded
multi-defined call lane's candidate local-slot layout and local `i32` stack
operand render helpers out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` into the existing
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` seam, reusing the
shared prepared local-slot layout builder and a new biased local-slot memory
helper without reopening Step 3 consumer work.

## Suggested Next

Continue Step 4 with another bounded extraction inside the same
multi-defined-call lane, targeting the remaining candidate-specific instruction
render flow such as the `CallInst`/local load-store orchestration around the
new shared helper calls, but keep that work on file organization rather than
semantic lowering changes.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Keep the next packet in Step 4 file organization. The existing
  `comparison.cpp` translation unit is still not part of the active backend
  target, so Step 4 organization still prefers dedicated prepared render files
  when no existing backend target source owns the seam cleanly.
- The shared local-slot render seam now also owns the caller-pushed candidate
  local-slot layout/memory lookup via an explicit stack-bias parameter. The
  remaining candidate-specific logic in `prepared_module_emit.cpp` is the
  bounded instruction orchestration around those helpers, not the local-slot
  layout math itself.
- The shared prepared immediate-branch helper still covers the immediate local
  guard ownership check that already exists in the boundary suite. The
  add-chain arithmetic variant is not yet published through that same shared
  helper contract, so do not hide that producer-side gap inside Step 4
  organization work.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 4 packet after extracting the candidate local-slot layout and
biased local-slot operand helper into the shared prepared local-slot render
seam; `test_after.log` is the canonical proof log.

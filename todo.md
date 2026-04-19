# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 5.2
Current Step Title: Split Residual Local-Slot Guard Lane
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed Step 5.2 by extracting the residual local add/sub/address guard lane
out of `tests/backend/backend_x86_handoff_boundary_test.cpp` into the focused
`tests/backend/backend_x86_handoff_boundary_local_slot_guard_lane_test.cpp`
translation unit, wiring that runner into the existing
`backend_x86_handoff_boundary_test` executable via `tests/backend/CMakeLists.txt`,
and deleting the moved lane's direct route checks plus their lane-specific
fixture helpers from the monolithic handoff file.

## Suggested Next

Advance to Step 5.3 by reviewing the remaining monolithic ownership after the
local-slot guard split; if only intentionally central harness glue remains,
finish Step 5 by removing any dead leftover helper and declaring the split
route exhausted instead of inventing another artificial lane.

## Watchouts

- Keep Step 5 focused on test translation-unit ownership only. Do not mix the
  next packet with emitter changes, expectation rewrites, or test weakening.
- The extracted compare-branch, joined-branch, short-circuit, and loop files
  now each carry a small duplicated harness subset (`check_route_outputs`,
  targeted control-flow helpers, and local fixture builders); the new
  multi-defined call, rejection, scalar-smoke, and local-slot guard-lane
  files follow that same bounded pattern. Prefer centralizing shared handoff
  helpers only when another family genuinely needs the seam, rather than
  turning Step 5 into a broad harness rewrite.
- The monolithic handoff file is now close to central-harness-only ownership.
  Step 5.3 should verify that claim instead of assuming one more semantic lane
  exists just because a few helper leftovers remain.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for Step 5.2 after splitting the residual local add/sub/address guard lane
into its own translation unit; the proof passed and `test_after.log` is the
canonical proof log for the packet.

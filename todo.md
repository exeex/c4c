# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Organize prepared_module_emit.cpp For Prepared Control-Flow Consumption
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Completed another Step 4 organization packet by extracting the bounded
multi-defined call lane's remaining `CallInst` plus local `i32`
load/store-orchestration helper out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the active
prepared x86 codegen surface in `src/backend/mir/x86/codegen/x86_codegen.hpp`,
so the emitter now delegates that bounded instruction render flow through one
shared helper without reopening Step 3 consumer semantics.

## Suggested Next

Continue Step 4 with another bounded extraction inside the same
multi-defined-call lane, targeting the remaining entry-lane wrapper work in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` such as candidate/data
aggregation around the new helper result, but keep that work on file
organization rather than semantic lowering changes.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Step 4 organization still has to respect the active backend target surface:
  `calls.cpp` is not currently part of `c4c_backend`, so helper extraction for
  this lane must stay on active prepared seams unless the supervisor chooses a
  wider ownership change.
- The shared local-slot render seam still owns the caller-pushed candidate
  local-slot layout/memory lookup via an explicit stack-bias parameter, and
  the new bounded helper now owns the candidate instruction stream. The
  remaining work in `prepared_module_emit.cpp` is the lane-level wrapper/data
  aggregation around those helpers.
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
for this Step 4 packet after extracting the bounded multi-defined call-lane
instruction orchestration into the active prepared helper surface;
`test_after.log` is the canonical proof log.

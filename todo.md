# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.1
Current Step Title: Bounded Multi-Defined Call-Lane Wrapper Contraction
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Completed another Step 4.1 organization packet by extracting the bounded
multi-defined call lane's remaining module-plus-data wrapper orchestration out
of `src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the active
prepared helper surface in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` plus
`src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now delegates
that lane's top-level render assembly through one shared helper instead of a
local lambda while keeping Step 3 semantics unchanged.

## Suggested Next

Continue Step 4.1 with another bounded extraction inside the same
multi-defined-call lane only if the remaining contract gate around helper
availability and `defined_functions.size() > 1` can move onto the same helper
surface cleanly; otherwise treat Step 4.1 as near exhaustion and choose the
next Step 4 packet from the single-function entry orchestration seams instead
of forcing more cosmetic churn here.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Step 4 organization still has to respect the active backend target surface:
  `calls.cpp` is not currently part of `c4c_backend`, so helper extraction for
  this lane must stay on active prepared seams unless the supervisor chooses a
  wider ownership change.
- The shared local-slot render seam now owns the candidate instruction stream,
  same-module helper-prefix rendering, and the combined module-plus-data
  wrapper for the bounded multi-defined lane. The remaining work in
  `prepared_module_emit.cpp` for this family is the adjacent contract gate and
  lane-selection fallback around those helpers.
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
for this Step 4.1 packet after extracting the bounded multi-defined call-lane
module/data wrapper into the active prepared helper surface; the delegated
proof passed and `test_after.log` is the canonical proof log.

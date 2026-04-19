# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.2
Current Step Title: Single-Function Entry Orchestration Extraction
Plan Review Counter: 9 / 10
# Current Packet

## Just Finished

Completed another Step 4.2 organization packet by extracting the
single-function countdown entry fallback wrapper out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the existing
countdown helper seam in `src/backend/mir/x86/codegen/prepared_countdown_render.cpp`
plus `src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now
delegates the “prepared loop-join countdown first, local i32 countdown second”
entry dispatch through one shared helper without changing prepared
control-flow semantics.

## Suggested Next

Continue Step 4.2 with the next bounded single-function entry helper group
still owned directly by `prepared_module_emit.cpp`, most likely by moving the
remaining local-slot-based fallback selection behind one shared helper surface
in `src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` if that can be
done without threading broader emitter-local block state through the API.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Step 4.1 is near exhaustion for now. The remaining multi-defined contract
  gate is intertwined with broader single-function dispatch, so do not force
  more cosmetic 4.1 churn when Step 4.2 entry-helper extraction is available.
- The active Step 4.2 seams now include the prepared param-zero / branch
  helper surface and the prepared countdown helper surface. The next honest
  packet should keep using one of those existing helper owners instead of
  inventing a new file unless no current translation unit can own the
  responsibility.
- The new countdown wrapper owns only route selection between the existing
  prepared loop-join countdown renderer and the existing local i32 countdown
  renderer. Do not treat this as permission to reopen Step 3 countdown
  semantics or to fold unrelated local-slot guard logic into the countdown
  file.
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
for this Step 4.2 packet after moving the countdown entry fallback selection
onto the active prepared countdown helper surface; the proof passed and
`test_after.log` is the canonical proof log.

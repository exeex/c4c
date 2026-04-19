# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.2
Current Step Title: Single-Function Entry Orchestration Extraction
Plan Review Counter: 8 / 10
# Current Packet

## Just Finished

Completed another Step 4.2 organization packet by extracting the prepared
compare-driven single-function entry route-selection wrapper out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the existing
prepared param-zero / branch helper seam in
`src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` plus
`src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now delegates
the “minimal prepared compare branch first, prepared compare-join second”
entry-orchestration check through one shared helper while leaving prepared
control-flow semantics unchanged.

## Suggested Next

Continue Step 4.2 with the next bounded single-function entry helper group
still owned directly by `prepared_module_emit.cpp`, most likely one coherent
non-return dispatch group after the prepared param-zero helper call if an
existing seam such as the prepared local-slot or countdown helper surface can
own that wrapper without dragging unrelated block-local state across the API
boundary.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Step 4.1 is near exhaustion for now. The remaining multi-defined contract
  gate is intertwined with broader single-function dispatch, so do not force
  more cosmetic 4.1 churn when Step 4.2 entry-helper extraction is available.
- The active Step 4.2 seams now include both the prepared local-slot helper
  surface and the prepared param-zero / branch helper surface. The next honest
  packet should keep using one of those existing owners instead of inventing a
  new file unless no current translation unit can own the responsibility.
- The prepared param-zero seam now owns compare-driven entry plan building,
  branch-plan rendering, short-circuit target translation, prepared
  short-circuit join-context lookup, the recursive block-render callback
  adapter, and the top-level prepared compare-driven entry wrapper that tries
  the minimal compare branch lane before the prepared compare-join lane.
  `prepared_module_emit.cpp` should keep owning recursive block rendering
  itself; the next packet should only move another coherent entry
  orchestration group if it does not require threading wider emitter state
  through the helper API.
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
for this Step 4.2 packet after moving the compare-driven entry
route-selection wrapper onto the active prepared param-zero / branch helper
surface; the proof passed and `test_after.log` is the canonical proof log.

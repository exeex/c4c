# Execution State

Status: Active
Source Idea Path: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan Path: plan.md
Current Step ID: 4.3
Current Step Title: Residual Prepared-Module Dispatch Narrowing
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Completed Step 4.2.2 by extracting
`render_local_i16_arithmetic_guard_if_supported()` out of
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` and onto the existing
local-slot helper seam in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp` plus
`src/backend/mir/x86/codegen/x86_codegen.hpp`, so the emitter now delegates
that supported local i16 arithmetic guard entry family through one shared
helper, and Step 4.2 is now reduced to exhausted wrapper extraction work
rather than more honest single-function entry packets.

## Suggested Next

Advance to Step 4.3 with the remaining compact dispatcher and route-selection
surface still owned directly by `prepared_module_emit.cpp`, keeping any
follow-on packet limited to residual top-level narrowing rather than more Step
4.2 helper extraction.

## Watchouts

- Step 3.4 is exhausted for the current runbook. Do not reopen countdown
  fallback cleanup, compare-join reproving, or more residual consumer packets
  unless the plan itself is revised.
- Step 4.1 is near exhaustion for now. The remaining multi-defined contract
  gate is intertwined with broader single-function dispatch, so do not force
  more cosmetic 4.1 churn when Step 4.2 entry-helper extraction is available.
- The active Step 4.2 seams now include the prepared param-zero / branch
  helper surface, the prepared countdown helper surface, and the prepared
  local-slot guard-chain plus local i32/i16 arithmetic helper surface. Treat
  Step 4.2 as exhausted unless a supposed follow-on packet is clearly still
  wrapper ownership rather than Step 4.3 dispatcher narrowing.
- The new local-slot wrapper now owns the full supported guard-chain entry
  family plus its same-module-global data collection. Do not treat that as
  permission to reopen Step 3 short-circuit semantics or to hide unrelated
  value-home/addressing work inside the local-slot helper API.
- The shared prepared immediate-branch helper still covers the immediate local
  guard ownership check that already exists in the boundary suite. Do not hide
  broader producer-side gaps inside Step 4.3 narrowing work now that the local
  arithmetic wrappers have been extracted.
- The broader `^backend_` checkpoint currently reproduces five known failures:
  `backend_prepare_phi_materialize`, `variadic_double_bytes`,
  `variadic_pair_second`, `local_direct_dynamic_member_array_store`, and
  `local_direct_dynamic_member_array_load`; do not widen this packet into
  those unrelated routes.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`
for this Step 4.2.2 packet after moving the local i16 arithmetic guard entry
wrapper onto the active prepared local-slot helper surface; the proof passed
and `test_after.log` is the canonical proof log.

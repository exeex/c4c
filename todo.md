# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.2.3
Current Step Title: Remove Residual Scalar Home-Reconstruction Seams
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 3.2.3 (`Remove Residual Scalar Home-Reconstruction Seams`) now routes the
minimal scalar x86 consumer through a single prepared-home loader in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp` instead of branching on
emitter-local register-vs-stack reconstruction. Register-backed, stack-backed,
and rematerializable-immediate scalar sourcing now all flow through the same
prepared value-home contract path before the existing move-bundle-driven return
logic emits assembly. Focused `backend_x86_handoff_boundary` proof passed, and
supervisor-side monotonic regression guard stayed flat against the matching
focused baseline.

## Suggested Next

Advance to Step 3.3 by widening prepared move-bundle consumption from the
minimal scalar return path into the next bounded join/call/return boundary that
still relies on x86-local movement decisions inside
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`.

## Watchouts

- Do not grow x86-local matcher or ABI/home guessing shortcuts.
- Keep value-home and move-bundle ownership in shared prepare.
- Do not silently fold idea 61 addressing/frame work or idea 59 generic
  instruction selection into this route.
- Keep the new consumer surface keyed by existing prepared IDs and name-table
  lookups rather than widening into string-owned parallel state.
- Parameter value homes still need to mean the canonical entry ABI location for
  consumers, not the later regalloc-assigned destination register that
  `BeforeInstruction` bundles may target.
- The bounded stack-home proofs still mutate prepared value homes inside the
  scalar smoke tests; that is acceptable as bounded proof for the cleaned
  consumer, but later validation should still look for a naturally produced
  stack-backed lane if one can stay honest and narrow.
- The rematerializable scalar proof is still limited to the immediate-root
  no-parameter lane; later widening should confirm shared producer support
  before depending on rematerializable parameter-style sources.
- `PreparedMovePhase::BlockEntry` is currently inferred from shared
  `phi_...` move reasons, while call/result/return bundles come from
  destination-kind classification in shared prepare; keep any later phase
  refinement shared instead of pushing it into x86.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary' > test_after.log 2>&1`,
which passed after the Step 3.2.3 prepared-home loader cleanup in
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`. `test_after.log` is the
canonical proof artifact for this packet. Supervisor-side monotonic regression
guard
(`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py
--before test_before.log --after test_after.log
--allow-non-decreasing-passed`) passed with no new failing tests and no
pass-count regression.

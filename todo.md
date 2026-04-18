# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by adding a shared helper
that resolves the full materialized compare-join render contract, including the
authoritative same-module globals, directly from the prepared module,
switching the x86 compare-join consumer to use that resolved contract instead
of composing branch-plan/global-resolution pieces locally, and extending the
handoff-boundary test suite to prove the resolved contract preserves the shared
branch plan, return contexts, return shapes, and same-module-global order for
this materialized compare-join family.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join composition seam, most likely around selected-value
render dispatch or branch-label assembly inside
`render_materialized_compare_join_if_supported()`, so
`prepared_module_emit.cpp` keeps shrinking its compare-join-specific control
logic without drifting into Step 4 file organization. Keep the work in shared
consumer helpers and focused proof, not emitter cleanup or broader backend
route changes.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join render contract now packages the authoritative
  branch labels, false-branch opcode, true/false return contexts, true/false
  return-lane shapes, and same-module-global ownership set, and the new helper
  resolves that full contract from the prepared module while preserving
  authoritative global order. Keep follow-on work focused on moving the
  remaining compare/join composition decisions into shared helpers instead of
  rebuilding lane semantics or global ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, or local join bundle reconstruction
  in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving compare-join render-contract
  resolution into the shared helper path.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared compare-join
render-contract resolution helper, the x86 consumer that now uses the resolved
authoritative contract instead of composing branch-plan/global-resolution
pieces locally, and the existing prepared branch/join ownership families that
still prove the same handoff contracts.

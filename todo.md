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
that directly publishes the resolved materialized compare-join render contract
from the prepared compare-join branches, switching the x86 compare-join
consumer to use that combined prepared helper instead of locally composing
render-contract construction plus resolution, and extending the
handoff-boundary test suite to prove the direct helper preserves the resolved
branch plan and authoritative same-module-global order for this
materialized compare-join family.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join composition seam inside
`render_materialized_compare_join_if_supported()`, most likely around the
duplicated true/false return rendering path so the emitter consumes one more
prepared compare-join helper instead of pairing return contexts and shapes
locally. Keep the work in shared consumer helpers and focused proof, not
emitter cleanup or broader backend route changes.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join render contract now has a direct resolved-helper path
  from prepared compare-join branches to authoritative branch labels,
  false-branch opcode, true/false return contexts, true/false return-lane
  shapes, and same-module-global ownership. Keep follow-on work focused on
  moving the remaining compare/join composition decisions into shared helpers
  instead of rebuilding lane semantics or global ownership in x86.
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
`backend_x86_handoff_boundary` subset for the new shared direct resolved
compare-join render-contract helper, the x86 consumer that now uses that
combined prepared helper instead of composing render-contract construction and
resolution locally, and the existing prepared branch/join ownership families
that still prove the same handoff contracts.

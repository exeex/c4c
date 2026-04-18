# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by adding a shared
prepared compare-join branch-plan helper and switching the minimal x86
materialized compare-join path to consume that helper instead of an
emitter-local compare-join render struct. The Step 3 x86 consumer now takes the
authoritative false-branch opcode plus true/false predecessor labels from
shared prepare code before rendering the compare-join returns, and the
handoff-boundary test suite now proves that branch-plan contract even when raw
entry terminator labels are desynchronized from prepared metadata.

## Suggested Next

The next accepted packet should stay in Step 3 and remove one more small
x86-local compare/join composition seam, most likely by moving one more
materialized compare-join return-lane ownership or same-module-global assembly
decision behind a shared prepared consumer helper instead of open-coded
lane-specific rendering in `prepared_module_emit.cpp`. Keep the work in shared
consumer helpers and focused proof, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The new shared compare-join branch-plan helper only covers authoritative
  compare true/false predecessor labels plus the false-branch opcode; follow-on
  work should continue moving compare/join composition into shared prepare code
  instead of rebuilding lane ownership in x86.
- The x86 short-circuit and compare-join consumers should continue treating
  returned prepared lane ownership, continuation labels, and compare-join
  branch labels as authoritative; do not reintroduce raw selected-value
  plumbing, source-label equality checks, or local join bundle reconstruction
  in the emitter.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshes `test_after.log`
  with the same focused proof command after moving the compare-join entry
  branch-plan contract into shared prepare code.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof refreshes `test_after.log` with the
`backend_x86_handoff_boundary` subset for the new shared compare-join
branch-plan helper, the x86 consumer that now resolves it, and the existing
prepared branch/join ownership families that still prove the same handoff
contracts.

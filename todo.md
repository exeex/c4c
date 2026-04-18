# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending the shared
materialized compare-join return contract with explicit prepared base-render
metadata for selected values, switching the x86 compare-join return renderer
to consume that prepared base contract instead of reusing the generic
param-derived helper for the base move decision, and adding focused ownership
coverage that asserts the shared helper now publishes the param-backed base
plus trailing immediate-op contract before asm emission.

## Suggested Next

The next accepted packet should keep shrinking Step 3 emitter-local seams in
the same joined-branch family by extending the prepared compare-join return
contract and ownership tests to cover direct immediate selected-value bases in
addition to param-backed bases, so the remaining compare-join return path stays
fully on shared consumer metadata for both immediate and param-origin values.
Keep that work in semantic consumer helpers, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared return-context helper in `prealloc.hpp` now owns the selected
  value base classification (`ParamValue` vs immediate), the selected-value
  immediate-op chain, and the supported trailing immediate-op classification
  for materialized compare joins; follow-on work should extend that prepared
  helper surface rather than routing base rendering back through generic x86
  value helpers.
- The compare-join return renderer should now treat prepared computed-value
  base metadata as authoritative; do not reintroduce raw named-value or
  join-block prefix binary lookups just to recover the base move decision.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof passed and refreshed `test_after.log` with the
`backend_x86_handoff_boundary` subset for the prepared compare-join base-render
contract, the selected-value computed metadata handoff, and the chained
selected-value xor `EdgeStoreSlot` ownership coverage.

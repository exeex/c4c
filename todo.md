# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by teaching
`find_prepared_materialized_compare_join_branches()` to publish fully prepared
true/false compare-join return contexts, switching the x86 compare-join
branch renderer to consume those branch-specific prepared return contexts
directly instead of passing raw selected values back into shared helpers, and
adding focused ownership coverage that asserts the shared branch contract now
includes the prepared param-backed base plus trailing immediate-op data for
both arms.

## Suggested Next

The next accepted packet should keep shrinking Step 3 emitter-local seams in
the same joined-branch family by extending the prepared compare-join branch
return contract and ownership coverage to direct immediate selected-value
bases, so the same shared branch-return helper surface proves both
param-origin and immediate-origin selected values without routing that decision
back through x86-local value classification. Keep that work in semantic
consumer helpers, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join branch helper in `prealloc.hpp` now owns the
  per-arm prepared return contexts, including selected-value base
  classification, selected-value immediate-op chains, and the supported
  trailing immediate-op contract; follow-on work should extend that prepared
  branch-return surface rather than recomputing per-arm return metadata in the
  emitter.
- The x86 compare-join branch renderer should now treat the prepared true/false
  return contexts as authoritative; do not reintroduce raw selected-value
  plumbing just to call shared classification helpers from x86 again.
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
`backend_x86_handoff_boundary` subset for the direct prepared compare-join
branch return contexts, the selected-value computed metadata handoff, and the
chained selected-value xor `EdgeStoreSlot` ownership coverage.

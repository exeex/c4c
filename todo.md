# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by promoting the
selected-value named-binary chain used before materialized compare-join
trailing returns into shared prepared computed-value metadata, switching the
x86 compare-join return renderer to consume that prepared selected-value
metadata instead of walking join-block prefix binaries locally, and extending
ownership coverage with a chained selected-value `EdgeStoreSlot` compare-join
fixture that would fail the old single-hop x86 lookup.

## Suggested Next

The next accepted packet should keep shrinking Step 3 emitter-local seams in
the same joined-branch family by lifting one more materialized compare-join
return dependency out of `prepared_module_emit.cpp`, ideally by turning the
base selected-value move decision itself into prepared consumer metadata so the
compare-join return renderer no longer reuses the generic param-derived helper
for prepared computed-value bases. Keep that work in semantic consumer helpers,
not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared return-context helper in `prealloc.hpp` now owns both the
  authoritative join-block selected-value computation classification and the
  supported trailing immediate-op classification for materialized compare
  joins; follow-on work should extend that prepared helper surface rather than
  rebuilding join-block value walks or binary shape checks in the emitter.
- The chained selected-value xor ownership test now also proves the
  `PreparedJoinTransferKind::EdgeStoreSlot` carrier path; do not reintroduce
  select-only assumptions, raw trailing-binary validation, or one-hop
  join-prefix map walking in the compare-join return renderer.
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
`backend_x86_handoff_boundary` subset for the prepared selected-value computed
metadata handoff plus the chained selected-value xor `EdgeStoreSlot`
ownership coverage.

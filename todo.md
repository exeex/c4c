# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending the
prepared compare-join ownership seam from param/immediate selected-value bases
to same-module `LoadGlobal i32` roots. Shared compare-join helpers now publish
`PreparedComputedBaseKind::GlobalI32Load` for supported join-block
`LoadGlobalInst` roots, and the focused handoff tests prove both the x86
consumer path and the same `PreparedJoinTransferKind::EdgeStoreSlot` contract
treat those global-root selected values plus the trailing xor contract as
authoritative prepared control-flow data.

## Suggested Next

The next accepted packet should stay in Step 3 and extend the same prepared
compare-join branch-return ownership surface to one additional bounded
non-param selected-value shape that can still be expressed as shared prepared
data, with preference for a small chain on top of the new global-load root or
another root family that does not require idea 61 stack/addressing work. Keep
the work in shared consumer helpers and focused ownership tests, not Step 4
file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join branch helper surface now has focused coverage for
  param-backed selected-value bases, direct immediate selected-value bases,
  immediate-base selected-value chains, same-module global-load selected-value
  bases, and the same immediate/global contracts when the join carrier is an
  `EdgeStoreSlot`; follow-on work should keep extending that prepared contract
  instead of reconstructing per-arm return metadata in x86.
- The x86 compare-join branch renderer should continue treating the prepared
  true/false return contexts as authoritative; do not reintroduce raw
  selected-value plumbing just to call shared classification helpers from x86
  again.
- `classify_computed_value()` now accepts supported immediate-root binary
  chains; follow-on packets should preserve that shared classification route
  instead of teaching x86 to special-case immediate-root join values locally.
- The new global-root lane intentionally keeps same-module scalar-load legality
  in the x86 consumer and does not open local-slot/address-based roots; do not
  widen this packet into idea 61 frame/addressing work.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command after adding same-module global selected
  value ownership checks.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof passed and refreshed `test_after.log` with the
`backend_x86_handoff_boundary` subset for the same-module global-load
selected-value compare-join return contexts, the matching `EdgeStoreSlot`
joined-branch route that consumes them, and the previously covered param-based
and immediate compare-join ownership seams.

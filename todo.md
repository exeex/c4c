# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending the focused
prepared compare-join ownership coverage from zero-offset pointer-backed
same-module global roots to one additional bounded family: fixed-offset
pointer-backed same-module global selected values and selected-value chains.
The existing shared prepared computed-value contract and Step 3 x86
compare-join consumer already carried this bounded offset route correctly; the
new focused ownership fixtures now prove both the plain joined-branch path and
the `PreparedJoinTransferKind::EdgeStoreSlot` carrier preserve pointer-root
global provenance, fixed-offset same-module scalar-load classification,
selected-value chain operations, and the authoritative compare-join return
contexts through the shared helper surface.

## Suggested Next

The next accepted packet should stay in Step 3 and either extend the same
prepared compare-join ownership surface to one additional bounded same-module
global-root shape that still fits shared prepared data without opening idea 61,
or escalate to a broader backend proving packet now that the compare-join
consumer has accumulated direct, chain, fixed-offset, pointer-backed, and
fixed-offset pointer-backed ownership coverage. Keep the work in shared
consumer helpers and focused proof, not Step 4 file organization.

## Watchouts

- Keep this family in Step 3 semantic consumer helpers; do not widen into Step
  4 file organization, idea 57, idea 59, idea 60, idea 61, or countdown-loop
  route changes.
- Do not solve remaining compare-join gaps with x86-side CFG scans or
  testcase-shaped matcher growth.
- The shared compare-join branch helper surface now has focused coverage for
  param-backed selected-value bases, direct immediate selected-value bases,
  immediate-base selected-value chains, direct same-module global-load
  selected-value bases, bounded same-module global-root immediate-op chains,
  fixed-offset same-module global-load selected-value bases, fixed-offset
  same-module global-root selected-value chains, direct pointer-backed
  same-module global selected values, pointer-backed same-module global
  selected-value chains, fixed-offset pointer-backed same-module global
  selected values, fixed-offset pointer-backed same-module global
  selected-value chains, and the same immediate/global contracts when the join
  carrier is an `EdgeStoreSlot`; follow-on work should keep extending that
  prepared contract instead of reconstructing per-arm return metadata in x86.
- The x86 compare-join branch renderer should continue treating the prepared
  true/false return contexts as authoritative; do not reintroduce raw
  selected-value plumbing just to call shared classification helpers from x86
  again.
- `classify_computed_value()` now accepts supported immediate-root binary
  chains; follow-on packets should preserve that shared classification route
  instead of teaching x86 to special-case immediate-root join values locally.
- The global-root lane still intentionally keeps same-module scalar-load
  legality in the x86 consumer and now covers both zero-offset and fixed-offset
  aggregate loads plus bounded immediate-op chains, plus pointer-backed and
  fixed-offset pointer-backed same-module global-root families for both direct
  loads and selected-value chains, but it still does not open nonzero
  pointer-base offsets, local-slot/address-based roots, or generic non-global
  pointer provenance; do not widen follow-on work into idea 61 frame/addressing
  work.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command after adding fixed-offset pointer-backed
  same-module global selected-value and selected-value-chain ownership checks.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof passed and refreshed `test_after.log` with the
`backend_x86_handoff_boundary` subset for fixed-offset pointer-backed
same-module global selected values, fixed-offset pointer-backed same-module
global selected-value chains, the matching compare-join return contexts, the
`EdgeStoreSlot` joined-branch route that consumes them, and the previously
covered param-based, immediate, same-module global, fixed-offset global, and
zero-offset pointer-backed compare-join ownership seams.

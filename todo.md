# Execution State

Status: Active
Source Idea: ideas/open/58_bir_cfg_and_join_materialization_for_x86.md
Source Plan: plan.md

# Current Packet

## Just Finished

Completed a Step 3 Consume Prepared Control-Flow packet in
`src/backend/prealloc/prealloc.hpp`,
`src/backend/mir/x86/codegen/prepared_module_emit.cpp`, and
`tests/backend/backend_x86_handoff_boundary_test.cpp` by extending the focused
prepared compare-join handoff from direct same-module `GlobalI32Load` roots to
one additional bounded non-param family: pointer-backed same-module global
selected-value roots. The shared prepared computed-value contract now carries
pointer-root global provenance, the Step 3 x86 compare-join consumer uses that
prepared data instead of recovering the root locally, and the focused
ownership fixtures prove both the plain joined-branch path and the
`PreparedJoinTransferKind::EdgeStoreSlot` carrier classify and consume
pointer-backed same-module global selected values through the shared helper
surface.

## Suggested Next

The next accepted packet should stay in Step 3 and extend the same prepared
compare-join branch-return ownership surface to one additional bounded
pointer-backed same-module global-root shape, with preference now on a
selected-value chain or fixed-offset pointer-backed variant that still fits
shared prepared data without opening idea 61 address/frame work. Keep the work
in shared consumer helpers and focused ownership tests, not Step 4 file
organization.

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
  same-module global-root selected-value chains, and the same
  immediate/global contracts when the join carrier is an `EdgeStoreSlot`;
  follow-on work should keep extending that prepared contract instead of
  reconstructing per-arm return metadata in x86.
- The x86 compare-join branch renderer should continue treating the prepared
  true/false return contexts as authoritative; do not reintroduce raw
  selected-value plumbing just to call shared classification helpers from x86
  again.
- `classify_computed_value()` now accepts supported immediate-root binary
  chains; follow-on packets should preserve that shared classification route
  instead of teaching x86 to special-case immediate-root join values locally.
- The global-root lane still intentionally keeps same-module scalar-load
  legality in the x86 consumer and now covers both zero-offset and fixed-offset
  aggregate loads plus bounded immediate-op chains, and now also covers a
  bounded pointer-backed same-module global-root direct-load family, but it
  still does not open local-slot/address-based roots; do not widen this packet
  into idea 61 frame/addressing work.
- The new pointer-backed lane is intentionally bounded to direct selected-value
  roots whose `LoadGlobal i32` records shared same-module pointer-root
  provenance; do not widen follow-on work into generic address-based loads or
  non-global pointer provenance.
- The joined-branch ownership tests still intentionally desynchronize raw entry
  terminator labels from prepared branch metadata; do not restore source-label
  equality checks in the x86 consumer.
- `test_before.log` remains the narrow baseline for
  `^backend_x86_handoff_boundary$`, and this packet refreshed `test_after.log`
  with the same focused proof command after adding pointer-backed same-module
  global selected-value ownership checks.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' | tee test_after.log`.
The focused proof passed and refreshed `test_after.log` with the
`backend_x86_handoff_boundary` subset for pointer-backed same-module global
selected-value roots, the matching compare-join return contexts, the
`EdgeStoreSlot` joined-branch route that consumes them, and the previously
covered param-based, immediate, and same-module global compare-join ownership
seams.

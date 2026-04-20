# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 2 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now routes the bounded compare-driven parameter-leaf return lane
through the shared `BeforeReturn` handoff instead of rebuilding the return move
from local ABI assumptions. The x86 compare-branch helper now receives the
return block context, looks up the authoritative `BeforeReturn` bundle for that
leaf, and emits the named return from the prepared value home at that boundary.
Focused `backend_x86_handoff_boundary` proof passed after adding a dedicated
compare-branch contract-drift test that removes the prepared return bundle and
verifies the x86 route rejects the drift instead of reopening a local return
fallback.

## Suggested Next

Keep Step 3.3 on boundary consumption and audit the next joined-branch or
materialized-compare leaf that still emits a named return without consulting a
shared `BeforeReturn` bundle. Prefer the smallest supported compare-join lane
where a focused contract-drift test can remove the return bundle and prove the
x86 route rejects the missing prepared boundary metadata.

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
- The bounded direct extern-call route now depends on prepared
  value-location context being threaded through the single-block dispatch
  path; keep any follow-up boundary work on that shared handoff instead of
  reintroducing default ABI register assumptions in local helpers.
- The new compare-branch drift proof in
  `backend_x86_handoff_boundary_compare_branch_test.cpp` deletes the
  parameter-leaf `BeforeReturn` bundle rather than mutating branch labels; keep
  that check focused on return-boundary ownership instead of folding it into
  broader prepared-control-flow assertions.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the Step 3.3 compare-branch parameter-leaf return helper was
rewired onto shared `BeforeReturn` bundle lookup and the focused missing-bundle
contract-drift proof was added. `test_after.log` is the canonical proof
artifact for this packet.

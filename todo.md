# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now threads prepared name/value-location context into
`render_prepared_minimal_direct_extern_call_sequence_if_supported(...)` so the
bounded direct extern-call handoff reads shared `BeforeCall` / `AfterCall`
bundles and canonical value-home lookups instead of defaulting argument/result
ABI placement locally. The x86 consumer now sources named i32 call arguments
from prepared homes or the current prepared call-result carrier, executes
shared `BeforeCall` destination-register moves when present, and applies
shared `AfterCall` result-home moves before the next consumer reads the value.
Focused `backend_x86_handoff_boundary` proof passed after adding a dedicated
direct extern-call contract-drift test that mutates the shared call bundle and
value-home metadata to prove the x86 route follows the prepared handoff.

## Suggested Next

Keep Step 3.3 on boundary consumption and audit the remaining supported-path
join or return helpers for any surviving ABI/home fallback that still bypasses
shared prepared move bundles. Start with the next bounded helper that still
defaults a boundary move when prepared metadata is absent, and add a focused
contract-drift proof instead of widening into generic call lowering.

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
- The new contract-drift proof in
  `backend_x86_handoff_boundary_direct_extern_call_test.cpp` intentionally
  mutates prepared `BeforeCall` / `AfterCall` metadata plus the first call
  result home; if later refactors change bundle indexing, update the proof to
  keep validating shared direct-call boundary ownership rather than deleting
  the drift coverage.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the Step 3.3 bounded direct extern-call consumer was
rewired onto shared prepared call bundles / value-home lookups and the focused
contract-drift proof was added. `test_after.log` is the canonical proof
artifact for this packet.

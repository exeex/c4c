# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now threads `PreparedBirModule` / prepared value-location context
through the bounded multi-defined dispatch surfaces
(`prepared_module_emit.cpp`, `x86_codegen.hpp`, and
`prepared_local_slot_render.cpp`) so the same-module call-lane renderer can
consume shared `BeforeCall` / `AfterCall` bundles instead of reconstructing i32
call argument/result ABI movement locally. The bounded route now drives named
call arguments from prepared `BeforeCall` metadata, applies `AfterCall`
result-home moves when shared prepare publishes them, and the focused
multi-defined call test now includes a prepared-contract drift lane that
mutates the shared call bundles / value home to prove the x86 consumer follows
that authoritative handoff. Focused `backend_x86_handoff_boundary` proof
passed and produced `test_after.log`.

## Suggested Next

Advance Step 3.3 into the next bounded call boundary that still reconstructs
ABI movement locally, most likely
`render_prepared_minimal_direct_extern_call_sequence_if_supported(...)` in
`src/backend/mir/x86/codegen/prepared_local_slot_render.cpp`, and move its
named call-argument / call-result handling onto shared prepared
`BeforeCall` / `AfterCall` bundles without widening into unrelated generic call
lowering.

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
- The bounded multi-defined call-lane route now depends on prepared
  value-location context being threaded through the dispatch state; keep any
  follow-up call-boundary work on that shared handoff path instead of cloning
  new route-local ABI helpers.
- The new contract-drift proof in
  `backend_x86_handoff_boundary_multi_defined_call_test.cpp` intentionally
  mutates prepared `BeforeCall` / `AfterCall` metadata for one bounded lane; if
  later refactors change bundle indexing, update the proof to keep validating
  shared call-boundary ownership rather than deleting the drift coverage.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the Step 3.3 bounded multi-defined call-lane consumer was
rewired onto shared prepared call bundles and the focused contract-drift proof
was added. `test_after.log` is the canonical proof artifact for this packet.

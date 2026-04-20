# Execution State

Status: Active
Source Idea Path: ideas/open/60_prepared_value_location_consumption.md
Source Plan Path: plan.md
Current Step ID: 3.3
Current Step Title: Consume Canonical Move Bundles For Join, Call, And Return Boundaries
Plan Review Counter: 4 / 10
# Current Packet

## Just Finished

Step 3.3 (`Consume Canonical Move Bundles For Join, Call, And Return
Boundaries`) now finishes the compare-join return-bundle handoff. The shared
compare-join return context no longer points at `join_block->insts.size()`;
instead it records the join-return source point, and the x86 consumer resolves
the authoritative `BeforeReturn` bundle from prepared value locations at that
block boundary. Temporary debug instrumentation and the extra post-render drift
hack were removed, and the focused missing-bundle compare-join test still
proves that x86 rejects reopening a local ABI return fallback.

## Suggested Next

Keep Step 3.3 on boundary consumption and audit the next prepared leaf that
still reconstructs a call/return/register boundary from local x86 assumptions
instead of consuming shared move bundles directly.

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
- Compare-join return rendering now tolerates stale local instruction-index
  hints by resolving the unique authoritative `BeforeReturn` bundle for the
  block when the exact hinted point is not present; keep any future refinement
  shared and bundle-driven rather than adding x86-only fallback reconstruction.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_x86_handoff_boundary$' > test_after.log 2>&1`,
which passed after the compare-join return handoff was rewired onto the
authoritative prepared `BeforeReturn` bundle lookup. `test_after.log` is the
canonical proof artifact for this packet.

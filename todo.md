Status: Active
Source Idea Path: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Review Entrypoint And Helper Placement

# Current Packet

## Just Finished

Step 3 of `plan.md` verified the accepted header-normalization slice.
`memory_helpers.hpp` is now a normal `#pragma once` header exposing only the
pure helper result structs and free helper declarations recorded in Step 1:

- `ScalarLayoutLeafFacts`
- `ScalarLayoutByteOffsetFacts`
- `AggregateByteOffsetProjection`
- `resolve_scalar_layout_facts_at_byte_offset(...)`
- `resolve_aggregate_byte_offset_projection(...)`
- `can_reinterpret_byte_storage_as_type(...)`

No member-fragment macro mode, macro-mode guards, declaration-injection shape,
or hidden stateful `BirFunctionLowerer` member declarations remain in
`memory_helpers.hpp`.

## Suggested Next

Delegate Step 4 as a narrow placement review packet: confirm memory entrypoints
and stateful helper flows remain `BirFunctionLowerer` members in
`lowering.hpp`, pure reusable reasoning stays in `memory_helpers.hpp`, and
file-private glue stays local to the memory `.cpp` files.

## Watchouts

- The active repo path is `src/backend/bir/lir_to_bir/...`, not the stale
  `src/c4c/lir_to_bir/...` path in the runbook.
- `memory_helpers.hpp` still includes `../lowering.hpp` because the remaining
  pure helper declarations use `BirFunctionLowerer` nested aliases/types.
- No local-slot helper movement was attempted in this packet.
- Step 4 should be a placement review first; do not move local-slot code unless
  the review finds a narrow, mechanical move that the supervisor explicitly
  delegates with compile proof.

## Proof

Read-only verification packet; no build/test proof was delegated or run.
No `test_after.log` was created. Step 2 already built and passed the
`^backend_` subset after the same header normalization; current canonical proof
log path remains `test_before.log`.

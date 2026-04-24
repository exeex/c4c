Status: Active
Source Idea Path: ideas/open/04_bir-memory-entrypoints-and-helper-boundaries.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove No Behavior Change

# Current Packet

## Just Finished

Step 4 of `plan.md` reviewed the post-cleanup memory boundary placement.
The target memory entrypoints remain explicit `BirFunctionLowerer` members in
`lowering.hpp`:

- `lower_local_memory_alloca_inst(...)`
- `lower_memory_gep_inst(...)`
- `lower_memory_store_inst(...)`
- `lower_memory_load_inst(...)`
- `lower_memory_memcpy_inst(...)`
- `lower_memory_memset_inst(...)`
- `try_lower_direct_memory_intrinsic_call(...)`

No broad `foo(BirFunctionLowerer& self, ...)` memory-lowering rewrite was found
under `src/backend/bir/lir_to_bir/memory/` or the adjacent split lowering
translation units. Pure reusable reasoning remains confined to
`memory_helpers.hpp` as result structs plus the three free helper declarations:
`resolve_scalar_layout_facts_at_byte_offset(...)`,
`resolve_aggregate_byte_offset_projection(...)`, and
`can_reinterpret_byte_storage_as_type(...)`.

File-private glue remains local: local-slot-only structs/lambdas stay in
`memory/local_slots.cpp`, helper implementation glue stays anonymous or
translation-unit local where it is used, and no implementation file was changed.
No safe narrow movement out of `local_slots.cpp` is recommended from this
review packet. The visible candidates there, including memcpy/memset and direct
memory intrinsic lowering, are still tied to local-slot state, value aliases,
family-failure notes, and local aggregate/address bookkeeping; moving them
should be deferred until a separately delegated compile-proven packet can own a
specific family.

## Suggested Next

Delegate Step 5 as the behavior-preservation proof packet: build the backend
target selected by the supervisor and run the relevant BIR/LIR-to-BIR narrow
tests without expectation rewrites.

## Watchouts

- The active repo path is `src/backend/bir/lir_to_bir/...`, not the stale
  `src/c4c/lir_to_bir/...` path in the runbook.
- `memory_helpers.hpp` still includes `../lowering.hpp` because the remaining
  pure helper declarations use `BirFunctionLowerer` nested aliases/types.
- `local_slots.cpp` remains large and mixed, but this packet found no
  placement-only move narrow enough to justify touching implementation files
  without compile proof.
- Any future move out of `local_slots.cpp` should name one family precisely and
  prove it with a fresh build/test subset.

## Proof

Read-only Step 4 placement review; no build/test proof was delegated or run.
No `test_after.log` was created. Current canonical prior proof log remains
`test_before.log`.

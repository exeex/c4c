# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: classified the prepared local load/frame-slot
lookup family and relocated it to the AArch64 memory owner. The public
`find_frame_slot`, `find_stack_object`, `prepared_frame_slot_load_address`, and
`prepared_local_load_offset` declarations now live in `memory.hpp`, their
definitions live in `memory.cpp`, and `dispatch_publication.hpp` no longer
declares them. Removed memory's duplicate private stack-layout lookup helpers
and routed existing memory users through the newly public memory-owned helpers.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `todo.md`

## Suggested Next

Continue Step 3 with the next still-unclassified dispatch-publication helper
family, excluding scalar/register helpers, current-block entry publication,
variadic support, generic prepared-value-home helpers, address materialization,
dispatch producer/source-producer support, and the prepared local
load/frame-slot lookup family now classified into memory.

## Watchouts

- `memory.cpp` still includes `dispatch_publication.hpp` for unrelated helper
  families used elsewhere in memory lowering; this packet only moved the
  stack-layout lookup/prepared load-address family.
- `comparison.cpp` has a private branch-fusion-specific frame-slot lookup and
  prepared frame-slot load-address helper with similar names; it was outside
  this packet and remains untouched.
- Root-level `test_before.log` and `test_baseline.log` were already present
  alongside the required `test_after.log`; this packet wrote only
  `test_after.log`.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`

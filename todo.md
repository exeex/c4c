# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the dispatch publication
source-producer helper group into the AArch64 producer owner. The public
`prepared_publication_source_producer_for_value` and
`prepared_source_producer_instruction` declarations now live in
`dispatch_producers.hpp`, their definitions live in `dispatch_producers.cpp`,
and `dispatch_publication.hpp` no longer declares them. Removed the duplicate
private same-block publication source-producer support from
`dispatch_publication.cpp`; the remaining implementation is the existing
producer-owned helper in `dispatch_producers.cpp`.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_producers.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `todo.md`

## Suggested Next

Continue Step 3 with the next dispatch-publication helper family, excluding the
scalar, current-block entry, variadic, generic prepared-value-home, and
address-materialization, and dispatch producer/source-producer families already
classified. A likely remaining family is the prepared local load/frame-slot
lookup support, if the supervisor wants to continue owner-by-owner relocation.

## Watchouts

- The relocated public producer helper keeps the publication-owned fallback path
  that builds local edge publication source-producer lookups when indexed
  prepared lookups are unavailable.
- `dispatch_producers.cpp` still includes `dispatch_publication.hpp` for
  unrelated current-block entry publication and prepared-value-home queries used
  by producer collision checks; this packet did not broaden into those helper
  families.
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

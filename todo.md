# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the address-materialization/local
slot address publication helper family out of dispatch publication and into the
AArch64 globals/address-materialization owner. `local_aggregate_address_frame_offset`,
`emit_local_slot_address_publication_to_register`,
`lower_local_slot_address_publication`, and
`record_address_materialization_result` are now declared from `globals.hpp` and
defined in `globals.cpp`; `prepared_frame_address_offset_for_value` moved with
the family as the private lookup helper.
`dispatch_publication.cpp` no longer defines them and
`dispatch_publication.hpp` no longer declares them directly.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/globals.cpp`
- `src/backend/mir/aarch64/codegen/globals.hpp`
- `todo.md`

## Suggested Next

Continue Step 3 with the next dispatch-publication helper family, excluding
the scalar, current-block entry, variadic, generic prepared-value-home, and
address-materialization families already classified.

## Watchouts

- Call sites needing the moved declarations already include `globals.hpp`
  directly; no compile-only test include updates were required.
- `globals.cpp` still includes `dispatch_publication.hpp` for unrelated
  publication-owner utilities such as `scalar_view_for_type` and
  `prepared_value_home_for_value`; this packet did not broaden into those
  unrelated helper families.
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

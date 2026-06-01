# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Continue owner-by-owner relocation

## Just Finished

Completed Step 3 from `plan.md`: relocated the variadic `va_list` field address
publication helper family out of dispatch publication and into the AArch64
variadic owner. `prepared_va_list_field_address` is now declared from
`variadic.hpp` and defined in `variadic.cpp`; the suffix parser moved with it as
the private `parse_va_list_field_suffix` helper. `dispatch_publication.cpp` no
longer defines the family and `dispatch_publication.hpp` no longer declares it.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/variadic.cpp`
- `src/backend/mir/aarch64/codegen/variadic.hpp`
- `todo.md`

## Suggested Next

Continue Step 3 with the next dispatch-publication helper family, excluding the
scalar, current-block entry, variadic, generic prepared-value-home, and
address-materialization families already classified. A likely remaining family
is the dispatch producer/source-producer helper group.

## Watchouts

- `dispatch_edge_copies.cpp` also uses `prepared_va_list_field_address`, so the
  helper remains a public `variadic.hpp` API. That file already included
  `variadic.hpp`, so no non-owned include edit was required.
- `variadic.cpp` still includes `dispatch_publication.hpp` for unrelated scalar
  helper declarations such as `scalar_view_for_type` and `gp_register_name`;
  this packet did not broaden into those helper families.
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

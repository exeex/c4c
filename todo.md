Status: Active
Source Idea Path: ideas/open/523_bir_route2_select_chain_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Extract route2 bodies

# Current Packet

## Just Finished

Completed Step 3 of `plan.md`: moved the selected route2 select-chain
implementation bodies from `src/backend/bir/bir.cpp` to the new
`src/backend/bir/bir_route2.cpp`.

Moved route2 bodies:

- `route2_select_chain_producer_kind`
- `route2_select_chain_producer_record`
- `route2_select_chain_value_record`
- `route2_build_select_chain_value_index`
- `route2_find_select_chain_value_record`

Moved the route2-owned anonymous helper
`route2_find_direct_global_dependency` into `bir_route2.cpp`'s anonymous
namespace. No `bir_private.hpp` boundary was added.

Preserved public declarations, route2 records, route2 enums, direct-global
dependency shape, and route6 behavior. Route6 still consumes route2 through the
public `route2_select_chain_value_record` call path in `bir.cpp`.

Updated the direct-source `backend_lir_to_bir_notes_test` metadata to compile
`src/backend/bir/bir_route2.cpp`. The backend library path already picks up the
new BIR TU through the existing backend source glob.

## Suggested Next

Execute Step 4: record the accepted Step 3 proof, run or accept supervisor
regression guard as required, and decide whether the route2 extraction is ready
for lifecycle close review.

## Watchouts

- `src/backend/bir/bir_route2.cpp` is intentionally body-only; declarations
  remain in `bir.hpp`.
- `route2_find_direct_global_dependency` remains private to the route2 TU.
- `backend_prepare_phi_materialize_test` still links `c4c_backend`, so it did
  not need a direct `bir_route2.cpp` source entry.
- `test_after.log` contains the passing delegated backend proof and should be
  preserved until the supervisor rolls validation forward.

## Proof

Ran:

```sh
git diff --check
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

Result: build passed, backend subset passed with `345/345` tests.

Proof log: `test_after.log`.

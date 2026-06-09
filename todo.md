Status: Active
Source Idea Path: ideas/open/133_shared_prepared_fact_query_surface_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Examine Nearby Same-Feature Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding `prepare::find_prepared_value_home_for_bir_value` in `src/backend/prealloc/value_locations.hpp`. The helper resolves a named BIR value through `PreparedNameTables` and then reuses the existing indexed value-home lookup path over `PreparedValueHomeLookups`, `PreparedRegallocFunction`, and `PreparedValueLocationFunction`.

Converted `dispatch_publication.cpp::prepared_value_home_for_value` and `dispatch_lookup.cpp::make_named_prepared_result_register` to use the shared query. AArch64 register parsing, scalar view selection, stack/frame-pointer behavior, scratch/hazard policy, and `RegisterOperand` construction remain local.

## Suggested Next

Execute the next narrow shared prepared-fact extraction selected by the supervisor, likely current-block entry publication or same-block producer context wiring, without mixing those larger follow-ups into this value-home slice.

## Watchouts

- `PreparedValueHomeLookups` is declared in `prepared_lookups.hpp`, which includes `value_locations.hpp`, so the new value-location helper is templated like the existing indexed lookup helpers to keep include direction stable.
- `dispatch_lookup.cpp` still has a local `prepared_named_value_id` helper for `emitted_scalar_value_available`; that was outside this packet's selected value-home rediscovery site.
- Larger rediscovery sites in `current_block_entry_publication_*`, edge-publication, and select-chain helpers remain untouched.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R '^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed, 179/179 backend tests passed. The supervisor has since rolled that accepted proof forward to the current canonical baseline log, `test_before.log`, so this scratchpad no longer names `test_after.log` as a present artifact.

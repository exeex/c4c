Status: Active
Source Idea Path: ideas/open/133_shared_prepared_fact_query_surface_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Examine Nearby Same-Feature Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 3 follow-up implementation for the shared value-home query extraction.

Converted the directly equivalent named BIR value -> `PreparedValueHome` rediscovery sites selected by the Step 3 evidence pass:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` `current_block_entry_publication_home` now delegates the local BIR-value home lookup to `prepare::find_prepared_value_home_for_bir_value`; current-block publication filtering remains local.
- `src/backend/mir/aarch64/codegen/select_materialization.cpp` `materialize_direct_global_select_chain_call_argument` now uses the shared helper before comparing with the prepared call-argument source id; select-chain and call-argument routing remain local.
- `src/backend/mir/aarch64/codegen/comparison.cpp` now uses the shared helper for the materialized condition home, constant-RHS fused compare RHS home, and emitted-condition scratch register value-id lookup; branch fusion, scratch selection, and emitted-register policy remain local.
- `src/backend/mir/aarch64/codegen/alu.cpp` now uses the shared helper for the scalar multiply stack-slot fallback after `make_named_prepared_result_register`; register-vs-stack publication handling remains local.
- Removed the now-unused local `prepared_named_value_id` helper from `dispatch_publication.cpp`.

Explicitly deferred:

- `alu.cpp` return-chain indexed home lookups that start from return-chain `ValueNameId` results rather than a local BIR value.
- `select_materialization.cpp` and `comparison.cpp` `prepared_named_value_id` uses that feed emitted-register lookup, direct-global dependency comparison, or non-home relationships.
- Broad `memory.cpp`/`calls.cpp` cleanup and prepared-id-only lookups carried by move bundles, call plans, memory-access plans, preserved values, or edge-publication relationships.

## Suggested Next

Supervisor review for Step 3 completion and route quality. If accepted, move toward the next lifecycle action for the shared prepared fact query surface extraction rather than widening this packet into `memory.cpp`/`calls.cpp`.

## Watchouts

- `PreparedValueHomeLookups` is declared in `prepared_lookups.hpp`, which includes `value_locations.hpp`, so the new value-location helper is templated like the existing indexed lookup helpers to keep include direction stable.
- `dispatch_lookup.cpp` still has a local `prepared_named_value_id` helper for `emitted_scalar_value_available`; that is not a `PreparedValueHome` rediscovery site.
- Do not convert lookups that start from `PreparedValueId` or non-local `ValueNameId` relationships (`move.from_value_id`, `move.to_value_id`, return-chain values, call-plan result ids, memory-access pointer ids, preserved ids, etc.); those are distinct indexed-home queries.
- Larger edge-publication producer, select-chain, memory-access, and call-plan relationships remain separate source facts. Only their directly equivalent local BIR-value-to-home subqueries belong on this shared helper.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R '^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 179/179 backend tests passed.

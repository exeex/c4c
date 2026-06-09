Status: Active
Source Idea Path: ideas/open/133_shared_prepared_fact_query_surface_extraction.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Finalize Shared Query Contract

# Current Packet

## Just Finished

Completed `plan.md` Step 4 finalization for the shared prepared value-home query extraction.

Final shared query contract:

- API: `prepare::find_prepared_value_home_for_bir_value(names, value_home_lookups, regalloc, function_locations, value)`.
- Owner rationale: the query lives in the shared prepared value-location surface because it answers a target-neutral fact relationship from prepared name tables plus `PreparedValueHomeLookups`/prepared value locations: named local BIR value -> `PreparedValueHome`. AArch64 callers still decide whether and how to use that home for registers, stack slots, branches, scratch registers, publications, or materialization.
- Lookup behavior: reject null function locations, unnamed or non-named BIR values, and unknown prepared value names; otherwise use the prepared value-id index when available and fall back through the existing prepared regalloc/function-location home lookup path.

All AArch64 local reconstruction sites converted in this runbook:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp` `current_block_entry_publication_home` now delegates the local BIR-value home lookup to `prepare::find_prepared_value_home_for_bir_value`; current-block publication filtering remains local.
- `src/backend/mir/aarch64/codegen/select_materialization.cpp` `materialize_direct_global_select_chain_call_argument` now uses the shared helper before comparing with the prepared call-argument source id; select-chain and call-argument routing remain local.
- `src/backend/mir/aarch64/codegen/comparison.cpp` now uses the shared helper for the materialized condition home, constant-RHS fused compare RHS home, and emitted-condition scratch register value-id lookup; branch fusion, scratch selection, and emitted-register policy remain local.
- `src/backend/mir/aarch64/codegen/alu.cpp` now uses the shared helper for the scalar multiply stack-slot fallback after `make_named_prepared_result_register`; register-vs-stack publication handling remains local.
- Removed the now-unused local `prepared_named_value_id` helper from `dispatch_publication.cpp`.

Explicitly deferred follow-ups:

- `alu.cpp` return-chain indexed home lookups that start from return-chain `ValueNameId` results rather than a local BIR value.
- `select_materialization.cpp` and `comparison.cpp` `prepared_named_value_id` uses that feed emitted-register lookup, direct-global dependency comparison, or non-home relationships.
- Broad `memory.cpp`/`calls.cpp` cleanup and prepared-id-only lookups carried by move bundles, call plans, memory-access plans, preserved values, or edge-publication relationships.
- `dispatch_lookup.cpp` `emitted_scalar_value_available`, which still uses local `prepared_named_value_id` for emitted-register availability rather than a value-home query.
- Edge-publication producer, select-chain, memory-access, and call-plan relationship extraction remain separate fact surfaces and should not be folded into this local BIR-value-to-home helper.

Confirmed boundaries:

- No AArch64 emission, register-view selection, scratch-register choice, branch-fusion behavior, hazard policy, relocation operand, instruction spelling, stack/publication policy, or target-specific materialization decision moved into shared code.
- No predecessor rescans, BIR-name rescans beyond the shared prepared name-table lookup, local reconstruction under a new wrapper, unsupported downgrades, test expectation weakening, or named-testcase shortcut was introduced.

## Suggested Next

Supervisor review for Step 4 completion and lifecycle disposition of active idea 133. The implementation runbook is finalized; any further shared fact extraction should be a new packet or separate idea rather than widening this value-home contract.

## Watchouts

- `PreparedValueHomeLookups` is declared in `prepared_lookups.hpp`, which includes `value_locations.hpp`, so the new value-location helper is templated like the existing indexed lookup helpers to keep include direction stable.
- Do not convert lookups that start from `PreparedValueId` or non-local `ValueNameId` relationships (`move.from_value_id`, `move.to_value_id`, return-chain values, call-plan result ids, memory-access pointer ids, preserved ids, etc.); those are distinct indexed-home queries.
- Matching `test_before.log` was preserved by prior acceptance state; this packet refreshed `test_after.log` only, per the delegated owned files.

## Proof

Ran exactly `(cmake --build --preset default && ctest --test-dir build -R '^backend_' --output-on-failure) > test_after.log 2>&1`.

Result: passed. `test_after.log` reports 100% tests passed, 0 tests failed out of 179.

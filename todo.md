Status: Active
Source Idea Path: ideas/open/133_shared_prepared_fact_query_surface_extraction.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Examine Nearby Same-Feature Cases

# Current Packet

## Just Finished

Completed `plan.md` Step 3 evidence pass for the shared value-home query extraction.

Converted already:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:194` `prepared_value_home_for_value` now delegates the named BIR value -> prepared value home relationship to `prepare::find_prepared_value_home_for_bir_value`.
- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp:26` `make_named_prepared_result_register` now uses the same shared query before applying AArch64-only register-bank, scalar-view, and `RegisterOperand` policy.
- Existing dispatch-family consumers in `dispatch.cpp`, `dispatch_producers.cpp`, `dispatch_edge_copies.cpp`, `dispatch_value_materialization.cpp`, `fp_value_materialization.cpp`, `cast_ops.cpp`, `memory_store_retargeting.cpp`, and selected `memory.cpp`/`calls.cpp` sites already reach the shared lookup through `prepared_value_home_for_value` or `make_named_prepared_result_register`.

Directly equivalent future conversion candidates:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp:229` `current_block_entry_publication_home` still resolves `prepared_named_value_id(context, value)` and then calls `find_indexed_prepared_value_home(...)`. The current-block publication filter is distinct, but the underlying BIR value -> home lookup is directly equivalent and should be converted in a small implementation packet.
- `src/backend/mir/aarch64/codegen/select_materialization.cpp:286` `materialize_direct_global_select_chain_call_argument` performs the same named-value-to-home chain before comparing the home with the prepared argument plan. The select-chain relationship is distinct, but the local home query is directly equivalent.
- `src/backend/mir/aarch64/codegen/comparison.cpp:2028`, `:2222`, and `:2286` contain direct condition/RHS named-value-to-home lookups. These are equivalent query-surface candidates, with comparison-specific branch fusion and scratch policy remaining local.
- `src/backend/mir/aarch64/codegen/alu.cpp:3837` falls back from `make_named_prepared_result_register` to another named-result-to-home lookup for stack-slot handling. This is directly equivalent for the home lookup but should be reviewed carefully because the surrounding code intentionally distinguishes register vs stack publication.
- Representative memory/call sites such as `memory.cpp:3363`, `memory.cpp:3481`, `memory.cpp:5113`, and `calls.cpp:8242` are also named-BIR-value-to-home chains, but the files contain many prepared-id-only and publication-plan lookups nearby. They are better handled by a later, focused adjacent-file cleanup rather than expanding this Step 3 packet.

Distinct/deferred relationships:

- `dispatch_producers.cpp:58` only needs a prepared value id for an instruction result, not a `PreparedValueHome`.
- `dispatch_publication.cpp:217` and the edge-publication paths are current-block entry or edge-publication relationships; only their local value-home subqueries are candidates for this helper.
- `dispatch_edge_copies.cpp:206` and `dispatch_producers.cpp:205` query edge-publication source producers, not value homes.
- Direct `find_indexed_prepared_value_home(..., PreparedValueId)` uses in `calls.cpp`, `memory.cpp`, and other AArch64 files already start from prepared ids carried by move bundles, call plans, memory-access plans, or preserved values; those are not equivalent to the new BIR-value query.

## Suggested Next

Run one narrow Step 3 follow-up implementation packet that converts the directly equivalent named-BIR-value-to-home sites in `dispatch_publication.cpp`, `select_materialization.cpp`, `comparison.cpp`, and possibly the single `alu.cpp` fallback to `prepare::find_prepared_value_home_for_bir_value`, stopping before broad `memory.cpp`/`calls.cpp` cleanup. Step 3 should not move to Step 4 finalization until the supervisor either accepts that follow-up packet or explicitly defers these candidates.

## Watchouts

- `PreparedValueHomeLookups` is declared in `prepared_lookups.hpp`, which includes `value_locations.hpp`, so the new value-location helper is templated like the existing indexed lookup helpers to keep include direction stable.
- `dispatch_lookup.cpp` still has a local `prepared_named_value_id` helper for `emitted_scalar_value_available`; that was outside this packet's selected value-home rediscovery site.
- Do not convert lookups that already start from `PreparedValueId` (`move.from_value_id`, `move.to_value_id`, call-plan result ids, memory-access pointer ids, preserved ids, etc.); those are distinct indexed-home queries.
- Larger current-block entry publication, edge-publication producer, select-chain, memory-access, and call-plan relationships remain separate source facts. Convert only their directly equivalent local BIR-value-to-home subqueries if the supervisor selects the follow-up.

## Proof

Evidence-only packet. No build or tests were run, and no `test_after.log` was produced, because this packet edited only `todo.md` and did not touch implementation files.

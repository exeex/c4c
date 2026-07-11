Status: Active
Source Idea Path: ideas/open/696_bir_producer_index_view_extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map The First Producer View Surface

# Current Packet

## Just Finished

Completed `plan.md` Step 1 discovery for the first named BIR producer-view
surface.

Selected first surface:
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
`verify_prepared_semantic_name_agreement_boundary`, specifically the focused
Route 1 `%sum` proof reader that checks same-block producer identity and
materialization availability against the prepared producer oracle.

Old Route 1 entry points:
`bir::route1_build_producer_index(block)`,
`bir::Route1SameBlockProducerQuery{.index = &route1_index,
.before_instruction_index = block.insts.size()}`,
`bir::route1_find_same_block_scalar_producer(route1_query,
bir::Value::named(bir::TypeKind::I64, "%sum"))`, and
`bir::route1_find_materialization_availability(route1_query,
bir::Value::named(bir::TypeKind::I64, "%sum"))`.

Intended named adapter:
add a `BirProducerView`-style Route 1 compatibility surface in BIR, for
example `bir::make_bir_producer_view(block)` plus
`bir::find_same_block_scalar_producer(view, value, before_instruction_index)`
and `bir::find_materialization_availability(view, value,
before_instruction_index)`, with exact naming left to the implementation
packet. The adapter should return the same producer record, instruction,
produced value, instruction index, producer kind, and materialization
availability as the current Route 1 path.

Route 2 decision:
defer Route 2 for the first migration. The selected proof reader only needs
Route 1 same-block producer identity and materialization; the nearby
select-chain fail-closed checks should remain on the existing Route 2 path
until a later packet explicitly scopes select-chain/direct-global producer
evidence.

Rollback point:
restore the local `route1_index`, `route1_query`,
`route1_find_same_block_scalar_producer`, and
`route1_find_materialization_availability` calls inside
`verify_prepared_semantic_name_agreement_boundary`. No production behavior,
expectation, unsupported marker, allowlist, timeout, root proof log, or harness
contract rollback should be needed.

## Suggested Next

Implement the named Route 1-only `BirProducerView` compatibility adapter in
`src/backend/bir/bir.hpp` and `src/backend/bir/bir.cpp` or the adjacent local
BIR producer implementation file, then migrate only the selected
`verify_prepared_semantic_name_agreement_boundary` Route 1 proof reader through
that adapter. Keep Route 2 select-chain/direct-global support deferred.

## Watchouts

Keep the new surface named around BIR producer-view vocabulary, not Route 1
route vocabulary. Do not expose `Route1ProducerIndex`,
`Route1SameBlockProducerQuery`, or `Route2SelectChainValueIndex` as new public
architecture under renamed wrappers. Do not migrate production MIR/codegen
readers, Route 2 select-chain readers, memory/publication views, call/return
views, prealloc authority, target materialization, dump vocabulary,
expectations, unsupported markers, allowlists, timeouts, runtime contracts, or
root-level proof logs in the next code packet.

## Proof

Discovery proof run:
`rg -n "Route1ProducerIndex|Route1SameBlockProducerQuery|route1_find_same_block_scalar_producer|route1_find_materialization_availability|Route2SelectChainValueIndex|route2_find_select_chain_value_record" src/backend tests/backend`

No build was required for this discovery-only packet. Per the delegated proof
contract, `test_before.log` and `test_after.log` were not created or modified.

Recommended focused implementation proof:
`cmake --build --preset default --target backend_prepared_lookup_helper_test && build/tests/backend/bir/backend_prepared_lookup_helper_test`

CTest source registration exists as `backend_prepared_lookup_helper` in
`tests/backend/bir/CMakeLists.txt`; the current build tree did not list that
test via `ctest -N -R '^backend_prepared_lookup_helper$'`, so the direct
executable command above is the reliable focused proof for the next packet.

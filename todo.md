Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Step 3 of `plan.md` needed no additional code/test change because the accepted
Step 2 slice already migrated the selected AArch64 ALU control-publication
select-result consumer onto the intended Route 2-first path.

`lower_scalar_select_publication(...)` now reads
`route2_control_select_chain_materialization(...)` for `select.result` at
`instruction_index + 1U` before consulting the prepared fallback. The Route 2
adapter uses `mir::find_bir_select_chain_identity(...)` through
`mir::BirSelectChainIdentityRequest`, requires a select root, root instruction
index, and scalar materialization eligibility, and maps only root identity plus
direct-global dependency presence into the existing prepared-shaped consumer
data. If Route 2 has no valid answer, the consumer still calls
`prepare::find_prepared_scalar_select_chain_materialization(...)` unchanged.

Result-register selection, stack publication, materialization sequencing, final
record spelling, memory/call policy, storage/home decisions, and broad prepared
aggregate ownership remain outside Route 2.

## Suggested Next

Run the next supervisor-selected Step 4 packet to prove route/prepared
equivalence for the migrated selected ALU select-result consumer, including
direct-global present/absent, scalar-eligible/ineligible, nested select-chain,
and prepared fallback behavior as appropriate.

## Watchouts

The Step 3 no-code conclusion depends on the current `alu.cpp` ordering:
Route 2 is queried first, and prepared lookup is used only when
`select_chain_materialization.available` is false. Keep
`prepare::find_prepared_scalar_select_chain_materialization(...)` public and
unchanged as fallback/oracle. Do not extend Step 4 into
`dispatch_value_materialization.cpp`, `dispatch_producers.cpp`, calls,
memory/store consumers, publication planning, printer/debug surfaces, broad
prepared aggregate APIs, or expectation rewrites.

## Proof

No code or tests changed in Step 3, so the delegated packet explicitly did not
require a new `test_after.log`. The accepted Step 2 proof for the same selected
consumer and subset has been rolled forward to `test_before.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'; } > test_after.log 2>&1`

Proof log: `test_before.log`.

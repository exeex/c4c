Status: Active
Source Idea Path: ideas/open/185_phase_e_route2_select_chain_view_consumer_migration.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Route/Prepared Equivalence

# Current Packet

## Just Finished

Step 4 of `plan.md` added focused selected-consumer coverage in
`tests/backend/mir/backend_aarch64_prepared_scalar_alu_records_test.cpp`.

The new select-publication fixture lowers the migrated AArch64 ALU
control-publication `select.result` consumer through
`lower_scalar_control_value_instruction(...)` and asserts:
- direct-global present scalar select results use Route 2 chain
  materialization instead of `csel`
- direct-global absent scalar select results keep the ordinary `csel` path
- nested direct-global select chains emit nested Route 2 chain labels instead
  of falling back to a flat selected testcase shape

The existing `backend_prepared_lookup_helper` oracle coverage remains active
in the delegated proof and continues to cover prepared/Route 2 agreement for
scalar-ineligible immediate roots, fail-closed missing roots, mismatched type
requests, direct-global present/absent, and nested select-chain facts.

## Suggested Next

Run the next supervisor-selected Step 5 packet for review or lifecycle
decision. No implementation follow-up is suggested by this executor packet.

## Watchouts

The Step 4 consumer assertions intentionally inspect the selected assembler
template shape for the chosen ALU select-result consumer only. They do not
claim migration of memory/store, call, publication-planning, or broader
prepared aggregate APIs. Keep `backend_prepared_lookup_helper` in the proof
subset because it remains the prepared fallback/oracle surface for
scalar-ineligible and fail-closed cases.

## Proof

The supervisor-selected Step 4 proof passed and was written to `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records|backend_aarch64_scalar_record_contract)$'; } > test_after.log 2>&1`

Proof log: `test_after.log`.

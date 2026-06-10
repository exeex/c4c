Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Contract Only Proven-Private Surface

# Current Packet

## Just Finished

Recorded Step 4 of `plan.md` metadata after the completed Step 3
implementation and supervisor-side broader backend validation.

Completed Step 3 of `plan.md`: migrated the selected AArch64 scalar
value-publication select-chain materialization consumer in
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` from
prepared select-chain materialization discovery to Route 2-backed MIR identity
queries.

The selected consumer now uses `mir::find_bir_select_chain_identity(...)` via a
local Route 2 adapter for the semantic select-chain facts:
- select root identity and root instruction index
- scalar materialization eligibility
- direct-global dependency presence

The existing target emission path still receives the prepared-shaped direct
global dependency because `emit_select_chain_value_to_register(...)` owns that
target API. Materialization cost, scratch/hazard decisions, register
availability, publication routing, homes, and final emission were left
unchanged.

## Suggested Next

Continue Step 4: re-scan direct prepared select-chain/direct-global/scalar
materialization consumers and contract only surfaces proven private after the
selected migration.

## Watchouts

- `emit_select_chain_value_to_register(...)` still accepts
  `prepare::PreparedDirectGlobalSelectChainDependency`; this packet kept a
  local prepared-shaped adapter for that unchanged target API.
- Do not delete or hide `select_chain_lookups.hpp` helpers or
  `PreparedFunctionLookups::edge_publication_source_producers` unless Step 4
  proves every remaining public consumer has moved or no longer needs them.
- Known residual prepared consumers include call planning, ALU/select-chain
  materialization paths, prepared printer/helper tests, and other prealloc
  surfaces outside this selected value-publication consumer.

## Proof

Delegated proof passed:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Supervisor-side broader backend validation passed:
`ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: 179/179 tests passed.

Proof log: `test_after.log`.

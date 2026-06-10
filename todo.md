Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The Selected Consumer

# Current Packet

## Just Finished

Completed Step 2 of `plan.md`: confirmed and extended Route 2 oracle coverage
for the selected AArch64 scalar value-publication select-chain materialization
family before switching the consumer.

Coverage now established in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`:
- Direct-global positive: selected scalar select-chain rooted in `SelectInst`
  exposes root instruction index, root value name, root-is-select,
  direct-global `LoadGlobalInst` dependency, and scalar materialization
  eligibility through `mir::find_bir_select_chain_identity(...)` and narrower
  Route 2 helpers, matching the prepared oracle.
- Non-select-root negative/alternate: direct `LoadGlobalInst`, `LoadLocalInst`,
  and nested binary roots expose Route 2 identity without claiming select-root
  status, matching prepared answers.
- No-dependency negative: local source records remain scalar-materializable but
  do not expose a direct-global dependency.
- Scalar-ineligible fail-closed: added an immediate-root Route 2 request check
  that rejects source producer, direct-global dependency, scalar eligibility,
  identity, and indexed record lookup while matching prepared fail-closed
  behavior.
- Existing boundary checks cover before-index misses, mismatched root type,
  missing roots, and Route 2 indexed-record equivalence.

Selected target-lowering coverage already exists in
`tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` for the
value-publication paths that materialize selected global-load values before
stores, fused branches, and related select-chain consumers.

## Suggested Next

Execute Step 3: migrate only
`src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` for the
selected AArch64 scalar value-publication select-chain materialization family
from `prepare::find_prepared_scalar_select_chain_materialization(...)` to Route
2-backed `mir::BirSelectChainIdentityRequest` queries.

## Watchouts

- The implementation packet should only replace semantic select-chain identity,
  direct-global dependency, and scalar eligibility reads. Leave target
  materialization cost, scratch/hazard decisions, register availability,
  publication routing, value homes, and final move/branch emission unchanged.
- Route 2 select-chain records do not currently represent nonmaterializable
  named producer kinds; the scalar-ineligible proof is the fail-closed
  immediate/non-record request path plus existing missing/mismatched rejects.
- Do not delete or hide `select_chain_lookups.hpp` helpers or
  `PreparedFunctionLookups::edge_publication_source_producers`; multiple call,
  publication, materialization, ALU, and prealloc consumers still require them.

## Proof

Delegated proof passed:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Proof log: `test_after.log`.

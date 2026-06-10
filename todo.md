Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: validated and handed off the selected Route 2
thinning slice after the Step 4 no-contraction decision.

Selected migration completed:
- The AArch64 scalar value-publication select-chain materialization path in
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` now uses
  Route 2-backed `mir::find_bir_select_chain_identity(...)` for select root
  identity, root instruction index, scalar materialization eligibility, and
  direct-global dependency presence.
- The selected path no longer uses prepared select-chain/direct-global lookup
  fields as the primary semantic source for those facts.
- Target emission policy, homes, publication routing, scratch/hazard behavior,
  and final emission choices remained unchanged.

No prepared select-chain/direct-global/scalar-materialization API or cache
surface was contracted. The Step 4 scan found remaining production consumers:
- `find_prepared_select_chain_source_producer(...)` remains used by prepared
  publication planning and by prepared direct-global/scalar select-chain helper
  APIs.
- `find_prepared_direct_global_select_chain_dependency(...)` remains used by
  call planning, AArch64 indirect-callee lowering, and prepared scalar
  materialization helper code.
- `find_prepared_store_source_direct_global_select_chain_dependency(...)`
  remains used by prepared publication planning and AArch64 memory/store-source
  lowering.
- `find_prepared_scalar_select_chain_materialization(...)` remains used by
  AArch64 ALU lowering and prepared printer select-chain output.
- `PreparedFunctionLookups::edge_publication_source_producers` still has broad
  AArch64 producer, edge-copy, memory, FP materialization, comparison, ALU, and
  call codegen consumers.
- `prepare::PreparedDirectGlobalSelectChainDependency` still crosses public
  call-plan, select-chain materialization, AArch64 fixture, and local
  compatibility-adapter surfaces.

The active runbook is ready for supervisor acceptance and plan-owner
close/deactivation review.

## Suggested Next

Supervisor should review acceptance and delegate plan-owner close/deactivation
review for `ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md`.

## Watchouts

- No contraction is safe in this packet: the remaining consumers are production
  paths, not only tests or private implementation details.
- Keep `select_chain_lookups.hpp`,
  `PreparedFunctionLookups::edge_publication_source_producers`,
  `PreparedDirectGlobalSelectChainDependency`, and the prepared scalar
  select-chain materialization struct public until later Route 2 packets migrate
  call planning, store/publication planning, AArch64 ALU/calls/memory/FP
  consumers, and prepared printer expectations.
- The selected value-publication consumer still needs a prepared-shaped local
  adapter because `emit_select_chain_value_to_register(...)` accepts
  `prepare::PreparedDirectGlobalSelectChainDependency`; that is residual target
  API compatibility, not evidence that the semantic lookup migration failed.

## Proof

Fresh validation run:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Result: passed. The build was up to date, and both selected tests passed:
`backend_prepared_lookup_helper` and `backend_aarch64_instruction_dispatch`.
Proof log path: `test_after.log`.

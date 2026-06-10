Status: Active
Source Idea Path: ideas/open/168_route2_select_chain_direct_global_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Or Confirm Route 2 Oracle Coverage

# Current Packet

## Just Finished

Completed Step 1 of `plan.md`: inventoried Route 2 select-chain/direct-global
consumer candidates and selected the first bounded migration family.

Selected consumer family:
- AArch64 scalar value-publication select-chain materialization in
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`.
- Prepared surface currently used:
  `prepare::find_prepared_scalar_select_chain_materialization(...)` via the
  local `prepared_scalar_select_chain_materialization(...)` adapter.
- Route 2-backed replacement surface:
  `mir::find_bir_select_chain_identity(...)` or the narrower
  `mir::find_bir_select_chain_direct_global_dependency(...)` plus
  `mir::find_bir_select_chain_scalar_materialization_eligibility(...)`, using
  `mir::BirSelectChainIdentityRequest`.
- Target files for the first implementation packet:
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`; oracle
  coverage should stay in `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
  and selected dispatch behavior should stay in
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`.

Selected because the consumer is a compact local prepared helper feeding the
existing value-publication `emit_select_chain_value_to_register(...)` path; it
can switch semantic select-chain facts to Route 2 without moving target
materialization cost, scratch/hazard policy, publication routing, homes, or
final emission choices into BIR.

Rejected candidates:
- `src/backend/mir/aarch64/codegen/calls.cpp` direct-global call-argument path:
  already has BIR direct-global coverage for the selected call-argument case,
  while the surrounding file still owns ABI/layout, indirect callee, aggregate
  transport, publication routing, and source-producer materialization policy.
- `src/backend/mir/aarch64/codegen/calls.cpp` indirect callee select-chain
  helpers: target-policy-bound and coupled to callee scratch/register choice.
- `src/backend/prealloc/call_plans.cpp` call-plan direct-global dependency
  production: aggregate call-plan construction remains an oracle/cache producer,
  not the first consumer to migrate.
- `src/backend/prealloc/publication_plans.cpp` pending store-source
  direct-global/source-producer planning: publication/store routing still
  mixes Route 2 facts with Route 4/5 publication policy and memory-source
  decisions.
- `src/backend/mir/aarch64/codegen/alu.cpp` scalar ALU select-chain
  direct-global materialization: viable later, but tied to operand-specific ALU
  emission and scratch selection; choose the more isolated value-publication
  helper first.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` direct-global
  predicate: already calls `mir::find_bir_select_chain_direct_global_dependency(...)`
  and is therefore not a prepared Route 2 consumer to switch.

Required cases before and during implementation:
- Direct-global positive: a selected scalar select-chain rooted in a select
  that reaches a direct `LoadGlobalInst` must expose root instruction index,
  root value name, root-is-select, scalar materialization eligibility, and
  direct-global dependency from Route 2.
- Non-select-root negative: a scalar value whose Route 2 root producer is not a
  select must not enter the select-chain materialization path.
- No-dependency negative: a select-chain with no direct global load dependency
  must remain eligible only for normal select materialization and must not pass
  a direct-global dependency pointer to target emission.
- Scalar-ineligible negative: a Route 2 record that is not scalar materializable
  must fail closed and fall back or reject exactly as the prepared helper did.

## Suggested Next

Execute Step 2: add or confirm Route 2 oracle coverage for the selected
AArch64 scalar value-publication select-chain materialization family, including
the direct-global positive case and the non-select-root, no-dependency, and
scalar-ineligible rejects.

## Watchouts

- Keep Step 2 oracle-first: do not switch the implementation until prepared and
  Route 2 answers are proven equivalent for the selected consumer's facts.
- The implementation packet should only replace semantic select-chain identity,
  direct-global dependency, and scalar eligibility reads. Leave target
  materialization cost, scratch/hazard decisions, register availability,
  publication routing, value homes, and final move/branch emission unchanged.
- Do not delete or hide `select_chain_lookups.hpp` helpers or
  `PreparedFunctionLookups::edge_publication_source_producers`; multiple call,
  publication, materialization, ALU, and prealloc consumers still require them.

## Proof

Not run; inventory-only packet per delegated proof.

Recommended future proof command:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

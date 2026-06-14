Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Direct-Global Select-Chain Helper Surface

# Current Packet

## Just Finished

Step 1 located the direct-global select-chain dependency surface.

Selected helper surface:
- `find_prepared_select_chain_source_producer(...)` is the producer agreement
  gate. It resolves the queried BIR value through prepared names and delegates
  same-block producer acceptance to `prepared_bir_value_name_agreement(...)`.
- `prepared_select_chain_contains_direct_global_load(...)` is the recursive
  chain scan. It re-enters the same producer gate for cast, binary, and select
  operands and returns `nullopt` for incomplete producer payloads.
- `find_prepared_direct_global_select_chain_dependency(...)` is the shared
  dependency classifier. It requires a root producer plus a positive recursive
  direct-global scan, then publishes `contains_direct_global_load`,
  `root_is_select`, and `root_instruction_index`.
- `find_prepared_store_source_direct_global_select_chain_dependency(...)` is a
  store-source alias of the shared dependency classifier.

Direct consumer boundary:
- `plan_call_argument_direct_global_select_chain_dependency(...)` in
  `call_plans.cpp` publishes call-argument dependency metadata only after the
  shared classifier reports an available dependency and the source value name
  is valid.
- `populate_store_source_publication_plans(...)` in `publication_plans.cpp`
  calls the store-source dependency alias for store-local/store-global source
  values and copies the three dependency fields through
  `plan_prepared_store_source_publication(...)`.
- Existing helper-test coverage for shared producer agreement and select-chain
  queries lives in `backend_prepared_lookup_helper_test.cpp`, especially the
  same-block scalar/select-chain agreement rows and the direct-global
  select-chain query rows.

## Suggested Next

Execute Step 2 by adding the prepared agreement boundary inside
`select_chain_lookups.cpp`, with `select_chain_lookups.hpp` touched only if a
small test-visible seam is unavoidable, and focused proof rows in
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`.

## Watchouts

- Keep this packet limited to the direct-global select-chain dependency
  candidate.
- Do not absorb the source-value/source-producer metadata candidate.
- Do not reactivate completed module, names, control-flow, recovered-source,
  or byval pointer-source packets.
- Preserve prepared source-producer authority, block identity checks, public
  prepared aggregate compatibility, and current fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, baselines,
  or target output to claim progress.
- Step 2 should not move broad publication policy, storage encoding, pending
  store-global policy, duplicate handling, pointer-base homes, target lowering,
  call argument lowering, or prepared aggregate compatibility into BIR.
- The smallest implementation boundary appears to be local agreement in the
  direct-global dependency path before the helper publishes root facts; do not
  widen into the separate source-value/source-producer metadata packet.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.
Result: passed, `1/1` focused test passed.
Log: `test_after.log`.

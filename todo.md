Status: Active
Source Idea Path: ideas/open/108_prepared_select_chain_dump_contract_coverage.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Lookup Facts And Existing Dump Coverage

# Current Packet

## Just Finished

Step 1 inventory completed for prepared select-chain lookup facts and existing
prepared-printer dump coverage.

Scalar select-chain materialization facts:
- `find_prepared_scalar_select_chain_materialization` publishes
  `PreparedScalarSelectChainMaterialization`.
- Fields are `available`, `root_value_name`, `root_is_select`,
  `root_instruction_index`, and nested `direct_global_dependency`.
- The nested `PreparedDirectGlobalSelectChainDependency` fields are
  `contains_direct_global_load`, `root_is_select`, and
  `root_instruction_index`.
- The root producer comes from
  `PreparedEdgePublicationSourceProducerLookups::producers_by_value_name` via
  `prepared_select_chain_source_producer`; eligible producer kinds are
  `load_local`, `load_global`, `cast`, `binary`, and
  `select_materialization`.
- Consumers found:
  `src/backend/mir/aarch64/codegen/alu.cpp` and
  `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` use the
  materialization fields to decide selected-value materialization and direct
  global select-chain handling.

Direct-global select-chain dependency facts:
- Generic lookup:
  `find_prepared_direct_global_select_chain_dependency` returns
  `contains_direct_global_load`, `root_is_select`, and
  `root_instruction_index`.
- Call-argument wrapper:
  `PreparedCallArgumentDirectGlobalSelectChainDependency` adds `available` and
  `source_value_name`; it is stored on `PreparedCallArgumentPlan` as
  `direct_global_select_chain_dependency` and consumed through
  `find_prepared_call_argument_direct_global_select_chain_dependency`.
- Store-source wrapper:
  `PreparedStoreSourcePublicationPlan` carries
  `direct_global_select_chain_source`,
  `direct_global_select_chain_root_is_select`, and
  `direct_global_select_chain_root_instruction_index`.
- Consumers found:
  `src/backend/mir/aarch64/codegen/select_materialization.cpp` consumes the
  call-argument wrapper; `src/backend/mir/aarch64/codegen/calls.cpp` calls the
  generic lookup for indirect callee stored-value/direct-global chains; and
  `src/backend/mir/aarch64/codegen/memory.cpp` consumes the store-source
  fields for store-local/global source publication and direct-global select
  chain handling.

Supporting source-producer facts:
- Source producer records are `PreparedEdgePublicationSourceProducer` with
  `kind`, `block_label`, `instruction_index`, and typed instruction pointers
  for `load_local`, `load_global`, `cast`, `binary`, or `select`.
- Edge publication and store-source plans copy producer provenance as
  `source_producer_kind`, `source_producer_block_label`, and
  `source_producer_instruction_index`, plus typed pointers such as
  `source_load_global`, `source_cast`, `source_binary`, and `source_select`.
- These fields directly support the select-chain/direct-global contract because
  missing producer kind/block/index makes lookup consumers fail closed or avoid
  select-chain materialization.

Current prepared-printer coverage:
- `prepared_printer/calls.cpp` prints call plan wrapper, indirect callee
  storage, argument source encoding, source value/base IDs, symbol/computed
  address facts, destination facts, source selection, and aggregate transport.
- It does not print `PreparedCallArgumentDirectGlobalSelectChainDependency`
  fields: no availability flag, source value, `contains_direct_global_load`,
  `root_is_select`, or root instruction index.
- It also does not print generic indirect-callee direct-global dependency facts
  because those are currently recomputed by the AArch64 consumer and are not
  carried in `PreparedCallPlan`.
- `prepared_printer/storage.cpp` prints only storage-plan value homes; it does
  not expose scalar select-chain materialization facts.
- No prepared-printer section currently prints `PreparedStoreSourcePublicationPlan`
  or its source-producer/direct-global fields.
- Existing `backend_prepared_printer_test.cpp` coverage near
  `prepare_call_argument_source_shape_dump_module` proves symbol-address and
  computed-address call argument source shapes, but not select-chain
  dependency/root/provenance dump visibility.

Candidate focused tests:
- Extend or add a `backend_prepared_printer_test.cpp` fixture that builds a
  same-block select chain rooted in a direct global load and passed as a call
  argument; assert dump text for call argument dependency source value,
  `contains_direct_global_load=yes`, `root_is_select=yes/no`, and root
  instruction index.
- Add scalar select-chain materialization dump coverage with a selected scalar
  value whose producer lookup is `select_materialization`, proving root value,
  root select status, and root instruction index are visible in the prepared
  dump.
- Add store-source/direct-global dependency coverage only if Step 2 chooses a
  dump surface for store-source publication plans; fixture should assert
  source-producer kind/block/index and direct-global root fields without
  broad unrelated publication output.
- Existing source-shape fixture can be reused for call argument source
  identity, but it likely needs an added select/global chain to exercise the
  new contract.

## Suggested Next

Execute `plan.md` Step 2: choose the prepared-printer dump contract and stable
labels for scalar select-chain materialization, call-argument/direct-global
dependency fields, and any bounded store-source/source-producer provenance
fields.

## Watchouts

- Indirect-callee direct-global dependency is currently not stored in
  `PreparedCallPlan`; exposing it may require either a narrow carried field or
  an explicit decision that only call-argument/store-source carried facts are
  printable in this route.
- Store-source facts are planner structs consumed in MIR memory lowering, not
  currently part of a prepared-printer section. Step 2 needs to decide whether
  to add a narrow dump section or keep store-source as test-only consumer proof.
- Do not satisfy this idea by broad dumping every edge/source publication fact;
  source-producer kind/block/index should appear only when tied to
  select-chain/direct-global visibility.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/prealloc tests/backend && printf 'analysis-only proof: no implementation or backend test diff for prepared select-chain dump inventory\n' > test_after.log`

Proof log: `test_after.log`.

Status: Active
Source Idea Path: ideas/open/135_shared_current_block_entry_publication_query.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Locate the Rediscovery Site

# Current Packet

## Just Finished

Completed Step 1 evidence pass. Selected
`src/backend/mir/aarch64/codegen/dispatch_publication.cpp:
current_block_entry_publication_register` as the concrete rediscovery site:
it resolves the prepared value home for a BIR named value, recollects current
block entry publications, locally filters by prepared value id and publication
availability, then parses the destination register name before returning the
AArch64 `RegisterOperand`.

Proposed shared owner/query shape:
`src/backend/prealloc/prepared_lookups.{hpp,cpp}` should own a narrow
target-neutral query over prepared current-block publication facts, reusing
`PreparedValueHomeLookups` and existing block-entry publication collection.
The query should accept prepared names/value-home lookups/regalloc/value
locations plus successor block label and BIR value or prepared value id, then
return the matching available `PreparedBlockEntryPublication` and
`PreparedValueHome` identity without parsing register names or choosing a
target register view. A plausible shape is a
`PreparedCurrentBlockEntryPublicationQueryInputs` plus
`find_prepared_current_block_entry_publication_for_value(...)` result carrying
status, publication pointer/value, destination home, destination value id/name,
and destination register spelling when present.

Relevant call paths found:
- `dispatch_value_materialization.cpp: emit_value_publication_to_register`
  calls `current_block_entry_publication_register` before same-block producer
  fallback and prepared value-home publication fallback.
- `comparison.cpp` branch-fusion paths call the same hook through
  `DispatchBranchFusionHooks::current_block_entry_publication_register`.
- `alu.cpp` checks `current_block_entry_publication_register` for prepared
  scalar publication reuse.
- `dispatch.cpp` calls
  `record_current_block_entry_publication_registers` at block lowering entry,
  and the same publication facts feed `block_entry_move_clobbers_current_join_publication`
  from `dispatch_producers.cpp`.

## Suggested Next

Step 2 should add the shared prepared/prealloc query in
`src/backend/prealloc/prepared_lookups.{hpp,cpp}` and focused shared coverage
for available, missing value-home identity, unsupported destination storage,
and missing register-name/current publication cases. Then Step 3 can replace
the local value-home rediscovery plus publication scan in
`current_block_entry_publication_register` with the shared query while keeping
the AArch64 register conversion local.

## Watchouts

- AArch64-only behavior that must stay local: parse AArch64 register spellings
  with `abi::parse_aarch64_register_name`, require/choose the GP register bank,
  coerce the destination register to the caller's expected view, construct
  `RegisterOperand`, record emitted scalar registers, preserve branch-fusion
  hooks, and preserve final instruction/register spelling.
- Existing shared facts: `collect_prepared_block_entry_publications` in
  `src/backend/prealloc/value_locations.hpp` already identifies block-entry
  publications and availability, while `PreparedValueHomeLookups` in
  `prepared_lookups.hpp` owns indexed value id/home lookup. Do not add a new
  AArch64 wrapper that just hides the same local scan.
- Deferred broader/different candidates:
  `value_has_current_block_entry_publication` is the same relationship but
  only asks a boolean by home; it can become a follow-on consumer after the
  selected query exists. `record_current_block_entry_publication_registers` and
  `block_entry_move_clobbers_current_join_publication` also loop over current
  publications but include local register recording/alias policy. The
  `prepare_current_block_join_parallel_copy_source_facts` route in
  `prepared_lookups.cpp` is a broader source/incoming-expression query and
  should not be conflated with the narrower destination publication lookup.
- Nearby proof recommendation for the implementation slice:
  `cmake --build --preset default`, then the focused backend MIR tests covering
  `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` and shared
  prealloc block-entry publication tests, followed by the delegated broader
  backend subset when acceptance proof is requested.

## Proof

Evidence-only locator packet. No implementation files, headers, expectations,
or proof logs were edited, so no build/tests were run and no `test_after.log`
was produced.

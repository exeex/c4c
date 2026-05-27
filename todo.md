# Current Packet

Status: Active
Source Idea Path: ideas/open/47_aarch64_dispatch_edge_copies_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Replace source-producer fallback recovery

## Just Finished

Step 2 replaced the `emit_edge_value_publication_to_register_impl` and
`edge_value_publication_may_read_register_index` fallback producer route with
`PreparedFunctionLookups::edge_publication_source_producers` via
`find_indexed_prepared_edge_publication_source_producer`. Child operands keep
the prepared publication context for recursive materialization/hazard checks,
and the two repaired call sites no longer call `find_edge_named_producer`.

The AArch64 dispatch test now covers a nested child operand with an explicit
prepared source-producer fact and a successor decoy that would be selected by a
local scan. Both emission and register-read hazard checks ignore the decoy and
consume the prepared source-producer fact.

## Suggested Next

Implement Step 3 by routing `emit_edge_load_local_to_register_impl` through
prepared source-memory facts and value-home authority, without extending raw
BIR memory/address rediscovery.

## Watchouts

`find_edge_named_producer` and `unique_branch_predecessor_context` remain
defined/exported, but the repaired source-producer materialization and hazard
paths no longer call them. Source-memory repair is still separate; pointer
source loads can still reach older memory-access recovery until Step 3.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.

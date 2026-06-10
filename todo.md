# Current Packet

Status: Active
Source Idea Path: ideas/open/156_bir_cfg_edge_publication_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch one edge-source consumer at a time

## Just Finished

Step 5 switched the narrow current-block join prepared-query routing consumer
in `build_current_block_join_prepared_query_routing` to use
`mir::find_bir_current_block_join_source_identity` for incoming-expression and
source booleans when BIR identity is available, while retaining the prepared
current-block join-source facts as the availability guard and fallback when BIR
identity is absent or incomplete.

Completed work item:

- Added instruction-result matching against BIR `SameBlockValueIdentity`
  vectors for the current-block join routing helper only.
- Left prepared fact construction, edge-copy source facts, move execution,
  register/home/storage decisions, stack-source routing, immediate
  materialization, and parallel-copy scheduling unchanged.
- Added focused AArch64 dispatch coverage for BIR-available routing and
  prepared fallback when the BIR join identity is absent.

## Suggested Next

Proposed Step 6 packet: run the acceptance proof for the completed semantic
edge-source route. Include the prepared lookup and AArch64 dispatch coverage
that exercises normal edge, memory-source, no-source, join-source, and the
single switched current-block join routing consumer.

## Watchouts

- This packet intentionally switched exactly one consumer:
  `build_current_block_join_prepared_query_routing`.
- The helper still requires prepared current-block join-source facts to be
  available before publishing routing booleans; BIR identity only supplies the
  semantic value-membership read when complete.
- Do not use `BirCurrentBlockJoinSourceIdentity` as authority for register
  names, homes, stack-source routing, immediate move details, move bundles, or
  prepared move records.

## Proof

Ran the delegated proof:

`cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R 'backend_(prepared_lookup_helper|aarch64_instruction_dispatch)' > test_after.log 2>&1`

Result: passed. `test_after.log` contains the passing
`backend_aarch64_instruction_dispatch` and `backend_prepared_lookup_helper`
CTest runs.

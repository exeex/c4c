Status: Active
Source Idea Path: ideas/open/69_aarch64_call_publication_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Consume Prepared Edge Publication Sources

# Current Packet

## Just Finished

Completed Step 3's `value_publication_may_read_register_index` migration in
`dispatch_publication.cpp`: the recursive register-read query now consumes the
existing prepared same-block scalar producer helper instead of falling back to
`mir::find_same_block_named_producer_record`. The local wrapper preserves the
cached `prepared_lookups` path and the existing absent-lookup fallback that
builds source-producer lookups from prepared control flow.

## Suggested Next

Next narrow Step 3 packet: inspect the remaining `dispatch_publication.cpp`
same-block/publication source-selection helpers for raw BIR producer discovery
and migrate another one only where an existing prepared helper preserves the
same source-selection contract.

## Watchouts

- `value_publication_may_read_register_index` still falls back to the prepared
  value home when no prepared same-block producer exists, matching the prior
  no-producer behavior.
- This slice did not edit `src/backend/prealloc/**`; the existing
  `find_prepared_same_block_scalar_producer` helper was sufficient.

## Proof

Command: `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(prealloc_block_entry_publications|riscv_prepared_edge_publication|publication_plan_record|x86_publication_plan_reuse|codegen_route_aarch64_store_global_stack_publication|cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication)$'; } > test_after.log 2>&1`

Result: passed; `test_after.log` contains the proof output.

Status: Active
Source Idea Path: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 refreshed the current first bad fact for the AArch64 local/formal
frame-slot publication source. The focused subset is green, including
`c_testsuite_aarch64_backend_src_00176_c`, so no current `00176.c` or focused
local/formal publication first bad fact remains under this proof.

## Suggested Next

Supervisor lifecycle routing. The active source does not currently own a live
local/formal frame-slot publication failure under the refreshed representative
subset.

## Watchouts

- The source idea is parked and says to resume only if fresh evidence again
  shows scalar fixed formal-to-local frame-slot publication as the first bad
  fact.
- The refreshed evidence did not reproduce stale local-slot reads before formal
  publication; avoid implementation from historical `partition` evidence.
- Do not reopen block-label ordering, recursive call preservation, indexed
  aggregate writeback, or selected-address work inside this plan without
  lifecycle routing.

## Proof

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00176_c)$'
```

Result: passed, 5/5 selected tests green.

Proof log: `test_after.log`.

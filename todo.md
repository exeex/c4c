Status: Active
Source Idea Path: ideas/open/353_aarch64_local_formal_frame_slot_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh Current First Bad Fact: refreshed the current `00176`
local/formal frame-slot publication proof and checked the nearby guardrail
subset. The focused proof passed 5/5 with 0 failures, including
`c_testsuite_aarch64_backend_src_00176_c`, so this refresh does not expose
scalar fixed formal-to-local frame-slot publication as a current first bad
fact.

## Suggested Next

Return to lifecycle routing for close, park, or deactivation decision using the
current green focused proof.

## Watchouts

- The source idea is parked and says to resume only if fresh evidence again
  shows scalar fixed formal-to-local frame-slot publication as the first bad
  fact.
- No live in-scope idea 353 first bad fact was observed in this refresh.
- Do not implement from stale `partition` evidence before refreshing current
  generated-code behavior.
- Do not reopen block-label ordering, recursive call preservation, indexed
  aggregate writeback, or selected-address work inside this plan without
  lifecycle routing.

## Proof

Command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_call_boundary_owner|backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00176_c)$' > test_after.log 2>&1
```

Result: build was up to date; focused subset passed 5/5 with 0 failures.
`00176` passed, and no guardrail in this subset exposed scalar fixed
formal-to-local frame-slot publication as a current first bad fact.

Proof log: `test_after.log`

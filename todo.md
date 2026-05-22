Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Current First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh Current First Bad Fact: rebuilt the current tree and ran the
supervisor-selected focused representative plus nearby backend guardrails.
The delegated proof passed 7/7, including
`c_testsuite_aarch64_backend_src_00181_c`; no live in-scope Hanoi
starting-state value-flow bad fact was exposed by this packet.

## Suggested Next

Supervisor lifecycle routing: treat the current Step 1 evidence as
already-green / no in-scope starting-state owner exposed, and decide whether
to close, deactivate, or park the active runbook.

## Watchouts

- The source idea was previously parked after scoped progress; do not assume
  the historical `A: 1 2 0 4` starting-state mismatch still exists.
- The focused representative `00181` and nearby selected guardrails all pass,
  so any next work should avoid widening this plan without fresh failing
  evidence.
- If current evidence points to materialized pointer `StoreLocal` writeback,
  pointer-derived load/address scaling, recursive post-call pointer-formal
  handling, or another owner, record the classification instead of widening
  this plan.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.

## Proof

Command:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_prepared_memory_operand_records|backend_prepare_authoritative_join_ownership|backend_prepare_stack_layout|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$'; } > test_after.log 2>&1
```

Result: passed. Build reported `ninja: no work to do`; CTest passed 7/7:
`backend_aarch64_instruction_dispatch`,
`backend_aarch64_prepared_memory_operand_records`,
`backend_prepare_authoritative_join_ownership`,
`backend_prepare_stack_layout`, `c_testsuite_aarch64_backend_src_00170_c`,
`c_testsuite_aarch64_backend_src_00181_c`, and
`c_testsuite_aarch64_backend_src_00189_c`.

Classification: already-green / no live in-scope Hanoi starting-state
value-flow bad fact exposed by the delegated proof.

Log path: `test_after.log`.

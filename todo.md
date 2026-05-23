Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh the Current `00181` First Bad Fact

# Current Packet

## Just Finished

Lifecycle activation created this refresh runbook from
`ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`.

## Suggested Next

Execute Step 1: rebuild, run the supervisor-selected focused `00181` and
starting-state guardrail proof, then record whether any current first bad fact
still belongs to idea 360.

Suggested proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_aarch64_prepared_memory_operand_records|backend_aarch64_call_boundary_owner|backend_prepare_frame_stack_call_contract|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$'
```

## Watchouts

- Treat expectation, timeout, runner, unsupported-classification, CTest, and
  proof-log-policy changes as out of scope.
- Do not claim progress through a filename-specific or literal-value shortcut.
- Do not reopen materialized pointer store writeback or pointer-derived load
  scaling unless fresh generated-code evidence makes either the current first
  bad fact.
- Preserve canonical proof-log names if producing executor proof logs:
  `test_before.log` and `test_after.log`.

## Proof

Not run during lifecycle activation.

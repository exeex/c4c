Status: Active
Source Idea Path: ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh the Current `00181` First Bad Fact

# Current Packet

## Just Finished

Step 1 - Refresh the Current `00181` First Bad Fact: rebuilt the default
preset and refreshed the supervisor-selected `00181` starting-state proof.
The focused subset passed 7/7 with 0 failures, including
`c_testsuite_aarch64_backend_src_00181_c`, so no live in-scope Hanoi
starting-state mismatch owned by idea 360 remains in the current tree.

## Suggested Next

Ask the plan owner for the next lifecycle decision for idea 360, using this
green proof and the required close-gate policy.

## Watchouts

- Do not reactivate idea 362 as the immediate next route unless fresh evidence
  requires it.
- Treat expectation, timeout, runner, unsupported-classification, CTest, and
  proof-log-policy changes as out of scope.
- Do not claim progress through a filename-specific or literal-value shortcut.
- Preserve canonical proof-log names if producing executor proof logs:
  `test_before.log` and `test_after.log`.
- This proof did not expose a residual first bad fact; if closure is attempted,
  the supervisor/plan-owner still own the regression-guard close gate.

## Proof

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_value_named_scalar_writeback_uses_computed_store_value|backend_aarch64_memory_operand_records|backend_aarch64_prepared_memory_operand_records|backend_aarch64_memory_operand_contract|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00189_c)$' > test_after.log 2>&1
```

Result: build was up to date; focused subset passed 7/7 with 0 failures.
`c_testsuite_aarch64_backend_src_00181_c` passed, so the old starting-state
mismatch (`A: 1 2 0 4` instead of `A: 1 2 3 4`) is not a live in-scope first
bad fact in this proof. Proof log: `test_after.log`.

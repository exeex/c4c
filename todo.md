Status: Active
Source Idea Path: ideas/open/300_rv64_direct_scalar_call_neighbor_coverage.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Run acceptance validation and document boundaries

# Current Packet

## Just Finished

Step 4 of `plan.md` ran the delegated acceptance command for the broadened
rv64 direct scalar call surface. The rv64 runtime subset passed 23/23 tests,
including all three neighboring direct scalar local-argument cases:
`backend_rv64_runtime_two_arg_local_arg`,
`backend_rv64_runtime_two_arg_both_local_arg`, and
`backend_rv64_runtime_two_arg_second_local_arg`.

Step 4 is complete and accepted with supervisor-established baseline evidence
for the focused RISC-V subset boundary. The second delegated RISC-V focused
subset preserved one baseline failure: `backend_riscv_prepared_edge_publication`,
with output `RISC-V prepared module should emit a register edge move`.
Supervisor validated that baseline by running the same Step 4 command at
pre-Step-3 commit `a90e3ff9` in
`/tmp/c4c-baseline-a90e3ff9/test_before_combined.log`, where rv64 runtime
passed 21/21 and the same focused test failed. Regression guard with
`--allow-non-decreasing-passed` reported PASS for the focused subset comparison
with no new failures.

## Suggested Next

Supervisor should call plan-owner to close or decide lifecycle state because
the runbook appears complete.

## Watchouts

- Final in-scope surface proven by the rv64 runtime subset: direct scalar
  register calls with rematerializable immediates and prepared scalar local
  homes, covering first-local, both-local, and second-local argument shapes.
- Final out-of-scope boundaries remain pointer parameters, member indexing,
  indirect calls, varargs, stack-passed arguments, aggregate/byval/sret ABI,
  globals, and function pointer tables.
- `backend_riscv_prepared_edge_publication` is a preserved baseline failure for
  this acceptance slice, not a new regression from the Step 3 adjacent-case
  registration work.

## Proof

Did not rerun tests in this documentation-only packet. Supervisor evidence:
the Step 4 proof command ran in root `test_after.log`:

`{ cmake --build --preset default; ctest --test-dir build -j --output-on-failure -R '^backend_rv64_runtime'; ctest --test-dir build -j --output-on-failure -R 'backend_riscv|backend_codegen_route_riscv64'; } > test_after.log 2>&1`

Result: accepted with a preserved baseline failure. Build completed, and
`^backend_rv64_runtime` passed 23 tests with 0 failures, including
`backend_rv64_runtime_two_arg_local_arg`,
`backend_rv64_runtime_two_arg_both_local_arg`, and
`backend_rv64_runtime_two_arg_second_local_arg`. The focused
`backend_riscv|backend_codegen_route_riscv64` subset preserved the baseline
failure `backend_riscv_prepared_edge_publication`; the supervisor baseline run
at commit `a90e3ff9` in
`/tmp/c4c-baseline-a90e3ff9/test_before_combined.log` showed the same focused
failure before the Step 3 change, and regression guard reported PASS with no
new failures.

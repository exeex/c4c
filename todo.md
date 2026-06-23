Status: Active
Source Idea Path: ideas/open/311_rv64_ordinary_control_expression_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair The First Semantic Emission Boundary

# Current Packet

## Just Finished

Step 2: Add Focused Control/Expression Coverage is implemented as a test-only
packet. Added focused RV64 prepared-BIR dump coverage for semantic fused-compare
conditional branches without c-testsuite filenames:

- `backend_dump_riscv64_prepared_fused_compare_loop_predicate` uses
  `tests/backend/case/riscv64_prepared_fused_compare_loop_predicate.c` and
  asserts a local-loaded loop predicate flows through `bir.ne`, `bir.cond_br`,
  and prepared `branch_condition ... kind=fused_compare ... true=block_2
  false=block_3`, with frame-slot addressing for the loaded predicate source.
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate` uses
  `tests/backend/case/riscv64_prepared_fused_compare_call_result_predicate.c`
  and asserts a same-module call result flows through `bir.ne`, `bir.cond_br`,
  the prepared fused-compare branch condition, and the prepared call-result
  publication plan.

The new tests are semantic prepared-control-flow coverage. They intentionally
do not claim RV64 final assembly/runtime success before the implementation
lowers the prepared fused-compare `cond_branch` tail.

## Suggested Next

Execute the next implementation packet for Step 3: lower RV64 prepared
`cond_branch` terminators with `branch_condition kind=fused_compare` after
operand publication, covering both local-loaded loop predicates and call-result
predicates through the same prepared-control-flow contract.

## Watchouts

- Do not implement filename-shaped logic or c-testsuite-specific shortcuts.
- Do not downgrade supported-path expectations or mark cases unsupported.
- Keep stack/local address materialization and external-stub policy separate
  unless they directly block ordinary control/expression completion proof.
- The current first boundary is not a generic append-epilogue problem. The
  emitted functions stop before the prepared fused compare/conditional branch;
  appending a `ret` would hide the missing control transfer instead of repairing
  it.
- `backend_riscv_prepared_edge_publication` remains the existing same-scope
  backend failure. The corrected regression guard shows this packet added only
  the two focused passing tests and introduced no new failures.

## Proof

Delegated proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log`

Supervisor regenerated matching before/after backend logs with the dirty-worktree
stash flow and ran:

`python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log`

Result: PASS. Same-scope regression guard reported before
`passed=213 failed=1 total=214`, after `passed=215 failed=1 total=216`, delta
`passed=2 failed=0`, and no new failing tests. The remaining failure is the
pre-existing `backend_riscv_prepared_edge_publication`; both new focused tests
pass in the after log.

Additional direct check for the new coverage:

`ctest --test-dir build -j --output-on-failure -R 'backend_dump_riscv64_prepared_fused_compare'`

Result: `2/2` tests passed.

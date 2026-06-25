Status: Active
Source Idea Path: ideas/open/361_rv64_vararg_abi_hook_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Milestone Validation and Handoff

# Current Packet

## Just Finished

Step 5 completed the milestone validation and handoff packet for the RV64
vararg ABI hook materialization runbook. The build was up to date, the four
focused backend CTest targets passed, and the representative RV64 allowlist ran
for `src/20030914-2.c` and `src/920908-1.c`.

Both representative cases still fail, but both now fail at the broader RV64
object-route admission diagnostic:
`unsupported_function_admission: variadic functions are not supported by the
RV64 object route; RV64 object variadic function lowering is not implemented`.
The inspected logs did not show the original target-only missing facts for
variadic entry state, `va_list` layout, `va_start` helper resources/homes, or
aggregate `va_arg` helper resources/homes.

Residual focused-test missing facts for scalar `va_arg` and `va_copy` remain
outside this milestone's original source-idea acceptance blockers. They should
be routed as a separate idea or later focused plan rather than expanding this
completed target-hook milestone.

## Suggested Next

Ask the plan owner to decide lifecycle disposition for this runbook: close or
retire this plan as complete against its source-idea acceptance criteria, and
open a separate scalar `va_arg`/`va_copy` initiative only if the supervisor
wants to pursue those remaining focused-test gaps now.

## Watchouts

Do not treat the remaining representative-case failures as target-hook
regressions: they are now classified at the broader RV64 object variadic
function lowering gate. Do not make scalar `va_arg` or `va_copy` support a
condition for closing this source idea unless the plan owner intentionally
expands or replaces the lifecycle state.

## Proof

Ran the delegated Step 5 command exactly and preserved `test_after.log`:

```sh
{ cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_cli_riscv64_variadic_entry_missing_contract_obj|backend_riscv_object_emission)$' && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-milestone-allowlist.txt && CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-milestone-allowlist.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh; } > test_after.log 2>&1 || true
```

Build and the four focused CTest targets passed:

- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_printer`
- `backend_cli_riscv64_variadic_entry_missing_contract_obj`
- `backend_riscv_object_emission`

The representative allowlist completed with `total=2 passed=0 failed=2`.
Inspected logs:

- `build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`
- `build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`

Both case logs report the same residual broader diagnostic:
`RV64 object variadic function lowering is not implemented`. `test_after.log`
is the preserved proof log.

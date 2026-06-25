Status: Active
Source Idea Path: ideas/open/362_rv64_scalar_vararg_and_variadic_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Representative and Baseline Validation

# Current Packet

## Just Finished

Step 5 validation completed for `plan.md` Step 5.

Ran the milestone proof for the completed scalar `va_arg`, `va_copy`, and RV64
variadic admission-gate work. The focused backend subset remains green:

- `backend_riscv_object_emission`
- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_printer`

Representative RV64 allowlist outcomes were rerun and preserved in
`test_after.log`:

- `src/20030914-2.c` still fails at
  `unsupported_param_home: RV64 object route requires all parameters in supported GPR homes`
  (`build/rv64_gcc_c_torture_backend/src_20030914-2.c/case.log`).
- `src/920908-1.c` still fails at
  `unsupported_variadic_helper_lowering: RV64 object route does not yet lower va_start helper`
  (`build/rv64_gcc_c_torture_backend/src_920908-1.c/case.log`).

The representative scan summary is `total=2 passed=0 failed=2`, which is
expected for this milestone validation.

## Suggested Next

Hand this completed milestone back to the supervisor/plan-owner for lifecycle
handling. The remaining representative blockers should be split into a separate
follow-up idea instead of expanding this plan:

- RV64 object parameter-home support for non-GPR or otherwise unsupported
  parameter homes, represented here by `src/20030914-2.c`.
- RV64 object lowering for prepared variadic helper calls, starting with
  `va_start`, represented here by `src/920908-1.c`.

## Watchouts

- The proof did not use testcase-name matching, expectation downgrades, or
  allowlist filtering to claim progress; the representative scan used the two
  delegated representatives only and allowed the expected scan failure via the
  delegated `|| true` wrapper.
- `src/20030914-2.c` is now classified outside this scalar vararg/helper fact
  milestone: it is blocked by general RV64 object parameter-home coverage.
- `src/920908-1.c` is now classified at the prepared variadic helper-lowering
  boundary. A future `va_start` packet should prove real helper materialization,
  not just another diagnostic rename.
- `fragment_for_prepared_call` intentionally rejects
  `PreparedCallWrapperKind::DirectExternVariadic`; do not conflate variadic
  function entry/helper support with external variadic call support.

## Proof

Focused proof passed, and the expected-failing representative scan was
preserved in `test_after.log`.

```sh
cmake --build build --target backend_riscv_object_emission_test backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test c4cll && ctest --test-dir build -R '^(backend_riscv_object_emission|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' --output-on-failure > test_after.log && printf 'src/20030914-2.c\nsrc/920908-1.c\n' > /tmp/rv64-vararg-step5-representatives.txt && (CASE_TIMEOUT_SEC=20 ALLOWLIST=/tmp/rv64-vararg-step5-representatives.txt scripts/check_progress_rv64_gcc_c_torture_backend.sh >> test_after.log 2>&1 || true)
```

Result: command exited 0. `test_after.log` records 3/3 focused CTests passed
and representative totals `total=2 passed=0 failed=2`.

Status: Active
Source Idea Path: ideas/open/300_aarch64_scalar_cast_machine_printer_forms.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove focused closure and report residuals

# Current Packet

## Just Finished

Step 4 proved focused scalar-cast printer closure after the Step 2 and Step 3
repairs. The required build succeeded, and the delegated focused CTest subset
still has the same residual-only shape:

- `00105`, `00126`, `00134`, and `00135` pass.
- `00035` fails with `RUNTIME_NONZERO exit=1`.
- `00151` fails with `RUNTIME_NONZERO exit=1`.
- `00208` fails with `RUNTIME_NONZERO exit=Segmentation fault`.

No focused case in the focused proof or broad backend inventory still reports
the old scalar-cast printer diagnostics involving unsupported `zero_extend`
forms, `sign_extend`, `scalar cast node requires a structured register source
operand`, or related structured-source admission failures.

The broad backend-regex inventory improved from the accepted post-299 baseline
of 294 passed / 58 failed to 298 passed / 54 failed. Current broad residual
buckets in `test_after.log` are:

- `FRONTEND_FAIL`: 14.
- `BACKEND_FAIL`: 1.
- `RUNTIME_NONZERO`: 23, including focused residuals `00035`, `00151`, and
  `00208`.
- `RUNTIME_MISMATCH`: 14.
- CTest timeouts: 2 (`00143`, `00220`).

## Suggested Next

Ask the plan owner to close or retire idea 300 as scalar-cast printer complete,
or explicitly split the remaining focused runtime residuals (`00035`, `00151`,
`00208`) into a separate owner if the supervisor wants to pursue them.

## Watchouts

- The scalar-cast owner is closure-ready from this Step 4 proof perspective:
  the old focused scalar-cast printer diagnostics are gone.
- `00035`, `00151`, and `00208` remain runtime residuals and should not be
  claimed as scalar-cast printer blockers without fresh generated-code evidence.
- No stale qemu, c-testsuite runner, or CTest runtime processes were found
  before or after the broad backend command, so no processes were terminated.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration.

## Proof

Ran:

```bash
cmake --build --preset default
ctest --test-dir build -j7 -R '^c_testsuite_aarch64_backend_src_(00035|00105|00126|00134|00135|00151|00208)_c$' --output-on-failure
timeout 900s ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: build succeeded; focused CTest failed 3/7 on residual runtime modes
only; broad backend-regex proof failed 54/352 with 298 passing. Proof log:
`test_after.log`.

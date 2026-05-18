Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Against The AArch64 C-Testsuite Backend Subset

# Current Packet

## Just Finished

Step 5 from `plan.md`: ran the targeted AArch64 c-testsuite backend subset for
the repaired symbol-address call-boundary move representatives and recorded the
truthful post-repair classifications.

The old representative failure is gone from the saved proof log:

- `deferred_unsupported: call-boundary move node requires prepared register source and destination`

Current representative subset classifications:

- `00040.c`: `[BACKEND_FAIL]` undefined temporary symbols `.LBB99_2`,
  `.LBB99_7`, `.LBB99_9`, `.LBB99_11`, `.LBB99_13`, `.LBB100_16`, and
  `.LBB100_25`.
- `00056.c`: `[RUNTIME_UNAVAILABLE]` because no
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is configured.
- `00125.c`: `[RUNTIME_UNAVAILABLE]` because no
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is configured.
- `00131.c`: `[RUNTIME_UNAVAILABLE]` because no
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is configured.
- `00156.c`: `[BACKEND_FAIL]` undefined temporary symbol `.LBB89_2`.

The selected subset still exits nonzero: five tests were selected, zero passed,
three are `[RUNTIME_UNAVAILABLE]`, and two are `[BACKEND_FAIL]` for undefined
temporary symbols. `[RUNTIME_UNAVAILABLE]` is not pass evidence.

## Suggested Next

Run Step 6 broader backend scan checkpoint with a supervisor-selected AArch64
backend label or regex to confirm the repair reduces the broader
call-boundary move family and does not merely clear the five representatives.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Remaining representative blockers are not the old call-boundary symbol-address
  move path.
- `00040.c` and `00156.c` now expose undefined temporary label backend failures;
  keep that as a separate capability family instead of folding it into the
  call-boundary move packet.
- `00056.c`, `00125.c`, and `00131.c` need an AArch64 runner or host for runtime
  evidence.

## Proof

Delegated proof command used:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00040|00056|00125|00131|00156)_c$'; } 2>&1 | tee test_after.log
```

Result:

- AArch64 scan configure/build succeeded.
- The representative c-testsuite command selected five tests and failed with
  the truthful next classifications: three `[RUNTIME_UNAVAILABLE]` results and
  two `[BACKEND_FAIL]` undefined temporary symbol results.
- `test_after.log` contains no occurrence of
  `deferred_unsupported: call-boundary move node requires prepared register source and destination`.

Proof log: `test_after.log`. The delegated proof command exits nonzero because
the representative c-testsuite cases still include runtime-unavailable and
undefined temporary symbol blockers; those blockers are outside this proof-only
packet's owned files.

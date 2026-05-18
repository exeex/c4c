Status: Active
Source Idea Path: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Backend Scan Checkpoint

# Current Packet

## Just Finished

Step 6 from `plan.md`: ran the broader AArch64 backend c-testsuite checkpoint
after the symbol-address call-boundary move repair and recorded post-repair
failure-stage counts.

CTest selected 220 `aarch64_backend` tests and all 220 failed. Stage counts in
the saved proof log:

- `[RUNTIME_UNAVAILABLE]`: 151
- `[FRONTEND_FAIL]`: 30
- `[BACKEND_FAIL]`: 39

The old call-boundary move diagnostic occurs 0 times in `test_after.log`:

- `deferred_unsupported: call-boundary move node requires prepared register source and destination`

Representative remaining follow-up families:

- Runtime evidence blocked by missing runner: `00001.c`, `00002.c`, `00056.c`,
  `00125.c`, and `00131.c` are `[RUNTIME_UNAVAILABLE]` because no
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is configured.
- Undefined temporary symbols: `00005.c`, `00006.c`, `00040.c`, `00156.c`, and
  `00220.c` are `[BACKEND_FAIL]` cases with `.LBB...` temporary labels.
- AArch64 machine-node printer gaps: `00024.c`, `00027.c`, `00028.c`, and
  `00031.c` are `[FRONTEND_FAIL]` cases that reach the machine-node printer but
  fail scalar instruction spelling.
- Semantic `lir_to_bir` coverage gaps: `00046.c`, `00140.c`, `00204.c`, and
  `00218.c` are `[FRONTEND_FAIL]` cases that fail before the prepared-module
  handoff.

This checkpoint gives completion-review evidence for the current
symbol-address call-boundary move idea: the old owned diagnostic is absent from
the broad scan. The scan is not pass evidence because runtime-unavailable cases
must not be counted as passes and unrelated backend/frontend blockers remain.

## Suggested Next

Supervisor/plan-owner completion review for the current symbol-address
call-boundary move idea. If this idea is parked or closed, keep the next code
packet focused on a separate source idea for the largest remaining non-runtime
family, likely undefined temporary label emission.

## Watchouts

- Do not re-activate the parked runtime-runner route from
  `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md` unless an
  AArch64 host or `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- Do not count `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not weaken expectations, allowlists, unsupported classifications, or
  stage-specific diagnostics.
- Remaining broad-scan blockers are not the old call-boundary symbol-address
  move path; do not fold undefined temporary labels, scalar printer gaps, or
  semantic `lir_to_bir` coverage into this completed repair idea.
- True runtime pass evidence still requires an AArch64 host or configured
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.

## Proof

Delegated proof command used:

```sh
set -o pipefail; { cmake -S . -B build-aarch64-scan -DENABLE_C4C_BACKEND=ON -DENABLE_C_TESTSUITE_AARCH64_BACKEND_SCAN=ON -DC_TESTSUITE_AARCH64_BACKEND_RUNNER="${C_TESTSUITE_AARCH64_BACKEND_RUNNER}" && cmake --build build-aarch64-scan --target c4cll -j && ctest --test-dir build-aarch64-scan --output-on-failure -L aarch64_backend; } 2>&1 | tee test_after.log
```

Result:

- AArch64 scan configure/build succeeded.
- The broader c-testsuite command selected 220 tests and failed with 151
  `[RUNTIME_UNAVAILABLE]`, 30 `[FRONTEND_FAIL]`, and 39 `[BACKEND_FAIL]`
  results.
- `test_after.log` contains 0 occurrences of
  `deferred_unsupported: call-boundary move node requires prepared register source and destination`.

Proof log: `test_after.log`. The delegated proof command exits nonzero because
the broader c-testsuite label still includes runtime-unavailable cases and
unrelated backend/frontend blockers; those blockers are outside this
proof/classification-only packet's owned files.

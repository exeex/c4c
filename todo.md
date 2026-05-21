Status: Active
Source Idea Path: ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Representative And Classify Residuals

# Current Packet

## Just Finished

Step 4 reran the delegated focused representative proof after commit
`047790dac`. The selected backend tests, `00140`, and `00204` all pass; there
is no new `00204` first bad fact in this proof.

The active stdarg format-byte owner is acceptance-proven for the delegated
subset. The representative no longer overreads from the second stdarg
`%7s %9s ...` format string into adjacent `HFA long double:` text, and it does
not advance to HFA, byval lane, aggregate `va_arg`, MOVI, or another known
handoff owner under this command.

Closure review did not close idea 331 because the plan-owner close-time
regression guard failed. The existing 145-test `test_before.log` /
`test_after.log` pair was rejected by the guard for equal pass count
(`145/145` before and after). A broader full-suite close guard was then run
using `test_baseline.log` as `test_before.log` and a fresh current full-suite
run as `test_after.log`; it improved the overall pass count by one and resolved
`c_testsuite_aarch64_backend_src_00140_c` plus
`c_testsuite_aarch64_backend_src_00204_c`, but introduced two new failing
tests relative to the baseline:
`c_testsuite_aarch64_backend_src_00123_c` and
`c_testsuite_aarch64_backend_src_00130_c`.

## Watchouts

Closure status: close rejected by regression guard. Do not move
`ideas/open/331_aarch64_variadic_stdarg_cursor_format_residual.md` to
`ideas/closed/` until the supervisor resolves or classifies the two new
full-suite failures and reruns an accepted close gate.

Keep the closure review scoped to the stdarg cursor/format-byte owner. The
green `00204` representative means no lifecycle handoff is currently needed
for HFA, byval aggregate, aggregate `va_arg`, MOVI, expectations, unsupported
classification, or runner behavior.

The failed full-suite close guard is a regression/lifecycle blocker, not
evidence that the stdarg format-byte owner itself remains broken.

## Proof

Ran the delegated Step 4 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded/up to date; `backend_.*` passed;
`c_testsuite_aarch64_backend_src_00140_c` passed;
`c_testsuite_aarch64_backend_src_00204_c` passed. The selected test set ran
145 tests with 0 failures (`100% tests passed, 0 tests failed out of 145`).
`test_after.log` is the preserved proof log.

Plan-owner close gate:

```sh
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log
```

Result: failed. Current `test_before.log` is the copied full-suite
`test_baseline.log`; current `test_after.log` is the fresh full-suite run.
The guard reported `before: passed=3341 failed=33 total=3374`,
`after: passed=3342 failed=33 total=3375`, resolved `00140` and `00204`, and
new failures in `00123` and `00130`.

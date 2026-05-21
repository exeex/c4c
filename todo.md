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

## Watchouts

Closure readiness: ready for plan-owner closure review for idea 331, subject
to any supervisor-selected broader regression guard. This executor packet did
not run a broader full-regression command beyond the delegated focused subset.

Keep the closure review scoped to the stdarg cursor/format-byte owner. The
green `00204` representative means no lifecycle handoff is currently needed
for HFA, byval aggregate, aggregate `va_arg`, MOVI, expectations, unsupported
classification, or runner behavior.

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

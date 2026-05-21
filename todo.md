Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Buckets

# Current Packet

## Just Finished

Step 1 captured the current backend-regex residual surface for the inventory
umbrella after closing focused ideas 366 and 370.

The existing `test_baseline.log` / `test_after.log` were not sufficient for
the exact backend-regex selected count: they showed the accepted full-suite
baseline (`3347` passed, `28` failed, `3375` total) but not a fresh
`ctest -R backend` surface. Ran the delegated backend-regex command and
preserved the result in `test_after.log`.

Backend-regex surface:

- selected: `356`
- passed: `344`
- failed total: `12`
- failed non-timeout: `10`
- timeout: `2`

No local `backend_*` unit, backend route, MIR, BIR, CLI, runtime, or smoke
tests failed in the backend-regex run. The residual failures are only external
`c_testsuite_aarch64_backend_*` tests:

```text
c_testsuite_aarch64_backend_src_00157_c (Failed)
c_testsuite_aarch64_backend_src_00163_c (Failed)
c_testsuite_aarch64_backend_src_00174_c (Failed)
c_testsuite_aarch64_backend_src_00176_c (Failed)
c_testsuite_aarch64_backend_src_00182_c (Failed)
c_testsuite_aarch64_backend_src_00183_c (Failed)
c_testsuite_aarch64_backend_src_00187_c (Failed)
c_testsuite_aarch64_backend_src_00200_c (Timeout)
c_testsuite_aarch64_backend_src_00205_c (Failed)
c_testsuite_aarch64_backend_src_00207_c (Timeout)
c_testsuite_aarch64_backend_src_00216_c (Failed)
c_testsuite_aarch64_backend_src_00218_c (Failed)
```

## Suggested Next

Execute Step 2 classification by grouping the 12 external AArch64 backend
residuals by first bad fact. Start with generated assembly/output evidence for
the two timeouts (`00200`, `00207`) and a small sample of failed runtime
mismatches (`00157`, `00163`, `00174`) before assigning focused owner ideas.

## Watchouts

This umbrella remains classification-only. Do not implement fixes under idea
295. Do not reopen closed owners from counts alone; require generated-code,
diagnostic, timeout, or proof evidence that contradicts their closure boundary.
Current evidence says the local backend unit/route surface is green, so Step 2
should focus on external AArch64 backend residual families.

## Proof

Ran the delegated Step 1 backend-regex capture:

```sh
cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1
```

Result: `97% tests passed, 12 tests failed out of 356`.
`test_after.log` is the preserved proof/classification log. The command exited
nonzero because the residual external backend failures are still present.

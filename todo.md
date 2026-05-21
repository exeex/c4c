Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Surface

# Current Packet

## Just Finished

Completed Step 1: Capture Current Backend Surface. Ran the supervisor-selected
backend proof after idea 377 and captured the current backend-regex residual
surface in `test_after.log`.

Counts from `ctest -R backend`:

- Selected: 357
- Passed: 353
- Failed total: 4
- Timeouts: 2

Current failing backend-regex residuals:

- `c_testsuite_aarch64_backend_src_00174_c` - failed with
  `[RUNTIME_MISMATCH]` in `tests/c/external/c-testsuite/src/00174.c`
- `c_testsuite_aarch64_backend_src_00200_c` - timeout
- `c_testsuite_aarch64_backend_src_00207_c` - timeout
- `c_testsuite_aarch64_backend_src_00216_c` - failed with
  `[RUNTIME_NONZERO]` / segmentation fault in
  `tests/c/external/c-testsuite/src/00216.c`

Local backend/unit tests are clean within this `-R backend` capture; residuals
remain only in external `c_testsuite_aarch64_backend_*` tests. `00187` is
absent from the residual list; `c_testsuite_aarch64_backend_src_00187_c` was
selected and passed.

## Suggested Next

Execute Step 2: classify the four residual buckets by first bad fact and owner
boundary before selecting a focused owner.

## Watchouts

This umbrella is for classification and focused-owner selection only. Do not
implement fixes under idea 295. Do not reopen closed owners from counts alone;
require generated-code, diagnostic, timeout, or proof evidence that
contradicts their closure boundary. Keep timeout-only cases (`00200`, `00207`)
quarantined unless a safe timeout-specific owner is explicitly selected.

## Proof

Ran:

```sh
{ cmake --build --preset default && ctest --test-dir build -j10 -R backend --output-on-failure; } > test_after.log 2>&1
```

The build step completed and CTest returned nonzero because the selected
backend subset still has the four expected external residuals listed above.
Proof log: `test_after.log`.

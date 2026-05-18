# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh AArch64 Inventory Evidence

# Current Packet

## Just Finished

Step 1 refreshed the AArch64 backend c-testsuite inventory evidence after the
closed LR-preservation and scalar call-value routes. The scan registered 212
cases and wrote refreshed evidence to
`/tmp/c4c_aarch64_backend_scan_212.log` and
`/tmp/c4c_aarch64_backend_scan_212.classified`.

Bucket counts:

- `PASS`: 38
- `FRONTEND_FAIL`: 49
- `RUNTIME_NONZERO`: 101
- `RUNTIME_MISMATCH`: 23
- `TIMEOUT`: 1
- `BACKEND_FAIL`: 0

Delta from the old 46/49/87/7/23 inventory:

- `PASS`: -8
- `FRONTEND_FAIL`: 0
- `RUNTIME_NONZERO`: +14
- `RUNTIME_MISMATCH`: +16
- `TIMEOUT`: -22
- `BACKEND_FAIL`: 0

## Suggested Next

Supervisor should use the refreshed classification evidence to choose the next
focused idea or hand the inventory to plan-owner for route selection. This
packet intentionally did not create or select that focused idea.

## Watchouts

- This umbrella route is planning and classification only; do not implement
  compiler, runner, or test expectation changes here.
- Both pre-scan and post-scan stale-process checks only matched the check
  command itself; no stale repo runtime/test process was cleaned up.
- The old 46/49/87/7/23 counts are stale for post-scalar-call-value routing.
- Split and switch to a focused idea before implementation work begins.

## Proof

Command:

```sh
ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure > /tmp/c4c_aarch64_backend_scan_212.log 2>&1
```

Result: exited nonzero as expected with remaining failures; CTest reported
`18% tests passed, 174 tests failed out of 212`. Classification proof is
`/tmp/c4c_aarch64_backend_scan_212.classified`, one line per registered case.
No root-level proof log was created for this evidence-only packet.

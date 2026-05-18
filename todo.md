# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Refresh Safe AArch64 Backend Scan

# Current Packet

## Just Finished

Completed Step 2 by refreshing the AArch64 backend c-testsuite scan with an
explicit timeout. No implementation, tests, lifecycle source files, or root
proof logs were changed.

Refresh command:

```bash
pgrep -af '/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' > /tmp/c4c_aarch64_backend_scan_212_refresh.pre_pgrep || true
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_backend_scan_212_refresh.log 2>&1
pgrep -af '/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' > /tmp/c4c_aarch64_backend_scan_212_refresh.post_pgrep || true
```

Artifacts:

- `/tmp/c4c_aarch64_backend_scan_212_refresh.log`
- `/tmp/c4c_aarch64_backend_scan_212_refresh.classified`
- `/tmp/c4c_aarch64_backend_scan_212_refresh.pre_pgrep`
- `/tmp/c4c_aarch64_backend_scan_212_refresh.post_pgrep`
- `/tmp/c4c_aarch64_backend_scan_212_refresh.filtered_pgrep`

Refreshed counts from the classified artifact:

- `PASS`: 82
- `FRONTEND_FAIL`: 49
- `BACKEND_FAIL`: 0
- `RUNTIME_NONZERO`: 53
- `RUNTIME_MISMATCH`: 25
- `TIMEOUT`: 3
- Total: 212

Runtime nonzero signal/exit families:

- `Segmentation fault`: 7
- exit `1`: 38
- exit `2`: 3
- exit `64`: 1
- exit `96`: 1
- exit `183`: 1
- exit `221`: 1
- exit `230`: 1

Generated-runtime process check:

- The raw pre/post `pgrep -af` files each contain a self-match for the shell
  command line because the search path appears in the command text.
- A filtered process check for command lines beginning with
  `/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/` wrote
  `/tmp/c4c_aarch64_backend_scan_212_refresh.filtered_pgrep` and found 0
  generated-runtime processes.
- No stale generated-runtime process is currently known.

## Suggested Next

Run Step 3 sampling against the refreshed classification before selecting or
splitting a next owner. Do not reopen a closed owner from counts alone; sample
the current generated BIR/prepared/MIR/assembly/runtime shape first.

Notable Step 3 inputs from the refreshed scan:

- Closed-owner representatives now passing include `src/00004.c`,
  `src/00005.c`, `src/00013.c`, `src/00087.c`, `src/00125.c`,
  `src/00131.c`, `src/00154.c`, and `src/00210.c`.
- `src/00089.c` is currently `RUNTIME_NONZERO` with `Segmentation fault` and
  `src/00124.c` is currently `RUNTIME_NONZERO` with exit `96`; these are
  contrary to their closed-owner summaries and require Step 3 evidence before
  deciding whether they are scan/build-state artifacts, fresh regressions, or
  separate current owners.
- Timeout cases are `src/00132.c`, `src/00173.c`, and `src/00220.c`; keep them
  quarantined unless Step 3 explicitly chooses a hang-safe route.

## Watchouts

- This umbrella route is for inventory and splitting only; do not implement a
  repair directly from it.
- Do not change implementation files, tests, expected outputs, runner behavior,
  CTest registration, allowlists, unsupported classifications, or timeout
  policy.
- Do not use broad runtime scans without explicit timeout and stale generated
  process cleanup.
- Keep timeout/hang cases separated unless they are proven to be the next safe
  semantic owner.
- Do not reopen closed AArch64 owners unless new proof contradicts their
  closure.
- The refreshed scan is suitable for Step 3 sampling, but it is not itself a
  semantic owner decision.
- Treat `src/00089.c` and `src/00124.c` as high-priority consistency checks
  because they conflict with recent closure summaries.

## Proof

The delegated timeout-safe broad scan was run and captured in
`/tmp/c4c_aarch64_backend_scan_212_refresh.log`. Classification was written to
`/tmp/c4c_aarch64_backend_scan_212_refresh.classified`. No root proof logs were
used.

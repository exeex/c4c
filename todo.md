# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Inspect Runtime Failure Families

# Current Packet

Use the confirmed 212-case AArch64 backend scan inventory as the starting
point. Do not implement fixes in this umbrella plan; create focused ideas for
semantic repair directions and switch before coding.

## Just Finished

- Completed plan.md Step 1, "Confirm Safe Inventory".
- Verified `/tmp/c4c_aarch64_backend_scan_212.classified` exists with 212
  classified cases and these counts: `46 PASS`, `49 FRONTEND_FAIL`,
  `87 RUNTIME_NONZERO`, `7 RUNTIME_MISMATCH`, `23 TIMEOUT`,
  `0 BACKEND_FAIL`.
- Verified scan artifact paths and mtimes:
  `/tmp/c4c_aarch64_backend_scan_212.classified`
  `2026-05-18 13:00:58.107144001 +0000`;
  `/tmp/c4c_aarch64_backend_scan_212.log`
  `2026-05-18 13:00:31.259906003 +0000`.
- Ran the stale-process check from the supervisor workflow; it matched only the
  check's own `ps | rg` pipeline and found no stale `ctest`, `cmake -D`,
  AArch64 test binary, or `c4cll` process.

## Suggested Next

- Sample low-numbered `RUNTIME_NONZERO` and `RUNTIME_MISMATCH` cases from
  `/tmp/c4c_aarch64_backend_scan_212.classified`.
- Group runtime failures by semantic owner, then create focused ideas for the
  clearest repair families.

## Watchouts

- Do not hand broad runtime scans to executors without timeout and stale-process
  cleanup.
- Do not count timeout/hang cases as ordinary repair packets.
- Do not change expected outputs, allowlists, unsupported classifications, or
  CTest behavior to improve counts.
- Keep frontend-owned failures separate from AArch64 backend repair ideas.

## Proof

Last useful scan command:

```bash
ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure
```

Inventory proof used existing artifacts
`/tmp/c4c_aarch64_backend_scan_212.classified` and
`/tmp/c4c_aarch64_backend_scan_212.log`.

Verified counts with:

```bash
awk '{counts[$1]++} END {for (k in counts) print k, counts[k]}' /tmp/c4c_aarch64_backend_scan_212.classified | sort
```

Stale-process check:

```bash
ps -eo pid,ppid,pgid,pcpu,stat,etime,cmd --sort=-pcpu | rg 'ctest|cmake -D|/workspaces/c4c/build[^ ]*(/| )[A-Za-z0-9_.+-]*\.bin|/workspaces/c4c/build[^ ]*/c4cll'
```

Result: no stale process found; output only contained the check's own pipeline.
No `test_after.log` was produced because this delegated packet was
artifact-inspection only and did not run CTest.

# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Safe Inventory

# Current Packet

Use the existing 212-case AArch64 backend scan inventory as the starting point.
Do not implement fixes in this umbrella plan; create focused ideas for semantic
repair directions and switch before coding.

## Just Finished

- Created the umbrella source idea and active runbook.
- Recorded the latest known classification:
  `46 PASS`, `49 FRONTEND_FAIL`, `87 RUNTIME_NONZERO`,
  `7 RUNTIME_MISMATCH`, `23 TIMEOUT`, `0 BACKEND_FAIL`.
- Runtime-hang quarantine already removed:
  `00021.c`, `00025.c`, `00033.c`, `00040.c`,
  `00078.c`, `00080.c`, `00083.c`, `00084.c`.

## Suggested Next

- Reuse `/tmp/c4c_aarch64_backend_scan_212.classified` if present; otherwise
  rerun the scan with explicit timeout.
- Start with low-numbered `RUNTIME_NONZERO` and `RUNTIME_MISMATCH` cases.
- Group failures by semantic owner, then create focused ideas for the clearest
  repair families.

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

Stale process check now lives in `.codex/skills/c4c-supervisor/SKILL.md`
baseline review follow-up.

# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Current AArch64 Backend Inventory

# Current Packet

## Just Finished

Step 1 refreshed the current `build-aarch64-scan` AArch64 backend c-testsuite
inventory with the delegated explicit-timeout CTest command.

Inventory result from `/tmp/aarch64_backend_inventory_step1_20260518T154828Z.log`:
- Total: 212 tests.
- Passed: 80.
- Failed, non-timeout: 129.
- Timed out: 3.
- Failed including timeouts: 132.

Runner message categories in the same pass:
- `[FRONTEND_FAIL]`: 49.
- `[BACKEND_FAIL]`: 2.
- `[RUNTIME_NONZERO]`: 55.
- `[RUNTIME_MISMATCH]`: 23.
- CTest `***Timeout`: 3.

Stale-process check was recorded at
`/tmp/aarch64_backend_inventory_step1_stale_process_check_20260518T154828Z.log`.
It matched only the checking shell and `rg` process itself; no stale `ctest`,
`cmake -D`, generated runtime `.bin`, or `build*/c4cll` process was left after
the scan.

## Suggested Next

Supervisor should route the refreshed inventory into the next lifecycle action:
either split a focused repair idea for one failure family or ask the plan owner
to update/deactivate the umbrella runbook if Step 1 is now exhausted.

## Watchouts

- This umbrella plan is inventory and lifecycle routing only; do not implement
  compiler repairs under it.
- Do not change tests, expected outputs, runner behavior, CTest registration,
  allowlists, unsupported classifications, or timeout policy.
- The refreshed scan still has 3 timeout cases; keep timeout/hang work separate
  from ordinary mismatch/nonzero/backend/frontend families unless a
  timeout-specific focused idea is created.
- Split a focused `ideas/open/*.md` repair idea before implementation starts.

## Proof

Command:
`ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure`

Result: CTest exited `8` with the expected inventory failures/timeouts:
80 passed, 129 failed non-timeout, and 3 timed out out of 212 total tests.
Raw log: `/tmp/aarch64_backend_inventory_step1_20260518T154828Z.log`.

Stale-process command:
`ps -eo pid,ppid,pgid,pcpu,stat,etime,cmd --sort=-pcpu | rg 'ctest|cmake -D|/workspaces/c4c/build[^ ]*(/| )[A-Za-z0-9_.+-]*\.bin|/workspaces/c4c/build[^ ]*/c4cll' || true`

Stale-process status: no stale generated-runtime or compiler processes found;
the saved output only contains the checking shell and `rg` command itself.
Raw stale-process log:
`/tmp/aarch64_backend_inventory_step1_stale_process_check_20260518T154828Z.log`.

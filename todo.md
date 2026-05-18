# AArch64 C-Testsuite Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rehydrate Inventory Context

# Current Packet

## Just Finished

Completed Step 1 inventory rehydration without running a broad scan.

Current source context:

- Active inventory source is `ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md`.
- Recent closed AArch64 owners `285` through `291` were inspected and should
  not be reopened without contrary evidence:
  `285` non-leaf link-register preservation, `286` scalar call value
  semantics, `287` string/global address direct external-call lowering, `288`
  stack-frame/SP alignment, `289` function-pointer indirect-call values, `290`
  scalar parameter/ALU authority, and `291` call-argument register authority.

Scan artifact decision:

- `/tmp/c4c_aarch64_backend_scan_212.log` exists and is timestamped
  `2026-05-18 14:39 UTC`.
- `/tmp/c4c_aarch64_backend_scan_212.classified` exists and is timestamped
  `2026-05-18 14:40 UTC`.
- The classified file reports 212 cases with 38 `PASS`, 49 `FRONTEND_FAIL`,
  101 `RUNTIME_NONZERO`, 23 `RUNTIME_MISMATCH`, and 1 `TIMEOUT`.
- That scan is stale for current ownership because it still marks closed-owner
  representatives as failing, including `src/00087.c`, `src/00089.c`,
  `src/00124.c`, and `src/00210.c`.
- No newer broad AArch64 scan artifact was found under `/tmp`; the latest scan
  pair is trusted only as historical inventory context, not as the current
  split source.

## Suggested Next

Run Step 2 to refresh the AArch64 backend inventory with an explicit per-test
timeout and stale generated-runtime process check. Use this exact timeout-safe
command shape, writing outside root proof logs:

```bash
pgrep -af '/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' > /tmp/c4c_aarch64_backend_scan_212_refresh.pre_pgrep || true
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_backend_scan_212_refresh.log 2>&1
pgrep -af '/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' > /tmp/c4c_aarch64_backend_scan_212_refresh.post_pgrep || true
```

If either pgrep file is nonempty, treat it as stale generated-runtime process
evidence to resolve before trusting follow-on scans.

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
- The current known-stale scan still contains pre-closure failures from owners
  `289`, `290`, and `291`; do not split new work from those entries until a
  refreshed scan confirms a current failure shape.

## Proof

No build proof was required and no broad scan was run in this packet. Evidence
came from inspecting source idea `284`, closed ideas `285` through `291`, and
the existing `/tmp/c4c_aarch64_backend_scan_212.*` artifacts.

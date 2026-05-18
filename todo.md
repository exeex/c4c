Status: Active
Source Idea Path: ideas/open/284_aarch64_c_testsuite_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Post-293 Inventory

# Current Packet

## Just Finished

Step 1: Refresh Post-293 Inventory recorded the supervisor-run AArch64 backend
inventory scan in canonical state only. The reused scan was:

```sh
ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure > /tmp/c4c_aarch64_post293_inventory_scan.log 2>&1
```

Result: 130/212 passed, 82 failed.

Failure buckets:

- `FRONTEND_FAIL` 52: representatives/all observed ids `00041`, `00024`,
  `00030`, `00031`, `00034`, `00035`, `00037`, `00046`, `00038`, `00054`,
  `00057`, `00055`, `00059`, `00064`, `00076`, `00077`, `00085`, `00092`,
  `00093`, `00101`, `00104`, `00105`, `00127`, `00126`, `00134`, `00135`,
  `00140`, `00139`, `00143`, `00151`, `00157`, `00173`, `00176`, `00181`,
  `00185`, `00182`, `00187`, `00194`, `00195`, `00207`, `00205`, `00203`,
  `00209`, `00208`, `00213`, `00214`, `00215`, `00212`, `00218`, `00216`,
  `00204`, `00200`.
- `RUNTIME_NONZERO` 22: representatives/all observed ids `00050`, `00121`,
  `00019`, `00089`, `00032`, `00066`, `00086`, `00102`, `00111`, `00113`,
  `00112`, `00123`, `00119`, `00130`, `00137`, `00138`, `00170`, `00144`,
  `00180`, `00179`, `00186`, `00189`.
- `RUNTIME_MISMATCH` 7: representatives/all observed ids `00159`, `00168`,
  `00217`, `00193`, `00175`, `00174`, `00196`.
- `TIMEOUT` 1: representative/all observed id `00220`.

The stale-process check after the scan was clean: only the checking shell and
`rg` process matched, with no stale generated c-testsuite runtime process.

## Suggested Next

Use the post-293 inventory to choose the next focused split from the largest
semantic bucket, starting with a representative `FRONTEND_FAIL` subfamily unless
the supervisor prefers a runtime bucket.

## Watchouts

- Do not implement fixes under this umbrella idea.
- Do not reopen closed AArch64 owners 285-293 without contradictory
  generated-code evidence.
- Preserve the post-293 broader-sample separation: `00159`, `00168`, and
  `00193` are closed-owner overlap until proven otherwise, while `00217` is
  pointer/address/string-heavy.
- Timeout cases require explicit timeout handling and stale-process cleanup.

## Proof

Documentation-only packet. Reused supervisor-run log
`/tmp/c4c_aarch64_post293_inventory_scan.log`; tests were not rerun.

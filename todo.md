# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Failure Sources

# Current Packet

Classify the captured `/workspaces/c4c/test_after.log` backend regex failures
from the main `/workspaces/c4c/build` tree. Do not implement fixes in this
umbrella plan.

## Just Finished

- Step 2: classified the captured main-build backend regex failures in
  `/workspaces/c4c/test_after.log` without rerunning CTest or changing code,
  expectations, allowlists, unsupported classifications, CTest registration,
  timeout policy, runner behavior, `test_after.log`, or `test_before.log`.
- The capture selected 352 backend-regex tests: 272 passed and 80 failed.
  Local backend/unit/CLI/internal failures are absent from the failed list; all
  captured failures are `c_testsuite_aarch64_backend_*` with labels
  `aarch64_backend c_testsuite`.
- Result-status split: 79 `Failed`, 1 `Timeout`
  (`c_testsuite_aarch64_backend_src_00220_c`).
- Source/family split:
  - 38 frontend machine-printer failures:
    - 22 fused compare-branch operand printer failures:
      `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`,
      `00057`, `00059`, `00076`, `00077`, `00085`, `00092`, `00093`,
      `00101`, `00127`, `00200`, `00203`, `00207`, `00212`, `00214`,
      `00215`.
    - 5 scalar-cast unsupported integer width printer failures: `00035`,
      `00105`, `00126`, `00151`, `00208`.
    - 3 scalar add/sub/bitwise immediate encoding range failures: `00031`,
      `00104`, `00213`.
    - 3 stack-slot store source scratch printer failures: `00173`, `00187`,
      `00194`.
    - 2 scalar cast unstructured source printer failures: `00134`, `00135`.
    - 2 scalar mul/div/rem operand-shape printer failures: `00064`, `00139`.
    - 1 scalar sub immediate-lhs printer failure: `00024`.
  - 14 semantic `lir_to_bir` admission failures:
    - 9 `gep local-memory` failures: `00143`, `00157`, `00176`, `00181`,
      `00182`, `00185`, `00195`, `00205`, `00209`.
    - 2 `store local-memory` failures: `00046`, `00140`.
    - 2 `load local-memory` failures: `00216`, `00218`.
    - 1 unsupported globals/aggregate bootstrap failure: `00204`.
  - 27 runtime failures after compilation:
    - 17 runtime nonzero exits: exit `1` for `00066`, `00086`, `00102`,
      `00111`, `00112`, `00113`, `00119`, `00123`, `00137`, `00138`,
      `00144`; exit `3` for `00050`; exit `215` for `00121`; Bus error for
      `00089`; Segmentation fault for `00170`, `00179`, `00189`.
    - 10 runtime output mismatches: `00159`, `00164`, `00168`, `00169`,
      `00172`, `00174`, `00175`, `00186`, `00193`, `00196`.
  - 1 timeout/hang case: `00220`.
- No existing per-test `.log`, `.out`, `.err`, `.stdout`, or `.stderr`
  artifacts were found under `build/c_testsuite_aarch64_backend/`.

## Suggested Next

- Pick one failure family for a focused follow-up packet, preferably by source
  owner rather than by individual c-testsuite filename.
- Before switching away from this umbrella, write durable findings back into
  `ideas/open/295_backend_regex_failure_family_inventory.md` under
  `## Deactivation Note`, following the idea 284 style.

## Watchouts

- Do not treat all `backend` regex failures as one owner.
- Keep local backend/unit regressions separate from AArch64 external
  c-testsuite runtime failures.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, or CTest registration to improve counts.
- Do not reopen closed AArch64 owners 285 through 294 from counts alone.
- The runtime buckets are symptom-based only; this packet did not inspect
  implementation or source-level c-testsuite semantics deeply enough to assign
  precise compiler root causes.

## Proof

Classification input:

```bash
/workspaces/c4c/test_after.log
```

Result: classified all 80 captured failures from the existing proof log:
38 frontend machine-printer failures, 14 semantic `lir_to_bir` admission
failures, 27 runtime failures, and 1 timeout/hang case. Proof log remains
`/workspaces/c4c/test_after.log`.

Artifact check:

```bash
find build/c_testsuite_aarch64_backend -maxdepth 3 -type f \( -name '*.log' -o -name '*.out' -o -name '*.err' -o -name '*.stderr' -o -name '*.stdout' \) | sort
```

Result: no matching per-test output artifacts found. No CTest rerun was
performed for this classification packet.

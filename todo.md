# AArch64 Scalar Expression Control-Value Authority Todo

Status: Active
Source Idea Path: ideas/open/292_aarch64_scalar_expression_control_value_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Owner-Boundary Validation And Close Readiness

# Current Packet

## Just Finished

Step 4 completed owner-boundary validation for the focused AArch64 scalar
expression/control-value authority slice without code changes.

The starter representatives remain green in the broader AArch64 backend
c-testsuite run: `src/00009.c`, `src/00012.c`, `src/00056.c`, `src/00156.c`,
`src/00161.c`, and `src/00211.c` all passed.

The timeout-safe broad run covered 212 AArch64 backend tests and reported 118
passes, 94 failures, and no stale generated runtime processes afterward.
Failure markers in the broad log were 55 `FRONTEND_FAIL`, 24
`RUNTIME_NONZERO`, 13 `RUNTIME_MISMATCH`, and 2 `TIMEOUT`.

Nearby same-family scalar sampling rejects filename overfit: compared with the
refreshed classification in `/tmp/c4c_aarch64_backend_scan_212_refresh.classified`,
37 previously failing cases now pass in the same broad run. In addition to the
six starters, representative nearby scalar/control wins include:

- integer/local scalar runtime cases: `src/00036.c`, `src/00039.c`,
  `src/00043.c`, `src/00044.c`, `src/00045.c`, `src/00047.c`,
  `src/00048.c`, `src/00049.c`, `src/00051.c`, `src/00058.c`,
  `src/00072.c`, `src/00073.c`, `src/00079.c`, `src/00081.c`,
  `src/00082.c`, `src/00090.c`, `src/00091.c`, `src/00115.c`,
  `src/00116.c`, `src/00146.c`, `src/00147.c`, `src/00148.c`,
  `src/00149.c`, and `src/00150.c`.
- control/value runtime-mismatch cases near the starter loop family:
  `src/00158.c`, `src/00160.c`, `src/00165.c`, `src/00177.c`,
  `src/00192.c`, and `src/00198.c`.
- closed-owner-adjacent address/call-value representatives that stayed green:
  `src/00125.c`, `src/00131.c`, and `src/00154.c`.

Owner-boundary buckets remain separated:

- pointer/aggregate address and object-layout failures remain out of this
  owner, including `src/00019.c` (`Segmentation fault`), `src/00042.c`
  (union object layout), `src/00050.c` (global struct/union aggregate), and
  `src/00172.c` (local pointer equality/dereference).
- timeout/hang-sensitive cases remain quarantined: `src/00132.c` and
  `src/00220.c` timed out under the required 5-second CTest timeout. The old
  timeout representative `src/00173.c` no longer timed out in this run but is
  now a compile-stage printer failure for a stack-slot store source scratch.
- compile-stage printer/admission gaps remain out of this owner, including
  `src/00024.c` (`scalar sub with an immediate lhs and register rhs is not
  printable`), `src/00173.c`/`src/00194.c` (stack-slot store source scratch is
  not printable), and `src/00200.c` (fused compare-branch operands are not
  printable).
- non-scalar or wider scalar areas remain separate: `src/00174.c` is floating
  arithmetic/comparison, `src/00175.c` is mixed char/int/float conversion and
  call lowering, `src/00186.c` is buffer/string formatting, and `src/00196.c`
  is short-circuit calls with side effects.

Closed-owner boundary evidence is still consistent with keeping previous
AArch64 owners closed: non-leaf LR preservation, string/global external-call
lowering, stack-frame/SP alignment, function-pointer indirect-call values,
scalar parameter ALU authority, and call-argument register authority all have
green representatives in this broad run (`src/00100.c`, `src/00121.c`,
`src/00125.c`, `src/00131.c`, `src/00154.c`, `src/00004.c`, `src/00005.c`,
`src/00013.c`, `src/00014.c`, `src/00015.c`, `src/00016.c`, `src/00087.c`,
and the starter call-argument cases). Residual failures should not reopen a
closed owner without generated-code evidence contradicting that closure.

Executor recommendation: source idea 292 is close-ready for plan-owner review.
The starter subset is green, broader proof shows semantic spillover beyond the
named starters, and remaining failures are separated into follow-on owner
buckets rather than a required next scalar substep for this idea.

## Suggested Next

Ask the supervisor to route source idea 292 to the plan owner for close or
deactivation review, using the Step 4 validation artifacts under
`/tmp/c4c_aarch64_scalar_step4_*` plus the existing focused proof history.

## Watchouts

- Do not solve this by filename checks, expectation changes, allowlist changes,
  unsupported downgrades, or CTest edits.
- Root `test_before.log` and `test_after.log` were intentionally not touched by
  this validation-only packet.
- Do not treat the current broad count as a full regression guard. It is a
  timeout-safe owner-boundary scan under `/tmp`, not a matched before/after
  acceptance log pair.
- Remaining broad failures include real future work, but the sampled residuals
  separate into pointer/aggregate object layout, timeout/hang, printer
  admission, floating/conversion/string, and side-effect control owners rather
  than a required continuation of source idea 292.
- If the plan owner wants a final close gate, the supervisor should request a
  matched regression-guard pass rather than asking an executor to reuse these
  `/tmp` logs as canonical before/after logs.

## Proof

Ran the timeout-safe broader AArch64 backend c-testsuite validation exactly as
delegated:

```sh
{ cmake --build build-aarch64-scan --target c4cll && ctest --test-dir build-aarch64-scan -L '^aarch64_backend$' -j 8 --timeout 5 --output-on-failure; } > /tmp/c4c_aarch64_scalar_step4_broad.log 2>&1
```

Result: failed as expected for broad inventory validation. Summary: 118/212
passed, 94/212 failed; timeout cases were `src/00132.c` and `src/00220.c`.
Proof log: `/tmp/c4c_aarch64_scalar_step4_broad.log`.

Required process check:

```sh
{ pgrep -af '^/workspaces/c4c/build-aarch64-scan/c_testsuite_aarch64_backend/' || true; } > /tmp/c4c_aarch64_scalar_step4_broad.pgrep
```

No generated runtime process remained. Process-check log:
`/tmp/c4c_aarch64_scalar_step4_broad.pgrep`.

Supporting transient artifacts:

- `/tmp/c4c_aarch64_scalar_step4_status.txt`
- `/tmp/c4c_aarch64_scalar_step4_new_passes.txt`
- `/tmp/c4c_aarch64_scalar_step4_summary.txt`

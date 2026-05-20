Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Residual Failures

# Current Packet

## Just Finished

Step 2: Classify Residual Failures completed from the existing post-339
`test_after.log` without rerunning tests or editing implementation files.

Current backend-regex summary from `test_after.log`:

- Selected: 354
- Passed: 329
- Failed/non-passed total: 25
- Timeout: 4
- Skipped: 0 reported
- Local backend/unit/CLI failures: 0
- External `c_testsuite_aarch64_backend_*` failures: 25

Classified first bad surface/stage:

- Frontend/prepared-node or machine-printer diagnostic, 1:
  `00143` fails with `[FRONTEND_FAIL]` after reaching the AArch64 machine-node
  printer; the first visible bad fact is scalar `sign_extend` printing without
  a structured register source operand.
- Semantic `lir_to_bir` admission or prepared-module handoff, 0 current
  failures in the CTest summary.
- Assembler/linker legality, 0 current failures in the CTest summary.
- Runtime nonzero/crash, 7:
  `00112` exits 160, `00119` exits 32, `00123` exits 112, `00140` segfaults,
  `00144` exits 176, `00181` segfaults, and `00200` exits 123 after printing
  `0 test(s) failed`.
- Runtime mismatch, 13:
  `00157`, `00159`, `00170`, `00171`, `00174`, `00175`, `00176`, `00182`,
  `00186`, `00195`, `00205`, `00216`, and `00218`.
- Timeout or output-storm, 4:
  `00173`, `00187`, `00207`, and `00220` each hit the 5-second CTest timeout.
  No separate output-storm body is visible in this log for those four timeout
  entries.

Closed-owner adjacency notes, using diagnostic/runtime evidence rather than
counts alone:

- `00143` is directly adjacent to closed idea 338's scalar cast
  register-source operand-fact owner and shows the same structured
  register-source printer diagnostic that idea 338's closure said had been
  cleared for `00143`; this is the only current compile-stage closed-boundary
  contradiction visible from `test_after.log`.
- `00112`, `00119`, `00123`, `00144`, `00174`, and `00200` match buckets that
  idea 339 explicitly parked outside the scalar local storage/writeback owner:
  pointer/null conditional materialization, FP comparison/expression
  materialization, and broad return-spill/ABI materialization.
- `00140` is adjacent to closed idea 311's call-boundary move owner, but the
  current first bad fact is runtime segfault, not the old selected-node
  machine-printer admission diagnostic.
- `00159` and `00170` are adjacent to the broad return-result/epilogue-clobber
  history from idea 336, but the current evidence is runtime mismatch output;
  reopening that owner would need generated-code evidence that the same stale
  return publication fact has returned.
- `00173`, `00175`, `00176`, `00181`, `00205`, and `00216` are adjacent to
  closed idea 338's focused bucket, but all except `00173` are now runtime
  mismatch/nonzero residuals rather than the old scalar-cast printer
  diagnostic; `00173` is a timeout residual.
- `00182` is adjacent to closed idea 307's large scalar immediate owner, but
  the current first bad fact remains runtime mismatch, not illegal immediate
  assembly.
- The remaining runtime mismatch and timeout cases (`00157`, `00171`,
  `00186`, `00187`, `00195`, `00207`, `00218`, `00220`) are under-evidenced
  as owners from this log alone. They need generated-code, prepared-state, or
  narrowed runtime probes before Step 3 can responsibly split a semantic
  repair owner.

## Suggested Next

Delegate Step 3 to select one focused owner only after reviewing the above
classification. The strongest evidence to scrutinize first is the `00143`
compile-stage scalar-cast printer diagnostic because it appears to contradict
closed idea 338's boundary; otherwise the runtime and timeout buckets need
narrow generated-code/prepared-state probes before selecting an owner.

## Watchouts

- Do not implement under the umbrella inventory plan.
- Do not reopen closed owners through idea 339 from counts alone; require the
  same first bad fact or generated-code/proof evidence tied to the closed
  boundary.
- Treat timeout and output-storm cases as unsafe for broad reruns unless the
  delegated command includes timeout and stale-process cleanup.
- Do not change expectations, runner behavior, unsupported classifications, or
  CTest registration to improve backend-regex counts.
- The failed-test summary contains only external
  `c_testsuite_aarch64_backend_*` entries: 21 ordinary `Failed` results and 4
  `Timeout` results.
- The timeout group is currently classification-only. Do not infer a semantic
  owner for `00173`, `00187`, `00207`, or `00220` without timeout-safe narrow
  proof.

## Proof

No command was rerun for this classification packet. The delegated proof was
the existing Step 1 log:

```sh
cmake --build build --target c4cll -j 2 && (ctest --test-dir build -j10 -R backend --output-on-failure > test_after.log 2>&1 || true)
```

Canonical proof log: `test_after.log`.

CTest summary from `test_after.log`:

- Selected: 354
- Passed: 329
- Failed/non-passed total: 25
- Timeout: 4
- Skipped: 0 reported
- Local backend/unit/CLI failures: 0
- `c_testsuite_aarch64_backend_*` failed/non-passed: 25

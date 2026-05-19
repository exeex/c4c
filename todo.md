# Backend Regex Failure Family Inventory Todo

Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Remaining Failures

# Current Packet

## Just Finished

Step 2: Classify Remaining Failures.

Classified the refreshed main-build backend-regex result from
`/workspaces/c4c/test_after.log` without rerunning the broad backend regex and
without implementation, expectation, allowlist, unsupported-classification,
CTest registration, timeout-policy, runner, or proof-log changes.

Local backend/unit/CLI bucket: 0 failures observed. The 62 refreshed failures
are all external `c_testsuite_aarch64_backend_*` tests.

Refreshed failure buckets:

- Machine-printer failures: 19 total.
  - Scalar add/sub/bitwise immediate outside plain `#imm` range: `00031`,
    `00104`, `00207`, `00213`, `00214`, `00215`.
  - Scalar zero/sign extension printer contract gaps: `00035`, `00105`,
    `00126`, `00134`, `00135`, `00151`, `00208`.
  - Scalar mul/div/rem operand-shape gaps: `00064`, `00139`.
  - Scalar sub immediate-lhs/register-rhs gap: `00024`.
  - Stack-slot store source scratch not printable: `00173`, `00187`, `00194`.
- `lir_to_bir` admission failures: 14 total.
  - GEP local-memory semantic family: `00143`, `00157`, `00176`, `00181`,
    `00182`, `00185`, `00195`, `00205`, `00209`.
  - Store local-memory semantic family: `00046`, `00140`.
  - Load local-memory semantic family: `00216`, `00218`.
  - Bootstrap global aggregate/array semantics gate: `00204`.
- Runtime nonzero failures from the runner line-94 path: 18 total: `00050`,
  `00066`, `00086`, `00089`, `00102`, `00111`, `00112`, `00113`, `00119`,
  `00121`, `00123`, `00137`, `00138`, `00144`, `00170`, `00179`, `00189`,
  `00200`.
- Runtime output mismatches from the runner line-98 path: 10 total: `00159`,
  `00164`, `00168`, `00169`, `00172`, `00174`, `00175`, `00186`, `00193`,
  `00196`.
- Timeout/hang bucket: 1 total: `00220`.

Closed-owner check: current evidence does not contradict closed owners 285
through 296. The previously closed fused compare-branch owner 296 remains
closed by current evidence: `00200` is still runtime-nonzero, and `00207`,
`00214`, and `00215` still fail later in the scalar add/xor immediate printer
bucket, matching the owner-296 closure boundary rather than reopening it.

## Suggested Next

Split the next focused owner before implementation. Highest-value candidate:
`lir_to_bir` local-memory admission, centered on the 9-case GEP local-memory
family (`00143`, `00157`, `00176`, `00181`, `00182`, `00185`, `00195`,
`00205`, `00209`) with nearby store/load admission cases (`00046`, `00140`,
`00216`, `00218`) as explicit boundary checks. This is the largest coherent
compile-stage semantic bucket in the refreshed inventory.

## Watchouts

- The scalar add/sub/bitwise immediate printer bucket is the next-best AArch64
  backend-printer candidate if the supervisor wants to stay in AArch64 printer
  code first; it has 6 cases and includes residuals `00207`, `00214`, and
  `00215` from owner 296.
- The scalar cast printer bucket has 7 cases but appears split between
  zero-extension width support and sign-extension source-shape support, so it
  needs a tighter focused idea than "all casts" before implementation.
- Runtime buckets are larger in aggregate, but the captured log alone does not
  identify one semantic owner; avoid splitting them until representative
  generated-code or runtime diagnostics identify a shared source.
- `00220` remains timeout/hang-only in the refreshed log and should not be used
  as evidence for any semantic owner until isolated safely.
- Do not reopen closed owners 285 through 296 from counts alone.
- Do not implement backend fixes while this umbrella inventory is active.
- Do not change expectations, allowlists, unsupported classifications, CTest
  registration, timeout policy, runner behavior, `test_after.log`, or
  `test_before.log`.

## Proof

Analyzed existing proof log only:

`/workspaces/c4c/test_after.log`

Result: Step 2 classification completed from the captured CTest output. The log
selects 352 tests and reports 290 passed / 62 failed, with all 62 failures
under `c_testsuite_aarch64_backend_*`.

No broad backend-regex rerun was performed for this packet. No narrow `c4cll`
diagnostics were needed beyond the existing CTest failure messages.

Proof log preserved: `/workspaces/c4c/test_after.log`.

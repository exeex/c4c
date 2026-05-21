Status: Active
Source Idea Path: ideas/open/295_backend_regex_failure_family_inventory.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Capture Current Backend Inventory

# Current Packet

## Just Finished

Step 1 captured the current backend inventory from the supervisor-accepted
`test_baseline.log`.

Baseline source:

- Commit: `2d52db63f06e514b4cc63ab174372cfb38a63e66`
- Subject: `Close signed remainder lowering idea`
- Scope: full-suite accepted baseline (`Baseline Regex: <full-suite>`)
- Counts: 3375 total, 3346 passed, 29 failed (`99% tests passed`)
- Backend-relevant selected labels in the full-suite summary:
  `backend` 143 tests, `backend_route` 77 tests, `aarch64_backend` 212 tests,
  `c_testsuite` 424 tests.

Failure split from `test_baseline.log`:

- Local backend/unit failures: none listed under the `backend`,
  `backend_route`, `mir`, `bir`, or `aarch64` local backend labels.
- Workflow/string authority failure: `string_authority_guard`.
- External AArch64 c-testsuite backend failures: 13 of 212
  `aarch64_backend` tests.
- External LLVM GCC torture failures: 15 `llvm_gcc_c_torture_*` tests; these
  are outside this backend c-testsuite inventory unless the supervisor widens
  the next classification packet.

Current AArch64 backend c-testsuite residuals:

- Runtime nonzero: `00112`, `00216`.
- Runtime mismatch: `00157`, `00163`, `00174`, `00176`, `00182`, `00183`,
  `00187`, `00205`, `00218`.
- Timeout quarantine candidates: `00200`, `00207`.

Recently repaired representatives are passing in the accepted full-suite
log family (`test_before.log` has the detailed per-test lines for the same
accepted run): `00123`, `00130`, `00140`, `00143`, `00195`, and `00204` all
pass on the AArch64 backend route. In particular, `00143` now passes after
idea 365 closure, and the old standalone `00220` timeout is no longer present
in the failure list.

## Suggested Next

Execute Step 2 from `plan.md`: classify the 13 current AArch64 backend
c-testsuite residuals by semantic owner before creating or selecting a focused
repair idea.

## Watchouts

Evidence gaps for Step 2:

- Need generated assembly and/or prepared dumps for the 11 non-timeout
  AArch64 residuals before assigning owners.
- Need bounded timeout handling for `00200` and `00207`; treat both as
  quarantine candidates until a safe reproducer distinguishes hang, output
  storm, or slow runtime.
- Do not reopen idea 364 or 365: `00143` passes in the accepted baseline.
- Do not fold unrelated full-suite failures into this backend inventory without
  explicit supervisor routing; `string_authority_guard` and the 15
  `llvm_gcc_c_torture_*` failures are outside the current AArch64 backend
  residual list.

## Proof

No command run. The packet explicitly used the supervisor-accepted
`test_baseline.log` as the inventory source and read `test_before.log` only for
per-test pass/failure detail where the baseline summary was compact.

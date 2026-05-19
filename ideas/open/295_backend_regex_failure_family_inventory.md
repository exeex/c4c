# Backend Regex Failure Family Inventory

Status: Open
Created: 2026-05-19

## Intent

Use the main build `ctest -R backend` result as an umbrella inventory, then
split focused repair ideas only when a failure group points to a semantic
backend capability instead of one testcase shape.

## Why This Exists

The AArch64 backend c-testsuite route is now registered inside the main
`/workspaces/c4c/build` tree, so the normal backend regex baseline includes
both local backend tests and external c-testsuite backend runtime tests.

## Current Evidence

The observed backend-regex command was:

```bash
ctest -j10 -R backend
```

Run from `/workspaces/c4c/build`, this selected 352 backend-matching tests,
including 212 `c_testsuite_aarch64_backend_*` tests. The user observed about
80 failing tests. That result is not yet a classified inventory: it may mix
unit backend regressions, AArch64 c-testsuite runtime failures, frontend
handoff failures, timeout/hang cases, and already-known residual families.

The inventory should be captured from the main build tree, not from the old
side build:

```bash
cd /workspaces/c4c/build
ctest -j10 -R backend --output-on-failure
```

The currently known registration shape is:

```text
352 tests selected by ctest -R backend
212 tests are c_testsuite_aarch64_backend_*
about 80 tests were observed failing by the user
```

The AArch64 backend c-testsuite outputs now live under the main build tree:

```text
/workspaces/c4c/build/c_testsuite_aarch64_backend/
```

This matters because `ctest -j` from `/workspaces/c4c/build` now includes the
AArch64 backend c-testsuite route in the normal baseline instead of relying on
the old `/workspaces/c4c/build-aarch64-scan` side tree.

## In Scope

- Capture a fresh `ctest -j10 -R backend --output-on-failure` log from the main
  build tree.
- Classify failures by source:
  - local backend/unit/CLI tests;
  - AArch64 c-testsuite backend runtime tests;
  - frontend or prepared-module handoff failures reached through backend tests;
  - timeout/hang or runtime-output-storm cases.
- Compare the failure list against recently closed AArch64 owners 285 through
  294 before deciding whether anything needs reopening.
- Create focused `ideas/open/*.md` files for semantic repair families.
- Switch lifecycle state to a focused idea before implementation.

## Out of Scope

- Implementing fixes inside this umbrella idea.
- Treating `ctest -R backend` as one monolithic failure bucket.
- Claiming backend progress by changing expectations, allowlists, unsupported
  classifications, CTest registration, timeout policy, or runner behavior.
- Reopening closed AArch64 owner ideas from failing counts alone.
- Matching exact c-testsuite filenames, local test names, or emitted
  instruction strings instead of identifying the semantic owner.
- Asking an executor to run broad runtime tests without timeout and stale
  process cleanup.

## Completion Criteria

- `todo.md` records a current classified `ctest -R backend` failure inventory.
- Local backend regressions are separated from AArch64 external c-testsuite
  failures.
- At least one focused repair idea is created when a tractable semantic owner
  is found, or `todo.md` explains why no owner is ready to split.
- Timeout/hang cases are quarantined, deferred, or split into a safe
  hang-specific idea.
- The active lifecycle state switches away from this umbrella idea before any
  implementation work starts.
- When a focused owner is split, completed, or rejected, this source idea gets
  a durable deactivation note that records the owner decision, proof scope, and
  remaining buckets, following the style of idea 284.

## Deactivation Note

This section is intentionally present from activation. It should accumulate
durable lifecycle notes as the umbrella inventory splits focused owners, then
reactivates for later classification passes.

Initial 2026-05-19 state:

- Main build now registers the AArch64 backend c-testsuite route directly.
- `ctest -R backend` selects 352 backend-matching tests, including 212
  `c_testsuite_aarch64_backend_*` tests.
- The user observed about 80 failing tests from `ctest -j10 -R backend`.
- The first active runbook step is to capture and classify that failure set
  from `/workspaces/c4c/build`, then split focused owner ideas before coding.

Durable inventory findings to preserve:

- This umbrella exists because the baseline surface changed: AArch64 backend
  c-testsuite is no longer a side proof under `build-aarch64-scan`; it is part
  of the main build's backend regex and normal `ctest -j` baseline.
- Any future pass-count claim must state whether it is for full main-build
  baseline, `ctest -R backend`, local backend tests only, or
  `c_testsuite_aarch64_backend_*` only.
- `ctest -R backend` is an imprecise regex selector. It includes local backend
  unit/CLI tests and external AArch64 c-testsuite runtime tests, so failures
  must be classified before repair work starts.
- Recently closed AArch64 owners 285 through 296 remain valid unless a new
  generated-code or proof artifact contradicts their closure boundary.
- Timeout/hang/runtime-output-storm cases remain environment-sensitive. Broad
  runtime scans require timeout plus stale-process cleanup before their logs are
  trusted.

Deactivation 2026-05-19 inventory result:

- The captured backend regex inventory selected 352 tests: 272 passed and 80
  failed.
- All 80 current failures are `c_testsuite_aarch64_backend_*` tests.
- Failure buckets are 38 machine-printer failures, 14 `lir_to_bir` admission
  failures, 27 runtime failures, and 1 timeout.
- Closed AArch64 owners 285 through 294 remain valid by current evidence; no
  current count alone reopens them without generated-code or proof evidence
  that contradicts their closure boundaries.
- The highest-value focused split is the 22-case fused compare-branch
  machine-printer/lowering operand-form family:
  `00030`, `00034`, `00037`, `00038`, `00041`, `00054`, `00055`, `00057`,
  `00059`, `00076`, `00077`, `00085`, `00092`, `00093`, `00101`, `00127`,
  `00200`, `00203`, `00207`, `00212`, `00214`, and `00215`.
- Implementation work should move to the focused fused compare-branch owner,
  where progress means semantic operand publication/printing for
  compare-branch forms, not filename matching or expectation/runner changes.

Focused owner closure 2026-05-19:

- Focused idea 296, `ideas/closed/296_aarch64_fused_compare_branch_operand_forms.md`,
  is closed as complete for the fused compare-branch operand-form owner.
- The accepted focused proof in `test_before.log` covers the 27-test focused
  scope and reports 23 passed / 4 failed. Earlier committed focused slices
  moved the scope from 5/27 to 23/27 overall, with the final accepted slice
  moving from 21/27 to 23/27.
- The repaired owner includes immediate-left compare operands, both-immediate
  constant compares, and non-encodable register/immediate compare operands.
  `00041` and `00203` now pass.
- Residual `00200` is a runtime mismatch and residual `00207`, `00214`, and
  `00215` are scalar add/xor immediate printer limits. They are outside the
  fused compare-branch operand-form closure boundary and should be considered
  by a later inventory pass before any new focused repair owner is split.
- The next lifecycle pass should re-inventory or classify the remaining
  backend-regex buckets without reopening closed owners 285 through 296 unless
  generated-code or proof evidence contradicts their closure boundaries.

Deactivation 2026-05-19 post-296 inventory result:

- The refreshed backend-regex inventory after closed owner 296 selected 352
  tests: 290 passed and 62 failed.
- All 62 refreshed failures are `c_testsuite_aarch64_backend_*` tests.
- Failure buckets are 19 machine-printer failures, 14 `lir_to_bir` admission
  failures, 28 runtime failures, and 1 timeout.
- Closed AArch64 owners 285 through 296 remain valid by current evidence; no
  refreshed failure contradicts their closure boundaries without separate
  generated-code or proof evidence.
- The next focused split is `lir_to_bir` local-memory admission, centered on
  the 9 GEP local-memory cases `00143`, `00157`, `00176`, `00181`, `00182`,
  `00185`, `00195`, `00205`, and `00209`, with store/load boundary checks
  `00046`, `00140`, `00216`, and `00218`.
- `00204` is preserved as a separate bootstrap global aggregate/array
  semantics gate and should not be folded into the local-memory owner without
  evidence.
- Implementation work should move to the focused local-memory admission owner,
  where progress means semantic `lir_to_bir` admission for local-memory
  GEP/store/load forms, not filename matching or expectation, unsupported,
  runner, timeout, or CTest-registration changes.

## Reviewer Reject Signals

Reject the route if it:

- fixes a named failing test without grouping the semantic owner;
- mixes backend unit failures and external AArch64 runtime failures without
  classification;
- uses expectation, allowlist, unsupported-classification, timeout, runner, or
  CTest registration changes to improve counts;
- reruns broad runtime tests without stale-process cleanup;
- reopens recently closed owners 285 through 296 without generated-code or
  proof evidence that contradicts their closure boundary.

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

The observed command was:

```bash
ctest -j10 -R backend
```

Run from `/workspaces/c4c/build`, this selected 352 backend-matching tests,
including 212 `c_testsuite_aarch64_backend_*` tests. The user observed about
80 failing tests. That result is not yet a classified inventory: it may mix
unit backend regressions, AArch64 c-testsuite runtime failures, frontend
handoff failures, timeout/hang cases, and already-known residual families.

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

## Reviewer Reject Signals

Reject the route if it:

- fixes a named failing test without grouping the semantic owner;
- mixes backend unit failures and external AArch64 runtime failures without
  classification;
- uses expectation, allowlist, unsupported-classification, timeout, runner, or
  CTest registration changes to improve counts;
- reruns broad runtime tests without stale-process cleanup;
- reopens recently closed owners 285 through 294 without generated-code or
  proof evidence that contradicts their closure boundary.

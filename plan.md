# LIR To BIR Local Memory Admission

Status: Active
Source Idea: ideas/open/297_lir_to_bir_local_memory_admission.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the semantic `lir_to_bir` admission path for local-memory address
formation and local-memory load/store boundaries reached by AArch64
c-testsuite backend cases.

## Goal

Make local-memory GEP/store/load forms admissible through `lir_to_bir` without
filename-specific shortcuts or changes to expectations, runner behavior,
unsupported classifications, timeout policy, allowlists, or CTest
registration.

## Core Rule

Progress must be a semantic local-memory admission repair. Do not claim
progress through named c-testsuite cases, emitted-diagnostic matching,
expectation rewrites, unsupported downgrades, runner changes, timeout policy,
allowlists, or CTest-registration changes.

## Read First

- `ideas/open/297_lir_to_bir_local_memory_admission.md`
- `ideas/open/295_backend_regex_failure_family_inventory.md`
- `todo.md`
- Current focused evidence in `test_after.log`

## Current Targets

- Local-memory GEP admission cases: `00143`, `00157`, `00176`, `00181`,
  `00182`, `00185`, `00195`, `00205`, `00209`.
- Store local-memory boundary checks: `00046`, `00140`.
- Load local-memory boundary checks: `00216`, `00218`.
- Focused proof should use the supervisor-selected backend c-testsuite subset
  for these cases plus build or compile proof appropriate to the touched code.

## Non-Goals

- Do not absorb `00204`; it is a separate bootstrap global aggregate/array
  semantics gate unless new evidence proves the same local-memory rule owns it.
- Do not repair unrelated runtime mismatch, timeout/hang, or machine-printer
  failures from the umbrella inventory.
- Do not broaden into frontend, HIR, LIR, BIR, runtime, or AArch64 printer
  rewrites beyond what local-memory `lir_to_bir` admission requires.
- Do not weaken any test contract or move a supported path to unsupported.

## Working Model

The refreshed post-296 backend-regex inventory has 62 failures, all AArch64
c-testsuite backend tests. The 14-case `lir_to_bir` admission bucket contains a
coherent local-memory family: 9 GEP cases plus store/load boundary checks.
`00204` remains outside this owner as a global aggregate/array bootstrap gate.

## Execution Rules

- Start with diagnosis of the admission rejection path before editing.
- Generalize from local-memory GEP/store/load semantics, not from c-testsuite
  filenames.
- Keep residuals classified by failure source if a focused case moves past
  `lir_to_bir` admission into another backend failure.
- Preserve canonical proof logs as `test_before.log` and `test_after.log`;
  do not leave extra root-level `.log` files.
- Escalate back to lifecycle review if evidence shows this owner is actually a
  global aggregate/array bootstrap issue, runtime issue, timeout issue, or
  machine-printer issue.

## Steps

### Step 1: Isolate The Admission Failure

Inspect representative local-memory GEP, store, and load failures to identify
the exact `lir_to_bir` rejection path and the IR shape it rejects.

Completion check: `todo.md` records the observed rejection, representative
cases, and the semantic local-memory form the implementation must admit.

### Step 2: Repair Local-Memory GEP Admission

Implement the narrow semantic admission change for GEP-derived local-memory
addresses, using existing lowering and BIR representation patterns where
possible.

Completion check: the focused GEP cases either pass or move past the old
`lir_to_bir` local-memory rejection into clearly classified later failures.

### Step 3: Check Store/Load Boundaries

Run the store/load boundary checks against the same admission rule and repair
only local-memory admission gaps that share the focused semantic owner.

Completion check: `00046`, `00140`, `00216`, and `00218` are either passing,
classified as fixed by the same rule, or documented as residual failures
outside this owner.

### Step 4: Prove The Focused Owner

Run the supervisor-selected focused backend c-testsuite subset plus fresh build
or compile proof for the touched code. Escalate to a broader backend check if
the implementation touches shared `lir_to_bir` behavior beyond local-memory
admission.

Completion check: proof results are recorded in `todo.md`, no forbidden
expectation/unsupported/runner/timeout/allowlist/CTest changes are present, and
residual focused failures are classified by their current failure source.

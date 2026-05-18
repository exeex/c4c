# AArch64 C-Testsuite Call-Boundary Move Lowering Runbook

Status: Active
Source Idea: ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md

## Purpose

Repair the AArch64 backend failure family where c-testsuite backend-route cases
reach call-boundary move lowering or selection and then surface at the machine
node printer as `deferred_unsupported`.

This is backend capability work. It must reduce the truthful call-boundary move
failure family without claiming runtime pass evidence on a host that lacks an
AArch64 runner.

## Goal

Add general AArch64 lowering or selection support for the missing
call-boundary move forms and prove the targeted failure family moves past the
old `deferred_unsupported` printer failure.

## Core Rule

Fix call-boundary move semantics, not c-testsuite case names. Do not weaken
expectations, allowlists, unsupported classifications, or diagnostics to claim
progress.

## Read First

- `ideas/open/277_aarch64_c_testsuite_call_boundary_move_lowering.md`
- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- Latest AArch64 backend scan evidence in `test_after.log`, if present
- AArch64 backend lowering, selection, ABI, and machine-node printing code
- Focused AArch64 backend tests under `tests/backend/`
- c-testsuite AArch64 backend registration and runner files under
  `tests/c/external/`

## Current Scope

- Identify representative c-testsuite cases that hit call-boundary move
  lowering or selection and print as `deferred_unsupported`.
- Trace the failing path from BIR/LIR facts through AArch64 call argument or
  result boundary handling.
- Implement a general lowering or selection rule for the missing move forms.
- Preserve stage-specific route diagnostics.
- Prove with focused backend tests and the relevant AArch64 c-testsuite backend
  subset.

## Non-Goals

- Do not configure an AArch64 runtime runner.
- Do not claim `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not edit c-testsuite expected outputs or allowlists.
- Do not add filename-specific or exact-shape printer shortcuts.
- Do not broadly rewrite unrelated AArch64 instruction selection, ABI, or
  frontend lowering.
- Do not hide missing frontend or BIR facts inside target-specific printer
  code.

## Working Model

- The parked runtime-route idea `276` already registered the AArch64 backend
  c-testsuite route and showed that runtime execution is blocked on this host
  without `C_TESTSUITE_AARCH64_BACKEND_RUNNER`.
- This plan owns only non-runtime backend capability failures from that route.
- A successful packet may reclassify a case to assembler/link, runtime
  unavailable, runtime nonzero, or runtime mismatch if that is the next
  truthful owner layer.
- A successful packet must not collapse backend, assembler/link, and runtime
  failures into one bucket.

## Execution Rules

- Keep implementation packets narrow and tied to one lowering path.
- Prefer existing AArch64 backend helpers and local test patterns before adding
  abstractions.
- When a c-testsuite case is used as evidence, also add or update a focused
  backend test that exercises the semantic move form directly.
- Preserve or improve diagnostics that distinguish frontend, backend,
  assembler/link, runtime unavailable, runtime nonzero, and runtime mismatch.
- Use `todo.md` for packet progress and proof results; do not rewrite this
  runbook for routine executor progress.

## Ordered Steps

### Step 1: Identify Representative Failure Cases

Goal: find a small, representative set of AArch64 backend c-testsuite failures
whose old owner is call-boundary move lowering or selection reaching
`deferred_unsupported`.

Actions:
- Inspect recent AArch64 backend scan output if available.
- If needed, run the relevant AArch64 backend c-testsuite subset far enough to
  reproduce the backend failures without requiring runtime pass evidence.
- Capture representative source cases, failure messages, and the failing
  machine-node or lowering form.
- Separate frontend/BIR failures from backend call-boundary move failures.

Completion check:
- `todo.md` names representative cases, the observed failing form, and the
  narrow proof command for the first implementation packet.

### Step 2: Trace The Owned Lowering Path

Goal: identify the exact target-owned path that should lower or select the
missing call-boundary move forms.

Actions:
- Trace the representative failure from BIR/LIR inputs into AArch64 ABI,
  call-boundary move materialization, instruction selection, and machine-node
  printing.
- Confirm whether the missing support belongs in lowering, selection,
  register-class handling, or printer coverage for an already-valid node.
- Identify any missing source facts that would make the issue frontend-owned
  instead of backend-owned.

Completion check:
- The next packet can name the owned files and rule to change without relying
  on c-testsuite filename matching.

### Step 3: Add Focused Backend Proof

Goal: create or update focused backend tests that cover the missing semantic
call-boundary move form directly.

Actions:
- Add the smallest backend test that reproduces the failing call-boundary move
  form outside the c-testsuite harness.
- Keep expected output or diagnostics strict enough to prevent the same
  `deferred_unsupported` path from hiding behind renamed output.
- Run the focused backend test before and after implementation when practical.

Completion check:
- There is a focused backend proof that fails for the old missing move form and
  can pass only when the semantic lowering/selection path is repaired.

### Step 4: Implement General Move Lowering Or Selection

Goal: repair the missing call-boundary move capability with a general
AArch64 backend rule.

Actions:
- Implement the smallest general lowering or selection change for the missing
  move forms.
- Reuse existing AArch64 move materialization, register-class, and ABI helpers
  where they fit.
- Preserve stage-specific diagnostics for failures that still belong to later
  stages.
- Avoid broad rewrites of unrelated call lowering or instruction selection.

Completion check:
- The focused backend proof passes, and representative old failures no longer
  reach the printer as `deferred_unsupported`.

### Step 5: Prove Against The AArch64 C-Testsuite Backend Subset

Goal: confirm the targeted c-testsuite failure family is reduced or moves to
the next truthful owner layer.

Actions:
- Run the relevant AArch64 c-testsuite backend subset that contains the
  representative call-boundary move cases.
- Compare the targeted failures against the Step 1 baseline.
- Record whether each case now passes compilation, reaches assembler/link,
  reaches `[RUNTIME_UNAVAILABLE]`, or exposes a different truthful failure.
- Confirm no runtime-unavailable case is counted as a pass.

Completion check:
- `todo.md` records the targeted family reduction or reclassification and the
  exact proof command/output path.

### Step 6: Broader Backend Scan Checkpoint

Goal: make sure the repair did not merely overfit the focused cases and did
not degrade the registered AArch64 backend route.

Actions:
- Run a broader AArch64 backend c-testsuite label/regex selected by the
  supervisor.
- Summarize counts by failure stage, with attention to remaining
  `deferred_unsupported` call-boundary move cases.
- Preserve representative follow-up failures for separate ideas if a new
  capability family appears.

Completion check:
- The supervisor has enough evidence to accept the capability slice, request a
  reviewer route check, or split a distinct follow-up idea.

# AArch64 C-Testsuite Undefined Temporary Labels Runbook

Status: Active
Source Idea: ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md

## Purpose

Repair the AArch64 backend failure family where c-testsuite backend-route cases
emit references to `.LBB...` temporary labels that are absent from the generated
assembly.

This is backend capability work. It must reduce the truthful undefined-label
failure family without claiming runtime pass evidence on a host that lacks an
AArch64 runner.

## Goal

Add a general AArch64 block-label, branch-target, or assembly-emission rule so
referenced temporary labels are defined in generated assembly, then prove the
targeted c-testsuite backend failures move past the undefined-symbol blocker or
reclassify to the next truthful owner layer.

## Core Rule

Fix label authority and emission semantics, not c-testsuite case names or exact
temporary-label spellings. Do not weaken expectations, allowlists, unsupported
classifications, or diagnostics to claim progress.

## Read First

- `ideas/open/278_aarch64_c_testsuite_undefined_temporary_labels.md`
- `ideas/open/276_aarch64_c_testsuite_backend_runtime_execution.md`
- Latest AArch64 backend scan evidence in `test_after.log`, if present
- AArch64 backend block, branch, label, and assembly emission code
- Shared MIR/BIR/LIR label handoff code if the AArch64 backend consumes labels
  from a common lowering path
- Focused AArch64 backend tests under `tests/backend/`
- c-testsuite AArch64 backend registration and runner files under
  `tests/c/external/`

## Current Scope

- Identify representative c-testsuite cases that reference undefined `.LBB...`
  temporary labels.
- Trace whether missing label definitions originate in BIR/LIR block labeling,
  prepared handoff, AArch64 branch lowering, or assembly emission.
- Implement a general rule for defining labels that are referenced by emitted
  AArch64 control-flow instructions.
- Preserve stage-specific route diagnostics.
- Prove with focused backend tests and the relevant AArch64 c-testsuite backend
  subset.

## Non-Goals

- Do not configure an AArch64 runtime runner.
- Do not claim `[RUNTIME_UNAVAILABLE]` as pass evidence.
- Do not reopen the completed call-boundary move lowering route unless the
  exact old diagnostic returns.
- Do not edit c-testsuite expected outputs, allowlists, unsupported
  classifications, or stage-specific diagnostics to make failures disappear.
- Do not add filename-specific label definitions or exact `.LBB` spelling
  shortcuts.
- Do not broadly rewrite unrelated AArch64 ABI, call lowering, instruction
  selection, or frontend paths.
- Do not hide missing frontend or BIR facts inside target-specific printer
  code.

## Working Model

- The parked runtime-route idea `276` remains blocked until an AArch64 host or
  `C_TESTSUITE_AARCH64_BACKEND_RUNNER` is available.
- The completed call-boundary move route removed the old owned
  `deferred_unsupported` diagnostic from the broad AArch64 backend scan.
- This plan owns the separate non-runtime backend family where assembler/link
  or backend validation reports references to undefined `.LBB...` temporary
  labels.
- A successful packet may reclassify a case to assembler/link, runtime
  unavailable, runtime nonzero, runtime mismatch, or a different backend
  failure if that is the next truthful owner layer.
- A successful packet must not collapse backend, assembler/link, and runtime
  failures into one bucket.

## Execution Rules

- Keep implementation packets narrow and tied to one label-authority or
  emission path.
- Prefer existing AArch64 backend helpers and local test patterns before adding
  new abstractions.
- When a c-testsuite case is used as evidence, also add or update a focused
  backend test that exercises the semantic label-reference path directly.
- Preserve or improve diagnostics that distinguish frontend, backend,
  assembler/link, runtime unavailable, runtime nonzero, and runtime mismatch.
- Use `todo.md` for packet progress and proof results; do not rewrite this
  runbook for routine executor progress.

## Ordered Steps

### Step 1: Identify Representative Undefined-Label Cases

Goal: find a small, representative set of AArch64 backend c-testsuite failures
that reference `.LBB...` temporary labels without defining them.

Actions:
- Inspect recent AArch64 backend scan output if available.
- If needed, run the relevant AArch64 backend c-testsuite subset far enough to
  reproduce the undefined-label failures without requiring runtime pass
  evidence.
- Capture representative source cases, referenced missing labels, failure
  stage, and the exact proof command.
- Separate undefined temporary label failures from runtime-unavailable cases,
  scalar printer gaps, semantic `lir_to_bir` gaps, and unrelated backend
  failures.

Completion check:
- `todo.md` names representative cases, the missing-label symptom, and the
  narrow proof command for the first implementation packet.

### Step 2: Trace Label Ownership

Goal: identify the exact layer that owns each referenced temporary label and
where the corresponding definition should be emitted.

Actions:
- Trace representative failures from BIR/LIR or prepared-module block facts
  through AArch64 branch lowering and assembly emission.
- Confirm whether the missing support belongs in common block labeling,
  prepared handoff, AArch64 branch target materialization, or printer/emitter
  output.
- Identify any missing source facts that would make the issue frontend-owned
  instead of backend-owned.
- Record the owned files and the intended rule in `todo.md` before changing
  implementation files.

Completion check:
- The next packet can name the general label rule to change without relying on
  c-testsuite filename matching or exact generated label strings.

### Step 3: Add Focused Backend Proof

Goal: create or update focused backend tests that cover the missing
label-reference path directly.

Actions:
- Add the smallest backend test that reproduces a branch or block-label
  reference whose definition is absent on the old path.
- Keep expected output strict enough to prove that every referenced temporary
  label has a matching emitted definition.
- Run the focused backend test before and after implementation when practical.

Completion check:
- There is a focused backend proof that fails for the old missing-label path
  and can pass only when label authority or emission is repaired.

### Step 4: Implement General Label Definition Repair

Goal: repair the missing temporary-label capability with a general backend
rule.

Actions:
- Implement the smallest general change in block-label ownership, branch-target
  lowering, prepared handoff, or assembly emission that defines referenced
  labels.
- Reuse existing AArch64 or shared MIR label helpers where they fit.
- Preserve stage-specific diagnostics for failures that still belong to later
  stages.
- Avoid broad rewrites of unrelated call lowering, ABI, instruction selection,
  or frontend lowering.

Completion check:
- The focused backend proof passes, and representative old failures no longer
  produce undefined `.LBB...` temporary label references.

### Step 5: Prove Against The AArch64 C-Testsuite Backend Subset

Goal: confirm the targeted c-testsuite undefined-label failure family is
reduced or moves to the next truthful owner layer.

Actions:
- Run the relevant AArch64 c-testsuite backend subset that contains the
  representative undefined-label cases.
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
- Summarize counts by failure stage, with attention to remaining undefined
  temporary label cases.
- Preserve representative follow-up failures for separate ideas if a new
  capability family appears.

Completion check:
- The supervisor has enough evidence to accept the capability slice, request a
  reviewer route check, or split a distinct follow-up idea.

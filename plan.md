# Post-Wave Residual Baseline Failures Runbook

Status: Active
Source Idea: ideas/open/675_post_wave_residual_baseline_failures.md

## Purpose

Resume post-wave residual reconciliation after the focused 676 and 677
follow-ups settled the two new-only RV64 backend rows.

## Goal

Classify the remaining backend residuals, preserve the baseline policy
boundary, and generate focused follow-up work only when a first owner is proven.

## Core Rule

Do not accept `test_baseline.new.log`, rewrite expectations, or treat row-count
improvement as progress until residual policy is settled by stable test name.

## Read First

- `ideas/open/675_post_wave_residual_baseline_failures.md`
- `build/agent_state/675_step1_candidate_delta/summary.md`
- `ideas/closed/676_rv64_pointer_global_local_publication_runtime_contract.md`
- `ideas/closed/677_rv64_call_arg_local_frame_address_object_materialization.md`
- `test_baseline.log`
- `test_baseline.new.log`

## Current Targets

- Immediate backend residual:
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- Parent residual policy:
  reconcile persistent common failures and decide whether a fresh candidate is
  monotonic or explicitly rejected.
- Baseline status:
  `test_baseline.new.log` remains unaccepted.

## Non-Goals

- Do not reopen closed ideas 676 or 677 without fresh evidence that their
  closure notes are wrong.
- Do not accept `test_baseline.new.log` in this runbook until the remaining
  candidate/common residual policy is settled.
- Do not edit unsupported markers, allowlists, timeouts, runtime policy, or
  baseline accounting.
- Do not merge RV64 CLI route fixes, AArch64 publication, dump-contract rows,
  and LLVM torture rows into one implementation owner without evidence of a
  shared first owner.

## Working Model

Idea 675 split the two new-only RV64 CLI rows into focused ideas 676 and 677.
Both are closed. Broad backend validation now leaves only the AArch64
prepared-BIR publication row failing in backend scope, but the source idea also
tracks persistent dump and LLVM residuals from the accepted/candidate baseline
comparison. The next route is reconciliation: prove the current residual set by
stable test name, classify the remaining backend row's first owner, and then
decide whether to create a focused follow-up or continue to baseline policy.

## Execution Rules

- Compare residuals by stable test name, not numeric row id.
- Treat 676 and 677 closure notes as settled unless fresh evidence contradicts
  their proof.
- Prefer classification and follow-up generation over broad implementation when
  the first owner is not yet proven.
- Compare baseline rows by stable test name whenever referencing broad logs.
- Preserve diagnostic evidence under `build/agent_state/675_*` if new proof
  artifacts are created.

## Steps

### Step 1: Reconcile The Post-676/677 Residual Set

Goal: Establish the current backend and baseline residual state after both
focused RV64 follow-ups closed.

Actions:

- Read the 676 and 677 closure notes.
- Compare `test_baseline.log`, `test_baseline.new.log`, and current backend
  proof by stable test name.
- Confirm whether
  `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
  is the only remaining backend failure.
- Preserve a compact reconciliation summary if new evidence is gathered.

Completion Check:

- The current residual set is listed by stable test name, and the settled 676
  and 677 rows are not treated as open work.

### Step 2: Classify The AArch64 Publication Row

Goal: Decide the first owner for the remaining backend failure without folding
unrelated persistent rows into it.

Actions:

- Inspect the failing AArch64 prepared-BIR publication row and relevant prior
  closure notes from ideas 668, 671, and 672.
- Identify whether the owner is prepared dump publication, AArch64 lowering,
  stale test contract, or a separate policy issue.
- Do not weaken the test contract or mark it unsupported as classification.

Completion Check:

- The row has a named first owner with evidence, or the missing evidence is
  precisely documented.

### Step 3: Route The Remaining Work

Goal: Decide whether 675 can continue directly, should create a focused
follow-up idea, or should reject the current baseline candidate.

Actions:

- If the AArch64 row has a proven focused owner, create or request activation
  of a focused follow-up idea with reviewer reject signals.
- If the row can be resolved within 675 without implementation, document the
  classification and required proof.
- Revisit the persistent dump and LLVM residuals only after the backend row is
  classified.
- Keep `test_baseline.new.log` rejected unless a fresh candidate is monotonic
  against the accepted baseline.

Completion Check:

- The next lifecycle route is explicit: focused follow-up, continued 675
  reconciliation, or baseline-candidate rejection with preserved evidence.

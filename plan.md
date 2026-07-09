# Prepared Branch Stack Clobber-Safety Authority Runbook

Status: Active
Source Idea: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md

## Purpose

Publish or validate explicit clobber-safety authority for branch stack-load
operands whose source freshness has already been selected.

## Goal

Move multiple already-selected branch stack-source rows past
`missing_stack_clobber_safety`, or reclassify them to a more precise owner
with current evidence.

## Core Rule

Branch stack-load admission must keep selected source freshness separate from
clobber-safety authority. RV64 must not infer branch stack-load safety from
frame homes, offsets, final assembly shape, or source filenames.

## Read First

- `ideas/open/635_prepared_branch_stack_clobber_safety_authority.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/closed/615_branch_stack_source_residual_audit.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`

## Current Targets

- Selected branch stack-source rows that still stop at
  `missing_stack_clobber_safety`.
- Representative sources: `src/20001017-1.c`, `src/loop-2e.c`,
  `src/pr39100.c`, `src/20000314-3.c`, `src/20140828-1.c`,
  `src/20080519-1.c`, and `src/20050125-1.c`.
- Prepared-layer evidence proving the selected stack source is not invalidated
  by intervening clobbers at the branch use point.
- Fail-closed diagnostics for missing, stale, contradictory, or unrelated
  clobber-safety evidence.

## Non-Goals

- RV64 fallback inference from stack homes, frame offsets, filenames, final
  assembly, or apparent no-clobber code shape.
- Branch stack-source freshness publication for no-candidate rows; that
  belongs to idea `636`.
- Select publication, generic terminator lowering, ABI, runtime,
  expectations, unsupported markers, allowlists, timeouts, or accounting.
- Any named-case shortcut for one representative source file.

## Working Model

- The in-scope family already has selected source freshness. The missing
  boundary is independent clobber-safety authority at the branch point.
- Evidence refresh comes first. Do not choose a producer or consumer edit until
  the current rows and first owners are confirmed.
- Prepared evidence should name the selected stack source and the branch use
  point strongly enough for consumers to reject stale, missing, or
  contradictory safety facts.
- If fresh probes prove a row is not a clobber-safety authority gap, classify
  it to the precise existing owner or recommend a lifecycle split.

## Execution Rules

- Keep routine packet progress in `todo.md`.
- Do not edit the source idea unless durable intent changes or closure notes
  are required.
- Prefer prepared-layer semantic authority over RV64 target-local inference.
- Add focused positive and negative tests for code-changing authority or
  consumer-admission packets.
- Use `cmake --build --preset default` plus a supervisor-selected focused
  backend or prepared-layer subset as the normal proof ladder for code slices.
- Treat expectation rewrites, unsupported-marker changes, filename matching,
  source-shape matching, stack-offset inference, final-assembly inference, and
  diagnostic-only changes as route failures.

## Step 1: Refresh Prepared Branch Clobber-Safety Evidence

Goal: confirm the current selected branch stack-source rows and their first
clobber-safety owner diagnostics.

Actions:
- Re-run focused probes for the seven representative branch stack-source rows.
- Capture source freshness status, selected candidate identity, branch use
  point, first owner diagnostic, and any visible clobber-safety facts.
- Separate in-scope selected-freshness rows from no-candidate freshness rows,
  select publication gaps, generic terminator gaps, unrelated ABI/runtime
  failures, and accounting-only cases.
- Record row evidence and the recommended next owner bucket in `todo.md`.

Completion check:
- `todo.md` lists the rows inspected, selected freshness facts, current
  clobber-safety evidence, first owner, and a next step that is not
  named-case-only.

## Step 2: Trace Prepared Clobber-Safety Authority

Goal: identify where prepared branch stack-load evidence should publish
clobber-safety authority for the selected stack source.

Actions:
- Trace selected branch stack-source construction for the Step 1 in-scope
  bucket.
- Locate the carrier fields or helper boundaries that already hold, or should
  hold, selected source identity, branch use point, intervening clobber
  analysis, and safety status.
- Identify the first missing, stale, contradictory, or unconsumed authority
  boundary before RV64 branch emission.
- Record fail-closed states for missing selected freshness, absent safety
  evidence, stale safety evidence, contradictory clobber evidence, and
  mismatched branch source identity.

Completion check:
- `todo.md` names the exact producer functions, carrier fields, consumer
  checks, and the smallest code-changing packet that can publish, verify, or
  consume branch stack-load clobber-safety authority.

## Step 3: Publish Or Validate Prepared Safety Evidence

Goal: make clobber-safety authority explicit before RV64 consumes selected
branch stack-load operands.

Actions:
- Add or tighten prepared-layer evidence that binds the selected stack source
  to the branch use point and proves no intervening clobber invalidates it.
- Preserve fail-closed behavior when safety evidence is absent, stale,
  contradictory, or tied to a different selected source.
- Add focused negative coverage for rows without selected freshness, without
  clobber-safety evidence, or with contradictory clobber evidence.
- Keep no-candidate freshness publication and select publication out of this
  packet.

Completion check:
- Focused prepared-layer tests pass, and the prepared contract distinguishes
  selected source freshness from independent clobber-safety authority.

## Step 4: Admit Only Explicitly Safe Branch Stack Loads

Goal: let RV64 branch stack-load consumption proceed only when selected
freshness and clobber-safety authority both match the branch use point.

Actions:
- Replace the relevant `missing_stack_clobber_safety` rejection only for rows
  with explicit matching safety authority.
- Validate selected freshness, selected source identity, branch use point, and
  clobber-safety evidence before branch stack-load emission.
- Preserve precise rejection for absent, stale, contradictory, mismatched, or
  unrelated evidence.
- Keep stack-home, offset, filename, final-assembly, and named-source
  inference out of the consumer.

Completion check:
- Focused positive and negative backend tests pass, and RV64 rejects branch
  stack loads without explicit selected freshness plus clobber-safety
  authority.

## Step 5: Reclassify Residual Branch Stack-Source Rows

Goal: decide whether idea `635` is complete, needs another clobber-safety
packet, or should split remaining work into separate initiatives.

Actions:
- Re-run the selected branch stack-source probes after any Step 2, Step 3, or
  Step 4 changes.
- Classify each remaining failure into clobber-safety authority, no-candidate
  freshness, select publication, terminator lowering, RV64 consumer admission,
  or another precise owner bucket.
- Record whether idea `635` is close-ready or which next clobber-safety packet
  is justified.

Completion check:
- `todo.md` contains row-by-row classification, proof results, and a clear
  close/split/continue recommendation for the supervisor.

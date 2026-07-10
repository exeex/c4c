# Prepared Backend Contract And CLI Publication Plan

Status: Active
Source Idea: ideas/open/662_prepared_backend_contract_and_cli_publication.md

## Purpose

Turn the prepared backend contract and CLI publication follow-up into an
execution runbook that refreshes evidence, names the first owner, and repairs
only the proven prepared contract-production, printer, or CLI exposure rule.

## Goal

Repair internal prepared backend contract publication and CLI exposure for
liveness, frame/stack call contracts, prepared printing, prealloc inline asm,
and prepared-BIR dump contract rows without absorbing RV64 runtime, RISC-V
object-emission, or AArch64 instruction-dispatch work.

## Core Rule

Prove whether each failure is owned first by prepared contract production,
printer formatting over an existing contract, CLI section exposure, or prealloc
inline-asm contract publication before changing backend behavior. Do not claim
capability progress through expectation churn, unsupported-marker downgrades,
diagnostic wording changes, or testcase identity.

## Read First

- `ideas/open/662_prepared_backend_contract_and_cli_publication.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `ideas/open/665_aarch64_instruction_dispatch_internal.md`
- `ideas/open/664_riscv_object_emission_internal_probe.md`

## Current Targets

Focused rows from the current backend baseline:

- `backend_prepare_liveness`
- `backend_prepare_frame_stack_call_contract`
- `backend_prepared_printer`
- `backend_prealloc_inline_asm`
- `backend_cli_dump_prepared_bir_exposes_contract_sections`
- `backend_cli_dump_prepared_bir_local_arg_call_contract`

Nearby rows to keep out of this route unless proof shows prepared contract
publication is first owner:

- `backend_aarch64_instruction_dispatch`
- `backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff_aarch64_publication`
- `backend_riscv_object_emission`

Keep RV64 pointer-local, byval, object-data static storage, destination
publication, callee-saved GPR, packed-member, and LLVM torture rows outside
this plan.

## Non-Goals

- Do not repair RV64 runtime lowering for pointer-local, byval, object-data,
  destination-publication, callee-saved GPR, or packed-member rows here.
- Do not implement RISC-V object emission or AArch64 instruction dispatch in
  this route.
- Do not perform LLVM torture owner discovery.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or test classification.
- Do not treat CLI text-only rewrites, helper renames, diagnostic relabeling,
  or classification-only edits as capability progress.

## Working Model

The active family has six focused internal prepared/CLI rows:

1. prepared/prealloc liveness
2. prepared frame/stack call contract
3. prepared printer
4. prealloc inline asm
5. CLI prepared-BIR contract-section exposure
6. CLI prepared-BIR local-arg call contract exposure

The first owner must be proven among prepared contract production, prepared
printer formatting, CLI section exposure, prealloc inline-asm publication, or a
smaller split. Implementation may start only after evidence identifies one
general boundary, or after `todo.md` records a split with concrete proof for
supervisor lifecycle routing.

## Execution Rules

- Keep routine evidence, packet state, and proof notes in `todo.md`.
- If a row belongs first to AArch64 dispatch, RISC-V object emission, RV64
  runtime lowering, or LLVM torture owner discovery, stop and recommend
  lifecycle routing instead of broadening this plan.
- For each code-changing step, run build proof plus the supervisor-selected
  focused prepared/CLI subset. Broader backend regression belongs to the
  supervisor acceptance pass.
- Preserve fail-closed diagnostics for missing, ambiguous, or stale prepared
  contract facts.
- Reject patches whose main effect is CLI wording churn, named-case matching,
  expectation edits, unsupported-marker downgrades, or preserving the same
  missing contract section behind a new helper name.

## Reviewer Reject Signals

- Reject treating CLI text-only expectation rewrites as capability progress.
- Reject merging target-specific AArch64 dispatch, RISC-V object emission, or
  RV64 runtime lowering into this contract-publication route without proof
  that prepared contract publication is first owner.
- Reject helper renames, diagnostic wording changes, classification-only
  edits, unsupported-marker downgrades, allowlist edits, or timeout changes
  claimed as progress.
- Reject named-case matching for prepared liveness, frame/stack call
  contracts, prepared printer, inline asm, or CLI dump rows.
- Reject leaving the same missing contract section or stale prepared-BIR
  exposure behind a new abstraction name.

## Ordered Steps

### Step 1: Refresh Prepared Contract Evidence

Goal: Reproduce the current failure boundaries for the six focused
prepared/CLI rows.

Primary targets:

- `tests/backend/CMakeLists.txt`
- prepared-BIR, CLI dump, backend printer, prealloc, and internal contract
  artifacts generated by the focused tests
- current diagnostics or missing snippets for the six focused rows

Actions:

- Run the focused prepared/CLI rows and collect the first failing diagnostic,
  missing snippet, or stale exposure for each row.
- Refresh semantic BIR, prepared-BIR, printer, CLI, and contract-publication
  evidence where the failure does not already name the owner.
- Distinguish prepared contract production from printer formatting, CLI
  section exposure, prealloc inline-asm publication, and unrelated target
  backend owners.
- Record whether each row is owned first by a prepared contract/CLI boundary
  or by a smaller split that should be routed separately.

Completion check:

- `todo.md` records a focused evidence summary naming the first owner or split
  for every targeted row, with no implementation changes required for this
  step.

### Step 2: Select The First Repair Boundary

Goal: Choose one general prepared contract-publication or CLI exposure boundary
for the first implementation packet.

Primary targets:

- the prepared producer, printer, CLI, or prealloc surface proven by Step 1
- focused backend cases that prove a positive shape and a fail-closed negative
  shape for that boundary

Actions:

- Select exactly one boundary from the Step 1 evidence.
- Define the positive and negative evidence the executor must preserve.
- If the six rows need separate owners, keep only the first prepared/CLI owner
  in this runbook and record separate owner candidates in `todo.md` for
  supervisor lifecycle routing.
- If the selected boundary belongs to AArch64 dispatch, RISC-V object emission,
  RV64 runtime lowering, or LLVM torture discovery, stop before implementation
  and return a lifecycle-routing recommendation.

Completion check:

- `todo.md` identifies one selected implementation boundary, the rows it owns,
  and the focused proof subset for the first repair packet.

### Step 3: Repair The Selected Contract Or CLI Rule

Goal: Implement one general prepared contract-publication, printer, prealloc,
or CLI exposure rule that fixes or fails closed at the proven owner.

Primary targets:

- the source files named by Step 1 and Step 2 evidence
- the focused backend prepared/CLI cases for the selected boundary

Actions:

- Repair publication, formatting, or exposure of the proven prepared contract
  facts.
- Preserve explicit owner evidence in prepared dumps, CLI dumps, or fail-closed
  diagnostics.
- Add or update focused positive and negative tests only when they prove the
  general rule rather than the named baseline row.
- Keep rows outside the selected owner fail-closed with precise diagnostics.

Completion check:

- The selected focused subset passes or fails closed at the proven owner.
- Build proof is fresh.
- No unrelated backend family changes are included.

### Step 4: Broaden Within The Prepared/CLI Family

Goal: Prove that the repair did not regress nearby prepared/CLI rows and decide
whether another contract-publication packet remains.

Primary targets:

- all six current prepared backend contract and CLI rows
- nearby AArch64 and RISC-V object-emission rows only as boundary checks, not
  implementation scope

Actions:

- Re-run the full focused prepared/CLI subset after the selected repair.
- Check whether remaining failures belong to the same selected owner, a
  separate prepared/CLI owner, or a different initiative.
- Record any remaining owner split in `todo.md` without mutating the source
  idea unless a separate initiative is required.

Completion check:

- `todo.md` records proof for the focused family and a clear next packet,
  lifecycle-routing recommendation, or close recommendation for the supervisor.

### Step 5: Supervisor Acceptance Checkpoint

Goal: Hand back a coherent implementation slice with enough proof for
supervisor review.

Actions:

- Ensure the final executor proof includes build plus the delegated focused
  CTest subset.
- Leave canonical broad regression-log decisions to the supervisor.
- Do not close the source idea unless all acceptance criteria in
  `ideas/open/662_prepared_backend_contract_and_cli_publication.md` are
  satisfied.

Completion check:

- The active plan has an executor-updated `todo.md` with current proof,
  watchouts, and either the next selected packet or a source-idea completion
  recommendation.

# RV64 Prepared Destination Publication Plan

Status: Active
Source Idea: ideas/open/661_rv64_prepared_destination_publication.md

## Purpose

Turn the prepared destination follow-up into an execution runbook that refreshes
evidence, names the first owner, and repairs only the proven RV64/prepared
destination publication or consumption rule.

## Goal

Repair RV64 prepared destination publication and consumption for parameter
homes, scalar frame-slot destinations, call-result predicates, and
function-pointer return-chain destinations without reopening stack-destination
fan-in authority from ideas 647 or 655.

## Core Rule

Prove the source fact, destination/home fact, and consumer point before changing
RV64 lowering or prepared publication. Do not infer authority from final
assembly, source filename, source order, value id, or testcase identity.

## Read First

- `ideas/open/661_rv64_prepared_destination_publication.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`

## Current Targets

Focused rows from the current backend baseline:

- `backend_dump_riscv64_stack_passed_parameter_home_publication`
- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

Nearby proof surface when a repair touches consumers:

- `backend_codegen_route_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_codegen_route_riscv64_function_pointer_return_chain`
- `backend_rv64_runtime_riscv64_function_pointer_return_chain`

Keep pointer-local, byval, object-data static storage, RISC-V object emission,
AArch64, callee-saved GPR, packed-member, prepared CLI, and LLVM torture rows
outside this plan.

## Non-Goals

- Do not reopen ordered final-state, mutual-exclusion, explicit-merge, or
  rejection authority for stack-destination fan-in from ideas 647 and 655.
- Do not repair RV64 pointer-local postincrement, byval aggregate, object-data
  static storage, callee-saved GPR, packed member offset, AArch64, CLI, or
  LLVM torture work here.
- Do not change expectations, unsupported markers, allowlists, timeout policy,
  runtime policy, baseline accounting, or test classification.
- Do not infer destination authority from final assembly, diagnostics, filename,
  value id, source order, or testcase identity.

## Working Model

The active family has four blocked dump rows:

1. stack-passed parameter-home publication
2. scalar compare frame-slot destination
3. prepared fused compare call-result predicate
4. function-pointer return-chain destination

The first owner must be proven among prepared destination publication,
parameter-home publication, destination-home consumption, or RV64 lowering of
already-published prepared facts. Implementation may start only after evidence
identifies one boundary, or after `todo.md` records a smaller split with
concrete proof for supervisor lifecycle routing.

## Execution Rules

- Keep routine evidence, packet state, and proof notes in `todo.md`.
- If the first owner is stack-destination fan-in authority from ideas 647 or
  655, stop and ask the supervisor for lifecycle routing instead of broadening
  this plan.
- For each code-changing step, run build proof plus the supervisor-selected
  focused destination-publication subset. Broader backend regression belongs to
  the supervisor acceptance pass.
- Preserve fail-closed diagnostics for missing, ambiguous, or mismatched
  destination facts.
- Reject patches whose main effect is expectation churn, named-case matching,
  diagnostics-only relabeling, or preserving the same destination-publication
  failure behind a new helper name.

## Reviewer Reject Signals

- Reject turning this into another packet under ideas 647 or 655 without
  explicit lifecycle switch evidence.
- Reject RV64 consumer materialization before prepared facts publish the
  relevant destination or parameter home.
- Reject named-case shortcuts for parameter-home, frame-slot destination,
  call-result predicate, or return-chain tests.
- Reject unsupported-marker downgrades, expectation rewrites, allowlist edits,
  helper renames, or classification-only edits claimed as capability progress.
- Reject broad changes that absorb pointer-local, byval, object-data,
  AArch64, CLI, RISC-V object-emission, callee-saved GPR, packed-member, or
  LLVM torture owners into this destination-publication route.
- Reject retaining the same missing or ambiguous destination-publication
  failure behind a renamed diagnostic.

## Ordered Steps

### Step 1: Refresh Prepared Destination Evidence

Goal: Reproduce the current failure boundaries for the four focused dump rows.

Primary targets:

- `tests/backend/CMakeLists.txt`
- `tests/c/external/gcc_torture/src/20001017-1.c`
- `tests/backend/case/riscv64_scalar_compare_frame_slot_destination.c`
- `tests/backend/case/riscv64_prepared_fused_compare_call_result_predicate.c`
- `tests/backend/case/riscv64_function_pointer_return_chain.c`
- current prepared-BIR, RV64 route, assembly, object, disassembly, and runtime
  artifacts generated by the focused tests

Actions:

- Run the focused dump rows and collect the first failing diagnostic or missing
  snippet for each row.
- Refresh semantic BIR, prepared-BIR, and RV64 consumer evidence where the dump
  failure does not already name the owner.
- Compare any stack-destination/fan-in evidence against ideas 647 and 655 so
  this route does not silently reopen their authority work.
- Record whether each row is owned first by prepared destination publication,
  parameter-home publication, destination-home consumption, RV64 consumer
  lowering, or a smaller split.

Completion check:

- `todo.md` records a focused evidence summary naming the first owner or split
  for every targeted row, with no implementation changes required for this
  step.

### Step 2: Select The First Repair Boundary

Goal: Choose one general prepared destination/publication boundary for the
first implementation packet.

Primary targets:

- the prepared/RV64 producer or consumer surface proven by Step 1
- focused backend cases that prove a positive shape and a fail-closed negative
  shape for that boundary

Actions:

- Select exactly one boundary from the Step 1 evidence.
- Define the positive and negative evidence the executor must preserve.
- If the four rows need separate owners, keep only the first owner in this
  runbook and record separate owner candidates in `todo.md` for supervisor
  lifecycle routing.
- If the selected boundary belongs to ideas 647 or 655, stop before
  implementation and return a lifecycle-routing recommendation.

Completion check:

- `todo.md` identifies one selected implementation boundary, the rows it owns,
  and the focused proof subset for the first repair packet.

### Step 3: Repair The Selected Destination Publication Rule

Goal: Implement one general prepared/RV64 destination-publication rule that
fixes or fails closed at the proven owner.

Primary targets:

- the source files named by Step 1 and Step 2 evidence
- the focused backend dump/route/runtime cases for the selected boundary

Actions:

- Repair publication or consumption of the proven destination, parameter home,
  call-result predicate, or return-chain facts.
- Preserve explicit owner and consumer-point evidence in prepared dumps or
  fail-closed diagnostics.
- Add or update focused positive and negative tests only when they prove the
  general rule rather than the named baseline row.
- Keep rows outside the selected owner fail-closed with precise diagnostics.

Completion check:

- The selected focused subset passes or fails closed at the proven owner.
- Build proof is fresh.
- No unrelated backend family changes are included.

### Step 4: Broaden Within The Destination Family

Goal: Prove that the repair did not regress nearby prepared destination rows
and decide whether another destination-publication packet remains.

Primary targets:

- all four current destination-publication dump rows
- route/runtime rows for call-result predicate and return-chain consumers when
  touched by the repair
- ideas 647 and 655 as nearby fan-in authority boundaries

Actions:

- Re-run the full focused destination-publication subset after the selected
  repair.
- Check whether remaining failures belong to the same selected owner, a
  separate destination-publication owner, ideas 647/655, or a non-destination
  initiative.
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
  `ideas/open/661_rv64_prepared_destination_publication.md` are satisfied.

Completion check:

- The active plan has an executor-updated `todo.md` with current proof,
  watchouts, and either the next selected packet or a source-idea completion
  recommendation.

# Stack-Passed Parameter Home Dump Contract Split Runbook

Status: Active
Source Idea: ideas/open/672_stack_passed_parameter_home_dump_contract_split.md

## Purpose

Resolve the residual stack-passed parameter-home dump row by separating callee
parameter-home publication evidence from caller ABI stack-binding dump contract
evidence.

Goal: classify whether the remaining dump mismatch is stale expectation,
missing caller ABI stack-binding exposure, or missing callee parameter-home
publication, then repair only the proven owner.

## Core Rule

Do not reopen stack-destination fan-in authority from ideas 647 or 655. Treat
caller ABI stack bindings and callee parameter homes as separate facts until
focused evidence proves one owner.

## Read First

- `ideas/open/672_stack_passed_parameter_home_dump_contract_split.md`
- `ideas/closed/661_rv64_prepared_destination_publication.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
- `ideas/open/647_ordered_or_exclusive_stack_destination_fan_in_authority.md`
- `ideas/open/655_stack_destination_fan_in_authority_decomposition.md`

## Current Target

- `backend_dump_riscv64_stack_passed_parameter_home_publication`

Nearby proof surfaces should be selected by the supervisor after the refreshed
dump evidence identifies whether the owner is caller ABI binding, callee
parameter-home publication, or stale snippet expectation.

## Non-Goals

- Do not work on scalar compare, fused compare, or function-pointer return-chain
  dump contract review; that was idea 671.
- Do not reopen ordered final-state, mutual-exclusion, explicit-merge, or
  rejection authority for stack-destination fan-in from ideas 647 and 655.
- Do not absorb RV64 byval, pointer-local, object-data static storage,
  object-emission, callee-saved GPR, packed-member, AArch64, prepared CLI, or
  LLVM torture work.
- Do not change unsupported markers, allowlists, timeout policy, runtime
  policy, or baseline acceptance.
- Do not update expectations without current caller ABI and callee
  parameter-home evidence.

## Working Model

The closed RV64 prepared destination publication route proved current callee
parameter homes for stack-passed parameters, including `%p.fdB value_id=10
slot#10 offset 40`, `%p.C value_id=12 slot#12 offset 48`, and `%p.fdC
value_id=13 slot#11 offset 44`. The residual dump row first missed the caller
ABI stack-binding snippet for `abi_index=8 stack_offset=0`. Current prepared
output publishes ABI stack bindings for indices 9, 11, and 12, while index 8 is
in register `a7` and index 10 is in register `fa1`. Execution must prove
whether the snippet contract is stale, whether caller stack-binding dump
exposure is missing, or whether callee parameter-home publication is incomplete.

## Execution Rules

- Refresh the focused dump row before selecting any repair owner.
- Record caller ABI binding facts and callee parameter-home facts separately.
- If an expectation update is justified, tie it to current ABI/home facts rather
  than final assembly or testcase identity.
- If code changes are justified, repair a general publication or dump-contract
  rule without matching the target name, fixed value IDs, or source order.
- Keep diagnostics fail-closed for missing, ambiguous, stale, or mismatched ABI
  and home facts.
- Include the supervisor-selected backend regression subset before close.

## Step 1: Refresh ABI And Parameter-Home Dump Evidence

Goal: establish the current prepared-BIR dump boundary for the residual
stack-passed parameter-home publication row.

Primary target:

- `backend_dump_riscv64_stack_passed_parameter_home_publication`

Actions:

- Run focused dump evidence for the target row.
- Capture current caller ABI bindings, callee parameter-home facts, stack
  offsets, ABI indices, value IDs, and the first mismatched or missing snippet.
- Compare current evidence with the closed idea 661 publication evidence and
  idea 672's recorded split.
- Classify the owner as stale snippet expectation, caller ABI stack-binding
  dump exposure, or missing callee parameter-home publication.

Completion check:

- `todo.md` records the focused command, result, and owner classification with
  concrete ABI/home evidence.

## Step 2: Select Repair Owner And Patch Narrowly

Goal: repair exactly the proven stack-passed parameter-home or ABI dump-contract
owner.

Actions:

- If the owner is stale expectation, update only the relevant expected dump
  snippet and document why current ABI/home facts are authoritative.
- If the owner is caller ABI stack-binding dump exposure, repair the general
  dump/emission path so valid caller stack bindings are exposed consistently.
- If the owner is missing callee parameter-home publication, repair the general
  publication path that should produce the home facts before dump emission.
- Preserve fail-closed diagnostics for missing or ambiguous owner facts.
- Keep ideas 647 and 655 outside the repair.

Completion check:

- The focused dump row passes or fails closed with a precise proven owner, and
  any code or expectation diff is justified by current ABI/home facts rather
  than testcase identity.

## Step 3: Prove Focused And Nearby Regression Safety

Goal: prove the selected repair did not regress the prepared destination or
stack-passed parameter-home surfaces.

Actions:

- Run the focused dump row after the repair.
- Run any nearby route/runtime or prepared destination rows the supervisor
  delegates as regression surfaces.
- Include any additional narrow backend subset the supervisor delegates for
  close readiness.
- Record exact commands and results in `todo.md`.

Completion check:

- Focused proof and supervisor-selected regression proof are green, or any
  remaining failure is fail-closed with an owner outside idea 672's scope.

## Step 4: Close Readiness

Goal: prepare the lifecycle close decision for idea 672.

Actions:

- Confirm the source idea acceptance criteria are satisfied.
- Confirm no forbidden scope was changed, especially ideas 647 and 655.
- Ensure canonical regression logs cover the supervisor-selected close scope.
- Hand off to the plan owner for close only after focused proof and regression
  proof are current.

Completion check:

- `todo.md` contains close-ready proof notes, and the active runbook can be
  evaluated against
  `ideas/open/672_stack_passed_parameter_home_dump_contract_split.md`.

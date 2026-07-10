# Prepared Destination Dump Contract Review Runbook

Status: Active
Source Idea: ideas/open/671_prepared_destination_dump_contract_review.md

## Purpose

Review the prepared-BIR dump contract for residual destination-publication
rows left after the RV64 prepared destination publication route.

Goal: determine whether each residual row is stale snippet expectation, dump
text emission over valid prepared facts, or missing prepared publication, then
repair only the proven dump-contract owner.

## Core Rule

Treat prepared destination publication as already repaired unless fresh
focused proof shows a current prepared fact is missing or ambiguous. Do not
claim expectation-only changes as lowering or publication capability progress.

## Read First

- `ideas/open/671_prepared_destination_dump_contract_review.md`
- `ideas/closed/661_rv64_prepared_destination_publication.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`

## Current Targets

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

Nearby proof surfaces should be selected by the supervisor after current
evidence identifies whether route/runtime coverage exists for each row.

## Non-Goals

- Do not work on stack-passed parameter-home publication or caller ABI
  stack-binding; that is idea 672.
- Do not reopen ideas 647 or 655 stack-destination fan-in authority.
- Do not absorb RV64 pointer-local, byval, object-data static storage,
  object-emission, callee-saved GPR, packed-member, AArch64, prepared CLI, or
  LLVM torture work.
- Do not change unsupported markers, allowlists, timeout policy, runtime
  policy, or baseline acceptance.
- Do not claim text-only expectation churn as compiler capability progress.

## Working Model

Idea 661 repaired RV64 prepared destination publication. The remaining dump
rows have evidence of current prepared facts whose IDs differ from stale dump
snippets: scalar compare exposes `%t2` at `value_id=3`, fused compare exposes
the call-result destination at `destination_value_id=1`, and function-pointer
return-chain exposes `@sub` at `value_id=6`. Current execution must refresh
that evidence and only change code when the refreshed facts prove a general
prepared publication or dump emission owner.

## Execution Rules

- Refresh all three focused dump rows before selecting a repair owner.
- Separate evidence from inference when deciding stale expectation versus dump
  emission versus missing prepared publication.
- If an expectation update is justified, pair it with current prepared facts
  and any nearby route/runtime proof the supervisor delegates.
- If code changes are justified, expose existing prepared facts generally
  without matching testcase names, value IDs, or final assembly shape.
- Keep diagnostics fail-closed for missing, stale, or ambiguous prepared facts.
- Use focused proof first, then include the supervisor-selected backend
  regression subset before close.

## Step 1: Refresh Destination Dump Evidence

Goal: establish the current prepared-BIR dump boundary for the three residual
prepared destination rows.

Primary targets:

- `backend_dump_riscv64_scalar_compare_frame_slot_destination`
- `backend_dump_riscv64_prepared_fused_compare_call_result_predicate`
- `backend_dump_riscv64_function_pointer_return_chain`

Actions:

- Run focused dump evidence for all three targets.
- Capture the current prepared facts, snippet expectations, value IDs, and
  first mismatched or missing lines.
- Compare the result against the closure evidence from idea 661.
- Record whether each row currently points to stale snippet expectation, dump
  text emission, or missing prepared publication.

Completion check:

- `todo.md` records the focused command, result, and a row-by-row owner
  classification with concrete prepared-fact evidence.

## Step 2: Select Repair Owner And Patch Narrowly

Goal: repair exactly the proven prepared destination dump-contract owner.

Actions:

- If the owner is stale expectation, update only the relevant expected dump
  snippets and document why current prepared facts are authoritative.
- If the owner is dump text emission, repair the general printer/emission path
  so valid prepared destination facts are exposed without testcase matching.
- If the owner is missing prepared publication, repair the general publication
  path that should produce the fact before dump emission consumes it.
- Avoid value-id-only shortcuts; explain why any ID currently in the dump is
  the authoritative fact.
- Keep the stack-passed parameter-home split outside this repair.

Completion check:

- The focused dump rows pass or fail closed with a precise proven owner, and
  any code or expectation diff is justified by current prepared facts rather
  than testcase identity.

## Step 3: Prove Destination Dump And Nearby Regression Safety

Goal: prove the selected repair did not regress the completed prepared
destination publication route.

Actions:

- Run the focused dump rows after the repair.
- Run any nearby route/runtime or prepared destination rows the supervisor
  delegates as regression surfaces.
- Include any additional narrow backend subset the supervisor delegates for
  close readiness.
- Record exact commands and results in `todo.md`.

Completion check:

- Focused dump proof and supervisor-selected regression proof are green, or
  any remaining failure is fail-closed with an owner outside idea 671's scope.

## Step 4: Close Readiness

Goal: prepare the lifecycle close decision for idea 671.

Actions:

- Confirm the source idea acceptance criteria are satisfied.
- Confirm no forbidden scope was changed.
- Ensure canonical regression logs cover the supervisor-selected close scope.
- Hand off to the plan owner for close only after focused proof and regression
  proof are current.

Completion check:

- `todo.md` contains close-ready proof notes, and the active runbook can be
  evaluated against `ideas/open/671_prepared_destination_dump_contract_review.md`.

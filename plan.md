# Byval Prepared Dump Contract Review Runbook

Status: Active
Source Idea: ideas/open/669_byval_prepared_dump_contract_review.md

## Purpose

Review the prepared-BIR dump contract for the residual byval rows left after
the RV64 byval prepared call-boundary route repaired route and runtime
behavior.

Goal: determine whether each residual row is stale snippet expectation, dump
text emission over valid prepared facts, or missing prepared publication, then
repair only the proven dump-contract owner.

## Core Rule

Treat route and runtime byval behavior as regression surfaces. Do not reopen
the closed RV64 byval runtime/codegen-route repair unless fresh focused proof
shows those passing rows regressed.

## Read First

- `ideas/open/669_byval_prepared_dump_contract_review.md`
- `ideas/closed/659_rv64_byval_prepared_call_boundary.md`
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`

## Current Targets

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_dump_riscv64_byval_preserved_pointer_args`

Nearby regression surfaces:

- `backend_codegen_route_riscv64_byval_aggregate_fixed_call`
- `backend_codegen_route_riscv64_byval_preserved_pointer_args`
- `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`
- `backend_rv64_runtime_riscv64_byval_preserved_pointer_args`

## Non-Goals

- Do not repair RV64 byval runtime or codegen-route behavior that already
  passed under idea 659.
- Do not work on object-runtime `BinaryInst` unsupported fragments; that is
  idea 670.
- Do not absorb pointer-local, destination-publication, static object-data,
  callee-saved GPR, packed-member, AArch64, LLVM torture, or stack-destination
  fan-in work.
- Do not change unsupported markers, allowlists, timeout policy, runtime
  policy, or baseline acceptance.
- Do not claim text-only expectation churn as compiler capability progress.

## Working Model

Idea 659 repaired the byval call-boundary route/runtime owner and proved the
broad backend guard. The remaining dump rows may now be stale snippets or dump
exposure mismatches over valid prepared facts. Implementation must start from
fresh dump evidence and only change code when the evidence proves a general
prepared publication or dump emission owner.

## Execution Rules

- Refresh both focused dump rows before selecting a repair owner.
- Separate evidence from inference when deciding stale expectation versus dump
  emission versus missing prepared publication.
- If an expectation update is justified, pair it with current prepared facts
  and nearby route/runtime proof; record that it is contract alignment, not
  lowering progress.
- If code changes are justified, expose existing prepared facts generally
  without matching testcase names, value IDs, or final assembly shape.
- Keep diagnostics fail-closed for missing, stale, or ambiguous prepared facts.
- Use focused proof first, then include the supervisor-selected backend
  regression subset before close.

## Step 1: Refresh Byval Dump Evidence

Goal: establish the current prepared-BIR dump boundary for both residual byval
rows.

Primary targets:

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_dump_riscv64_byval_preserved_pointer_args`

Actions:

- Run focused dump evidence for both targets.
- Capture the current prepared facts, snippet expectations, value IDs, and
  first mismatched or missing lines.
- Compare the result against the closure evidence from idea 659.
- Record whether each row currently points to stale snippet expectation, dump
  text emission, or missing prepared publication.

Completion check:

- `todo.md` records the focused command, result, and a row-by-row owner
  classification with concrete prepared-fact evidence.

## Step 2: Select Repair Owner And Patch Narrowly

Goal: repair exactly the proven dump-contract owner.

Actions:

- If the owner is stale expectation, update only the relevant expected dump
  snippets and document why current prepared facts are authoritative.
- If the owner is dump text emission, repair the general printer/emission path
  so valid byval prepared facts are exposed without testcase matching.
- If the owner is missing prepared publication, repair the general publication
  path that should produce the fact before dump emission consumes it.
- Keep route/runtime byval behavior unchanged except for regression proof.
- Avoid value-id-only shortcuts; explain why any ID currently in the dump is
  the authoritative fact.

Completion check:

- The focused dump rows pass or fail closed with a precise proven owner, and
  any code or expectation diff is justified by current prepared facts rather
  than testcase identity.

## Step 3: Prove Byval Dump And Nearby Regression Safety

Goal: prove the selected repair did not regress the completed byval
call-boundary route/runtime behavior.

Actions:

- Run the focused dump rows after the repair.
- Run nearby byval route/runtime rows from idea 659 as regression surfaces.
- Include any additional narrow backend subset the supervisor delegates for
  close readiness.
- Record exact commands and results in `todo.md`.

Completion check:

- Focused dump proof and nearby route/runtime regression proof are green, or
  any remaining failure is fail-closed with an owner outside idea 669's scope.

## Step 4: Close Readiness

Goal: prepare the lifecycle close decision for idea 669.

Actions:

- Confirm the source idea acceptance criteria are satisfied.
- Confirm no forbidden scope was changed.
- Ensure canonical regression logs cover the supervisor-selected close scope.
- Hand off to the plan owner for close only after focused proof and regression
  proof are current.

Completion check:

- `todo.md` contains close-ready proof notes, and the active runbook can be
  evaluated against `ideas/open/669_byval_prepared_dump_contract_review.md`.

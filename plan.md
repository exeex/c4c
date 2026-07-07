# RV64 Scalar Compare Publication Runbook

Status: Active
Source Idea: ideas/open/580_rv64_scalar_compare_publication.md

## Purpose

Repair RV64 object emission for ordinary F32/F64 compare results whose prepared
non-terminator result home is a GPR.

## Goal

Publish scalar FPR equality and inequality compare results into prepared GPR
homes, then prove representative scalar-compare residual rows advance beyond
the old compare-publication failure.

## Core Rule

Do not solve this by matching residual testcase names, rewriting expectations,
or widening the work into floating casts, variadic helpers, F128, or broad FPR
ownership rewrites.

## Read First

- ideas/open/580_rv64_scalar_compare_publication.md
- build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv
- build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md
- RV64 object-emission compare/select code and focused backend tests

## Scope

- Ordinary F32/F64 `eq` and `ne` compare result publication.
- Non-terminator compare instructions with prepared GPR result homes.
- Direct compare-result users and select/materialization paths that consume a
  published compare result.
- Representative route proof for at least one simple compare row and one
  select-consuming compare row.

## Non-Goals

- Do not implement F128, long-double, external soft-float helper, or libc helper
  support.
- Do not implement ordinary floating-cast lowering or variadic helper lowering.
- Do not rewrite unsupported markers, route allowlists, or expected outputs as
  progress.
- Do not perform broad FPR or select rewrites beyond what compare-result
  publication requires.

## Working Model

The current residual evidence points at one semantic owner: RV64 object emission
can produce an FPR comparison, but cannot publish the scalar boolean result into
the prepared GPR home for later non-terminator use. Treat direct consumers and
select-consuming rows as proofs of the same publication capability unless fresh
evidence shows a later distinct owner.

## Execution Rules

- Keep progress and proof notes in `todo.md`.
- Save representative route artifacts under
  `build/agent_state/580_rv64_scalar_compare_publication/`.
- Add focused tests before or with the implementation so the semantic shape is
  covered without residual testcase-name checks.
- If a select-consuming row advances to a new owner, record that later owner
  instead of widening this plan silently.
- For code-changing steps, prove with `cmake --build --preset default` plus the
  focused RV64 backend test, then escalate to the backend subset before closure.

## Ordered Steps

### Step 1: Reproduce Compare Publication Owners

Goal: establish current direct and select-consuming scalar compare publication
failures from existing evidence or fresh representative routes.

Actions:
- Read the Step 3 salvage evidence for the four retained compare rows.
- Rerun representative RV64 object routes only where the existing logs are
  stale or insufficient.
- Record the first failing owner, opcode, operand types, prepared result home,
  and whether the compare feeds a select/materialization path.
- Save any fresh commands, return codes, logs, and prepared dumps under the
  Step 1 agent-state directory.

Completion check:
- `todo.md` records at least one simple compare representative and one
  select-consuming representative with current owner facts.
- The next step has enough evidence to build a focused semantic fixture without
  matching testcase names.

### Step 2: Add Focused Failing Coverage

Goal: capture the unsupported compare-publication shape in RV64 object-emission
tests before changing lowering behavior.

Actions:
- Add focused test coverage for an ordinary F32/F64 compare result whose
  prepared destination is a GPR.
- Include coverage for a direct published result and, if practical, a
  select-consuming result path.
- Assert the current failure or missing publication precisely enough that the
  implementation step has a meaningful proof target.

Completion check:
- `cmake --build --preset default` succeeds.
- The focused RV64 object-emission test fails for the expected semantic reason,
  and `todo.md` records the command and result.

### Step 3: Implement Semantic Compare Publication

Goal: lower or materialize ordinary F32/F64 compare results into prepared GPR
homes for non-terminator use.

Actions:
- Repair the RV64 object-emission path that handles ordinary FPR `eq` and `ne`
  compare results with scalar GPR destinations.
- Prefer semantic opcode/type/home handling over named-case or fixed-register
  branches.
- Preserve existing terminator compare behavior and unrelated unsupported
  diagnostics.
- Extend focused tests as needed for both `eq` and `ne`, F32/F64, and prepared
  GPR homes used by later instructions.

Completion check:
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
  passes.
- `todo.md` records the implementation surface and proof.

### Step 4: Prove Representative Route Advancement

Goal: show the residual compare rows advance beyond the old
`unsupported_scalar_compare_publication` owner.

Actions:
- Rerun at least one simple compare row, such as `src/20080529-1.c` or
  `src/930818-1.c`.
- Rerun at least one select-consuming compare row, such as `src/loop-8.c` or
  `src/strct-pack-1.c`.
- Record whether each route passes, advances to a later owner, or exposes a
  narrower follow-up.
- Save route commands, return codes, logs, and prepared dumps under the Step 4
  agent-state directory.

Completion check:
- At least one current scalar-compare residual row advances past
  `unsupported_scalar_compare_publication`.
- Select-consuming rows either advance or have a later, narrower owner recorded
  in `todo.md`.

### Step 5: Backend Closure Readiness

Goal: validate the touched RV64 backend bucket and prepare the plan for
supervisor closure evaluation.

Actions:
- Run the backend validation subset chosen by the supervisor, normally
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Confirm the focused coverage and representative route evidence satisfy the
  source idea acceptance criteria.
- Record any later owners as follow-up candidates instead of absorbing them into
  this plan.

Completion check:
- Backend validation passes or the blocker is recorded precisely in `todo.md`.
- `todo.md` states whether the source idea is ready for plan-owner closure
  evaluation.

# RV64 Ordinary Floating Cast Lowering Runbook

Status: Active
Source Idea: ideas/open/581_rv64_ordinary_floating_cast_lowering.md

## Purpose

Repair or narrow RV64 object lowering for ordinary non-F128 floating casts
identified by the scalar/FPR residual salvage evidence.

## Goal

Lower ordinary F32/F64 `fptrunc`, `fpext`, and justified integer-to-FP forms in
RV64 object emission, then prove retained representative rows advance beyond
the old floating-cast owner or stop at a narrower semantic diagnostic.

## Core Rule

Do not use F128, long-double, testcase-name checks, expectation rewrites, or
unsupported-marker changes to claim ordinary floating-cast progress.

## Read First

- ideas/open/581_rv64_ordinary_floating_cast_lowering.md
- build/agent_state/550_rv64_scalar_fpr_residual_salvage/step2/summary.md
- build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/owners.tsv
- build/agent_state/550_rv64_scalar_fpr_residual_salvage/step3/summary.md
- RV64 object-emission `CastInst` lowering code and focused backend tests

## Scope

- Ordinary F32/F64 `fptrunc` and `fpext` lowering.
- Ordinary integer-to-FP forms justified by the retained `uitofp i32 to float`
  evidence.
- Focused tests that prove opcode/type/home semantics rather than residual row
  names.
- Representative route proof for `src/920618-1.c`, `src/ieee/pr67218.c`,
  `src/pr23941.c`, or current stronger substitutes.

## Non-Goals

- Do not implement F128, long-double, soft-float helper, or F128 carrier
  plumbing.
- Do not absorb scalar compare publication or variadic `va_start` helper work.
- Do not attempt full cast-matrix completion beyond forms justified by the
  retained ordinary F32/F64 evidence.
- Do not rewrite route expectations, unsupported markers, allowlists, or
  runtime comparisons as progress.

## Working Model

The retained rows point at RV64 object `CastInst` lowering for ordinary
floating casts. Treat the F128 and long-double rows quarantined by the salvage
lane as separate evidence that must not drive this implementation. If the
ordinary retained rows split into independent owners, record that split before
widening the route.

## Execution Rules

- Keep progress and proof notes in `todo.md`.
- Save representative route artifacts under
  `build/agent_state/581_rv64_ordinary_floating_cast_lowering/`.
- Add focused coverage before or with implementation so the tested shapes are
  semantic cast forms, not residual testcase names.
- Preserve fail-closed diagnostics for unsupported cast forms outside this
  lane.
- For code-changing steps, prove with `cmake --build --preset default` plus the
  focused RV64 backend test, then escalate to the backend subset before
  closure.

## Ordered Steps

### Step 1: Reproduce Ordinary Cast Owners

Goal: establish the current RV64 object-emission owner facts for the retained
ordinary floating-cast rows.

Actions:
- Read the Step 2 and Step 3 salvage evidence for retained ordinary cast rows.
- Rerun representative RV64 object routes only where existing logs are stale or
  insufficient.
- Record the first failing owner, cast opcode, source type, destination type,
  operand home, result home, and whether the row includes chained casts.
- Explicitly confirm that F128 and long-double quarantined rows are outside the
  proof route.
- Save any fresh commands, return codes, logs, and prepared dumps under the
  Step 1 agent-state directory.

Completion check:
- `todo.md` records current owner facts for at least one retained ordinary
  `fptrunc` or `fpext` row and the retained `uitofp i32 to float` evidence if
  still relevant.
- The next step has enough evidence to build focused semantic fixtures without
  matching residual testcase names.

### Step 2: Add Focused Failing Coverage

Goal: capture the unsupported ordinary cast shapes in RV64 object-emission
tests before changing lowering behavior.

Actions:
- Add focused coverage for ordinary F32/F64 `fptrunc` and `fpext` lowering.
- Add focused coverage for `uitofp i32 to float` only if Step 1 confirms it
  remains in the retained owner route.
- Assert the current failure or missing lowering precisely enough that the
  implementation step has a meaningful proof target.
- Keep F128, long-double, and helper-based casts out of these fixtures.

Completion check:
- `cmake --build --preset default` succeeds.
- The focused RV64 object-emission test fails for the expected ordinary
  floating-cast reason, and `todo.md` records the command and result.

### Step 3: Implement Ordinary Floating Cast Lowering

Goal: lower the retained ordinary cast forms through semantic opcode/type/home
handling.

Actions:
- Repair the RV64 object-emission path for ordinary F32/F64 `fptrunc` and
  `fpext`.
- Implement the retained integer-to-FP form only where Step 1 and Step 2 prove
  it shares the same ordinary cast-lowering boundary.
- Publish cast results into the prepared destination homes expected by later
  instructions.
- Preserve existing unsupported diagnostics for unhandled types, homes, F128,
  long-double, and helper-required casts.
- Extend focused tests as needed for both positive lowering and fail-closed
  unsupported forms.

Completion check:
- `cmake --build --preset default` succeeds.
- `ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'`
  passes.
- `todo.md` records the implementation surface and proof.

### Step 4: Prove Representative Route Advancement

Goal: show retained ordinary floating-cast rows advance beyond the old
`unsupported_floating_cast` owner or expose a narrower downstream owner.

Actions:
- Rerun retained ordinary floating-cast representatives such as
  `src/920618-1.c`, `src/ieee/pr67218.c`, and `src/pr23941.c`, or current
  stronger substitutes.
- Record whether each route passes, advances to a later owner, or exposes a
  narrower follow-up.
- Keep quarantined F128 and long-double rows out of the completion claim.
- Save route commands, return codes, logs, and prepared dumps under the Step 4
  agent-state directory.

Completion check:
- At least one retained ordinary floating-cast row advances past
  `unsupported_floating_cast`, or records a narrower semantic owner with
  concrete route evidence.
- Any remaining retained rows have clear owner notes in `todo.md` rather than
  being silently counted as complete.

### Step 5: Backend Closure Readiness

Goal: validate the touched RV64 backend bucket and prepare the plan for
supervisor closure evaluation.

Actions:
- Run the backend validation subset chosen by the supervisor, normally
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
- Confirm the focused coverage and representative route evidence satisfy the
  source idea acceptance criteria.
- Record any later owners as follow-up candidates instead of absorbing them
  into this plan.

Completion check:
- Backend validation passes or the blocker is recorded precisely in `todo.md`.
- `todo.md` states whether the source idea is ready for plan-owner closure
  evaluation.

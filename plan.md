# RV64 Ordinary Control And Expression Completion Runbook

Status: Active
Source Idea: ideas/open/311_rv64_ordinary_control_expression_completion.md

## Purpose

Complete ordinary RV64 control-flow and expression-result emission so compiled
C functions do not stop after partial expression lowering and fall through
without required branch, return-value, epilogue, or `ret` tails.

## Goal

Repair the backend emission rule or rules behind the
`incomplete_control_or_expression_emission` family identified by the RV64
c-testsuite QEMU triage.

## Core Rule

Fix semantic RV64 lowering for ordinary expression/control completion. Do not
match c-testsuite filenames, rewrite expectations, or append generic tails that
hide unmaterialized predicates, call results, or return values.

## Read First

- `ideas/open/311_rv64_ordinary_control_expression_completion.md`
- `build/rv64_c_testsuite_probe_latest/triage_step4/summary.md`
- `build/rv64_c_testsuite_probe_latest/triage_step4/classification.tsv`
- Representative candidates: `src/00008.c`, `src/00030.c`, `src/00105.c`

## Current Targets

- RV64 backend emission for ordinary local expression completion.
- Conditional and loop branch predicate publication.
- Call-result value use after calls return into `a0`.
- Return-value finalization, function epilogues, and final `ret` emission.

## Non-Goals

- Do not solve local stack-slot or address-taken object materialization except
  where directly required by ordinary expression/control completion.
- Do not solve external libc, libm, string, or bodyless-stub policy.
- Do not solve global storage layout or undefined temporary-label assembler
  issues.
- Do not mark supported-path cases unsupported or weaken expectations without
  explicit approval.
- Do not broaden this into a full c-testsuite runtime cleanup.

## Working Model

The failing family enters ordinary expression, condition, call-result, branch,
or loop lowering and then stops before publishing the value/control result that
the surrounding block needs. The repair should make the prepared-BIR to RV64
emission path complete those tails for a class of operations, then prove that
nearby loop, call-result, and return-tail cases move for the same reason.

## Execution Rules

- Work in small implementation steps with fresh build or compile proof for each
  code-changing packet.
- Prefer focused unit or integration tests that encode local control/expression
  behavior over c-testsuite-only evidence.
- Use the c-testsuite candidates as representative runtime probes, not as
  implementation selectors.
- Keep remaining failures classified by mechanism when they are outside this
  idea, especially stack/local address materialization or external stubs.
- Escalate to supervisor/reviewer if progress depends on expectation rewrites,
  unsupported downgrades, or filename-shaped logic.

## Step 1: Normalize Representative Failure Evidence

Goal: identify the exact BIR/prepared-BIR and assembly tail shape for the first
repair target.

Primary targets:

- `src/00008.c`
- `src/00030.c`
- `src/00105.c`

Actions:

- Re-read the Step 4 triage evidence for the representative candidates.
- Regenerate or inspect focused `--dump-bir`, `--dump-prepared-bir`, and
  `--codegen asm` artifacts for the representative candidates if existing
  artifacts are stale or insufficient.
- Identify the common emission boundary where predicate publication,
  call-result use, return-value finalization, epilogue, or `ret` emission is
  missing.
- Record the selected first repair target and any explicitly deferred
  non-owned mechanisms in `todo.md`.

Completion check:

- `todo.md` names the first semantic emission boundary to repair and shows why
  it covers more than one representative candidate without relying on filenames.

## Step 2: Add Focused Control/Expression Coverage

Goal: lock in ordinary backend behavior before and during the repair.

Actions:

- Add or update focused tests for loop condition completion, call-result value
  use, and return/epilogue completion.
- Keep tests semantic and compact; avoid importing c-testsuite filenames as the
  test contract.
- Run the narrow test subset or compile proof selected by the supervisor for
  the touched backend surface.

Completion check:

- The focused tests fail for the old missing-tail behavior or otherwise prove
  the current expected behavior, and the selected narrow command has fresh
  output.

## Step 3: Repair The First Semantic Emission Boundary

Goal: implement the smallest general RV64 backend rule that completes the
selected ordinary control/expression tail.

Actions:

- Modify the RV64 backend emission path for the selected BIR/prepared-BIR
  operation family.
- Ensure branch predicates, call-result values, return values, epilogues, and
  `ret` tails are produced by semantic lowering rather than by case-specific
  post-processing.
- Run build or compile proof plus the focused tests from Step 2.

Completion check:

- The focused tests pass, assembly for the representative target contains the
  required value/control tail, and no expectation downgrade is part of the
  change.

## Step 4: Probe Nearby Runtime Candidates

Goal: verify the repair moves the triaged family broadly enough to count as
  semantic progress.

Actions:

- Run an RV64 c-testsuite subset that includes at least `src/00008.c`,
  `src/00030.c`, `src/00105.c`, and nearby candidates from the same bucket.
- Compare outcomes against the Step 4 classification.
- Document any remaining failures by their actual mechanism rather than
  counting them as fixed.

Completion check:

- Runtime evidence shows representative candidates moved for the repaired
  semantic reason, or `todo.md` records the specific mechanism blocking them.

## Step 5: Complete Remaining Ordinary Tails

Goal: finish any remaining ordinary control/expression completion mechanisms
that are still in scope after the first repair.

Actions:

- Repeat the evidence -> focused test -> semantic repair -> representative
  probe loop for remaining ordinary branch, call-result, return, epilogue, or
  `ret` tails.
- Keep stack address materialization and external-stub policy failures out of
  this runbook unless they are required to prove the ordinary tail itself.

Completion check:

- The candidate family no longer contains unclassified missing branch,
  call-result, return, epilogue, or `ret` tails owned by this idea.

## Step 6: Acceptance Sweep And Closure Readiness

Goal: decide whether idea 311 is complete or whether a narrower follow-up is
needed.

Actions:

- Run the supervisor-selected broader validation checkpoint.
- Re-run the targeted c-testsuite subset and summarize moved, still-failing,
  and out-of-scope candidates.
- Confirm tests cover loop condition completion, call-result value use, and
  return/epilogue completion.
- Prepare `todo.md` for lifecycle closure review if the source idea acceptance
  criteria are satisfied.

Completion check:

- Fresh validation exists, remaining failures are outside the owned feature
  family or recorded as follow-up scope, and `todo.md` clearly says whether the
  source idea is closure-ready.

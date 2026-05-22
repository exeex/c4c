# AArch64 Scalar Cast Stack-Homed Register Source Publication Runbook

Status: Active
Source Idea: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Activated from: parked source idea with no active plan present

## Purpose

Refresh and, only if still current, repair the AArch64 scalar cast
source-publication owner recorded in the source idea.

## Goal

Ensure selected AArch64 scalar cast nodes consume a structured register source
when their original source value is stack-homed but prepared lowering already
materialized a consumer stack-to-register move.

## Core Rule

Do not assume the historical `00143` scalar cast printer diagnostic still
exists. Start by proving the current first bad fact before changing code.

## Read First

- `ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md`
- Existing focused backend coverage for selected scalar casts whose source is
  stack-homed and moved into a consumer register.
- Current prepared BIR, selected records, and generated AArch64 artifacts for
  `c_testsuite_aarch64_backend_src_00143_c` only as representative evidence,
  not as a testcase-specific repair target.

## Current Scope

- Selected scalar cast source publication for stack-homed source values.
- Prepared consumer stack-to-register moves that should become the structured
  register source consumed by selected AArch64 printing.
- Focused backend contracts that prove the rule independent of one external
  c-testsuite file.

## Non-Goals

- Do not rewrite closed idea 338 or mutate files under `ideas/closed/`.
- Do not reopen idea 339 local storage/writeback sizing without fresh
  generated-code evidence that the current first bad fact moved there.
- Do not broaden into runtime value correctness, frame-layout sizing, compare
  lowering, timeout/output-storm behavior, aggregates, variadics, or semantic
  `lir_to_bir` admission.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not special-case `00143`, `%t76`, `%t81`, value ids, `x13`, one block,
  one instruction, one diagnostic string, or one emitted instruction sequence.

## Working Model

The historical failure was a selected-machine-node printer diagnostic where a
`sign_extend` scalar cast had a stack-homed source and a prepared
consumer-stack-to-register move, but the selected AArch64 node did not expose
that moved register as its structured source operand. The source idea says
that owner was repaired and the remaining `00143` failure moved to an
out-of-scope fallthrough fixed-offset local load/store residual. This runbook
therefore starts with a refresh and only proceeds to implementation if the
scalar cast source-publication owner is live again.

## Execution Rules

- Treat the source idea as the durable scope contract.
- Prefer focused backend records and generated-code evidence over external
  output alone.
- If the current first bad fact is not scalar cast register-source
  publication, classify it in `todo.md` and return for lifecycle routing
  rather than widening this plan.
- Any repair must preserve or add focused coverage that fails without the
  general stack-homed scalar cast source-publication behavior.
- For any code-changing step, require build proof plus the
  supervisor-selected focused CTest subset before acceptance.

## Step 1: Refresh Current First Bad Fact

Goal: determine whether scalar cast stack-homed register source publication is
still a live owner.

Primary target: the supervisor-selected focused representative for this idea,
normally `c_testsuite_aarch64_backend_src_00143_c` plus existing scalar cast
source-publication backend contracts.

Actions:

- Rebuild the current tree before classification.
- Run the focused representative and nearby scalar cast backend coverage.
- Inspect current prepared/generated artifacts if anything fails.
- Decide whether any observed failure still shows a selected scalar cast
  missing its structured register source after a prepared consumer move.

Completion check:

- `todo.md` records either a current scalar cast source-publication bad fact
  with concrete evidence, or a current out-of-scope / already-green
  classification for supervisor lifecycle routing.

## Step 2: Localize The Source-Publication Boundary

Goal: if Step 1 finds an in-scope failure, identify exactly where the moved
register source stops being attached to the selected scalar cast.

Primary target: prepared consumer move creation, selected-node normalization,
structured operand publication, and AArch64 scalar cast printing.

Actions:

- Compare semantic BIR, prepared BIR, selected records, and emitted assembly
  for the failing cast and its first missing source operand.
- Identify whether the gap is in move consumption, selected-node handoff,
  operand normalization, register publication, or printer lookup.
- Record the smallest focused backend shape that should fail without the
  repair.

Completion check:

- The failing boundary is named in terms of cast, original stack source,
  prepared register carrier, expected structured operand, and observed missing
  operand.

## Step 3: Repair General Publication

Goal: repair the localized scalar cast source-publication rule without
testcase-shaped matching.

Actions:

- Implement the smallest general lowering, normalization, or publication
  change needed for the localized owner.
- Add or update focused backend coverage for stack-homed scalar cast sources
  with prepared consumer stack-to-register moves.
- Keep adjacent scalar publication and local storage/writeback behavior
  stable.

Completion check:

- Focused backend coverage proves selected scalar casts receive the structured
  register source.
- The implementation does not mention `00143`, one value id, one register, or
  one emitted instruction sequence as a control path.

## Step 4: Prove And Reclassify

Goal: prove the repaired owner and hand off any remaining first bad fact.

Actions:

- Run the exact focused proof command delegated by the supervisor.
- Include `00143` representative proof if it remains the external witness.
- If the representative still fails, classify the new first bad fact and keep
  it separate from this scalar cast source scope.

Completion check:

- `todo.md` records the proof command and result.
- The source idea is either ready for close consideration, or the next first
  bad fact is clearly classified for lifecycle handoff.

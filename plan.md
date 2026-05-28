# AArch64 Dispatch Prepared Publication Decomposition Runbook

Status: Active
Source Idea: ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md
Supersedes active runbook for: ideas/open/58_aarch64_prepared_authority_regression_recovery.md

## Purpose

Replace the stuck Step 3 assertion-chasing route with a decomposition-oriented
runbook that splits AArch64 dispatch/prepared-publication behavior into focused
probes and owned backend seams.

## Goal

Make the next implementation packet start from one focused seam and proof,
instead of using `backend_aarch64_instruction_dispatch` as the only route
driver.

## Core Rule

Do not continue to chase the next monolithic dispatch assertion until the
behavior is bound to a focused probe or a documented seam owner. Decomposition
is route-quality work, not backend capability progress by itself.

## Read First

- `ideas/open/60_aarch64_dispatch_prepared_publication_decomposition.md`
- `ideas/open/58_aarch64_prepared_authority_regression_recovery.md`
- `review/step3_dispatch_route_review.md`
- `todo.md`
- `test_after.log`

## Current Targets

- AArch64 dispatch prepared-publication seams exposed by the dirty Step 3
  implementation stack.
- Focused backend probes under `tests/backend/case/` or existing equivalent
  backend route tests.
- The current focused four-test proof command used by the supervisor:
  `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`,
  `c_testsuite_aarch64_backend_src_00196_c`, and
  `c_testsuite_aarch64_backend_src_00207_c`.

## Non-Goals

- Do not edit implementation files during decomposition packets unless the
  supervisor explicitly delegates an implementation packet after a focused seam
  is selected.
- Do not close idea 58; its four-test recovery target remains open.
- Do not weaken any dispatch, backend route, or c-testsuite contract.
- Do not add testcase-name checks, exact temporary-name checks, literal label
  checks, or fixed stack-offset checks.
- Do not turn the decomposition idea into a broad AArch64 cleanup.

## Working Model

The dirty implementation stack contains partial semantic repairs that should
not be reverted as part of lifecycle work:

- store-global pending publication ownership/accounting
- selected fused-compare scratch preference
- call/outgoing stack argument materialization fallbacks
- direct edge `LoadLocal` prepared source-memory consumption

The same proof still reports `2/4` passing. The latest first bad dispatch
assertion is GOT-required `LoadGlobal` materialization. The separate `00196`
runtime mismatch remains the later `78730af2f` family from idea 58 unless a
focused probe proves shared ownership.

## Execution Rules

- Keep decomposition packets read-only or test/probe-oriented until a focused
  owner is selected.
- Prefer one probe per semantic contract.
- Reuse existing backend cases when they already isolate a seam.
- Record harness limitations in `todo.md` instead of forcing a weak probe.
- When implementation resumes, the packet must name the selected focused probe,
  backend owner surface, and proof command before code edits begin.
- Preserve the dirty implementation summary in `todo.md`; do not revert or
  reclassify those edits as accepted progress until proof is green enough for
  supervisor acceptance.

## Steps

### Step 1: Establish The Blocked Failure-Family Baseline

Goal: capture the current stuck route and dirty implementation context.

Primary target: `todo.md`, `test_after.log`, and
`review/step3_dispatch_route_review.md`.

Actions:

- Read the latest `todo.md` proof summary and `test_after.log`.
- Record the current focused proof result and latest first bad dispatch
  assertion.
- Identify which dirty implementation surfaces are incomplete context, not yet
  accepted recovery progress.
- Confirm idea 58 remains open and incomplete.

Completion check:

- `todo.md` records the blocked baseline, dirty context, current first bad
  assertion, and the fact that decomposition is now the active route.

### Step 2: Inventory The Separable Dispatch Seams

Goal: turn the monolithic dispatch route into a finite seam list.

Primary target: AArch64 dispatch, memory, publication, calls, edge-copy, and
global-load materialization surfaces named by the dirty route.

Actions:

- Inventory each seam named in the source idea.
- For each seam, record the expected semantic owner and the evidence that made
  it visible.
- Mark which seams already have partial dirty implementation and which are only
  next-blocker candidates.
- Keep the `00196` runtime mismatch separate unless evidence shows it shares a
  dispatch/prepared-publication seam.

Completion check:

- `todo.md` maps every visible seam to a likely owner surface and evidence
  source.

### Step 3: Split The Monolithic Probe Into Focused Cases

Goal: choose or extract focused probes so each seam can be proven separately.

Primary target: `tests/backend/case/` and existing backend route tests.

Actions:

- Search for existing focused backend cases that already isolate each seam.
- For seams without coverage, propose one focused case file per contract using
  the source idea candidate names as defaults.
- Keep the monolithic dispatch test as integration coverage, not as the only
  proof for a repair.
- If a seam cannot be expressed in the backend case harness, record the exact
  harness gap.

Completion check:

- `todo.md` lists the selected or proposed focused probe for each seam, with
  any harness gaps called out explicitly.

### Step 4: Bind Each Focused Probe To One Backend Owner

Goal: prevent future packets from touching unrelated surfaces.

Primary target: selected probes plus their corresponding owner files/helpers.

Actions:

- For each focused probe, name the backend file or helper family that should
  own the repair.
- Define the smallest acceptable proof command for that seam.
- Flag dependencies between seams only when one helper boundary genuinely
  requires another.
- Identify the narrowest next implementation packet.

Completion check:

- The next implementation packet has a selected probe, owner surface, proof
  command, and explicit do-not-touch boundaries.

### Step 5: Resume Implementation On The Narrowest Generic Seam

Goal: hand execution back to implementation only after decomposition creates a
safe route.

Primary target: the seam selected in Step 4.

Actions:

- Implement only the selected generic seam.
- Run build proof and the selected focused probe.
- Run the four-test focused proof only as integration confirmation, not as the
  only evidence.
- Stop and decompose again if the first bad fact moves to another unrelated
  surface.

Completion check:

- The selected focused probe passes without reject signals, and
  `todo.md` records whether the integration dispatch failure family shrank or
  exposed another separate seam.

### Step 6: Return To Idea 58 Recovery Criteria

Goal: reconnect decomposition results to the original recovery target.

Primary target: idea 58 focused four-test set.

Actions:

- After focused seams are repaired, rerun the supervisor-selected four-test
  command.
- Confirm whether `backend_aarch64_instruction_dispatch` is no longer blocked
  by the decomposed prepared-publication family.
- Leave `c_testsuite_aarch64_backend_src_00196_c` to the idea 58
  `78730af2f` path unless decomposition proved shared ownership.

Completion check:

- The supervisor can switch back to idea 58 with a narrower implementation
  route, or close this decomposition idea only after each seam has a focused
  owner/probe and the next recovery packet is no longer monolithic.

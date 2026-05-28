# AArch64 Prepared Authority Regression Recovery Runbook

Status: Active
Source Idea: ideas/open/58_aarch64_prepared_authority_regression_recovery.md

## Purpose

Resume the AArch64 prepared-authority regression recovery after idea 60 split
and accepted the dispatch/prepared-publication seam work.

## Goal

Repair the remaining `c_testsuite_aarch64_backend_src_00196_c` runtime
mismatch while preserving the prepared-fact ownership model and keeping the
already accepted dispatch, dynamic-stack, and `00207` recoveries green.

## Core Rule

Repair the semantic AArch64 lowering or prepared-fact consumption fault. Do
not key behavior to testcase names, exact labels, temporary names, or fixed
stack offsets, and do not weaken any target test contract without explicit
approval.

## Read First

- `ideas/open/58_aarch64_prepared_authority_regression_recovery.md`
- `ideas/closed/60_aarch64_dispatch_prepared_publication_decomposition.md`
- `todo.md`
- `test_after.log`
- `log/baseline_83d5470731cc32247c364d73ad03d8ddc478ec90.log`
- `log/baseline_78730af2f15da3f272aaa7d138bffd886908cbd0.log`

## Current Targets

- Remaining failing target:
  `c_testsuite_aarch64_backend_src_00196_c`, still failing with the known
  `joe() && fred()` runtime mismatch.
- Must-stay-green targets:
  `backend_aarch64_instruction_dispatch`,
  `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`, and
  `c_testsuite_aarch64_backend_src_00207_c`.
- Likely remaining family: the `78730af2f` boundary from idea 58.

## Non-Goals

- Do not reopen the closed idea 60 dispatch/prepared-publication seams unless
  the `00196` investigation proves a shared semantic owner.
- Do not re-open unrelated architecture backends.
- Do not broad-rewrite ALU, memory, dispatch, calls, comparison, or variadic
  lowering unless regression evidence proves the shared prepared authority
  contract is wrong.
- Do not mark any target test unsupported or weaken its expected behavior.

## Working Model

Idea 60 retired the monolithic dispatch/prepared-publication route: dispatch,
dynamic-stack fixed-slot FP anchoring, and `00207` now pass in the focused
four-test proof. The remaining red target is `00196`, which the idea 60 review
kept separate as the later `78730af2f` runtime-mismatch family.

## Execution Rules

- Keep each implementation packet narrowly tied to the remaining `00196`
  family or to a shared prepared-fact contract proven by evidence.
- Prefer shared prepared facts or a small semantic prepared lookup over
  restoring duplicate local scans as permanent authority.
- Add or adjust focused coverage only when it captures the repaired semantic
  capability, not the exact failing testcase shape.
- Run build proof before any code-changing packet is acceptance-ready.
- Run the focused four-test proof as integration confirmation after each
  repair; dispatch, dynamic-stack, and `00207` must stay green.
- Escalate to the supervisor for broader AArch64 regression guard once the
  focused four-test set is green.

## Steps

### Step 1: Confirm The Returned Failure Shape

Goal: establish the post-idea-60 focused state before changing code.

Primary target: the four-test focused failure set.

Actions:

- Read `test_after.log` and the latest `todo.md` summary.
- Confirm dispatch, dynamic-stack fixed-slot FP anchoring, and `00207` pass.
- Confirm `00196` still fails with the known `joe() && fred()` runtime
  mismatch.
- Identify nearby same-feature probes that should guard against testcase
  overfit for the remaining boolean/control-flow runtime behavior.

Completion check:

- `todo.md` records the focused command, observed remaining failure shape, and
  nearby same-feature probes for the next repair packet.

### Step 2: Re-Inspect The `78730af2f` Boundary

Goal: explain the remaining `00196` failure first visible at `78730af2f`.

Primary target: AArch64 prepared ALU/load-local or related c-testsuite runtime
behavior for `00196`.

Actions:

- Compare behavior and relevant diffs before and after `78730af2f`.
- Determine whether the runtime mismatch is a control-flow, boolean value,
  prepared load-local, or ALU publication-consumption fault.
- Check nearby same-feature behavior before treating `00196` as complete.
- Keep the closed idea 60 seam work as context only unless evidence proves a
  shared semantic owner.

Completion check:

- `todo.md` records the remaining semantic owner and proof target for the next
  packet, or names the exact blocker if ownership is still ambiguous.

### Step 3: Repair The Remaining `00196` Family

Goal: fix the semantic fault responsible for `00196`.

Primary target: the AArch64 lowering or prepared-fact consumption path
identified in Step 2.

Actions:

- Implement the smallest semantic repair needed for the remaining family.
- Preserve the prepared-authority direction.
- Avoid testcase-name matching, exact temporary-name matching, literal labels,
  and fixed stack-offset authority.
- Run build proof, the `00196` target, and any nearby same-feature checks
  selected by the supervisor.

Completion check:

- `00196` and its selected nearby same-feature probes pass without weakening
  contracts or adding testcase-shaped matching.

### Step 4: Prove The Focused Regression Set

Goal: prove all four original failures are repaired together.

Primary target: the four-test focused failure set.

Actions:

- Run the matching focused command for all four target tests.
- Inspect output for hidden expectation downgrades or unsupported-path drift.
- Record proof in `test_after.log` if delegated as the executor proof log.

Completion check:

- All four focused failures pass in the same proof run, and `todo.md` records
  the command and result.

### Step 5: Run Broader AArch64 Regression Guard

Goal: show the repairs did not introduce new failures in the chosen AArch64
backend scope.

Primary target: supervisor-selected regression guard or equivalent matching
before/after proof for AArch64 backend coverage.

Actions:

- Run the broader proof chosen by the supervisor, preferably a matching
  before/after regression guard when acceptance is being evaluated.
- Compare failures against the pre-repair baseline.
- Record unrelated remaining failures separately in `todo.md`.

Completion check:

- The broader proof shows no new failures in scope, or any remaining red tests
  are explicitly classified as unrelated to this source idea.

### Step 6: Closure Readiness Review

Goal: prepare the source idea for closure only if its acceptance criteria are
actually satisfied.

Actions:

- Confirm the four target tests pass.
- Confirm the broader AArch64 proof is acceptable.
- Confirm `todo.md` explains which boundary was responsible for each failing
  family: `c92708627`, `78730af2f`, or a shared later interaction.
- Confirm no reject signal from the source idea applies to the final diff.

Completion check:

- The supervisor can route to plan-owner close review with complete proof, or
  `todo.md` clearly names the remaining blocker.

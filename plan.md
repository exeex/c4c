# AArch64 External Libc Call-Result Publication Runbook

Status: Active
Source Idea: ideas/open/377_aarch64_external_libc_call_result_publication.md
Activated From: ideas/open/295_backend_regex_failure_family_inventory.md

## Purpose

Repair the focused AArch64 call-result publication boundary selected from the
idea 295 residual inventory, starting with the `00187` post-`fread` stale
stack-home comparison.

## Goal

Ensure scalar return values from external/libc calls are published from the
AAPCS64 return register to the scalar consumer that follows the call.

## Core Rule

Fix the semantic call-result publication path. Do not special-case `00187`,
`fread`, one stack offset, or one emitted instruction sequence, and do not
change expectations, runners, registrations, timeout policy, or proof-log
policy.

## Read First

- `ideas/open/377_aarch64_external_libc_call_result_publication.md`
- `todo.md`
- generated `build/c_testsuite_aarch64_backend/00187.c.s`
- source `tests/c/external/c-testsuite/src/00187.c`
- adjacent open ideas only for guardrails, not as implementation scope

## Current Scope

- scalar return values from external calls after AArch64 `bl`
- libc/file-call return counts such as `fread`
- immediate scalar consumers: comparison, store, branch, or local home
- external representative `c_testsuite_aarch64_backend_src_00187_c`

## Non-Goals

- Recursive caller-clobbered argument preservation.
- Direct argument/formal publication or fixed-formal entry publication.
- Function return epilogue clobbering when returning from the current
  function.
- Scalar FP expression materialization.
- Aggregate initializer/relocation or function-pointer-table behavior.
- Dynamic stack/VLA and shift/type-promotion timeout work.
- Expectation, unsupported-classification, runner, CTest registration,
  timeout, or proof-log changes.

## Working Model

The current first bad fact is inside `main` after the external `fread` call:
the call result should be available in `x0`, but generated code compares a
stale stack home (`[sp, #96]`) against `6`. The repair should connect the
prepared call-result value to the selected scalar consumer after `bl`, without
confusing it with caller argument preservation or current-function return
epilogue publication.

## Execution Rules

- Start from focused local/backend coverage for external-call result
  consumption before claiming `00187` progress.
- Inspect prepared value IDs, selected AArch64 values, and emitted code around
  the call and immediate consumer.
- Keep the implementation general for scalar external-call results and scalar
  consumers.
- Treat adjacent failures as guardrails only unless fresh first-bad-fact
  evidence proves the same owner.
- After each code slice, run the supervisor-delegated build/proof command and
  record results in `todo.md`.

## Steps

### Step 1: Localize Call-Result Publication

Goal: identify the concrete handoff where the external call return value loses
connection to the following scalar consumer.

Primary target: prepared/selected AArch64 state and generated code around
`00187`'s `fread` call.

Actions:

- Inspect the prepared call node, result value, selected machine node, and
  emitted `bl fread` neighborhood.
- Identify whether the stale stack read is introduced during call lowering,
  selected value publication, stack-home assignment, or consumer materializing.
- Compare with any existing focused backend tests for call-result consumption
  and add the smallest missing coverage if needed.
- Record the exact first bad fact and proof command in `todo.md`.

Completion check:

- `todo.md` names the specific publication boundary and the focused proof
  target for the implementation slice.

### Step 2: Repair Scalar External-Call Result Publication

Goal: publish the scalar return value from the ABI return register to the
selected scalar consumer after the call.

Primary target: the AArch64 call-result lowering/publication path localized in
Step 1.

Actions:

- Implement the narrow semantic repair for scalar external-call results.
- Keep argument publication, recursive preservation, current-function return
  epilogues, FP expression materialization, and aggregate paths unchanged
  unless directly required by the localized boundary.
- Ensure the consumer sees the call result value rather than an uninitialized
  or stale stack home.
- Preserve nearby guardrail behavior selected by the supervisor.

Completion check:

- Focused backend proof for scalar external-call result consumption passes or
  advances to a new first bad fact without expectation or runner changes.

### Step 3: Prove External Representative

Goal: verify that `00187` advances past the stale post-`fread` comparison.

Primary target: `c_testsuite_aarch64_backend_src_00187_c`.

Actions:

- Regenerate and inspect `00187.c.s` around `fread`.
- Confirm the comparison against `6` consumes the published return count from
  the call result path.
- Run the supervisor-delegated focused c-testsuite proof.
- If `00187` exposes a new first bad fact, record it without widening this
  idea.

Completion check:

- `todo.md` records the focused proof result and whether `00187` passed or
  advanced to a new owner boundary.

### Step 4: Broader Guard And Handoff

Goal: prove the repair did not regress adjacent backend routes and prepare the
next lifecycle decision.

Primary target: supervisor-selected backend guardrail subset, with broader
backend-regex validation if requested.

Actions:

- Run the exact broader guard command delegated by the supervisor.
- Confirm no expectation, unsupported, runner, timeout, CTest registration, or
  proof-log policy changes were used as progress.
- Summarize remaining residuals or new first bad facts in `todo.md`.
- Ask plan-owner to close, park, or split only after source-idea completion is
  actually proven.

Completion check:

- `todo.md` contains accepted proof and a lifecycle recommendation for idea
  377.

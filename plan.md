# AArch64 Recursive Call Argument Preservation Runbook

Status: Active
Source Idea: ideas/open/349_aarch64_recursive_call_argument_preservation.md

## Purpose

Refresh and complete the recursive/nested-call argument-preservation owner
without expanding it into adjacent AArch64 publication or aggregate-writeback
work.

## Goal

Confirm whether generated AArch64 still consumes stale caller-clobbered
argument registers after a nested or recursive `bl`, and repair only that
general preservation boundary if fresh evidence appears.

## Core Rule

Do not special-case `00176`, `00181`, `quicksort`, `Hanoi`, one argument
number, one register name, or one emitted instruction neighborhood. Progress
requires a semantic call-boundary preservation or reload repair.

## Read First

- `ideas/open/349_aarch64_recursive_call_argument_preservation.md`
- AArch64 call argument publication and preservation lowering code reached by
  the focused backend tests.
- Existing focused backend tests covering recursive or nested calls that use a
  caller-side value after a `bl`.

## Scope

- Localize live caller-side values that must survive recursive or nested calls.
- Repair post-call reload, caller-save spill, or preserved-home handoff when a
  later use currently reads only a caller-clobbered argument register.
- Use `00176` and `00181` as representatives only after the general owner is
  identified.

## Non-Goals

- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log behavior.
- Do not broaden into indexed aggregate selected-address/writeback, block
  label ordering, frame-slot layout, unsigned div/rem producer publication,
  semantic BIR pointer loads, or string literal pointer publication without a
  separate lifecycle split.
- Do not claim progress from helper renames, diagnostics, generated-text
  reshuffling, or classification-only updates.

## Working Model

The source idea is a parked completion candidate. Previous refresh evidence
showed `00176` and `00181` passing and no current representative
caller-clobbered recursive argument failure. This runbook first rechecks that
state. If no owned failure remains, the executor should report close-ready
evidence instead of inventing implementation work.

## Execution Rules

- Keep routine progress and proof notes in `todo.md`.
- If a fresh first bad fact belongs to another owner, report the split target
  instead of widening this plan.
- For any code-changing packet, run build proof plus the supervisor-delegated
  focused CTest subset before reporting completion.
- Treat testcase-shaped matching or expectation downgrades as blocking route
  drift.

## Ordered Steps

### Step 1: Refresh Representative Evidence

Goal: Determine whether the recursive-call argument-preservation failure still
exists.

Primary Target: generated AArch64 for `00176` and `00181` plus focused backend
call-preservation guardrails.

Actions:

- Run the supervisor-delegated focused proof command:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c)$'`
- If either representative fails, inspect the generated AArch64 around nested
  or recursive `bl` sites and post-call uses of original caller-side argument
  values.
- Classify the first bad fact as one of:
  - in-scope stale caller-clobbered post-call argument consumption;
  - a different already-split owner;
  - no current owned failure.
- Record the proof result and first-bad-fact classification in `todo.md`.

Completion Check:

- The executor can state whether an in-scope stale recursive or nested-call
  argument preservation failure remains, with the exact proof command and
  result recorded.

### Step 2: Repair Only If Owned Failure Reappears

Goal: Fix the general call-boundary preservation path if Step 1 finds an
in-scope failure.

Primary Target: the AArch64 publication, preserved-home, caller-save, or
post-call reload path responsible for making a live caller-side value available
after `bl`.

Actions:

- Localize where the prepared live value should be reloaded or published after
  a call.
- Add or extend focused backend coverage for a recursive or nested-call value
  used after `bl`.
- Implement the narrow semantic repair without matching filenames, function
  names, register spellings, or emitted instruction neighborhoods.
- Prove the focused backend coverage and representative command requested by
  the supervisor.

Completion Check:

- Focused coverage fails without the repair and passes with it, and generated
  post-call consumers no longer read stale caller-clobbered argument registers.

### Step 3: Close-Readiness Handoff

Goal: Provide enough lifecycle evidence for the supervisor and plan owner to
  close or park the source idea correctly.

Actions:

- If Step 1 finds no owned failure, report the green proof and state that this
  idea is close-ready pending the supervisor's regression-guard policy.
- If Step 2 repaired a fresh owned failure, report the exact code/test slice
  and proof logs.
- If the first bad fact belongs elsewhere, identify the owning idea or the need
  for a new `ideas/open/` split.

Completion Check:

- `todo.md` contains the latest proof result, first-bad-fact classification,
  and a clear supervisor next action: close, park, split, or continue.

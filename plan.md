# AArch64 C-Testsuite 00205 Value Materialization Residual Runbook

Status: Active
Source Idea: ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md
Switched From: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md after timeout repair exposed an output mismatch

## Purpose

Classify and repair the now-reachable `00205` output mismatch without
reopening the repaired timeout, sign-extension legality, or test-policy routes.

## Goal

Find the semantic generated-code reason `00205` reads or prints wrong case
field values after the timeout repair.

## Core Rule

Treat this as stack-frame, aggregate, address-computation, or value
materialization generated-code work. Do not change expectations, allowlists,
unsupported classifications, timeout policy, runner behavior, proof-log policy,
or CTest registration to claim progress.

## Read First

- Source idea:
  `ideas/open/305_aarch64_ctestsuite_00205_value_materialization_residual.md`
- Parked timeout owner:
  `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`
- Accepted timeout result: `00205` completes quickly and fails output
  comparison instead of timing out.
- Accepted legality result: generated assembly contains legal `sxtw x9, w13`,
  not illegal `sxtw w9, w13`.
- Current residual evidence: generated code reads case fields from offsets
  such as `[sp, #632]`, `[sp, #1064]`, and `[sp, #1496]` even though the
  prologue reserves only `sub sp, sp, #48`.

## Current Targets

- `c_testsuite_aarch64_backend_src_00205_c`
- Generated AArch64 assembly and runtime output for the post-timeout mismatch
- Stack-frame layout, local aggregate materialization, address computation,
  spill/reload, and value-copy semantics connected to the bad case-field reads

## Non-Goals

- Do not reopen the branch/compare timeout route unless the same timeout
  mechanism returns.
- Do not reopen sign-extension legality unless illegal `sxtw` destination
  width returns.
- Do not claim pass-count progress from classification-only work.
- Do not alter timeout thresholds, runner behavior, stale-process policy,
  expectations, allowlists, unsupported classifications, proof-log policy, or
  CTest registration.
- Do not perform a broad ABI, stack-frame, aggregate, or instruction-selection
  rewrite before classifying the mismatch mechanism.

## Working Model

The previous failure mode was non-terminating generated control flow caused by
loop-bound compare lowering. The current failure mode is wrong output after
normal completion. The first packet should identify whether the high stack
offset reads are caused by frame-size accounting, aggregate/local placement,
address calculation, value copy lowering, or another concrete backend route.

## Execution Rules

- Start from generated `00205` assembly and the observed runtime mismatch.
- Keep probes narrow and tied to the stack offsets, frame layout, and value
  flow that feed the printed case fields.
- Preserve idea 303 sign-extension legality and idea 304 timeout repair.
- If the mismatch belongs to a broader semantic family, record the evidence and
  request lifecycle split instead of absorbing the whole family here.
- Record fresh build proof and the supervisor-selected focused subset in
  `todo.md`.

## Steps

### Step 1: Classify Mismatch Mechanism

Goal: identify why generated `00205` reads or prints wrong case-field values
after the timeout path is repaired.

Primary target: generated assembly, frame layout, and value flow for focused
`00205`

Actions:

- Inspect the generated assembly around the case aggregate initialization,
  address formation, loads, stores, and printed field values.
- Explain why offsets such as `[sp, #632]`, `[sp, #1064]`, and `[sp, #1496]`
  are emitted despite a `sub sp, sp, #48` prologue, or replace that hypothesis
  with a better concrete mechanism.
- Run bounded probes only as needed to determine whether the cause is
  frame-size accounting, aggregate/local materialization, address calculation,
  spill/reload, value-copy lowering, or another backend semantic.
- Verify generated assembly still uses legal sign-extension spelling and still
  emits conditional loop-header compares.

Completion check:

- `todo.md` records the concrete output-mismatch mechanism or records why the
  evidence requires a different split owner.

### Step 2: Repair Or Split The Value Owner

Goal: fix the classified generated-code cause, or hand it to a more precise
owner if the evidence is broader than this singleton.

Primary target: the backend route identified in Step 1

Actions:

- If the mismatch has a focused semantic cause, repair that backend route
  without testcase-shaped matching.
- Add or update focused backend unit coverage for the semantic route when the
  repair touches shared lowering code.
- If the mismatch belongs to a broader family, stop and request lifecycle split
  with the concrete evidence from Step 1.
- Build and prove the supervisor-selected focused subset.

Completion check:

- The focused proof no longer fails for the classified output-mismatch reason,
  or the lifecycle handoff names the broader owner and preserves the evidence.

### Step 3: Residual Proof And Handoff

Goal: separate this owner's result from any later residual exposed by the same
testcase.

Primary target: focused proof results after repair or split

Actions:

- Re-run the focused proof selected by the supervisor.
- Preserve the sign-extension legality result and timeout-repair result in the
  proof notes.
- Record any later residual separately without claiming unrelated pass-count
  progress.

Completion check:

- `todo.md` records proof for the value-materialization classification/repair
  and names any remaining residual bucket without expanding this owner
  silently.

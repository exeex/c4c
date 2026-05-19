# AArch64 C-Testsuite 00205 Timeout Residual Runbook

Status: Active
Source Idea: ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md
Switched From: ideas/open/303_aarch64_sign_extension_assembler_legality.md after close-gate proof was unavailable

## Purpose

Classify and repair the focused `00205` timeout exposed after the
sign-extension assembler-legality repair.

## Goal

Find the semantic generated-code reason `00205` times out after assembler
validation now succeeds.

## Core Rule

Treat this as timeout/runtime generated-code classification and repair. Do not
change expectations, allowlists, unsupported classifications, timeout policy,
runner behavior, proof-log policy, or CTest registration to claim progress.

## Read First

- Source idea: `ideas/open/304_aarch64_ctestsuite_00205_timeout_residual.md`
- Parked legality owner:
  `ideas/open/303_aarch64_sign_extension_assembler_legality.md`
- Accepted sign-extension result: generated assembly contains legal
  `sxtw x9, w13`, not illegal `sxtw w9, w13`
- Current residual: focused `00205` times out after 5.01 seconds

## Current Targets

- `c_testsuite_aarch64_backend_src_00205_c`
- Generated AArch64 assembly and runtime behavior for the post-assembly
  timeout path
- Compare, branch, loop, value-flow, or materialization semantics only if
  evidence connects them to the timeout

## Non-Goals

- Do not reopen sign-extension legality unless illegal `sxtw` destination
  width returns.
- Do not claim pass-count progress from classification-only work.
- Do not alter timeout thresholds, runner behavior, stale-process policy,
  expectations, allowlists, unsupported classifications, proof-log policy, or
  CTest registration.
- Do not perform a broad backend rewrite before classifying the timeout
  mechanism.

## Working Model

The previous failure mode was assembler rejection of illegal sign-extension
assembly. The current failure mode is later runtime non-termination or timeout.
The first packet should identify whether generated control flow, compare
lowering, branch lowering, value materialization, or another concrete backend
semantic route causes the binary to keep running.

## Execution Rules

- Start from the generated assembly and runtime observation for `00205`.
- Keep probes narrow and timeout-bounded.
- Preserve the idea 303 legality result while investigating.
- If the timeout belongs to a broader semantic family, record the evidence and
  request a lifecycle split instead of absorbing the whole family here.
- Record fresh build proof and the supervisor-selected focused subset in
  `todo.md`.

## Steps

### Step 1: Inspect Timeout Mechanism

Goal: identify why the generated `00205` binary times out after assembler
validation succeeds.

Primary target: generated assembly and runtime behavior for focused `00205`

Actions:

- Inspect the generated assembly around the loop, compare, branch, and value
  updates involved in the timed-out path.
- Run bounded probes only as needed to determine whether the timeout is caused
  by generated control-flow, compare/branch lowering, value materialization, or
  another concrete backend semantic.
- Verify the generated assembly still uses legal sign-extension spelling such
  as `sxtw x9, w13`.

Completion check:

- `todo.md` records the concrete timeout mechanism or records why the evidence
  requires a different split owner.

### Step 2: Repair Or Split The Semantic Owner

Goal: fix the classified generated-code cause, or hand it to a more precise
owner if the evidence is broader than this singleton.

Primary target: the backend route identified in Step 1

Actions:

- If the timeout has a focused semantic cause, repair that backend route
  without testcase-shaped matching.
- If the timeout belongs to a broader family, stop and request lifecycle split
  with the concrete evidence from Step 1.
- Build and prove the supervisor-selected focused subset.

Completion check:

- The focused proof no longer times out for the classified reason, or the
  lifecycle handoff names the broader owner and preserves the evidence.

### Step 3: Residual Proof And Handoff

Goal: separate this owner's result from any later residual exposed by the same
testcase.

Primary target: focused proof results after repair or split

Actions:

- Re-run the focused proof selected by the supervisor.
- Preserve the sign-extension legality result in the proof notes.
- Record any later residual separately without claiming unrelated pass-count
  progress.

Completion check:

- `todo.md` records proof for the timeout classification/repair and names any
  remaining residual bucket without expanding this owner silently.

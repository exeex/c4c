# AArch64 Synthetic Select Label Uniqueness Refresh Runbook

Status: Active
Source Idea: ideas/open/364_aarch64_synthetic_select_label_uniqueness.md

## Purpose

Refresh the parked synthetic select-label uniqueness idea and decide whether
current tree behavior still has an in-scope duplicate-label failure, advances
to the already recorded signed-remainder residual, or is closure-ready under a
fresh guard.

## Goal

Prove the AArch64 synthetic select/materialized-label allocation path remains
unique across multiple emitted regions, without reopening adjacent block-order
or scalar-cast owners.

## Core Rule

Do not treat runtime value failures in `00143` as synthetic label progress
unless generated assembly still has duplicate `.Lselect_mat_*` definitions or
focused backend coverage exposes a general synthetic-label collision.

## Read First

- `ideas/open/364_aarch64_synthetic_select_label_uniqueness.md`
- Generated assembly for `c_testsuite_aarch64_backend_src_00143_c` when the
  representative is rebuilt.
- Existing backend coverage around AArch64 synthetic select/materialized-label
  emission and instruction dispatch.

## Current Scope

- AArch64 synthetic select/materialized-label allocation and formatting.
- Duplicate true/end label detection within one generated assembly file.
- The external `00143` representative only as proof after focused coverage or
  generated-assembly evidence identifies the same owner.

## Non-Goals

- Do not repair signed remainder, pointer comparisons, aggregate writeback,
  scalar FP, conditional operators, file I/O return publication, `sizeof`,
  aggregate initializer layout, enum bit-field layout, or timeout residuals.
- Do not reopen block label emission ordering, fallthrough epilogue placement,
  scalar-cast source publication, or local storage/writeback sizing without
  fresh first-bad-fact evidence.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, or assembler/test
  contracts.

## Working Model

The source idea was parked after synthetic select labels became unique and
`00143` advanced past duplicate-label assembly failure into a runtime failure.
This refresh should verify whether the current tree still satisfies that
boundary. If duplicate labels are still absent, the supervisor can either
accept a closure attempt under a matching regression guard or hand off the
separate runtime first bad fact under a different source idea.

## Execution Rules

- Use generated-code evidence before claiming the synthetic-label owner is
  live or absent.
- Prefer focused backend coverage before relying on the external
  c-testsuite representative.
- Keep `todo.md` as the execution scratchpad; do not edit this plan for
  routine packet progress.
- If the representative fails only after assembly succeeds, classify the new
  first bad fact instead of expanding this source scope.

## Ordered Steps

### Step 1: Refresh Duplicate-Label Evidence

Goal: Rebuild the current tree and inspect whether synthetic select labels can
still collide.

Primary target: generated AArch64 assembly and focused backend tests for
synthetic select/materialized-label emission.

Actions:

- Run a supervisor-selected focused build and test subset that includes the
  synthetic select/materialized-label backend coverage and
  `c_testsuite_aarch64_backend_src_00143_c`.
- Inspect the generated `00143.c.s` file for duplicate `.Lselect_mat_*`
  definitions if the representative builds far enough to emit assembly.
- Record whether the old duplicate-label assembler failure is absent,
  present, or replaced by a later failure.

Completion check:

- `todo.md` records the exact proof command, result, and generated-assembly
  duplicate-label finding.

### Step 2: Repair Only If The Collision Is Live

Goal: Repair the general synthetic-label allocation path only if Step 1 proves
the old owner is live again.

Primary target: the AArch64 select/materialized-label naming or allocation
boundary identified by Step 1.

Actions:

- Localize the collision to label allocation, label formatting, or repeated
  emission-site identity.
- Repair uniqueness across all relevant select/materialized-label regions
  without hard-coding `00143`, one block, one instruction index, or one label
  suffix.
- Add or restore focused backend coverage that would fail on duplicate
  synthetic labels.

Completion check:

- Focused coverage proves multiple synthetic select/materialized-label regions
  cannot collide, and `00143` no longer fails with duplicate
  `.Lselect_mat_*` labels.

### Step 3: Classify Or Close The Refreshed Boundary

Goal: Decide the lifecycle outcome for source idea 364 after the refreshed
proof.

Primary target: source-scope completion, not unrelated runtime residuals.

Actions:

- If duplicate labels are absent and focused proof is green, report the idea as
  closure-ready for the plan owner close gate.
- If `00143` fails after assembly, classify the new first bad fact without
  implementing it under this plan.
- If a new first bad fact is a separate initiative and no matching open idea
  exists, ask the supervisor to route lifecycle creation rather than expanding
  this source idea.

Completion check:

- `todo.md` records whether the source scope is satisfied, blocked by a live
  synthetic-label collision, or requires a separate lifecycle handoff.

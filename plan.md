# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Step 7.21 legacy `LirIndirectBr` receipt (`b528dc1`),
the durable pause record in the source idea (`78eae8fe5`), and closed idea 755
conditional-condition authority handoff (`c3d759cb8`, implementation
`ae006a0f1`)

## Purpose

Resume the Raw-BIR receiver route without resetting accepted Steps 1 through
7.21. Receive exactly the newly authorized typed `LirCondBr` CFG row.

## Goal

Map one existing typed conditional branch to a verified, target-independent
Raw-BIR conditional-jump destination without presentation recovery or partial
publication.

## Core Rule

Use only `LirCondBr.condition` as the current-function `LirValueId` and the
typed `true_successor` and `false_successor` `LirBlockId`s. `cond_name`,
labels, printer output, and rendered text are never semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/755_lir_conditional_branch_condition_identity_publication.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- existing Raw-BIR conditional-jump container, builder/view, verifier, and
  LIR-to-Raw-BIR terminator dispatch
- `src/backend/bir/lir_to_bir/README.md` terminator matrix

## Landed Progress

- Steps 1 through 7.21 are accepted historical work; do not repeat them.
- Step 6.3 received direct `LirBr` (`97efe9c38`), Step 7.20 received the
  selected `LirMemcpyOp` row (`1a3adbc58`), and Step 7.21 received legacy
  `LirIndirectBr` (`b528dc1`).
- Closed 750 established typed successor authority (`0f214dc`, `40673da`) and
  closed 755 established `LirCondBr.condition` authority (`ae006a0f1`).
- Prior accepted proof includes the matching backend guard 5/5 for Step 7.21,
  3034/3034 full checkpoint after 750, and 755's fresh build, direct frontend
  proof, matching backend 5/5 guard, and broader 35/35 proof.

## Non-Goals

- no LIR producer/schema changes, presentation recovery, canonicalization,
  target lowering, MIR, emission, or legacy-BIR revival
- no `LirSwitch` or `LirIndirectBrOp` receiver work; selector/address identity
  remains unsupported and fail closed
- no PHI, local/object, memory/va, aggregate/vector, body-parameter, or other
  remaining receiver family

## Execution Rules

1. Implement only one `LirCondBr` receiving container/wiring path.
2. Preserve typed condition and same-function true/false destination ownership;
   reject absent, invalid, foreign, duplicate/ambiguous, non-boolean, or
   incoherent authority before Raw-BIR publication.
3. Add reachable verification and nearby positive/negative transactional
   coverage with no partial module on failure.
4. Run a fresh build and narrow proof. Leave broader/full acceptance and
   regression-log handling to the supervisor.

## Ordered Steps

### Step 7.22 - Receive typed conditional-branch authority

Goal: receive exactly `LirCondBr.condition` and its typed current-function
true/false successors into a Raw-BIR conditional-jump destination.

Primary targets:

- the smallest existing/new Raw-BIR conditional-jump container, builder/view,
  and verifier seams required by this row
- LIR-to-Raw-BIR terminator dispatch and transactional module boundary
- focused receiver coverage plus malformed-authority neighbours

Actions:

- map only `LirCondBr.condition` as `LirValueId` and `true_successor` /
  `false_successor` as ordered branch destinations; do not inspect `cond_name`
  or labels for semantics
- validate condition presence, validity, current-function ownership, boolean
  suitability, successor presence/ownership, distinctness/ambiguity, and
  destination coherence before publication
- prove one valid conditional branch and missing, invalid, foreign,
  non-boolean, misleading-display, duplicate/ambiguous, or incoherent
  authority failures with whole-module rollback
- retain `LirSwitch` and `LirIndirectBrOp` as unsupported fail-closed
  neighbours; do not absorb PHI or other CFG/value families

Completion check:

- a fresh build and focused positive/negative proof establish the one typed
  transactional conditional-branch receiver without presentation recovery;
  return source completion state to plan-owner rather than inferring closure.

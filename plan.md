# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Step 7.20 selected `LirMemcpyOp` receipt (`1a3adbc58`)
and closed idea 750 CFG successor-authority handoff (`f372b29`, lifecycle
pointer `62be1f1`)

## Purpose

Resume the Raw-BIR receiver route without resetting the accepted Steps 1
through 7.20 history. Receive exactly the next source-authorized legacy
indirect-branch row.

## Goal

Map one existing typed legacy `LirIndirectBr` CFG fact to a verified,
target-independent Raw-BIR indirect-jump destination without presentation
recovery or partial publication.

## Core Rule

Use only `LirIndirectBr.addr` as `LirValueId` and its ordered
current-function `LirBlockId` targets. Labels and printer text are never
semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- existing Raw-BIR indirect-jump container, builder, verifier, and importer
- `src/backend/bir/lir_to_bir/README.md` terminator matrix

## Landed Progress

- Steps 1 through 7.20 are accepted historical work; do not repeat them.
- Step 6.3 already received the direct `LirBr` row (`97efe9c38`).
- Closed idea 750 established typed active-successor authority with accepted
  commits `0f214dc` and `40673da`, focused 1/1 pre/post guard, and fresh full
  3034/3034 proof.

## Non-Goals

- no LIR producer/schema changes, label-text recovery, canonicalization,
  target lowering, MIR, emission, or legacy-BIR revival
- no `LirCondBr`, `LirSwitch`, or `LirIndirectBrOp` receiver work: their
  condition, selector, or address identities remain outside this packet and
  fail closed
- no PHI, memory/va, local/object, aggregate/vector, body-parameter, or other
  remaining receiver family

## Execution Rules

1. Implement only one legacy `LirIndirectBr` receiving container/wiring path.
2. Preserve the typed address, current-function target ownership, and target
   order; reject absent, invalid, foreign, duplicate/ambiguous, or incoherent
   authority before Raw-BIR publication.
3. Add reachable verification and nearby positive/negative transactional
   coverage with no partial module on failure.
4. Run a fresh build and narrow proof. Leave broader/full acceptance and
   regression-log handling to the supervisor.

## Ordered Steps

### Step 7.21 - Receive legacy indirect-branch authority

Goal: receive exactly legacy `LirIndirectBr`'s typed address and ordered CFG
targets into a Raw-BIR typed indirect-jump destination.

Primary targets:

- the smallest Raw-BIR indirect-jump container, builder/view, and verifier
- LIR-to-Raw-BIR terminator dispatch and transactional module boundary
- focused receiver coverage plus malformed authority neighbours

Actions:

- map only the existing `LirValueId` address and ordered current-function
  `LirBlockId` targets; preserve order without inspecting labels
- validate address/target presence, ownership, uniqueness/ambiguity, and
  destination coherence before publication
- prove one valid indirect branch and malformed missing, invalid, duplicate or
  ambiguous, and foreign-authority failures with whole-module rollback
- retain `LirCondBr`, `LirSwitch`, and `LirIndirectBrOp` as unsupported,
  fail-closed neighbours in this packet

Completion check:

- a fresh build and focused positive/negative proof establish the one typed
  transactional legacy-indirect receiver without display-text recovery; return
  the source completion state to plan-owner rather than inferring closure.

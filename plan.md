# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Step 7.22 typed `LirCondBr` receipt (`220a3b5ad`) and
closed idea 756 switch-selector authority handoff (`cafc757ec`); 756's focused
frontend LIR proof passed 4/4 with a matching non-decreasing guard and its
broader proof passed 35/35.

## Purpose

Resume the Raw-BIR receiver route without resetting accepted Steps 1 through
7.22. Receive exactly the newly authorized typed `LirSwitch` CFG row.

## Goal

Map one existing typed switch selector with its typed default and ordered case
destinations to a verified, target-independent Raw-BIR switch destination
without presentation recovery or partial publication.

## Core Rule

Use only `LirSwitch.selector` as the current-function `LirValueId` and typed
`default_successor` plus ordered `case_successors` as CFG authority.
`selector_name`, `selector_type`, labels, printer output, and rendered text
are never semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/756_lir_switch_selector_value_identity_publication.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- existing Raw-BIR switch container, builder/view, verifier, and LIR-to-Raw-BIR
  terminator dispatch
- `src/backend/bir/lir_to_bir/README.md` terminator matrix

## Landed Progress

- Steps 1 through 7.22 are accepted historical work; do not repeat them.
- Step 6.3 received direct `LirBr` (`97efe9c38`), Step 7.20 received the
  selected `LirMemcpyOp` row (`1a3adbc58`), Step 7.21 received legacy
  `LirIndirectBr` (`b528dc1`), and Step 7.22 received `LirCondBr`
  (`220a3b5ad`).
- Closed 750 established typed switch successor authority; closed 756
  established typed integer selector authority (`cafc757ec`).

## Non-Goals

- no LIR producer/schema changes, presentation recovery, canonicalization,
  target lowering, MIR, emission, or legacy-BIR revival
- no `LirIndirectBrOp`, PHI, local/object, memory/va, aggregate/vector,
  body-parameter, or other remaining receiver family
- do not absorb case interpretation beyond preserving the existing typed
  selector and ordered case facts

## Execution Rules

1. Implement only one `LirSwitch` receiving container/wiring path.
2. Preserve typed selector authority, typed default/case destination ownership,
   and case order; reject absent, invalid, foreign, non-integer, duplicate or
   ambiguous, or incoherent authority before Raw-BIR publication.
3. Add reachable verification and nearby positive/negative transactional
   coverage with no partial module on failure.
4. Run a fresh build and narrow proof. Leave broader/full acceptance and
   regression-log handling to the supervisor.

## Ordered Steps

### Step 7.23 - Receive typed switch authority

Goal: receive exactly `LirSwitch.selector` with its typed current-function
default and ordered case successors into a Raw-BIR switch destination.

Primary targets:

- the smallest existing/new Raw-BIR switch container, builder/view, and
  verifier seams required by this row
- LIR-to-Raw-BIR terminator dispatch and transactional module boundary
- focused receiver coverage plus malformed-authority neighbours

Actions:

- map only `LirSwitch.selector` as `LirValueId` and
  `default_successor`/`case_successors` as ordered destinations; do not inspect
  selector text or labels for semantics
- validate selector presence, validity, current-function ownership, integer
  suitability, successor presence/ownership, case correspondence/order,
  duplicate or ambiguous destinations, and destination coherence before
  publication
- prove one valid switch and missing, invalid, foreign, non-integer,
  misleading-display, duplicate/ambiguous, or incoherent authority failures
  with whole-module rollback
- retain `LirIndirectBrOp`, PHI, and all other unreceived families as
  unsupported fail-closed neighbours in this packet

Completion check:

- a fresh build and focused positive/negative proof establish the one typed
  transactional switch receiver without presentation recovery; return source
  completion state to plan-owner rather than inferring closure.

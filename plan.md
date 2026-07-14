# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Step 7.23 typed `LirSwitch` receipt (`0995a3deb`) and
closed idea 757 computed-goto address-authority handoff (`8527c6dbc`). The
fresh focused producer proof passed 1/1; the matching `^backend_` regression
guard passed 5/5 before and after under allow-non-decreasing.

## Purpose

Resume the Raw-BIR receiver route without resetting accepted Steps 1 through
7.23. Receive exactly the newly authorized typed `LirIndirectBrOp` CFG row.

## Goal

Map one existing typed computed-goto address with ordered typed destinations to
a verified, target-independent Raw-BIR indirect-jump destination without
presentation recovery or partial publication.

## Core Rule

Use only `LirIndirectBrOp.addr_value` as the current-function pointer
`LirValueId` and ordered `successors` as CFG authority. `addr`, labels, printer
output, and rendered text are never semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/757_lir_computed_goto_address_value_identity_publication.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- existing Raw-BIR indirect-jump container, builder/view, verifier, and
  LIR-to-Raw-BIR terminator dispatch
- `src/backend/bir/lir_to_bir/README.md` terminator matrix

## Landed Progress

- Steps 1 through 7.23 are accepted historical work; do not repeat them.
- Step 7.21 received legacy `LirIndirectBr` (`b528dc1`), Step 7.22 received
  `LirCondBr` (`220a3b5ad`), and Step 7.23 received `LirSwitch`
  (`0995a3deb`).
- Closed 750 established ordered computed-goto successor authority; closed 757
  established typed pointer `addr_value` authority (`8527c6dbc`).

## Non-Goals

- no LIR producer/schema changes, presentation recovery, canonicalization,
  target lowering, MIR, emission, or legacy-BIR revival
- no legacy `LirIndirectBr`, `LirCondBr`, `LirSwitch`, PHI, local/object,
  memory/va, aggregate/vector, body-parameter, or other remaining receiver
  family
- do not interpret the address beyond preserving its existing typed pointer
  fact and ordered targets

## Execution Rules

1. Implement only one `LirIndirectBrOp` receiving container/wiring path.
2. Preserve typed pointer address authority, typed destination ownership, and
   destination order; reject absent, invalid, foreign, non-pointer, duplicate,
   ambiguous, or incoherent authority before Raw-BIR publication.
3. Add reachable verification and nearby positive/negative transactional
   coverage with no partial module on failure.
4. Run a fresh build and narrow proof. Leave broader/full acceptance and
   regression-log handling to the supervisor.

## Ordered Steps

### Step 7.24 - Receive typed computed-goto authority

Goal: receive exactly `LirIndirectBrOp.addr_value` with its ordered
current-function successors into a Raw-BIR indirect-jump destination.

Primary targets:

- the smallest existing/new Raw-BIR indirect-jump container, builder/view, and
  verifier seams required by this row
- LIR-to-Raw-BIR terminator dispatch and transactional module boundary
- focused receiver coverage plus malformed-authority neighbours

Actions:

- map only `addr_value` as `LirValueId` and `successors` as ordered
  destinations; do not inspect address or label text for semantics
- validate address presence, validity, current-function ownership, pointer
  suitability, successor presence/ownership/order, duplicate or ambiguous
  destinations, and destination coherence before publication
- prove one valid computed goto and missing, invalid, foreign, non-pointer,
  misleading-display, duplicate/ambiguous, or incoherent authority failures
  with whole-module rollback
- retain PHI and all other unreceived families as unsupported fail-closed
  neighbours in this packet

Completion check:

- a fresh build and focused positive/negative proof establish the one typed
  transactional computed-goto receiver without presentation recovery; return
  source completion state to plan-owner rather than inferring closure.

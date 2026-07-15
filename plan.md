# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Steps 1 through 7.24; closed idea 751's typed PHI
incoming value/predecessor handoff (`6ece9fe8f`); closed idea 786's native PHI
SpecialToken authority handoff (`91b5bde43`); and closed idea 787's accepted
parallel CFG-edge occurrence handoff (`a889ce33f`, lifecycle closure
`a0a3ff33b` / `8af2421df`).

## Purpose

Resume the Raw-BIR receiver route at the unchanged Step 7.25 without resetting
accepted receiver work or accepting the preserved PHI WIP.

## Goal

Receive one structured `LirPhiOp` result and its ordered typed incoming
value/predecessor pairs into verified, target-independent Raw BIR, preserving
each exact CFG-edge occurrence, including parallel occurrences, without
presentation recovery or partial publication.

## Core Rule

Use only `LirPhiOp`'s typed result/type, closed 751's `LirPhiIncoming` value
and current-function `LirBlockId` predecessor authority, and closed 786's
native SpecialToken authority. Closed 787 establishes that duplicate typed
conditional and switch successors are distinct ordered CFG-edge occurrences.
Value spellings, labels, printer output, LLVM text, and instruction order are
never semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- the closed 751, 786, and 787 handoff records
- existing Raw-BIR value, block, instruction, builder/view, and verifier seams
- `src/backend/bir/lir_to_bir.cpp` and nearby LIR-to-Raw-BIR dispatch/tests
- `src/backend/bir/lir_to_bir/README.md` PHI and values-def-use matrix rows

## Landed Progress And Acceptance Boundary

- Steps 1 through 7.24 are accepted historical work and must not be repeated.
- Closed 751 supplies verified PHI incoming value/predecessor authority for the
  selected ternary, logical, AArch64-vaarg, and AMD64-vaarg producers.
- Closed 786 supplies native semantic authority for the relevant PHI
  `SpecialToken` operands; no spelling/classification recovery is permitted.
- Closed 787 accepts duplicate conditional and switch successor occurrences as
  distinct ordered edges. It accepted no Raw-BIR receiver work.
- Existing Raw-BIR PHI receiver worktree changes and its focused 1/1 after
  result are unaccepted WIP. Do not commit, repeat, or retroactively accept
  them as Step 7.25 evidence.

## Non-Goals

- no LIR producer/schema changes, presentation recovery, canonicalization,
  target lowering, MIR, emission, or legacy-BIR revival
- no alternate PHI producer, local/object, memory/va, aggregate/vector,
  body-parameter, or any other unreceived receiver family
- no inferred CFG edge, predecessor, or value identity; preserve only the
  typed authority already carried by the selected `LirPhiOp` row

## Execution Rules

1. Implement exactly one typed `LirPhiOp` Raw-BIR receiving path.
2. Preserve the result/type, each incoming value, and each predecessor as
   current-function typed identities; bind every incoming to the exact existing
   predecessor-to-PHI-block CFG edge occurrence, preserving multiplicity/order.
3. Treat duplicate conditional/switch successor occurrences as distinct ordered
   edges. Do not collapse repeated predecessor blocks or infer edge identity.
4. Reject absent, invalid, foreign, duplicate/ambiguous occurrence,
   type-incoherent, or predecessor/edge-mismatched authority before Raw-BIR
   publication.
5. Add reachable nearby positive/negative transactional coverage covering the
   closed-751 ternary, logical, and vaarg producer families, including exact
   parallel-edge coverage where applicable. Do not use test names or rendered
   text as matching inputs.
6. Run a fresh build and narrow proof. The supervisor owns regression logs and
   broader/full acceptance.

## Ordered Steps

### Step 7.25 - Receive typed PHI incoming authority

Goal: receive one structured `LirPhiOp` result and ordered incoming
value/predecessor authority into a typed Raw-BIR PHI container with exact
CFG-edge occurrence binding.

Primary targets:

- the smallest existing/new Raw-BIR PHI container, builder/view, and verifier
  seams required by this row
- LIR-to-Raw-BIR instruction dispatch, current-function value/block mapping,
  and transactional module boundary
- focused PHI receiver coverage plus malformed-authority neighbours

Actions:

- map only the typed `LirPhiOp` result/type and ordered `LirPhiIncoming`
  value/predecessor fields, including native SpecialToken authority; resolve
  their IDs through current-function maps and exact received terminator
  successor occurrences
- represent one typed PHI result and one typed incoming per exact edge
  occurrence; preserve source incoming order, repeated predecessor blocks, and
  accepted parallel successor occurrences
- validate result and incoming values, types, ownership, predecessor presence
  and ownership, exact predecessor-edge coherence, and complete/unique incoming
  occurrence coverage before publication
- prove selected ternary, logical, AArch64-vaarg, and AMD64-vaarg positive
  receipts; prove missing/unknown/cross-function value, missing/foreign
  predecessor, predecessor-edge mismatch, duplicate/ambiguous occurrence,
  parallel-edge preservation, misleading-display, and rollback failures
- retain every later family as unsupported fail-closed; return source completion
  state to plan-owner after this bounded receiver rather than inferring closure

Completion check:

- a fresh build and focused positive/negative proof establish one typed,
  transactional PHI receiver with exact ordinary and parallel edge-occurrence
  semantics and no presentation recovery.

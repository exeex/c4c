# LIR-To-New-BIR Container And Import Completeness Runbook

Status: Active
Source Idea: ideas/open/734_lir_to_new_bir_container_completeness.md
Resumed from: accepted Steps 1 through 7.24, including the typed computed-goto
receiver, and closed idea 751's typed PHI incoming authority handoff
(`6ece9fe8f`).

## Purpose

Resume the Raw-BIR receiver route at the first newly authorized PHI row without
resetting previously accepted receiver work.

## Goal

Receive one structured `LirPhiOp` result and its ordered typed incoming
value/predecessor pairs into verified, target-independent Raw BIR, preserving
exact CFG-edge occurrence identity without presentation recovery or partial
publication.

## Core Rule

Use `LirPhiOp`'s typed result/type and the closed-751 `LirPhiIncoming`
`LirOperand` value plus current-function `LirBlockId` predecessor authority.
Value spellings, labels, printer output, LLVM text, and instruction-order
guesses are never semantic inputs.

## Read First

- `ideas/open/734_lir_to_new_bir_container_completeness.md`
- `ideas/closed/751_lir_phi_incoming_value_and_predecessor_identity.md`
- existing Raw-BIR value, block, instruction, builder/view, and verifier seams
- `src/backend/bir/lir_to_bir.cpp` and its nearby LIR-to-Raw-BIR dispatch/tests
- `src/backend/bir/lir_to_bir/README.md` PHI and values-def-use matrix rows

## Landed Progress

- Steps 1 through 7.24 are accepted historical work and must not be repeated.
- Step 7.24 accepted the typed `LirIndirectBrOp` receiver; closed 751 now
  publishes verified incoming value and predecessor authority for ternary,
  logical, AArch64-vaarg, and AMD64-vaarg PHI producers.

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
3. Reject absent, invalid, foreign, duplicate, ambiguous, type-incoherent, or
   predecessor/edge-mismatched authority before Raw-BIR publication.
4. Add reachable nearby positive/negative transactional coverage covering the
   closed-751 ternary, logical, and vaarg producer families. Do not use test
   names or rendered text as matching inputs.
5. Run a fresh build and narrow proof. The supervisor owns regression logs and
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
  value/predecessor fields; resolve their IDs through the current-function
  maps and exact already-received terminator successor occurrences
- represent one typed PHI result and one typed incoming per exact edge
  occurrence; preserve source incoming order and do not collapse repeated
  predecessor blocks or parallel edges
- validate result and incoming values, types, ownership, predecessor presence
  and ownership, exact predecessor-edge coherence, and complete/unique incoming
  occurrence coverage before publication
- prove selected ternary, logical, AArch64-vaarg, and AMD64-vaarg positive
  receipts; prove missing/unknown/cross-function value, missing/foreign
  predecessor, predecessor-edge mismatch, duplicate/ambiguous occurrence,
  misleading-display, and rollback failures
- retain every later family as unsupported fail-closed; return source completion
  state to plan-owner after this bounded receiver rather than inferring closure

Completion check:

- a fresh build and focused positive/negative proof establish one typed,
  transactional PHI receiver with exact edge occurrence semantics and no
  presentation recovery.

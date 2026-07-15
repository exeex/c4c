# LIR PHI Incoming Authority Runbook

Status: Active
Source Idea: ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md
Activated from: closed `ideas/closed/775_lir_phi_producer_helper_result_identity.md`.

## Purpose

Make `LirPhiOp` incoming entries retain structured value and predecessor-block
authority now that the bounded ternary, logical-RHS, and vaarg producer facts
are available.

## Goal

Replace or augment raw PHI incoming `(value, label)` compatibility pairs with
native current-function `LirOperand` value authority and `LirBlockId`
predecessor authority, without recovering either from display text.

## Core Rule

PHI incoming values and predecessors are semantic authority. `%t` names,
labels, rendered LLVM, instruction order, and testcase names are never
recovery inputs.

## Read First

- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `ideas/closed/775_lir_phi_producer_helper_result_identity.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- active `LirPhiOp` producers, verifier, and focused PHI coverage

## Non-Goals

- no helper-producer, generic expression API, Raw-BIR/importer, backend,
  target-lowering, MIR, or emission work
- no terminator successor publication; closed 750 owns that completed contract
- no parsing of value names, labels, printer text, rendered LLVM, or test text

## Execution Rules

1. Start from the accepted 775 handoff: `LirVaArgOp.result`, the selected
   logical RHS `LirCastOp.result`, and both selected ternary-arm
   `LirCastOp.result` fields are authoritative producer facts, not text.
2. Keep the repair in the `LirPhiOp` carrier and verification seam while
   retaining rendering compatibility.
3. Fail closed for missing, unknown, cross-function, and predecessor/edge
   incoherent authority.
4. Prove a fresh build and focused positive/malformed PHI coverage before the
   supervisor selects broader or full baseline acceptance.

## Ordered Steps

### Step 1 - Publish and verify typed PHI incoming authority

Goal: replace or augment raw PHI incoming `(value, label)` facts with checked
structured current-function value and predecessor-block authority.

Primary targets:

- `LirPhiOp` representation and verifier
- active ternary, logical short-circuit, and vaarg PHI producers
- nearby focused positive and malformed-authority coverage

Actions:

- trace the accepted producer IDs to each raw PHI creation site and retain
  them structurally with predecessor block IDs;
- verify ownership, predecessor existence, and predecessor/edge coherence
  before printing or downstream use;
- retain the current rendered form as compatibility only; and
- add nearby positive and malformed cases for all three producer families.

Completion check:

- a fresh build and focused PHI production/malformed proof pass with unknown,
  cross-function, missing, and edge-mismatched authority rejected; publish the
  bounded typed PHI contract to 734 without claiming Raw-BIR receiver work.

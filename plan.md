# LIR PHI Incoming Authority Runbook

Status: Active
Source Idea: ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md
Activated from: the post-772 repair of 734's completed Step 7.24 disposition.

## Purpose

Supply the first remaining structured LIR authority required before 734 can
resume its next bounded Raw-BIR receiver: PHI incoming values and predecessor
blocks.

## Goal

Make active `LirPhiOp` incoming entries retain typed current-function value and
block authority without recovering either fact from presentation text.

## Core Rule

PHI incoming values and predecessors are semantic authority. `%t` names,
labels, printed LLVM, instruction order, and testcase names are display or
incidental data, never recovery inputs.

## Read First

- `ideas/open/751_lir_phi_incoming_value_and_predecessor_identity.md`
- `ideas/closed/750_lir_cfg_terminator_block_identity_completion.md`
- `ideas/open/734_lir_to_new_bir_container_completeness.md` resumption update
- active `LirPhiOp` producers, verifier, and focused PHI coverage

## Non-Goals

- no Raw-BIR/importer receiver work; 734 owns its later bounded receiver
- no terminator successor publication; closed 750 owns that completed contract
- no local/object, memory/va, aggregate/vector, target-lowering, MIR, or
  emission expansion
- no parsing of value names, labels, printer text, or rendered LLVM

## Execution Rules

1. Start from the existing ternary, logical short-circuit, and vaarg PHI
   producers; keep the repair within the LIR PHI carrier and verification seam.
2. Publish only valid current-function `LirOperand` value authority and
   `LirBlockId` predecessor authority, retaining rendering compatibility.
3. Preserve fail-closed handling for missing, unknown, foreign, and
   predecessor/edge-incoherent authority.
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

- trace each active producer to the first point it drops value or predecessor
  identity, then retain the typed facts structurally
- verify ownership, predecessor existence, and predecessor/edge coherence
  before printing or downstream use
- keep the current rendered form compatible without making it authoritative
- add nearby positive and malformed cases covering all named producer families

Completion check:

- fresh build and focused PHI production/malformed proof pass with unknown,
  cross-function, missing, and edge-mismatched authority still rejected;
  hand the bounded typed PHI contract back to 734 for its next receiver-plan
  repair.

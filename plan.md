# LIR CFG Terminator Block Identity Completion Runbook

Status: Active
Source Idea: ideas/open/750_lir_cfg_terminator_block_identity_completion.md
Activated from: paused idea 734 after accepted Step 7.20 selected memcpy
Raw-BIR receipt (`1a3adbc58`)

## Purpose

Establish structured current-function block identity for every active LIR
terminator successor before any further Raw-BIR CFG receiver work resumes.

## Goal

Publish and verify `LirBlockId` authority for conditional, switch, and
computed-goto successors without recovering semantics from labels.

## Core Rule

Successor labels remain display-only. Every accepted control-flow edge must
carry a verifier-checked current-function block identity before printing or
downstream consumption.

## Read First

- `ideas/open/750_lir_cfg_terminator_block_identity_completion.md`
- the accepted direct-branch structured-successor route and its verifier
- active `LirCondBr`, `LirSwitch`, and `LirIndirectBrOp` producers
- the focused CFG tests and current LIR verifier ownership checks

## Non-Goals

- no Raw-BIR import, canonical BIR, target lowering, MIR, or emission work
- no PHI incoming authority (idea 751), local/object pointers, memory/va,
  aggregate/vector identity, or label-text recovery

## Execution Rules

1. Keep `LirBlockId` authoritative and retain strings only as mirrors.
2. Reject absent, duplicate, ambiguous, invalid, or cross-function successors
   before any downstream consumer can use them.
3. Keep each terminator family bounded and prove malformed authority has no
   rendering-based fallback.
4. Run a fresh build and focused positive/negative proof for each packet;
   require the supervisor's broader/full acceptance checkpoint before closure.

## Ordered Steps

### Step 1 - Define and verify conditional/switch successor authority

Goal: give active `LirCondBr` and `LirSwitch` default/case edges structured
current-function `LirBlockId` authority.

Actions:

- trace existing producers and introduce the smallest typed successor carrier
  consistent with the direct-branch route
- populate conditional true/false and switch default/case targets without
  deriving IDs from labels
- extend verification for missing, duplicate/ambiguous, invalid, and
  cross-function target authority
- add focused forward-label, conditional, and switch positive/negative tests

Completion check:

- fresh build and focused tests establish that misleading label text cannot
  select or repair a conditional or switch successor.

### Step 2 - Publish computed-goto target-list authority

Goal: give active `LirIndirectBrOp.targets` an equivalent structured ordered
current-function block-target representation.

Actions:

- preserve target-list order while publishing typed target identities
- reject missing, invalid, duplicate/ambiguous, or foreign targets
- add focused computed-goto positive/negative coverage without parsing labels

Completion check:

- fresh build and focused tests prove target-list identity and verifier
rejection independently of rendered labels.

### Step 3 - Consolidate verifier coverage and hand off

Goal: demonstrate complete active terminator-successor coverage and prepare an
exact receiver handoff for the paused Raw-BIR source.

Actions:

- verify direct, conditional, switch, and indirect successor ownership using
  one coherent current-function contract
- run the supervisor-selected broader/full checkpoint
- record the exact typed fields, selected receiver-ready row, proof, and
  fail-closed boundaries for resuming idea 734

Completion check:

- all acceptance criteria for idea 750 are met and its handoff lets
  plan-owner reactivate 734 for one bounded Raw-BIR CFG receiver packet.

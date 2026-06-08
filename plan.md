# AArch64 ALU Post-Contract Boundary Audit Runbook

Status: Active
Source Idea: ideas/open/127_aarch64_alu_post_contract_boundary_audit.md

## Purpose

Audit `src/backend/mir/aarch64/codegen/alu.cpp` after recent prepared,
publication, dispatch, comparison, calls, and memory contract work. Classify
whether remaining ALU-side responsibilities are shared compiler authority,
AArch64-local emission policy, review visibility gaps, or local clarity work.

## Goal

Produce a focused audit result with concrete follow-up ideas only for real
remaining boundary gaps, or explicitly record `no new idea` where current
evidence shows the boundary is already clean.

## Core Rule

This plan is analysis-only. Do not edit implementation files, tests, build
metadata, or closed idea archives while executing it.

## Read First

- `ideas/open/127_aarch64_alu_post_contract_boundary_audit.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- Closed idea context when needed for evidence:
  - idea 51: ALU prepared authority repair
  - idea 55: scalar cast/ALU publication prepared authority repair
  - idea 71: scalar/control-flow prepared authority cleanup
  - idea 74: local scalar register helper fold-back
  - idea 116: dispatch prepared producer contract surface
  - idea 117: comparison fused-compare/current-block publication contract
  - idea 122: call argument producer materializability contract
  - idea 123: call result late-publication contract

## Current Scope

- Prepared scalar operand and result homes.
- Scalar producer and publication consumption.
- Immediate and constant materialization policy.
- Control-source and fallback operand materialization.
- Physical split readiness for local AArch64 ALU clarity.

## Non-Goals

- Do not shrink `alu.cpp` as its own acceptance criterion.
- Do not reopen closed scalar prepared-authority, publication, dispatch,
  comparison, call, or local register-view routes without new evidence.
- Do not move AArch64 opcode choice, immediate encodability, scratch register
  policy, extension/mul/div/rem spelling, or machine-record emission into
  shared code.
- Do not start x86 or RISC-V codegen work.
- Do not create vague follow-up ideas without files, owner boundary, proof
  route, and testcase-overfit reject signals.

## Working Model

- Shared authority belongs in prepared/BIR/prealloc contract surfaces.
- AArch64 register spelling, scratch selection, immediate encodability, opcode
  choice, and assembler/machine-record emission belong in AArch64 codegen.
- Large helper clusters are acceptable if their responsibility is target-local
  and visible; they need follow-up only when they hide shared authority,
  duplicate closed-route decisions, or block review evidence.

## Execution Rules

- Prefer evidence from current code and closed ideas over inference.
- For every audit standard, classify the result as one of:
  - `shared-boundary-gap`
  - `contract-visibility-gap`
  - `aarch64-local-clarity`
  - `no-new-idea`
- Any follow-up idea under `ideas/open/` must name owner boundary, likely
  files, proof route, and testcase-overfit reject signals.
- Record routine audit notes and packet progress in `todo.md`.
- Preserve source idea stability unless closure or new durable intent requires
  an edit.

## Steps

### Step 1: Map Current ALU Helper Clusters

Goal: Build a helper/cluster map for current large `alu.cpp` regions before
judging ownership boundaries.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect top-level helpers, local helper clusters, and public entry points.
- Group helpers by responsibility: operand preparation, producer/publication,
  immediate/materialization, control/fallback, arithmetic emission, and
  machine-record emission.
- Note where each cluster consumes prepared facts versus computing authority.
- Avoid implementation edits.

Completion check:

- `todo.md` records the helper/cluster map and any uncertain areas that need
  closed-idea context.

### Step 2: Audit Prepared Operand And Result Home Boundaries

Goal: Decide whether ALU consumes prepared value-home, storage, and regalloc
facts for scalar operands and results.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- relevant prepared/prealloc helper surfaces referenced by ALU

Actions:

- Trace scalar operand and result-home paths.
- Identify any value-name or raw storage recovery inside ALU.
- Compare uncertain paths against closed ideas 51, 55, 71, and 74.
- Classify the standard and record `no new idea` when closed work already
  covers it.

Completion check:

- The audit notes distinguish prepared-fact consumption from any remaining
  ALU-local authority rebuilds for operand/result homes.

### Step 3: Audit Scalar Producer And Publication Consumption

Goal: Decide whether ALU still owns target-neutral producer,
materializability, current-block publication, stack publication, or
select-chain dependency decisions.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- shared producer/publication contract surfaces used by dispatch,
  comparison, calls, and memory routes

Actions:

- Trace same-block scalar producer and load-local producer decisions.
- Trace current-block entry publication, stack publication, and select-chain
  dependency decisions.
- Compare with closed ideas 116, 117, 122, and 123.
- Separate target-neutral decisions from AArch64 operand spelling.

Completion check:

- The audit records either concrete shared-boundary follow-up scope or explicit
  `no new idea` evidence for scalar producer/publication consumption.

### Step 4: Audit Immediate And Constant Materialization Policy

Goal: Separate target-neutral immediate semantics from AArch64 immediate
encodability, reduction, scratch materialization, and assembler spelling.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect immediate reduction and constant materialization helpers.
- Identify whether helpers decide semantic materializability or only AArch64
  encodability and scratch mechanics.
- Keep AArch64-specific opcode, immediate, scratch, extension, and
  machine-record policy local.

Completion check:

- The audit classifies immediate/materialization helpers as shared-boundary
  gaps, contract-visibility gaps, AArch64-local clarity work, or
  `no-new-idea`.

### Step 5: Audit Control-Source And Fallback Operand Materialization

Goal: Decide whether fallback operand selection, control publication operands,
return ABI retargeting, and materialized control-source helpers are pure
prepared-fact consumers.

Primary targets:

- `src/backend/mir/aarch64/codegen/alu.cpp`
- related scalar/control-flow prepared contract surfaces

Actions:

- Trace fallback operand and control publication operand selection.
- Check return ABI retargeting and materialized control-source helpers.
- Identify any producer/control-flow authority rebuilt inside ALU.
- Compare with closed scalar/control-flow prepared-authority evidence.

Completion check:

- The audit records the boundary classification and any concrete follow-up
  idea scope for control-source or fallback materialization gaps.

### Step 6: Decide Physical Split Readiness And Draft Outputs

Goal: Convert the audit evidence into closure-ready output and follow-up ideas
only where warranted.

Primary targets:

- `todo.md` for current audit findings
- `ideas/open/` only for concrete follow-up ideas

Actions:

- Build the final classification table for the five audit standards.
- Preserve the helper/cluster map from Step 1.
- Write explicit `no new idea` entries for standards covered by closed work.
- Draft follow-up `ideas/open/` files only for unresolved concrete gaps.
- If boundaries are clean but local clarity would help, make any extraction
  idea explicitly mechanical and AArch64-local.

Completion check:

- The active lifecycle state has enough evidence for plan-owner closure review:
  classification table, helper/cluster map, `no new idea` entries, and any
  concrete follow-up idea files.

# Shared MIR Same-Block Producer And Select-Chain Queries Runbook

Status: Active
Source Idea: ideas/open/shared-mir-same-block-producer-select-queries.md

## Purpose

Move same-block producer discovery and select-chain dependency analysis out of
AArch64 codegen into a shared MIR query surface that AArch64, x86, and RISC-V
lowering can reuse before final target instruction selection.

## Goal

Expose target-neutral same-block producer and select-chain query records while
preserving the current AArch64 machine instruction output.

## Core Rule

Move generic query ownership only. Keep AArch64 instruction spelling, ABI
policy, target condition mapping, and final machine construction in the AArch64
layer.

## Read First

- `ideas/open/shared-mir-same-block-producer-select-queries.md`
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- AArch64 users of producer/select-chain facts in call arguments, branch
  fusion, value publication, and store handling.

## Current Targets

- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp`
- AArch64 call, branch-fusion, publication, and store consumers that currently
  depend on local producer/select-chain helpers.
- A new or existing shared MIR-owned helper location for target-neutral query
  records.
- One x86 or RISC-V compile fixture, adapter, or consumer-facing unit test that
  proves the query shape is not AArch64-only.

## Non-Goals

- Do not move publication planning, store-source planning, or call-boundary
  ordering.
- Do not encode AArch64 register names, condition mnemonics, ADRP/ADD forms,
  memory operand spelling, or final machine construction in the shared API.
- Do not weaken AArch64 tests or mark supported behavior unsupported.
- Do not perform helper-only renames that leave AArch64 codegen as the real
  owner of the generic query responsibility.

## Working Model

Separate target-neutral local def-use discovery from target-specific emission:

- shared MIR owns same-block producer lookup and select-chain dependency
  traversal records;
- AArch64 users ask the shared query for facts and keep their target policy at
  the call site;
- x86 or RISC-V proof must compile or exercise the same query surface without
  depending on AArch64 namespaces.

## Execution Rules

- Start by mapping current helper call sites before extracting code.
- Preserve behavior first; do not combine this migration with publication,
  store-source, or call-ordering changes.
- Keep the shared API small enough for current users and one cross-target proof.
- Prefer semantic query records over testcase-shaped matching.
- For code-changing steps, run build proof plus focused AArch64 parity tests and
  the x86/RISC-V proof named by the step.
- Escalate to a backend subset when shared MIR headers or common lowering paths
  change.

## Ordered Steps

### Step 1: Map Current Producer And Select-Chain Users

Goal: Identify the exact AArch64 helper logic and consumer call sites that
should move to or consume the shared query surface.

Concrete actions:

- Inspect `dispatch_producers.cpp` and related declarations.
- List same-block producer lookup, constant discovery, select-chain dependency,
  and support-instruction filtering responsibilities.
- List each AArch64 consumer in call arguments, branch fusion, value
  publication, and store handling.
- Mark which decisions are target-neutral facts and which remain AArch64
  target policy.

Completion check:

- `todo.md` records the helper-to-consumer map and the boundary between shared
  facts and retained AArch64 policy.

### Step 2: Introduce Shared MIR Query Records

Goal: Add the smallest shared MIR-owned API needed for same-block producer and
select-chain queries.

Concrete actions:

- Choose a shared MIR-owned or target-neutral helper location consistent with
  nearby code.
- Move generic same-block producer lookup and select-chain dependency traversal
  into that location.
- Define target-neutral result records that do not mention AArch64 instruction
  spelling or register names.
- Keep target-specific legality, mnemonic choice, and emission decisions out of
  the shared API.

Completion check:

- The project builds through the affected C++ targets, and the shared API can
  be included without AArch64 codegen ownership.

### Step 3: Rewire AArch64 Users To The Shared Query

Goal: Make current AArch64 consumers use the shared query while preserving
their emitted behavior.

Concrete actions:

- Replace local AArch64 helper ownership with calls to the shared query.
- Keep call, branch-fusion, publication, and store target policy in AArch64
  codegen.
- Remove only declarations whose underlying ownership actually moved.
- Avoid changing expected machine instruction sequences except where an
  existing test already permits equivalent output.

Completion check:

- Focused AArch64 parity tests or fixtures for the affected producer and
  select-chain cases still pass.

### Step 4: Add Cross-Target Reuse Proof

Goal: Prove the new query is usable outside AArch64 codegen.

Concrete actions:

- Add one x86 or RISC-V compile fixture, adapter, or consumer-facing unit test
  that includes or exercises the shared query surface.
- Keep the proof small and targeted to the query shape.
- Do not fake reuse with a comment-only proof or unused include.

Completion check:

- The x86 or RISC-V proof compiles or runs and demonstrates the query surface is
  not AArch64-only.

### Step 5: Validation And Lifecycle Handoff

Goal: Prove the migration and leave lifecycle state ready for close or the next
focused follow-up.

Concrete actions:

- Run build proof for affected C++ targets.
- Run focused AArch64 parity tests for same-block producer and select-chain
  behavior.
- Run the x86/RISC-V proof from Step 4.
- If shared MIR or common backend headers changed, run a backend subset
  regression guard before closure.

Completion check:

- Validation is recorded in `todo.md`, and the supervisor can either close this
  idea or route the next generated follow-up without re-reading the route.

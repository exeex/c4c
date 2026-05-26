# RISC-V Stack-Destination Policy Broadening Plan

Status: Active
Source Idea: ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md

## Purpose

Broaden RISC-V prepared edge-publication consumption for moves into
`StackSlot` destinations beyond the focused `Register -> StackSlot` form
closed by idea 27.

## Goal

Implement at least one additional source-to-`StackSlot` destination form
through shared `edge_publications` authority, or record the concrete RISC-V
policy reason the remaining candidate forms must stay fail-closed for now.

## Core Rule

Do not rediscover edge-copy facts from RISC-V predecessor or successor block
shape. Every supported stack-destination move must be driven by shared
prepared `edge_publications` lookup authority.

## Read First

- `ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md`
- `ideas/closed/27_riscv_prepared_edge_publication_stack_destination_support.md`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Current Scope

- Target destination home family: `StackSlot` destinations for prepared edge
  publications.
- Existing baseline to preserve: `Register -> StackSlot` stores into concrete
  4-byte stack-slot destinations through shared `edge_publications`.
- Candidate broadened source homes include
  `RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
  `PointerBasePlusOffset -> StackSlot`.
- Keep existing register-destination consumers supported, including
  `Register -> Register`, `RematerializableImmediate -> Register`,
  `StackSlot -> Register`, and `PointerBasePlusOffset -> Register` forms
  already accepted by earlier ideas.

## Non-Goals

- Do not broaden stack-source register-destination policy in this plan.
- Do not broaden pointer-base register-destination policy in this plan.
- Do not move RISC-V store, load, address materialization, or scratch policy
  into shared prepare, BIR, or target-neutral helpers.
- Do not add RISC-V-local edge-publication discovery.
- Do not downgrade existing `Register -> StackSlot` behavior.
- Do not claim broad source-to-stack support from helper renames or expectation
  rewrites alone.

## Working Model

The RISC-V backend already consumes shared prepared edge publications for
`Register -> StackSlot` when the destination is a concrete 4-byte stack slot.
This plan keeps shared lookup fixed and asks the RISC-V backend to choose the
next target-local store or materialization policy. Immediate materialization,
stack-to-stack copies, pointer-base address values, scratch-register needs,
destination size, and stack-offset range must be explicit before any new form
is accepted.

## Execution Rules

- Record routine packet progress and evidence in `todo.md`.
- Keep code changes scoped to the RISC-V prepared edge-publication consumer,
  target-local materialization or store helpers, and focused tests.
- Add positive coverage for any newly supported source-to-stack form.
- Add negative coverage proving missing shared publication authority,
  unsupported source homes, malformed stack destinations, non-move
  publications, and missing lookup facts fail closed.
- Use a focused RISC-V prepared edge-publication validation bucket, then
  request broader backend validation before closure.

## Steps

### Step 1: Inventory RISC-V Stack-Destination Source Options

Goal: Identify the next source-to-stack form that can be supported without
guessing materialization, store width, address, or scratch-register policy.

Actions:

- Inspect the current RISC-V stack-destination rendering helper and focused
  prepared edge-publication tests.
- List the source and destination facts available to codegen for source home
  kind, destination stack slot id, concrete offset, size, and immediate range.
- Choose the first broadened source-to-stack form to attempt, or record the
  concrete policy blocker that keeps all remaining forms fail-closed.
- Record the focused proof command and unsupported forms in `todo.md`.

Completion check:

- `todo.md` names the target helper files/functions, selected broadened
  source-to-stack form or policy blocker, focused proof command, and
  fail-closed forms.

### Step 2: Implement or Preserve Fail-Closed Source-to-Stack Policy

Goal: Add the selected stack-destination materialization policy, or make the
fail-closed reason explicit if no additional form is supportable yet.

Actions:

- Extend the RISC-V prepared edge-publication consumer only for the selected
  source-to-stack form from Step 1.
- Keep shared `edge_publications` and prepared value-home lookup as the only
  semantic authorities.
- Keep target-local store, address materialization, instruction selection, and
  scratch policy inside the RISC-V backend.
- Preserve existing `Register -> StackSlot` behavior and all supported
  register-destination consumers.
- Fail closed for malformed destinations, unsupported source homes, non-move
  publications, missing shared authority, missing lookup facts, unsupported
  sizes, and out-of-range stack offsets unless Step 1 explicitly selected one
  of those forms with a concrete policy.

Completion check:

- Focused positive and negative tests cover the selected behavior or the
  explicit fail-closed policy.
- The delegated proof command passes and writes `test_after.log`.

### Step 3: Review Route Quality

Goal: Verify the stack-destination route matches the source idea and avoids
testcase-shaped broadening.

Actions:

- Request reviewer scrutiny after the implementation or explicit fail-closed
  policy packet.
- Treat fixture-shaped matching, local edge rediscovery, downgraded existing
  support, register-destination scope creep, target-neutral store-policy moves,
  or helper-only renames as blocking failures.

Completion check:

- A review artifact under `review/` reports no blocking findings, or the route
  is corrected before continuing.

### Step 4: Validate the Stack-Destination Broadening Slice

Goal: Prove the accepted fail-closed stack-destination policy does not regress
the relevant backend surface.

Actions:

- Run or refresh the focused RISC-V prepared edge-publication proof selected in
  Step 1, making sure root `test_after.log` exists and matches `todo.md`.
- Run matching regression guard when before/after logs have the same scope.
- Run an appropriate backend bucket before closure.
- Preserve canonical root logs as `test_before.log` and `test_after.log` only.
- Do not introduce scratch-register policy in this validation step; that work
  is follow-up scope.

Completion check:

- `todo.md` records focused proof, matching guard status when applicable,
  broader backend evidence, and any accepted baseline evidence.

### Step 5: Handoff or Close

Goal: Close idea 30 as a validated fail-closed policy slice, or hand off if
validation exposes a blocker.

Actions:

- Record that the only supported source-to-stack form remains
  `Register -> StackSlot`.
- Preserve the explicit scratch-register blocker for
  `RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
  `PointerBasePlusOffset -> StackSlot`.
- Point future scratch-register work at
  `ideas/open/32_riscv_prepared_edge_publication_stack_destination_scratch_policy.md`.
- Do not close if validation cannot refresh the missing canonical proof
  evidence noted by the route review.

Completion check:

- The source idea can be closed with concrete fail-closed policy and durable
  follow-up scope, or the active route is left blocked with the exact
  validation failure.

# RISC-V Stack Source Policy Broadening Plan

Status: Active
Source Idea: ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md

## Purpose

Broaden RISC-V prepared edge-publication consumption for stack-slot sources
beyond the focused 4-byte concrete-offset `StackSlot -> Register` path closed
by idea 25.

## Goal

Either implement one additional stack-source form through shared
`edge_publications` authority, or record the concrete RISC-V policy reason the
remaining stack-source forms must stay fail-closed for now.

## Core Rule

Do not rediscover edge-copy facts from RISC-V predecessor or successor block
shape. Every supported stack-source load must be driven by shared prepared
`edge_publications`.

## Read First

- `ideas/open/28_riscv_prepared_edge_publication_stack_source_policy_broadening.md`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Current Scope

- Target source home family: stack-slot sources feeding register destinations.
- Existing baseline to preserve: concrete-offset 4-byte `StackSlot -> Register`
  emitted as `lw <dst>, <offset>(sp)`.
- Candidate broadened surfaces include non-4-byte concrete stack loads,
  stack-source forms without concrete offsets, dynamic stack addressing, and
  large-offset materialization.
- Keep existing `Register -> Register`,
  `RematerializableImmediate -> Register`,
  `PointerBasePlusOffset -> Register`, and `Register -> StackSlot` consumers
  supported.

## Non-Goals

- Do not broaden pointer-base source policy in this plan.
- Do not broaden source-to-`StackSlot` destination policy in this plan.
- Do not move RISC-V stack-source load, extension, scratch, or address policy
  into shared prepare, BIR, or target-neutral helpers.
- Do not downgrade existing register, immediate, pointer-base, stack-source,
  or stack-destination edge-publication tests.
- Do not add RISC-V-local edge-publication discovery.

## Working Model

The RISC-V backend already consumes shared prepared edge publications for
register destinations and can load focused 4-byte concrete stack sources with
`lw`. This plan keeps the shared lookup path fixed and asks the RISC-V backend
to choose a target-local policy for the next stack-source form. Width,
extension, offset, scratch, and address materialization decisions must be
explicit before any new form is accepted.

## Execution Rules

- Record routine packet progress and evidence in `todo.md`.
- Keep code changes scoped to the RISC-V prepared edge-publication consumer and
  focused tests.
- Add positive coverage for any newly supported stack-source form.
- Add negative coverage proving missing shared publication authority,
  malformed stack-source facts, unsupported stack-source forms, and non-move
  publications fail closed.
- Use the focused RISC-V prepared edge-publication validation bucket, then
  request broader backend validation before closure.

## Steps

### Step 1: Inventory RISC-V Stack Source Broadening Options

Goal: Identify the next stack-source form that can be supported without
guessing target-local load, extension, scratch, or address policy.

Actions:

- Inspect the current RISC-V stack-source rendering helper and tests.
- List the stack-source facts available to codegen for offset, size, and any
  width or extension-relevant metadata.
- Choose the first broadened stack-source form to attempt, or record the
  concrete policy blocker that keeps all remaining forms fail-closed.
- Record the focused proof command and unsupported forms in `todo.md`.

Completion check:

- `todo.md` names the target helper files/functions, selected broadened
  stack-source form or policy blocker, focused proof command, and fail-closed
  forms.

### Step 2: Implement or Preserve Fail-Closed Stack-Source Policy

Goal: Add the selected stack-source load policy, or make the fail-closed reason
explicit if no additional form is supportable yet.

Actions:

- Extend the RISC-V prepared edge-publication consumer only for the selected
  stack-source form from Step 1.
- Keep shared `edge_publications` lookup as the only semantic authority.
- Keep target-local load, extension, scratch, and address materialization
  inside the RISC-V backend.
- Preserve existing 4-byte concrete-offset stack-source behavior and all other
  supported edge-publication consumers.
- Fail closed for unsupported widths, missing offsets, dynamic stack addressing,
  large offsets, malformed facts, and non-move publications unless Step 1
  explicitly selected one of those forms with a concrete policy.

Completion check:

- Focused positive and negative tests cover the selected behavior or the
  explicit fail-closed policy.
- The delegated proof command passes and writes `test_after.log`.

### Step 3: Review Route Quality

Goal: Verify the stack-source route matches the source idea and avoids
testcase-shaped broadening.

Actions:

- Request reviewer scrutiny after the implementation or explicit fail-closed
  policy packet.
- Treat fixture-shaped matching, local edge rediscovery, downgraded existing
  support, target-neutral load-policy movement, or helper-only renames as
  blocking failures.

Completion check:

- A review artifact under `review/` reports no blocking findings, or the route
  is corrected before continuing.

### Step 4: Validate the Stack-Source Broadening Slice

Goal: Prove the selected stack-source policy does not regress the relevant
backend surface.

Actions:

- Run the focused RISC-V prepared edge-publication proof selected in Step 1.
- Run matching regression guard when before/after logs have the same scope.
- Run an appropriate backend bucket before closure.
- Preserve canonical root logs as `test_before.log` and `test_after.log` only.

Completion check:

- `todo.md` records focused proof, matching guard status when applicable,
  broader backend evidence, and any accepted baseline evidence.

### Step 5: Handoff or Close

Goal: Decide whether idea 28 is complete or whether additional stack-source
policy should move to separate durable follow-up ideas.

Actions:

- Record exactly which stack-source forms are now supported.
- Preserve explicit caveats for unsupported stack-source forms.
- If remaining work is adjacent but outside the completed slice, create or
  recommend separate durable follow-up ideas instead of expanding this plan.

Completion check:

- The source idea can be closed with concrete support or concrete fail-closed
  policy, or the active route is intentionally rewritten/deactivated with
  durable follow-up scope.

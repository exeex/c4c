# RISC-V Pointer-Base Policy Broadening Plan

Status: Active
Source Idea: ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md

## Purpose

Broaden RISC-V prepared edge-publication consumption for
`PointerBasePlusOffset -> Register` beyond the focused register-base,
signed-12-bit `addi` policy closed by idea 26.

## Goal

Either implement one additional pointer-base materialization policy through
shared `edge_publications` authority, or record the concrete RISC-V policy
reason the remaining pointer-base forms must stay fail-closed for now.

## Core Rule

Do not rediscover edge-copy facts from RISC-V predecessor or successor block
shape. Every supported pointer-base move must be driven by shared prepared
`edge_publications` and prepared value-home lookup authority.

## Read First

- `ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md`
- `src/backend/mir/riscv/codegen/emit.hpp`
- `src/backend/mir/riscv/codegen/emit.cpp`
- `tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`

## Current Scope

- Target source home family: `PointerBasePlusOffset` feeding register
  destinations.
- Existing baseline to preserve: pointer base resolved through prepared
  value-home lookup to a register home, with a present delta that fits signed
  12-bit RISC-V `addi`.
- Candidate broadened surfaces include pointer bases whose prepared home is
  not a register and deltas that do not fit the signed 12-bit `addi`
  immediate.
- Keep existing `Register -> Register`,
  `RematerializableImmediate -> Register`, `StackSlot -> Register`, and
  `Register -> StackSlot` consumers supported.

## Non-Goals

- Do not broaden source-to-`StackSlot` destination policy in this plan.
- Do not broaden stack-source register-destination policy in this plan.
- Do not treat missing base names, unresolved base names, or absent deltas as
  valid moves without a separate source-model decision.
- Do not move RISC-V address materialization, instruction selection, or
  scratch policy into shared prepare, BIR, or target-neutral helpers.
- Do not add RISC-V-local edge-publication discovery.
- Do not downgrade existing register-base signed-12-bit pointer-base behavior.

## Working Model

The RISC-V backend already consumes shared prepared edge publications for
`PointerBasePlusOffset -> Register` when the base name resolves through the
prepared value-home table to a register and the delta fits `addi`. This plan
keeps shared lookup fixed and asks the RISC-V backend to choose the next
target-local materialization policy. Large deltas, scratch usage, and
non-register base homes must be made explicit before any new form is accepted.

## Execution Rules

- Record routine packet progress and evidence in `todo.md`.
- Keep code changes scoped to the RISC-V prepared edge-publication consumer,
  target-local materialization helpers, and focused tests.
- Add positive coverage for any newly supported pointer-base form.
- Add negative coverage proving missing shared publication authority,
  unresolved bases, absent deltas, unsupported base homes, and non-move
  publications fail closed.
- Use the focused RISC-V prepared edge-publication validation bucket, then
  request broader backend validation before closure.

## Steps

### Step 1: Inventory RISC-V Pointer-Base Policy Options

Goal: Identify the next pointer-base form that can be supported without
guessing base materialization, large-delta, or scratch-register policy.

Actions:

- Inspect the current RISC-V pointer-base rendering helper and focused tests.
- List the source facts available to codegen for base name resolution, base
  home kind, delta presence, and delta range.
- Choose the first broadened pointer-base form to attempt, or record the
  concrete policy blocker that keeps all remaining forms fail-closed.
- Record the focused proof command and unsupported forms in `todo.md`.

Completion check:

- `todo.md` names the target helper files/functions, selected broadened
  pointer-base form or policy blocker, focused proof command, and fail-closed
  forms.

### Step 2: Implement or Preserve Fail-Closed Pointer-Base Policy

Goal: Add the selected pointer-base materialization policy, or make the
fail-closed reason explicit if no additional form is supportable yet.

Actions:

- Extend the RISC-V prepared edge-publication consumer only for the selected
  pointer-base form from Step 1.
- Keep shared `edge_publications` and prepared value-home lookup as the only
  semantic authorities.
- Keep target-local address materialization, instruction selection, and scratch
  policy inside the RISC-V backend.
- Preserve existing register-base signed-12-bit `addi` behavior and all other
  supported edge-publication consumers.
- Fail closed for missing base names, unresolved base names, absent deltas,
  unsupported base homes, and out-of-range deltas unless Step 1 explicitly
  selected one of those forms with a concrete policy.

Completion check:

- Focused positive and negative tests cover the selected behavior or the
  explicit fail-closed policy.
- The delegated proof command passes and writes `test_after.log`.

### Step 3: Review Route Quality

Goal: Verify the pointer-base route matches the source idea and avoids
testcase-shaped broadening.

Actions:

- Request reviewer scrutiny after the implementation or explicit fail-closed
  policy packet.
- Treat fixture-shaped matching, local edge rediscovery, downgraded existing
  support, stack-policy scope creep, or helper-only renames as blocking
  failures.

Completion check:

- A review artifact under `review/` reports no blocking findings, or the route
  is corrected before continuing.

### Step 4: Validate the Pointer-Base Broadening Slice

Goal: Prove the selected pointer-base policy does not regress the relevant
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

Goal: Decide whether idea 29 is complete or whether additional pointer-base
policy should move to separate durable follow-up ideas.

Actions:

- Record exactly which pointer-base forms are now supported.
- Preserve explicit caveats for unsupported pointer-base forms.
- If remaining work is adjacent but outside the completed slice, create or
  recommend separate durable follow-up ideas instead of expanding this plan.

Completion check:

- The source idea can be closed with concrete support or concrete fail-closed
  policy, or the active route is intentionally rewritten/deactivated with
  durable follow-up scope.

# 179 BIR Return-Chain Consumer Migration Runbook

Status: Active
Source Idea: ideas/open/179_bir_return_chain_consumer_migration.md

## Purpose

Move AArch64 return-chain consumers from public prepared return-chain helper reads
to the BIR-owned route now that oracle equivalence exists.

## Goal

AArch64 semantic terminal and next-operand return-chain recovery reads the BIR
route as the authority, while AArch64 keeps target-local register, ABI, scratch,
and emission policy.

## Core Rule

The migration changes the consumer authority only. Do not move target-local
AArch64 policy into BIR, do not contract the prepared API, and do not weaken
existing supported return-chain behavior.

## Read First

- `ideas/open/179_bir_return_chain_consumer_migration.md`
- `src/backend/mir/aarch64/codegen/alu.cpp`
- `src/backend/bir/bir.hpp`
- `src/backend/bir/bir.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- `tests/backend/mir/backend_aarch64_return_lowering_test.cpp`

## Current Targets

- Replace AArch64 semantic return-chain terminal lookups currently using
  `prepare::find_prepared_return_chain_terminal_value`.
- Replace AArch64 semantic return-chain next-operand lookups currently using
  `prepare::find_prepared_return_chain_next_operand_value`.
- Add or thread only the context projection AArch64 needs to address the
  BIR-owned route.
- Preserve fail-closed behavior when BIR has no answer or reports invalid or
  conflicting route state.

## Non-Goals

- Do not add the BIR schema or route 8 index from scratch.
- Do not add the initial prepared/BIR oracle equivalence suite.
- Do not remove, hide, or narrow prepared return-chain helper APIs.
- Do not move AArch64 value-home, ABI return move, register parsing,
  alias/scratch, scalar register view, ALU record construction, or emission
  order policy into BIR.
- Do not retarget tests by weakening expectations or marking supported paths
  unsupported.

## Working Model

- BIR route 8 owns return-chain terminal and next-operand identity.
- AArch64 consumes the BIR identities and maps them through existing
  target-local value-home and register machinery.
- Prepared helpers may remain available for diagnostics or explicitly bounded
  fallback only; they must not remain the semantic source for migrated
  consumers.

## Execution Rules

- Keep each implementation packet narrow enough to prove with a build plus the
  focused AArch64 return-chain tests and route 8 oracle tests.
- Prefer shared BIR lookup/projection helpers only when they reduce duplicated
  route access without absorbing AArch64 policy.
- Fail closed on missing, invalid, or conflicting BIR route answers instead of
  inventing target-local winners.
- If a packet changes a route facade, public lookup header, or context
  projection shape, escalate proof to broader backend validation.

## Ordered Steps

### Step 1: Map Existing Consumers

Goal: identify every AArch64 semantic terminal and next-operand consumer that
still reads prepared return-chain helpers.

Primary target: `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Inspect prepared helper call sites in AArch64 lowering.
- Classify each use as terminal recovery, next-operand alias/scratch recovery,
  diagnostic-only, or unrelated prepared lookup.
- Record any required BIR block/instruction/value context needed to query route
  8 without changing target policy.

Completion check:

- `todo.md` names the owned AArch64 call sites and the BIR context each one
  needs before code edits begin.

### Step 2: Add BIR Route Projection For AArch64

Goal: make the BIR-owned return-chain route addressable from the AArch64
consumer boundary.

Primary targets: `src/backend/mir/aarch64/codegen/alu.cpp`,
`src/backend/bir/bir.hpp`, and `src/backend/bir/bir.cpp` if an existing route
8 query is insufficient.

Actions:

- Reuse existing `bir::route8_find_return_chain_*` APIs when they provide the
  required identity and status.
- Add only minimal adapter/projection code needed to form a route 8 key from
  AArch64 lowering context.
- Keep target homes, register conversion, alias checks, scratch choice, and ALU
  record construction in AArch64 code.
- Preserve fail-closed status handling for no-match, invalid, and conflict
  cases.

Completion check:

- AArch64 can query BIR route 8 for the target consumer context without reading
  prepared helper answers for semantic authority.
- The build succeeds.

### Step 3: Migrate Terminal Return-Chain Recovery

Goal: replace prepared terminal helper reads with BIR-owned terminal identity
  reads.

Primary target: terminal recovery in `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace semantic calls to
  `prepare::find_prepared_return_chain_terminal_value` with BIR route 8
  terminal identity reads.
- Keep the existing AArch64 value-home lookup and register conversion path
  after the BIR identity is known.
- Add or update focused tests for accepted terminal identities and fail-closed
  missing/conflict terminal cases when current coverage does not prove the new
  consumer route.

Completion check:

- Focused AArch64 return-chain lowering tests still pass.
- Route 8 prepared/BIR oracle tests still pass.
- Prepared terminal helper reads are no longer the semantic authority for the
  migrated AArch64 terminal consumer.

### Step 4: Migrate Next-Operand Alias And Scratch Recovery

Goal: replace prepared next-operand helper reads with BIR-owned next-operand
identity reads.

Primary target: next-operand handling in `src/backend/mir/aarch64/codegen/alu.cpp`

Actions:

- Replace semantic calls to
  `prepare::find_prepared_return_chain_next_operand_value` with BIR route 8
  next-operand identity reads.
- Keep AArch64 alias checks, scalar register view, scratch selection, and ALU
  record construction target-local.
- Preserve behavior when BIR legitimately reports no next operand.
- Add or update focused tests for accepted next-operand identities and
  fail-closed next-operand conflicts when current coverage does not prove the
  new consumer route.

Completion check:

- Focused AArch64 return-chain lowering tests still pass.
- Route 8 prepared/BIR oracle tests still pass.
- Prepared next-operand helper reads are no longer the semantic authority for
  the migrated AArch64 next-operand consumer.

### Step 5: Bound Or Remove Prepared Fallbacks

Goal: ensure prepared return-chain helpers are not silently retained as the
semantic source for migrated consumers.

Primary targets: migrated AArch64 call sites and any temporary diagnostic
fallbacks introduced during Steps 2-4.

Actions:

- Remove semantic prepared fallback reads from migrated AArch64 consumers.
- If a prepared diagnostic or fallback remains, document in code or `todo.md`
  why it is bounded and what later idea removes it.
- Do not contract the public prepared API; that belongs to idea 180.

Completion check:

- A search for `find_prepared_return_chain_terminal_value` and
  `find_prepared_return_chain_next_operand_value` shows no migrated AArch64
  semantic consumer still depends on prepared answers.

### Step 6: Acceptance Validation

Goal: prove the migration without relying on weakened tests or named-case
shortcuts.

Actions:

- Run the focused AArch64 return-chain lowering tests.
- Run the route 8 prepared/BIR oracle equivalence tests from the previous idea.
- Run a build or compile proof covering touched targets.
- Escalate to broader backend validation if the diff changes BIR route
  facades, public lookup headers, or context projection shape.

Completion check:

- Fresh proof logs show the focused test set and any required broader backend
  validation pass.
- `todo.md` records the exact proof commands and results for supervisor
  acceptance.

## Reviewer Reject Signals

- The migrated consumer still depends on prepared answers without a bounded
  fallback removal plan.
- BIR records gain target homes, registers, scratch decisions, final ALU
  records, or other AArch64 policy.
- Missing or conflicting BIR answers are resolved by choosing target-local
  winners instead of failing closed.
- Existing supported return-chain behavior is weakened or retargeted to weaker
  tests.
- The work contracts prepared APIs or bundles idea 180 cleanup into this
  migration.

# BIR Return-Chain Schema/Index Runbook

Status: Active
Source Idea: ideas/open/177_bir_return_chain_schema_index.md

## Purpose

Create the BIR-owned return-chain record and function-local lookup/index for
same-block terminal return value and immediate next-operand identity.

## Goal

Add a target-neutral BIR return-chain route that can answer the same logical
schema/index questions as the current prepared return-chain helpers, without
migrating consumers or contracting the prepared API.

## Core Rule

Keep this route as BIR identity only: block, instruction reference or stable
index, chain value, terminal return value, and optional immediate next-operand
value. Do not import homes, registers, ABI return placement, scratch policy,
alias decisions, ALU record construction, or final emission order.

## Read First

- `ideas/open/177_bir_return_chain_schema_index.md`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`
- Existing BIR route/index patterns in `src/backend/bir/` and
  `src/backend/mir/query.cpp`

## Current Targets

- Add a distinct BIR return-chain route/schema.
- Add a cheap function-local index keyed by function/block/instruction
  position or stable instruction reference plus chain value.
- Define construction or omission behavior for accepted, rejected, ambiguous,
  and conflicting records.
- Add focused schema/index tests for positive, negative, ambiguous, and
  conflict fixtures.

## Non-Goals

- Do not migrate AArch64 consumers in
  `src/backend/mir/aarch64/codegen/alu.cpp`.
- Do not hide, privatize, narrow, or delete prepared return-chain helpers.
- Do not add oracle-equivalence coverage against prepared helper answers; that
  belongs to `ideas/open/178_bir_return_chain_oracle_equivalence.md`.
- Do not move target homes, registers, ABI moves, scratch choice, alias checks,
  ALU record construction, or final instruction emission policy into BIR.
- Do not fold this route into Route 1 producer identity or Route 7 comparison
  provenance.

## Working Model

- A return-chain record represents one same-block relationship from a chain
  value at an instruction position to the block return terminator's terminal
  return value.
- When the immediate next instruction consumes the current chain value as an
  operand, the record may also expose that next operand value.
- Unsupported opcodes, unnamed values, broken walks, non-return terminators,
  cross-block relationships, ambiguous cases, and conflicting duplicate
  publications must fail closed by omitting an answer or recording an explicit
  unavailable status.
- Duplicate publications for the same logical key must not choose a winner
  when terminal or next-operand answers conflict.

## Execution Rules

- Prefer existing route/index naming and test style over inventing a separate
  framework.
- Keep the public prepared return-chain API available as a future oracle.
- Build narrowly after each code-changing step.
- For proof, use the smallest backend/BIR test target that exercises the new
  route/index, then run the focused CTest subset named by the supervisor.
- Escalate to the supervisor if the route needs target-specific state or a
  consumer migration to prove usefulness.

## Ordered Steps

### Step 1: Inspect Existing Route and Return-Chain Shapes

Goal: identify the smallest local schema/index insertion point and test
fixture style.

Primary targets:

- `src/backend/bir/`
- `src/backend/mir/query.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp`

Actions:

- Inspect existing `Route*Record`, `Route*Index`, build, and find helpers.
- Inspect `PreparedReturnChainLookups`,
  `prepared_return_chain_value_key(...)`,
  `make_prepared_return_chain_lookups(...)`,
  `find_prepared_return_chain_terminal_value(...)`, and
  `find_prepared_return_chain_next_operand_value(...)`.
- Identify which existing backend/BIR test file can host focused schema/index
  fixtures without testing consumer migration.
- Record any unavoidable naming or placement decision in `todo.md`.

Completion check:

- The executor can name the concrete files/functions that will receive the new
  route, index, and tests.

### Step 2: Add Return-Chain Record and Key Types

Goal: introduce the target-neutral BIR schema for return-chain identity.

Primary targets:

- BIR route/header/source files matching local route patterns.

Actions:

- Add record/status/query/key types for block identity, instruction reference
  or stable instruction index, chain value, terminal return value, and optional
  immediate next-operand value.
- Represent unsupported, unavailable, ambiguous, and conflict cases without
  selecting a conflicting winner.
- Keep payload free of target homes, register names, ABI placement, scratch
  decisions, emitted instruction order, and prepared helper ownership.

Completion check:

- The new types compile and can express accepted, omitted, ambiguous, and
  conflicting return-chain outcomes.

### Step 3: Build Function-Local Index and Lookup Helpers

Goal: make return-chain answers cheap to query by the logical key used by the
prepared helpers.

Primary targets:

- BIR route/index implementation files matching local patterns.

Actions:

- Build a function-local index from BIR blocks and instructions.
- Key lookups by function/block/instruction reference or stable instruction
  index plus chain value.
- Reject or omit answers for unsupported opcodes, unnamed values, broken
  same-block walks, non-return terminators, cross-block relationships, and
  conflicting duplicate records.
- Ensure key stability expectations are explicit: rebuild the index when the
  indexed BIR block/function changes.

Completion check:

- Positive lookups return terminal and optional next-operand identity; negative,
  ambiguous, and conflict lookups fail closed.

### Step 4: Add Focused Schema/Index Tests

Goal: prove the new route/index behavior without using consumer migration or
API contraction as evidence.

Primary targets:

- `tests/backend/bir/backend_prepared_lookup_helper_test.cpp` or the closest
  existing backend/BIR route test file.

Actions:

- Add positive same-block return-chain fixtures for terminal return value and
  immediate next-operand identity.
- Add negative fixtures for unsupported opcode, unnamed value, broken walk,
  non-return terminator, and cross-block relationship.
- Add ambiguous/conflict fixtures proving no winner is selected for duplicate
  conflicting publications.
- Avoid expectation rewrites that weaken existing supported behavior.

Completion check:

- Focused tests fail before the implementation or explicitly exercise the new
  missing behavior, then pass after the route/index exists.

### Step 5: Validate and Hand Off

Goal: leave the route ready for the oracle-equivalence idea without changing
  consumers.

Actions:

- Run the delegated build and focused CTest command.
- Summarize proof in `todo.md`.
- Confirm prepared return-chain helpers and AArch64 consumers remain public and
  behaviorally unchanged.
- Identify `ideas/open/178_bir_return_chain_oracle_equivalence.md` as the next
  lifecycle prerequisite once schema/index work is complete.

Completion check:

- Build and focused tests are green, `todo.md` records proof, and no consumer
  migration or prepared API contraction is included in the slice.

# Prepared Pending Store-Global Publication Producer Contract

Status: Active
Source Idea: ideas/open/125_prepared_pending_store_global_publication_producer_contract.md

## Purpose

Move pending store-global stack-value publication producer selection into the
shared prepared publication contract.

## Goal

AArch64 memory lowering consumes a prepared producer instruction/index fact
instead of rediscovering the producer by scanning BIR instructions.

## Core Rule

Shared prepared/prealloc code owns producer identity and fail-closed behavior;
AArch64 memory lowering owns only target-local emission from the prepared fact.

## Read First

- `ideas/open/125_prepared_pending_store_global_publication_producer_contract.md`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

## Current Targets

- Extend `PreparedStoreGlobalPublicationCandidate` or an adjacent prepared
  publication record so pending store-global candidates expose the resolved
  source producer instruction/index when available.
- Teach `plan_pending_prepared_store_global_publications` to populate that fact
  from shared prepared/BIR inputs and to fail closed when no unique producer is
  available.
- Update `lower_pending_store_global_stack_value_publications` to consume the
  prepared producer fact.
- Add shared-prepared and AArch64 consumer coverage.

## Non-Goals

- Do not rework general store-source publication planning already covered by
  ideas 34, 39a, and 111.
- Do not reopen broader AArch64 memory prepared-authority cleanup from idea 50.
- Do not move AArch64 opcode choice, scratch policy, machine-record
  construction, or memory operand emission into shared prepared code.
- Do not create an AArch64 memory-private physical split in this runbook.

## Working Model

- The pending store-global prepared publication candidate should carry the
  producer instruction/index needed by consumers.
- Ambiguous or missing producers should prevent candidate publication rather
  than forcing target backends to recover identity by name/type.
- AArch64 should check publication availability, select/register stack
  publication state, call the existing publication emitter with the prepared
  producer instruction, and spell the resulting machine instruction.

## Execution Rules

- Keep source-idea intent stable; routine execution progress belongs in
  `todo.md`.
- Prefer small code slices that prove shared contract behavior before consumer
  rewiring.
- Do not replace the existing AArch64 scan with another name-shaped shortcut,
  testcase-only match, or hard-coded instruction offset.
- Do not weaken unsupported contracts or rewrite expectations as a substitute
  for capability repair.
- For code-changing steps, provide fresh build or focused test proof. The
  supervisor owns any broader acceptance gate.

## Ordered Steps

### Step 1: Shared Prepared Producer Contract

Goal: Add the prepared producer identity fact to pending store-global
publication planning.

Primary Targets:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`

Actions:

- Inspect `PreparedStoreGlobalPublicationCandidate` and adjacent publication
  records for the narrowest place to store source producer instruction/index.
- Extend the prepared record so pending store-global candidates can expose the
  resolved producer instruction/index.
- Update `plan_pending_prepared_store_global_publications` to populate the
  producer fact from shared prepared/BIR inputs.
- Make missing or ambiguous producer identity fail closed in shared planning.
- Add focused shared-prepared tests for a candidate carrying the expected
  producer instruction/index.
- Add focused shared-prepared tests for absent and ambiguous producer
  fail-closed behavior when practical within the existing test harness.

Completion Check:

- Shared prepared tests prove the producer fact exists on valid pending
  store-global candidates and that planner behavior fails closed without a
  unique producer.
- No AArch64-local producer rediscovery changes are required to make the shared
  tests meaningful.

### Step 2: AArch64 Consumer Rewire

Goal: Remove AArch64-local producer rediscovery from pending store-global
publication lowering.

Primary Targets:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`

Actions:

- Update `lower_pending_store_global_stack_value_publications` to consume the
  prepared producer instruction/index fact from the pending publication plan.
- Remove the scan over earlier `context.bir_block->insts` that matches
  `result->name == plan.source_value.name` plus type.
- Preserve existing duplicate-publication, stack-home-only, and
  pending-publication behavior.
- Keep target-local emission responsibilities in AArch64: availability checks,
  stack publication state, existing publication emitter calls, and machine
  instruction spelling.
- Update AArch64 prepared memory operand records coverage so the pending
  publication path emits through the prepared producer fact.

Completion Check:

- `lower_pending_store_global_stack_value_publications` no longer scans earlier
  BIR instructions to rediscover the producer by result name/type.
- AArch64 tests still prove stack-homed pending publication emission.

### Step 3: Focused Backend Proof

Goal: Prove the shared contract and AArch64 consumer behavior together.

Primary Targets:

- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `tests/backend/mir/backend_aarch64_prepared_memory_operand_records_test.cpp`
- build/test commands selected by the supervisor

Actions:

- Run the focused backend test subset selected by the supervisor, including
  both target test files.
- Inspect failures for route drift, testcase-shaped matching, or expectation
  downgrades before accepting a green narrow subset.
- Record exact proof commands and results in `todo.md`.

Completion Check:

- Focused backend proof is green for the shared prepared contract and AArch64
  consumer coverage.
- Any remaining validation needs are explicit for the supervisor.


# Prealloc Call-Boundary Ordering And Republication Runbook

Status: Active
Source Idea: ideas/open/prealloc-call-boundary-ordering-republication.md

## Purpose

Move generic call-boundary ordering and republication intent into prealloc call
plans while keeping target ABI policy and final machine emission in AArch64.

## Goal

Extend prepared call plans so before-call moves, after-call moves, return-value
publication, preservation-home population, source/destination effect resources,
and republication intent are available before target call emission.

## Core Rule

Move ordering and intent facts, not ABI or instruction policy. AArch64 must
continue to own ABI lane assignment, variadic details, byval copy spelling,
indirect callee materialization, concrete register spelling, and final machine
instruction construction.

## Read First

- `ideas/open/prealloc-call-boundary-ordering-republication.md`
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp`

## Current Scope

- Generic before-call and after-call move ordering around prepared call plans.
- Return-value and preserved-value republication intent that can be described
  without AArch64 register spelling or machine instructions.
- Source and destination effect resources needed to reason about call-boundary
  moves before target lowering.
- AArch64 adapters that consume the richer plan while preserving current call
  behavior.
- One x86 or RISC-V proof path that consumes the generic ordering model
  without AArch64 codegen headers.

## Non-Goals

- Do not move AArch64 ABI lane policy, concrete register names, variadic ABI
  details, F128 carrier policy, byval copy instructions, indirect callee
  materialization, stack-pointer sequences, or final machine moves into
  prealloc.
- Do not change target ABI classification rules or machine move emission.
- Do not include general scalar publication planning outside call-boundary
  republication intent.
- Do not include store-source planning, branch fusion, dynamic-stack
  operations, or unrelated call-lowering rewrites.
- Do not weaken tests or convert supported call paths to unsupported.

## Working Model

Prepared call plans should describe which call-boundary effects must happen and
in what generic order. Target lowering then maps those effects to ABI lanes,
concrete registers, memory operands, stack adjustments, and machine
instructions.

## Execution Rules

- Start with mapping and classification before changing implementation files.
- Keep each implementation step behavior-preserving and small enough to prove
  with build plus focused backend coverage.
- Add neutral records or helpers only for facts that x86/RISC-V could consume
  without importing AArch64 emission policy.
- Keep compatibility wrappers when they reduce churn, but do not count a rename
  of old AArch64-owned logic as migration progress.
- Escalate validation when a step touches shared prealloc call plans, move
  classification, or multiple AArch64 call emission paths.

## Step 1: Map Call-Boundary Ordering Surfaces

Goal: identify the exact prealloc and AArch64 surfaces that currently own
generic call-boundary ordering and republication intent.

Primary targets:
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`

Actions:
- Inspect before-call, after-call, return-value, preservation, indirect-callee,
  and byval call-boundary paths.
- Classify each discovered fact as generic ordering, generic republication
  intent, AArch64 ABI/emission policy, or mixed policy needing a target hook.
- Record the smallest viable first extraction boundary in `todo.md`.

Completion check:
- `todo.md` lists candidate planning facts, keep-local facts, and the first
  implementation slice without changing implementation files.

## Step 2: Extend Neutral Call-Boundary Plan Data

Goal: add prealloc-owned records or helper surfaces for generic call-boundary
ordering and republication intent.

Primary targets:
- `src/backend/prealloc/calls.hpp`
- `src/backend/prealloc/call_plans.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`

Actions:
- Represent before-call, after-call, return-value, preserved-value, source
  effect, destination effect, and republication intent facts without AArch64
  register spelling or instruction objects.
- Preserve existing prepared call-plan data used by AArch64 consumers.
- Add focused record-level coverage when there is a local test seam.

Completion check:
- The extended plan builds, is inspectable before target call emission, and
  does not depend on AArch64 codegen headers.

## Step 3: Adapt AArch64 Call Consumption

Goal: rewire AArch64 call lowering to consume the neutral ordering and
republication facts while preserving emitted behavior.

Primary targets:
- `src/backend/mir/aarch64/codegen/dispatch_calls.cpp`
- `src/backend/mir/aarch64/codegen/calls.cpp`
- `src/backend/mir/aarch64/codegen/calls_moves.cpp`
- `src/backend/mir/aarch64/codegen/calls_preservation.cpp`
- `src/backend/mir/aarch64/codegen/calls_argument_sources.cpp`
- `src/backend/mir/aarch64/codegen/calls_byval_aggregates.cpp`

Actions:
- Introduce narrow AArch64 adapters for the neutral call-boundary facts.
- Migrate one call-boundary path at a time: before-call moves, after-call
  moves, return-value publication, preservation, and republication intent.
- Keep ABI lane selection, variadic behavior, byval emission, indirect callee
  materialization, register spelling, stack-pointer sequences, and instruction
  construction in AArch64.

Completion check:
- AArch64 call behavior remains equivalent for touched paths, and old
  target-local generic ordering logic is reduced rather than renamed.

## Step 4: Add Cross-Target Ordering Proof

Goal: prove the generic call-boundary ordering model can be consumed by another
target without inheriting AArch64 emission policy.

Primary targets:
- `tests/backend/mir/`
- x86 or RISC-V call-plan test or fixture surfaces

Actions:
- Add an x86 or RISC-V compile/unit fixture or adapter that consumes the
  neutral call-boundary ordering and republication records.
- Ensure the proof includes no AArch64 codegen headers and no AArch64-specific
  operand, ABI lane, or instruction assumptions.

Completion check:
- The cross-target proof builds and demonstrates generic ordering-model
  consumption without AArch64 dependencies.

## Step 5: Validate And Close Readiness

Goal: prove the migration did not regress backend behavior and decide whether
the source idea is complete.

Actions:
- Run build proof for affected C++ targets.
- Run focused AArch64 backend tests covering before-call moves, after-call
  moves, preserved values, indirect callees, byval handling, and return values
  touched by the migration.
- Run the x86/RISC-V reuse proof.
- Escalate to broader backend or regression-guard validation if shared
  prealloc call-plan surfaces or multiple call emission paths changed.
- Record remaining target-policy-adjacent call wrappers in `todo.md`; do not
  keep the idea open for wrappers that are intentionally out of scope.

Completion check:
- Validation is green, the cross-target reuse path exists, and the source
  idea's acceptance criteria are satisfied or explicitly handed off to a
  separate follow-up idea.

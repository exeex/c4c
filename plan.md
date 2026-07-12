# X86 Named Handoff Materializer Cleanup Runbook

Status: Active
Source Idea: ideas/open/708_x86_named_handoff_materializer_cleanup.md
Resumed after completion of: ideas/closed/716_prepared_call_plan_cursor_complete_production.md

## Purpose

Resume removal of executable route dependencies from x86 MIR materialization
now that cursor-complete common prepared-call production is accepted.

## Goal

Make x86 semantic materializers consume common named and prepared views as
their sole handoff authority, fail closed when prepared placement authority is
missing or inconsistent, and reach zero route-vocabulary hits in semantic x86
materialization files.

## Core Rule

Common/prepared producers own handoff semantics. X86 may realize those facts
as target instructions and ABI operations, but must not rebuild route indexes,
use route agreement as an emission condition, or retain route-derived
fallbacks.

## Read First

- `ideas/open/708_x86_named_handoff_materializer_cleanup.md`
- `ideas/closed/706_common_mir_named_query_migration.md`
- `ideas/closed/716_prepared_call_plan_cursor_complete_production.md`
- `src/backend/mir/x86/module/module.cpp`
- `src/backend/mir/x86/x86.hpp`
- `tests/backend/bir/backend_x86_handoff_boundary_multi_defined_call_test.cpp`
- `tests/backend/bir/backend_x86_handoff_boundary_joined_branch_test.cpp`

## Current Scope

- X86 semantic MIR materialization, beginning in `src/backend/mir/x86/module/module.cpp`.
- Direct-call, scalar, memory, publication, and joined-branch handoff paths.
- Common named/prepared view consumption and fail-closed placement checks.
- Focused x86 boundary proof plus broader backend validation at acceptance.

## Non-Goals

- Do not migrate AArch64 or RV64 consumers owned by ideas 709 and 710.
- Do not quarantine or delete private BIR route implementations owned by idea 711.
- Do not perform route-labelled debug and fixture vocabulary cleanup owned by idea 712 unless a label directly selects x86 codegen.
- Do not change x86 ABI or instruction-selection policy.
- Do not weaken expectations, supported behavior, or proof breadth.

## Working Model

- Step 1 inventory is complete and Step 2 resumes after its earlier attempted slice was reverted.
- Idea 716 now guarantees cursor-exact prepared call plans from semantic operands with optional unique relationship refinement.
- Common named queries identify semantic values and memory/publication facts; prepared views own placement, homes, moves, preservation, edge publication, frame, and ABI handoff facts.
- Missing, ambiguous, incomplete, or inconsistent prepared authority rejects materialization; route records never provide compatibility authority.

## Execution Rules

- Migrate one coherent consumer family at a time and keep nearby positive and fail-closed proof green.
- Replace route-derived decisions with an existing named/prepared contract; if that contract is incomplete, stop and record the producer gap instead of reconstructing authority in x86.
- Remove target-local route indexes and compatibility fallbacks as their last semantic consumers disappear.
- Audit semantic files under `src/backend/mir/x86/`, distinguishing executable dependencies from debug-only vocabulary deferred to idea 712.
- Run the supervisor-delegated build and focused tests for each code step; require broader backend validation before closure.

## Ordered Steps

### Step 1: Inventory x86 route dependencies and bind migration packets

Goal: classify every route-derived x86 semantic decision by consumer family and identify its named/prepared replacement authority.

Completion check:

- Every semantic route dependency has a bounded owner, replacement contract, negative-state rule, and proof surface. This inventory was completed before idea 716 blocked Step 2.

### Step 2: Migrate direct-call and scalar handoff materialization

Goal: remove route-derived call argument and scalar source authority from x86 materialization.

Actions:

- Consume cursor-exact prepared call, argument, preservation, return, value-home, move, and ABI facts.
- Remove x86-owned route indexes and route-derived source fallback for migrated paths.
- Preserve target-local register, stack-argument, frame, and instruction choices only after prepared authority validates.
- Prove positive and fail-closed direct-call/scalar behavior across more than one fixture shape.

Completion check:

- Direct-call and scalar emission no longer depends on route records or fallback, missing prepared authority rejects precisely, and focused x86 call proof is green.

### Step 3: Migrate memory and edge-publication materialization

Goal: remove Route 3/5 agreement and compatibility decisions from x86 memory and publication paths.

Actions:

- Consume common/prepared source-memory, frame-slot, value-home, move-bundle, and edge-publication facts directly.
- Remove target-local Route 3 memory lookup and Route 5 publication agreement or compatibility gates.
- Preserve address, width, volatility, alignment, frame, and move identity checks using producer-owned facts.
- Prove local/global memory and publication behavior plus fail-closed mutations.

Completion check:

- X86 memory and edge-publication emission is selected only by named/prepared authority and focused proof is green.

### Step 4: Migrate joined-branch and control materialization

Goal: consume prepared branch, join, value, memory, and publication authority without route reconstruction.

Actions:

- Replace route-derived selected-arm, source-memory, join, and transfer decisions with common/prepared views.
- Preserve x86 comparison, branch, label, and instruction realization while validating producer-owned identity.
- Reject unavailable or inconsistent authority without plain-branch, source-order, or route-agreement fallback.
- Prove joined scalar, local-memory, publication, and control shapes plus fail-closed mutations.

Completion check:

- Joined-branch/control materialization contains no executable route dependency and supported plus invalid shapes are proven.

### Step 5: Enforce the x86 retirement guard and acceptance proof

Goal: establish closure-quality evidence that semantic x86 materialization is route-independent.

Actions:

- Audit `src/backend/mir/x86/` and classify any remaining route vocabulary as debug-only for idea 712 or a blocking semantic dependency.
- Confirm no helper rebuilds route indexes, hides route analysis, or uses debug state to select codegen.
- Run focused direct-call and joined-branch proof across scalar, memory, publication, call, and fail-closed placement behavior.
- Run the supervisor-selected broader backend before/after regression guard.

Completion check:

- Semantic x86 materialization has zero route-vocabulary dependencies, debug residue cannot affect lowering, focused and broader proof are green, and no contract was weakened.

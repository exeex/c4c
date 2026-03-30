# Backend Ready-IR Contract Runbook

Status: Active
Source Idea: ideas/open/35_backend_ready_ir_contract_plan.md
Activated from: ideas/closed/38_lir_de_stringify_legacy_safe_refactor_plan.md

## Purpose

Define a first-class backend-owned IR contract so backend work stops depending
on `src/backend/lir_adapter.cpp` shape recognition as the semantic ingestion
layer.

## Goal

Land the backend IR data model, validator, and printer contract that later
lowering and emitter migration work can target directly.

## Core Rule

Do not solve backend coverage by extending string-heavy adapter pattern
matching. New backend contract work must prefer typed backend-owned data
structures and explicit transition seams scheduled for deletion.

## Read First

- [ideas/open/35_backend_ready_ir_contract_plan.md](/workspaces/c4c/ideas/open/35_backend_ready_ir_contract_plan.md)
- [src/backend/lir_adapter.hpp](/workspaces/c4c/src/backend/lir_adapter.hpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- [src/backend/backend.hpp](/workspaces/c4c/src/backend/backend.hpp)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)
- [src/codegen/lir/ir.hpp](/workspaces/c4c/src/codegen/lir/ir.hpp)
- [src/codegen/lir/call_args.hpp](/workspaces/c4c/src/codegen/lir/call_args.hpp)
- [ref/claudes-c-compiler/src/ir/module.rs](/workspaces/c4c/ref/claudes-c-compiler/src/ir/module.rs)
- [ref/claudes-c-compiler/src/ir/instruction.rs](/workspaces/c4c/ref/claudes-c-compiler/src/ir/instruction.rs)
- [ref/claudes-c-compiler/src/backend/README.md](/workspaces/c4c/ref/claudes-c-compiler/src/backend/README.md)

## Current Scope

- backend-owned IR data structures under `src/backend/`
- backend IR validation and debug-print support
- explicit backend dispatch boundary between LIR and target emitters
- transition plan for retiring adapter-owned call normalization

## Non-Goals

- implementing the full `LIR -> backend IR` lowering pipeline in this runbook
- migrating x86 or AArch64 emitters off LIR in this runbook
- broad backend testsuite convergence work
- reviving raw string-based backend dispatch for calls, arithmetic, or compares

## Working Model

1. Audit the current backend ingress surface and adapter-owned assumptions.
2. Define backend-owned IR entities and validation invariants.
3. Add printer and validation support that makes the contract observable in
   focused tests.
4. Introduce an explicit lowering entry seam in backend dispatch without
   attempting full lowering yet.
5. Record the handoff required for ideas `36` and `37`.

## Execution Rules

- Treat `src/codegen/lir/call_args.hpp` and the typed LIR wrappers in
  `src/codegen/lir/ir.hpp` as transition inputs, not as the backend contract.
- Keep adapter compatibility isolated; do not expand `parse_backend_*` helper
  surface unless it is required to bridge a narrow migration step.
- Prefer focused backend IR tests over end-to-end backend feature work.
- Record every remaining adapter-owned compatibility seam in `plan_todo.md`.

## Ordered Steps

### Step 1: Audit the current backend ingress contract

Goal: inventory what the backend currently assumes about LIR and adapter-owned
call normalization.

Primary targets:

- [src/backend/lir_adapter.hpp](/workspaces/c4c/src/backend/lir_adapter.hpp)
- [src/backend/lir_adapter.cpp](/workspaces/c4c/src/backend/lir_adapter.cpp)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)
- [src/backend/x86/codegen/emit.cpp](/workspaces/c4c/src/backend/x86/codegen/emit.cpp)
- [src/backend/aarch64/codegen/emit.cpp](/workspaces/c4c/src/backend/aarch64/codegen/emit.cpp)

Actions:

- list the current adapter-owned instruction/value/block/module abstractions
- enumerate which backend consumers still depend on adapter-specific call or
  control-flow normalization
- capture the typed-LIR compatibility seams inherited from `38`

Completion check:

- `plan_todo.md` records a concrete audit of the current backend ingress
  surface and the transition seams that the backend IR contract must replace

### Step 2: Define the backend-owned IR model

Goal: introduce the data model that will become the backend-facing contract.

Primary targets:

- [src/backend/ir.hpp](/workspaces/c4c/src/backend/ir.hpp)
- [src/backend/ir.cpp](/workspaces/c4c/src/backend/ir.cpp)

Actions:

- define backend module, function, block, instruction, terminator, global, and
  extern entities
- make call, compare, and memory semantics structured instead of adapter-text
  driven
- keep the model narrow enough that `36` can lower into it incrementally

Completion check:

- the backend IR types compile and express the current supported backend
  semantics without relying on raw adapter string payloads

### Step 3: Add backend IR validation and printer seams

Goal: make the new contract observable and reject malformed backend IR early.

Primary targets:

- [src/backend/ir_validate.hpp](/workspaces/c4c/src/backend/ir_validate.hpp)
- [src/backend/ir_validate.cpp](/workspaces/c4c/src/backend/ir_validate.cpp)
- [src/backend/ir_printer.hpp](/workspaces/c4c/src/backend/ir_printer.hpp)
- [src/backend/ir_printer.cpp](/workspaces/c4c/src/backend/ir_printer.cpp)

Actions:

- add validation for structural module/function/block/instruction invariants
- add a printer or dump format usable from focused tests
- cover malformed and well-formed backend IR cases with targeted tests

Completion check:

- tests can build, validate, and print representative backend IR fixtures

### Step 4: Make backend dispatch own an explicit lowering boundary

Goal: create the contract seam that later lowering work will fill in.

Primary targets:

- [src/backend/backend.hpp](/workspaces/c4c/src/backend/backend.hpp)
- [src/backend/backend.cpp](/workspaces/c4c/src/backend/backend.cpp)

Actions:

- add the explicit `lower_to_backend_ir(...)` boundary to backend dispatch APIs
- keep `src/backend/lir_adapter.{hpp,cpp}` transition-only
- avoid pretending the lowering pipeline is complete before `36`

Completion check:

- backend dispatch exposes a backend-owned IR contract instead of leaving the
  emitter boundary implicit in `LirModule`

### Step 5: Record the handoff to `36` and `37`

Goal: leave the next backend plans with a stable contract and explicit cleanup
targets.

Primary targets:

- [ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md](/workspaces/c4c/ideas/open/36_lir_to_backend_ready_ir_lowering_plan.md)
- [ideas/open/37_backend_emitter_backend_ir_migration_plan.md](/workspaces/c4c/ideas/open/37_backend_emitter_backend_ir_migration_plan.md)

Actions:

- record which backend IR types and validators are stable
- record which adapter compatibility seams remain temporary
- record which emitter assumptions must move behind the lowering boundary

Completion check:

- follow-on ideas can resume without re-deriving the backend contract from
  source or git history

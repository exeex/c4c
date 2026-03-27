# Backend LIR Adapter Runbook

Status: Active
Source Idea: ideas/open/01_backend_lir_adapter_plan.md

## Purpose

Create the narrowest backend attachment layer so a C++ port of `ref/claudes-c-compiler/src/backend/` can consume the existing LIR without forcing an early backend redesign.

## Goal

Bring up a minimal backend entry path that can lower trivial return-only programs through an isolated adapter layer and an external assembler/linker fallback.

## Core Rule

Keep backend core code mechanically close to the ref backend and keep all LIR-shape translation inside a clearly bounded adapter boundary.

## Read First

- `ideas/open/01_backend_lir_adapter_plan.md`
- `ideas/open/__backend_port_plan.md`
- `ref/claudes-c-compiler/src/backend/`
- `src/codegen/lir/`
- `CMakeLists.txt`

## Scope

- create `src/backend/` skeleton and top-level entry wiring
- define target selection and backend factory boundaries
- define the dispatch contract from `LirModule` / `LirInst` into backend codegen
- choose the first-pass Stage 3 handling policy
- keep Stage 3 translation inside the adapter layer
- wire external assembler/linker fallback for first executable bring-up
- add minimal tests for backend entry and trivial executable flow

## Non-Goals

- full target instruction coverage
- register allocation
- built-in assembler or linker
- redesigning existing LIR structures
- broad refactors outside the backend-entry slice

## Working Model

- Treat `src/backend/` as the new execution boundary.
- Keep target-independent adapter logic separate from target-specific codegen.
- Prefer a thin shim from current LIR into backend-local expectations over changing existing LIR producers.
- First executable bring-up may use external assembler/linker invocation as long as the contract is explicit and testable.

## Execution Rules

- Keep patches mechanical and narrowly scoped.
- Do not change `src/codegen/lir/` structures unless the current slice cannot proceed otherwise.
- Do not bury Stage 3 policy decisions inside target-specific code.
- Add targeted tests before or alongside new behavior.
- Separate adapter failures from target codegen failures in tests and diagnostics.

## Ordered Steps

### Step 1: Inspect backend and LIR attachment surfaces

Goal: identify the minimum seam between the existing LIR and the ref backend shape.

Primary targets:

- `ref/claudes-c-compiler/src/backend/`
- `src/codegen/lir/`
- current driver / codegen entrypoints

Concrete actions:

- inspect the ref backend entry structure and required input contracts
- inspect current LIR module/instruction types and where backend invocation should attach
- record the smallest viable adapter contract for a return-only bring-up

Completion check:

- the active plan names the exact files and types that will define the first adapter boundary

### Step 2: Add backend skeleton and entry wiring

Goal: establish `src/backend/` as a compilable subsystem with a stable entry point.

Primary targets:

- `src/backend/`
- build-system wiring
- top-level compiler driver path

Concrete actions:

- create backend scaffolding, target enum/factory, and entry-point interfaces
- wire CMake and top-level compilation so the backend path builds cleanly
- keep unimplemented target behavior explicit and narrow

Completion check:

- the tree builds with the new backend skeleton in place and no accidental target-specific behavior spread outside `src/backend/`

### Step 3: Implement the first LIR adapter slice

Goal: translate the minimal trivial-program subset into backend-facing operations.

Primary targets:

- adapter layer between `LirModule` / `LirInst` and backend codegen

Concrete actions:

- choose and encode the first-pass Stage 3 handling policy
- keep translation logic in one adapter boundary
- support the narrowest return-only program slice needed for executable bring-up

Completion check:

- a trivial return-only program reaches backend emission through the adapter without requiring LIR redesign

### Step 4: Wire external assembler/linker fallback

Goal: turn backend output into a runnable executable through a stable external-tools contract.

Primary targets:

- tool discovery / invocation path
- backend output handoff

Concrete actions:

- define how target selection maps to external tool invocation
- surface toolchain failures clearly
- keep the contract deterministic and easy to test

Completion check:

- backend output can be assembled and linked through the external fallback path for the minimal supported target flow

### Step 5: Add minimal validation coverage

Goal: lock down the first working slice and make failures attributable.

Primary targets:

- backend entry tests
- trivial executable integration tests

Concrete actions:

- add tests for target factory / dispatch entry
- add validation for `int main() { return 0; }`
- add validation for `return 2 + 3` with exit code `5`
- ensure failures distinguish adapter problems from target codegen or toolchain problems where practical

Completion check:

- targeted tests cover backend entry and trivial executable bring-up for the first slice

## Exit Criteria

- `src/backend/` exists and is wired into the build
- one explicit adapter contract bridges existing LIR into backend codegen
- trivial return-only programs lower through backend entry
- external assembler/linker fallback is wired for the first bring-up path
- minimal tests cover backend dispatch and trivial executable execution

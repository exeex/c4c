# AArch64 Local Memory Addressing Runbook

Status: Active
Source Idea: ideas/open/09_backend_aarch64_local_memory_addressing_plan.md
Activated from: ideas/open/09_backend_aarch64_local_memory_addressing_plan.md

## Purpose

Extend the AArch64 backend from the narrow register-only bring-up seam into the first explicit backend-owned local stack-addressing path, starting with the bounded `local_array.c` case.

## Goal

Promote one local-array runtime case onto the AArch64 asm path by making stack-slot base-address formation and indexed local load/store lowering explicit, test-backed backend behavior.

## Core Rule

Keep this runbook limited to stack-local addressing for one bounded local-array slice. Do not widen into global-addressing, broad pointer arithmetic, linker work, or general aggregate lowering.

## Read First

- [ideas/open/09_backend_aarch64_local_memory_addressing_plan.md](/Users/chi-shengwu/c4c/ideas/open/09_backend_aarch64_local_memory_addressing_plan.md)
- [ideas/closed/02_backend_aarch64_port_plan.md](/Users/chi-shengwu/c4c/ideas/closed/02_backend_aarch64_port_plan.md)
- [ideas/open/10_backend_aarch64_global_addressing_plan.md](/Users/chi-shengwu/c4c/ideas/open/10_backend_aarch64_global_addressing_plan.md)
- [ideas/open/__backend_port_plan.md](/Users/chi-shengwu/c4c/ideas/open/__backend_port_plan.md)
- `tests/c/internal/backend_case/local_array.c`
- `tests/backend/backend_lir_adapter_tests.cpp`
- the current AArch64 adapter and emitter surfaces under `src/backend/aarch64/`

## Current Targets

- capture the real AArch64 LIR and asm shape for `local_array.c`
- define the narrowest backend-owned representation for stack-local base address plus indexed element access
- promote `backend_runtime_local_array` to `BACKEND_OUTPUT_KIND=asm` only when the backend seam is explicit and test-backed

## Non-Goals

- global-address materialization
- broad pointer arithmetic lowering
- struct/member-array generalization outside the first local-array slice
- built-in assembler or linker work
- regalloc or peephole cleanup beyond what the bounded local-memory slice strictly requires

## Working Model

- treat `local_array.c` as the contract for the first local-memory slice
- prefer explicit backend seams over ad hoc LIR collapse
- add or tighten the narrowest synthetic adapter/emitter test before widening runtime coverage
- split a new idea instead of silently absorbing broader local/global address lowering

## Execution Rules

- inspect the exact frontend-emitted AArch64 LIR before changing lowering behavior
- keep the first slice centered on stack-local integer arrays with straightforward indexed addressing
- route only the bounded runtime case through backend-owned asm once synthetic coverage proves the seam
- record any broader follow-on cases back into open ideas instead of expanding this runbook ad hoc

## Step 1: Pin The First Local-Array Contract

Goal: capture the exact `local_array.c` LIR and the smallest synthetic backend shape that matches it.

Primary targets:

- `tests/c/internal/backend_case/local_array.c`
- `tests/backend/backend_lir_adapter_tests.cpp`
- AArch64 LIR inspection helpers or dump paths

Actions:

- inspect the current frontend-emitted AArch64 LIR for `local_array.c`
- identify the smallest synthetic backend fixture that matches the same stack-slot and indexed-address pattern
- record the exact expected stack-local base-address, derived-address, and load/store behavior for the first slice

Completion check:

- one concrete local-array addressing slice is named with explicit LIR and asm expectations

## Step 2: Make The Backend-Owned Local Addressing Seam Explicit

Goal: add the narrowest adapter and/or emitter seam needed to represent stack-local base address plus indexed local access.

Primary targets:

- `src/backend/aarch64/`
- `tests/backend/backend_lir_adapter_tests.cpp`

Actions:

- add or tighten targeted synthetic tests before implementation
- introduce only the backend-owned representation needed for the bounded local-array slice
- keep the seam explicit instead of hiding it inside one-off pattern collapse

Completion check:

- synthetic backend coverage names and validates the first explicit local-memory addressing seam

## Step 3: Promote The Runtime Case Through AArch64 Asm

Goal: route `backend_runtime_local_array` through `BACKEND_OUTPUT_KIND=asm` using the explicit local-memory seam from Step 2.

Primary targets:

- runtime/backend tests under `tests/c/internal/`
- AArch64 adapter/emitter code under `src/backend/aarch64/`

Actions:

- implement the minimum adapter/emitter changes needed for the bounded local-array runtime case
- keep the implementation restricted to stack-local integer array addressing
- validate runtime behavior once the synthetic coverage is passing

Completion check:

- `backend_runtime_local_array` passes on the AArch64 asm path without widening into broader address-lowering work

## Acceptance Checks

- the first local-array addressing seam is explicit in backend code and tests
- `tests/backend/backend_lir_adapter_tests.cpp` covers the exact bounded local-memory slice
- `backend_runtime_local_array` runs with `BACKEND_OUTPUT_KIND=asm`
- broader local/global address lowering remains out of scope

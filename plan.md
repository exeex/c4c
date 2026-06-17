# RV64 Runtime Defined Global Scalar Memory Foundation Runbook

Status: Active
Source Idea: ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md

## Purpose

Build the first qemu-backed RV64 runtime support for defined scalar global
memory without widening the backend route beyond simple i32 globals.

Goal: make prepared RV64 assembly handle initialized global int loads,
zero-initialized global int loads, and global int stores followed by reloads
through semantic global-memory lowering.

Core Rule: add real prepared global-symbol lowering rules; do not recognize
candidate test names, weaken runtime contracts, or accept BIR/LLVM fallback as
success.

## Read First

- `ideas/open/303_rv64_runtime_defined_global_scalar_memory_foundation.md`
- Existing RV64 prepared emitter owner files from the post-302 layout
- Existing backend runtime cases under `tests/backend/case/`
- Existing CMake/runtime registration for `c4c_add_backend_rv64_runtime_case(...)`

## Current Targets

- `tests/backend/case/global_load.c`
- `tests/backend/case/global_load_zero_init.c`
- `tests/backend/case/global_store.c`
- Prepared RV64 emission surfaces for globals, global loads, and global stores
- Runtime registration only for cases that execute through real RV64 asm under
  qemu

## Non-Goals

- Global arrays, nested aggregates, string globals, aggregate stores, or
  pointer-valued global initializers
- Extern globals, TLS, GOT/PLT-specific support, or object-emission work beyond
  assembler/linker-accepted assembly for this scalar case
- Function-pointer globals or indirect calls
- Varargs, stack arguments, aggregate ABI, dynamic stack, or aggregate
  call/return support
- RV64 runtime test runner contract changes
- Repairing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea owns that work

## Working Model

- Start from the prepared BIR for the three candidate cases and identify the
  available `Global`, `LoadGlobalInst`, `StoreGlobalInst`, and prepared memory
  metadata.
- Keep `emit.cpp` thin. Prefer a narrowly owned prepared global-memory emitter
  or equivalent owner that matches the 301/302 decomposition.
- Emit defined scalar i32 globals into a minimal RV64 assembly data/bss
  representation.
- Use one global-symbol address authority for both loads and stores, such as
  `%pcrel_hi/%pcrel_lo` or an assembler-supported equivalent.
- Preserve fail-closed behavior for unsupported global forms.

## Execution Rules

- Work in small implementation packets tied to one step at a time.
- Register a runtime case only after the real RV64 asm route executes it under
  qemu with the expected exit code.
- After each accepted case, run the focused RV64 runtime proof selected by the
  supervisor.
- Before closure, run focused RISC-V/backend validation and backend-wide
  validation as directed by the supervisor.
- If a candidate case requires arrays, pointer initializers, aggregate layout,
  GOT semantics, or larger object-linker work, stop and open a separate idea
  instead of expanding this plan.

## Steps

### Step 1: Inspect Prepared Global Inputs

Goal: map the existing compiler/backend facts available for the three candidate
global cases.

Primary targets:

- `tests/backend/case/global_load.c`
- `tests/backend/case/global_load_zero_init.c`
- `tests/backend/case/global_store.c`
- Prepared BIR/HIR/debug dumps relevant to RV64 prepared emission

Actions:

- Inspect the prepared BIR emitted for each candidate case.
- Identify how defined initialized and zero-initialized globals are represented.
- Identify the exact load/store instructions and metadata available to the RV64
  prepared route.
- Note unsupported forms encountered without changing expectations.

Completion check:

- `todo.md` records the global representation, load/store instruction facts,
  and the smallest implementation surface needed for Step 2.
- No implementation files or test registrations are changed in this step.

### Step 2: Add Prepared Global-Memory Owner Surface

Goal: create the smallest owner surface for prepared RV64 global memory while
keeping `emit.cpp` thin.

Primary targets:

- RV64 prepared emitter owner files matching the 301/302 layout
- Any narrowly shared prepared emission context support required for symbols

Actions:

- Add or extend a dedicated prepared global-memory owner for global data,
  global load, and global store responsibilities.
- Route from existing prepared emission dispatch into that owner without moving
  unrelated lowering logic.
- Keep unsupported global forms fail-closed with clear diagnostics or existing
  unsupported paths.

Completion check:

- The backend builds.
- `emit.cpp` does not regain prepared lowering ownership.
- No runtime case is registered solely because the owner surface exists.

### Step 3: Emit Defined i32 Global Storage

Goal: emit minimal assembly storage for defined scalar i32 globals.

Primary targets:

- Prepared RV64 global data emission owner
- Assembly output for initialized and zero-initialized i32 globals

Actions:

- Emit initialized scalar i32 globals into an assembler/linker-accepted data
  form.
- Emit zero-initialized scalar i32 globals into a bss/zero-fill form or an
  equivalent accepted representation.
- Reject unsupported global sizes, aggregate globals, arrays, string globals,
  extern globals, and pointer initializers.

Completion check:

- The backend builds.
- Generated RV64 assembly contains semantic global definitions for the candidate
  scalar globals.
- Unsupported global forms still fail closed.

### Step 4: Lower Initialized Global i32 Loads

Goal: make `global_load.c` execute through the real RV64 asm route under qemu
with exit code `11`.

Primary targets:

- Prepared RV64 global load lowering
- Runtime registration for `global_load.c`

Actions:

- Implement symbol-address lowering for i32 global loads using the owner from
  earlier steps.
- Register `global_load.c` with `c4c_add_backend_rv64_runtime_case(...)` only
  after the real RV64 route succeeds.
- Confirm BIR/LLVM fallback output remains rejected by the runtime path.

Completion check:

- `backend_rv64_runtime_global_load` passes under qemu with expected exit code
  `11`.
- The proof covers real RV64 assembly, not fallback output.

### Step 5: Lower Zero-Initialized Global i32 Loads

Goal: make `global_load_zero_init.c` execute through the real RV64 asm route
under qemu with exit code `0`.

Primary targets:

- Prepared RV64 zero-initialized global representation
- Runtime registration for `global_load_zero_init.c`

Actions:

- Ensure zero-initialized scalar globals are addressable by the same symbol
  authority used for initialized globals.
- Register `global_load_zero_init.c` only after qemu execution succeeds through
  the real RV64 route.
- Keep arrays, aggregates, and pointer initializers rejected.

Completion check:

- `backend_rv64_runtime_global_load_zero_init` passes under qemu with expected
  exit code `0`.
- The implementation does not duplicate testcase-specific load logic.

### Step 6: Lower Global i32 Stores

Goal: make `global_store.c` execute through the real RV64 asm route under qemu
with exit code `7`.

Primary targets:

- Prepared RV64 global store lowering
- Runtime registration for `global_store.c`

Actions:

- Implement i32 global store lowering through the same symbol-address authority
  used by global loads.
- Register `global_store.c` only after store/reload execution succeeds under
  qemu.
- Preserve fail-closed behavior for unsupported store targets and global forms.

Completion check:

- `backend_rv64_runtime_global_store` passes under qemu with expected exit code
  `7`.
- Loads and stores share semantic global-address lowering rather than separate
  named-case shortcuts.

### Step 7: Focused Runtime Validation

Goal: prove the accepted RV64 runtime global cases and nearby RV64 runtime
bucket.

Primary target:

- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`

Actions:

- Run the RV64 runtime suite with all registered cases.
- Investigate any new failure before accepting the packet.
- Preserve documented unrelated baseline failures only when the supervisor has
  accepted that baseline.

Completion check:

- The RV64 runtime suite passes with the newly registered global cases, or the
  only remaining failures are explicitly documented accepted baseline failures.

### Step 8: Focused RISC-V and Backend Validation

Goal: validate the feature without hiding regressions outside the runtime
subset.

Primary targets:

- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- `ctest --test-dir build -R '^backend_' --output-on-failure`

Actions:

- Run focused RISC-V/backend route validation.
- Run backend-wide validation before closure.
- Preserve only accepted documented baselines, including the known
  `backend_riscv_prepared_edge_publication` issue when it is unrelated.

Completion check:

- Focused and backend-wide validation results are recorded in `todo.md`.
- Any remaining failure has a clear baseline explanation or a separate idea.

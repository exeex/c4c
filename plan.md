# RV64 Runtime Global Array Address Materialization To Local Pointer Runbook

Status: Active
Source Idea: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md

## Purpose

Extend the qemu-backed RV64 runtime path from direct global array subobject
loads/stores to local pointer values initialized from defined global `i32`
array addresses.

Goal: support materializing a defined global `i32` array address plus constant
subobject offset into a local pointer value, then using that pointer value for
prepared `i32` loads and stores.

## Core Rule

Implement semantic prepared global-address and pointer-value lowering. Do not
match source filenames, global names, local variable names, or fixed instruction
shapes, and do not broaden into pointer-valued global initializers.

## Read First

- Source idea:
  `ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md`
- Prior accepted boundary:
  `ideas/closed/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md`
- Runtime candidate cases:
  `tests/backend/case/defined_global_array_pointer.c`
  `tests/backend/case/defined_global_array_pointer_store.c`
- Boundary probes:
  `tests/backend/case/defined_pointer_global_array.c`
  `tests/backend/case/defined_pointer_global_array_offset.c`
  `tests/backend/case/defined_pointer_global_array_store.c`

## Current Targets

- `defined_global_array_pointer.c`: initialized `int arr[2][2]`, local pointer
  `int *p = &arr[0][0]`, load `*(p + 3)`, expected exit code `7`.
- `defined_global_array_pointer_store.c`: initialized `int arr[2][2]`, local
  pointer `int *p = &arr[0][0]`, store `p[2] = 9`, reload `arr[1][0]`,
  expected exit code `9`.
- Prepared RV64 owner surfaces for global-memory address materialization,
  local pointer slots, frame/context support, and pointer-value memory access.
- Runtime registration only for cases that execute through real RV64 assembly
  under qemu and still reject BIR/LLVM fallback output.

## Non-Goals

- Pointer-valued global initializers such as `int *gp = arr` or
  `int *gp = &arr[1]`.
- Global pointer loads/stores through global data slots.
- Extern globals, TLS, GOT/PLT-specific lowering, relocation-heavy object
  emission, or linker behavior beyond assembler-supported RV64 text.
- Dynamic global array indexing that requires new runtime multiplication or
  address computation beyond already-prepared pointer plus constant offsets.
- Aggregate struct globals, string globals, function-pointer globals, indirect
  calls, varargs, stack arguments, aggregate ABI, dynamic stack, or by-value
  aggregate call/return support.
- Repairing the known `backend_riscv_prepared_edge_publication` baseline
  failure unless a separate idea owns that work.

## Working Model

- Idea 304 already accepted direct global-symbol plus constant-offset `i32`
  array access through the prepared RV64 global-memory owner.
- The missing boundary is local pointer materialization from a direct global
  symbol plus constant subobject offset, followed by pointer-value memory
  access.
- `PointerValue` based loads/stores should be accepted only when the pointer
  value came from the supported local pointer/global-array materialization path.
- Unsupported global/pointer forms must fail closed before producing incomplete
  assembly that can segfault under qemu.
- `emit.cpp` must remain thin; ownership should stay in prepared RV64 global,
  local-memory, frame, or context support files as appropriate.

## Execution Rules

- Work in small packets tied to one step at a time.
- Register a runtime case only after real RV64 assembly executes under qemu
  with the expected exit code.
- Prefer shared semantic helpers for direct global-address materialization and
  pointer-value access over testcase-shaped conditionals.
- Keep `todo.md` as the packet progress and proof log; do not churn this
  runbook for routine executor updates.
- If the route requires pointer-valued global initializers, data-section
  pointer relocation records, GOT/object semantics, or broad local-memory
  rewrites, stop and ask the supervisor to open or activate a separate idea.

## Step 1: Inspect Prepared Pointer-Local Inputs

Goal: map the prepared BIR, addressing metadata, and local pointer home used by
the two candidate cases.

Primary targets:

- `tests/backend/case/defined_global_array_pointer.c`
- `tests/backend/case/defined_global_array_pointer_store.c`
- Prepared BIR and RV64 prepared debug output for global address
  materialization and pointer-value memory access

Actions:

- Inspect the prepared address materialization for `&arr[0][0]`.
- Identify the local pointer slot or value home used to store and reload the
  materialized pointer.
- Inspect the later prepared `PointerValue` based `i32` load/store metadata,
  including pointer value identity and constant byte offsets.
- Compare the input facts with the accepted idea 304 direct global-symbol plus
  constant-offset access path.
- Record unsupported or missing metadata in `todo.md` without changing source
  intent.

Completion check:

- `todo.md` names the exact prepared facts available for the local pointer
  materialization and subsequent pointer-value access, plus the smallest owner
  surface that should change next.
- No implementation files or test registrations are changed in this
  inspection-only step.

## Step 2: Add Direct Global Address Materialization Helper

Goal: create the smallest owner-surface helper needed to materialize a direct
global array address plus constant subobject offset into an RV64 register.

Primary targets:

- Prepared RV64 global-memory owner files
- Narrow frame/context support needed to expose the materialized address to a
  local pointer slot

Actions:

- Reuse the same global-symbol authority that supports idea 303/304 global
  memory access.
- Accept only defined global `i32` array addresses with constant subobject
  offsets that fit the already-supported global-array boundary.
- Emit an assembler-supported RV64 address sequence for the direct global
  symbol plus constant offset.
- Keep unsupported pointer/global forms fail-closed with the existing
  unsupported-route behavior.
- Keep `emit.cpp` out of prepared lowering ownership.

Completion check:

- The backend builds.
- The helper is semantic for direct global address materialization and does not
  reference candidate filenames, symbols, or local variable names.
- Unsupported global pointer initializers remain unsupported.

## Step 3: Store And Reload Materialized Local Pointer Values

Goal: route a materialized direct global array address through the existing
prepared local pointer slot path.

Primary targets:

- Prepared RV64 local-memory/frame/context support
- Pointer local slot store and reload paths used by the candidate cases

Actions:

- Store the materialized address into the prepared local pointer home selected
  by the input metadata.
- Reload the pointer value for subsequent prepared pointer-value memory access.
- Preserve existing scalar local and stack memory behavior.
- Fail closed if the local pointer home cannot be identified or is not a
  supported pointer local slot.

Completion check:

- The backend builds.
- Generated RV64 assembly keeps the pointer value live through the local
  pointer slot path without falling back to BIR/LLVM output or incomplete
  assembly.

## Step 4: Lower Pointer-Value I32 Loads

Goal: make `defined_global_array_pointer.c` execute through real RV64 assembly
under qemu with exit code `7`.

Primary targets:

- Prepared RV64 pointer-value `i32` load lowering
- Runtime registration for `defined_global_array_pointer.c`

Actions:

- Lower `PointerValue` based `i32` loads when the pointer value came from the
  supported direct global array address materialization path.
- Apply the prepared constant byte offset to the pointer value.
- Register `backend_rv64_runtime_defined_global_array_pointer` only after qemu
  execution succeeds through real RV64 assembly with expected exit code `7`.
- Confirm BIR/LLVM fallback output remains rejected by the runtime path.

Completion check:

- `backend_rv64_runtime_defined_global_array_pointer` passes under qemu with
  expected exit code `7`.
- Focused RV64 runtime validation passes for the accepted load case.

## Step 5: Lower Pointer-Value I32 Stores

Goal: make `defined_global_array_pointer_store.c` execute through real RV64
assembly under qemu with exit code `9`.

Primary targets:

- Prepared RV64 pointer-value `i32` store lowering
- Runtime registration for `defined_global_array_pointer_store.c`

Actions:

- Reuse the Step 4 pointer-value identity and offset validation model for
  stores.
- Store through the pointer value and confirm the later direct global reload
  observes the updated array element.
- Register `backend_rv64_runtime_defined_global_array_pointer_store` only
  after qemu execution succeeds through real RV64 assembly with expected exit
  code `9`.
- Preserve fail-closed behavior for unsupported wider forms.

Completion check:

- `backend_rv64_runtime_defined_global_array_pointer_store` passes under qemu
  with expected exit code `9`.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with both local-pointer global-array cases registered.

## Step 6: Probe Global-Pointer Data Initializer Boundary

Goal: document that pointer-valued global initializer cases remain outside this
local-pointer foundation unless a separate idea owns them.

Primary targets:

- `tests/backend/case/defined_pointer_global_array.c`
- `tests/backend/case/defined_pointer_global_array_offset.c`
- `tests/backend/case/defined_pointer_global_array_store.c`

Actions:

- Probe the three boundary cases after Steps 4 and 5 are stable.
- Confirm whether they require pointer-valued global initializers or data
  relocation semantics.
- Do not register them as accepted runtime cases under this idea unless the
  supervisor explicitly approves a scope change backed by source intent.
- Record the observed boundary in `todo.md`.

Completion check:

- `todo.md` records whether the boundary cases remain unsupported and why.
- No pointer-valued global initializer support is silently added under this
  local-pointer idea.

## Step 7: Focused RISC-V Validation

Goal: prove the RV64 local-pointer global-array capability without hiding
nearby RISC-V regressions.

Actions:

- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- Preserve only documented baseline failures, especially the known
  `backend_riscv_prepared_edge_publication` failure.
- Investigate any new failure before claiming acceptance.

Completion check:

- Focused RISC-V/backend route validation either passes or reports only the
  documented accepted baseline.

## Step 8: Backend-Wide Closure Validation

Goal: establish closure confidence for the source idea.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Ensure both required local-pointer global-array runtime cases remain
  accepted under qemu.
- Ensure unsupported pointer/global forms still fail closed and do not fall
  back to BIR/LLVM text or incomplete assembly.

Completion check:

- Backend-wide validation preserves the accepted baseline and the source idea's
  acceptance criteria are satisfied, or any remaining semantic boundary is
  explicitly documented for supervisor review.

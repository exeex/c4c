# RV64 Runtime Defined Global I32 Array Subobject Foundation Runbook

Status: Active
Source Idea: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md

## Purpose

Extend the qemu-backed RV64 runtime path from scalar defined globals to simple
defined global `i32` arrays with constant subobject offsets.

Goal: support loading from initialized global array storage and storing back to
that storage through prepared RV64 assembly.

## Core Rule

Build on the prepared RV64 global-memory owner. Do not reintroduce prepared
lowering into `emit.cpp`, scalar-memory owners, or local-memory owners.

## Read First

- Source idea:
  `ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md`
- Runtime cases:
  `tests/backend/case/defined_global_array.c`
  `tests/backend/case/defined_global_array_store.c`
- Ownership reference only:
  `src/backend/mir/aarch64/codegen/globals.*`

## Current Targets

- `defined_global_array.c`: initialized `int arr[2][2]`, load `arr[1][1]`,
  expected exit code `7`.
- `defined_global_array_store.c`: initialized `int arr[2][2]`, store
  `arr[1][0] = 9`, reload `arr[1][0]`, expected exit code `9`.
- Optional boundary probes only if they follow naturally from the same global
  symbol plus constant-offset rule:
  `defined_global_array_pointer.c` and
  `defined_global_array_pointer_store.c`.

## Non-Goals

- Pointer-valued global initializers such as `int *gp = arr`, unless retained
  only as non-accepted boundary probes.
- Aggregate struct globals, nested aggregate semantics beyond flat `i32` array
  element data, string globals, extern globals, TLS, GOT/PLT-specific lowering,
  object-emission/linker work, function-pointer globals, or indirect calls.
- Dynamic global array indexing requiring runtime multiplication or address
  computation beyond already-prepared constant byte offsets.
- Varargs, stack arguments, aggregate ABI, dynamic stack, or by-value aggregate
  call/return support.
- Repairing the known `backend_riscv_prepared_edge_publication` baseline
  failure.

## Working Model

- RV64 global-memory lowering should remain a capability island owned by the
  prepared global-memory path.
- Accepted runtime tests must execute under qemu and reject BIR/LLVM fallback
  output.
- Unsupported global forms must fail closed.
- Progress must lower global symbol plus constant byte-offset semantics, not
  names such as `arr` or specific source filenames.

## Execution Rules

- Keep each code-changing step small enough to build and run a focused proof.
- Prefer semantic lowering over testcase-shaped matching.
- Register runtime cases only after qemu execution works for the underlying
  capability.
- If work starts pulling in pointer-valued globals, GOT/object semantics, or
  indirect calls, stop and split a separate idea.
- Update `todo.md` with packet progress and proof results; do not churn this
  runbook for routine executor updates.

## Step 1: Inspect Prepared Global Array Inputs

Goal: understand the prepared BIR and metadata shape for the two target cases.

Actions:

- Inspect prepared BIR and prepared memory metadata for
  `defined_global_array.c`.
- Inspect the same artifacts for `defined_global_array_store.c`.
- Identify where scalar defined global `i32` storage and access currently
  succeed.
- Identify the exact constant byte-offset representation that should be
  accepted for array element load/store.
- Record unsupported boundary observations in `todo.md` instead of changing
  source intent.

Completion check:

- The executor can point to the existing scalar-global code path, the missing
  array storage/access behavior, and the narrow helper or owner surface to
  modify next.

## Step 2: Emit Simple I32 Array Global Storage

Goal: extend defined global storage emission from scalar `i32` to simple fixed
`i32` array initializer elements.

Actions:

- Add support in the RV64 prepared global-memory owner or a narrowly named
  helper used by that owner.
- Flatten simple initialized `i32` array elements into emitted storage.
- Add mechanical zero padding only if the existing representation makes it
  straightforward for fixed-size `i32` arrays.
- Fail closed on pointer, aggregate struct, string, extern, TLS, or other
  unsupported global forms.
- Keep `emit.cpp` thin.

Completion check:

- The relevant backend build succeeds.
- Storage emission is semantic for simple `i32` arrays and does not match test
  filenames, symbols, or fixed source shapes.

## Step 3: Allow Constant Offset I32 Global Loads

Goal: generalize scalar-global access checks so prepared global `i32` loads can
use nonzero constant byte offsets.

Actions:

- Route through the existing global-symbol address authority.
- Accept only single-`i32` size/alignment accesses with constant byte offsets.
- Preserve fail-closed behavior for dynamic indexing and unsupported forms.
- Register `backend_rv64_runtime_defined_global_array` only after qemu output
  is real RV64 assembly and the expected exit code is `7`.

Completion check:

- `backend_rv64_runtime_defined_global_array` passes under qemu.
- Focused RV64 runtime validation passes for the accepted load case.

## Step 4: Allow Constant Offset I32 Global Stores

Goal: lower prepared global `i32` stores with nonzero constant byte offsets
through the same global-symbol authority.

Actions:

- Reuse the same offset validation model as Step 3.
- Store to the selected array element and reload through runtime global
  storage.
- Preserve fail-closed behavior for unsupported wider forms.
- Register `backend_rv64_runtime_defined_global_array_store` only after qemu
  output is real RV64 assembly and the expected exit code is `9`.

Completion check:

- `backend_rv64_runtime_defined_global_array_store` passes under qemu.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the newly registered global-array cases.

## Step 5: Probe Optional Pointer-Local Boundary Cases

Goal: decide whether pointer-local array cases are same-rule fallout or a
separate capability boundary.

Actions:

- Probe `defined_global_array_pointer.c` and
  `defined_global_array_pointer_store.c` only after Steps 3 and 4 are stable.
- Accept them only if they require no pointer-valued global initializer or
  broader global-address materialization work.
- If they require a wider capability, document the boundary in `todo.md` and
  leave them out of the accepted runtime set.

Completion check:

- Optional cases are either accepted through the same semantic rule or clearly
  documented as out of scope for a future idea.

## Step 6: Focused RISC-V Validation

Goal: prove the RV64 global-array foundation without hiding regressions in
nearby RISC-V routing.

Actions:

- Run:
  `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
- Preserve only documented baseline failures, especially the known
  `backend_riscv_prepared_edge_publication` failure.
- Investigate any new failure before claiming acceptance.

Completion check:

- Focused RISC-V/backend route validation either passes or reports only the
  documented accepted baseline.

## Step 7: Backend-Wide Closure Validation

Goal: establish closure confidence for the source idea.

Actions:

- Run:
  `ctest --test-dir build -R '^backend_' --output-on-failure`
- Ensure the two required RV64 runtime array cases remain accepted.
- Ensure unsupported global forms still fail closed and do not fall back to
  BIR/LLVM text.

Completion check:

- Backend-wide validation preserves the accepted baseline and the source idea's
  acceptance criteria are satisfied or any remaining semantic boundary is
  explicitly documented for supervisor review.

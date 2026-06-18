# RV64 Runtime Defined Global I32 Array Subobject Foundation

## Goal

Extend the qemu-backed RV64 runtime path from scalar defined globals to simple
defined global `i32` arrays with constant subobject offsets: loading an element
from initialized global array storage and storing an element back to that
storage.

This should build on the prepared RV64 global-memory owner added for idea 303,
not reintroduce prepared lowering into `emit.cpp` or scalar/local-memory
owners.

## Why This Exists

Idea 303 closed the first RV64 global-memory capability island:

- initialized scalar `i32` global storage and loads;
- zero-initialized scalar `i32` global storage and loads;
- scalar `i32` global stores followed by reloads;
- all through qemu-backed real RV64 assembly;
- backend-wide validation preserved only the documented
  `backend_riscv_prepared_edge_publication` baseline failure.

The natural next step is still global memory, but one notch wider: global array
storage and constant element offsets. This keeps the work semantic and small
while deferring pointer-valued global initializers, aggregate globals, extern
globals, GOT-heavy forms, and indirect calls.

The AArch64 backend is useful as a layout reference here: global-memory
lowering has a dedicated owner (`globals.*`) and block dispatch routes
memory-family instructions to it. RV64 should keep following that ownership
shape while staying on the current direct-assembly prepared emitter route.

## Candidate Runtime Cases

Start from existing `tests/backend/case/` files:

1. `defined_global_array.c`
   - initialized `int arr[2][2]`;
   - load `arr[1][1]`;
   - expected exit code `7`.
2. `defined_global_array_store.c`
   - initialized `int arr[2][2]`;
   - store `arr[1][0] = 9`;
   - reload `arr[1][0]`;
   - expected exit code `9`.

Optional boundary probes, only if they fall out naturally from the same global
symbol plus constant-offset rule:

- `defined_global_array_pointer.c`
- `defined_global_array_pointer_store.c`

If those require broader global-address materialization through local pointer
homes, keep them out and document the boundary instead of widening the idea.

## In Scope

- Emit initialized defined global `i32` array storage from BIR global
  initializer elements.
- Support simple zero padding only if the existing global representation makes
  it mechanical for fixed-size `i32` arrays.
- Lower prepared global `i32` loads with nonzero constant byte offsets through
  the existing global-symbol address authority.
- Lower prepared global `i32` stores with nonzero constant byte offsets through
  the same authority.
- Keep the implementation in the prepared RV64 global-memory owner or a
  narrowly named support helper used by that owner.
- Preserve fail-closed behavior for unsupported global forms.
- Register runtime cases only when they execute under qemu and reject BIR/LLVM
  fallback output.

## Out of Scope

- Pointer-valued global initializers such as `int *gp = arr` unless they are
  explicitly kept as non-accepted boundary probes.
- Aggregate struct globals, nested aggregate semantics beyond flat `i32` array
  element data, string globals, extern globals, TLS, GOT/PLT-specific lowering,
  or object-emission/linker work.
- Dynamic global array indexing that requires runtime multiplication/address
  computation beyond already-prepared constant byte offsets.
- Function-pointer globals or indirect calls.
- Varargs, stack arguments, aggregate ABI, dynamic stack, or by-value aggregate
  call/return support.
- Fixing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea owns that repair.

## Suggested Execution Order

1. Inspect prepared BIR and prepared memory metadata for
   `defined_global_array.c` and `defined_global_array_store.c`.
2. Extend global storage emission from scalar `i32` globals to simple `i32`
   array initializer elements, while failing closed on pointer/aggregate/string
   initializers.
3. Generalize the current scalar-global access check to allow nonzero constant
   byte offsets when size/alignment are still a single `i32`.
4. Register and prove `backend_rv64_runtime_defined_global_array`.
5. Register and prove `backend_rv64_runtime_defined_global_array_store`.
6. Probe the optional pointer-local cases only to determine whether they are
   same-rule fallout or a new capability boundary.
7. Run RV64 runtime validation after each accepted case.
8. Run focused RISC-V and backend-wide validation before closure.

If implementation pressure moves toward pointer-valued globals, GOT/object
semantics, or testcase-specific matching, stop and split a new idea.

## Acceptance Criteria

- `backend_rv64_runtime_defined_global_array` executes under qemu with expected
  exit code `7`.
- `backend_rv64_runtime_defined_global_array_store` executes under qemu with
  expected exit code `9`, unless closure documents a real semantic boundary.
- The implementation lowers global symbol plus constant byte-offset semantics,
  not specific source filenames or global names.
- `emit.cpp` remains thin and does not regain prepared lowering ownership.
- Unsupported global forms still fail closed instead of falling back to BIR or
  LLVM text.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the newly registered global-array cases.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- `ctest --test-dir build -R '^backend_' --output-on-failure` is run before
  closure and preserves the accepted baseline.

## Reviewer Reject Signals

- The implementation recognizes `defined_global_array.c`, `arr`, or fixed test
  names instead of lowering prepared global-address metadata.
- Global pointer initializers, extern/TLS/GOT behavior, indirect calls, or
  aggregate ABI sneak into this array foundation.
- Runtime tests accept BIR/LLVM fallback output.
- The diff grows `emit.cpp` or turns the global-memory owner into an
  unbounded mixed-feature pile.
- Tests or expectations are weakened to claim support.

## Notes For The Agent

- Use `src/backend/mir/aarch64/codegen/globals.*` only as an ownership model,
  not as a machine-node architecture to copy wholesale.
- Prefer one accepted runtime case plus a clearly documented boundary over a
  wide, fragile implementation.
- Keep every accepted runtime case backed by qemu execution.

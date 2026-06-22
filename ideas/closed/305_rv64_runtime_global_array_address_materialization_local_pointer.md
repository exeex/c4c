# RV64 Runtime Global Array Address Materialization To Local Pointer

## Goal

Extend the qemu-backed RV64 runtime path to handle a local pointer value that
is initialized from a defined global `i32` array address, then used for
pointer-value based `i32` loads and stores.

This is the follow-up boundary documented by idea 304. It should support local
pointer materialization from a global array subobject without adding
pointer-valued global initializers or broader GOT/object semantics.

## Why This Exists

Idea 304 completed direct global-symbol plus constant-offset array access:

- `defined_global_array.c` executes under qemu with expected exit code `7`;
- `defined_global_array_store.c` executes under qemu with expected exit code
  `9`;
- the implementation lives in the prepared RV64 global-memory owner and keeps
  `emit.cpp` thin;
- backend-wide validation preserved only the documented
  `backend_riscv_prepared_edge_publication` baseline failure.

Its optional pointer-local probes showed a distinct boundary:

- `defined_global_array_pointer.c` lowers `int *p = &arr[0][0]` into a local
  pointer value, then performs a final `i32` load with prepared addressing
  `base=pointer_value`, `pointer=%t4`, `offset=12`.
- `defined_global_array_pointer_store.c` similarly performs the pointer write
  through prepared addressing `base=pointer_value`, `pointer=%t4`, `offset=8`;
  the later direct reload of `arr[1][0]` already uses the accepted
  global-symbol plus constant-offset rule.
- The current RV64 assembly route can emit an incomplete function body for
  these cases; assemble/link can succeed, but qemu segfaults. Treat that as a
  missing lowering boundary, not as runtime success.

The next small capability island is therefore not global pointer data
initializers. It is local pointer materialization from a global symbol plus
constant subobject offset, followed by pointer-value based global memory access.

## Candidate Runtime Cases

Start from existing `tests/backend/case/` files:

1. `defined_global_array_pointer.c`
   - initialized `int arr[2][2]`;
   - local pointer `int *p = &arr[0][0]`;
   - load `*(p + 3)`;
   - expected exit code `7`.
2. `defined_global_array_pointer_store.c`
   - initialized `int arr[2][2]`;
   - local pointer `int *p = &arr[0][0]`;
   - store `p[2] = 9`;
   - reload `arr[1][0]`;
   - expected exit code `9`.

Register accepted cases with `c4c_add_backend_rv64_runtime_case(...)` only when
they execute through real RV64 assembly under qemu and still reject BIR/LLVM
fallback output.

## In Scope

- Recognize prepared address materialization for direct global symbols when the
  result is a local pointer value that can be stored in an existing prepared
  pointer local slot.
- Materialize a defined global `i32` array address plus constant subobject
  offset into an RV64 register using the same global-symbol authority as the
  303/304 global-memory owner.
- Store and reload that pointer through the existing local pointer slot path.
- Lower `PointerValue` based `i32` loads and stores when the pointer value was
  prepared from the supported local pointer/global-array materialization path.
- Keep implementation in owner-oriented files, likely the prepared
  global-memory and local-memory/frame/context support surfaces.
- Preserve fail-closed behavior for unsupported pointer/global forms.

## Out of Scope

- Pointer-valued global initializers such as `int *gp = arr` or `int *gp =
  &arr[1]`.
- Global pointer loads/stores through global data slots.
- Extern globals, TLS, GOT/PLT-specific lowering, relocation-heavy object
  emission, or linker behavior beyond assembler-supported RV64 text.
- Dynamic global array indexing that requires new runtime multiplication or
  address computation beyond already-prepared pointer plus constant offset.
- Aggregate struct globals, string globals, function-pointer globals, indirect
  calls, varargs, stack arguments, aggregate ABI, dynamic stack, or by-value
  aggregate call/return support.
- Fixing the known `backend_riscv_prepared_edge_publication` baseline failure
  unless a separate idea owns that repair.

## Suggested Execution Order

1. Re-inspect prepared BIR and prepared addressing metadata for the two
   candidate cases, using the idea 304 boundary notes as the baseline.
2. Identify the prepared address materialization record for `&arr[0][0]` and
   the local pointer value home used by the subsequent pointer-value access.
3. Add the smallest owner-surface helper needed to materialize a direct global
   address into a register or pointer local slot.
4. Support the pointer-value `i32` load path for
   `defined_global_array_pointer.c` and register/prove it under qemu.
5. Support the pointer-value `i32` store path for
   `defined_global_array_pointer_store.c` and register/prove it under qemu.
6. Probe, but do not accept by default, `defined_pointer_global_array.c`,
   `defined_pointer_global_array_offset.c`, and
   `defined_pointer_global_array_store.c` to document the global-pointer-data
   initializer boundary.
7. Run RV64 runtime validation after each accepted case.
8. Run focused RISC-V and backend-wide validation before closure.

If the route starts requiring pointer-valued global initializers or relocation
records in data sections, stop and open a separate idea rather than widening
this one.

## Acceptance Criteria

- `backend_rv64_runtime_defined_global_array_pointer` executes under qemu with
  expected exit code `7`.
- `backend_rv64_runtime_defined_global_array_pointer_store` executes under qemu
  with expected exit code `9`, unless closure documents a real semantic
  boundary after accepting the load case.
- The implementation lowers prepared direct-global address materialization and
  pointer-value memory semantics, not specific source filenames or global
  names.
- `emit.cpp` remains thin and does not regain prepared lowering ownership.
- Pointer-valued global initializer cases remain unsupported unless a separate
  idea owns them.
- Unsupported global/pointer forms fail closed instead of producing incomplete
  assembly that later segfaults under qemu.
- `ctest --test-dir build -R '^backend_rv64_runtime' --output-on-failure`
  passes with the newly registered local-pointer global-array cases.
- `ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`
  either passes or preserves only documented baseline failures.
- `ctest --test-dir build -R '^backend_' --output-on-failure` is run before
  closure and preserves the accepted baseline.

## Reviewer Reject Signals

- The implementation recognizes `defined_global_array_pointer.c`, `arr`, `p`,
  or fixed instruction shapes instead of prepared global-address and
  pointer-value metadata.
- Pointer-valued global initializers, extern/TLS/GOT behavior, indirect calls,
  or aggregate ABI sneak into this local-pointer foundation.
- Runtime tests accept incomplete assembly, BIR text, LLVM text, or qemu
  crashes as success.
- The diff grows `emit.cpp` or turns a global/local owner into an unbounded
  mixed-feature pile.
- Tests or expectations are weakened to claim support.

## Notes For The Agent

- Closed after Step 8: both local-pointer global-array RV64 runtime cases execute
  under qemu with the expected exit codes, pointer-valued global initializer
  probes remain unsupported and documented out of scope, and backend-wide
  validation preserves only the accepted `backend_riscv_prepared_edge_publication`
  baseline failure.
- Use AArch64 global/address-materialization ownership only as a layout model;
  do not attempt to port its full machine-node architecture under this idea.
- The important boundary is local pointer materialization from a direct global
  symbol. Global pointer data initializers should remain a separate next idea.
- Keep every accepted runtime case backed by qemu execution.

Status: Active
Source Idea Path: ideas/open/388_rv64_object_route_variadic_va_end_boundary.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Implement Narrow RV64 Object Route

# Current Packet

## Just Finished

Steps 3-4 added focused RV64 backend coverage and implemented narrow
object-route no-op lowering for prepared direct extern `llvm.va_end.p0`.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Implementation summary:

- Added `fragment_for_prepared_variadic_va_end` in RV64 object emission. It
  returns an empty `RiscvEncodedFragment` only for the exact prepared direct
  extern `llvm.va_end.p0` shape from Step 2.
- The guard requires `callee == "llvm.va_end.p0"`, an indexed prepared call
  plan for the instruction, matching direct callee name,
  `DirectExternFixedArity`, no indirect/callee-value state, no memory return,
  no outgoing stack argument area, void return, exactly one pointer call arg,
  and exactly one prepared argument.
- Added fail-closed dispatch so malformed `llvm.va_end.p0` calls do not fall
  through to ordinary extern-call relocation emission.
- Did not add fake symbols, linker allowlists, prepared `VaEnd` helper enum
  expansion, same-module sret work, or frame-slot-address changes.

Focused coverage:

- Added a prepared RV64 variadic `va_end` fixture proving exact
  `llvm.va_end.p0` direct extern calls emit no relocation and no external
  `llvm.va_end.p0` symbol.
- Added malformed shape coverage for mismatched prepared callee names and
  two-argument `llvm.va_end.p0`, both rejected before ordinary direct-call
  fallback.

## Suggested Next

Execute Step 5: rerun `tests/c/external/gcc_torture/src/va-arg-13.c` through
the RV64 gcc-torture object runner, record whether it now links/runs or reaches
a later compile/link/runtime boundary, and route any later owner without
expanding this idea silently.

## Watchouts

- Keep idea 386 closed; do not reopen frame-slot-address call argument
  lowering under this plan.
- Keep idea 387 sret work out of scope.
- Do not hide unresolved `llvm.va_end.p0` with fake symbols or linker
  workarounds.
- The current prepared variadic entry plan does not publish `va_end` helper
  operands, so a no-op route must either be justified by exact callee/argument
  facts on the direct extern call or add preparation support for a real
  `va_end` helper classification.
- Do not generalize ordinary `direct_extern_fixed_arity` intrinsic calls unless
  the route is guarded to `llvm.va_end.p0` and the one-argument va_list shape.
- Unsupported variants that remain out of scope: `va_copy`, scalar `va_arg`,
  aggregate `va_arg`, changes to `va_start`, ordinary extern calls, indirect
  calls, direct extern variadic calls, memory-return calls, outgoing stack
  argument calls, malformed `llvm.va_end.p0` calls with missing or multiple
  arguments, mismatched prepared call-plan callee names, and any fake symbol or
  linker allowlist workaround.

## Proof

Ran the supervisor-selected Steps 3-4 proof:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1
```

Result: passed.

Proof log: `test_after.log`

Additional local check before full proof:

```bash
cmake --build --preset default --target backend_riscv_object_emission_test && ./build/tests/backend/mir/backend_riscv_object_emission_test
```

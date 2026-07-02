Status: Active
Source Idea Path: ideas/open/555_rv64_prepared_local_memory_addressing_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Classify Local-Memory Facts

# Current Packet

## Just Finished

Step 1 - Reproduce And Classify Local-Memory Facts completed for
`src/960209-1.c` without semantic repairs.

The failing operation is in function `f`, `block_12`, `inst_index=5`:
`bir.store_local %t68.store.addr, i8 %t66, addr %t67`.

Prepared facts available for that operation:

- load/store kind: `StoreLocalInst`
- value type: `i8`
- stored value: `%t66`
- address base kind: `pointer_value`
- pointer-value base: `%t67`
- byte offset: `0`
- size/alignment: `size=1 align=1`
- base-plus-offset: `yes`
- address-space/volatility: default, non-volatile by prepared access contract
- range/layout facts: `layout_authority=unknown`,
  `range_verdict=unknown_compatible`
- relevant homes: `%t67 value_id=38 kind=register reg=s1`;
  `%t66 value_id=37 kind=register reg=t0`;
  `%t68.store.addr` has a lowering-scratch stack object, but the store's
  prepared address is through pointer `%t67`, not a frame-slot base.

First owner classification: RV64 object-route support for already-published
prepared pointer-value base-plus-offset local-memory facts. The BIR/prepared
producer has already published the required pointer-value base, offset, value
type, and register homes for the failing store.

## Suggested Next

Execute Step 2 by adding generalized RV64 object-route consumption for the
already-published `i8` pointer-value local store shape, preserving the existing
prepared-fact checks instead of inferring from source/testcase shape.

## Watchouts

- Do not infer local-memory addresses from raw target or testcase shape.
- The current first bad fact is not missing prepared producer facts: the
  prepared dump has `access block=block_12 inst_index=5 base=pointer_value
  stored=%t66 pointer=%t67 offset=0 size=1 align=1 base_plus_offset=yes`.
- The object-route diagnostic is emitted after `fragment_for_prepared_store_local`
  fails and the diagnostic's `local_memory_diagnostic` cannot accept the local
  pointer-value shape through the existing object-route support path.
- There is nearby textual asm support for pointer-value `i16`, `i32`, and `f32`
  stores plus frame-slot `i8` stores; avoid a named-case shortcut and implement
  the semantic pointer-value byte store capability.
- Keep idea 547 as broader bucket-review context; do not silently expand this
  route into all local-memory rows.

## Proof

Proof log: `test_after.log`.

Commands run:

```sh
cmake --build --preset default
cmake -DCOMPILER="$PWD/build/c4cll" -DCLANG="$(command -v clang)" -DQEMU_RISCV64="$(command -v qemu-riscv64)" -DSRC="$PWD/tests/c/external/gcc_torture/src/960209-1.c" -DROOT="$PWD/tests/c/external/gcc_torture" -DTARGET_TRIPLE="riscv64-linux-gnu" -DSYSROOT="/usr/riscv64-linux-gnu" -DOUT_CLANG_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/clang.bin" -DOUT_OBJECT="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.o" -DOUT_C4C_BIN="$PWD/build/rv64_gcc_c_torture_backend/src_960209-1.c/c4c.bin" -DCASE_TIMEOUT_SEC="20" -P "$PWD/tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake"
```

Result: build succeeded (`ninja: no work to do`); the one-row RV64 object route
reproduced `[RV64_C4C_OBJ_COMPILE_FAIL]` with
`unsupported_local_memory_access: RV64 object route requires prepared frame-slot
or pointer-value base-plus-offset local memory addressing`.

Inspection command used outside the proof log:
`./build/c4cll -I tests/c/external/gcc_torture --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/960209-1.c`.

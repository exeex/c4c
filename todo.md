Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Lower Pointer-Value I32 Stores

# Current Packet

## Just Finished

Completed plan.md Step 5 by lowering, proving, and registering the RV64
runtime case for prepared pointer-value `i32` stores through a local pointer
slot initialized with a direct global `i32` array address.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `tests/backend/CMakeLists.txt`
- `todo.md`

Semantic basis: prepared RV64 store lowering now accepts `StoreLocalInst`
pointer-value `i32` stores whose prepared memory access is default-address-space
`PointerValue`, has no load result, matches the named stored value when present
or no prepared stored name for immediate values, has a prepared base pointer
register, and uses a nonnegative aligned signed-12-bit base-plus-offset byte
offset. The direct runtime probe emitted `lla t1, arr`, `sd t1, 0(sp)`,
`ld s1, 0(sp)`, `li t1, 9`, `sw t1, 8(s1)`, and a later direct reload from
`arr+8`, then qemu proved expected exit code `9`; only after that proof was
`backend_rv64_runtime_defined_global_array_pointer_store` registered.

## Suggested Next

Continue with plan.md Step 6 only if delegated: probe the pointer-valued global
initializer boundary cases and record whether they remain unsupported because
they require global pointer data slots or relocation semantics outside this
local-pointer idea.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register additional runtime cases until the real RV64 asm route
  executes them under qemu with expected exit codes.
- Nonzero direct-global materialization offsets intentionally remain
  fail-closed in this local-pointer route.
- Step 5 covers local pointer values initialized from direct global array
  addresses; the Step 6 boundary probes should not silently add pointer-valued
  global initializer support under this idea.

## Proof

Manual pre-registration qemu gate passed with expected exit code `9`:

`cmake -DCOMPILER=$PWD/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=$PWD/tests/backend/case/defined_global_array_pointer_store.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_ASM=$PWD/build/backend_rv64_runtime/defined_global_array_pointer_store.manual.s -DOUT_BIN=$PWD/build/backend_rv64_runtime/defined_global_array_pointer_store.manual.bin -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=10 -DEXPECTED_RUN_CODE=9 -P tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Delegated Step 5 proof command passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_rv64_runtime" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; all 30 selected RV64 runtime CTest tests passed,
including `backend_rv64_runtime_defined_global_array_pointer_store` under qemu.
Proof log: `test_after.log`.

Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Lower Pointer-Value I32 Loads

# Current Packet

## Just Finished

Completed plan.md Step 4 by proving and registering the RV64 runtime case for
prepared pointer-value `i32` loads from a local pointer slot initialized with a
direct global `i32` array address.

Changed files:

- `tests/backend/CMakeLists.txt`
- `todo.md`

Semantic basis: existing prepared RV64 lowering accepts a `PointerValue`
prepared memory access whose result is named `i32`, whose base pointer value has
a prepared register home after reloading the local pointer slot, and whose
default-address-space byte offset is aligned, nonnegative, base-plus-offset
addressable, and signed-12-bit encodable. The direct runtime probe emitted
`lla t1, arr`, `sd t1, 0(sp)`, `ld s1, 0(sp)`, and `lw s2, 12(s1)`, then qemu
proved expected exit code `7`; only after that proof was the
`backend_rv64_runtime_defined_global_array_pointer` CTest case registered.

## Suggested Next

Continue with plan.md Step 5 only if delegated: lower prepared pointer-value
`i32` stores through the same supported local pointer slot path, then register
the store runtime case only after qemu proves its expected exit code.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register additional runtime cases until the real RV64 asm route
  executes them under qemu with expected exit codes.
- Nonzero direct-global materialization offsets intentionally remain
  fail-closed in this packet.
- The store candidate remains outside this packet because pointer-value store
  lowering and registration belong to Step 5.
- Step 4 did not require new backend C++ edits because the existing semantic
  prepared lowering already covered the pointer-value load path; this packet
  only registered the runtime case after qemu proof.

## Proof

Manual pre-registration qemu gate passed with expected exit code `7`:

`cmake -DCOMPILER=$PWD/build/c4cll -DCLANG=$(command -v clang) -DQEMU_RISCV64=$(command -v qemu-riscv64) -DSRC=$PWD/tests/backend/case/defined_global_array_pointer.c -DTARGET_TRIPLE=riscv64-linux-gnu -DOUT_ASM=$PWD/build/backend_rv64_runtime/defined_global_array_pointer.manual.s -DOUT_BIN=$PWD/build/backend_rv64_runtime/defined_global_array_pointer.manual.bin -DSYSROOT=/usr/riscv64-linux-gnu -DCASE_TIMEOUT_SEC=10 -DEXPECTED_RUN_CODE=7 -P tests/backend/cmake/run_backend_rv64_runtime_case.cmake`

Delegated Step 4 proof command passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_rv64_runtime_defined_global_array|backend_rv64_runtime_defined_global_array_store|backend_rv64_runtime_defined_global_array_pointer" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; all 5 selected CTest tests passed, including
`backend_rv64_runtime_defined_global_array_pointer` under qemu. Proof log:
`test_after.log`.

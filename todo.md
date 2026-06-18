Status: Active
Source Idea Path: ideas/open/304_rv64_runtime_defined_global_i32_array_subobject_foundation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Probe Optional Pointer-Local Boundary Cases

# Current Packet

## Just Finished

Completed plan.md Step 5 optional boundary probes for
`defined_global_array_pointer.c` and
`defined_global_array_pointer_store.c`.

Classification:
- `defined_global_array_pointer.c` is not same-rule fallout. Prepared BIR
  lowers `int *p = &arr[0][0]; return *(p + 3);` through a local pointer value:
  `bir.store_local %lv.p, ptr %t3`, `%t4 = bir.load_local ptr %lv.p`, then
  `bir.load_local i32 ... addr %t4+12`. Prepared addressing reports
  `base=pointer_value`, not `base=global_symbol`.
- `defined_global_array_pointer_store.c` is also not same-rule fallout for the
  pointer write. Prepared BIR lowers `p[2] = 9` as
  `bir.store_local ... addr %t4+8` with prepared addressing
  `base=pointer_value`. The later `return arr[1][0]` still uses the accepted
  same-rule direct global load: `bir.load_global i32 @arr, offset 8`.
- Both optional cases require broader pointer/global-address materialization:
  the RV64 route would need to produce and preserve a pointer value for
  `&arr[0][0]`, then support pointer-value based global memory access. That is
  outside the current direct global-symbol plus constant-offset rule.

## Suggested Next

Proceed to Step 6 focused RISC-V validation:
`ctest --test-dir build -R 'backend_riscv|backend_codegen_route_riscv64' --output-on-failure`.

## Watchouts

The optional pointer-local cases are useful future coverage for a separate
global-address materialization capability, but registering them now would
expand beyond this plan's accepted direct global-memory rule. Current RV64 asm
emission for both optional cases returns success but writes incomplete assembly
that only contains data plus `main` frame allocation; assemble/link can still
succeed, and qemu then segfaults. Treat that as out-of-scope boundary evidence,
not as a Step 5 implementation target.

## Proof

Inspection/probe only; no code changes and no canonical proof log update
requested for this packet.

Commands run:
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/backend/case/defined_global_array_pointer.c`
  passed; prepared addressing for the final load is `base=pointer_value`
  `pointer=%t4` `offset=12`, not `base=global_symbol`.
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/backend/case/defined_global_array_pointer_store.c`
  passed; prepared addressing for the pointer store is `base=pointer_value`
  `pointer=%t4` `offset=8`, while the following direct reload is
  `base=global_symbol` `symbol=arr` `offset=8`.
- `./build/c4cll --codegen asm --target riscv64-linux-gnu tests/backend/case/defined_global_array_pointer.c -o /tmp/c4c_defined_global_array_pointer.rv64.s`
  passed but emitted incomplete RV64 assembly ending after
  `addi sp, sp, -16`.
- `./build/c4cll --codegen asm --target riscv64-linux-gnu tests/backend/case/defined_global_array_pointer_store.c -o /tmp/c4c_defined_global_array_pointer_store.rv64.s`
  passed but emitted incomplete RV64 assembly ending after
  `addi sp, sp, -16`.
- Manual RV64 runtime helper probes with `/tmp` outputs and `/usr/bin/clang`
  both reached qemu and failed with `BACKEND_RV64_QEMU_UNEXPECTED_RETURN`:
  `defined_global_array_pointer.c` exited `Segmentation fault` instead of `7`,
  and `defined_global_array_pointer_store.c` exited `Segmentation fault`
  instead of `9`.

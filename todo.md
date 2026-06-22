Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Store And Reload Materialized Local Pointer Values

# Current Packet

## Just Finished

Completed plan.md Step 3 verification for routing a materialized direct-global
array address through the prepared local pointer slot.

Observed assembly:

- `defined_global_array_pointer.c` emits `lla t1, arr`, stores it with
  `sd t1, 0(sp)`, reloads the pointer with `ld s1, 0(sp)`, and continues into
  the pointer-value load as `lw s2, 12(s1)`.
- `defined_global_array_pointer_store.c` emits the same `lla`/`sd`/`ld` local
  pointer slot sequence, then stops at the later pointer-value store boundary.

No implementation change was needed for this packet.

## Suggested Next

Continue with plan.md Step 4: lower prepared pointer-value `i32` loads when the
pointer value was reloaded from the supported local pointer slot initialized by
a direct global array address, then register
`backend_rv64_runtime_defined_global_array_pointer` only after qemu proves exit
code `7`.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Nonzero direct-global materialization offsets intentionally remain
  fail-closed in this packet.
- The store candidate still stops after `ld s1, 0(sp)` because pointer-value
  store lowering belongs to Step 5.

## Proof

Delegated Step 3 proof command passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c -o /tmp/defined_global_array_pointer.step3.s && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c -o /tmp/defined_global_array_pointer_store.step3.s && ctest --test-dir build -R "backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_rv64_runtime_defined_global_array|backend_rv64_runtime_defined_global_array_store" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; both Step 3 candidate asm files were written under
`/tmp`; all 4 selected CTest tests passed. Prepared dumps were inspected under
`/tmp/defined_global_array_pointer.step3.prepared.txt` and
`/tmp/defined_global_array_pointer_store.step3.prepared.txt`.

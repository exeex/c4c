Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Direct Global Address Materialization Helper

# Current Packet

## Just Finished

Completed plan.md Step 2 consumer-side direct-global materialization for RV64
prepared local pointer stores.

Implementation:

- Added `emit_riscv_direct_global_address_materialization(...)` in the prepared
  RV64 global-memory owner. It accepts only `DirectGlobal` materializations
  with default address space, direct policy, non-TLS state, a structured symbol
  identity, zero byte offset, and an existing simple defined `i32` global.
- Extended the RV64 prepared local pointer `store_local` consumer to select the
  same-instruction pointer materialization for the stored value and route
  `FrameSlot` through the existing `sp + offset` sequence or `DirectGlobal`
  through the new global helper.
- Candidate RV64 asm now emits `lla t1, arr` followed by `sd t1, 0(sp)` for
  the local pointer slot initialization instead of stopping after the prologue.

Observed boundary:

- `defined_global_array_pointer.c` continues through the pointer reload and
  prepared pointer-value `i32` load, producing `lw s2, 12(s1)`.
- `defined_global_array_pointer_store.c` now gets past the local pointer
  initialization and reload, then stops at the later pointer-value store path
  that belongs to Step 5.

## Suggested Next

Continue with plan.md Step 4: lower prepared pointer-value `i32` loads when the
pointer value was reloaded from the supported local pointer slot initialized by
a direct global array address, then register the load runtime case only after
qemu proves exit code `7`.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Nonzero direct-global materialization offsets intentionally remain
  fail-closed in this packet.
- The later store candidate still stops after `ld s1, 0(sp)` because
  pointer-value store lowering is not part of Step 2.

## Proof

Delegated proof command passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c -o /tmp/defined_global_array_pointer.s && ./build/c4cll --codegen asm --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c -o /tmp/defined_global_array_pointer_store.s && ctest --test-dir build -R "backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_rv64_runtime_defined_global_array|backend_rv64_runtime_defined_global_array_store" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; both candidate asm files were refreshed under `/tmp`;
all 4 selected CTest tests passed.

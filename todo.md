Status: Active
Source Idea Path: ideas/open/305_rv64_runtime_global_array_address_materialization_local_pointer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Direct Global Address Materialization Helper

# Current Packet

## Just Finished

Completed plan.md Step 2 producer-side unblock for zero-offset global-array
pointer values stored into local pointer slots.

Implementation:

- `lower_memory_gep_inst(...)` now publishes resolved zero-offset global GEP
  results as `bir::Value::named_symbol_pointer(...)` whenever the resolved
  address carries a valid `LinkNameId`, independent of target architecture.
- Relative global GEPs from an existing global-pointer SSA now preserve the
  same symbol-pointer identity for zero-offset results.
- Local pointer-slot stores of global-pointer SSA values now use the shared
  symbol-pointer value helper for all targets instead of only AArch64, while
  still requiring zero byte offset and a valid `LinkNameId`.

Observed proof facts:

- `defined_global_array_pointer.c` now dumps semantic BIR as
  `bir.store_local %lv.p, ptr @arr`.
- The prepared dump now includes
  `address_materialization block=entry inst_index=0 kind=direct_global
  result=@arr symbol=arr policy=direct offset=0`.
- `defined_global_array_pointer_store.c` shows the same `ptr @arr` source and
  direct-global address materialization fact for the local pointer store.

## Suggested Next

Continue plan.md Step 2 on the RV64 prepared consumer side: use the now-present
direct-global address materialization fact for `store_local %lv.p, ptr @arr`
to emit the supported RV64 address sequence into the local pointer frame slot,
then fail closed for unsupported nonzero or unresolved global-pointer forms.

## Watchouts

- Do not broaden into pointer-valued global initializers, global pointer data
  slots, GOT/object semantics, dynamic global indexing, or testcase-specific
  matching.
- Do not register runtime cases until the real RV64 asm route executes them
  under qemu with expected exit codes.
- Preserve fail-closed behavior for unsupported pointer/global forms instead
  of accepting incomplete assembly that later segfaults under qemu.
- The pointer-value load/store metadata is already present for `%t4` with
  constant offsets `12` and `8`; this packet only repairs the local pointer
  store source identity and direct-global address-materialization authority.
- The semantic BIR no longer prints `%t3` for the zero-offset global pointer
  source because the value alias collapses it to `@arr`; consumers should rely
  on the `LinkNameId`-backed symbol identity, not the old temporary spelling.

## Proof

Delegated proof command passed and wrote `test_after.log`:

`bash -lc 'set -o pipefail; { cmake --build --preset default && ./build/c4cll --dump-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c >/tmp/defined_global_array_pointer.bir && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer.c >/tmp/defined_global_array_pointer.prepared && ./build/c4cll --dump-prepared-bir --target riscv64-unknown-linux-gnu tests/backend/case/defined_global_array_pointer_store.c >/tmp/defined_global_array_pointer_store.prepared && ctest --test-dir build -R "backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_rv64_runtime_defined_global_array|backend_rv64_runtime_defined_global_array_store" --output-on-failure; } 2>&1 | tee test_after.log'`

Result: build succeeded; BIR/prepared dumps were refreshed under `/tmp`; all
4 selected CTest tests passed.

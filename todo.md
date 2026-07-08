Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Local-Memory Store Production

# Current Packet

## Just Finished

Completed Step 2 producer-boundary tracing for the selected BIR local-memory store rows. Existing case logs under `build/rv64_gcc_c_torture_backend/` show all four selected store probes still fail before prepared/RV64 handoff with `semantic lir_to_bir ... failed in store local-memory semantic family`; focused `--dump-bir --target riscv64-linux-gnu` reruns reproduced the same semantic BIR stop, while `--dump-hir-summary` and `--codegen llvm --target riscv64-linux-gnu` succeeded.

Owning producer path:
- Dispatcher: `src/backend/bir/lir_to_bir/memory/coordinator.cpp` dispatches `LirStoreOp` through `BirFunctionLowerer::lower_scalar_or_local_memory_inst(...)`, and records `store local-memory semantic family` when `lower_memory_store_inst(...)` returns false.
- Store producer: `src/backend/bir/lir_to_bir/memory/local_slots.cpp` `BirFunctionLowerer::lower_memory_store_inst(...)`.
- Store subproducers: dynamic local aggregate store, pointer-provenance store, global-provenance store, then `try_lower_local_slot_store(...)`; pointer/addressed stores are further owned by `src/backend/bir/lir_to_bir/memory/provenance.cpp` `try_lower_pointer_provenance_store(...)` and `try_lower_addressed_pointer_store(...)`.

First missing semantic store facts by selected probe:
- `src/20010605-2.c` (`main`): LLVM route reaches local complex stores including `store fp128 0xL...` into GEP-derived local aggregate fields. BIR lowering reaches the store producer, but `lower_memory_store_inst(...)` cannot lower the `fp128` immediate source value, so no `bir::StoreLocalInst` is emitted for the local `f128` slot/field write.
- `src/20020413-1.c` (`test`): LLVM route reaches ordinary local `fp128` stores, including an immediate `store fp128 0xL...` to `%lv.tmp`. The same first missing fact is an `f128` local `StoreLocalInst` with a materialized `bir::Value::immediate_f128_bits(...)` source.
- `src/pr39120.c` (`bar`): LLVM route reaches `load ptr` from global aggregate field `@x.p`, then `store i32 1, ptr %t1`. The missing store fact is an addressed pointer/local-memory `StoreLocalInst` with pointer provenance for the loaded pointer destination; this is destination/address authority, not the `fp128` source-value case.
- `src/strct-pack-2.c` (`main`): LLVM route reaches a packed local aggregate pointer-array GEP and `store ptr %lv.i, ptr %t5`. The missing store fact is a pointer local-memory store into a selected local aggregate array element with local-slot-address provenance for `%lv.i`.

Step 3 implementation ownership should start at `lower_memory_store_inst(...)` and its BIR store subproducers, not prepared/RV64 materialization. Keep source-value handling (`fp128` immediate stores) separate from destination/address authority (`pr39120.c` and `strct-pack-2.c` pointer stores).

## Suggested Next

Execute Step 3: implement the minimal BIR store producer repair in `lower_memory_store_inst(...)`/owned subhelpers, with the first narrow target being ordinary local `f128` store value materialization and `StoreLocalInst` emission for `src/20010605-2.c` and `src/20020413-1.c`. Treat the pointer/address authority probes (`src/pr39120.c`, `src/strct-pack-2.c`) as same-family store probes that may need a follow-up substep if the `f128` source-value repair does not move them.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/20010605-2.c`, `src/20020413-1.c`, `src/pr39120.c`, and `src/strct-pack-2.c` as probes for semantic producer repair, not as implementation keys.
- The selected non-store guard rows remain excluded from Step 3 implementation ownership: `src/20041124-1.c` (load), `src/20030717-1.c` (GEP), `src/20180921-1.c` (alloca), `src/20000722-1.c` (prepared/RV64 local memory), `src/20000603-1.c` (ABI), `src/pr38533.c` (runtime), `src/20010924-1.c` (prepared/global-data), and `src/20020118-1.c` (RV64/global-data).
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, or accounting behavior.
- Preserve the completed load-semantics boundary from idea 602.

## Proof

No build required for this evidence-only tracing packet. No delegated proof command was run, and no `test_after.log` was generated.

Logs/dumps inspected:
- Existing case logs: `build/rv64_gcc_c_torture_backend/src_20010605-2.c/case.log`, `build/rv64_gcc_c_torture_backend/src_20020413-1.c/case.log`, `build/rv64_gcc_c_torture_backend/src_pr39120.c/case.log`, `build/rv64_gcc_c_torture_backend/src_strct-pack-2.c/case.log`.
- Focused transient dumps under `/tmp`: `c4c_20010605_hir.out`, `c4c_20010605_bir.err`, `c4c_20020413_bir.err`, `c4c_pr39120_bir.err`, `c4c_strct_pack_2_bir.err`, and LLVM route dumps `c4c_20010605.ll`, `c4c_20020413.ll`, `c4c_pr39120.ll`, `c4c_strct_pack_2.ll`.

Exact delegated proof command for the later implementation packet:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_(20010605_2|20020413_1|pr39120|strct_pack_2|20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

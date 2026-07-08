Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Same-Family Breadth

# Current Packet

## Just Finished

Completed Step 3 pointer/address-authority store producer repair for the remaining local-memory store probes.

Implementation surface:
- `try_lower_global_provenance_load(...)` now publishes an opaque runtime `PointerAddress` fact when loading a pointer from a global pointer slot and no stronger address/global fact is known. This gives later stores through that loaded pointer real pointer-value store authority without changing expectations or global-data accounting.
- `lower_memory_store_inst(...)` now lowers dynamic local pointer-array stores by loading each tracked pointer slot, selecting the incoming pointer value for the runtime-selected index and the old value otherwise, and storing the selected pointer back to the concrete local pointer slots. The path clears per-slot address/provenance side tables that cannot represent a runtime-selected element precisely, while preserving pointer value aliases for later value loads.

Progressed rows:
- `build/c4cll --dump-bir --target riscv64-linux-gnu --mir-focus-function main tests/c/external/gcc_torture/src/strct-pack-2.c` now succeeds. The old `store ptr %lv.i, ptr %t5` boundary lowers into selected stores for `%lv.a.8` and `%lv.a.16`.
- `src/pr39120.c` progressed beyond the old `bar` store-through-loaded-pointer producer boundary. After the pointer-field load publication, the next focused stop is the separate `main` aggregate store to global `x` (`store %struct.X %t0, ptr @x`), which is outside this packet's local pointer/address store scope.
- The existing `f128` local-store probes `src/20010605-2.c` and `src/20020413-1.c` still dump BIR successfully after this packet.

The selected non-store guard rows and selected RV64 local-memory rows were included in the delegated CTest proof and stayed green.

## Suggested Next

Lifecycle decision: Step 3 is complete. The remaining `src/pr39120.c` full-file stop is a downstream aggregate-to-global store boundary (`store %struct.X %t0, ptr @x`) and is out of scope for this local-memory store runbook/source idea.

Advance to Step 4 same-family breadth. Use the selected proof rows plus the broader local-memory store family evidence to count which rows progressed beyond the old BIR store producer stop, which rows still expose local-memory store limitations, and which rows now hand off to adjacent owners such as global-data/aggregate stores.

No separate `ideas/open/*` initiative is required yet from this lifecycle decision alone; create one only if the supervisor decides aggregate/global-data store repair should become an active durable route.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/pr39120.c` and `src/strct-pack-2.c` as probes for semantic destination/address repair, not implementation keys.
- Keep ordinary `f128` source-value support separate from pointer/address authority. The `f128` repair lives in generic scalar value lowering and already feeds existing local store production.
- `src/pr39120.c` no longer demonstrates the original `bar` store-through-loaded-pointer producer stop; its remaining visible semantic stop is `main` assigning the returned aggregate to global `x`, which should not be folded into local pointer/address repair without supervisor approval.
- Dynamic local pointer-array stores intentionally preserve pointer values but clear precise per-element address facts because a runtime-selected element cannot be represented by the current `LocalSlotAddress` side table shape.
- The selected non-store guard rows remain excluded from Step 3 implementation ownership: `src/20041124-1.c` (load), `src/20030717-1.c` (GEP), `src/20180921-1.c` (alloca), `src/20000722-1.c` (prepared/RV64 local memory), `src/20000603-1.c` (ABI), `src/pr38533.c` (runtime), `src/20010924-1.c` (prepared/global-data), and `src/20020118-1.c` (RV64/global-data).
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, or accounting behavior.
- Preserve the completed load-semantics boundary from idea 602.

## Proof

Delegated proof passed and was written to `test_after.log`:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(backend_lir_to_bir_notes|llvm_gcc_c_torture_src_(20010605_2|20020413_1|pr39120|strct_pack_2|20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```

Result: build passed; selected CTest subset passed 13/13, including `backend_lir_to_bir_notes` and the 12 selected RV64 rows.

Supervisor-side broader backend validation after the pointer/address repair also passed:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: broader backend CTest subset passed 346/346.

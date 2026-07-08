Status: Active
Source Idea Path: ideas/open/603_bir_local_memory_store_semantics.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace the Store Producer Boundary

# Current Packet

## Just Finished

Completed Step 1 row selection for the BIR local-memory store semantics route. The current scan evidence in `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md` reports `56` local-memory store rows; representative proof rows were selected from the July 8 per-case logs under `build/rv64_gcc_c_torture_backend/`.

Selected store proof rows, all currently expected to fail before the fix with `semantic lir_to_bir ... failed in store local-memory semantic family`:
- `src/20010605-2.c` (`main`)
- `src/20020413-1.c` (`test`)
- `src/pr39120.c` (`bar`)
- `src/strct-pack-2.c` (`main`)

Selected nearby guard rows and current expected owners:
- `src/20041124-1.c`: load guard; currently `load local-memory semantic family`.
- `src/20030717-1.c`: GEP guard; currently `gep local-memory semantic family`.
- `src/20180921-1.c`: alloca guard; currently `alloca local-memory semantic family`.
- `src/20000722-1.c`: prepared/RV64 local-memory guard; currently `unsupported_local_memory_access` requiring prepared frame-slot or pointer-value base-plus-offset local memory addressing.
- `src/20000603-1.c`: ABI/RV64 guard; currently `unsupported_call_abi`.
- `src/pr38533.c`: runtime guard; currently `RV64_BACKEND_RUNTIME_MISMATCH` with c4c aborting where clang exits `0`.
- `src/20010924-1.c`: prepared/global-data guard; currently selected object-data contract `unsupported_but_coherent`.
- `src/20020118-1.c`: RV64/global-data guard; currently cannot emit prepared global symbol.

## Suggested Next

Execute Step 2: trace the selected store proof rows through HIR/BIR dumping, identify the first missing semantic store fact and owning BIR producer path, and keep the guard rows classified as non-store owners.

## Watchouts

- Keep the route limited to BIR local-memory store semantics.
- Treat `src/20010605-2.c`, `src/20020413-1.c`, `src/pr39120.c`, and `src/strct-pack-2.c` as probes for a semantic producer repair, not as implementation keys.
- The load, GEP, alloca, prepared/RV64 local-memory, ABI, runtime, prepared/global-data, and RV64/global-data rows above are guard rows; a store repair should not reclassify them as store progress.
- Do not change tests, expectations, unsupported markers, allowlists, runtime, timeout, or accounting behavior.
- Preserve the completed load-semantics boundary from idea 602.

## Proof

No build required for this row-selection packet; no proof command was run and no `test_after.log` was generated.

Exact delegated proof command for the later implementation packet:

```sh
cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_(20010605_2|20020413_1|pr39120|strct_pack_2|20041124_1|20030717_1|20180921_1|20000722_1|20000603_1|pr38533|20010924_1|20020118_1)_c)$' >> test_after.log 2>&1
```
